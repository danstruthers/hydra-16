# Calling the system

## The jump table

A program calls the system with `jsr` to the call's slot in the jump table, on BIOS ROM page 0 from `$F800`:
`jsr OPEN`.  The slots' addresses are in `hydra.inc` (the assembly SDK) and `hydracalls.h` (C), both made from
`spec/api.def` by the build; a slot never moves once it's published (a call is only ever added to the end of its
group, and a withdrawn one answers `E_NOSYS`), so a program built today runs on later ROMs.

The calls come in groups: **system** (`SYSINFO`, `ERRSTR`, `KMESG`, `REBOOT`, `XCALL`), **task** (`SPAWN`, `EXITS`,
`WAIT`, `SLEEP`, `NOTE`, the environment's `ENV_*`, semaphores `SEM_*` ...), **memory** (`BREAK`, `PAGES_*`,
`BANKS_*`, `SEG_*`), **file** (`OPEN`, `READ`, `WRITE`, `STAT`, `PIPE`, `FD2PATH` ...), **name** (`BIND`,
`MOUNT`, `UNMOUNT`), **cons** (`PUTC`, `PUTS`, `GETC`: fd 1 and fd 0), **time** (`TIME`, `TIME_SET`, `RTC`) and
**server** (for drivers: `SRV_*`, `CLIENT_*`, `IRQ_OWN` ...).  `/rom/doc/api.md` has each one's registers, errors and
whether it waits.

## Registers

| Where | What |
| :---- | :--- |
| `.A`, `.X`, `.Y` | Small arguments and results; a 16-bit value is `.A` (low) and `.X` (high) |
| `r0`-`r15` (`$02`-`$21`) | The call registers: pointers and 16-bit values, two bytes each, low first |
| C | **0: success; 1: failure, with the error code in `.A`.**  Always, for every call |
| `$22`-`$7F` | The program's own zero page: no call touches it |

A call may change `.A`, `.X`, `.Y`, `r0`-`r15` and the flags; keep what you need in `$22`-`$7F` or your RAM.  The
error codes (`E_NOENT` "not found", `E_INTR` "interrupted" ...) are `spec/errors.def`'s; `ERRSTR` gives one's text.

## The SDK's macros

`macros.inc` (assembly):

| Macro | What it does |
| :---- | :----------- |
| `CALL name` | A system call (the same as `jsr name`; it reads as one) |
| `CHECK label` | On to `label` if the call before it failed (C = 1) |
| `LDR reg, value` | A call register = a 16-bit value (an address) |
| `MOVR to, from` | One register (or zero-page word) = another |
| `PRINT label`, `PRINT "text"` | A string to fd 1 |

None defines a label (but unnamed ones), so the code around them keeps its cheap locals.

## A program, walked through

`sdk/asm/samples/hi/hi.s`:

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

`main` gets its arguments at `r0`: strings one after another, each with a zero byte, and an empty one after the
last (the program's name isn't one of them).  It prints `Hello, NAME!` for each:

```
@name:
            PRINT       "Hello, "                           ; "Hello, NAME!"
            MOVR        r0, arg
            CALL        PUTS
            PRINT       s_bang
```

then its task and directory:

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

`rts` from `main` ends it with code 0; `EXITS` ends it anywhere, with a code (`.A`) and a message (`r0`, or 0): its
status, which the parent gets from `WAIT` (and rc as `$status`).

## From C

The C SDK (`sdk/c`) is cc65 with the Hydra's library under the standard one: stdio over fds, `open`/`read`/`write`,
`stat`, `opendir`, `getenv`, `system` (`rc -c`), `signal` over notes, conio over the console's raw mode.  A failed
call returns -1 and sets `errno` (and `_oserror` the system's own code).  Any call is `hy_call (HY_NAME, &regs)`;
`hydra.h` wraps the Hydra's own: `hy_spawn`, `hy_wait`, `hy_note`, `hy_bind`, `hy_banks_alloc`, `hy_sem_*`,
`hy_fd2path` ...  `num.h` calls the number libraries (modules of the paged ROM, by `XCALL`): `num_add`, `num_sqrt`
..., and `printf`'s `%N`.  [../../sdk/c/README.md](../../sdk/c/README.md) is its guide.

## From HyForth, hylang and BASIC

Each call a program makes is a word in HyForth (`sys-open`, in `hydra.fl`: its registers as stack items) and a
function in hylang (`(sys-open "x" 0)`).  `/rom/doc/api.md` shows both forms for each call.  BASIC's `SYS "NAME"`
makes any of them by its name (`.A`, `.X` and `.Y` from its numbers, `r0`-`r15` POKEd before it; `RREG` reads them
after: [../using/basic.md](../using/basic.md)).
