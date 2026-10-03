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
  `spec/errors.def` (each with its text, and the C library's `errno` for it).
* A call may change `.A`, `.X`, `.Y`, `r0`-`r15` and the flags; it never touches `$22`-`$7F`.
* Everything outside the kernel runs with `W = 0` (BIOS ROM page 0 at `$E000`).
* Never edit what's made from `spec/` (`obj/gen/*`, `obj/sdk/hydra.inc`, `obj/sdk/c/hydracalls.h` and
  `oserrmap.inc`): change the specification and build.

## Memory

Every task has its own `$0000`-`$7FFF` (the `T` register selects it) and its own bank registers:

| Where | What |
|---|---|
| `$00`, `$01` | Its RAM bank (`$8000`-`$9FFF`) and paged ROM bank (`$A000`-`$DFFF`) registers |
| `$02`-`$21` | `r0`-`r15`, the call registers |
| `$22`-`$7F` | The program's own zero page: never touched by the system |
| `$80`-`$FF` | The OS zero page (`TK_*` the task's state, `KC_*` kcopy's, `KF_*` the far call's, `K_*` the call stubs' scratch, `TN_*` and `TQ_*` the notes', `F_*` the file calls', `L_*` SPAWN's and the loader's; `$CF`-`$FF` free for the kernel's growth).  One byte of it is a program's to write: its event count, `TASK_EVENT` (`$BD`) |
| `$0100`-`$01FF` | Its stack; a task that isn't running has its frame on top (`U Y W X A P PCL PCH`) |
| `$0200`-`$03FF` | The OS area (`TA_*`): a server's request being served (`TASK_INBOX`), its entries, its break, its note handler, its name, its copy of the IRQ lines' owners, its page and bank maps, the request it's making, the name a request names (`TASK_PATH`), its fds, its current directory, its arguments at `$0350` (`TASK_ARGS`) |
| `$0400`-`$7FFF` | The program's RAM: its data and BSS, then its break (`BREAK`); pages from the top down (`PAGES_ALLOC`).  Task F's top page is the DS1747's |

The kernel task (task 0) keeps the kernel's state: its program zero page (`K0_*`) and its RAM from `$0400`
(`K_*` tables; the environments, 1K a task, from `$3000`).  Every fixed address is in `include/layout.inc`, and nowhere else.

## Tasks

* **States** (`TK_STATE`, the state of the context on top of the task's stack): `FREE`, `READY`, `WAIT` (for a
  `WAKE`), `CALL` (calling another task), `IDLE` (a driver between calls), `NEW`, `SLEEP` (till a tick count),
  `BLOCKED` (waiting to call a busy task), `EVENT` (waiting for a server's event count to change, or a `WAKE`).
  Only `READY` runs; the scheduler makes a `SLEEP` whose time has come, a `BLOCKED` whose task is free, and an
  `EVENT` whose server's count has changed `READY` as it looks for the next task.
* **Waiting** is always the same: set the state, `YIELD`, and look again when back.  A wake for nothing costs a
  look.  A wake that comes before the wait (`TK_WOKEN`) makes the wait return at once.
* **The kernel task** is never preempted (its `TK_PREEMPT` is always at least 1): a `KCALL` runs to its end, so
  the kernel's tables need no locks.  When nothing can run it idles the CPU (`WAI`); it takes a turn in the round
  only when irq entries have queued notes to groups for it to post (`NOTE_QUEUE`).
* **Programs** take the lowest free task (init is task 1); **drivers** the highest (task F first).
* A task's **exit record** (its code and message) waits for its parent's `WAIT`, and the task isn't used again
  till then; a parent that ends first leaves its children and their records to init.

## Reaching other tasks

* **Quick looks**: a moment in another task's memory with `T` switched, IRQs off and no stack use (the stack page
  changes with `T`), then `T` back (`QL_GET`, `QL_PUT`, `K0_GET`, `K0_PUT` in `kernel/kdefs.inc`).  `T` is
  written only by quick looks, the IRQ path, the scheduler, SCALL and kcopy.
* **SCALL** runs a task's serve entry in that task (its zero page, stack and banks); **KCALL** runs a kernel
  routine (on any page) in the kernel task, the routine named in the caller's own zero page.  A task serves one
  call at a time.
* **kcopy** copies between this task's memory and another's, each side as its task sees it.  Its pointer is in
  the partner's zero page, so the partner must be one that can't be in a kcopy of its own: the caller of a call
  being served, the task called, the kernel task, a task not started.  Anything else is read with quick looks.
* **Scratch ownership**: `K_A` and `K_X` are SCALL's (a call changes them); `TK_PICKS` is the scheduler's (it runs
  in the zero page of the task being switched out, at any moment); `K0_*` belong to the KCALL running; an irq
  entry uses its own task's memory.  Nothing that can be preempted keeps a value in another layer's scratch.

## The kernel's pages

* BIOS ROM page 0 has what runs often (the interrupt path, the scheduler, SCALL, kcopy, a task's side of the
  calls that wait); page 1 the kernel task's side of the task calls (the KCALLs, setting a task up, the boot),
  memory, TASKINFO; page 2 files; page 3 namespaces; page 4 POST.  Page 0 is the scarce one.
* The kernel calls a routine on another page with `FARCALL` (the COMMON block's `K_FAR`: `.A`, `.X`, `.Y` and C
  both ways).  A system call on another page is marked `far` in `spec/api.def`: its jump table slot goes to a
  6-byte stub on page 0.
* A system call that waits ends through `K_NOTE_CHECK` (or `K_NOTE_RETURN` with `E_INTR`), at the program's
  return address, so a note that came is taken on the way out.  A far call's stub ends through `K_NOTE_CHECK`
  itself, so a far call may wait too: it returns `E_INTR`, and the stub takes the note.
* Code on page 1 or later reaches page 0's routines by `FARCALL` too (`KPRINT`'s `K_PUTSTR` reads page 0's ROM:
  a page's own strings are printed a byte at a time, as `task.s` and `post.s` do).

## Notes

* A note is taken in the task's own code, never inside the kernel: at the switch into it (when its frame is in
  its own code), or at the end of a system call that waits.  A handler (`NOTIFY`) gets `.A` = the note and returns
  C = 0 to go on, C = 1 for the default; the default ends the task with 128 + the note's Unix number.
* A program's own notes are 16-31.  A kill is never the handler's.

## Interrupts

* One path for every line: a line's vector points at its stub in the COMMON block (the same on all 16 BIOS ROM
  pages), which saves `W` and goes on to the dispatcher, whose main path is in the COMMON block too; the line's
  owner gets the interrupt in its own task, at its irq entry, with the line in `.A`.  The entry answers `.A = 0`,
  or `IRQ_RESCHED` for a task switch.  It runs with IRQs off and never waits.
* VIA timer 2 is a line of its own, `LINE_VIA_T2` (16): the VIA's stub sends its interrupt there.  Owning it is
  owning the timer (one-shot, its interrupt on); the VIA's other registers stay the kernel's.
* **An irq entry has about 85 cycles** of the 200 (the dispatch takes about 115): it wakes clients by adding 1 to
  its event count (`inc TASK_EVENT`), never with `WAKE`, and sends a note to a group with `NOTE_QUEUE`.
* **IRQs off**: no masked stretch of code over 200 cycles anywhere (a character at 115200 is 320 cycles at
  3.58 MHz).  Long work runs in steps with a moment between: `cli`, `nop`, `sei` (or `php`/`sei` ... `plp`).
* A line must be owned (`IRQ_OWN`) before its device interrupts: a line nobody owns is counted and ignored, and
  a level-triggered one comes straight back.

## Modules

* A module is a HYX2 image (`sdk/asm/hyx2.inc`: the 48-byte header, then the code), linked by
  `modules/module.cfg` to run in place at `$A000` in its own paged ROM bank; its data is copied into its task's
  RAM and its BSS cleared before it starts.
* `HYX2_PROGRAM "name", main`: `main` gets `r0` = its arguments (`TASK_ARGS`: zero-terminated strings, an empty
  one after the last); returning is `EXITS` with code 0.
  `HYX2_DRIVER "name", init, serve, irq, stop, flags`: `init` (C = 1 and `.A` = an error ends it), then `serve`
  for its calls (`.Y` = the caller) and `irq` for its lines; `HF_BOOT` starts it at boot.
* The module directory (paged ROM bank 0 at `$A200`, written by `tools/romimg.js`) lists each module's bank, type,
  flags and name, 127 modules at most; `#m/NAME` reads as a module's image, and `#m/bin` lists the programs (bound
  at `/bin`).  A module's data is copied and its BSS cleared by its own task as it starts (`K_TASK_DATA`).
* `SPAWN` takes a path, through the caller's namespace (`/bin/NAME`, `#m/NAME`), and reads the file's HYX2
  header: a module (`HF_INPLACE`) runs in place, found in the module directory by the header's name; any other
  program is a RAM program, read into its task's RAM at its load address by the task itself as it starts (the file
  its fd 15 meanwhile).  The child's fds are the caller's 0, 1 and 2, or with `SPAWN_FDMAP` the map at `r2`; its
  current directory is the caller's, and its environment a copy of the caller's (`SPAWN_NOENV`: an empty one).
* **A task's environment** is 1K of the kernel task's RAM (`K_ENV`, a block a task): its variables, each a name
  and a value of bytes (`ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`; any task's, by number).  `#e` serves the
  caller's as files, mounted at `/env`, as Plan 9's.  rc keeps its variables there, a list's words each ending
  with a zero byte, and its functions as `fn#NAME`.
* A RAM program is assembled with `-D HYX2_RAM` (`hyx2.inc`: no `HF_INPLACE`, loaded at `$0800`) and linked by
  `sdk/asm/hyx2.cfg`: its header, code, read-only data and data one image from `$0800`, its BSS after them.  The
  test RAM programs are `tests/ram/NAME/`, built into `obj/tests/NAME.hyx`; the ROM disk's programs (`/rom/bin`,
  listed in `romfs/romfs.txt`) are `programs/NAME/`, built into `obj/programs/NAME.hyx`; the SDK's samples are
  `sdk/asm/samples/NAME/` (`/rom/sample`); and a program of one's own, anywhere, `node build.js prog DIR`
  (`sdk/asm/README.md`).
* A C program is a folder of `.c` files (and `.s` files, if it has any) in the same places (`sdk/c/samples/NAME/`
  for `/rom/sample/c`), compiled by cc65 for its target `none` and linked by `sdk/c/hydra.cfg` with the C library,
  `obj/sdk/c/hydra.lib`: cc65's `none.lib` with `sdk/c/lib`'s modules in place of cc65's, each named as the module
  it replaces (a cc65 module whose functions the library has under another name is dropped: `build.js`'s
  `CC65_DROPPED`).  cc65's runtime has the zero page from `$22` (26 bytes).  A library routine that C calls may
  change the runtime's scratch (`ptr1`-`ptr4`, `tmp1`-`tmp4`); one that cc65's own assembly calls (`_cputc`,
  `_fgetc`) keeps what that code keeps across it (`ptr1`-`ptr4`, `tmp1`), so it's in assembly, or saves them
  around its C.
* **The tools** (`modules/NAME`, or `programs/NAME` for the ROM disk) are built on `sdk/asm/toollib.s`
  (`toollib.inc` at the top, for its zero page; `toollib.s` at the end), and behave as Plan 9's: flags first
  (`-abc`), then names; a name that fails is said on fd 2 as `tool: name: why` and the rest go on, the tool ending
  with code 1; a write to fd 1 that fails is `tool: write error: why`, and the tool ends with `write error`; a bad
  flag or too few names is `usage: ...`, and the tool ends with `usage`.  A tool's output goes out 256 bytes at a
  time.  What reads files reads fd 0 when it's given none.
* The boot starts the drivers (`HF_BOOT`, task F down, in the directory's order), waits for their inits (their
  devices registered; 2 seconds at most), then starts init.  The system's modules are in `modules/rom.txt`.
* A module bigger than a bank has two (`HYX2_DRIVER ..., 2`, linked by `modules/module2.cfg` when its sources use
  the segment `CODE2`): its second bank's code and read-only data in `CODE2` and `RODATA2`, at the same addresses
  as the first's, reached through `FAR2` (and back through `FAR1`), trampolines in its RAM that switch its own bank
  register (`HYX2_BANKS_INIT` notes its banks).  Such a module owns no IRQ line.  The header's `HX_LENGTH` is the
  image's length in its last bank.

## Files and servers

* A task's fds (`TA_FD`: 16) name channels, the kernel task's table of open files (48); a channel names its
  server, its fid there, its mode and offset.  `DUP` and inheritance share a channel, and its offset.  A child
  gets its parent's fds 0, 1 and 2.
* A request is a block (`RQ_*`, 32 bytes) the client fills in its own `TA_REQ`; the server's serve entry takes it
  into its `TASK_INBOX` (`SRV_TAKE`), moves the data with `CLIENT_READ` and `CLIENT_WRITE`, and answers
  (`SRV_REPLY`).  `READ` and `WRITE` ask for `IO_UNIT` (512) bytes at most a request; a short read ends the
  `READ`, a short write is sent again for the rest.
* **A server never waits.**  When it can't answer yet, it answers `E_AGAIN`, and the kernel has the client wait
  for the server's event count to change from what it was as the server took the request (`RQ_EVENT`), or for a
  `WAKE` (a wait mask's), then sends the request again; a note ends the wait (`R_FLUSH` to the server, `E_INTR`).
  A server adds 1 to its event count (`inc TASK_EVENT`) whenever something its clients may be waiting for has
  happened.
* Servers are built on srvlib (`sdk/asm/srvlib.inc` at the top, `srvlib.s` at the end): a tree of entries
  (directories, text files made on each read, ctl files of commands, data files with a handler, dynamic directories
  whose children a handler makes), the fids, the stat records; a tree a device letter (`SRV_TREES`) for a server of
  several; and a raw device (`SK_RAW`), all of whose requests go to one handler, with fids of its own (a file
  system: `#f`).  Control is text written to ctl files: a command's words after it, the last of them the rest of
  the line.
* The kernel's own devices (`#/`, `#n`, `#t`, `#m`, `#p`, `#|`) are a driver module like any other (`kdev`), not
  the kernel task's.
* **One driver owns the SPI bus and every disk** (`storage`): the SPI devices (`#S`), the cards, the ROM disk and
  the RAM disks (`#d`), and HydraFS on them (`#f`), so a transfer never meets another.  An SPI device is a card's or
  `#S`'s, never both at once (`E_BUSY`).
* **The console is windows, not job control** (Plan 9's way, rio's): each window a whole console (`#cN`), chosen
  for a shell by its namespace (`#cN` at `/dev`).  There's no foreground group and no `fg`: which program gets
  the keys is which window is shown, and a window's interrupts go to its note group.
* `PUTC`, `PUTS` and `GETC` are a write to fd 1 and a read from fd 0; a task without them (the kernel, a driver)
  has the bring-up console, polled.

## Names

* A name is made whole and clean before it's looked up: a relative one after the current directory (`TA_CWD`),
  then `.`, `..` and empty elements gone.  A `#x` name is device `x`'s own, in no namespace; what follows the
  letter, up to the `/`, is its spec, as in Plan 9 (`#c2/cons`: the console's window 2).
* A namespace is a table of mount entries in the kernel task (`kernel/ns.s`); tasks share one till one of them
  changes it, which copies it first.  An entry is one member of the union at a mount point: a device, a spec and
  a path in that device.  Binds are resolved when they're made, as in Plan 9: binding a mount point binds all its
  members; anything else binds the first of its candidates that's there.
* A name's mount point is the longest one it starts with, in whole elements; its candidates are that union's
  members, in order, each with the rest of the name.  `OPEN`, `REMOVE` and the stat calls try them in turn till
  one isn't `E_NOENT`; `CREATE` goes to the `MCREATE` member (or the first).  A directory opened at a mount point
  with more members than one is a union directory: `READ` gives every member's records, one member after another.
* A task's default namespace comes from the namespace file, `/rom/lib/namespace` (and a card's after it), by
  `sdk/asm/nslib.s`'s `ns_default`, Plan 9's `newns`: init's own, and each shell's, which init starts with an empty
  one (`SPAWN_NEWNS`).  `$task` in it is the task, whose own area of the RAM disk (`r/N`) is its `/ram`.

## Source style

* ca65 syntax, 65C02.  Columns: a label at column 1; the mnemonic at column 13; the operand at column 25; a
  comment at column 61 (or on its own lines above).  A blank line after an unconditional jump or return that
  ends a block.
* Each file starts with a `; ****` line and a paragraph saying what it is and how it works; each routine with a
  comment saying what it does, its `IN:`, `OUT:` and what it changes.  Comments are sentences.
* `; ---- ` marks the steps of a long routine, and a switch of `T` (`; ---- The new task`, `; ---- Back`).
* A macro defines no labels but unnamed ones (`:`), so the cheap locals of the routine using it keep their scope;
  a test's strings follow its `jsr` (`tests/mod/testlib.inc`).
* Names: `UPPER_SNAKE` for constants, calls and kernel routines; a call `NAME` is implemented by `K_NAME`, and
  its kernel-task half (a KCALL) by `K_NAME_K`; cheap locals (`@name`) inside a routine.  Prefixes: `TK_` (OS
  zero page), `TA_` (OS area), `K_` (kernel task's tables, or call scratch), `K0_` (kernel task's zero page),
  `KC_` (kcopy), `KF_` (the far call), `HX_`/`HT_`/`HF_` (the module header), `MD_`/`ME_` (the module directory),
  `E_` (errors), `ST_` (states), `TI_` (TASKINFO's answer), `NOTE_` (notes), `RQ_`/`R_`/`O_`/`SR_` (requests, open
  modes, stat records), `F_` (the file calls' scratch), `srv_` (srvlib), `P_` (POST), `m_`/`n_`/`f_`/`t_` (mem.s,
  notes.s, file.s and task.s's own helpers).
* Text files have CRLF line endings in the working copy (Git stores LF).
