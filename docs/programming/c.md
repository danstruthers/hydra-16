## **The C Programmer's Guide**

How to write programs for the Hydra-16 in C: setting up cc65, building and running a program, what a C program gets from the system, the library (the standard one and the Hydra's own), the console, files, processes and time, mixing in assembly, performance, debugging, and the limits to keep in mind.  It's for programmers who know C but not the Hydra or cc65.  Part of the [Programmer's Guide](README.md); [programs.md](programs.md) has the executable format underneath.

### **Contents**
1. [Quick start](#1-quick-start)
2. [The machine, as C sees it](#2-the-machine-as-c-sees-it)
3. [Building](#3-building)
4. [A program's life: arguments, environment, exit status](#4-a-programs-life-arguments-environment-exit-status)
5. [Files and devices](#5-files-and-devices)
6. [The console: stdio and conio](#6-the-console-stdio-and-conio)
7. [Running other programs](#7-running-other-programs)
8. [Time](#8-time)
9. [Tasks working together: semaphores](#9-tasks-working-together-semaphores)
10. [Errors](#10-errors)
11. [Memory](#11-memory)
12. [Assembly in a C program](#12-assembly-in-a-c-program)
13. [Performance](#13-performance)
14. [Debugging and testing](#14-debugging-and-testing)
15. [Limits and gotchas](#15-limits-and-gotchas)
16. [Reference: `hydra.h`](#16-reference-hydrah)
17. [Working on the library](#17-working-on-the-library)

### **1. Quick start**

**What you need:**

| Tool | For | Where |
| :--- | :-- | :---- |
| cc65 (V2.19 or later) | The C compiler, assembler and linker | [cc65.github.io](https://cc65.github.io/): the Windows snapshot zip, or build it from source.  The build looks in `CC65_HOME`, then on the `PATH`, then in `C:\source\cc65\win64_snapshot` |
| Node.js | The emulator and the card image tool | [nodejs.org](https://nodejs.org) (no packages needed) |
| The ROM images | The emulator runs them | Built already in `os_rom/bin`, or build them ([getting started](../getting-started.md)) |

**Build the library** (once, and after pulling changes to `programs/c/lib`).  It builds `lib\hydra.lib` and the samples in `programs/c/samples`:

```
set CC65_HOME=C:\path\to\cc65        (if it isn't in the default place)
programs\c\make.bat
```

**Write a program**, `hello.c`:

```c
#include <stdio.h>

int main (int argc, char* argv[])
{
    printf ("Hello from %s, with %d arguments\n", argv[0], argc - 1);
    return 0;
}
```

**Build it** into `programs\c\bin\hello.hyx`:

```
programs\c\hyc.bat hello.c
```

**Run it in the emulator.**  Make a card image, put the program in its `bin` directory, and boot with the card (from `sim/`; in Git Bash, leave out a card path's first `/`):

```
node tools/hydrafs.js mkfs work.img 16 WORK
node tools/hydrafs.js mkdir work.img bin
node tools/hydrafs.js put work.img ../programs/c/bin/hello.hyx bin
node hydrasim.js -i --sd work.img
```

At HyForth's prompt, a program runs by its name:

```
0:/> hello a "b c"
Hello from hello, with 2 arguments
0:/> status .
 0000
```

Ctrl-A x quits the emulator ([its keys](../tools/emulator.md#using-the-hydra-from-your-terminal)); `--speed 0` runs it as fast as the PC can.

**On the real Hydra**, the card is the same format: read the card into an image with a disk imager (Win32 Disk Imager, `dd`), `put` the program into the image, and write it back.  Or put it on one card and `cp` it to another on the Hydra.  The emulator is the quickest loop, and what the Hydra does with a program there it does on the board.

**Not on Windows?**  The two `.bat` files run `build.js` (at the top of the repository), which works on any OS: `node build.js c` builds the library and the samples, and `node build.js prog hello.c` a program of your own (`programs/c/bin/hello.hyx`).  By hand, the library is each file in `lib/crt`, `lib/io`, `lib/env`, `lib/conio`, `lib/snd` and `lib/sys`, assembled or compiled, and added (`ar65 a`) to a copy of cc65's `none.lib` as `lib/hydra.lib`; a program is then:

```
cc65 -t none --cpu 65C02 -O -I programs/c/include -o obj/hello.s hello.c
ca65 --cpu 65C02 -o obj/hello.o obj/hello.s
ld65 -C programs/c/hydra.cfg -o hello.hyx obj/hello.o programs/c/lib/hydra.lib
```

### **2. The machine, as C sees it**

* **The CPU** is a W65C02S at 3.58 MHz.  cc65's code runs roughly 10-20 times slower than careful assembly, so a C program on the Hydra is about as fast as compiled BASIC on a home computer of the 1980s: fine for tools, games and logic, slow for heavy number crunching ([performance](#13-performance)).
* **Types:** `int` and pointers are 16 bits, `long` is 32.  **`char` is unsigned** (cc65's default; `-j` makes it signed).  **No `float` or `double`** (cc65 says "Floating point type is currently unsupported"), and **no `long long`**: use integers, `long` or fixed point.
* **A task of its own.**  A program runs in one of the 16 tasks, preemptively scheduled at 200 Hz alongside the shell, drivers and other programs.  A read that has to wait (the keyboard, a pipe) puts the task to sleep; so does `sleep`.  Nothing a program does can stop the others, short of turning interrupts off.
* **Its own 32K.**  Each task has its own RAM from `$0000` to `$7FFF`, zero page and stack included: the hardware swaps it on a task switch.  So a program's memory is its own, and a crash can't corrupt another task's.
* **No screen.**  The console is a terminal on the serial port (ANSI, 9600 baud to start), so text I/O is the interface; `conio` drives it with ANSI sequences ([below](#6-the-console-stdio-and-conio)).
* **The OS**, Plan 9 style: everything is a file.  The cards (`/sd/0/...`), devices (`/dev/cons`, `/dev/time`), the environment (`/env/NAME`) and the tasks (`/dev/proc`) are all read and written through file descriptors.  The C library puts the standard calls on top of that.

**A program's memory** (the defaults in `programs/c/hydra.cfg`):

| Addresses | What |
| :-------- | :--- |
| `$0000-$00DF` | The OS's zero page: don't touch |
| `$00E0-$00F9` | cc65's runtime zero page (`sp`, `ptr1`-`ptr4`, `tmp1`-`tmp4`, `regbank`, ...) |
| `$00FA-$00FF` | Free: 6 bytes of zero page for your assembly |
| `$0100-$01FF` | The 6502 stack (return addresses; the C stack is separate).  At the start, `$0110` has the arguments and `$0150` the name (copied by the start-up code) |
| `$0200-$07FF` | The OS's per-task buffers (stdout's at `$0700`, stdin's read-ahead at `$0780`): leave them alone |
| `$0800-...` | The program: code, read-only data, data, then the BSS (cleared at the start) |
| `...-$67FF` | The heap (`malloc`): from the BSS's end up to the stack |
| `$6800-$6FFF` | The C stack (2K, `__STACKSIZE__`), growing down from `$7000` (`__RAMTOP__`) |
| `$7000-$7CFF` | The MMU's pages (`MM_ALLOC`) |
| `$7D00-$7FFF` | The task system page (the fd table, IRQ tables) and the MMU's state |
| `$8000-$9FFF` | An 8K window: the task's own RAM banks, when it maps one ([memory](#11-memory)) |
| `$A000-$FFFF` | ROM and I/O |

So code, data, heap and stack share 26K.  A small program (`hello.c`: printf, strings, time) is about 8K, which leaves some 16K of heap; a program with only `puts` has over 20K.

### **3. Building**

| Script | Does |
| :----- | :--- |
| `programs\c\make.bat` | Builds `lib\hydra.lib` (cc65's `none.lib` with the Hydra's modules added or in place of cc65's) and every `samples\*.c` into `bin\` |
| `programs\c\hyc.bat FILE.c [MORE.c ...] [MORE.s ...]` | Builds one program, `bin\FILE.hyx` (named after the first file), and its map, `bin\FILE.map` |

`hyc.bat` compiles each `.c` with `cc65 -t none --cpu 65C02 -O` and the include path `programs\c\include` (for `hydra.h`), assembles each `.s` with `ca65` (with `programs\c\lib` on its include path, for `hydra.inc`), and links with `hydra.cfg` and `lib\hydra.lib`.  Give each file its own name: `prog.c` and `prog.s` would both make `obj\prog.o`.

**Options** go in two environment variables:

| Variable | Goes to | E.g. |
| :------- | :------ | :--- |
| `HYC_CFLAGS` | cc65 | `--check-stack` (stop on a stack overflow: [below](#11-memory)); `-Oi` (inline more), `-Or` (register variables), `-Os` (inline some library functions); `-DDEBUG=1` |
| `HYC_LDFLAGS` | ld65 | `-D __STACKSIZE__=$0400` (a 1K C stack); `-D __RAMTOP__=$7C00` (all of task RAM: see [memory](#11-memory)) |

```
set HYC_CFLAGS=--check-stack -Or
programs\c\hyc.bat game.c map.c sound.s
```

**The map file** (`bin\NAME.map`) lists each segment's size and where every function and variable went: the place to look when a program gets too big, and to find an address for the debugger.

**The executable** (`.hyx`): a 16-byte header, then the code and data, loaded at `$0800`.  The header also tells the loader how big the BSS is (it clears it, so it isn't in the file) and where the program's RAM ends (the MMU's pages start above it).  The start-up code (`lib/crt/crt0.s`) fills all of that in; there's nothing to set by hand.  The format is in [programs.md](programs.md#the-file).

### **4. A program's life: arguments, environment, exit status**

**Running it.**  HyForth runs a program by name: a word it doesn't know is looked for as `name.hyx` (then `name.hys`, a script, and `name.zsm`, a song) in the current directory, then in `/bin` (a union in the namespace of the program caches, the boot card's `/bin` and the ROM's: [io.md](io.md#namespaces)), then in the directories of `$PATH`.  `run file.hyx args` runs one by its file name.  **HyForth's own words come first**: a program called `fg`, `ls` or `wait` never runs by name (use `run`, or another name; `words` lists them).

**What it starts with:**
* **`argc` and `argv`.**  `argv[0]` is the name as it was typed (`hello`, `bin/hello.hyx`).  The rest of the line is split at spaces; `"two words"` is one argument (without the quotes).  At most 15 arguments, and 63 characters of line.
* **stdin, stdout and stderr** (fds 0-2): the shell's, usually the console.  `prog < in.txt`, `prog > out.txt`, `prog >> log.txt` and pipelines (`a | b`) change them before the program starts.
* **The current directory, namespace and environment**: copies of the shell's.  What the program changes (`chdir`, `setenv`) is its own, and passes to the programs it starts.
* **The console**, if the shell had it: the program's reads get the keys, and Ctrl-C stops it.

**Ending.**  `main` returns, or the program calls `exit` or `hy_exits`.  The functions registered with `atexit` run, the files are closed (buffered output goes out), and the memory is freed.  The **exit status**, as Plan 9 has it, is a code (0-255) and, optionally, a message:

| How it ends | Code | Message |
| :---------- | :--- | :------ |
| `return n;` from `main`, `exit (n)` | n (its low byte) | none |
| `hy_exits ("bad input")` | 1 | `bad input` (30 characters at most) |
| `hy_exits (NULL)`, `hy_exits ("")` | 0 | none |
| Ctrl-C (a break) | 130 | `interrupt` |
| Killed (Ctrl-\\, `kill`, its shell's Ctrl-C) | 137 | `killed` |
| A stack overflow, built with `--check-stack` | 4 | none |

**`atexit` functions don't run on Ctrl-C or a kill**: the task just ends (its files are still closed, and output written with `write` or `printf` is never lost).  Don't rely on an `atexit` function to save state.

The shell keeps the status of each program it waits for.  HyForth's `status` gives the code, and `/env/status` has it as text: the message, the code if there's no message, or nothing for 0 (Plan 9's `$status`).  So scripts can check a step:

```
0:/> mytool in.txt
0:/> status .
 0001
0:/> cat /env/status
bad input
```

Use 0 for success, 1 with a message for a failure, and 2 for a usage error, as Unix tools do.

**The background:** `prog &` starts it without waiting: the shell prints its task (`[B]`), puts it in `/env/apid`, and gives the prompt back.  A background program waits if it reads or writes the console, until it's brought to the front (`fg`, Ctrl-] and its task) or waited for (`$B wait`: task numbers are hex).  Ctrl-C at the prompt kills the programs the shell started, the background ones too ([the console keys](io.md#the-console)).

### **5. Files and devices**

**Names.**  A name is relative to the current directory unless it starts with `/`.  The namespace:

| Path | What |
| :--- | :--- |
| `/sd/0/...`, `/sd/1/...` | The files on SD card 0, 1, ... (HydraFS).  The shell's `0:/` prompt is `/sd/0` |
| `/dev/cons` | The console |
| `/dev/null`, `/dev/zero` | As on Unix |
| `/dev/time` | The clock, as text: `2026-09-30 14:05:00`; write one to set it |
| `/dev/proc/N/...` | Task N: its `cwd`, `env`, `mem` |
| `/dev/snd` | The YM2151 sound chip ([below](#sound-sndh)) |
| `/env/NAME` | An environment variable (below) |

**Size buffers for names with `HY_NAME_MAX` and `HY_PATH_MAX`, not `FILENAME_MAX`.**  A file's name is up to 31 characters (`HY_NAME_MAX`, 32 with its 0), and a path up to 64 (`HY_PATH_MAX`, 65); cc65's `FILENAME_MAX` is 17 for this target, so a buffer sized with it cuts names short.  `/` and `/dev` aren't directories you can list; a card's directories are.  See [io.md](io.md) for the devices in full.

**What works:**

| Header | Calls |
| :----- | :---- |
| `stdio.h` | `fopen` (`r`, `w`, `a`, `r+`, `w+`, `a+`; there's no text mode: bytes are bytes), `fread`, `fwrite`, `fgets`, `fputs`, `fprintf`, `fscanf`, `fseek`, `ftell`, `rewind`, `fflush`, `feof`, `ferror`, `fclose`, `remove`, `rename`, `perror` |
| `fcntl.h`, `unistd.h` | `open` (`O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`, `O_TRUNC`, `O_APPEND`, `O_EXCL`), `read`, `write`, `lseek`, `close`, `chdir`, `getcwd`, `rmdir`, `isatty` |
| `sys/stat.h` | `stat`, `fstat` (`st_size`, `st_mode`, `st_mtime`), `mkdir` |
| `dirent.h` | `opendir`, `readdir`, `closedir`, `rewinddir`, `telldir`, `seekdir` |

**Listing a directory**, with each entry's size and kind (`hy_dirstat` gives the entry's `stat` without opening it):

```c
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <hydra.h>

void ls (const char* path)
{
    DIR* d = opendir (path);
    struct dirent* e;
    struct stat st;

    if (d == 0) {
        perror (path);
        return;
    }
    while ((e = readdir (d)) != 0) {
        hy_dirstat (d, &st);
        printf ("%-20s %8ld%s\n", e->d_name, st.st_size, S_ISDIR (st.st_mode) ? " dir" : "");
    }
    closedir (d);
}
```

**Details:**
* `st_mode` has `S_IREAD`, `S_IWRITE` (the file isn't read-only) and `S_IFDIR` (`S_ISDIR`, in `hydra.h`); `st_mtime` is the time of the last change, from HydraFS's stamps.  Devices give zeros.
* `rename` renames a file within its directory; it can't move one to another directory yet.
* `mkdir`'s mode is ignored.
* A task has 12 fds; stdin, stdout and stderr take 3.  An open directory takes one.
* Writes to a card go straight through to it (there's a one-block cache for reading), so a `fclose` is all it takes to be safe.

**The environment** is Plan 9's: each variable is a file, `/env/NAME`.  `getenv` reads one, `setenv`, `putenv` and `unsetenv` change it.  A name is 1-30 characters (no `/` or `=`); a value up to 255 (`HY_ENV_MAX`); a task's variables share 256 bytes.  `getenv` returns a static buffer, overwritten by the next `getenv`: copy a value to keep it.

```c
char* home = getenv ("HOME");           /* NULL if it isn't set */
setenv ("MODE", "fast", 1);             /* This program's, and the programs it starts */
```

The shell uses `PATH`, `LIBPATH` and `HOME`, and sets `status` and `apid`.  conio uses `COLUMNS` and `LINES`.  Reading `/env/NAME` with `fopen` works as well.

#### **Sound: `snd.h`**

The YM2151 has 8 channels (0-7), each a voice of 4 FM operators.  `snd.h` plays them through `/dev/snd` and the ROM's sound library ([io.md](io.md#sound-devsnd)): a channel plays a **patch** (0-127 are General MIDI's instruments, 128-162 drum and percussion sounds) at a **MIDI note** (60 is middle C), with its own volume, speakers and bend.

| Call | Does |
| :--- | :--- |
| `snd_claim (mask)`, `snd_release (mask)` | The channels (bit n: channel n) this program's alone: other programs' writes to them are dropped.  `EBUSY` if another has one.  Given back when the program ends |
| `snd_patch (ch, p)` | Load patch p |
| `snd_note (ch, n)`, `snd_off (ch)` | Key a MIDI note on; key off (the note's release) |
| `snd_vol (ch, v)`, `snd_volume (v)` | A channel's volume, the master volume (0-127) |
| `snd_pan (ch, SND_PAN_LEFT \| _RIGHT \| _BOTH)` | Its speakers |
| `snd_bend (ch, b)` | Its bend, in 64ths of a semitone (-128 to 127) |
| `snd_drum (ch, n)` | A General MIDI drum (35-36 kick, 38 snare, 42 closed hi-hat, 46 open hi-hat, 49 crash ...) |
| `snd_write (reg, val)`, `snd_writes (pairs, n)` | The chip's own registers (the YM2151's datasheet), one or n at a time |
| `snd_regs (buf)` | All 256 registers, as written |
| `snd_reset ()` | Clear the chip and the settings |
| `snd_play (song, loops)` | Play a song (a ZSM file: the Commander X16's format, which the Furnace tracker exports) in the ROM's player, a task of its own: its task, at once.  `loops`: its loop that many more times (0: the song once; `SND_FOREVER`).  `hy_wait` waits for it, `hy_kill` stops it |

```c
#include <hydra.h>
#include <snd.h>

int main (void)
{
    static const unsigned char tune[] = { 60, 64, 67, 72 };
    unsigned char i;

    if (snd_claim (1) < 0) {                /* Channel 0 */
        hy_exits ("sound busy");
    }
    snd_patch (0, 73);                      /* A flute */
    for (i = 0; i < sizeof tune; ++i) {
        snd_note (0, tune[i]);
        hy_sleep_ticks (40);                /* 0.2 s */
        snd_off (0);
    }
    return 0;
}
```

Each call is one request to the sound driver (a couple of thousand cycles), fine for music at human speeds.  For music, a song is easier: `snd_play` ("background.zsm", SND_FOREVER) plays it while the program goes on (a game's sound effects can use the channels the song doesn't), and `hy_kill` on its task stops it; `samples/jukebox.c` shows how.  Songs come from the Furnace tracker (it exports ZSM) or from a score compiled on the PC (`sim/tools/hysong.js`: [the emulator's tools](../tools/emulator.md#songs-the-score-compiler)).  For timing, sleep between events (`hy_sleep_ticks`: 5 ms steps), and measure with `hy_ticks` so delays don't add up.  `samples/tones.c` shows the rest.

### **6. The console: stdio and conio**

**stdio.**  `printf` and `puts` go to stdout, and `fgets`, `scanf` and `getchar` read stdin, as on Unix.  When stdin or stdout is the console, the library acts as a Unix terminal driver does:
* **Output:** each LF (`\n`) goes out as CR LF, so lines start at the left on a real terminal.  (Files and pipes get the bytes as they are.)
* **Input:** the terminal's Enter key (CR) comes to the program as `\n`, so `fgets` and `scanf` see ends of line; the console echoes each key as it's typed.
* **Backspace:** the console erases the character on the screen, but the program still gets the backspace (`\b`, 8): `fgets` keeps it in the line.  For typed input that can be corrected, read a line yourself:

```c
/* A line from stdin, Backspace handled: its length, or -1 at the end of input */
int readline (char* buf, int size)
{
    int n = 0;
    char c;

    for (;;) {
        if (read (0, &c, 1) != 1) {
            return n ? n : -1;          /* Ctrl-D, or the end of a file or pipe */
        }
        if (c == '\n') {
            break;
        }
        if (c == '\b' || c == 127) {    /* (Erased on the screen already) */
            if (n) {
                --n;
            }
        } else if (n < size - 1) {
            buf[n++] = c;
        }
    }
    buf[n] = 0;
    return n;
}
```

* **The end of input** is Ctrl-D (or Ctrl-Z) at the keyboard: `fgets` returns `NULL`, `read` returns 0.
* A program in the background waits for the console, and a program started from a script with its input redirected reads the file.  `isatty (0)` says whether stdin is the console.

**Filters** work as on Unix: read stdin to its end, write stdout.  `samples/upper.c` is one: `hello | upper`, `upper < notes.txt > NOTES.TXT`.

**conio** (`#include <conio.h>`) is cc65's console library, here on an ANSI terminal:

| Call | Does |
| :--- | :--- |
| `clrscr`, `gotoxy`, `gotox`, `gotoy`, `wherex`, `wherey` | Clear the screen, move the cursor, where it is (as far as conio's own output goes: `printf` doesn't move it) |
| `cputc`, `cputs`, `cprintf`, `cputcxy`, `cputsxy` | Output (`\n` is a new line) |
| `textcolor`, `bgcolor` | Colours: `COLOR_BLACK` ... `COLOR_WHITE` (0-7), and the bright ones, `COLOR_GRAY` ... `COLOR_BRIGHTWHITE` (8-15) in `hydra.h` |
| `revers`, `cursor` | Reverse video; show or hide the cursor |
| `chline`, `cvline`, `cclear` (and their `xy` forms) | Lines (`-` and `\|`), blanks |
| `screensize` | `$COLUMNS` x `$LINES`, or 80 x 24 |
| `cgetc`, `kbhit` | A key, raw: as it's typed, with no echo; whether one's waiting |

**Keys:** conio switches the console to raw (writing `rawon` to `/dev/cons/ctl`) the first time it reads one, so keys come as they're typed, unechoed, and Ctrl-D is a key like any other.  The terminal's cursor, editing and function keys come as single codes: `CH_CURS_UP`, `CH_CURS_DOWN`, `CH_CURS_LEFT`, `CH_CURS_RIGHT`, `CH_HOME`, `CH_END`, `CH_PAGE_UP`, `CH_PAGE_DOWN`, `CH_INS`, `CH_DELETE`, `CH_F1`-`CH_F4` (in `hydra.h`); Enter is `CH_ENTER` (13), Esc `CH_ESC`, Backspace `CH_DEL` (127, on most terminals) or 8.  The console goes back to normal when the program ends, however it ends.  Ctrl-C still stops the program.

```c
#include <conio.h>
#include <hydra.h>

int main (void)
{
    unsigned char w, h, x = 40, y = 12;
    char c;

    screensize (&w, &h);
    clrscr ();
    textcolor (COLOR_YELLOW);
    cputsxy (0, 0, "Arrow keys move the *, q quits");
    textcolor (COLOR_WHITE);
    for (;;) {
        cputcxy (x, y, '*');
        c = cgetc ();
        cputcxy (x, y, ' ');
        if (c == 'q') {
            break;
        }
        switch (c) {
        case CH_CURS_UP:    if (y > 1) --y;     break;
        case CH_CURS_DOWN:  if (y < h - 1) ++y; break;
        case CH_CURS_LEFT:  if (x > 0) --x;     break;
        case CH_CURS_RIGHT: if (x < w - 1) ++x; break;
        }
    }
    clrscr ();
    return 0;
}
```

conio and `printf` can be mixed: their output stays in order.  `samples/keys.c` shows the keys' codes.

### **7. Running other programs**

A command line runs as it would at the prompt, in a **command shell** (HyForth reading the line from a pipe, with no banner or prompt: Plan 9's `rc -c`), in a task of its own, with this program's fds, current directory and environment.  So it can be a program, a script, a pipeline or a HyForth word.

| Call | Does |
| :--- | :--- |
| `system (cmd)` | Runs it and waits: its exit code (0-255), or -1.  `system (NULL)` is 1: there's a shell |
| `hy_spawn (cmd)` | Starts it and returns at once: its task (1-15), or -1 |
| `hy_wait (task, msg)` | Waits for a task this program started: its exit code, and its message into `msg` (`HY_STATUS_MAX` bytes; `NULL` if not wanted).  While it waits, the task has the console, if this program has it |

```c
#include <stdio.h>
#include <stdlib.h>
#include <hydra.h>

int main (void)
{
    char msg[HY_STATUS_MAX];
    int task, code;

    if (system ("ls bin") != 0) {
        return 1;
    }
    task = hy_spawn ("mytool in.txt > out.txt");
    /* ... other work while it runs ... */
    code = hy_wait (task, msg);
    if (code != 0) {
        printf ("mytool failed: %d %s\n", code, msg);
    }
    return code;
}
```

A command line is at most 250 characters.  Its output comes out wherever this program's goes (redirect it in the line: `"prog > file"`).  A spawned task that's never waited for runs to its end on its own.

### **8. Time**

| Call | Gives |
| :--- | :--- |
| `time`, `localtime`, `strftime`, `mktime`, `difftime` | The Hydra's clock, to the second, as local time (the Hydra keeps local time; cc65's time zone is UTC, so `localtime` shows it as it is).  Set it by writing `/dev/time` (`echo 2026-09-30 14:05 > /dev/time`) |
| `clock` | Ticks since the program started: `CLOCKS_PER_SEC` is 200.  It keeps counting past the tick counter's wrap (5.5 minutes) as long as it's called at least that often |
| `sleep (s)` | Sleeps s seconds: other tasks run meanwhile |
| `hy_ticks ()`, `hy_sleep_ticks (n)` | The scheduler's 16-bit tick count (200 a second; it wraps), and a sleep of n ticks (up to 32767) |
| `hy_clock ()` | The clock as seconds since 2000-01-01 00:00:00 |
| `hy_yield ()` | Gives up the rest of this tick to the other tasks |

**Don't busy-wait.**  A loop that polls something (a key, the time) should `hy_sleep_ticks (1)` or `hy_yield ()` each time round: otherwise it takes the CPU from every other task, the shell included.

```c
#include <stdio.h>
#include <time.h>

int main (void)
{
    time_t now = time (0);
    char when[32];

    strftime (when, sizeof when, "%Y-%m-%d %H:%M:%S", localtime (&now));
    printf ("%s\n", when);
    return 0;
}
```

### **9. Tasks working together: semaphores**

The Hydra has 16 semaphores for the whole system, known by number (1-16), so one program can make a semaphore and hand its number to the programs it starts (as an argument or in the environment).  A **mutex** is a semaphore of 1 that only the task holding it can release.

| Call | Does |
| :--- | :--- |
| `hy_sem_new (n)` | A semaphore of n (0-255): n acquires before one has to wait.  Its number, or -1 (`EAGAIN`: all 16 in use) |
| `hy_mutex_new ()` | A mutex |
| `hy_sem_acquire (s)` | Take one, waiting (using no CPU) until there is one.  Ctrl-C ends the wait |
| `hy_sem_try (s)` | Take one if there is one (1), or 0 at once |
| `hy_sem_release (s)` | Give one back (a mutex: only its holder can) |
| `hy_sem_free (s)` | Free it: tasks waiting for it get -1 |

When a program ends, however it ends, the semaphores it made are freed and the mutexes it holds are released, so a crash can't leave one held.  Semaphores are for coordinating tasks (one waits for another's step, or they take turns at a device or a file); a program's tasks don't share memory, since each has its own RAM.

```c
int s = hy_sem_new (0);                     /* Nothing to take yet */
char cmd[40];

sprintf (cmd, "worker %d", s);              /* The worker releases s when it's ready */
hy_spawn (cmd);
hy_sem_acquire (s);                         /* Wait for it */
```

### **10. Errors**

A failed library call returns -1 (or `NULL`) and sets `errno`, as usual; `perror` and `strerror` describe it.  `_oserror` (in `errno.h`) keeps the Hydra's own error code ([the list](rom-layout.md#error-codes)), which says more, and `_stroserror (_oserror)` (`string.h`) gives it as text, as HyForth's `!IO ERR!` does (`"not found"`, `"disk full"` ...; `_poserror ("prog")` prints it after `prog: `):

| The Hydra's error | `errno` |
| :---------------- | :------ |
| `ERR_IO_NOT_FOUND` (`$70`) | `ENOENT` |
| `ERR_IO_BAD_FD` (`$71`) | `EBADF` |
| `ERR_IO_MODE` (`$72`), `ERR_IO_NOT_EMPTY` (`$83`), `ERR_IO_IS_DIR` (`$86`), `ERR_SEM_NOT_HELD` (`$63`) | `EACCES` |
| `ERR_IO_WOULD_BLOCK` (`$73`), `ERR_SEM_NONE` (`$61`), `ERR_SEM_BUSY` (`$62`) | `EAGAIN` |
| `ERR_IO_NO_FDS` (`$75`) | `EMFILE` |
| `ERR_IO_NAME` (`$77`), `ERR_IO_NOT_DIR` (`$85`), `ERR_SEM_BAD` (`$60`), `ERR_MEM_*` | `EINVAL` |
| `ERR_IO_BAD_REQ` (`$78`) | `ENOSYS` |
| `ERR_IO_DEVICE` (`$79`), `ERR_IO_NOT_READY` (`$7E`), `ERR_IO_NOT_FS` (`$80`) | `ENODEV` |
| `ERR_IO_BROKEN` (`$7A`), `ERR_IO_MEDIA` (`$7F`) | `EIO` |
| `ERR_IO_FULL` (`$81`) | `ENOSPC` |
| `ERR_IO_EXISTS` (`$82`) | `EEXIST` |
| `ERR_IO_BUSY` (`$84`) | `EBUSY` |
| `ERR_IO_NOT_EXEC` (`$87`) | `ENOEXEC` |
| `ERR_OUT_OF_MEMORY` (`$02`) | `ENOMEM` |
| `ERR_SEM_FULL` (`$64`) | `ERANGE` |
| Anything else | `EUNKNOWN` |

```c
FILE* f = fopen ("data/scores", "r");

if (f == 0) {
    perror ("data/scores");                 /* data/scores: No such file or directory */
    hy_exits ("no scores");
}
```

### **11. Memory**

* **The heap** is `malloc`, `calloc`, `realloc` and `free`, between the program's BSS and its stack.  `_heapmemavail ()` gives the free bytes, `_heapmaxavail ()` the biggest block.  `malloc` returns `NULL` when it's out: check it.
* **The C stack** (locals, arguments) is 2K by default.  cc65 doesn't check it unless asked: a stack that overflows runs into the heap and corrupts it, with confusing results.  Build with `HYC_CFLAGS=--check-stack` while developing, and an overflow ends the program with exit status 4.  Large locals (`char buf[512]`) and deep recursion are what fill it: make big buffers `static`, or `malloc` them.  A different size: `HYC_LDFLAGS=-D __STACKSIZE__=$0C00`.
* **More room:** `-D __RAMTOP__=$7C00` gives the program all of its task RAM (29K), but then `MM_ALLOC` has no task RAM pages left to hand out (bank memory, below, still works).
* **Bank memory:** a task can have up to 16 banks of 8K per installed RAM module (typically well over 100K), seen one at a time at `$8000-$9FFF`.  The MMU hands them out (`MM_ALLOC` with `AI_PAGED`, then `MM_LOCK` to map one: [memory.md](memory.md)).  The library has no C calls for them yet; reach them with a little assembly ([below](#12-assembly-in-a-c-program)).  Two rules: the window only shows the bank while it's mapped, and **never hand the IO calls a buffer in the window** (the IO layer maps its transfer area there): copy through a buffer below `$7000`.

### **12. Assembly in a C program**

Put the assembly in a `.s` file and name it on `hyc.bat`'s line after the C files.  `lib/hydra.inc` has the OS's calls (the `$F8xx` thunks), their zero-page parameters and constants.

**cc65's calling convention** (`__fastcall__`, the default):
* The last argument is in `.A` (low) and `.X` (high); the others are on the C stack, which `popa` (a byte) and `popax` (a word) take off, last first.  A `long` argument or result has its high word in `sreg`.
* The result is in `.A` and `.X` (an `int`: set `.X`, even for a `char`).
* A function may change `.A`, `.X`, `.Y`, `ptr1`-`ptr4`, `tmp1`-`tmp4`, `sreg` and `regsave`; it must leave `sp` (the C stack pointer) and `regbank` as they were.
* Its C name gets an underscore: `getkey` is `_getkey`.

**The OS's calls** keep their own conventions ([the API index](rom-layout.md#api-index-the-thunks)): registers in, C = 1 for an error with the code in `.A`.  They're callable from any task.

A non-blocking key from stdin (`READ_CHAR`: C = 1 with a key in `.A`):

```
; getkey.s - int getkey (void): a key from stdin if one has been typed, or -1, without waiting

        .export     _getkey
        .include    "hydra.inc"

        .code

_getkey:
        jsr         READ_CHAR           ; C = 1: .A = the key
        bcc         @none
        ldx         #0                  ; (An int: .X is the high byte)
        rts
@none:
        lda         #$FF                ; -1
        tax
        rts
```

```c
int getkey (void);                      /* getkey.s */

    while ((k = getkey ()) < 0) {
        hy_sleep_ticks (2);             /* (Don't spin: let the others run) */
    }
```

```
programs\c\hyc.bat game.c getkey.s
```

`lib/sys/hydra.s` and `lib/io/fileio.s` have more examples: calls with arguments on the C stack, returning a `long`, setting `errno` on an error (`jmp ___mappederrno` with the OS's error in `.A`).

### **13. Performance**

The CPU does about 3.6 million simple operations a second, and cc65's code isn't dense or fast, so it pays to know what costs:
* **Printing** is limited by the serial line first: at 9600 baud a character takes 1 ms, so 100 lines of 20 characters take 2 s whatever the program does.  After that, formatting: a `printf` of a short line costs about 4 ms of CPU, `sprintf` and one `write` about 2.5 ms (each `write` is an IO request, about 2,000 cycles).  Faster lines: `stty` a higher rate on the Hydra and the terminal (HyForth's `"b19200" stty`).
* **Types:** `unsigned char` is the fastest type, then `int`; `long` arithmetic is several times slower, and multiply and divide are subroutines.  Count loops with `unsigned char` when they're under 256.
* **Locals and arguments** live on the C stack and are reached through a pointer: statics are faster (`static` locals, or `-Cl` to make every local static, if nothing is recursive).  `-Or` puts `register` variables in the zero page.
* **Arrays of structs:** indexing multiplies; walking a pointer is cheaper.
* **IO:** each `read` or `write` is a request to a server task (about 2,000 cycles plus the data); read and write in blocks, not a byte at a time.  `fgetc` and `fputc` are one request per byte.
* **Inner loops** that matter go into assembly ([above](#12-assembly-in-a-c-program)).

### **14. Debugging and testing**

* **`printf`** to stderr (`fprintf (stderr, ...)`): it goes to the console even when stdout is redirected.
* **The exit status** says how a program ended: 130 (Ctrl-C), 137 (killed), 4 (a stack overflow, with `--check-stack`).
* **The emulator** runs the same ROM and program, with tools the board hasn't got ([emulator.md](../tools/emulator.md)):
  * scripted runs: `--input` types a line (`'\w\w\wmyprog a b\r'`: `\w` waits for the boot) and `--cycles` sets how long, so a run can be repeated exactly;
  * `--watch ADDR@TASK` reports every write to an address in a task's RAM (find a variable's address in `bin\NAME.map`), `--dump ADDR:LEN@TASK` shows memory after the run, `--pc ADDR` the registers each time the PC gets there, `--trace N` the last instructions;
  * a program runs in the highest free task (usually `B`): `ps` shows it, and `hy_task ()` tells the program.
* **Tests:** `sim/regress.js` boots the emulator with a card image and checks the output.  Its `c-programs` test runs the samples (`C_SAMPLES`); a test for a program of your own follows the same pattern ([emulator.md](../tools/emulator.md#regression-tests)).

### **15. Limits and gotchas**

| | |
| :- | :- |
| No `float`, `double` or `long long` | Integers, `long`, fixed point |
| `char` is unsigned | Compare with `(signed char)` casts, or build with `-j` |
| 26K for code, data, heap and stack | Watch the map file; `malloc` returns `NULL` when it's out |
| A 2K C stack, unchecked | `--check-stack` while developing; big buffers `static` |
| 15 arguments, 63 characters of line | Longer input: a file, or stdin |
| Names 31 characters, paths 64 | `HY_NAME_MAX`, `HY_PATH_MAX` (not `FILENAME_MAX`, 17) |
| 12 fds a task, 3 used | Close what you open |
| `rename` within a directory | Copy and remove to move a file |
| `getenv`'s buffer is shared | Copy the value before the next `getenv` |
| `atexit` functions don't run on Ctrl-C or a kill | Don't save state in them |
| HyForth's words shadow program names | `run prog.hyx`, or another name |
| A function's locals: 256 bytes at most ("Too many local variables") | Make big arrays `static`, or `malloc` them |
| Backspace reaches `fgets` | A `readline` of your own ([above](#6-the-console-stdio-and-conio)) |
| `/` and `/dev` can't be listed | List a card: `/sd/0`, `.` |
| No signals, no `fork` | `hy_spawn`, `system`; Ctrl-C ends the program |
| No threads | Tasks (`hy_spawn`) and semaphores |
| A buffer in `$8000-$9FFF` can't go to IO | Copy it below `$7000` first |

### **16. Reference: `hydra.h`**

| Call or constant | Does |
| :--------------- | :--- |
| `HY_PATH_MAX` (65), `HY_NAME_MAX` (32) | A path's and a name's buffer, with the 0 |
| `HY_ENV_MAX` (255), `HY_STATUS_MAX` (31), `HY_STAT_SIZE` (48) | A variable's value; an exit message's buffer; the OS's stat record |
| `HY_TICKS_PER_SEC` (200), `CLOCKS_PER_SEC` | The scheduler's tick |
| `unsigned hy_ticks (void)` | The tick count (it wraps) |
| `void hy_sleep_ticks (unsigned ticks)` | Sleep, up to 32767 ticks |
| `void hy_yield (void)` | Let the other tasks run |
| `unsigned long hy_clock (void)` | The clock: seconds since 2000-01-01 |
| `unsigned char hy_task (void)` | This program's task (1-15) |
| `int hy_spawn (const char* cmd)` | Start a command line: its task, or -1 |
| `int hy_wait (int task, char* msg)` | Wait for a task: its code, and its message |
| `int hy_kill (int task)` | End a task and the tasks it started (its status: 137) |
| `int hy_bind (new, old, flags)`, `int hy_mount (dev, old, flags, spec)` | The namespace, as Plan 9's `bind` and `mount`: names under `old` stand for names under `new` (or are device `dev`'s, such as `"zero"`; `spec`: `NULL`, or what the server serves there, as Plan 9's, `hy_mount ("hfs", "/a", HY_MREPL, "r")`).  `flags`: `HY_MREPL` (replace `old`'s entries), `HY_MBEFORE`, `HY_MAFTER` (a union's member, before or after the others), `HY_MCREATE` (files made in the union go to it).  0, or -1 |
| `int hy_unmount (new, old)`, `int hy_hide (path)` | Take `old`'s member `new` out (`NULL`: all of `old`'s entries); make nothing under `path` found.  0, or -1 |
| `void hy_exits (const char* msg)` | End with a message (code 1), or success (`NULL`, `""`) |
| `int hy_sem_new (unsigned char count)`, `int hy_mutex_new (void)` | A semaphore (1-16), or -1 |
| `int hy_sem_acquire (s)`, `hy_sem_try (s)`, `hy_sem_release (s)`, `hy_sem_free (s)` | [Semaphores](#9-tasks-working-together-semaphores) |
| `int fstat (int fd, struct stat* st)` | `stat` for an open file |
| `int hy_dirstat (DIR* dir, struct stat* st)` | The last `readdir` entry, as `stat` |
| `int isatty (int fd)` | Is it the console? |
| `int setenv (name, value, overwrite)`, `int unsetenv (name)` | The environment |
| `S_IFDIR`, `S_ISDIR (m)` | A directory, in `st_mode` |
| `COLOR_BLACK` ... `COLOR_BRIGHTWHITE` | conio's 16 colours |
| `CH_ENTER`, `CH_ESC`, `CH_DEL`, `CH_CURS_UP` ... `CH_F4` | `cgetc`'s keys |
| `CH_HLINE`, `CH_VLINE` | `chline`'s and `cvline`'s characters |

### **17. Working on the library**

`programs/c/`:

| Path | What |
| :--- | :--- |
| `hydra.cfg` | The linker config: the header, the memory, `__STACKSIZE__`, `__RAMTOP__`, the zero page |
| `include/hydra.h`, `include/snd.h` | The Hydra's own calls and constants; the sound chip's |
| `lib/hydra.inc` | The OS's calls, zero page and constants for the library's assembly.  The `c-programs` test checks each name against the ROM's build, so keep it in step with `os_rom/include` |
| `lib/crt/` | `crt0.s` (the header, start-up and `exit`), `mainargs.s` (`argc`, `argv`) |
| `lib/io/` | Files: `fileio.s` (the raw IO calls), `read.c` and `write.c` (the console's line ends), `open.c`, `lseek.c`, `stat.c`, `dirent.c`, `isatty.c`, `sysfile.s` (remove, rename, mkdir, rmdir), `_cwd.s`, `oserror.s` (errors to `errno`), `oserrlist.s` (the errors as text: `_stroserror`) |
| `lib/env/` | `getenv.c`, `putenv.c` |
| `lib/conio/` | `conio.c` (ANSI output, keys), `conglue.s` (the entry points cc65's own conio code calls), `cursor.c` |
| `lib/snd/` | `snd.c`: `snd.h` over `/dev/snd`; `sndplay.c`: `snd_play` |
| `lib/sys/` | `hydra.s` (tasks, semaphores, ticks, sleep), `system.c`, `gettime.c`, `clock.s` |
| `samples/` | `hello.c`, `upper.c`, `code.c`, `keys.c`, `tones.c`, `jukebox.c`, `ctest.c` (the library's own test) |

* **A module replaces cc65's module of the same file name** (`make.bat` adds the objects to a copy of `none.lib`): `getenv.c` makes `getenv.o`, which takes the place of cc65's.  Name a new module after the cc65 module it replaces, or something cc65 doesn't have (`ar65 t lib\hydra.lib` lists them).  Two files with the same base name (`conio.c` and `conio.s`) would make the same object.
* **cc65's common code calls some routines expecting `ptr1`-`ptr3` and `tmp1` to survive** (conio's `cputs` and `cprintf` call `cputc` and `gotoxy`): a C version of one of those needs an assembly wrapper that keeps them, as `conglue.s` does.
* **A new call in the ROM** gets a line in `lib/hydra.inc` (the test checks it) and, for C, a function in `lib/sys/hydra.s` and a prototype in `hydra.h`.
* **Test it:** `ctest.c` checks the library from the inside (its last line is `N failed`), and `sim/regress.js c-programs` runs it in the emulator with the other samples.  Run the whole suite (`node sim/regress.js`) before committing.
