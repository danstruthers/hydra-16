# Using BASIC

BASIC is the Hydra-16's BASIC: a structured BASIC in QuickBASIC's way, line numbers optional, labels to go to,
`SUB`s and `FUNCTION`s with their own variables, and the Hydra's numbers: every number exact (`1/3` is a third, `2 ^
100` whole), in any base.  It's also a shell (`basic -l`).  This guide is its reference: every statement and
function.  [../basic.md](../basic.md) is its design; [the plan](../design/plans/BASIC.md) says why it's as it is.

Contents: [Starting it](#starting-it) · [A program](#a-program) · [Numbers](#numbers) · [Strings](#strings) ·
[Names, arrays and records](#names-arrays-and-records) · [Expressions](#expressions) · [Control](#control) ·
[Procedures](#procedures) · [Data](#data) · [The console](#the-console) · [Files](#files) · [Errors](#errors) ·
[Sound](#sound) · [The system](#the-system) · [Inline assembly](#inline-assembly) · [Graphics](#graphics) ·
[At the prompt](#at-the-prompt) · [The shell](#the-shell) · [Functions, all of them](#functions-all-of-them) ·
[From other BASICs](#from-other-basics)

## Starting it

| Typed at rc's prompt | What runs |
| :--- | :--- |
| `basic` | BASIC at the console: its prompt, `> ` |
| `basic prog.bas a b` | A program run, then BASIC ends (its status: `END n`'s, or 1 after an error).  A file whose first line is `#!/bin/basic` runs by its name too (`./prog.bas`); `ARG$(1)` is `a` |
| `basic <prog.bas` | The file's lines as if typed at the prompt, quietly; its end ends BASIC |
| `basic -l` | BASIC as a login shell: its namespace made, `/lib/basic/profile.bas` run, then its prompt (`/> `) |

`SYSTEM` (or `BYE`) ends BASIC, at the prompt or in a program (`SYSTEM n`: its status n); at the shell, `exit` too.

## A program

```
' primes.bas: the primes below a number
INPUT "Below"; limit
FOR n = 2 TO limit - 1
    IF isPrime(n) THEN PRINT n;
NEXT
PRINT

FUNCTION isPrime (n)
    FOR d = 2 TO SQR(n)
        IF n MOD d = 0 THEN RETURN 0
    NEXT
    RETURN -1
END FUNCTION
```

* A program is a text file (`.bas`) of lines; a line is statements separated by `:`.  `'` and `REM` start a
  comment, to the line's end.  A line ending in ` _` goes on on the next.
* Keywords and names are the same in either case (`print` is `PRINT`), and a keyword is a whole word (`SCORE` is a
  name).
* **A label** is a name and a colon at a line's start (`again:`), alone or before the line's statements.  **A line
  number** at a line's start is a label too (`100 PRINT "hi"`, and `GOTO 100`), so old programs run; the numbers
  need not be in order, nor on every line.  `GOTO`, `GOSUB`, `RESTORE`, `ON ... GOTO`, `ON ERROR GOTO` and
  `RESUME` take a label or a line number.
* The program is read whole before it runs: a `SUB` or `FUNCTION` is known wherever it's written (`DECLARE` is
  allowed, and not needed), and a mistake in its form is said before it starts, with its file and line
  (`primes.bas:4: NEXT without FOR`).  The lines outside procedures are the main program; it ends at its last line,
  or `END`.
* `INCLUDE "lib.bas"` reads another file's lines there (QuickBASIC's `'$INCLUDE: 'lib.bas'` too), from the program's
  directory or `/lib/basic`.

## Numbers

Every number is exact, of one type: the Hydra's numbers ([../design/plans/NUMBERS.md](../design/plans/NUMBERS.md)),
the same as hylang's, HyForth's and C's.

| Kind | Written | |
| :--- | :--- | :--- |
| An integer, of any size | `42`, `-7`, `2 ^ 100` (31 digits) | Exact, to 255 bytes (614 digits) |
| A fixed decimal | `0.5`, `3.14159`, `1.250` | Exact: `0.1 + 0.2 = 0.3` is true, and `FOR x = 0 TO 1 STEP 0.1` runs 11 times |
| A fraction | `10 / 4` is `5/2`; `1 / 3` | A division's result when it isn't whole: exact, in lowest terms |
| A complex number | `2i`, `1 + 2i`, `SQR(-4)` is `2i` | Its parts any of the above |

* **Written exactly**: `PRINT 1 / 3` shows `1/3`, never `.3333333`; `PRINT 2 ^ 100` shows all of it.  A number
  has one form, so a fixed decimal shows no 0s at its end (`1.50` is `1.5`, `100.0` is `100`).  `PRINT USING`
  rounds to a picture when asked (below).
* **Exponents**: `1E6`, `2.5E-3`, `1D2` (QuickBASIC's), read exactly, in the program and by `VAL`, `READ` and
  `INPUT` (in decimal: in another base, `E` is a digit); `.5` is `0.5`.
* **In any base**: QuickBASIC's `&HFF`, `&O17`, `&B101`, and the Hydra's own, a `#` and the base: `#xFF`, `#b101`,
  `#o17`, `#16r1F`, `#c+-0` (balanced ternary), `#b0.1` (a half).  `BASE "x"` makes hexadecimal the base everything
  is shown and read in: `PRINT`, `STR$`, `VAL`, `INPUT`, `READ`, and the program's own numbers after it (in base x
  a number starts with a digit: `0FF`, as `FF` is a name).  `BASE "d"` is decimal again; `BASE$` is the base's
  string.  The bases are hylang's: `d`, `x` (`FF`), `#x` (`#xFF`, its prefix shown), `b`, `o`, `c`, `16r`, `<x`
  (least digit first), `[01]` (digits of its own) ... ([hylang.md](hylang.md), "Numbers").  `STR$(x, "b")` writes
  one number in a base.
* **The math functions** (`SQR`, `EXP`, `LOG`, `SIN`, `COS`, `TAN`, `ATN`, `PI`, `^` of a power that isn't whole)
  are exact when the answer is (`SQR(9/4)` is `3/2`, `4 ^ 0.5` is `2`), else a fixed decimal of `DIGITS`
  significant digits, correctly rounded: 12 at the start, `DIGITS 30` for 30 (1 to 100); `DIGITS()` is the
  precision.
* QuickBASIC's number types are all this one: the suffixes `%`, `&`, `!` and `#` are taken and dropped (`x%`, `x&`,
  `x!`, `x#` and `x` are one variable), `AS INTEGER`, `AS LONG`, `AS SINGLE`, `AS DOUBLE` and `AS NUMBER` are the
  same, and `DEFINT`, `DEFLNG`, `DEFSNG`, `DEFDBL` change nothing.  `CINT` and `CLNG` round to an integer (a half to
  the even one); `CSNG` and `CDBL` leave a number as it is.
* True is -1, false 0; any number not 0 is true.

## Strings

`"text"`, and names ending in `$` (`name$`), of any length to 64K.  `+` joins them; `=`, `<`, `>` ... compare them
by their bytes.  The functions: `LEN`, `LEFT$`, `RIGHT$`, `MID$`, `INSTR`, `UCASE$`, `LCASE$`, `LTRIM$`, `RTRIM$`,
`SPACE$`, `STRING$`, `CHR$`, `ASC`, `STR$`, `VAL`, `HEX$`, `OCT$` ([below](#functions-all-of-them)).
`MID$(a$, 2, 3) = "xyz"` changes part of one.  A fixed-length string, `DIM s AS STRING * 10`, is always 10
characters: what's put in it is padded with spaces or cut.

## Names, arrays and records

* **Names**: letters, digits, `_` and `.`, a letter first, any length, all of it significant; no keyword.  A name
  ending in `$` (or declared `AS STRING`, or starting with a letter `DEFSTR` named) is a string's, any other a
  number's.  A variable needs no declaring: it starts as 0, or `""`.
* **Arrays**: `DIM a(10)` (0 to 10), `DIM grid(1 TO 8, 1 TO 8)`, `DIM names$(100)`; any number of dimensions, 64K
  of elements' room each (some 13,000 numbers).  An array used before a `DIM` is 0 to 10 in each dimension it's
  used with.  `REDIM a(n)` makes it anew (`REDIM PRESERVE` keeps what's in it), `ERASE a` drops it;
  `LBOUND(a)`, `UBOUND(a, 2)` give its bounds.  `OPTION BASE 1` makes 1 the lowest index when only the highest is
  given.  An array and a variable may have the same name (`a` and `a(3)`).
* **Constants**: `CONST pi2 = 2 * PI, title$ = "Hydra"`: a name for a value, which can't change.
* **Records**: QuickBASIC's `TYPE`.

```
TYPE point
    x AS INTEGER
    y AS INTEGER
    label AS STRING * 8
END TYPE
DIM p AS point, path(1 TO 100) AS point
p.x = 3: path(1) = p: PRINT path(1).x
```

  A field is a number, a string, a fixed-length string or a record of another `TYPE`; `p = q` copies one, and so do
  `s.at = p` and `q = path(2).at` (a field that's a record).
* **`SHARED`** and **`STATIC`**: [Procedures](#procedures).  `DIM SHARED x` at the main level makes `x` every
  procedure's.

## Expressions

From the first done to the last:

| Operators | |
| :--- | :--- |
| `^` | A power (`2 ^ 10`; `2 ^ -1` is `1/2`; `-2 ^ 2` is -4) |
| `-` (one operand) | Negation |
| `*`, `/` | Multiplication, division (exact) |
| `\` | Integer division: each operand rounded to an integer, the quotient cut toward 0 (`7 \ 2` is 3) |
| `MOD` | The remainder of `\` (its sign the first's: `-7 MOD 2` is -1) |
| `+`, `-` | Addition, subtraction; `+` joins strings |
| `=`, `<>`, `<`, `>`, `<=`, `>=` | Comparisons: -1 or 0 (complex numbers by their real parts, then their imaginary) |
| `NOT` | Bitwise not (`NOT 0` is -1) |
| `AND` | Bitwise and |
| `OR` | Bitwise or |
| `XOR`, `EQV`, `IMP` | Bitwise exclusive or, equivalence, implication |

The bitwise operators work on integers of any size, in two's complement; a number that isn't whole is rounded
first.  Parentheses group.

## Control

| Statement | |
| :--- | :--- |
| `IF c THEN s [ELSE s]` | On one line; `s` is statements (`:` between them), or a label or line number to go to |
| `IF c THEN` ... `ELSEIF c THEN` ... `ELSE` ... `END IF` | A block, nested to any depth |
| `SELECT CASE x` ... `CASE 1, 3, 5` ... `CASE 10 TO 20` ... `CASE IS > 100` ... `CASE ELSE` ... `END SELECT` | The first case that matches; numbers or strings |
| `FOR i = a TO b [STEP s]` ... `NEXT [i]` | `a` to `b`, exactly (`STEP` may be a fraction or below 0); `NEXT i, j` ends two |
| `DO [WHILE c \| UNTIL c]` ... `LOOP [WHILE c \| UNTIL c]` | The test at the start, at the end, or neither |
| `WHILE c` ... `WEND` | |
| `EXIT FOR`, `EXIT DO`, `EXIT SUB`, `EXIT FUNCTION` | Out of the innermost one |
| `GOTO label` | |
| `GOSUB label` ... `RETURN` | `RETURN label` goes back to a label instead |
| `ON n GOTO a, b, c`, `ON n GOSUB a, b, c` | The nth label (none if n is out of range) |
| `END [n]`, `STOP` | The end (its status n); `STOP` stops it at the prompt, for `CONT` |
| `SLEEP s` | Waits s seconds (`SLEEP 0.25`), or till a key with none |

## Procedures

```
SUB swap2 (a, b)
    t = a: a = b: b = t
END SUB

FUNCTION area (r)
    area = PI * r ^ 2
END FUNCTION

x = 1: y = 2: swap2 x, y: PRINT x; y; area(1)
```

* `SUB name (params)` ... `END SUB`, called as a statement, `name args` or `CALL name (args)`.  `FUNCTION name
  (params)` ... `END FUNCTION` gives a value: assigned to its name (`area = ...`), or by `RETURN value`, which
  leaves it too; a name ending in `$` gives a string.  Both can call themselves.
* **Parameters**: `a`, `b$`, `c()` (an array), `d AS INTEGER`, `p AS point`.  **By reference**: a variable, an
  array's element, a record's field or a whole array given is the procedure's to change (`swap2 x, y` above); an
  expression, a constant or a variable in parentheses (`(x)`) is a copy.  A variable given must be of the
  parameter's kind.
* **Its variables are its own**: a name in a procedure is new each call, but its parameters, `SHARED x, y()` (the
  main program's), the main program's `DIM SHARED` names and `CONST`s.  `STATIC x` keeps one between calls
  (`SUB name (...) STATIC`: all of them).
* `EXIT SUB`, `EXIT FUNCTION` leave it.

## Data

`DATA 1, 2.5, "three", four` lists values in the program; `READ a, b, c$, d$` takes them in turn (a number read
as `VAL` reads it); `RESTORE` goes back to the first, `RESTORE label` to the first after a label.

## The console

| | |
| :--- | :--- |
| `PRINT [items]`, `?` | Items separated by `;` (none between) or `,` (the next zone of 14 columns); a `;` or `,` at the end: no new line.  A number has a space before it (or its `-`) and one after.  `TAB(n)` to column n, `SPC(n)` n spaces |
| `PRINT USING fmt$; items` | QuickBASIC's pictures: `#` a digit, `.` the point, `,` thousands, `+` `-` a sign, `$$` `**`, `!` `\  \` `&` strings, `_` the next character as it is; and the Hydra's placeholders: `{}` a value in the base, `{x}` `{#b}` `{c}` in a base named (`PRINT USING "{x} is {}"; 255, 255`) |
| `WRITE items` | Comma-separated, strings quoted |
| `INPUT [;] ["prompt";] a, b$` | A line typed, its values comma-separated (`?` after the prompt; `,` in its place: none); not enough of them, or a number that isn't: `Redo from start`.  `INPUT ;`: the line goes on after the answer |
| `LINE INPUT [;] ["prompt";] a$` | A whole line, as it was typed |
| `INKEY$` | The key typed, or `""` if none (it doesn't wait); the arrow keys and the like as two characters, `CHR$(0)` and a code |
| `INPUT$(n)` | n keys, waited for |
| `CLS`, `LOCATE row, col`, `CSRLIN`, `POS(0)` | The screen cleared; the cursor moved (from 1, 1); where it is |
| `COLOR fg [, bg]` | The text's colours (0-15, the terminal's); in a graphics `SCREEN`, the pen's |
| `WIDTH n`, `BEEP` | The line's width for `PRINT`'s zones and wrapping (80); the bell |

## Files

```
OPEN "scores.txt" FOR OUTPUT AS #1
FOR i = 1 TO 3: PRINT #1, i; i * i: NEXT
CLOSE #1
OPEN "scores.txt" FOR INPUT AS #1
DO UNTIL EOF(1)
    INPUT #1, a, b: PRINT a + b
LOOP
CLOSE
```

| | |
| :--- | :--- |
| `OPEN path FOR INPUT \| OUTPUT \| APPEND \| BINARY AS #n` | A file (made or emptied for `OUTPUT`), or a device (`/dev/cons`, `/pc/x`); `#n` 1 to 255, `FREEFILE` the next free.  A path goes through the namespace, as rc's do |
| `CLOSE [#n, ...]` | Those files (one not open: nothing), or all |
| `PRINT #n, ...`, `PRINT #n, USING ...`, `WRITE #n, ...` | As the console's |
| `INPUT #n, a, b$`, `LINE INPUT #n, a$`, `INPUT$(k, #n)` | As the console's; past the end, `Input past end of file` |
| `GET #n, [pos], v`, `PUT #n, [pos], v` | In `BINARY`: a string's bytes (its length's worth read), or a number's (its bytes in the stored format, as `MKN$` makes them), at the byte pos (from 1) or where it is |
| `SEEK #n, pos`, `SEEK(n)`, `LOC(n)`, `LOF(n)`, `EOF(n)` | Its position (from 1); its length; at its end? |
| `KILL path`, `NAME old AS new`, `MKDIR`, `RMDIR`, `CHDIR`, `FILES [path]` | Files and directories: removed, renamed, made; the current directory; a directory's names |
| `DIR$(path)`, `DIR$` | A directory's first name, then the next (`""` after the last) |

In a file's statements a `#` is the file's: `PRINT #x1, ...` writes to the file whose number is in `x1`, though
`#x1` is a number elsewhere (`PRINT (#xFF)` prints 255).

## Errors

* Without a handler, an error stops the program with its message, file and line, on a line of its own:
  `primes.bas:12: division by zero` (a typed program's, by the line's number: `line 20: ...`; an `INCLUDE`d
  file's, its own name and line; a file BASIC can't open: `none.bas: not found`), and BASIC's status is 1 in a
  script.
* `ON ERROR GOTO label` sends an error to a handler (`ON ERROR GOTO 0`: none).  In it, `ERR` is its code, `ERL` its
  line (in a program with line numbers its line's number, or the nearest one before it, as QuickBASIC's; else its
  line in its file), `ERR$` its message; `RESUME` runs the statement again, `RESUME NEXT` the one after it,
  `RESUME label` goes there.  `ERROR n` makes one.
* The codes are QuickBASIC's (5 illegal function call, 6 overflow, 9 subscript out of range, 11 division by zero, 13
  type mismatch, 53 file not found, 62 input past end of file ...), and the system's errors are one list with
  them: a few as QuickBASIC's (53 not found, 58 already exists, 61 disk full, 70 permission denied, 76 path not
  found), the rest 256 and the system's code (`ERR$` its text: `not a directory`).
* Ctrl-C stops a program (`Break in primes.bas:12`); `CONT` goes on.

## Sound

`SOUND ch, note [, patch [, vol]]` plays a MIDI note (60 is middle C) on a channel (0-7 the YM2151's; 8-23 a Vera
X's PSG voices), its instrument and volume (0-127) if they're given; `SOUND ch` lets it go.  `SOUND "text"` sends
the sound driver its commands (`"pan 0 left"`, `"wave 8 saw"`: [tools.md](tools.md)).  `PLAY "t180 o4 l8 c d e"`
plays a line of the score language on channel 0, `PLAY ch, "..."` on another, `PLAY "song.zsm"` a song or a score;
each waits till it's played.  `BEEP` rings the bell.

## The system

| | |
| :--- | :--- |
| `SHELL "rc line"` | rc runs it, and BASIC waits; `STATUS` is its status after |
| `SHELL$("line")` | Its output, as a string (its new lines at the end dropped) |
| `ENV$("name")`, `ENVIRON$("name")`, `ENVIRON "name=value"` | The environment's variables (rc's) |
| `ARG$(n)`, `COMMAND$` | A script's arguments (`ARG$(0)` its name); all of them as one line |
| `TIMER`, `DATE$`, `TIME$` | Seconds since midnight, by the ticks (1/200): one `TIMER` less another is exact; `"2026-10-08"`, `"14:05:09"` |
| `SYS "NAME" [, a [, x [, y]]]`, `RREG a, x, y, p` | A system call by its name (`/rom/doc/api.md`): `r0`-`r15` from bytes 2-33 (`POKE` them first); the registers after (`p` bit 0: it failed, `a` the error) |
| `SYS addr`, `CALL ABSOLUTE (addr)` | Machine code (at `$8000`-`$9FFF`: in `BANK`'s bank) |
| `CALL ASM label [, a [, x [, y]]]` | An `ASM` block's label: [Inline assembly](#inline-assembly) |
| `PEEK(addr)`, `POKE addr, b`, `BANK n`, `BANK()` | A byte of the task's memory; the RAM bank at `$8000` (BASIC's own data lives in banks too: leave theirs alone) |
| `FRE()` | The memory free, in bytes |

## Inline assembly

`ASM` and `END ASM`, each alone on its line, hold assembly in `as`'s language (ca65's:
[tools.md](tools.md#the-assembler)), anywhere in a program (in a `SUB` too) but not at the prompt.  The blocks are
assembled as the program is compiled, after its code, as one source in their order (a label in one is known in the
others), into a RAM bank of their own, seen at `$8000`-`$9FFF` while their code runs: 8K for their code, their data
and their `.bss`, each block starting in `.code`.  `CALL ASM label [, a [, x [, y]]]` calls one of their labels as
`SYS` calls machine code (`.A`, `.X` and `.Y` from the numbers, 0 if they're left out; `RREG` reads them after).
The label is written as the block writes it (the assembler tells capitals from lower case), a keyword's spelling
too (`double`).  At the prompt, `CALL ASM` calls a label the program's own `CALL ASM`s name.

The program's names are symbols in the blocks: a global number variable (not a procedure's own) is its value's
address, 5 bytes: its kind (0 for an integer of 32 bits), then an integer's 4 bytes, lowest first (code that writes
one writes its kind too); an integer `CONST` is its value.  Each is there in capitals and in lower case (`COUNT`
and `count`), but `A`, `X` and `Y`, the registers' names; a block's own label of the same name is the block's.
`.include "hydra.inc"` (from `/lib/as`) gives the system calls by name and `r0`-`r15`.  The code may use
`r0`-`r15`, keeps BASIC's zero page (`$22`-`$7F`: no `.zeropage`), and ends with `rts`.

```
CONST K = 3
total = 0
FOR i = 1 TO 4: CALL ASM addk: NEXT
CALL ASM double, 21: RREG a
PRINT total; a                    ' 12  42

ASM
addk:   lda total + 1           ; total: its kind (0, an integer), then its 4 bytes
        clc
        adc #K
        sta total + 1
        rts
double: asl a
        rts
END ASM
```

An error in a block is the assembler's, at its line (`line 50: ASM: undefined: nowhere`); a label no block has, at
the `CALL ASM` that first names it; an `ASM` without its `END ASM`, at the `ASM`.  The blocks add the assembler's
time to the program's compiling: a quarter of a second for a few, some 3 seconds with `hydra.inc`.  The assembler is
the asm library's, the one `as` runs.  A longer example is `/rom/bench/benchasm.bas`: the twenty benchmarks of
`bench.bas`, each in assembly ([../basic.md](../basic.md#against-hylang-and-hyforth)).

## Graphics

On a Vera X, through `/dev/vid/draw` ([../programming/video.md](../programming/video.md)): the console's text stays
over the drawing.

| | |
| :--- | :--- |
| `SCREEN n` | 0: text alone; 1: 320 by 240, 4 colours; 2: 640 by 480, 2 colours; 7: 320 by 240, 16 colours; 12: 640 by 480, 4 colours; 13: 320 by 240, 256 colours |
| `PSET (x, y) [, c]`, `PRESET (x, y) [, c]` | A point, in c or the pen's colour (`PRESET`: the background's) |
| `LINE [(x1, y1)]-(x2, y2) [, c [, B \| BF]]` | A line from where the last one ended; `B` a box, `BF` a filled box.  `STEP (dx, dy)` is relative |
| `CIRCLE (x, y), r [, c]`, `CIRCLE ... , , , , F` | A circle; filled with `F` |
| `PAINT (x, y) [, c [, border]]` | Fills the area around a point, to the border's colour |
| `DRAW "u10 r10 d10 l10"` | QuickBASIC's turtle: `U D L R E F G H n` (a move), `M x,y`, `B` (don't draw), `N` (come back), `C n`, `A n`, `TA deg`, `S n` |
| `GPRINT (x, y), text$` | Text on the drawing |
| `PALETTE n, rgb` | A colour's 12 bits (`&HF00` red) |
| `POINT(x, y)` | A pixel's colour |
| `WINDOW (x1, y1)-(x2, y2)`, `VIEW (x1, y1)-(x2, y2)` | Coordinates of one's own; drawing kept to a part of the screen |
| `SPRITE n, x, y [, image$]` | The Vera's sprite n (1-127) at x, y, its image a string of 8-bit pixels (`SPRITE n OFF`) |

## At the prompt

BASIC keeps a program at its prompt, as QuickBASIC's window did.

| | |
| :--- | :--- |
| `LOAD "f.bas"`, `SAVE ["f.bas"]`, `NEW` | A file read into it; written (with no name, to the one it came from); emptied |
| `RUN`, `RUN "f.bas"` | It run (read and checked first, its numbers in decimal); a file loaded and run |
| `LIST [from-to]` | Its lines, or those from a line or label to another |
| `EDIT [label]` | The editing mode: the program in the screen editor (`edit`), at the label, or the last error's line; ^X back to the prompt, the program read and checked again |
| `CONT` | On after `STOP`, Ctrl-C or an error's stop |
| `DELETE from-to` | Numbered lines taken out |
| `CLEAR` | The variables emptied |

A line typed with a number first goes into the program at its number (a number alone takes that line out), so a
program can be typed line by line as BASIC's always were.  Any other line runs at once: `PRINT 2 ^ 64`, `x = 5`,
`plot 1, 2` (the program's `SUB plot`).  After a run its variables and procedures are still there to use.  These
commands are the prompt's, at a line's start: in a program, or after a `:`, they're `only at the prompt`.

## The shell

`basic -l` is a shell: a line is BASIC's if it's a program line (a number first), `?`, a statement's keyword
first (`print`, `run`, `list`), an assignment (`x = 5`, `a$(1) = "hi"`), or a call of a `SUB` of its program;
any other is an rc command line, run by rc and waited for.

```
/> print 2 ^ 70
 1180591620717411303424
/> ls /rom/lib/basic
profile.bas
/> cd /ram
```

A name in both is BASIC's (`sleep`, `if`, `for`): `%` before a line makes it rc's whatever it is.  `cd`, `bind`,
`mount`, `unmount` and `newns` are the shell's own, as an rc line can't change BASIC's directory or namespace;
`exit` ends it; a line ending in `&` runs in the background.  The prompt is the current directory.  `basic -l`
runs `/lib/basic/profile.bas` before its first prompt (a card's or the RAM disk's in the ROM's place).

## Functions, all of them

| Numbers | |
| :--- | :--- |
| `ABS(x)`, `SGN(x)` | A real number's size; -1, 0 or 1 |
| `INT(x)`, `FIX(x)`, `CINT(x)`, `CLNG(x)` | Down to an integer; toward 0; to the nearest (a half to the even) |
| `CSNG(x)`, `CDBL(x)` | x as it is |
| `SQR`, `EXP`, `LOG`, `SIN`, `COS`, `TAN`, `ATN`, `PI` | The math functions, in radians, to `DIGITS` digits (exact when they can be) |
| `RND`, `RND(x)` | A number from 0 up to 1, of `DIGITS` digits (`RND(0)`: the last again); `RANDOMIZE [n]` seeds it (none: from the clock) |
| `FIX(x * 100) / 100`, `FIXED(x, n)` | A number cut to n places, as a fixed decimal (`FIXED(1/3, 4)` is `0.3333`) |
| `RATIONAL(x)`, `NUMERATOR(x)`, `DENOMINATOR(x)` | As a fraction; its parts, in lowest terms |
| `COMPLEX(re, im)`, `REAL(x)`, `IMAG(x)` | A complex number; its parts |
| `GCD(a, b)`, `FIB(n)` | The greatest common divisor; the nth Fibonacci number |
| `SHL(x, n)`, `SHR(x, n)`, `BIT(x, n)` | Shifts; bit n (0 or 1) |
| `VAL(s$)`, `STR$(x [, base$])` | A string's number (the longest start that is one; 0 if none); a number's text |
| `HEX$(x)`, `OCT$(x)` | In base 16 and 8 |
| `MKN$(x)`, `CVN(s$)` | A number as the bytes of the stored format, and back |

| Strings | |
| :--- | :--- |
| `LEN(s$)` | Its length |
| `LEFT$(s$, n)`, `RIGHT$(s$, n)`, `MID$(s$, i [, n])` | Its first n, last n, n from the ith (from 1) |
| `INSTR([i,] s$, t$)` | Where t$ is in s$ (from i), or 0 |
| `UCASE$`, `LCASE$`, `LTRIM$`, `RTRIM$` | In capitals, in small letters; its spaces at the start, at the end, taken off |
| `SPACE$(n)`, `STRING$(n, c)` | n spaces; n of a character (c a code or a string's first) |
| `CHR$(n)`, `ASC(s$)` | A character from its code; the first one's code |

| The rest | |
| :--- | :--- |
| `ERR`, `ERL`, `ERR$` | The last error's code, line, message |
| `EOF(n)`, `LOF(n)`, `LOC(n)`, `SEEK(n)`, `FREEFILE` | Files |
| `LBOUND(a [, d])`, `UBOUND(a [, d])` | An array's bounds |
| `TIMER`, `DATE$`, `TIME$`, `INKEY$`, `INPUT$(n [, #f])`, `CSRLIN`, `POS(0)` | The clock, the console |
| `ENV$`, `ENVIRON$`, `ARG$`, `COMMAND$`, `SHELL$`, `STATUS`, `DIR$` | The system |
| `PEEK`, `BANK()`, `FRE()`, `POINT(x, y)` | Memory, the drawing |
| `BASE$`, `DIGITS()` | The base's string, the precision |

## From other BASICs

* **From QuickBASIC**: the same language, but for one number type, exact, and what the Hydra adds.  Not here:
  `DEF FN` (a `FUNCTION` instead), `FIELD`, `LSET` and `RSET`, `RANDOM` files, the `ON` events, `PCOPY`, `VARPTR`
  and `SADD`, `$DYNAMIC` and `$STATIC`, `PRINT USING`'s `^^^^`, `RUN` from a line, and `RUN` and `CLEAR` in a
  program (they're the prompt's).
* **From the Hydra's first BASIC** (Microsoft's, 2A): a program with line numbers mostly runs as it is, with
  spaces between its keywords and names where it ran them together (`FORI=1TO9` is `FOR I = 1 TO 9`), and these
  changed: `DEF FN` and `USR` are gone; `OPEN n, "name", "W"` is `OPEN "name" FOR OUTPUT AS #n`; `GET` is
  `INKEY$`; `HIMEM` and `WAIT` are gone; a name has all its letters (`SCORE` and `SC` are two names); and `PRINT 1/3`
  shows `1/3`.
