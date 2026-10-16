# The system calls

How a program asks the kernel for something: the jump table, the registers, how errors come back, what each group of
calls is for, and how to make a call from each language.  [The call reference](../09-programming/calls-reference.md)
lists every call in full.

## Contents

1. [The jump table](#the-jump-table)
2. [Registers](#registers)
3. [Errors](#errors)
4. [The groups](#the-groups)
5. [A program, walked through](#a-program-walked-through)
6. [From each language](#from-each-language)
7. [Stability: the ABI](#stability-the-abi)

---

## The jump table

A program calls the system with **`jsr` to the call's slot** in the jump table, on BIOS ROM page 0 from `$F800`:

```
            jsr         OPEN                                ; ($F8C0)
```

Each slot is 3 bytes, a `jmp` to the kernel's routine (or, for a call whose routine is on another page, to a stub that
makes the far call).  The names and addresses are in **`hydra.inc`** for assembly and **`hydracalls.h`** for C, both
made from `base/spec/api.def` by the build.

Programs always run with BIOS page 0 in place (`W = 0`), so the table is always there.

---

## Registers

| Where | What |
| :---- | :--- |
| `.A`, `.X`, `.Y` | Small arguments and results.  A 16-bit value is `.A` (low) and `.X` (high) |
| `r0`-`r15` (`$02-$21`) | The **call registers**: pointers and 16-bit values, two bytes each, low first.  A 32-bit value is two of them, `r0/r1` (`r0` the low half) |
| C | **0: success.  1: failure, with the error code in `.A`.**  Always, for every call |
| `$22-$7F` | The program's own zero page: no call touches it |

A call may change `.A`, `.X`, `.Y`, `r0`-`r15` and the flags: keep what you need in `$22-$7F` or in your RAM.  Strings
are zero-terminated, passed by address in a call register.

---

## Errors

A call that fails returns **C = 1 and an error code in `.A`**: `E_NOENT` (`$20`, "not found"), `E_INTR` (`$05`,
"interrupted"), `E_AGAIN` (`$04`, "not yet: try again") and so on, from `base/spec/errors.def`.  `ERRSTR` turns a code
into its text.  The usual pattern:

```
            jsr         OPEN
            bcs         @error                              ; .A = the error code
            sta         fd
            ...
@error:
            LDR         r0, buffer                          ; Its text, for a message
            jsr         ERRSTR
```

Three errors come from the kernel's own machinery, not the call's purpose:

* **`E_INTR`**: a note came while the call waited.  The call stopped, and the note is taken (its handler runs, or the
  default ends the task) as the call returns.
* **`E_AGAIN`**: the file was opened `O_NONBLOCK` and there's nothing yet.
* **`E_NOSYS`**: no such call (a slot not yet used, or withdrawn).

[The reference](../09-programming/calls-reference.md#the-error-codes) has every code.

---

## The groups

The table is in groups, each with room to grow:

| Group | Slots | What's there |
| :---- | :---- | :----------- |
| **system** | `$F800`, 16 | `SYSINFO` (the ABI, the memory cards, the free tasks), `ERRSTR`, `KMESG` (the kernel's messages), `REBOOT`, `XCALL` (call a library in another bank) |
| **task** | `$F830`, 32 | `SPAWN`, `EXEC`, `EXITS`, `WAIT`, `GETPID`, `GETPPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`/`ON`, `TASKINFO`, `TASKREAD`, `NOTIFY`, `NOTE`, `MODINFO`, the environment (`ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`), semaphores (`SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `SEM_WAIT`) |
| **memory** | `$F890`, 16 | `BREAK`, `PAGES_ALLOC`/`FREE`, `BANKS`, `BANKS_ALLOC`/`FREE`/`ALLOC_IN`, shared segments (`SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `SEG_CREATE_IN`) |
| **file** | `$F8C0`, 32 | `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH` |
| **name** | `$F920`, 16 | `BIND`, `MOUNT`, `UNMOUNT` (of one thing, or of everything: an empty namespace), `NSINFO` |
| **cons** | `$F950`, 16 | `PUTC`, `PUTS`, `GETC`, `PUTHEX`: a byte or a string to fd 1, a byte from fd 0 |
| **time** | `$F980`, 16 | `TIME`, `TIME_SET`, `RTC` |
| **server** | `$F9B0`, 32 | For drivers: `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP` (a driver's wait between turns of its own work), `INIT_RUN` (init started again) |
| **dbg** | `$FA10`, 16 | Debugging and tests: unstable, may change in any version |

The [kernel](kernel.md) and [files](files.md) pages explain what the calls do; the reference has each one's registers.

---

## A program, walked through

`sdk/asm/samples/hi/hi.s`, a program that greets each of its arguments, then says its task and directory:

```
.include "hydra.inc"                                        ; The calls and constants (made from spec/api.def)
.include "hyx2.inc"                                         ; The header: HYX2_PROGRAM
.include "macros.inc"                                       ; LDR, MOVR, PRINT, CALL, CHECK

            HYX2_PROGRAM "hi", main                         ; Its name, and where it starts

.zeropage
arg:        .res        2                                   ; An argument (its zero page: $22-$7F)

.bss
cwd:        .res        PATH_MAX + 1                        ; Its current directory
```

`main` gets its arguments at `r0`: strings one after another, each with a zero byte, and an empty one after the last
(the program's name isn't one of them).  For each it prints `Hello, NAME!`:

```
@name:
            PRINT       "Hello, "                           ; "Hello, NAME!"
            MOVR        r0, arg
            CALL        PUTS
            PRINT       s_bang
```

then its task and its directory:

```
            CALL        GETPID                              ; .A = this task (0-15)
            ...
            LDR         r0, cwd
            CALL        GETCWD                              ; Its current directory, into cwd
            PRINT       cwd
```

and a variable of its environment, which may not be there:

```
            LDR         r0, s_window                        ; Its window, if its environment has one
            LDR         r1, window
            LDR         r2, 7
            stz         r3
            stz         r3 + 1
            lda         #$FF                                ; (This task's)
            CALL        ENV_GET
            CHECK       @end                                ; (Not there: C = 1, .A = E_NOENT)
```

`rts` from `main` ends it with code 0; `EXITS` ends it anywhere, with a code and a message: its **status**, which its
parent gets from `WAIT` (and rc shows as `$status`).

**The SDK's macros** (`macros.inc`):

| Macro | What it does |
| :---- | :----------- |
| `CALL name` | A system call (the same as `jsr name`, but it reads as one) |
| `CHECK label` | On to `label` if the call before it failed (C = 1) |
| `LDR reg, value` | A call register = a 16-bit value (an address, a number) |
| `MOVR to, from` | One call register (or zero-page word) = another |
| `PRINT label`, `PRINT "text"` | A string to fd 1 |

None defines a label (but unnamed ones), so the code around them keeps its cheap locals (`@name`).

---

## From each language

Every call a program makes is reachable from every language on the Hydra:

| Language | How | Example |
| :------- | :-- | :------ |
| Assembly | `jsr NAME`, with `hydra.inc` | `jsr GETPID` |
| C | `hy_call (HY_NAME, &regs)`, or `hydra.h`'s wrappers, or the standard library over them (`open`, `read`, `printf`) | `hy_task ()` |
| HyForth | `sys-name`, its registers on the stack (`hydra.fl`) | `sys-getpid .` |
| hylang | `(sys-name ...)`, its arguments by kind | `(sys-getpid)` |
| HyBASIC | `SYS "NAME", a, x, y`: `r0`-`r15` POKEd first (bytes 2-33); `RREG a, x, y, p` reads the registers after | `SYS "GETPID": RREG A, X, Y, P` |

[The reference](../09-programming/calls-reference.md) shows each call's HyForth and hylang forms; [the C
guide](../09-programming/c.md) how C reaches them.

---

## Stability: the ABI

The calls are the kernel's promise to programs.  **A slot never moves once it's published**: a new call is only ever
added at the end of its group, and a withdrawn call keeps its slot and answers `E_NOSYS`.  So a program built against
`hydra.inc` today runs on later kernels.  The banner and `SYSINFO` give the **ABI version** (1); it changes only if that
promise has to be broken.

The `dbg` group is excepted: its calls are for the tests and may change in any version.
