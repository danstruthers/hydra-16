# C

Programs for the Hydra in C, with **cc65** (`cc65`, `ca65`, `ld65`).  A C program is a HYX2 file, as an assembly one
is: a 48-byte header, then its code and data, which `SPAWN` reads into a task's RAM at `$0800` and starts.  cc65's own
target `none` does the compiling; the Hydra's part is the start-up, the link, and a library under the standard one, so
standard C (stdio, files, `malloc`, `printf`, `system`, `signal`, conio) works on the Hydra's files and devices.

## Contents

1. [The SDK](#the-sdk)
2. [A program](#a-program)
3. [Numbers](#numbers)
4. [Building it](#building-it)
5. [Running it](#running-it)
6. [The multitasking demos](#the-multitasking-demos)
7. [How C reaches the system](#how-c-reaches-the-system)

---

## The SDK

The C SDK is `reborn/sdk/c`; the build makes its library and copies it, with the generated headers and the samples, to
`reborn/bin/sdk/c`, to take anywhere.

| File | What it is |
| :--- | :--------- |
| `include/hydra.h` | The Hydra's own calls (tasks and exit statuses, the namespace, the tick, RAM banks, semaphores, shared segments, the windows, any call by `hy_call`), and what cc65's headers leave to a target: `setenv`, `fstat`, `isatty`, conio's colours and keys |
| `include/hydracalls.h` | Every system call's address, error code and constant, each with `HY_` before its name.  Made from `base/spec/` by the build, kept in Git: never edit it |
| `include/snd.h` | Sound: the YM2151 and the Vera X's PSG through the sound driver: claims, patches, notes, volumes, bends, drums, waveforms, MML, songs |
| `include/vera.h` | The Vera X: the bitmap and drawing on it, VRAM, the palette, sprites, frames, the mouse; the chip claimed and its registers; `hydra_tgi`, cc65's TGI's driver |
| `include/num.h`, `numdefs.h` | The Hydra's numbers ([below](#numbers)) |
| `include/asm.h`, `asmdefs.h` | The asm library: `dis_insn` (an instruction disassembled), symbol files (`lbl_load` ...) |
| `hydra.cfg` | The link: the header, code and data from `$0800`, the BSS, the heap, and the C stack (2K) down from `$7F00` |
| `lib/` | The library's sources, built with cc65's `none.lib` into `hydra.lib` |
| `samples/` | `hello`, `upper`, `code`, `keys`, `tones`, `jukebox`, `sketch`, `shapes`, `ctest`, `ntest`, and the multitasking demos |

---

## A program

```
#include <stdio.h>

int main (int argc, char* argv[])
{
    printf ("hello, %s\n", argc > 1 ? argv[1] : "world");
    return 0;
}
```

* `argv[0]` is its name: its file's (`/sd/0/sample/c/hello`'s is `hello`); its arguments are rc's words.
* `main`'s value, or `exit`'s, is its exit code (0 is success); `hy_exits ("why")` ends it with a message and
  code 1, as Plan 9's `exits` does.  rc's `$status` is the message if there is one, else the code.
* Its fds 0, 1 and 2 are stdin, stdout and stderr: its window's console, or what rc gave it (`<`, `>`, `|`).
  stdio is buffered: a file's bytes are read and written 255 at a time, a console's output a line at a time
  (and before any read), stderr's at once; it all goes out at `fflush`, `fclose` and the program's end.
* Files: `fopen`, `open`, `read`, `write`, `lseek`, `stat`, `fstat`, `remove`, `rename`, `mkdir`, `rmdir`,
  `chdir`, `getcwd`, and `opendir`/`readdir` (`hy_dirstat`: an entry's whole stat record).  Names go through its
  namespace, as rc's do; `hy_bind`, `hy_mount` and `hy_unmount` change it.
* A failed call returns -1 (or NULL) and sets `errno` (cc65's: `ENOENT` ...) and `_oserror` (the kernel's own
  code, `HY_E_NOENT` ...; `_stroserror` and `hy_errstr` give its text, the kernel's).
* Its environment is a copy of rc's variables: `getenv`, `setenv`, `putenv`, `unsetenv`.
* `system ("cmd")` runs a command line with rc (`rc -c`): its value is the command's exit code.  `hy_spawn`
  starts a program in a task of its own, `hy_wait` waits for it and takes its code and message; `hy_exec` runs a
  program in this one's place, in its task (Plan 9's `exec`: back only if it fails); `hy_parent` is
  the task that started this one.
* Semaphores, for tasks that share something: `hy_sem_new` (a count, or `HY_SEM_MUTEX`), `hy_sem_acquire` (it
  waits), `hy_sem_try`, `hy_sem_release`, `hy_sem_free`.  Every task's, by number; the program's end frees those
  it made and gives back the mutexes it holds.
* Shared segments, memory several tasks see: `hy_seg_create` (banks of 8K: its number, this task attached),
  `hy_seg_attach` (another task, given the number), `hy_seg_map` (a bank of it at `HY_BANK_WINDOW`, `$8000`),
  `hy_seg_detach`.  The last task attached to go frees it.
* `signal (SIGINT, f)`: Ctrl-C at its window (the interrupt note) runs `f`; `SIG_IGN` ignores it; the default
  ends the program.
* conio (`conio.h`) works the console as an ANSI terminal: `clrscr`, `gotoxy`, `textcolor`, `revers`, `cursor`;
  `cgetc` reads keys raw (no echo, each as it's typed, the cursor and function keys as one code each: `CH_*` in
  `hydra.h`) until the program ends.  `screensize` is the window's size (its `consctl`'s `size` line); `cgetc`
  gives `CH_RESIZE` when it changes, and `screensize` then has the new one.  With `keys mods` written to `consctl`, a
  key the terminal sent modified comes as `CH_MOD`, its modifiers (`HY_MOD_SHIFT`, `HY_MOD_ALT`, `HY_MOD_CTRL`), then
  the key.
* The window's chrome (`hydra.h`): `hy_wlabel` its title, `hy_wstatus` its status line (the footer's `%s`),
  `hy_wctl` any line of its `wctl` (`chrome screen off` ...); `hy_wnew (flags)` a window made, not shown (`HY_WGROUP`:
  in a group of its own), its number: its files `#cN` (`#cN/cons` ...), the window gone with its last cons closed.
* Sound (`snd.h`): claim the channels it uses (`snd_claim`), then patches, notes, volumes, bends and drums on
  them; they're given back as it ends.  `snd_play` plays a song in the background (`play`, in a task of its own).
* `time` is the system's clock (`/dev/time`; `hy_time`, in seconds since 2000), `clock` the ticks since the
  program's start; `sleep` and `hy_sleep_ticks` let the other tasks run, and `hy_sleep_until` sleeps till a tick
  count (for steady timing: a time already passed returns at once).
* Its RAM: `$0800` to `$7F00`, for the program, its BSS, the heap (`malloc`) and the C stack (2K); its own RAM
  banks at `$8000`-`$9FFF` (`hy_banks_alloc`, `hy_bank`).  The 6502's stack is 256 bytes: deep recursion runs
  out of it.  cc65's runtime has the zero page from `$22` (26 bytes); the program's own goes after it, to `$7F`.

---

## Numbers

`num.h` gives C the numbers hylang, HyForth and BASIC have: integers of any size (to 255 bytes), fixed decimals
(`1.25`), rationals (`2/3`) and complex numbers (`1+2i`), all exact, worked by the number libraries in the paged
ROM.  A number is an array of bytes (`num_t`) in their stored format: `NUM_MAX` (1,040) at most, most of them a few.

```
#include <stdio.h>
#include <num.h>

int main (void)
{
    num_t a[32], b[32], c[64];

    num_parse (a, sizeof a, "2/3", NULL, NULL);
    num_parse (b, sizeof b, "0.5", NULL, NULL);
    num_add (c, sizeof c, a, b);
    printf ("%N, in binary %{b}N\n", c, c);        /* 7/6, in binary 111/110 */
    return 0;
}
```

* A function that makes a number takes its place and its room first, then its operands, and gives back the
  result's length; or -1, and `num_error` says why (`NE_DIV0` ...; `num_strerror` its text).  A result may go
  over one of its operands.  `num_size` is a number's length, from its bytes.
* The first call readies the libraries (`num_init`): it takes two of the program's RAM banks.
* Numbers are read and written in the base, decimal at the start; `num_set_base` changes it, with hylang's base
  strings (`"x"` writes `FF`, `"#x"` `#xFF`; `"b"`, `"o"`, `"c"` balanced ternary, `"16r"`, `"[01]"` digits of
  its own ...).  `num_parse` and `num_display` take a base of their own too (`NULL`: the base); `num_format` is
  hylang's `format` (`"{} is {x}"`).
* `num_digits` sets the math functions' precision, 12 significant digits at the start: `num_sqrt` of 2 is
  `1.41421356237`, of `9/4` exactly `3/2`.
* `printf` (`fprintf`, `sprintf`, `snprintf` and their `v` forms) writes a number with `%N` (a `num_t*`), in the
  base; and with a base in braces right after the `%`, for `%N` and C's integers alike: `%{x}N`, `%{#b}d`
  (`#b101`), `%{c}ld`, `%{16r}u`; `%{}N` the base, and `%{*}N` the base from the arguments, a string (before a `*`
  width's).  The width, `-` and `0` pad the whole text, a `+` or a space goes before a number not below 0, and a
  precision is passed over: a number is written whole, however long (2^1000's 302 digits; its text 8K at most).
  Without braces, `%d` and `%x` are C's own.
* `scanf` (`fscanf`, `sscanf`) reads them the same way: `%N` a number into a place and its room, two arguments
  (`scanf ("%N", n, sizeof n)`), and `%{x}d` an `int` in base x.  Such a conversion reads a word: up to white
  space, its width, or the character the format has next (`"%N,%N"` reads `1/2,3`), all of which must be a number.
* `calc` (`programs/calc`) is a program on it; the sample `ntest` is its test.  Assembly has the same calls:
  `sdk/asm`'s `numbers.inc` and `numlib.s` ([libraries](libraries.md#the-number-libraries)).

---

## Building it

In this repository, a program in a folder of its own (its `.c` files, and `.s` files if it has any):

```
node build.js prog path/to/hello          # path/to/hello/hello.hyx
```

With cc65 and `bin/sdk/c` alone:

```
cc65 -t none --cpu 65C02 -O -I sdk/c/include hello.c
ca65 --cpu 65C02 hello.s
ld65 -C sdk/c/hydra.cfg -o hello.hyx hello.o sdk/c/lib/hydra.lib
```

---

## Running it

As an assembly program is run ([assembly](assembly.md#running-it)): copy it to a card or the RAM disk, and type its path, or its
name if it's in `.` or a `bin`.  The samples are on the SD card (`bin/sdcard.img`): `/sd/0/sample/c/hello you`,
`echo hi | /sd/0/sample/c/upper`, `/sd/0/sample/c/keys`, `/sd/0/sample/c/tones`; `cd /ram; /sd/0/sample/c/ctest a 'b c'` runs the
library's test.

---

## The multitasking demos

Five samples show tasks working together.  Each is one program that starts copies of itself as its workers (each in
a task of its own, given what it needs as arguments: a shared segment's number, semaphores' numbers), draws what
they do as they do it, and says how it went.  They find themselves by `$prog`, the path rc started them from
(`getenv ("prog")`: so a copy runs from anywhere, `/ram/round`), else as `/bin/NAME`, `./NAME` or
`/sd/0/sample/c/NAME` (a file, not a directory of that name), so after `bind -a /sd/0/sample/c /bin` (at rc, or
HyForth's `bind`) they run by name.  Each
needs four or five tasks free besides its own; Ctrl-C ends them all, and the program says how far it got.

| Sample | What it shows |
| :----- | :------------ |
| `race [TASKS [ADDS]]` | Why shared memory needs a lock: four tasks add 1 to a counter in a shared segment, 500 times each (read it, work a little, write it back).  With nothing to keep them apart, a task stopped between its read and its write undoes the others' adds, and many are lost; with a mutex around each read and write, none are |
| `chorus` | The console as a shared resource: four tasks print a line each a letter at a time.  With nothing between them the letters tangle; with a mutex held for a whole line each line is whole; with a baton (a semaphore each, passed round: wait for yours, print, give the next one's) the lines come in turn |
| `philo [-d] [-n MEALS] [N]` | The dining philosophers: five tasks, a fork (a mutex) between each two, a live table of who's thinking, hungry or eating and who holds which fork.  Taking the lower-numbered fork first, they never deadlock; with `-d` each takes their left fork first, and they soon do: the program sees it and ends them |
| `prodcons [-n ITEMS]` | Producers and consumers: two tasks make items and two use them, through a ring of 8 slots in a shared segment, kept in step by two counting semaphores (free slots, filled ones) and a mutex.  The ring fills while the producers are quicker and empties while the consumers are, each side waiting (using no CPU) when it must |
| `round [TIMES [TICKS]]` | "Row, Row, Row Your Boat" as a round: four tasks a voice each, on YM2151 channels 0-3, two bars apart.  They meet at a barrier (semaphores), then each keeps its own time by the tick (`hy_sleep_until`), and the words light up as they're sung.  In the emulator, `sim/run.js -i --sound` plays it in a browser |

Each runs as a test too (`tests/tests.js`: race, chorus, philo, prodcons, round): no adds lost with the mutex, the
lines whole and in turn, no fork ever in two hands and the deadlock seen, every item used once, every note in time.

---

## How C reaches the system

* **The start-up** (`lib/crt0.s`) is the program's HYX2 header and its `main`: it sets up cc65's C stack and the heap,
  turns the kernel's argument list into `argc` and `argv`, calls `main`, and ends with `EXITS` and `main`'s value.
* **The library** is cc65's `none.lib` with the Hydra's modules **in place of** cc65's: each module of `lib/` is named as
  the cc65 module it replaces (`open`, `read`, `write`, `fclose`, `getenv`, `conio` ...), so cc65's own code (stdio's
  formatting, `malloc`, the string functions) runs unchanged on top.
* **Any call**: `hy_call (HY_NAME, &regs)`, with `struct hy_regs` holding `.A`, `.X`, `.Y` and `r0`-`r3`:

```
#include <hydra.h>

struct hy_regs r;
r.a = 0;                                    /* .A: a task */
if (hy_call (HY_TASKINFO, &r) != 0) ...     /* 0, or the error code (and errno, _oserror) */
```

* **Errors**: a failed call returns -1 (or NULL) and sets `errno` (the POSIX code `base/spec/errors.def` gives each of
  the kernel's codes) and `_oserror` (the kernel's own, `HY_E_NOENT` ...).
* **The zero page**: cc65's runtime has `$22` up (26 bytes), and the program's own goes after it, to `$7F`.  A library
  routine that cc65's own assembly calls (`_cputc`, `_fgetc`) keeps the runtime's scratch registers it relies on.
* **Notes**: `signal (SIGINT, f)` is a note handler for Ctrl-C's note; the default ends the program.
