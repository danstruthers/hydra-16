# 9. The programming guide

How to write programs of your own for the Hydra-16: in assembly or C, as programs in files or modules in the ROM, as
servers that make devices of their own, and with the system's libraries.  It builds on [chapter 3](../03-base/README.md)
(the kernel and its calls); [the call reference](calls-reference.md) is every call in full.

## Contents

1. [Assembly](assembly.md): the assembly SDK, a program walked through, building and running it, the memory a program
   has, modules of the paged ROM, modules of several banks, the tools' library.
2. [C](c.md): the C SDK (cc65), the standard library on the Hydra, the Hydra's own calls (`hydra.h`), sound, the screen,
   numbers, and the multitasking demos.
3. [Servers and drivers](servers.md): serving files with srvlib, requests and waiting, interrupts, putting a driver in
   the ROM.
4. [Libraries](libraries.md): library modules and `XCALL`, the number libraries and their stored format, the asm
   library.
5. [An operating system of your own](your-own-os.md): what boots, init, and a system of your own on the base.
6. [The system call reference](calls-reference.md): every call, generated from the specification.

---

## What a program is

Every executable on the Hydra is a **HYX2** file: a 48-byte header, then its code and data.  The header says what it
is (a program, a driver or a library), where it loads, its entry points, and its name.  There are two ways to run one:

| | A program in a file | A module of the paged ROM |
| :- | :------------------ | :------------------------ |
| Where it lives | A card, a RAM disk, `/pc`, the ROM disk | The paged ROM, in banks of its own |
| How it runs | `SPAWN` gives the new task the file, and it **reads itself into RAM at `$0800`** as it starts | **Runs in place**: the task's `$01` selects its bank, so its code is at `$A000`; only its data is copied into RAM |
| Its size | Up to `$7FFF`, less its data | 16K a bank, up to 8 banks |
| Built with | `-D HYX2_RAM`, linked by `hyx2.cfg` | Linked by `module.cfg` (`module2.cfg` ... `module8.cfg`), listed in `rom.txt` |
| For | Your programs | The system's programs and drivers, or yours in a ROM of your own |

You'll start with programs in files: build on the PC, copy to a card or `/pc`, run by name.

---

## The workflow

1. **Write** the source on the PC (or on the Hydra with `edit`).
2. **Build** it: `node build.js prog DIR` in `reborn/` builds the folder DIR's `.s` or `.c` files into `DIR/NAME.hyx`.
   Or use the SDK alone with cc65's tools: `bin/sdk/asm` and `bin/sdk/c` are the SDKs, whole, to take anywhere.  Or
   assemble on the Hydra itself: `as file.s` makes the same program, byte for byte.
3. **Run** it in the emulator: `node sim/run.js -i --pc-dir DIR`, then `/pc/NAME` at the prompt.  Or put it on a card
   image (`hydrafs.js put`) and run it with `--sd`.
4. **Debug** it: `--trace-calls` in the emulator shows every call it makes; `db` on the Hydra steps it
   ([db](../07-tools.md#db-the-debugger)); `--break` and the emulator's monitor stop anywhere.
5. **Run it on the board**: copy it to a card, or over `/pc` with the PC tool, and type its path.  `bind -a /sd/0/bin
   /bin` (or a card's `/bin`, which is bound already) runs it by name.

---

## The rules a program keeps

* **Call the system with `jsr`** to the jump table; arguments in `.A`, `.X`, `.Y` and `r0`-`r15`; C = 1 on failure with
  the error in `.A` ([the system calls](../03-base/calls.md)).
* **Your zero page is `$22-$7F`**: no call touches it.  `$80-$FF` and `$0200-$03FF` are the system's (but `TASK_EVENT`,
  a server's).
* **Your RAM is `$0400` up** (a RAM program's from `$0800`), to `$7FFF`; your banks are at `$8000` when you select them.
* **Never write `T`, `V` or `W`**, and leave the I/O area to the drivers: use the devices' files.  (`U` and `$00` are
  yours, for your shared segments and banks.)
* **Never hold interrupts off** for more than 200 cycles.
* **A long job is fine**: the scheduler preempts you.  But a busy loop waiting for something wastes the CPU: wait in a
  call (`READ`, `SLEEP`, `SEM_ACQUIRE`, `PAUSE`) instead.
* **Take your notes**: Ctrl-C ends a program unless it has a handler (`NOTIFY`), and a handler should end it soon.

---

## A first program

`sdk/asm/samples/hi/hi.s` greets its arguments and says where it runs: its arguments come at `r0`, it prints with
`PUTS` and `PUTC`, asks the kernel its task (`GETPID`) and its directory (`GETCWD`), and reads its environment
(`ENV_GET`); returning from `main` ends it with code 0.  [The system calls](../03-base/calls.md#a-program-walked-through)
walks through it, and it's on HydraOS's card:

```
/> /sd/0/sample/hi Ann Bob
Hello, Ann!
Hello, Bob!
I'm task 4, in /, in window 0.
```

The samples, in `reborn/sdk/asm/samples` and `reborn/sdk/c/samples` (built onto the card at `/sd/0/sample` and
`/sd/0/sample/c`):

| Sample | Shows |
| :----- | :---- |
| `hi` (asm) | Arguments, the task, the directory, the environment |
| `upper` (asm) | A filter on the tools' library |
| `tick` (asm) | A note handler |
| `counter` (asm) | A server: a driver, a module run in place, on srvlib |
| `nsum` (asm) | The number libraries |
| `hello`, `upper`, `code` (C) | Arguments, a filter, exit statuses |
| `keys` (C) | conio: the screen and raw keys |
| `tones`, `jukebox` (C) | Sound |
| `sketch`, `shapes` (C) | The Vera X: drawing and the mouse; cc65's TGI |
| `race`, `chorus`, `philo`, `prodcons`, `round` (C) | Tasks working together: shared segments and semaphores |
| `ntest`, `ctest` (C) | The number library's test; the C library's test |
