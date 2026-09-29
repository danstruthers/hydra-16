## **HyForth: the Hydra's shell**

HyForth is the Hydra-16's shell and programming language: a small Forth that starts in task 1 at boot and prints `HF>`.  It began as AGSB's Forth engine, adapted by Patrick Struthers for the Hydra.  Sources: `os_rom/hyforth/`.  See also [WOZMON](wozmon.md) and the [Programmer's Guide](../programming/README.md).

### **Contents**
1. [The basics](#the-basics)
2. [Numbers](#numbers)
3. [Strings](#strings)
4. [Defining words](#defining-words)
5. [Control flow: the training scripts](#control-flow-the-training-scripts)
6. [Word reference](#word-reference)
7. [Files and devices](#files-and-devices)
8. [Pipelines](#pipelines)
9. [Tasks and the console](#tasks-and-the-console)
10. [Errors and keys](#errors-and-keys)
11. [How HyForth uses memory](#how-hyforth-uses-memory)

### **The basics**

HyForth reads a line, then runs each word in it, left to right.  Numbers go on the **data stack**; words take their arguments from it and leave results on it.

```
HF>1 2 + .
 0003
HF>words
```

* **Words are separated by spaces.**  Names are case-sensitive.
* **Lines** can be up to 255 characters.
* **`.`** prints and drops the top of the stack, and **`.S`** shows the whole stack.  The format is `S<address> <depth> <items, top first>`, and `xS` clears it.
* **`words`** lists every word.
* **`bye`** leaves HyForth for [WOZMON](wozmon.md).  Ctrl-\\ starts a fresh HyForth.

Stack effects are written `( before -- after )`, with the top of the stack on the right.

### **Numbers**

* **Input** is decimal: `11`, `-2`.  A `$` prefix gives hex (`$B`, `$FFFF`), and `%` binary (`%101`).
* **Output** is always 4 hex digits: `11 .` prints `000B`.
* **Size:** numbers are 16 bits.  Decimal input is signed, `-32768` to `32767`; hex input covers `$0000-$FFFF`.
* **Inside a definition**, a number other than a single digit is written `lit [ 65 , ]`, because numbers are converted as they're read, even while compiling.  Single digits (`0`-`9`, `-1`-`-9`, `$0`-`$F`) are words, so they compile as they are.

### **Strings**

`q^text^` makes a string (in HyForth's memory records) and pushes its reference:
* `q^hello^ .sz` prints it.
* Words that take a name take a string like this: `q^/dev/zero^ 1 open`.

### **Defining words**

```
HF>: sq dup * swap drop ;
HF>5 sq .
 0019
HF>var x   7 x !   x @ .
 0007
HF>9 cons nine   nine .
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
HF>ftrain autoload
HF>: t 3 0 do i . loop ;   t
 0000 0001 0002
HF>: s 3 = if 1 . else 2 . then ;   3 s 4 s
 0001 0002
HF>: c 0 begin 1 + dup 5 = until . ;   c
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
| `.`, `.C` | `( u -- )` | Print as hex / as two characters |
| `.sz` | `( sz -- )` | Print a `q^...^` string |
| `Acls`, `Ascr ( c r -- )`, `Acol ( c -- )` | | ANSI: clear the screen, move the cursor, set attributes |
| `in>`, `reset` | | Read the input buffer; reset it |

**System:**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `syscall` | `( addr a y -- x )` | Call machine code at `addr` with `.A` and `.Y` set; pushes `.X`.  E.g. a thunk ([API index](../programming/rom-layout.md#api-index-the-thunks)) |
| `disasm` | `( addr n -- )` | Disassemble n instructions |
| `mmtest` | | Run the MMU self test |
| `debug`, `s@` | | Toggle debug; the status word's address |
| `bye` | | Leave for WOZMON |
| `abort` | | Abort to the prompt |

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
| `cat` | | Copy stdin to stdout until end of file |
| `wc` | `( -- lines words chars )` | Count stdin until end of file |
| `mount` | `( sz-path sz-dev -- )` | Mount a device at a path: `q^/z^ q^zero^ mount`, then `q^/z^ 1 open` |
| `bind` | `( sz-path sz-target -- )` | Make a path stand for another: `q^/tty^ q^/dev/cons^ bind` |
| `unmount` | `( sz-path -- )` | Remove a mount or bind |
| `ns` | | List the namespace |
| `stty` | `( sz -- )` | Change the serial port's settings: `q^b19200^ stty`, `q^l7 pe s1^ stty` (b = baud rate, l = data bits, p = parity n/o/e/m/s, s = stop bits).  Output so far goes out first; then switch the terminal |
| `stty?` | | Show the serial port's settings, e.g. `b9600 l8 pn s1` |
| `ctl` | `( sz-file sz-text -- )` | Write a command to a ctl file: `q^/dev/sd/0/ctl^ q^check^ ctl`, `q^/dev/proc/3/ctl^ q^kill^ ctl` |
| `ls` | `( sz -- )` | List a directory on a card: `q^/sd/0^ ls` |
| `create` | `( sz mode -- fd )` | Make a file (mode 0; 64 = append-only, 1 = read-only) and open it for reading and writing; a file that's there is emptied.  Mode 128 makes a directory |
| `mkdir` | `( sz -- )` | Make a directory: `q^/sd/0/games^ mkdir` |
| `remove` | `( sz -- )` | Remove a file, or an empty directory |
| `rename` | `( sz-old sz-new -- )` | Rename, in the same directory: `q^/sd/0/notes^ q^old-notes^ rename` |

**Buffers:** a failed call prints `!IO ERR!`, and `ioerr` gives the code ([error codes](../programming/rom-layout.md#error-codes)).  `here @` is a handy scratch buffer.

```
HF>q^/dev/zero^ 1 open .                     \ fd 3
 0003
HF>3 here @ 16 read .
 0010
HF>q^/dev/sd/0/ctl^ 1 open 0 fdup2 cat | cat
sdhc 7580 MB 15523840 blocks
HF>q^/dev/sd/0/data^ 3 open .                \ the SD card as bytes
HF>3 512 0 seek   3 here @ 16 read .         \ block 1's first 16 bytes
```

(`\` isn't a comment word: the examples just annotate.)

**The files on a card** are at `/sd/N` ([io.md](../programming/io.md#the-files-on-a-card)).  Reading a directory gives a line per entry, so `cat` lists it, and a file reads like any other fd:

```
HF>q^/sd/0^ 1 open 0 fdup2 cat | cat
hello.txt 13
games/
HF>q^/sd/0/games^ 1 open 0 fdup2 cat | cat
star.frt 1234
HF>q^/sd/0/games/star.frt^ 1 open 0 fdup2 cat | cat
HF>q^/sd/0/hello.txt^ 1 open .
 0003
HF>3 here @ 128 read .
 000D
```

Writing works as on any fd, and `ls`, `create`, `mkdir`, `remove` and `rename` do the rest:

```
HF>q^/sd/0/games^ mkdir
HF>q^/sd/0/games/hi^ 0 create .              \ fd 3, open for writing
 0003
HF>72 here @ c! 105 here @ 1 + c!
HF>3 here @ 2 write . 3 close                \ "Hi"
 0002
HF>q^/sd/0/games^ ls
hi 2
HF>q^/sd/0/games/hi^ q^hello^ rename
HF>q^/sd/0/games/hello^ remove
```

**Close what you write** before taking the card out: a file's new size goes to the card when it's closed.

**The card itself** is managed through its ctl file, with `ctl`, and `ls` shows it:

```
HF>q^/dev/sd/0/ctl^ q^check^ ctl             \ check the card
HF>q^/dev/sd/0/ctl^ ls
sdhc 7580 MB 15523840 blocks
hydrafs label=GAMES
free 6246400 KB of 7761920 KB
check: lost 0, unmarked 0, twice 0
HF>q^/dev/sd/0/ctl^ q^check fix^ ctl         \ ... and repair its free map
HF>q^/dev/sd/0/ctl^ q^label TOYS^ ctl        \ a new label
HF>q^/dev/sd/0/ctl^ q^format GAMES^ ctl      \ start afresh: everything on it is lost
```

### **Pipelines**

A line with `|` (with spaces around it, outside `q^...^` strings) is a **pipeline**:

```
HF>words | wc . . .
 0DB4 021C 002C
HF>words | cat | wc .S
```

* **Each stage but the last** runs in a copy of the shell's task (`TASK_CLONE`: same dictionary, same stack), with its stdout into a pipe.
* **The last stage** runs in the shell, with its stdin from the pipe.
* When the line is done, stdin is put back.

### **Tasks and the console**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `shell` | `( -- n )` | Start another HyForth in task n; it waits until brought to the front |
| `fg` | `( n -- )` | Bring task n to the front: it gets the keyboard, and the others wait to print |
| `kill` | `( n -- )` | Kill task n and the tasks it started |
| `ps` | | List the tasks (from `/dev/proc`) |
| `sleep` | `( n -- )` | Sleep n ticks (200 a second; `200 sleep` is 1 s); Ctrl-C ends it |

```
HF>shell .
 000B                   \ the new shell is task B: Ctrl-] B switches to it, Ctrl-] 1 back
HF>ps
0 R -
1 R 0 *
B W 1
C D -
...
```

**Sound:**

| Word | Stack | Does |
| :--- | :---- | :--- |
| `sndinit` | | Clear the YM2151 |
| `sndtest` | | Play the test tune in the background |
| `sndstop` | | Stop it |
| `ywrite` | `( xxaa -- f )` | Write value `aa` to YM2151 register `xx`; true if it went |

### **Errors and keys**

| Message | Meaning |
| :------ | :------ |
| `!UNK WORD!` | No such word (or a number it can't read) |
| `!DS PTR ERROR!`, `!RT PTR ERROR!` | Data / return stack under- or overflow |
| `!DIV ZERO!` | Division by zero |
| `!LOW MEM!` | Out of memory (dictionary or records) |
| `!SECURITY!` | A write to a protected area |
| `!SYS ERR!` | `syscall` returned an error |
| `!IO ERR!` | An IO word failed: see `ioerr` |
| `!BREAK!` | Ctrl-C |

| Key | Does |
| :-- | :--- |
| Ctrl-C | Break: back to the prompt, keeping the dictionary |
| Ctrl-\\ | Kill: a fresh HyForth (the dictionary is lost) |
| Ctrl-D / Ctrl-Z | End of input (`cat`, `wc`, `key`) |
| Ctrl-] then `0-F` | Switch to that task; `l` lists them |
| Backspace | Erase a character |

### **How HyForth uses memory**

HyForth runs in task 1, and its RAM image is copied from the paged ROM to `$0800` at startup.

| What | Where |
| :--- | :---- |
| Input buffer | `$0200` |
| Data and return stacks | `$0300-$03FF` |
| Record stack | `$0400-$05FF` |
| Dictionary | Grows up from its end, `here @` |
| Small records (strings, `malloc`) | A 2K arena from the MMU |
| Records of 256 bytes or more | Their own MMU blocks |

HyForth keeps the MMU's page floor 2 pages above `here`, so the dictionary and the MMU's allocations never meet: `!LOW MEM!` comes first.  A copy of the shell (a pipeline stage, `TASK_CLONE`) gets all of this copied.
