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
| `samples/` | `hello` (arguments), `upper` (a filter), `code` (exit statuses), `keys` (conio: the screen and raw keys), `tones` (sound: `snd.h`), `jukebox` (a song in the background: `snd_play`), `ctest` (the library's test) |

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
* `signal (SIGINT, f)`: Ctrl-C at its window (the interrupt note) runs `f`; `SIG_IGN` ignores it; the default
  ends the program.
* conio (`conio.h`) works the console as an ANSI terminal: `clrscr`, `gotoxy`, `textcolor`, `revers`, `cursor`;
  `cgetc` reads keys raw (no echo, each as it's typed, the cursor and function keys as one code each: `CH_*` in
  `hydra.h`) until the program ends.  `screensize` is the window's size (its `consctl`'s `size` line); `cgetc`
  gives `CH_RESIZE` when it changes, and `screensize` then has the new one.
* Sound (`snd.h`): claim the channels it uses (`snd_claim`), then patches, notes, volumes, bends and drums on
  them; they're given back as it ends.  `snd_play` plays a song in the background (`play`, in a task of its own).
* `time` is the system's clock (`/dev/time`; `hy_time`, in seconds since 2000), `clock` the ticks since the
  program's start; `sleep` and `hy_sleep_ticks` let the other tasks run.
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
