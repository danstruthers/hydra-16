# The assembly SDK

Programs for the Hydra-16 in 65C02 assembly, with the cc65 tools (`ca65`, `ld65`).  A program is a HYX2 file: a
48-byte header, then its code and data, which `SPAWN` reads into a task's RAM at `$0800` and starts.  Put it on a
card or a RAM disk, and rc runs it by its path, or by its name from `.` or `/bin`.

The SDK is this folder; `node build.js` also copies it, with the generated `hydra.inc` and the samples, to
`bin/sdk/asm`, to take elsewhere.

| File | What it is |
|---|---|
| `hydra.inc` | The system calls (their addresses in the jump table), the error codes and the constants.  Made from `spec/api.def` by the build (`obj/sdk/hydra.inc`); never edit it |
| `hyx2.inc` | The header: `HYX2_PROGRAM "name", main` |
| `hyx2.cfg` | The link for a program in a file: header, code and data from `$0800`, the BSS after them |
| `macros.inc` | `CALL name` (a system call), `CHECK label` (on to label if the call failed), `LDR reg, value` (a call register = a 16-bit value), `MOVR to, from` (one register = another), `PRINT label` or `PRINT "text"` (a string to fd 1); `CR`, `LF`, `TAB` |
| `toollib.inc`, `toollib.s` | What the system's tools share: flags, errors and exit statuses as Plan 9's, buffered output, input a file at a time, directories, paths, numbers (the comment at its top lists them) |
| `srvlib.inc`, `srvlib.s` | A file server's library (the system's drivers use it) |
| `nslib.s` | A task's default namespace, from the namespace file (Plan 9's `newns`) |
| `samples/` | `hi` (arguments, task, directory, environment), `upper` (a filter on `toollib`), `tick` (a note handler), `counter` (a server: a driver, a module that runs in place, on `srvlib`) |

The calls are described in `/rom/doc/api.md` on the Hydra (the build's `obj/gen/api.md`), and the rules the
system keeps in `docs/conventions.md`.

## A program

```
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "hello", main

.code
main:                                       ; r0: the arguments
            PRINT       "Hello"
            lda         #LF
            CALL        PUTC
            lda         #0                  ; (Returning: EXITS with code 0)
            rts
```

* `main` gets `r0` pointing at its arguments: zero-terminated strings one after another, an empty one after the
  last (`hello a b`: `"a", 0, "b", 0, 0`).  Its name is the header's (11 characters at most).
* Returning ends the task with code 0.  `EXITS` ends it with a code (`.A`) and a message (`r0`, or 0): rc's
  `$status` is the message if there is one, else the code (true is 0).
* A call is a `jsr` to its name (`CALL name` says the same).  Arguments and results are `.A`, `.X`, `.Y` and the
  call registers `r0`-`r15` (`$02`-`$21`); a call may change all of them.  C = 0 is success; C = 1 is failure,
  with the error in `.A` (`ERRSTR` gives its text: `"not found"`); `CHECK label` goes on to label on a failure.
* The program's own zero page is `$22`-`$7F` (`.zeropage`: no call touches it); its RAM is `$0800` up to its
  break (`BREAK` moves it), and pages above (`PAGES_ALLOC`), to `$7FFF`.  It has its own RAM banks at
  `$8000`-`$9FFF` too (16 a RAM module: `BANKS`), and its stack.
* Its fds 0, 1 and 2 are its window's console, or what rc gave it (`<`, `>`, `|`); `PUTC`, `PUTS` and `GETC`
  write and read them a byte or a string at a time; `OPEN`, `READ`, `WRITE` and `CLOSE` do the rest.  Names go
  through its namespace, as rc's do.
* A note (Ctrl-C at its window: `NOTE_INTERRUPT`) ends it, unless it has a handler (`NOTIFY`: `tick`).
* Its environment (`ENV_GET`, `ENV_PUT`) is a copy of rc's: rc's variables, `$window`, `$path` ...

## Building it

In this repository, a program in a folder of its own (its `.s` files):

```
node build.js prog path/to/hello          # path/to/hello/hello.hyx
```

With the cc65 tools and `bin/sdk/asm` alone:

```
ca65 --cpu 65C02 -D HYX2_RAM -I sdk/asm -o hello.o hello.s
ld65 -C sdk/asm/hyx2.cfg -o hello.hyx hello.o
```

(`-D HYX2_RAM`: a program in a file.  Without it, `hyx2.inc` makes a module of the paged ROM, run in place, linked
with `module.cfg` (`module2.cfg` to `module4.cfg` for one of two to four banks); those are the system's, built into the ROM with
`modules/rom.txt`.  A server is one: a
driver, `HYX2_DRIVER`, which registers a device letter and answers its clients' requests through `srvlib`.  The
sample `counter` is one; the build makes it a module, and the tools test puts it in its ROM.  A module may have
library modules of its own, `HYX2_LIBRARY`, linked by its own config: `docs/conventions.md`.)

## Running it

Copy it to a card, or to the RAM disk, and type its path (`/sd/0/hello`), or its name if it's in `.` or a `bin`
(`$path` is `(. /bin)`; a card's `/bin` is part of `/bin`).  In the emulator, from `reborn/`:

```
node ../sim/tools/hydrafs.js mkfs card.img 8 MINE           # an empty card image, 8 MB
node ../sim/tools/hydrafs.js put card.img path/to/hello/hello.hyx hello
node sim/run.js -i --sd card.img                            # then, at the % prompt: /sd/0/hello
```

The samples are on the ROM disk: `/rom/sample/hi Ann Bob`, `echo hi | /rom/sample/upper`, `/rom/sample/tick`
(Ctrl-C to stop it).
