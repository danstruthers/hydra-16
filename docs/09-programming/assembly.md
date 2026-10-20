# Assembly

Programs for the Hydra in 65C02 assembly, with cc65's assembler and linker (`ca65`, `ld65`) on a PC, or with `as` on the
Hydra itself: the same source makes the same program, byte for byte.  This page is the assembly SDK, a program from
source to running, the memory a program has, and the larger forms: modules of the paged ROM and modules of several
banks.

## Contents

1. [The SDK](#the-sdk)
2. [A program](#a-program)
3. [Building it](#building-it)
4. [Running it](#running-it)
5. [A program's memory](#a-programs-memory)
6. [Notes and Ctrl-C](#notes-and-ctrl-c)
7. [The tools' library](#the-tools-library)
8. [Modules of the paged ROM](#modules-of-the-paged-rom)
9. [Modules of several banks](#modules-of-several-banks)
10. [Assembling on the Hydra](#assembling-on-the-hydra)
11. [The source style](#the-source-style)

---

## The SDK

The assembly SDK is `base/sdk/asm` (the core every system shares) and `reborn/sdk/asm` (HydraOS's additions).  The build
copies both, with the samples, to `reborn/bin/sdk/asm`, to take anywhere.

| File | What it is |
| :--- | :--------- |
| `hydra.inc` | The system calls (their addresses), the error codes and the constants.  Made from `base/spec/api.def` by the build and kept in Git: never edit it |
| `hyx2.inc` | The header: `HYX2_PROGRAM "name", main`, `HYX2_DRIVER`, `HYX2_LIBRARY` |
| `hyx2.cfg` | The link for a program in a file: header, code and data from `$0800`, the BSS after them |
| `macros.inc` | `CALL name`, `CHECK label`, `LDR reg, value`, `MOVR to, from`, `PRINT label` or `PRINT "text"`; `CR`, `LF`, `TAB` |
| `toollib.inc`, `toollib.s` | What the system's tools share ([below](#the-tools-library)) |
| `srvlib.inc`, `srvlib.s` | The server library ([servers and drivers](servers.md)) |
| `nslib.s` | A task's default namespace from the namespace file (Plan 9's `newns`) |
| `numbers.inc`, `numlib.s` | The number libraries' calls ([libraries](libraries.md#the-number-libraries)) |
| `asmlib.inc` | The asm library's calls: the disassembler and the assembler ([libraries](libraries.md#the-asm-library)) |
| `samples/` | `hi`, `upper`, `tick`, `counter`, `nsum` |

---

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

* `main` gets `r0` pointing at its **arguments**: zero-terminated strings one after another, an empty one after the last
  (`hello a b`: `"a", 0, "b", 0, 0`).  The program's name is the header's (11 characters at most), not one of them.
* **Returning** ends the task with code 0.  `EXITS` ends it anywhere, with a code (`.A`) and a message (`r0`, or 0): rc's
  `$status` is the message if there is one, else the code.
* **A call** is a `jsr` to its name (`CALL name` says the same).  C = 0 is success; C = 1 is failure, with the error in
  `.A` (`ERRSTR` gives its text); `CHECK label` goes on to label on a failure.
* **Its fds** 0, 1 and 2 are its window's console, or what rc gave it (`<`, `>`, `|`).  `PUTC`, `PUTS` and `GETC` write
  and read them a byte or a string at a time; `OPEN`, `READ`, `WRITE` and `CLOSE` do the rest.  Names go through its
  namespace, as rc's do.
* **Its environment** (`ENV_GET`, `ENV_PUT`) is a copy of rc's: rc's variables, `$window`, `$path`.

---

## Building it

In the repository, a program in a folder of its own (its `.s` files):

```
$ cd reborn
$ node build.js prog path/to/hello          # path/to/hello/hello.hyx
```

With cc65's tools and the SDK alone:

```
$ ca65 --cpu 65C02 -D HYX2_RAM -I sdk/asm -o hello.o hello.s
$ ld65 -C sdk/asm/hyx2.cfg -o hello.hyx hello.o
```

`-D HYX2_RAM` makes a program in a file.  Without it, `hyx2.inc` makes a module of the paged ROM
([below](#modules-of-the-paged-rom)).

**A bank program** is built from the same sources: a file named `bank` in its folder, with `run` in it (or `task`, for
one that must have a task of its own), and `node build.js prog` assembles it with `-D HYX2_BANK`, links it with
`sdk/asm/bank.cfg` (an o65 module at `$8000`, its relocations kept) and puts a copy of its HYX2 header, `HF_BANK` set,
before it in its file.  The shells run a bank program in their own task, kept in a RAM bank for the next time ([the
kernel](../03-base/kernel.md#bank-programs)); in its own task it runs from a bank of that task's.  Its code, data and BSS
fit one bank, 7.5K less its data's size again (a copy, for each run), and it mustn't map a bank at `$8000` itself.  A
program that changes what's its task's (the environment, the namespace, the directory) or needs much RAM (`BREAK`)
should be `task`: in place those are its shell's.

---

## Running it

Copy it to a card, a RAM disk or `/pc`, and type its path (`/sd/0/hello`), or its name if it's in `.` or a `bin` (`$path`
is `(. /bin)`; a card's `/bin` is part of `/bin`).  In the emulator, from `reborn/`:

```
$ node sim/run.js -i --pc-dir path/to/hello         # then: /pc/hello
```

or on a card image:

```
$ node sim/tools/hydrafs.js mkfs card.img 8 MINE
$ node sim/tools/hydrafs.js put card.img path/to/hello/hello.hyx hello
$ node sim/run.js -i --sd card.img                  # then: /sd/0/hello
```

The samples are on HydraOS's card: `/sd/0/sample/hi Ann Bob`, `echo hi | /sd/0/sample/upper`, `/sd/0/sample/nsum 1/3 0.5
2`, `/sd/0/sample/tick` (Ctrl-C stops it).

---

## A program's memory

| Where | What |
| :---- | :--- |
| `$00`, `$01` | Its bank registers: the RAM bank at `$8000`, the paged ROM bank at `$A000` |
| `$02-$21` | `r0`-`r15`, the call registers |
| `$22-$7F` | **Its own zero page** (`.zeropage`): no call touches it |
| `$80-$FF` | The system's (one byte is the program's: `TASK_EVENT`, a server's event count) |
| `$0100-$01FF` | Its stack |
| `$0200-$03FF` | The system's: its fds, name, arguments (`TASK_ARGS`, `$0350`), current directory ... |
| `$0800-` | **Its program**: its header, code, data, then BSS; then its break |
| `-$7FFF` | Pages given out from the top down (`PAGES_ALLOC`) |
| `$8000-$9FFF` | The RAM bank `$00` selects: one of its own, or a shared one |
| `$A000-$DFFF` | The paged ROM bank `$01` selects |
| `$E000-$FFFF` | The BIOS ROM: the jump table at `$F800`; the I/O at `$FF00` (the drivers') |

**More memory:**

* `BREAK` moves the end of its data (as Unix's `brk`); `PAGES_ALLOC` gives runs of 256-byte pages from the top.
* **Banks**: 16 of 8K for each memory card.  `BANKS_ALLOC` gives a run of them; write one's number to `$00`, and it's at
  `$8000`:

```
            lda         #4
            jsr         BANKS_ALLOC                         ; Four banks: .A = the first
            bcs         @nomem
            sta         $00                                 ; The first at $8000
            lda         #$5A
            sta         $8000
```

* **Shared segments** are banks several tasks see: `SEG_CREATE`, `SEG_ATTACH`, then `SEG_MAP` for the values of `U` and
  `$00` ([memory](../03-base/kernel.md#memory)).
* **Semaphores** keep tasks in step: `SEM_NEW`, `SEM_ACQUIRE`, `SEM_RELEASE` ([semaphores](../03-base/kernel.md#semaphores)).

Another task's memory is reachable only through `/proc/N/mem`.

---

## Notes and Ctrl-C

A note (Ctrl-C at its window: `NOTE_INTERRUPT`) ends a program, unless it has a handler.  `NOTIFY` sets one: it's
called with `.A` the note, and returns C = 0 to go on where the program was, C = 1 for the default.
`sdk/asm/samples/tick/tick.s`:

```
main:
            LDR         r0, handler                         ; Its note handler
            jsr         NOTIFY
@second:
            lda         #<TICK_HZ                           ; A second (TICK_HZ ticks): a note ends it sooner
            ldx         #>TICK_HZ
            jsr         SLEEP
            lda         noted
            bne         @end
            ...

; The note handler: .A = the note.  C = 0: the task goes on where it was (here, SLEEP ends early); C = 1 would be the
; default
handler:
            sta         noted
            clc
            rts
```

A call that was waiting when the note came returns `E_INTR`.  A program that runs code in more than one bank keeps its
handler in RAM: the kernel calls it with whatever bank is at `$A000`.

---

## The tools' library

`toollib` is what the system's tools share, so a tool of your own behaves as theirs do.  Include `toollib.inc` at the top
(its zero page) and `toollib.s` at the end; define `tl_name` (the tool's name), `tl_flagset` (the flag letters it takes)
and `tl_usage`.

| Routine | What it does |
| :------ | :----------- |
| `tl_start` | Parses `main`'s arguments: the flags into `tl_flags` (a bit each), `tl_arg` the first argument after them; a bad flag says the usage and ends |
| `tl_next`, `tl_count` | The next argument; how many are left |
| `tl_putc`, `tl_puts`, `tl_space`, `tl_nl`, `tl_dec` | Output, buffered 256 bytes at a time |
| `tl_flush`, `tl_end` | The output written; the end, with `tl_code` |
| `tl_err`, `tl_warn` | `NAME: name: why` and `NAME: message` on fd 2, the status 1 |
| `tl_eachin` | Each input named (none: fd 0) handed to a routine, with `tl_getc` reading it |
| `tl_readdir`, `tl_walk` | A directory read whole; a tree walked, depth first |
| `tl_pcat`, `tl_pcut`, `tl_base` | Paths |
| `tl_atoi`, `tl_div`, `tl_date`, `tl_field` | Numbers, dates, columns |

`samples/upper` is a filter on it: each input's text in capitals.

---

## Modules of the paged ROM

Without `-D HYX2_RAM`, `hyx2.inc` makes a **module**: linked by `modules/module.cfg` to run in place in the paged ROM,
its data copied into its task's RAM (from `$0400`) and its BSS cleared as it starts.  The system's drivers, shells and
languages are modules: they cost no load (a `SPAWN` of one takes about 14 ms) and leave the task nearly all its RAM.
A module of one bank is linked first at `$A000`; then the build finds it a place (`romimg.js`'s plan: bank 0 after
the directory, the free top of another module's bank, or a bank of its own) and links it again at that page, `$A000 +
256 * page`, so small modules share banks.  Its header's `HX_LOAD` says where (`HYX2_LOAD`, the link's
`__ROM_START__`).  A module's code mustn't assume `$A000`: its own labels are where it is.

* **To put one in a ROM**: a folder in `modules/` (`modules/NAME/*.s`), listed in `modules/rom.txt`, and `node build.js`
  builds it with the rest.  `#m/NAME` is then its file, and `#m/bin` (bound at `/bin`) runs it by name.
* **A program** module: `HYX2_PROGRAM "name", main`.
* **A driver**: `HYX2_DRIVER "name", init, serve, irq, stop, flags`, with `HF_BOOT` to start it at boot
  ([servers and drivers](servers.md)).
* **A library**: `HYX2_LIBRARY` ([libraries](libraries.md)).

The module directory (bank 0) lists each module's bank, type, flags, name (10 characters at most) and page, 127
modules at most ([the boot](../03-base/boot.md#the-module-directory)).

---

## Modules of several banks

A module bigger than a bank (16K) spans two to eight, one after another:

* `HYX2_PROGRAM "name", main, 2` (or `HYX2_DRIVER ..., 2`), linked by `modules/module2.cfg` (up to `module8.cfg`).
* Its second bank's code and read-only data are in the segments `CODE2` and `RODATA2`, its third's in `CODE3` and
  `RODATA3`, and so on, all at the same addresses as the first's (`$A000-$DFFF`).
* No bank calls another directly: **`FAR2 routine`** calls a routine in the second bank from the first, **`FAR1 routine`**
  one in the first from the second, and **`FARN bank, routine`** one in any bank from any, the caller's bank set again
  after.  `.A`, `.X`, `.Y` and C pass both ways.  The trampolines are in the module's RAM, and switch its own bank
  register (`$01`); `HYX2_BANKS_INIT`, first thing in `init` or `main`, notes the module's banks.
* **Its note handler, and its interrupt entry** (and what that calls) must be in its RAM (its `DATA`): a note or an
  interrupt may come while any of its banks is at `$A000`.
* Code that every bank calls often can be in `DATA` too, if it calls nothing of any bank's.
* **The build checks it** (`base/tools/bankcheck.js`, from the link's debug information): an instruction in one bank
  that uses a label in another directly (`jsr`, `jmp`, `lda label`) stops the build, as it would run what its own
  bank has at that address.  A directive (`.word label`), an immediate (`#<label`, `LDR`) and the far calls are fine.
  A comment `bankcheck: far NAME` names a macro of the module's own that only takes an address or calls far, and
  `bankcheck: copied` marks a file whose code is copied to RAM and run with another bank in place.

`play` and the storage driver are two banks each, the console driver three, HyBASIC five and hylang eight (rc was
two, its lexer and parser the second, till they fit in one).

---

## Assembling on the Hydra

`as [-bl] file.s [out]` assembles the same source on the Hydra itself ([as](../07-tools.md#as-the-assembler)):

```
/> as -l /sd/0/sample/as/hi.s /ram/hi
/> /ram/hi Ann
Hello, Ann!
I'm task 4, in /, in window 0.
```

`.include` finds the SDK's files in `/lib/as`.  `-l` writes the labels too, which `db` reads.  `dis` turns a program
back into source `as` assembles to the same bytes ([dis](../07-tools.md#dis-the-disassembler)).

---

## The source style

The system's sources keep one style, which yours may follow:

* ca65's syntax, 65C02.  A label at column 1; the mnemonic at column 13; the operand at column 25; a comment at column 61
  (or on its own lines above).  A blank line after a jump or return that ends a block.
* Each file starts with a `; ****` line and a paragraph saying what it is and how it works; each routine with a comment
  saying what it does, its `IN:`, `OUT:` and what it changes.
* `; ---- ` marks the steps of a long routine.
* A macro defines no labels but unnamed ones (`:`), so the cheap locals (`@name`) of the routine using it keep their
  scope.
* Names: `UPPER_SNAKE` for constants and calls; a call `NAME` is implemented by the kernel's `K_NAME`.
