## **HyForth: the Hydra's shell**

HyForth is the Hydra-16's shell and programming language: a small Forth that starts in task 1 at boot, and prompts with the card and directory it's in (`0:/> `).  It began as AGSB's Forth engine, adapted by Patrick Struthers for the Hydra.  Sources: `os_rom/hyforth/`.  See also [WOZMON](wozmon.md) and the [Programmer's Guide](../programming/README.md).

### **Contents**
1. [The basics](#the-basics)
2. [Numbers](#numbers)
3. [Strings](#strings)
4. [Defining words](#defining-words)
5. [Control flow: the training scripts](#control-flow-the-training-scripts)
6. [Word reference](#word-reference)
7. [The base and its libraries](#the-base-and-its-libraries)
8. [The shell: directories, files and programs](#the-shell-directories-files-and-programs)
9. [Files and devices](#files-and-devices)
10. [Pipelines](#pipelines)
11. [Tasks and the console](#tasks-and-the-console)
12. [Background tasks and exit statuses](#background-tasks-and-exit-statuses)
13. [Errors and keys](#errors-and-keys)
14. [How HyForth uses memory](#how-hyforth-uses-memory)

### **The basics**

HyForth reads a line, then runs each word in it, left to right.  Numbers go on the **data stack**; words take their arguments from it and leave results on it.

```
0:/> 1 2 + .
 0003
0:/> words
```

* **Words are separated by spaces.**  Names are case-sensitive.
* **Lines** can be up to 255 characters.
* **`.`** prints and drops the top of the stack, and **`.S`** shows the whole stack.  The format is `S<address> <depth> <items, top first>`, and `xS` clears it.
* **`words`** lists every word: the ones you've defined, then the base's, then each loaded [library's](#the-base-and-its-libraries).
* **`bye`** leaves HyForth for [WOZMON](wozmon.md) (in a script that `run` started, it ends the script).  Ctrl-\\ starts a fresh HyForth.
* **The prompt** shows the current card and directory: `0:/games> ` is `/games` on card 0 ([the shell](#the-shell-directories-files-and-programs)).

Stack effects are written `( before -- after )`, with the top of the stack on the right.

### **Numbers**

* **Input** is decimal: `11`, `-2`.  A `$` prefix gives hex (`$B`, `$FFFF`), and `%` binary (`%101`).
* **Output** is 4 hex digits at first: `11 .` prints `000B`.  After `decimal`, `.` prints in decimal, signed (`-264 .` prints `-264`), and `u.` unsigned (`$FFFF u.` prints `65535`); `hex` goes back.  Input isn't affected: it's decimal, with `$` for hex, either way.
* **Size:** numbers are 16 bits.  Decimal input is signed, `-32768` to `32767` (a bigger one isn't a number: `!UNK WORD!`); hex input covers `$0000-$FFFF`.
* **Inside a definition**, a number is compiled into the word, as in any Forth: `: x 65 . ;` prints `0041` each time `x` runs.  (`lit [ 65 , ]` does the same by hand.)

### **Strings**

`"text"` makes a string (in HyForth's memory records) and pushes its reference.  `q^text^` is the same, for text with a `"` in it:
* `"hello world" .sz` prints it: spaces and all, up to the closing `"`.  `""` is an empty string.
* Words that take a name take a string like this: `"/dev/zero" 1 open`.
* A string starts at the start of a word: `"` or `q^` after a space.  There are no escapes: a `"` can't go in a `"..."` string (use `q^...^`), nor a `^` in a `q^...^` one.
* Like numbers, a string is made as it's read, even while compiling, so it isn't compiled into a definition.

### **Defining words**

```
0:/> : sq dup * swap drop ;
0:/> 5 sq .
 0019
0:/> var x   7 x !   x @ .
 0007
0:/> 9 cons nine   nine .
 0009
```

| Word | Stack | Does |
| :--- | :---- | :--- |
| `: name ... ;` | | Define a word |
| `I` | | After `;`: make the last word *immediate* (runs while compiling; used by control words) |
| `[`, `]` | | Stop / resume compiling, inside a definition |
| `,` | `( n -- )` | Compile `n` into the definition |
| `lit` | | Compile the next cell as a number (`lit [ 65 , ]`) |
| `var name` | | A variable: `name` pushes its address |
| `n cons name` | `( n -- )` | A constant: `name` pushes `n` |
| `exit` | | Return from the current word |
| `exec` | `( a -- )` | Run the word at address `a` |

### **Control flow: the training scripts**

The control words aren't built in: they're defined in HyForth itself, by the **training scripts** in the paged ROM.  Load them first:

```
0:/> ftrain autoload
0:/> : t 3 0 do i . loop ;   t
 0000 0001 0002
0:/> : s 3 = if 1 . else 2 . then ;   3 s 4 s
 0001 0002
0:/> : c 0 begin 1 + dup 5 = until . ;   c
 0005
```

`ftrain autoload` defines:
* `2*`;
* `?branch` and `branch`, which the others compile;
* `if` / `else` / `then`;
* `begin` / `until`, `begin` / `again`, `begin` / `while` / `repeat`;
* `do` / `loop` with `i`, `j`, `k` (loop indexes);
* `mdump`, `decsz!`, `256/`.

`autoload ( addr -- )` loads any such list: zero-terminated lines, then an empty one.  `cload ( addr -- )` loads one script from memory, and `bload ( addr -- )` loads native code; `bltest bload` loads the built-in sample.

### **Word reference**

**Stack:**

| Word | Stack |
| :--- | :---- |
| `dup` | `( a -- a a )` |
| `drop` | `( a -- )` |
| `swap` | `( a b -- b a )` |
| `over` | `( a b -- a b a )` |
| `rot` | `( a b c -- b c a )` |
| `nip` | `( a b -- b )` |
| `tuck` | `( a b -- b a b )` |
| `2dup`, `2drop`, `2over` | Pairs |
| `pick`, `snip` | Reach into / remove from the stack |
| `>r`, `r>` | To / from the return stack |
| `xS`, `xR` | Clear the data / return stack |
| `.S`, `.R` | Show the data / return stack |
| `?S`, `?R` | Is the data / return stack empty? |

**Arithmetic and logic:**

| Word | Stack | Notes |
| :--- | :---- | :---- |
| `+`, `-` | `( a b -- a+b )`, `( a b -- a-b )` | |
| `*` | `( a b -- hi lo )` | The 32-bit product: `lo` on top (`10 3 * .` prints `001E`; `0000` stays below) |
| `/` | `( a b -- rem quot )` | Unsigned: the quotient on top, the remainder below |
| `min` | `( a b -- max min )` | Both stay: the smaller on top |
| `max` | | The larger on top |
| `and`, `or`, `xor`, `nand`, `not`, `neg` | | Bitwise; `neg` is the one's complement |
| `<<`, `>>` | `( w n -- w' )` | Shift left / right by n |
| `tbit`, `sbit`, `cbit` | `( n b -- ... )` | Test, set, clear bit b |
| `=`, `<>`, `<`, `>`, `<=`, `>=`, `0=` | `( a b -- flag )` | Flags are `$FFFF` (true) or `0` |
| `bool` | `( n -- flag )` | 0 stays 0; anything else becomes `$FFFF` |
| `TRUE`, `FALSE` | `( -- $FFFF )`, `( -- 0 )` | |
| `rand`, `rand32`, `rseed` | | Random numbers (`rseed ( s s -- )` seeds) |

**Memory:**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `@`, `!` | `( a -- w )`, `( w a -- )` | Fetch / store a 16-bit cell |
| `c@`, `c!` | `( a -- b )`, `( b a -- )` | Fetch / store a byte |
| `cells` | `( -- 2 )` | |
| `here`, `last`, `back`, `sp`, `rp`, `memptr`, `>in` | `( -- a )` | Addresses of HyForth's variables: `here @` is the next free dictionary byte |
| `memcpy` | `( start end dest -- )` | Copy `start`..`end` to `dest` |
| `dump` | `( addr n -- )` | Hex and ASCII dump of n × 256 bytes; waits for a key after each 256 |
| `free` | `( -- n )` | Bytes free between the dictionary and the MMU's lowest page |
| `malloc` | `( bytes type -- addr )` | A HyForth memory record (strings use these) |
| `mlen` | `( addr -- len )` | A record's length |
| `mktemp`, `purge0` | | Mark a record temporary; free the temporary ones |
| `halloc` | `( bytes flags -- h )` | An MMU allocation: flags 0 = task RAM, 1 = 8K RAM banks |
| `hfree` | `( h -- )` | Free it |
| `hlock` | `( h -- addr )` | Its address (a bank allocation: selects the bank at `$8000`).  One at a time |
| `hunlock` | `( h -- )` | Undo `hlock` |

**Input, output, terminal:**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `key` | `( -- c )` | Wait for a key (stdin); -1 at end of file |
| `emit` | `( c -- )` | Write a character (stdout) |
| `cr`, `spc` | | Newline; `spc` pushes 32 |
| `.`, `.C` | `( n -- )` | Print as a number (hex, or decimal after `decimal`: [numbers](#numbers)) / as two characters |
| `u.` | `( u -- )` | Print unsigned (hex, or decimal after `decimal`) |
| `decimal`, `hex` | `( -- )` | The base `.` and `u.` print in (hex at first) |
| `.sz` | `( sz -- )` | Print a string (`"..."` or `q^...^`) |
| `Acls`, `Ascr ( c r -- )`, `Acol ( c -- )` | | ANSI: clear the screen, move the cursor, set attributes |
| `in>`, `reset` | | Read the input buffer; reset it |

**System:**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `syscall` | `( addr a y -- x )` | Call machine code at `addr` with `.A` and `.Y` set, and BIOS ROM page 0 selected (as a program runs), so any thunk works ([API index](../programming/rom-layout.md#api-index-the-thunks)); pushes `.X` |
| `sys` | `( addr a x y -- a x y p )` | The same with every register, in and out, and the flags after it (`p`: C is bit 0, which the OS's calls set when they fail, with the error in `.A`).  E.g. `$F8EA 0 0 0 sys` gives the tick count in `a` and `y` |
| `disasm` | `( addr n -- )` | Disassemble n instructions |
| `mmtest` | | Run the MMU self test |
| `hwtest` | | Run the [hardware test](wozmon.md#the-hardware-test): it takes the machine over, and ends with a reset |
| `debug`, `s@` | | Toggle debug; the status word's address |
| `bye` | | Leave for WOZMON |
| `abort` | | Abort to the prompt |

### **The base and its libraries**

HyForth is a base language, plus libraries of words for the rest of the system.  The base is the Forth itself: the interpreter and compiler, the stacks, arithmetic and logic, memory and memory records, numbers and strings, `key` and `emit`, and loading scripts from memory.  Each library is a set of words with a dictionary chain of its own: in ROM, or read from a file on a card (below).  To find a word, the interpreter searches the words you've defined, then the libraries loaded from files, then the base, then each ROM library that's loaded.

**At startup** HyForth loads the base and every ROM library, which is everything the shell uses, and then runs the shell.  So the prompt works as it always has.  A new shell (`shell`) starts the same way.  The copy of the shell that runs a pipeline's left side, or a script (`run`), has the libraries of the shell that started it.

**A bare Forth** (`forth`) is a new HyForth task with only the base loaded: a plain `> ` prompt, no shell and no `boot.hys`.  It's there for Forth on its own; `lib` loads what it needs, and `lib all` makes it a full shell.

| Library | Words | Loads too |
| :------ | :---- | :-------- |
| `io` | `open`, `close`, `read`, `write`, `seek`, `ioctl`, `fdup2`, `pipe`, `create`, `mount`, `bind`, `unmount`, `ns`, `stty`, `stty?`, `ctl`, `ioerr` | |
| `files` | `cd`, `pwd`, `ls`, `rm`, `rmdir`, `mkdir`, `cp`, `mv` (and their stack forms, `(cd)` ...), `cat`, `wc`, `vols`, `mkfs`, `mkfs-full`, `mkfs-size`, `mkfs-part`, `relabel`, `fsck`, `fsfix` | `io` |
| `shell` | `prompt`, `include`, `run` (and `(include)`, `(run)`), `args`, `edit`, `echo`.  Also the shell's part of reading a line: the prompt's format, pipelines (`\|`), redirection (`>`, `>>`, `<`), and running a program for a word HyForth doesn't know | `io`, `files` |
| `tasks` | `shell`, `forth`, `fg`, `kill`, `sleep`, `ps`, `wait`, `sem`, `mutex`, `acquire`, `acquire?`, `release`, `-sem` | `io` |
| `sound` | `sndinit`, `sndtest`, `sndstop`, `ywrite`, `patch`, `note`, `noteoff`, `play` | `io` |
| `mem` | `halloc`, `hfree`, `hlock`, `hunlock` (MMU memory) | |
| `tools` | `dump`, `disasm`, `syscall`, `sys`, `mmtest`, `hwtest` | |
| `term` | `Acls`, `Ascr`, `Acol` (the ANSI terminal) | |

| Word | Does |
| :--- | :--- |
| `libs` | List the libraries: `forth` (the base), each ROM library, then the ones loaded from files.  A library in parentheses isn't searched |
| `lib name` | Load a library, and the ones it needs.  A name no ROM library has is a library file's (below) |
| `-lib name` | Unload a library: its words aren't found any more.  Words already compiled into definitions still run |
| `lib all`, `-lib all` | Every library: the ROM libraries, and the ones loaded from files |

```
0:/> -lib sound

0:/> libs
forth io files shell tasks (sound) mem tools term
0:/> sndtest

 !UNK WORD!

0:/> -lib shell

> 1 2 + .
 0003
> lib shell

0:/>
```

Without the `shell` library, the prompt is a plain `> `, a line's `|`, `>` and `<` are words like any other (unknown ones), and a word HyForth doesn't know is an error, not a program to run.

**Libraries from files.** `lib name`, for a name that isn't a ROM library's, reads `name.hyl`: HyForth source, like a script.  It's looked for the way a program is: in the current directory, then (for a name with no `/`) in the caches' `lib` directories on the RAM disks, in the directories of `$LIBPATH`, or, with no `LIBPATH`, in `/lib` on the current directory's card, and last in `/rom/lib`.  The words the file defines become the library's, and it's searched from then on.

* **While it loads,** the words you've defined yourself aren't searched: a library uses the base, the ROM libraries and other libraries.  A library file can load the libraries it needs with `lib` lines of its own.
* **An error** while it loads (an unknown word, say) stops it, and the library is dropped.  A file that isn't there is `!IO ERR!`.
* **`-lib name`** stops searching it, but leaves it in memory: `lib name` then searches it again, without reading the file again.  To read a changed file, start a new HyForth (`shell`, or `forth`).
* **Up to 4** can be loaded in a task, with names up to 11 characters; a fifth is `!LOW MEM!`.  A new HyForth starts with none.

```
0:/> cat /lib/greet.hyl
: greet 7 . ;
: twice dup + ;
0:/> lib greet

0:/> 3 twice .
 0006
0:/> libs
forth io files shell tasks sound mem tools term greet
0:/> -lib greet

0:/> libs
forth io files shell tasks sound mem tools term (greet)
```

### **The shell: directories, files and programs**

**At boot** the shell looks for HydraFS volumes on the SD cards (`/sd/0` to `/sd/7`), lists the ones it finds (`hydrafs 0 2`), and makes the lowest one's root the **current directory**.  Then it runs **`boot.hys`** from there, if there is one, before the first prompt: a script for your own words and settings.  **With no card**, the current directory is the shell's own area on the RAM disk, `/ram/1` (the prompt `/ram/1> `), where files can be saved until a reset, and the ROM's `/rom/boot.hys` runs instead.

**Names** are relative to the current directory unless they start with `/`, and `.` and `..` work anywhere in them: `games/star.frt`, `../notes`, `/sd/1/log`.  The current directory is a directory on a card, or `/` (`cd /`); each task has one of its own, and the tasks it starts get a copy.

**Commands** take their arguments from the rest of the line, like a Unix shell's: `cd games`, `cp notes notes.bak`, and `cd "my games"` for a name with spaces in it.  Each also has a **stack form**, in parentheses, which takes strings, for definitions and scripts' Forth code: `"games" (cd)`.  (Inside a definition only the stack form works: the parsing form's argument would be read while compiling.)

| Command | Stack form | Does |
| :------ | :--------- | :--- |
| `cd [dir]` | `(cd) ( sz -- )` | Change directory.  `cd` alone: `$HOME`, or the current card's root |
| `pwd` | | Show the current directory, as a whole path: `/sd/0/games` |
| `ls [dir]` | `(ls) ( sz -- )` | List a directory, a line per entry: `name size`, or `name/` for a directory (`ls` alone: the current one).  A file: its text (`ls /dev/sd/0/ctl`) |
| `ls -l [dir]` | | The same, with each entry's date and time (its last change, by the Hydra's clock: `name size 2026-09-29 18:05:30`); `ls -l file` shows one file's line |
| `cat [file]` | | Show a file; with no name, copy stdin to stdout until end of file |
| `cp from to` | `(cp) ( sz-from sz-to -- )` | Copy a file: to a new name (a file that's there is replaced), or into a directory, with the same name |
| `mv from to` | `(mv) ( sz-from sz-to -- )` | Rename a file or directory (`to` a plain name: in the same directory); or move a file (`to` a path, or a directory to move it into: a copy, then the original removed) |
| `rm file` | `(rm) ( sz -- )` | Remove a file (not a directory) |
| `mkdir dir` | `(mkdir) ( sz -- )` | Make a directory |
| `rmdir dir` | `(rmdir) ( sz -- )` | Remove an empty directory |
| `include file` | `(include) ( sz -- )` | Read a HyForth script into this shell (below) |
| `run file [args]` | `(run) ( sz -- )` | Run a program in a task of its own, and wait for it (below) |
| `edit [file]` | | Edit a text file (below) |
| `echo text` | | Print the text, and a new line |
| `prompt` | `( sz -- )` | Set the prompt's format (below) |

```
0:/> ls
hello.txt 13
games/
0:/> cd games
0:/games> cp star.frt /sd/1
0:/games> cd ..
0:/> mv hello.txt hi.txt
0:/> pwd
/sd/0
```

**The prompt** is a format, set with `prompt`: `%v` is the volume (`0:`, or nothing off the cards), `%d` the directory on the card (off the cards, the whole path: `cd /` gives `/> `), `%p` the whole path, `%l` the card's HydraFS label (nothing off the cards), `%t` the task (`0`-`F`), and `%%` a `%`.  The default is `"%v%d> " prompt`; `"%t %p$ " prompt` gives `1 /sd/0/games$ `, and `"[%l] %v%d> " prompt` gives `[GAMES] 0:/games> `.  Up to 31 characters.  The label is read from the card's ctl file when the prompt moves to another card, and again after `mkfs` or `relabel` (a label changed by hand through the ctl file shows once you're on another card and back).

**Scripts** (`.hys` files) are lines of HyForth, as you'd type them.  `include file` reads one into this shell, as if it were typed (no prompts, no echo), so its definitions stay.  An error, or Ctrl-C, stops it and the scripts that include it, and says which line (`line 0002`, in hex).  Scripts nest up to 4 deep; a script's lines may end with CR LF, CR or LF.

**Programs** run in a task of their own, and the shell waits for them: `run file` (or, with `&` at the line's end, doesn't: [below](#background-tasks-and-exit-statuses)).
* A **Hydra executable** (`.hyx`: a file that starts with an `HYX1` header; [writing one](../programming/programs.md)) is loaded into its new task's RAM and run until it returns.
* A **song** (`.zsm`: a file that starts with `zm`) is played by the ROM's song player, in a task of its own (see `play`, [below](#tasks-and-the-console)).
* **Anything else is a HyForth script**, read by a copy of the shell, as a pipeline's stage is: it starts with this shell's dictionary and stack, and what it defines or leaves on the stack goes away with it.  `bye` in it ends it.
* A program has the console while it runs (if the shell has it), so **Ctrl-C stops it**, and gets copies of the shell's fds, namespace and current directory: it can be a pipeline's stage (`run hello.hyx | wc`).

**A program by its name:** a word HyForth doesn't know is looked for as a program, `name.hyx`, then `name.hys`, then `name.zsm` (a song, played: [below](#tasks-and-the-console)): in the current directory, then (for a name with no `/`) in the program caches on the RAM disks (the shell's own, `/ram/1/bin`, then the shared `/ram/s/bin`: [`/ram`](../programming/io.md#the-ram-disks-ram)), in the directories of `$PATH` (below), or, with no `PATH`, in `/bin` on the current directory's card, and last in `/rom/bin`, the ROM's own ([`/rom`](../programming/io.md#the-roms-files-rom)).  So `cp game.hyx /ram/s/bin/game.hyx` makes `game` load from RAM.  So `hello` runs `hello.hyx`, and `theme` plays `theme.zsm`; with no card, `ls /rom/bin` shows what runs.

**The environment:** variables, `NAME=value`, as files under `/env`; each task has its own, and the tasks it starts (programs, scripts, shells) get a copy.  The shell uses three, and sets two:

| Variable | Does |
| :------- | :--- |
| `PATH` | Where programs are found by name: directories, `:` between them (`/sd/0/bin:/sd/1/tools`) |
| `LIBPATH` | Where `lib` finds library files (`name.hyl`), the same way (without it: `/lib` on the current card) |
| `HOME` | Where `cd` alone goes (without it: the current card's root) |
| `status` | Set by the shell: the last program's exit status, as Plan 9's `$status` (the message, or the code if there's none, or empty for success; [below](#background-tasks-and-exit-statuses)) |
| `apid` | Set by the shell: the task of the last program started with `&` (Plan 9's `$apid`) |

```
0:/> echo /sd/0/bin:/sd/0/tools > /env/PATH
0:/> echo /sd/0/work > /env/HOME
0:/> ls /env
PATH=/sd/0/bin:/sd/0/tools
HOME=/sd/0/work
0:/> cat /env/HOME
/sd/0/work
0:/> rm /env/HOME
```

Put them in `boot.hys` to have them at every boot.  A program reads one as a file (`/env/NAME`); `/dev/proc/N/env` shows task N's.

**Arguments:** the rest of the line after a program's name (`run prog a b`, or `prog a b`) is the program's, as it was typed (63 characters at most): an executable gets it in `.A.Y` ([programs.md](../programming/programs.md)), and a script with `args ( -- sz )`, a string.  A script also starts with a copy of the shell's stack.  (`(run)` gives no arguments.)

```
0:/> hello
Hello from task B
0:/> 3 4 add                    \ add.hys:  + .
 0007
0:/> greet Ann                  \ greet.hys:  "Hello, " .sz args .sz cr
Hello, Ann
0:/> nosuch
 !UNK WORD!
```

**Redirection:** a command's output can go to a file, and its input come from one:

| | Does |
| :- | :--- |
| `command > file` | stdout to the file: made, or emptied first |
| `command >> file` | stdout added to the file's end (made, if it isn't there) |
| `command < file` | stdin from the file |

`>`, `>>` and `<` are words of their own (spaces around them), outside strings; the name can be `"in quotes"`.  They apply to the whole line, or to a pipeline's last command, and stdin and stdout go back when the line's done.  In a script, a line's `<` doesn't lose the script's place.

```
0:/> words > words.txt
0:/> words | wc . . . >> counts.txt
0:/> wc < notes.txt . . .
0:/> echo run hello > /sd/0/boot.hys
```

**`echo text`** prints the rest of the line, and a new line (without its `"`s): with `>`, a quick way to make a small file.

**`edit [file]`** edits a text file: a line editor in the manner of Unix's `ed`, run in a task of its own.  It reads the file (or starts a new one), then takes commands at its `*` prompt, with line numbers before or after them (`n`, `$` for the last, `a,b` for a range):

| Command | Does |
| :------ | :--- |
| `p [a[,b]]` | Print lines, numbered (`p` alone: all of them) |
| `a [n]` | Add lines after line n (the last, if none), typed until a line of just `.` |
| `i [n]` | Insert lines before line n (the first, if none), until `.` |
| `c a[,b]` | Change lines: they go, and the lines typed until `.` take their place |
| `d a[,b]` | Delete lines |
| `w [file]` | Write the file (or another: then that's the file) |
| `q`, `Q` | Quit (with changes not written, `q` asks for a second `q`); quit at once |
| `h` | Help |

Ctrl-C comes back to its prompt, keeping the text.  Lines end with CR LF in the file; the text can be up to 29 KB.  Its commands can come from a file too: `edit notes.txt < changes.txt`.

```
0:/> edit hello.hys
hello.hys: new file
*a
"Hello, world" .sz cr
.
*w
 23 bytes
*q
0:/> hello
Hello, world
```

### **Files and devices**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `open` | `( sz mode -- fd )` | Open a file: mode 1 = read, 2 = write, 3 = both; + `$80` = don't wait (reads give `ioerr` `$73` instead); + 4 = a directory as stat records; + 8 (with 2) = empty the file first |
| `close` | `( fd -- )` | |
| `read` | `( fd addr n -- n' )` | Read up to n bytes into addr (task RAM); n' = bytes read, 0 = end of file |
| `write` | `( fd addr n -- n' )` | Write n bytes from addr |
| `seek` | `( fd lo hi -- )` | Set the offset for the next read or write: `hi * 65536 + lo` |
| `ioctl` | `( fd code arg -- )` | Device control, e.g. `1 1 N ioctl` on the console makes task N the foreground task |
| `fdup2` | `( fd newfd -- )` | Make newfd refer to fd's file: `fd 1 fdup2` sends `emit`'s output there |
| `pipe` | `( -- rfd wfd )` | Make a pipe |
| `ioerr` | `( -- n )` | The last IO error code |
| `cat` | | Copy stdin to stdout until end of file (`cat file` shows a file: [the shell](#the-shell-directories-files-and-programs)) |
| `wc` | `( -- lines words chars )` | Count stdin until end of file |
| `mount` | `( sz-path sz-dev -- )` | Mount a device at a path: `"/z" "zero" mount`, then `"/z" 1 open` |
| `bind` | `( sz-path sz-target -- )` | Make a path stand for another: `"/tty" "/dev/cons" bind` |
| `unmount` | `( sz-path -- )` | Remove a mount or bind |
| `ns` | | List the namespace |
| `stty` | `( sz -- )` | Change the serial port's settings: `"b19200" stty`, `"l7 pe s1" stty` (b = baud rate, l = data bits, p = parity n/o/e/m/s, s = stop bits).  Output so far goes out first; then switch the terminal |
| `stty?` | | Show the serial port's settings, e.g. `b9600 l8 pn s1` |
| `ctl` | `( sz-file sz-text -- )` | Write a command to a ctl file: `"/dev/sd/0/ctl" "check" ctl`, `"/dev/proc/3/ctl" "kill" ctl` |
| `create` | `( sz mode -- fd )` | Make a file (mode 0; 64 = append-only, 1 = read-only) and open it for reading and writing; a file that's there is emptied.  Mode 128 makes a directory |
| `vols` | | The cards: for each of 0-7, what it is (or `none`), and its HydraFS label, free space and last check |
| `mkfs` | `( n sz-label -- )` | Make an empty HydraFS on card n (everything on it is lost): `0 "GAMES" mkfs`.  A quick format: a moment, whatever the card's size |
| `mkfs-size` | `( n sz-label mb -- )` | The same, `mb` megabytes big (up to 65535, `$FFFF`), if the card is bigger: `0 "SMALL" 4096 mkfs-size` |
| `mkfs-part` | `( n sz-label -- )` | The same, in a HydraFS partition: one is made after the card's other partitions (a FAT one a PC made, say), or with a new partition table: `0 "GAMES" mkfs-part`.  (`mkfs` on a card that has one formats the partition) |
| `mkfs-full` | `( n sz-label -- )` | A full format: the whole free map written now (a version 1 HydraFS), with its progress shown: minutes on a big card |
| `relabel` | `( n sz-label -- )` | Give card n's HydraFS a new label: `0 "TOYS" relabel` |
| `fsck` | `( n -- )` | Check card n's HydraFS, and show what it found |
| `fsfix` | `( n -- )` | Check it, and repair its free map |

**Buffers:** a failed call prints `!IO ERR!`, and `ioerr` gives the code ([error codes](../programming/rom-layout.md#error-codes)).  `here @` is a handy scratch buffer.

```
0:/> "/dev/zero" 1 open .                    \ fd 3
 0003
0:/> 3 here @ 16 read .
 0010
0:/> "/dev/sd/0/ctl" 1 open 0 fdup2 cat | cat
sdhc 7580 MB 15523840 blocks
0:/> "/dev/sd/0/data" 3 open .               \ the SD card as bytes
0:/> 3 512 0 seek   3 here @ 16 read .       \ block 1's first 16 bytes
```

(`\` isn't a comment word: the examples just annotate.)

**The files on a card** are at `/sd/N` ([io.md](../programming/io.md#the-files-on-a-card)), and [the shell's commands](#the-shell-directories-files-and-programs) cover the everyday work.  Underneath, reading a directory gives a line per entry (so `cat` lists it), and a file reads like any other fd:

```
0:/> "/sd/0" 1 open 0 fdup2 cat | cat
hello.txt 13
games/
0:/> "games/star.frt" 1 open 0 fdup2 cat | cat
0:/> "hello.txt" 1 open .
 0003
0:/> 3 here @ 128 read .
 000D
```

Writing works as on any fd, with `create` to make a file:

```
0:/> mkdir games
0:/> "games/hi" 0 create .                 \ fd 3, open for writing
 0003
0:/> 72 here @ c! 105 here @ 1 + c!
0:/> 3 here @ 2 write . 3 close            \ "Hi"
 0002
0:/> ls games
hi 2
0:/> mv games/hi hello
0:/> rm games/hello
```

**Close what you write** before taking the card out: a file's new size goes to the card when it's closed.

**The cards themselves** (HydraFS volumes) have words of their own.  They work through each card's ctl file (`/dev/sd/N/ctl`), so `ctl` and `ls` can do the same by hand:

```
0:/> vols                                   \ what's in the sockets
0: sdhc 7580 MB 15523840 blocks
hydrafs label=GAMES
free 6246400 KB of 7761920 KB
1: none
...
0:/> 1 "WORK" mkfs                          \ a new HydraFS on card 1: everything on it is lost
0:/> 0 fsck                                 \ check card 0
sdhc 7580 MB 15523840 blocks
hydrafs label=GAMES
free 6246400 KB of 7761920 KB
check: lost 0, unmarked 0, twice 0
0:/> 0 fsfix                                \ ... and repair its free map
0:/> 0 "TOYS" relabel                       \ a new label
```

`fsck` counts clusters lost (marked in use, but nothing uses them: wasted space), unmarked (in use, but marked free: a new file could be given them) and used twice (two files share them: one is damaged).  `fsfix` frees the lost ones and marks the unmarked ones; a cluster used twice is only shown, as a person has to decide which file keeps it.  A card takes a pass for each 256 MB, so checking a big one takes a while; from 4 GB up, `10% 20% ... 100%` shows how far it's got.  Empty space checks quickly: an empty 244 GB card takes about a minute and a half (quick-formatted), or 7 minutes (full-formatted: its whole free map is read).

`mkfs` is a **quick format**: it writes just the superblock, and the free map is written as the card fills, so a 244 GB card is ready in a moment.  `mkfs-full` writes the whole map first (about 13 minutes for 244 GB, with its progress shown), for a card an older ROM will read.  `mkfs-size` makes a HydraFS smaller than the card.  `mkfs-part` puts it in a partition, so the card can also hold a FAT partition for a PC: partition the card on the PC first, leaving room after the FAT partition, then `mkfs-part` on the Hydra.  By hand, the ctl command is `format [-f] [-p] [-s size] [label]` (size in megabytes, or gigabytes with a G: `"/dev/sd/0/ctl" "format -s 8G WORK" ctl`, or `echo format -p WORK > /dev/sd/0/ctl`).

**The date and time:** `cat /dev/time` shows the Hydra's clock, and `echo 2026-09-29 18:05 > /dev/time` sets it.  With a **DS1747** in U7 (a task RAM with a clock that runs while the Hydra's off: [hardware](../hardware.md#task-ram-and-the-bank-registers)), the boot finds it and sets the clock from it, and says so after the volumes: `clock 2026-09-30 14:05:00` (with `battery low` when its battery is flat), or `clock stopped: set the time`.  Setting the time sets the DS1747 too, and a DS1747 that wasn't set (the boot said `no clock`) is found then.  Without one (`no clock`), the clock starts at 2000-01-01 at power-up, so set it after a boot for the files you write to have the right dates (`ls -l` shows them); `boot.hys` can't know the time, but a line you type can.

**Sparse files:** a write that starts past a file's end (after `seek`) fills the gap with zeros, and whole 4 KB clusters of the gap take no space on the card: a file can have holes.  `"f" 0 create .` then `3 0 $400 seek 3 here @ 4 write .` makes a 64 MB file that takes 4 KB.

### **Pipelines**

A line with `|` (with spaces around it, outside `"..."` and `q^...^` strings) is a **pipeline** (and its last command's output can go to a file: [redirection](#the-shell-directories-files-and-programs)):

```
0:/> words | wc . . .
 0DB4 021C 002C
0:/> words | cat | wc .S
```

* **Each stage but the last** runs in a copy of the shell's task (`TASK_CLONE`: same dictionary, same stack), with its stdout into a pipe.
* **The last stage** runs in the shell, with its stdin from the pipe.
* When the line is done, stdin is put back.

### **Tasks and the console**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `shell` | `( -- n )` | Start another HyForth in task n; it waits until brought to the front |
| `forth` | `( -- n )` | The same, but a bare Forth: only the base loaded (see [the base and its libraries](#the-base-and-its-libraries)) |
| `fg` | `( n -- )` | Bring task n to the front: it gets the keyboard, and the others wait to print |
| `kill` | `( n -- )` | Kill task n and the tasks it started |
| `ps` | | List the tasks (from `/dev/proc`: its files also give each task's directory, environment and memory: `cat /dev/proc/1/mem`) |
| `sleep` | `( n -- )` | Sleep n ticks (200 a second; `200 sleep` is 1 s); Ctrl-C ends it |
| `sem` | `( n -- s )` | A semaphore of n: n takes before a task has to wait; s = its number (1-16), which every task can use |
| `mutex` | `( -- s )` | A mutex: a semaphore of 1 that only the task that took it can release (released if that task ends) |
| `acquire` | `( s -- )` | Take one of semaphore s, waiting (using no CPU) until there is one; Ctrl-C ends the wait |
| `acquire?` | `( s -- f )` | Take one if there is one (true), or false at once |
| `release` | `( s -- )` | Give one back (a mutex: only its holder can): a task waiting for it goes on |
| `-sem` | `( s -- )` | Free semaphore s: the tasks waiting for it get `!IO ERR!` (`ioerr` `60`) |

```
0:/> shell .
 000B                     \ the new shell is task B: Ctrl-] B switches to it, Ctrl-] 1 back
0:/> ps
0 R -
1 R 0 *
B W 1
C D -
...
```

A semaphore made in the shell can be used by the tasks it starts, and by a pipeline's stages.  Here the shell waits for the pipeline's first stage, which runs alongside it:

```
0:/> 0 sem .
 0001
0:/> 200 sleep 1 release | 1 acquire 7 .
 0007                     \ a second later: when the first stage released it
```

A failed call gives `!IO ERR!`, with the reason in `ioerr` (`60` not a semaphore, `61` all 16 in use, `63` a mutex this task doesn't hold).  When a task ends, the semaphores it made are freed and the mutexes it holds released.

**Sound:**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `sndinit` | | Clear the YM2151 |
| `sndtest` | | Play the test song in the background: a minute of music that uses the whole YM2151 (`/rom/songs/test.zsm`: `play` plays it too; `sndstop` ends it) |
| `sndstop` | | Stop it |
| `ywrite` | `( xxaa -- f )` | Write value `aa` to YM2151 register `xx`; true if it went |
| `patch` | `( p ch -- )` | Load patch p into channel ch (0-7): 0-127 are General MIDI's instruments (0 piano, 24 guitar, 40 violin, 56 trumpet, 73 flute ...), 128-162 drum sounds |
| `note` | `( n ch -- )` | Play MIDI note n on channel ch (60: middle C; 69: A, 440 Hz) |
| `noteoff` | `( ch -- )` | Key channel ch off |
| `play` | `play song [n] [&]` | Play a song (a ZSM file), in a task of its own, and wait for it (`&`: don't).  Its loop is played n more times (none: the song once, to its end; 0: forever, until Ctrl-C or `kill`) |

```
0:/> 0 0 patch 60 0 note 100 sleep 64 0 note 100 sleep 0 noteoff
```

**Songs** are ZSM files, the Commander X16's format: the YM2151's register writes and their timing, which the Furnace tracker exports, and X16 music comes in.  `play` plays one in the ROM's player, and so does a song's name, as a program's does (`theme` for `theme.zsm`), or `run theme.zsm`.  The player claims the channels the song uses, so another program can't play over them; Ctrl-C (or `kill`, for one in the background) stops it, and its channels go quiet.  The exit status is 0 when it plays to its end, 130 on Ctrl-C.

```
0:/> play theme.zsm 0 &           \ the music, forever, in the background
[B]
0:/> $B kill                      \ and stopped
```

The library's other commands (a channel's volume, speakers, bend, a drum) are register numbers the chip doesn't have, sent with `ywrite` after the channel (`$02`): `$0203 ywrite drop $0640 ywrite drop` sets channel 3's volume to 64 (`$06`); `$07` is the speakers (1 left, 2 right, 3 both), `$09` the bend (64ths of a semitone), `$0A` a General MIDI drum (`$0A24`: a kick).  See [`/dev/snd`](../programming/io.md#sound-devsnd).

### **Background tasks and exit statuses**

**`&`** at the end of a line runs its program (or script, or pipeline) without waiting for it, as in Plan 9's `rc` or a Unix shell: the shell prints the task's number, puts it in `/env/apid`, and gives the prompt back.  The program runs alongside the shell; it doesn't have the console, so it waits if it reads or writes it, until it's brought to the front (`fg`, Ctrl-] and its number) or waited for.

**Exit statuses**, as Plan 9's: a program ends with a code (0-255, 0 for success) and a message (up to 30 characters, or none).  A program that returns has 0; Ctrl-C gives 130, `interrupt`, and a kill 137, `killed`; a C program has `main`'s value or `exit`'s, or `hy_exits`'s message.  A script ends with its last program's status, or its error's (`!UNK WORD!` is 5, the message `UNK WORD`), or what `exits` gives it.  The shell keeps the status of each program it waits for, and of each error at its prompt.

| Word | Stack | Does |
| :--- | :---- | :--- |
| `status` | `( -- n )` | The last exit status's code (the base) |
| `exits` | `( n -- )` | End this script, pipeline stage or command shell with exit status n (the base).  At the boot shell, it only sets the status |
| `wait` | `( n -- )` | Wait for task n (one started with `&`), giving it the console meanwhile; its exit status is the status then (`tasks`) |

`/env/status` has the status as text (Plan 9's `$status`): the message, or the code if there's no message, or empty for success.

```
0:/> upper &
[B]                       \ task B: the number is in /env/apid too
0:/> ps
0 R -
1 R 0 *
B W 1                     \ waiting for the console
...
0:/> $B wait              \ (task numbers are hex: $B)
hiHI                      \ it has the console: type, then Ctrl-D ends it
0:/> status .
 0000
0:/> nosuch

 !UNK WORD!
0:/> cat /env/status
UNK WORD
```

A **command shell** (`SHELL_CMD`, as Plan 9's `rc -c`) is HyForth running the lines on its stdin, with no banner or prompt, ending at its input's end with the last status: C's `system` runs one ([programs.md](../programming/programs.md#c-programs)).

### **Errors and keys**

| Message | Meaning |
| :------ | :------ |
| `!UNK WORD!` | No such word, and no program by that name (or a number it can't read) |
| `!DS PTR ERROR!`, `!RT PTR ERROR!` | Data / return stack under- or overflow |
| `!DIV ZERO!` | Division by zero |
| `!LOW MEM!` | Out of memory (dictionary or records) |
| `!SECURITY!` | A write to a protected area |
| `!IO ERR!` | An IO word failed, and why (`!IO ERR! not found`); `ioerr` gives the code |
| `!BREAK!` | Ctrl-C |

| Key | Does |
| :-- | :--- |
| Ctrl-C | Break: back to the prompt, keeping the dictionary.  While a program runs (`run`), it stops the program |
| Ctrl-\\ | Kill: a fresh HyForth (the dictionary is lost) |
| Ctrl-D / Ctrl-Z | End of input (`cat`, `wc`, `key`) |
| Ctrl-] then `0-F` | Switch to that task; `l` lists them |
| Backspace, Delete (Ctrl-D) | Erase the character before the cursor / at it |
| Left, Right (Ctrl-B, Ctrl-F) | Move along the line |
| Home, End (Ctrl-A, Ctrl-E) | To the line's start / end |
| Up, Down (Ctrl-P, Ctrl-N) | The lines typed before (about the last 255 characters' worth) |
| Ctrl-U | Erase the line |

The line editing is HyForth's, at the prompt when its input is the console: it turns the console raw while a line is typed (`/dev/cons/ctl`), and echoes the line itself.  Programs reading the console (`cat`, `wc`, C's `fgets`) get it as before: echoed, with Backspace.

### **How HyForth uses memory**

HyForth runs in task 1.  Its code and the built-in words run from the BIOS ROM (page 1, with some words' code on page A), so only its variables are in RAM: they're copied from the paged ROM to `$0800` at startup (about 1.3K: the shell's settings, the line editor's history, the sample scripts).  The build's link map (`os_rom/obj/os_rom_C02.map`, segment `FORTH_DATA`) has the size.

| What | Where |
| :--- | :---- |
| Input buffer | `$0200` |
| Data and return stacks | `$0300-$03FF` |
| Record stack | `$0400-$05FF` |
| Variables (the prompt's format, the shell's settings) | `$0800` |
| The shell's buffers (names, a block of a file being shown, include's saved fds) | Just after the variables: not in the paged ROM |
| Dictionary (your words) | Grows up from the page after them (`$0B00`), `here @` |
| Small records (strings, `malloc`) | A 2K arena from the MMU |
| Records of 256 bytes or more | Their own MMU blocks |

HyForth keeps the MMU's page floor 2 pages above `here`, so the dictionary and the MMU's allocations never meet: `!LOW MEM!` comes first.  A copy of the shell (a pipeline stage, or a script that `run` started: `TASK_CLONE`) gets all of this copied.
