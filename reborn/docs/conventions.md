# Conventions

The rules every source in `reborn/` follows, so the whole system reads the same way and a program written for
one part works with every other.  The reasons are in the plan
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), §7-§9); these are
the rules as built.

## Calling the system

* A program calls the system with `jsr` to the call's slot in the jump table (`$F800` up, on BIOS ROM page 0),
  by name from `hydra.inc`.  The slots come from `spec/api.def` and never move: a call is only ever added to the
  end of its group, and a withdrawn call keeps its slot and answers `E_NOSYS`.
* Arguments and results: `.A`, `.X`, `.Y` and the call registers `r0`-`r15` (`$02`-`$21`).  A 16-bit value is
  `.A` (low) and `.X` (high), or a call register.
* **C = 0 is success; C = 1 is failure, with the error code in `.A`.  Always**, for every call, with the codes of
  `spec/errors.def`.
* A call may change `.A`, `.X`, `.Y`, `r0`-`r15` and the flags; it never touches `$22`-`$7F`.
* Everything outside the kernel runs with `W = 0` (BIOS ROM page 0 at `$E000`).
* Never edit what's made from `spec/` (`obj/gen/*`, `obj/sdk/hydra.inc`): change the specification and build.

## Memory

Every task has its own `$0000`-`$7FFF` (the `T` register selects it) and its own bank registers:

| Where | What |
|---|---|
| `$00`, `$01` | Its RAM bank (`$8000`-`$9FFF`) and paged ROM bank (`$A000`-`$DFFF`) registers |
| `$02`-`$21` | `r0`-`r15`, the call registers |
| `$22`-`$7F` | The program's own zero page: never touched by the system |
| `$80`-`$FF` | The OS zero page (`TK_*` the task's state, `KC_*` kcopy's, `K_*` the call stubs' scratch; `$A2`-`$FF` free for the kernel's growth) |
| `$0100`-`$01FF` | Its stack; a task that isn't running has its frame on top (`U Y W X A P PCL PCH`) |
| `$0200`-`$03FF` | The OS area (`TA_*`): the request block, its entries, its name, its copy of the IRQ lines' owners, its arguments at `$0300` |
| `$0400`-`$7FF7` | The program's RAM: its data and BSS (task F's top 8 bytes are the DS1747's registers) |

The kernel task (task 0) keeps the kernel's state: its program zero page (`K0_*`) and its RAM from `$0400`
(`K_*` tables).  Every fixed address is in `include/layout.inc`, and nowhere else.

## Tasks

* **States** (`TK_STATE`, the state of the context on top of the task's stack): `FREE`, `READY`, `WAIT` (for a
  `WAKE`), `CALL` (calling another task), `IDLE` (a driver between calls), `NEW`, `SLEEP` (till a tick count),
  `BLOCKED` (waiting to call a busy task).  Only `READY` runs; the scheduler makes a `SLEEP` whose time has come,
  and a `BLOCKED` whose task is free, `READY` as it looks for the next task.
* **Waiting** is always the same: set the state, `YIELD`, and look again when back.  A wake for nothing costs a
  look.
* **The kernel task** is never preempted (its `TK_PREEMPT` is always at least 1): a `KCALL` runs to its end, so
  the kernel's tables need no locks.  When nothing can run it idles the CPU (`WAI`).
* **Programs** take the lowest free task (init is task 1); **drivers** the highest (task F first).
* A task's **exit record** (its code and message) waits for its parent's `WAIT`, and the task isn't used again
  till then; a parent that ends first leaves its children and their records to init.

## Reaching other tasks

* **Quick looks**: a moment in another task's memory with `T` switched, IRQs off and no stack use (the stack page
  changes with `T`), then `T` back (`QL_GET`, `QL_PUT`, `K0_GET`, `K0_PUT` in `kernel/kdefs.inc`).  `T` is
  written only by quick looks, the IRQ path, the scheduler, SCALL and kcopy.
* **SCALL** runs a task's serve entry in that task (its zero page, stack and banks); **KCALL** runs a kernel
  routine in the kernel task.  A task serves one call at a time.
* **kcopy** copies between this task's memory and another's, each side as its task sees it.
* **Scratch ownership**: `K_A` and `K_X` are SCALL's (a call changes them); `TK_PICKS` is the scheduler's (it runs
  in the zero page of the task being switched out, at any moment); `K0_*` belong to the KCALL running; an irq
  entry uses its own task's memory.  Nothing that can be preempted keeps a value in another layer's scratch.

## Interrupts

* One path for every line: a line's vector points at its stub in the COMMON block (the same on all 16 BIOS ROM
  pages), which saves `W` and comes to the dispatcher on page 0; the line's owner gets the interrupt in its own
  task, at its irq entry, with the line in `.A`.  The entry answers `.A = 0`, or `IRQ_RESCHED` for a task
  switch.  It runs with IRQs off and never waits.
* **IRQs off**: no masked stretch of code over 200 cycles anywhere (a character at 115200 is 320 cycles at
  3.58 MHz).  Long work runs in steps with a moment between: `cli`, `nop`, `sei` (or `php`/`sei` ... `plp`).
* A line must be owned (`IRQ_OWN`) before its device interrupts: a line nobody owns is counted and ignored, and
  a level-triggered one comes straight back.

## Modules

* A module is a HYX2 image (`sdk/asm/hyx2.inc`: the 48-byte header, then the code), linked by
  `modules/module.cfg` to run in place at `$A000` in its own paged ROM bank; its data is copied into its task's
  RAM and its BSS cleared before it starts.
* `HYX2_PROGRAM "name", main`: `main` gets `r0` = its arguments; returning is `EXITS` with code 0.
  `HYX2_DRIVER "name", init, serve, irq, stop, flags`: `init` (C = 1 and `.A` = an error ends it), then `serve`
  for its calls (`.Y` = the caller) and `irq` for its lines; `HF_BOOT` starts it at boot.
* The module directory (paged ROM bank 0 at `$A200`, written by `tools/romimg.js`) lists each module's bank, type,
  flags and name.  `SPAWN "#m/NAME"` starts a program from it.

## Source style

* ca65 syntax, 65C02.  Columns: a label at column 1; the mnemonic at column 13; the operand at column 25; a
  comment at column 61 (or on its own lines above).  A blank line after an unconditional jump or return that
  ends a block.
* Each file starts with a `; ****` line and a paragraph saying what it is and how it works; each routine with a
  comment saying what it does, its `IN:`, `OUT:` and what it changes.  Comments are sentences.
* `; ---- ` marks the steps of a long routine, and a switch of `T` (`; ---- The new task`, `; ---- Back`).
* Names: `UPPER_SNAKE` for constants, calls and kernel routines; a call `NAME` is implemented by `K_NAME`, and
  its kernel-task half (a KCALL) by `K_NAME_K`; cheap locals (`@name`) inside a routine.  Prefixes: `TK_` (OS
  zero page), `TA_` (OS area), `K_` (kernel task's tables, or call scratch), `K0_` (kernel task's zero page),
  `KC_` (kcopy), `HX_`/`HT_`/`HF_` (the module header), `MD_`/`ME_` (the module directory), `E_` (errors), `ST_`
  (states).
* Text files have CRLF line endings in the working copy (Git stores LF).
