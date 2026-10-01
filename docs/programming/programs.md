## **Programs: Hydra executables (`.hyx`)**

A Hydra executable is a program on a card that the shell loads and runs in a task of its own: `run hello.hyx`, or just `hello` ([HyForth](../using/hyforth.md#the-shell-directories-files-and-programs)).  It's machine code linked for a fixed address in task RAM, with a 16-byte header in front.  Write one in assembly (a sample is in `programs/`), or in C ([below](#c-programs): `programs/c/`).

### **The file**

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 4 | Magic: `HYX1` |
| 4 | 2 | Load address |
| 6 | 2 | Length of the code (the bytes after the header) |
| 8 | 2 | Entry point |
| 10 | 2 | Flags: 0 |
| 12 | 2 | BSS: the bytes after the code that the loader clears (0: none) |
| 14 | 2 | Top: the end of the RAM the program uses directly (its heap and stack's too); 0: the end of its code and BSS |

Then the code, which is loaded at the load address as it is: a program isn't relocatable.  The code, its BSS and its top must fit in `$0800-$7BFF` (`ERR_IO_NOT_EXEC`, `$87`, otherwise).  As in Plan 9's `a.out` header, the BSS isn't in the file: the loader clears it.  An older header, with 4 zero bytes at offset 12, still loads.  The constants are in `os_rom/include/shell.inc` (`HYX_*`).

`run` tells a program from a script by the header, not by the name: a file without `HYX1` at its start is run as a HyForth script, except a **song** (a ZSM file, which starts with `zm`), which the ROM's song player plays in a task of its own ([HyForth's play](../using/hyforth.md#tasks-and-the-console)).  Name them `.hyx` (`.zsm`) anyway, so the shell finds them by name.

### **What a program gets**

* **A task of its own**, started with copies of the shell's fds (stdin, stdout and stderr: fds 0-2), namespace and current directory.  So a program can read and write files by relative names, and be a stage of a pipeline (`run hello.hyx | wc`).
* **Its RAM:** task RAM `$0800-$7BFF`.  Its code is loaded at its load address, its BSS is cleared, and the MMU's page floor is set just above the higher of its BSS's end and its top, so the MMU's allocations come from the pages above them.  For RAM beyond its end, a program allocates it (`MM_ALLOC`), or raises the floor (`MM_SET_FLOOR`) to use it directly ([memory.md](memory.md#the-mmu-a-tasks-own-memory)).
* **Its arguments:** at the entry point, `.A.Y` points to them: the rest of the line after its name (`prog a b` gives `a b`), zero-terminated, at `HYX_ARGS` (`$0110`, 63 characters at most; empty if there are none).  They travel from the shell through a pipe on fd 10 (`SH_ARGS_FD`), which the loader reads and closes.
* **Its name:** as it was run (`hello`, `bin/hello.hyx` ...), zero-terminated, at `HYX_NAME` (`$0150`, 31 characters at most): C's `argv[0]`.  It comes through the same pipe, after the arguments.
* **Its environment:** a copy of the shell's: `/env/NAME` files it can read (and change, for itself and the tasks it starts).
* **Zero page** `$E0-$FF` (`PROGRAM_ZP`: the OS's stays below it; the bytes between the OS's end, `$B7` in this build, and `$E0` may go to the OS in a later ROM).  The calls' zero-page parameters (`ZP_IO_BUF` ...) are at fixed addresses ([the API index](rom-layout.md#zero-page)).  And the stack page: the loader has used a few bytes of the stack, `$0100-$010F` held the header, the arguments are at `$0110-$014F`, and the name at `$0150-$016F`.
* **The OS calls** through the `$F8xx` thunks ([the API index](rom-layout.md#api-index-the-thunks)): the entry point is called on ROM page 0, where they are.
* **The console** while it runs, if the shell had it: its keys come to the program, and **Ctrl-C ends it** (a task without a break handler ends on a break; `TASK_SET_BREAK` sets one: [tasks.md](tasks.md#signals-break-and-kill)).  The shell's prompt comes back when it ends.

**Ending:** the program returns (`rts` from its entry point), or ends with an exit status (`TASK_EXITS`), or it's killed (Ctrl-C, Ctrl-\\, `kill`).  Either way its task ends: its fds are closed (stdout's buffered output goes out first), and its memory is freed.

**Its exit status** (Plan 9's `exits`): a code, 0-255, and a message of up to 30 characters.  `TASK_EXITS` ends the task with them (`.A` = the code, `ZP_IO_BUF` = the message, or 0 for none).  A program that returns ends with 0; a break with 130, `interrupt`; a kill with 137, `killed`.  The shell keeps it: HyForth's `status`, and `/env/status` (the message, or the code if there's none; empty for 0: Plan 9's `$status`).  A task that started another gets it from `TASK_JOIN` ([tasks.md](tasks.md#exit-statuses)).

### **Building one**

`programs/asm/` has a sample, `samples/hello.s`, and what it's built with (C programs are in `programs/c/`: below):

| File | What |
| :--- | :--- |
| `hyx.inc` | `HYX_HEADER entry`: the header, in its own segment, with the load address and length filled in by the linker |
| `hyx.cfg` | The ld65 config: the header, then `CODE`, `RODATA` and `DATA` at `$0800`.  There's no BSS segment: everything is in the file (variables go in `DATA`) |
| `make.bat` | Runs `build.js asm`: ca65, then ld65 with `hyx.cfg`, for each `samples\*.s`: `bin\hello.hyx` |

```
.include "hyx.inc"
WRITE_CHAR      = $F803
            HYX_HEADER  start
.code
start:      lda         #'!'
            jmp         WRITE_CHAR              ; its rts ends the program
```

From another assembler, make a raw binary for a fixed address, then put the header on it with `sim/tools/mkhyx.js` (Node.js):

```
node sim/tools/mkhyx.js prog.bin prog.hyx $0800 [ENTRY]     the header on a binary (ENTRY: default the load address)
node sim/tools/mkhyx.js --info prog.hyx                      show a .hyx file's header
```

### **C programs**

`programs/c/` has a C library for [cc65](https://cc65.github.io/): cc65's own C library (stdio, strings, malloc, time ...) on the Hydra's calls, and the Hydra's own (`hydra.h`).  A C program is a `.hyx` like any other.  **[The C Programmer's Guide](c.md)** covers writing one in depth; this section is the summary.

```
programs\c\make.bat                            build the library (lib\hydra.lib) and the samples (bin\*.hyx)
programs\c\hyc.bat prog.c [more.c] [more.s]    build a program of your own: bin\prog.hyx
```

They run `build.js` (`node build.js c`, `node build.js prog prog.c ...` on any OS), which finds cc65 in `CC65_HOME`, on the `PATH`, or in `C:\source\cc65\win64_snapshot`.  `hyc.bat` compiles with `cc65 -t none --cpu 65C02 -O` and links with `hydra.cfg` and `lib\hydra.lib`.

| File | What |
| :--- | :--- |
| `hydra.cfg` | The ld65 config: the header, then the program at `$0800`; zero page `$E0-$FF` |
| `include/hydra.h` | The Hydra's own calls and constants (below) |
| `lib/hydra.inc` | The calls' addresses, zero-page parameters and constants, for the library's assembly (the `c-programs` test checks them against the ROM) |
| `lib/crt/` | `crt0.s`: the header (with its BSS and top), the start (the stack, the constructors, `main`), and the end (`exit`: the destructors, then `TASK_EXITS` with the status); `mainargs.s`: `argc` and `argv` |
| `lib/io/` | Files: `read`, `write`, `open`, `close`, `lseek`, `remove`, `rename`, `mkdir`, `rmdir`, `chdir`, `getcwd`, `stat`, `fstat`, `isatty`, `opendir` and the rest of `dirent.h`, errno and `_oserror` |
| `lib/env/` | The environment: `getenv`, `putenv`, `setenv`, `unsetenv` |
| `lib/conio/` | `conio.h` on the console (an ANSI terminal) |
| `lib/snd/` | `snd.h`: the YM2151, through `/dev/snd` and the ROM's sound library; songs (`snd_play`) |
| `lib/sys/` | `system`, `clock`, `clock_gettime` (so `time`), `sleep`; `hydra.h`'s calls (tasks, exit statuses, semaphores, ticks) |
| `samples/` | `hello.c` (arguments, arithmetic, the heap, the clock), `upper.c` (a filter: stdin to stdout in capitals), `code.c` (an exit status from its argument), `keys.c` (conio: the screen and raw keys), `tones.c` (`snd.h`: patches, notes, volume, a bend, drums), `jukebox.c` (a song in the background, stopped), `ctest.c` (the library's test) |

A module in `lib/` replaces cc65's module of the same name (`make.bat` adds them to a copy of cc65's `none.lib`), so name a new one after the cc65 module it replaces, or something cc65 doesn't have.

**What a C program gets:**
* **The standard library:** `printf` and the rest of stdio on the Hydra's files (`stdin`, `stdout`, `stderr` are fds 0-2, as the shell gives them, so `prog < in.txt | wc` works), `malloc`, strings, `time` and `localtime` (the Hydra's clock: to the second, as local time), `sleep`, and the file and directory calls above.  Names are relative to the current directory unless they start with `/`.
* **Its RAM:** `$0800-$6FFF`: the program, its BSS, the heap, and a 2 KB C stack at the top (`__RAMTOP__` and `__STACKSIZE__` in `hydra.cfg`).  The header gives the loader the BSS's size (it clears it) and `__RAMTOP__` as the top (the MMU's floor goes there), so the file holds only the code and data, and the MMU's `MM_ALLOC` pages come from above `$7000`.
* **`argc` and `argv`:** `argv[0]` is the program's name as it was run; the arguments are split at spaces, `"a b"` as one (15 at most).
* **The environment** (Plan 9's: each variable is a file, `/env/NAME`, in a copy of the shell's environment): `getenv` reads one (into a static buffer, up to `HY_ENV_MAX` characters), and `setenv`, `putenv` (`"NAME=value"`; `"NAME"` alone removes it) and `unsetenv` change it.  The changes are the program's own, and the commands it runs get them.
* **Files and directories:** `stat` and `fstat` (`st_size`, `st_mode`: `S_ISDIR`, `S_IREAD`, `S_IWRITE`; `st_mtime` from HydraFS's stamps), `opendir`/`readdir`/`closedir` (`d_name`; `hy_dirstat` gives the entry's stat without opening it), `isatty`.
* **The console** through `conio.h` (below), or stdio.  stdio on the console acts as a Unix terminal: an LF written goes out as CR LF, and Enter (CR) reads as `\n` (`read.c`, `write.c`).
* **Commands:** `system ("ls /bin | wc")` runs a command line as the prompt would (in a command shell, `SHELL_CMD`: Plan 9's `rc -c`), waits, and returns its exit code; `hy_spawn` starts one without waiting, and `hy_wait` waits for it and gets its code and message.
* **Time:** `time` and `localtime` (the Hydra's clock, to the second, as local time); `clock` (ticks since the program started: `CLOCKS_PER_SEC` is 200); `sleep`.
* **Errors:** a failed call returns -1 and sets `errno`; `_oserror` has the Hydra's own error ([error codes](rom-layout.md#error-codes)).
* **Ending:** `main` returns, or `exit(code)`: the destructors (and `atexit`'s) run, and the task ends with that exit status.  `hy_exits ("message")` ends it with a message too (and code 1; `NULL` or `""`: success).  The shell keeps it (`status`, `$status`).
* **Not yet:** `rename` only within a directory; `FILENAME_MAX` is cc65's 17 for this target, so use `HY_PATH_MAX` (65) for paths, `HY_NAME_MAX` (32) for a name.

**`hydra.h`:**

| Call | Does |
| :--- | :--- |
| `hy_sem_new (count)`, `hy_mutex_new ()` | A semaphore (1-16), or a mutex ([semaphores](tasks.md#semaphores)) |
| `hy_sem_acquire (s)` | Take one, waiting until there's one |
| `hy_sem_try (s)` | Take one if there is one: 1; else 0, at once |
| `hy_sem_release (s)`, `hy_sem_free (s)` | Give one back; free it |
| `hy_ticks ()`, `hy_sleep_ticks (n)` | The scheduler's tick count (`HY_TICKS_PER_SEC`, 200 a second); sleep n ticks |
| `hy_yield ()` | Let the other tasks run |
| `hy_clock ()` | The clock: seconds since 2000-01-01 |
| `hy_task ()` | This task's number (1-15, as `ps` shows it) |
| `hy_spawn (cmd)` | A command line in a task of its own, not waited for: its task, or -1 |
| `hy_wait (task, msg)` | Wait for a task this one started to end: its exit code (0-255), and its message into `msg` (`HY_STATUS_MAX` bytes; `NULL`: not wanted) |
| `hy_exits (msg)` | End the program with a message (Plan 9's `exits`) |
| `hy_kill (task)` | End a task and the tasks it started (its status: 137, `killed`) |
| `fstat (fd, st)`, `hy_dirstat (dir, st)`, `isatty (fd)` | What POSIX has and cc65's headers don't for this target |
| `setenv`, `unsetenv` | (Also POSIX's) |
| `COLOR_*`, `CH_*` | conio's colours (ANSI's 16) and keys |

**conio:** the console is an ANSI terminal on the serial port, so `conio.h` works through its sequences: `clrscr`, `gotoxy` and the rest, `wherex`/`wherey` (of what conio writes), `textcolor`, `bgcolor` (`COLOR_*`), `revers`, `cursor`, `chline`/`cvline` (`-` and `|`), `cclear`, `cputs`, `cprintf`, `cscanf`.  `screensize` is `$COLUMNS` x `$LINES`, or 80 x 24.  `cgetc` and `kbhit` read the keys raw: conio opens `/dev/cons/ctl` and writes `rawon` ([Plan 9's consctl](io.md#the-console)): no echo, each key as it's typed, no Ctrl-D.  The terminal's cursor, editing and function keys come as one code each (`CH_CURS_UP` ... `CH_F4`).  Raw ends when the program does.  conio's output and stdout's keep their order (both go out through `WRITE_CHAR`).

```
#include <stdio.h>

int main (int argc, char* argv[])
{
    printf ("Hello from C: %d arguments\n", argc - 1);
    return 0;
}
```

### **Putting it on a card**

* **In the emulator, or on a real card's image:** `node sim/tools/hydrafs.js mkdir card.img bin`, then `node sim/tools/hydrafs.js put card.img programs/asm/bin/hello.hyx bin` ([the card tool](../tools/emulator.md#hydrafs-card-images)).
* **On the Hydra:** copy it from one card to another with `cp`.

A program in the current directory, or in `/bin` on the current directory's card, runs by its name: `hello`.

### **How `run` does it** (`os_rom/shell/run.s`, BIOS ROM page 7)

1. The shell opens the file, and reads its start: an `HYX1` header is checked (it has to fit in task RAM); anything else is a script.
2. The file goes on fd 11 (`SH_RUN_FD`), back at its start, and the arguments into a pipe on fd 10 (`SH_ARGS_FD`); the shell starts a task (`TASK_RUN`) at the loader, `SH_LOAD`, which inherits both.
3. The shell waits (`TASK_JOIN`): it hands the console to the new task if it has it (`CONS_SET_FG`), makes itself the task's parent, and pauses, as `TASK_START` does.  The task's end wakes it, and `CONS_RELEASE` gives the console back (and the shell takes it back itself too, for a task that ended before it had it).  It keeps the task's exit status (`status`, `/env/status`).  A line ending in `&` isn't waited for: the shell prints the task's number (`[B]`), puts it in `/env/apid`, and goes on; HyForth's `wait` waits for it later ([HyForth](../using/hyforth.md#background-tasks-and-exit-statuses)).
4. The loader, in the new task, reads the header and the code into place, clears the BSS, sets the MMU's page floor above it (or the header's top), closes fd 11, reads the arguments and the name from fd 10 into `HYX_ARGS` and `HYX_NAME` and closes it, and calls the entry point on ROM page 0 with `.A.Y` pointing to the arguments.  When that returns, the loader returns, and the task ends (exit status 0).

A script (`.hys`) is run by HyForth itself: a copy of the shell's task (`TASK_CLONE`) reads it from fd 11 as `include` would, and ends at its end; the shell waits for it the same way.
