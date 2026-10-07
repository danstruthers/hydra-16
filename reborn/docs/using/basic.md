# Using BASIC

BASIC is the Hydra-16's BASIC: EhyBASIC, which is Microsoft BASIC 2A for the 6502 (the BASIC of the Commodore PET
and its cousins), with the Hydra's files, sound, system calls and memory added.  It's also a shell: `basic -l` runs
a line as BASIC when it is BASIC's, and as an rc command line otherwise.  This guide is for using it;
[../basic.md](../basic.md) is its design (what was converted, and why each addition is as it is).

Contents: [Starting it](#starting-it) · [The basics](#the-basics) · [Statements and functions](#statements-and-functions)
· [Programs in files](#programs-in-files) · [Files](#files) · [Sound](#sound) ·
[Machine code and system calls](#machine-code-and-system-calls) · [Memory](#memory) · [The shell](#the-shell) ·
[Errors and Ctrl-C](#errors-and-ctrl-c)

## Starting it

| Typed at rc's prompt | What runs |
| :--- | :--- |
| `basic` | BASIC at the console: the banner, then `OK` after each command |
| `basic prog.bas a b` | A script: the program loaded and run, then BASIC ends (code 1 after an error).  A file whose first line is `#!/bin/basic` runs by its name too (`./prog.bas`) |
| `basic <prog.bas` | The file's lines as if typed, quietly (no banner, no `OK`); its end ends BASIC |
| `basic -l` | BASIC as a login shell: its namespace made, its profile run, then its prompt (`/> `) |

`bye` ends BASIC (and `exit` the shell).  To make BASIC every window's shell, put a line `/bin/basic -l` in
`/lib/shell` on a card (`/sd/0/lib/shell`) or the shared RAM disk.

## The basics

A line typed with a number first is part of the program; any other is run at once.  Keywords and names are taken in
either case (`print` is `PRINT`), and LIST shows them in capitals.

```
EHYBASIC FOR THE HYDRA-16 (MICROSOFT BASIC 2A)
 38550 BYTES FREE

OK
10 FOR I=1 TO 3: GOSUB 100: NEXT: ? "done"
20 END
100 ? I; I*I;: RETURN
run
 1  1  2  4  3  9 done

OK
list

10 FOR I=1 TO 3: GOSUB 100: NEXT: PRINT "done"
20 END
100 PRINT I; I*I;: RETURN
OK
```

`?` is PRINT.  EhyBASIC's short forms are still taken, and LIST shows the full names: `JSR` (GOSUB), `RTN` (RETURN),
`RSTR` (RESTORE), `CLR` (CLEAR), `LT$` `RT$` `MD$` `ST$` `CH$` (LEFT$ RIGHT$ MID$ STR$ CHR$), `&` `|` `!` (AND OR NOT).
As in every Microsoft BASIC, a keyword is found anywhere in a line, even inside a name (`SCORE` is `SC` `OR` `E`), and
only a variable name's first two characters count.  Numbers are 9-digit floating point; `A%` is an integer, `A$` a
string (255 characters at most).

## Statements and functions

| Statements | |
| :--- | :--- |
| `LET` (or none) `PRINT` `?` `INPUT` `GET` `READ` `DATA` `RESTORE` | Variables, the console, data in the program |
| `GOTO` `GOSUB` `RETURN` `ON ... GOTO/GOSUB` `IF ... THEN` `FOR ... TO ... STEP` `NEXT` `END` `STOP` `CONT` | Control |
| `DIM` `DEF FN` `CLEAR` `NEW` `RUN [line\|"name"]` `LIST [from-to]` `REM` | The program and its variables |
| `LOAD "name"` `SAVE "name"[,B]` | Programs in files |
| `OPEN n,"name"[,"R"\|"W"\|"A"]` `CLOSE [n]` `PRINT #n,` `INPUT #n,` `GET #n,` | Files |
| `SOUND ch,note[,patch[,vol]]` `SOUND ch` `SOUND "word"` `BEEP` `SLEEP s` | Sound and time |
| `SYS addr\|"NAME"[,a[,x[,y]]]` `RREG a,x,y,p` `POKE addr,b` `WAIT addr,mask[,eor]` `HIMEM n` | Machine code, system calls, memory |
| `BYE` | BASIC's end |

| Functions | |
| :--- | :--- |
| `ABS` `INT` `SGN` `SQR` `RND` `LOG` `EXP` `SIN` `COS` `TAN` `ATN` | Numbers (`RND(1)` the next random number, `RND(-n)` seeds it) |
| `LEN` `LEFT$` `RIGHT$` `MID$` `STR$` `VAL` `ASC` `CHR$` | Strings |
| `FRE(0)` `POS(0)` `PEEK(addr)` `USR(x)` | Memory free, the column, a byte, machine code |
| `EOF(n)` `ENV$("name")` | A file's end; the environment's variable (`ENV$("home")`) |

## Programs in files

`SAVE "name"` writes the program as text, its listing, so `cat` shows it and an editor can change it; `LOAD "name"`
reads it back (a line without a number, a `#!` line say, is passed over).  `SAVE "name",B` writes it tokenized,
faster to load but readable only by this BASIC; LOAD tells the two apart.  `RUN "name"` loads and runs.  Names are
paths as anywhere else (`/sd/0/games/hello.bas`, or relative to the current directory).

```
#!/bin/basic
10 PRINT "hello from a script"
```

## Files

```
10 OPEN 1,"scores.txt","W"
20 FOR I=1 TO 3: PRINT #1, I;",";I*I: NEXT
30 CLOSE 1
40 OPEN 1,"scores.txt"
50 IF EOF(1) THEN 80
60 INPUT #1, A, B: PRINT A; B
70 GOTO 50
80 CLOSE 1
```

Four channels, 1 to 4, each a file or a device (`/dev/cons`, `#n/kmesg` ...): read (`"R"`, as with no mode), written
(`"W"`: made, or emptied first) or added to (`"A"`).  `INPUT #n` reads a line's comma-separated values (its end is
`?END OF FILE ERROR`: check `EOF(n)` first), `GET #n` a byte at a time (`""` at the end).  RUN, NEW and CLEAR close
them all, and so does entering a program line.

## Sound

`SOUND ch, note, patch, vol` plays a MIDI note (60 is middle C) on one of the YM2151's channels (0-7), with an
instrument (0-127: General MIDI's; 128-162: drums and percussion) and a volume (0-127) if they're given; `SOUND ch`
lets the note go.  `SLEEP s` waits `s` seconds (`SLEEP .25`).  `SOUND "claim 255"`, `"release 255"`, `"volume 150"`
and `"reset"` are the sound driver's own words.  `BEEP` rings the bell.

```
10 FOR N=60 TO 72: SOUND 0,N,0,100: SLEEP .2: NEXT: SOUND 0
```

## Machine code and system calls

`SYS "NAME"` makes one of the system's calls by its name (`/rom/doc/api.md` lists them), its registers `.A` `.X`
`.Y` from SYS's numbers and `r0`-`r15` from bytes 2-33 (POKE them just before the SYS); `RREG A,X,Y,P` reads the
registers after it (bit 0 of `P` set: it failed, `A` the error).

```
SYS "GETPID": RREG T: PRINT "my task is"; T
SYS "TICKS": RREG L,H: PRINT H*256+L
```

`SYS address` calls machine code.  Keep room for it at the top of BASIC's memory with `HIMEM`:
`HIMEM 40704` keeps `$9F00`-`$9FFF` (40704-40959), where it can be POKEd.  `USR(x)` jumps through bytes 1285-1286.

## Memory

BASIC has the task's RAM and a RAM bank of its own after it, one piece to `$9FFF`: 38,550 bytes free for the program,
its variables, arrays and strings (`FRE(0)`).  Leave the bank register (address 0) alone while BASIC runs: the window
at `$8000` is BASIC's own memory.

## The shell

`basic -l` is a shell: a line is BASIC's if it's a program line, `?`, a statement's keyword first (`print`, `run`,
`list`), or an assignment (`x=5`, `a$(1)="hi"`); any other is an rc command line, run by rc and waited for.

```
/> print 1+1
 2
/> ls /rom/lib/basic
profile.bas
/> cd /ram
/ram> echo $status
0
```

A name in both is BASIC's (`sleep`, `wait`, `if`, `for`): put `%` before a line to make it rc's whatever it is (that
works at a plain BASIC's prompt too).  `cd`, `bind`, `mount`, `unmount` and `newns` are the shell's own, as an rc line
can't change BASIC's directory or namespace; `exit` ends it with the last command's code; a line ending in `&` runs
in the background (`$apid`).  The prompt is the current directory.  `basic -l` runs `/lib/basic/profile.bas` as typed
lines before its first prompt: a card's or the RAM disk's in the ROM's place, through the `/lib` union.

## Errors and Ctrl-C

An error stops the program with Microsoft's message and its line (`?DIVISION BY ZERO ERROR IN 30`); the system's
show the same way (`?NOT FOUND ERROR`).  Ctrl-C stops a program (`BREAK IN 20`, and `CONT` goes on), or the INPUT it
waits in; at the prompt it gives a new line.
