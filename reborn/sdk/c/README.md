# The C SDK

Programs for the Hydra-16 in C, with cc65 (`cc65`, `ca65`, `ld65`).  A program is a HYX2 file, as an assembly
one is (`sdk/asm/README.md`): a 48-byte header, then its code and data, which `SPAWN` reads into a task's RAM at
`$0800` and starts.  cc65's own target `none` does the compiling; the Hydra's part is the start-up, the link and
the library under the standard one.

The SDK is this folder; `node build.js` builds the library and also copies the SDK, with the generated
`hydracalls.h`, the library and the samples, to `bin/sdk/c`, to take elsewhere.

| File | What it is |
|---|---|
| `include/snd.h` | The YM2151, and the Vera X's PSG (channels 8-23), through the sound driver: channels claimed, patches, notes, volumes, bends, drums, waveforms, raw registers |
| `include/hydra.h` | The Hydra's own calls (tasks and exit statuses, the namespace, the tick, RAM banks, any call by `hy_call`), and what cc65's headers leave to a target: `setenv`, `fstat`, `isatty`, conio's colours and keys |
| `hydracalls.h` | Every system call's address, error code and constant, each with `HY_` before its name.  Made from `spec/` by the build (`obj/sdk/c/hydracalls.h`); never edit it.  `hydra.h` includes it |
| `hydra.cfg` | The link: the header, code and data from `$0800`, the BSS after them, the heap, and the C stack (2K) down from `$7F00` |
| `lib/hydra.lib` | The library: cc65's `none.lib`, with the modules of `lib/` in place of cc65's that a target gives (the build: `obj/sdk/c/hydra.lib`) |
| `lib/` | Its sources: `crt0.s` (the header, the start, `exit`), the files and stdio's buffers, the environment, `system`, `signal`, `time` and `clock`, conio, errors |
| `samples/` | `hello` (arguments), `upper` (a filter), `code` (exit statuses), `keys` (conio: the screen and raw keys), `tones` (sound: `snd.h`), `jukebox` (a song in the background: `snd_play`), `ctest` (the library's test); the multitasking demos (below): `race`, `chorus`, `philo`, `prodcons`, `round` |

## A program

```
#include <stdio.h>

int main (int argc, char* argv[])
{
    printf ("hello, %s\n", argc > 1 ? argv[1] : "world");
    return 0;
}
```

* `argv[0]` is its name: its file's (`/rom/sample/c/hello`'s is `hello`); its arguments are rc's words.
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
  starts a program in a task of its own, `hy_wait` waits for it and takes its code and message; `hy_parent` is
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
  gives `CH_RESIZE` when it changes, and `screensize` then has the new one.
* The window's chrome (`hydra.h`): `hy_wlabel` its title, `hy_wstatus` its status line (the footer's `%s`),
  `hy_wctl` any line of its `wctl` (`chrome screen off` ...).
* Sound (`snd.h`): claim the channels it uses (`snd_claim`), then patches, notes, volumes, bends and drums on
  them; they're given back as it ends.  `snd_play` plays a song in the background (`play`, in a task of its own).
* `time` is the system's clock (`/dev/time`; `hy_time`, in seconds since 2000), `clock` the ticks since the
  program's start; `sleep` and `hy_sleep_ticks` let the other tasks run, and `hy_sleep_until` sleeps till a tick
  count (for steady timing: a time already passed returns at once).
* Its RAM: `$0800` to `$7F00`, for the program, its BSS, the heap (`malloc`) and the C stack (2K); its own RAM
  banks at `$8000`-`$9FFF` (`hy_banks_alloc`, `hy_bank`).  The 6502's stack is 256 bytes: deep recursion runs
  out of it.  cc65's runtime has the zero page from `$22` (26 bytes); the program's own goes after it, to `$7F`.

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

## Running it

As an assembly program is run (`sdk/asm/README.md`): copy it to a card or the RAM disk, and type its path, or its
name if it's in `.` or a `bin`.  The samples are on the ROM disk: `/rom/sample/c/hello you`,
`echo hi | /rom/sample/c/upper`, `/rom/sample/c/keys`, `/rom/sample/c/tones`; `cd /ram; /rom/sample/c/ctest a 'b c'` runs the
library's test.

## The multitasking demos

Five samples show tasks working together.  Each is one program that starts copies of itself as its workers (each in
a task of its own, given what it needs as arguments: a shared segment's number, semaphores' numbers), draws what
they do as they do it, and says how it went.  They find themselves as `/bin/NAME`, `./NAME` or
`/rom/sample/c/NAME`, so after `bind -a /rom/sample/c /bin` (at rc, or HyForth's `bind`) they run by name.  Each
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
