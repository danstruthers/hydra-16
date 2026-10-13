## RANDOMIZE

A statement: seeds the random number generator.

#### Syntax

```text
RANDOMIZE [n]
```

#### Arguments

*n*
: A numeric expression: the seed. Only its low 16 bits are used, so there are 65,536 different sequences.

#### Description

`RND` produces a fixed sequence of numbers from wherever the generator stands. `RANDOMIZE n` puts the generator
at the start of sequence *n*: the same *n* always gives the same numbers afterwards, which is what a program
being tested wants.

`RANDOMIZE` with no number, or with 0, takes the seed from the system's clock, so that each run is different.
That is what a game wants, once, at its start.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *n* is a string |

#### Notes

QuickBASIC asks `Random-number seed?` when `RANDOMIZE` has no number. HyBASIC asks nothing.

#### See also

`RND`, `TIMER`.

#### Example

```session
> RANDOMIZE 1 : A = RND : B = RND : RANDOMIZE 1 : PRINT A = RND; B = RND
-1 -1
```

## RATIONAL

A numeric function: a real number as an exact fraction.

#### Syntax

```text
RATIONAL(x)
```

#### Arguments

*x*
: A numeric expression whose value is real.

#### Result

The rational number equal to *x*, in lowest terms. A decimal becomes the fraction it exactly is: 0.75 becomes
3/4, and 0.1 becomes 1/10, with no error at all, since a decimal here is held in decimal. An integer or a
rational is returned unchanged.

#### Description

Adding, subtracting and multiplying decimals gives a decimal. Division is always exact, and gives a fraction
when the quotient isn't whole, a decimal's too: `0.75 / 3` is 1/4. `RATIONAL` turns a decimal into a fraction
without dividing it by anything; `FIXED` goes the other way.

The result is exact for the decimal it is given. If that decimal was itself rounded, as the result of `SQR` or
`SIN` is, the fraction is the rounded value's.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### See also

`FIXED`, `NUMERATOR`, `DENOMINATOR`.

#### Example

```session
> PRINT RATIONAL(0.75); RATIONAL(2.5); RATIONAL(0.1); RATIONAL(7); RATIONAL(SQR(2))
 3/4  5/2  1/10  7  141421356237/100000000000
```

## READ

A statement: takes the next items of the program's `DATA` into variables.

#### Syntax

```text
READ variable [, variable] ...
```

#### Arguments

*variable*
: A numeric or string variable, an array's element, or a record's field.

#### Description

Each variable is given the next unread item of the program's `DATA`, in the order the `DATA` statements stand
in the program. The place carries on from one `READ` to the next, until `RESTORE` moves it.

An item read into a string variable is its text. An item read into a numeric variable is read as a number, in
any form a program's numbers may take; an item that isn't a number, and an empty item, give 0 with no error.
`DATA` describes the items.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 4 | `out of DATA` | No item is left |

#### See also

`DATA`, `RESTORE`, `INPUT`.

#### Example

```session
> 10 DATA 1, 2
> 20 DATA 3, text
> 30 READ A, B, C, D$ : PRINT A; B; C; D$
> 40 RESTORE 20 : READ E : PRINT E
> RUN
 1  2  3 text
 3
```

## REAL

A numeric function: the real part of a number.

#### Syntax

```text
REAL(z)
```

#### Result

The real part of *z*, of the kind it has in *z*. A real number is returned unchanged. See `IMAG`, which has
the errors and an example.

## REDIM

A statement: makes an array again, with new bounds.

#### Syntax

```text
REDIM [PRESERVE] name(bounds [, bounds] ...) [AS type] [, name ...] ...
```

#### Arguments

As `DIM`'s.

#### Description

Where `DIM` refuses to make an array that exists, `REDIM` replaces it. An array that doesn't exist yet is
simply made, so `REDIM` can be used for the first time as well as the next.

Without `PRESERVE`, the new array is empty: every element 0 or the empty string.

With `PRESERVE`, the old array's elements are copied into the new one **in order of position**, not by index:
the old array's first element goes to the new array's first element, the second to the second, and so on, for
as many as both arrays have. If the two have the same lowest index, as they usually do, an element keeps its
index; if the lowest index changes, every element moves. The number of dimensions may change too, the elements
still being taken in their stored order.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 9 | `subscript out of range` | Bounds that `DIM` would refuse |
| 7 | `out of memory` | No room for the new array |

#### See also

`DIM`, `ERASE`, `LBOUND`, `UBOUND`.

#### Example

In the second line the lowest index moves from 0 to 1, so the old `A(0)` and `A(1)` become the new `A(1)` and
`A(2)`:

```session
> DIM A(3) : A(0) = 5 : A(1) = 6 : REDIM PRESERVE A(10) : PRINT A(0); A(1); UBOUND(A)
 5  6  10
> REDIM PRESERVE A(1 TO 2) : PRINT A(1); A(2) : REDIM A(4) : PRINT A(1)
 5  6
 0
```

## REM

A statement: a comment.

#### Syntax

```text
REM text
' text
```

#### Description

Everything from `REM` to the end of the line is ignored, colons included: a statement can't follow a comment on
the same line. An apostrophe does the same, and can come after a statement without a colon before it.

#### Notes

One comment is not ignored: `REM $INCLUDE: 'file'`, and the same with an apostrophe, is QuickBASIC's way of
writing `INCLUDE`. See `INCLUDE`.

#### Example

```session
> REM this is ignored : PRINT "not printed"
> PRINT "printed" ' and this is ignored too : PRINT "nor this"
printed
```

## RESTORE

A statement: moves the place `READ` takes `DATA` from.

#### Syntax

```text
RESTORE [line | label]
```

#### Description

With nothing after it, the next `READ` starts again at the program's first `DATA` item. With a line number or
a label, it starts at the first item of the first `DATA` statement at or after that line.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 8 | `label not defined` | No line has that number or label |

#### See also

`READ`, `DATA`.

## RESUME

A statement: ends an error handler, and says where the program carries on.

#### Syntax

```text
RESUME [0]
RESUME NEXT
RESUME {line | label}
```

#### Description

| Form | Carries on |
|---|---|
| `RESUME`, `RESUME 0` | At the statement that failed, which is run again |
| `RESUME NEXT` | At the statement after the one that failed |
| `RESUME line` | At that line |

The statement that failed is the one in which the error happened, not the line: with several statements on a
line, `RESUME NEXT` goes on with the next statement of that line.

`RESUME` is what marks the handler's end. After it, a new error goes to the handler again; until it, a second
error stops the program.

`RESUME` alone suits an error that the handler has put right, a missing file it has now made. If the cause is
still there the statement fails again, and the program goes round for ever.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 20 | `RESUME without error` | A `RESUME` that is not in a handler that an error started |
| 8 | `label not defined` | No line has that number or label |

#### See also

`ON`, `ERR`, `ERROR`.

#### Example

See `ON`.

## RETURN

A statement: comes back from a subroutine that `GOSUB` called.

#### Syntax

```text
RETURN
```

#### Description

Control goes back to the statement after the most recent `GOSUB` that hasn't been returned from.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 3 | `RETURN without GOSUB` | There is no `GOSUB` to go back to: usually, control has run into a subroutine's lines from above |

#### Notes

To leave a `SUB` or a `FUNCTION`, use `EXIT SUB` or `EXIT FUNCTION`, not `RETURN`.

#### See also

`GOSUB`, `ON`, `EXIT`.

## RND

A numeric function: a random number.

#### Syntax

```text
RND
RND(x)
```

#### Arguments

*x*
: A numeric expression. Its sign chooses what happens:

| *x* | Result |
|---|---|
| Left out, or more than 0 | The next number of the sequence |
| 0 | The last number again |
| Less than 0 | The generator is seeded with *x* first, then the next number. The same *x* gives the same number every time |

#### Result

A decimal from 0 up to, but never reaching, 1, with as many digits as `DIGITS` has set (12 unless it has been
changed).

#### Description

The numbers are a fixed sequence worked out from a seed, and they look random. HyBASIC seeds the generator
afresh each time it starts, so two sessions give different numbers; within a session the sequence simply carries
on, from one `RUN` to the next. `RANDOMIZE n` gives a sequence that can be repeated.

A whole number from 1 to *n* is `INT(RND * n) + 1`.

#### Notes

The generator has 16 bits of seed. It is good enough for games and simulations and not for anything that has to
be unguessable.

#### See also

`RANDOMIZE`, `DIGITS`, `INT`.

#### Example

```session
> RANDOMIZE 1 : A = RND : PRINT A; RND(0) = A; A >= 0 AND A < 1
 0.778176127105 -1 -1
> X = RND(-5) : Y = RND(-5) : PRINT X = Y : DIGITS 5 : PRINT RND : DIGITS 12
-1
 0.25799
```

## RREG

A statement: copies the processor's registers, as machine code left them, into variables.

#### Syntax

```text
RREG a [, x [, y [, p]]]
```

#### Arguments

*a*, *x*, *y*, *p*
: Numeric variables. Each is given one register's value, 0 to 255: A, X, Y, and the status flags, in that order.
  The list can stop after any of them.

#### Description

`SYS`, `CALL ABSOLUTE` and `CALL ASM` each run machine code and then save the registers it returned with.
`RREG` reads them: the values are those of the most recent such call, and stay until the next.

In the flags, bit 0 is the carry (C), bit 1 zero (Z), bit 6 overflow (V) and bit 7 negative (N). A system call
reports failure by returning with the carry set and an error code in A, so `BIT(p, 0)` tests whether a call
worked.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 40 | `variable required` | An argument that is not a variable |

#### See also

`SYS`, `CALL`, `ASM`, `BIT`.

#### Example

`GETPID` returns the task's number in A. Here HyBASIC is task 2:

```session
> SYS "GETPID" : RREG A, X, Y, P : PRINT A; BIT(P, 0)
 2  0
```

## RUN

A command of the prompt: runs the program.

#### Syntax

```text
RUN ["file"]
```

#### Arguments

*file*
: A file's name, in quotes: a program to load first, as `LOAD` would, and then run.

#### Description

`RUN` first clears away what an earlier run left: every variable and array is dropped, every file is closed,
`DATA` is back at its first item and any stopped program is forgotten. Then the whole program is compiled, and
if that finds an error it is reported with its line and nothing runs. Otherwise the program runs from its
first line.

The program's numbers are always read in decimal as it is compiled, whatever `BASE` was left at.

`RUN` must be alone on its line.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 89 | `only at the prompt` | `RUN` in a program's line, or after other statements on a line typed at the prompt |
| 288 | `not found` | There is no such *file* |

Any error the compiler finds in the program is reported by `RUN`, with its line.

#### Notes

A line number after `RUN` is accepted and ignored: `RUN 100` runs the program from its first line. To start
part-way, put a `GOTO` at the top.

#### See also

`CONT`, `LOAD`, `NEW`, `END`, `STOP`.

#### Example

```session
> 10 PRINT "running"; X : X = X + 1
> X = 5
> RUN
running 0
> RUN
running 0
```

## SAVE

A command of the prompt: writes the program to a file.

#### Syntax

```text
SAVE ["file"]
```

#### Arguments

*file*
: The file's name, in quotes. Nothing is added to it.

#### Description

The program's lines are written to the file as plain text, a line to a line, as `LIST` shows them. A file of
that name is replaced.

With no name, the program is saved to the file it was last loaded from or saved to.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 70 | `permission denied` | The file, or its disk, can't be written |
| 53 | `file not found` | A directory in the name isn't there |
| 61 | `disk full` | No room left |
| 89 | `only at the prompt` | `SAVE` in a program's line |

#### Notes

`/ram` is emptied when the Hydra is switched off or reset. Save what matters to an SD card (`/sd/0`) or to the
PC (`/pc`).

#### See also

`LOAD`, `LIST`, `NEW`.

## SCREEN

A statement: chooses a graphics mode, or returns to text alone.

#### Syntax

```text
SCREEN n
```

#### Arguments

*n*
: A numeric expression: the mode.

| *n* | Bitmap | Colours | The pen starts as |
|---|---|---|---|
| 0 | None: text alone | | |
| 1 | 320 × 240 | 4 | 3 |
| 2 | 640 × 480 | 2 | 1 |
| 7 | 320 × 240 | 16 | 15 |
| 12 | 640 × 480 | 4 | 3 |
| 13 | 320 × 240 | 256 | 15 |

#### Description

Graphics need a Vera X card (Chapter 16). In a graphics mode the screen has a **bitmap**, a grid of pixels
each holding a colour number, with the console's text shown over it. The drawing statements change the bitmap,
and `PRINT` goes on writing text as before.

`SCREEN n` sets the mode, clears the bitmap to colour 0, sets the **pen** (the colour used when a drawing
statement is given none) to the mode's brightest, up to 15, and puts the **last point referenced** at the
middle of the bitmap. Any `WINDOW` and `VIEW` are cancelled. `SCREEN 0` takes the bitmap away.

**Coordinates.** A point is written `(x, y)`. Unless `WINDOW` says otherwise, *x* counts pixels from the left
edge, starting at 0, and *y* counts them down from the top: in a 320 × 240 mode the corners are (0, 0) and
(319, 239). A coordinate that isn't an integer is rounded to the nearest, a half going to the even one.

**The last point referenced** is where the most recent drawing statement finished. `STEP (dx, dy)` means the
point that far from it, and `LINE -(x, y)` starts from it.

**Colours.** A colour is a number from 0 to 255. A mode with fewer colours uses as many low bits as it has, so
in a 4-colour mode 5 is colour 1. What colour a number looks like is the palette's business: see `PALETTE`.

The mode numbers are QuickBASIC's for the nearest thing each mode is.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is not one of the six modes |
| 53 | `file not found` | There is no Vera X card: the screen driver's files are not there |

#### Notes

The drawing is done by the screen's driver, to which HyBASIC writes commands (`/dev/vid/draw`, section 16.4).
A program in another language, or the shell, can draw on the same bitmap by the same route.

#### See also

`PSET`, `LINE`, `CIRCLE`, `PAINT`, `DRAW`, `GPRINT`, `POINT`, `COLOR`, `PALETTE`, `WINDOW`, `VIEW`, `SPRITE`,
`CLS`.

## SEEK

A statement and a function: sets, or reports, an open file's position.

#### Syntax

```text
SEEK [#]f, position
SEEK(f)
```

#### Arguments

*f*
: A numeric expression: the number of an open file, in any mode.

*position*
: A numeric expression, 1 or more: the byte the next read or write will start at, counting the file's first byte
  as 1.

#### Result

`SEEK(f)` is the file's present position, counted from 1: one more than `LOC(f)`.

#### Description

The statement moves the position. What is read or written next starts there, whatever statement does it.

A position past the end of the file is allowed. A read there finds the end of the file; a write there makes
the file longer, the gap filled with zero bytes.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *position* is less than 1 |
| 52 | `bad file number` | File *f* isn't open |

#### See also

`LOC`, `LOF`, `GET`, `PUT`, `OPEN`.

#### Example

```session
> OPEN "/ram/s.txt" FOR OUTPUT AS #1 : PRINT #1, "abcdef" : SEEK #1, 3 : PRINT #1, "X"; : PRINT SEEK(1) : CLOSE
 4
> OPEN "/ram/s.txt" FOR INPUT AS #1 : LINE INPUT #1, L$ : PRINT L$ : SEEK #1, 100 : PRINT EOF(1) : CLOSE
abXdef
-1
```

## SELECT CASE

A statement block: runs one of several groups of statements, chosen by a value.

#### Syntax

```text
SELECT CASE value
CASE test [, test] ...
    [statements]
[CASE ...] ...
[CASE ELSE
    [statements]]
END SELECT
```

where *test* is one of

```text
expression
low TO high
IS operator expression
```

#### Arguments

*value*
: An expression, numeric or string. It is evaluated once.

*test*
: An expression of the same type as *value*. *operator* is `=`, `<>`, `<`, `>`, `<=` or `>=`.

#### Description

The `CASE` lines are taken in order from the top, and the tests of each from left to right. A plain expression
matches if *value* equals it; `low TO high` if *value* is from *low* to *high*, both included; and `IS` if the
comparison of *value* with the expression holds. The statements under the first `CASE` that has a matching test
are run, and then control goes on after `END SELECT`: at most one group runs, even if a later `CASE` would
match as well.

`CASE ELSE` matches anything, so it is put last. If nothing matches and there is no `CASE ELSE`, no group
runs, and that is not an error.

Strings are compared byte by byte, as `<` and `>` compare them, so ranges of strings work: `CASE "a" TO "m"`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 80 | `block without its start or end` | `SELECT CASE` with no `END SELECT`, or `CASE` or `END SELECT` with none open |
| 13 | `type mismatch` | A test of the other type from *value* |

#### Notes

Statements written between `SELECT CASE` and the first `CASE` are run every time, before the choice is made.
QuickBASIC doesn't allow them.

The whole block can be written on one line with colons, which is the only form the prompt can take.

#### See also

`IF`, `ON`.

#### Example

```session
> 10 X = 2
> 20 SELECT CASE X
> 30 CASE 1 : PRINT "one"
> 40 CASE 2, 3 : PRINT "two or three"
> 50 CASE IS > 1 : PRINT "more than one: not run, an earlier CASE matched"
> 60 CASE ELSE : PRINT "other"
> 70 END SELECT
> 80 SELECT CASE "pear" : CASE "apple" TO "fig" : PRINT "a to f" : CASE IS > "m" : PRINT "after m" : END SELECT
> RUN
two or three
after m
```

## SGN

A numeric function: the sign of a number.

#### Syntax

```text
SGN(x)
```

#### Arguments

*x*
: A numeric expression.

#### Result

−1 if *x* is negative, 0 if it is zero, and 1 if it is positive. For a complex number, the sign of its real
part.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *x* is a string |

#### See also

`ABS`.

#### Example

```session
> PRINT SGN(-5); SGN(0); SGN(7/2); SGN(-0.1); SGN(-1+2i)
-1  0  1 -1 -1
```

## SHARED

A statement, in a procedure: makes variables of the main program visible there.

#### Syntax

```text
SHARED name[()] [AS type] [, name ...] ...
DIM SHARED ...
```

#### Arguments

*name*
: A variable of the main program. An array is written with empty parentheses.

#### Description

A `SUB` or a `FUNCTION` has variables of its own: a name used inside it is a new variable, which starts at 0 or
empty on each call, whatever the main program calls its own variables. There are two ways to reach the main
program's variables instead.

**`SHARED`, inside a procedure**, names the main program's variables that this procedure uses. From that line
on, within the procedure, those names mean the main program's variables.

**`DIM SHARED`, in the main program**, declares variables and arrays that every procedure can use without
saying so. It is `DIM` with one more word: see `DIM`.

Constants need neither. A `CONST` is known everywhere.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 85 | `not in a procedure` | `SHARED` on its own in the main program. (`DIM SHARED` is the form for there) |

#### See also

`DIM`, `STATIC`, `SUB`, `FUNCTION`, `CONST`.

#### Example

```session
> 10 DIM SHARED G : G = 1 : L = 2
> 20 SUB Show : PRINT "G ="; G; "L ="; L : END SUB
> 30 SUB Uses : SHARED L : L = L + 10 : END SUB
> 40 Show : Uses : PRINT L
> RUN
G = 1 L = 0
 12
```

## SHELL

A statement: runs a command line in the shell, and waits for it.

#### Syntax

```text
SHELL [line$]
```

#### Arguments

*line$*
: A string expression: a line as it would be typed at rc's prompt (Chapter 8). It can be anything rc accepts:
  a program and its arguments, a pipeline, several commands.

#### Description

HyBASIC starts rc with the line (`rc -c line`), with the program's standard input and output, its current
directory and a copy of its environment, and waits until rc has finished. What the commands print appears on
the screen in the ordinary way. `STATUS` then gives how the last command ended.

With no line, rc is started by itself: an interactive shell, which lasts until `exit` or
<kbd>CTRL</kbd>-<kbd>D</kbd>, when the program carries on.

Nothing the shell does to its own variables or its own current directory comes back to HyBASIC: it is another
task. Files it makes and removes are, of course, made and removed.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *line$* is a number |

A command that fails is not an error in the program. Its message is printed by rc or by the command, and
`STATUS` is not 0.

#### See also

`SHELL$`, `STATUS`, `ENVIRON`, `SYSTEM`.

#### Example

```session
> SHELL "echo from the shell" : PRINT STATUS
from the shell
 0
> SHELL "ls /nothere" : PRINT STATUS
ls: /nothere: not found
 1
```

## SHELL$

A string function: what a command line prints.

#### Syntax

```text
SHELL$(line$)
```

#### Arguments

*line$*
: A string expression: a line for rc, as `SHELL`'s.

#### Result

Everything the line's commands wrote to standard output, as one string, 8K at most. The line end after the
last line is not included; the ones between lines are, as `CHR$(10)`.

What the commands write to standard error is not captured. It goes to the screen, as their messages should.

`STATUS` is set as `SHELL` sets it.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *line$* is a number |

#### See also

`SHELL`, `STATUS`.

#### Example

```session
> A$ = SHELL$("echo one; echo two") : PRINT LEN(A$); INSTR(A$, CHR$(10)); "["; LEFT$(A$, 3); "]"
 7  4 [one]
> A$ = SHELL$("ls /nothere") : PRINT "["; A$; "]"; STATUS
ls: /nothere: not found
[] 1
```

## SHL, SHR

Numeric functions: an integer shifted left, or right, by a number of bits.

#### Syntax

```text
SHL(x, n)
SHR(x, n)
```

#### Arguments

*x*
: A numeric expression whose value is real, rounded to the nearest integer: the number to shift.

*n*
: A numeric expression: how many bits.

#### Result

`SHL(x, n)` is *x* multiplied by 2 to the power *n*. There is no word size, so nothing is ever shifted out at
the top: `SHL(1, 100)` is 2 to the power 100.

`SHR(x, n)` is *x* divided by 2 to the power *n* and rounded down. For a negative *x* this is the arithmetic
shift, which keeps the sign: `SHR(-8, 1)` is −4.

A negative *n* shifts the other way: `SHR(1, -2)` is `SHL(1, 2)`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | An argument is a string |
| 2 | `syntax error` | One argument only |

#### See also

`AND`, `OR`, `XOR`, `BIT`.

#### Example

```session
> PRINT SHL(1, 4); SHR(256, 4); SHL(1, 100); SHR(-8, 1); SHR(1, 5); SHR(1, -2)
 16  16  1267650600228229401496703205376 -4  0  4
```

## SIN

A numeric function: the sine of a real number.

#### Syntax

```text
SIN(x)
```

#### Arguments

*x*
: A numeric expression whose value is real: an angle in radians.

#### Result

The sine of *x*, from −1 to 1. `SIN(0)` is the integer 0. Any other result is a decimal, correct to the number
of significant digits that `DIGITS` has set (12 unless it has been changed).

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### Notes

`PI` is itself rounded to the digits set, so `SIN(PI)` is not exactly 0: it is a very small number, the sine
of the angle that `PI` really is.

#### See also

`COS`, `TAN`, `ATN`, `PI`, `DIGITS`.

#### Example

```session
> PRINT SIN(0); SIN(1); SIN(PI / 2); SIN(-1); SIN(PI)
 0  0.841470984808  1 -0.841470984808 -0.000000000000206761537357
```

## SLEEP

A statement: waits for a time, or for a key.

#### Syntax

```text
SLEEP [seconds]
```

#### Arguments

*seconds*
: A numeric expression: how long to wait. It need not be whole: `SLEEP 0.5` waits half a second.

#### Description

The program stops for that long and then goes on. While it waits it uses no processor time, so other tasks run
freely; this is the right way to slow a loop down. The time is counted in the system's ticks, 200 to the
second, so the shortest wait is five thousandths of a second, and a wait can be a tick longer than asked.

With no argument, or with a negative one, `SLEEP` waits until a key is pressed. The key is used up by the
wait: the program doesn't see it.

`SLEEP 0` returns at once.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *seconds* is a string |

#### See also

`TIMER`, `INKEY$`, `INPUT$`.

#### Example

```session
> T = TIMER : SLEEP 0.5 : PRINT TIMER - T >= 0.5; TIMER - T < 0.6
-1 -1
```

## SOUND

A statement: starts or stops a note; or sends the sound driver a command.

#### Syntax

```text
SOUND channel [, note [, patch [, volume]]]
SOUND command$
```

#### Arguments

*channel*
: A numeric expression: one of the sound driver's channels (section 15.1).

*note*
: A numeric expression: the note's number. 60 is middle C and each step is a semitone, so 69 is the A above it
  (section 15.2). If it is left out, the channel's note is turned off.

*patch*
: A numeric expression: the instrument to give the channel first (section 15.4).

*volume*
: A numeric expression: the channel's volume, set before the note starts. It can be given only after a patch.

*command$*
: A string expression: a line for the sound driver, as it would be written to `/dev/sndctl` (section 15.3):
  `"volume 150"`, `"pan 0 left"`.

#### Description

`SOUND channel, note` starts a note and returns at once. The note goes on sounding until `SOUND channel` stops
it, or another note is started on the same channel. Timing is the program's business: `SOUND 0, 60 : SLEEP 0.5
: SOUND 0` plays middle C for half a second.

The patch and the volume, when they are given, are set before the note starts, and all of it is sent to the
driver in one write, so that another program's commands can't come between.

The string form passes its text to the driver unchanged, and whatever the driver's commands can do, it does.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | A channel, note or patch number the driver doesn't have |
| 258 | `invalid argument` | The driver did not understand *command$* |
| 2 | `syntax error` | No arguments; or an argument skipped |

#### Notes

This is not QuickBASIC's `SOUND frequency, duration`. `SOUND 440, 18` is an error here: 440 is not a channel.

For a tune, `PLAY` is far less work.

#### See also

`PLAY`, `BEEP`, `SLEEP`.

#### Example

```session
> SOUND 0, 60 : SLEEP 0.1 : SOUND 0 : PRINT "one note"
one note
> SOUND "volume 150" : SOUND "nonsense here"
invalid argument
```

## SPACE$

A string function: a string of spaces.

#### Syntax

```text
SPACE$(n)
```

#### Arguments

*n*
: A numeric expression, 0 or more, rounded to the nearest integer.

#### Result

A string of *n* spaces; the empty string if *n* is 0.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is negative |
| 13 | `type mismatch` | *n* is a string |

#### Notes

`SPACE$` is the usual way to give a string variable a length before `GET` fills it.

#### See also

`STRING$`, `SPC`, `GET`.

#### Example

```session
> PRINT "["; SPACE$(3); "]"; LEN(SPACE$(0)); LEN(SPACE$(1000))
[   ] 0  1000
```

## SPC

Used in `PRINT`: `SPC(n)` writes *n* spaces. See `PRINT`.

## SPRITE

A statement: shows, moves or hides a sprite.

#### Syntax

```text
SPRITE n, x, y [, image$]
SPRITE n OFF
```

#### Arguments

*n*
: A numeric expression, 1 to 127: which sprite.

*x*, *y*
: Numeric expressions: where on the screen the sprite's top left corner goes, in pixels.

*image$*
: A string expression of exactly 64 or 256 bytes: the sprite's picture, one byte to a pixel, each byte a colour
  number, row by row from the top left. 64 bytes make a sprite 8 pixels square; 256 make one 16 square. A pixel
  of colour 0 is transparent.

#### Description

A sprite is a small picture that the screen shows over everything else, wherever it is told to, without
anything being drawn into the bitmap. Moving one is instant and leaves the bitmap as it was underneath.

`SPRITE n, x, y, image$` gives sprite *n* its picture and shows it at the point. `SPRITE n, x, y`, without a
picture, moves it there with the picture it has. `SPRITE n OFF` hides it; its picture is kept.

Sprites need a Vera X card but no graphics mode: they show over text too.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is not 1 to 127; or *image$* is not 64 or 256 bytes long |
| 13 | `type mismatch` | A number where the picture should be, or a string where a number should be |

#### Notes

A picture is most easily built from `CHR$` and `STRING$`, or read from `DATA`. `STRING$(64, CHR$(7))` is a solid
square of colour 7.

Sprites are the screen's, not HyBASIC's. One left showing stays on the screen after the program ends: hide it
first.

#### See also

`SCREEN`, `PALETTE`, `CHR$`, `STRING$`.

## SQR

A numeric function: the square root.

#### Syntax

```text
SQR(x)
```

#### Arguments

*x*
: A numeric expression whose value is real.

#### Result

The square root of *x*.

* If the root is exact, it is returned exactly, as an integer, a rational or a decimal: `SQR(16)` is 4,
  `SQR(1/4)` is 1/2, and `SQR(2 ^ 100)` is 2 to the power 50.
* Otherwise it is a decimal, rounded to the number of significant digits that `DIGITS` has set (12 unless it
  has been changed).
* If *x* is negative, the result is imaginary: the root of its size, times i. `SQR(-16)` is `4i`. It is not an
  error.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### Notes

A program brought from another BASIC that relies on `SQR` of a negative number being an error will go on with
an imaginary number instead, and fail later, at the first function that won't take one. Test the sign first.

#### See also

`EXP`, `LOG`, `DIGITS`, `COMPLEX`.

#### Example

```session
> PRINT SQR(16); SQR(2); SQR(1/4); SQR(0.25); SQR(-16); SQR(-2); SQR(2 ^ 100)
 4  1.41421356237  1/2  0.5  4i  1.41421356237i  1125899906842624
```

## STATIC

A statement, and a word on a procedure's first line: makes a procedure's variables keep their values between
calls.

#### Syntax

```text
STATIC name[()] [AS type] [, name ...] ...
SUB name [(parameters)] STATIC
FUNCTION name [(parameters)] STATIC
```

#### Description

A procedure's own variables are normally made afresh, 0 or empty, each time it is called. A variable named in
a `STATIC` statement inside the procedure is made once and keeps its value from one call to the next. It is
still the procedure's own: no other part of the program can see it.

`STATIC` at the end of a `SUB` or `FUNCTION` line does the same for every variable of the procedure.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 85 | `not in a procedure` | A `STATIC` statement in the main program |

#### See also

`SHARED`, `SUB`, `FUNCTION`, `DIM`.

#### Example

`K` is static and counts the calls; `M` is not, and starts at 0 each time:

```session
> 10 SUB Keeps : STATIC K : K = K + 1 : M = M + 1 : PRINT K; M : END SUB
> 20 Keeps : Keeps : Keeps
> RUN
 1  1
 2  1
 3  1
```

## STATUS

A numeric function: how the last `SHELL` command ended.

#### Syntax

```text
STATUS
```

#### Result

The exit code of the command line that the most recent `SHELL` or `SHELL$` ran: 0 if it succeeded, and not 0
if it failed. For a line of several commands it is the last one's.

#### See also

`SHELL`, `SHELL$`, `SYSTEM`.

## STEP

Part of `FOR`, where it gives the amount the counter changes by each time. In the graphics statements, `STEP`
before a point means that its coordinates are an offset from the last point. See `FOR` and `PSET`.

## STOP

A statement: stops the program in a way that can be continued.

#### Syntax

```text
STOP
```

#### Description

The program stops, `Break in line` and the line's number are printed, and the prompt returns. Everything is as
it was at that moment: the variables can be looked at and changed, and `CONT` carries on with the statement
after the `STOP`. <kbd>CTRL</kbd>-<kbd>C</kbd> stops a running program in the same way, wherever it is.

#### See also

`CONT`, `END`.

#### Example

See `CONT`.

## STR$

A string function: a number's text.

#### Syntax

```text
STR$(x [, base$])
```

#### Arguments

*x*
: A numeric expression of any kind.

*base$*
: A string expression: a base string (section 18.4), `"x"` for hex, `"b"` for binary.

#### Result

**With one argument**, the text `PRINT` would write for *x* in the base that `BASE` has set, without the space
after it: a number that isn't negative has a space in front, where a negative one has its minus sign.

**With a base**, the text of *x* in that base, with no space in front.

Either way the text is the number's exact value: all of an integer's digits, a rational with its `/`, a decimal
with its point.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *base$* isn't a base string |
| 13 | `type mismatch` | *x* is a string, or *base$* a number |

#### Notes

To drop the leading space, `LTRIM$(STR$(x))`; or give the base, `STR$(x, "d")`.

#### See also

`VAL`, `HEX$`, `OCT$`, `BASE`, `PRINT`.

#### Example

```session
> PRINT "["; STR$(5); "]["; STR$(-5); "]["; STR$(1/3); "]["; STR$(255, "x"); "]["; STR$(5, "b"); "]"
[ 5][-5][ 1/3][FF][101]
```

## STRING$

A string function: one character, repeated. `STRING` is also the name of the string type, in `AS STRING`.

#### Syntax

```text
STRING$(n, s$)
STRING$(n, code)
```

#### Arguments

*n*
: A numeric expression, 0 or more, rounded to the nearest integer: how many.

*s$*
: A string expression of one byte or more. Its first byte is the one repeated; the rest are not used.

*code*
: A numeric expression from 0 to 255: the byte to repeat, given as its code.

#### Result

A string of *n* bytes, all the same.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is negative; *s$* is empty; *code* is not 0 to 255 |

#### See also

`SPACE$`, `CHR$`.

#### Example

```session
> PRINT STRING$(3, "ab"); "|"; STRING$(3, 65); "|"; STRING$(0, "x"); "|"
aaa|AAA||
```

## SUB ... END SUB

A block: defines a subprogram, a named piece of the program with parameters and variables of its own.

#### Syntax

```text
SUB name [(parameter [, parameter] ...)] [STATIC]
    [statements]
END SUB
```

#### Arguments

*name*
: The subprogram's name.

*parameter*
: A variable's name, optionally followed by `AS type`; or an array's, written with empty parentheses.

`STATIC`
: The subprogram's variables keep their values from one call to the next. See `STATIC`.

#### Description

A `SUB` is run by calling it: its name, then an argument for each parameter, with or without the word `CALL`
and with or without parentheses. See `CALL`, which describes how arguments are passed: a variable by reference,
anything else by value.

Inside the `SUB`, the names of variables are its own. Each is made anew at every call, 0 or empty, and is gone
when the `SUB` returns, and it has nothing to do with a variable of the same name anywhere else. The exceptions
are the parameters, the names declared `SHARED` or `STATIC`, and constants.

The `SUB` returns when control reaches `END SUB` or an `EXIT SUB`. A `SUB` may call itself.

Subprograms are found when the program is compiled, wherever they are in its text, so one can be called from a
line before it. When control reaches a `SUB` line in the ordinary run of the program, it skips to after `END
SUB`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 10 | `duplicate definition` | Two procedures of one name |
| 80 | `block without its start or end` | `SUB` with no `END SUB` |
| 2 | `syntax error` | `SUB` typed at the prompt: a subprogram can be defined only in a program |

The errors of calling one are under `CALL`.

#### See also

`CALL`, `FUNCTION`, `EXIT`, `SHARED`, `STATIC`, `GOSUB`.

#### Example

```session
> 10 SUB Greet (N$, Times)
> 20   FOR I = 1 TO Times : PRINT "Hello, "; N$ : NEXT
> 30 END SUB
> 40 I = 99 : Greet "Sam", 2 : PRINT I
> RUN
Hello, Sam
Hello, Sam
 99
```

## SWAP

A statement: exchanges the values of two variables.

#### Syntax

```text
SWAP a, b
```

#### Arguments

*a*, *b*
: Two variables, array elements or record fields, of the same type.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | One is a number's and the other a string's |
| 40 | `variable required` | An argument that is not a variable |

#### Example

```session
> A = 1 : B = 2 : SWAP A, B : PRINT A; B : DIM V(1) : V(0) = 7 : V(1) = 8 : SWAP V(0), V(1) : PRINT V(0); V(1)
 2  1
 8  7
```

## SYS

A statement: calls machine code at an address, or a system call by its name.

#### Syntax

```text
SYS address [, a [, x [, y]]]
SYS "NAME" [, a [, x [, y]]]
```

#### Arguments

*address*
: A numeric expression, 0 to 65,535: the address of a machine code routine.

*NAME*
: A string expression: the name of a system call (Appendix K), in capitals or small letters.

*a*, *x*, *y*
: Numeric expressions: the values of the A, X and Y registers as the code is entered. Each is 0 if it is left
  out.

#### Description

The code is called as a subroutine (`jsr`) and must return with `rts`. When it does, its A, X and Y and the
flags are kept for `RREG` to read, and the program goes on.

Anything the program has printed is written out before the call, so output appears in order even if the code
prints too.

A system call that takes its arguments in the call registers r0 to r15 needs them put there first. They are
two bytes each in the zero page from address 2 (r0 is at 2 and 3, r1 at 4 and 5), and `POKE` sets them; HyBASIC
leaves r0 and r1 as the program's `POKE`s made them until the call.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 259 | `no such call` | *NAME* is not a system call's name |
| 13 | `type mismatch` | A register's value is a string |

A system call that fails is not an error in the program. It returns with the carry flag set and its error code
in A: read them with `RREG`.

#### Notes

The code runs inside HyBASIC's task with nothing to protect HyBASIC from it. Code that doesn't return, or
writes over HyBASIC's memory, ends the session.

For code of the program's own, an `ASM` block is easier than `POKE`s: it is assembled with the program, and
`CALL ASM` finds it by name.

#### See also

`RREG`, `CALL`, `ASM`, `PEEK`, `POKE`.

#### Example

```session
> SYS "getpid" : RREG A : PRINT A
 2
> SYS "NOSUCH"
no such call
```

## SYSTEM

A statement: leaves HyBASIC.

#### Syntax

```text
SYSTEM [n]
BYE [n]
```

#### Arguments

*n*
: A numeric expression from 0 to 255, rounded to the nearest integer: the exit code. 0, for success, if it is
  left out.

#### Description

HyBASIC ends, from a program or from the prompt, and control returns to whatever started it, usually a shell.
Open files are closed. The program in memory is lost unless it has been saved; there is no question first.

The exit code is what the shell sees as the command's status (section 8.9): 0 is success, and anything else a
failure of the program's own choosing.

`BYE` is the same statement.

#### See also

`END`, `STOP`, `SHELL`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo 'PRINT "leaving" : SYSTEM 3 : PRINT "not printed"' >bye.bas
% basic bye.bas; echo status $status
leaving
status 3
```

## TAB

Used in `PRINT`: `TAB(n)` moves to column *n*. See `PRINT`.

## TAN

A numeric function: the tangent of a real number.

#### Syntax

```text
TAN(x)
```

#### Arguments

*x*
: A numeric expression whose value is real: an angle in radians.

#### Result

The tangent of *x*. `TAN(0)` is the integer 0. Any other result is a decimal, correct to the number of
significant digits that `DIGITS` has set (12 unless it has been changed).

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### Notes

The tangent has no value at π/2, but no number a program can write is exactly π/2. `TAN(PI / 2)` is therefore
not an error: it is a very large number, the tangent of the angle that `PI / 2` really is.

#### See also

`SIN`, `COS`, `ATN`, `PI`.

#### Example

```session
> PRINT TAN(0); TAN(1); TAN(PI / 4)
 0  1.55740772465  1
```

## THEN

Part of `IF`. See `IF`.

## TIME$, TIMER

Functions: the time of day, as text and as a number of seconds.

#### Syntax

```text
TIME$
TIMER
```

#### Result

`TIME$` is a string of eight characters, `hh:mm:ss`, on the 24-hour clock.

`TIMER` is the number of seconds since midnight, as a decimal with a fraction: the system's clock ticks 200
times a second, so it moves in steps of 0.005.

Both come from the system's clock. On a Hydra with no clock chip that clock starts at midnight when the system
starts, so both then tell the time since it was switched on.

#### Notes

To time something, take `TIMER` before and after and subtract. The answer is wrong, by a day's worth of
seconds, if midnight comes between.

#### See also

`DATE$`, `SLEEP`, `RANDOMIZE`.

#### Example

```session
> PRINT LEN(TIME$); MID$(TIME$, 3, 1) : T = TIMER : SLEEP 1 : PRINT TIMER - T >= 1
 8 :
-1
```

## TO

Part of `FOR` (`FOR i = a TO b`), of `DIM` (`DIM a(1 TO 10)`) and of `CASE` (`CASE 1 TO 5`). See those.

## TYPE ... END TYPE

A block: defines a record type, a group of named fields that travel together.

#### Syntax

```text
TYPE name
    field AS type
    [field AS type] ...
END TYPE
```

#### Arguments

*name*
: The type's name.

*field*
: A field's name.

*type*
: `INTEGER`, `LONG`, `SINGLE`, `DOUBLE` or `NUMBER`, for a number; `STRING`, for a string of any length.

#### Description

`TYPE` makes no variable. It describes a kind of variable, which `DIM` then makes: `DIM P AS Point`, or an
array of them, `DIM Pts(10) AS Point`. A field is written with a full stop: `P.X`, `Pts(3).Label`. It is used
like any variable of its type.

Assigning one record to another of the same type copies every field. A record can be written to and read from
a `BINARY` file with `PUT` and `GET`.

The type is known in the whole program, procedures included, from where it is defined.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 88 | `no such field` | A field name that the record's type doesn't have. Found when the program is compiled |
| 80 | `block without its start or end` | `TYPE` with no `END TYPE` |

#### Notes

QuickBASIC's fixed-length strings, `STRING * 10`, are not here. A `STRING` field holds a string of whatever
length it is given.

A full stop is also a legal character in an ordinary name. `P.X` is the field X of P only if `P` has been
declared as a record; otherwise it is a variable called `P.X`.

#### See also

`DIM`, `PUT`, `GET`, `SUB`.

#### Example

```session
> 10 TYPE Point
> 20   X AS INTEGER
> 30   Y AS INTEGER
> 40   Label AS STRING
> 50 END TYPE
> 60 DIM P AS Point, Q AS Point : P.X = 3 : P.Y = 4 : P.Label = "here" : Q = P : Q.X = 9
> 70 PRINT P.X; P.Y; P.Label; Q.X; Q.Label
> 80 DIM Pts(2) AS Point : Pts(1).X = 5 : PRINT Pts(1).X; Pts(0).X
> RUN
 3  4 here 9 here
 5  0
```

## UNTIL

Part of `DO` loops: `DO UNTIL condition` and `LOOP UNTIL condition`. See `DO`.

## USING

Part of `PRINT USING`, which writes values in a format. See `PRINT`.

## VAL

A numeric function: the number that a string's text says.

#### Syntax

```text
VAL(s$)
```

#### Arguments

*s$*
: A string expression.

#### Result

The number written at the start of *s$*, after any spaces. It can be in any form a program's own numbers take:
a whole number, a decimal, a rational (`1/3`), a complex number (`1+2i`), one with an exponent (`-3.5e2`), or
one with a prefix that says its base (`&HFF`). Without a prefix it is read in the base that `BASE` has set.

Reading stops at the first character that can't be part of the number, and what was read up to there is the
result: `VAL("12abc")` is 12, and `VAL("1,000")` is 1. If there is no number at the start, the result is 0. It
is never an error for the text to be wrong.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *s$* is a number |

#### Notes

`VAL` reads a number. It doesn't work anything out: `VAL("2^3")` is 2.

Since a wrong text gives 0, `VAL` can't tell "0" from nonsense. A program that must know checks the text
itself.

#### See also

`STR$`, `INPUT`, `BASE`, `CVN`.

#### Example

```session
> PRINT VAL("12"); VAL(" -3.5e2"); VAL("1/3"); VAL("&HFF"); VAL("12abc"); VAL("abc"); VAL("")
 12 -350  1/3  255  12  0  0
```

## VIEW

A statement: moves the origin of the drawing coordinates to a rectangle of the bitmap.

#### Syntax

```text
VIEW [[SCREEN] (x1, y1)-(x2, y2) [, fill [, border]]]
```

#### Arguments

*x1*, *y1*, *x2*, *y2*
: Numeric expressions: two opposite corners of a rectangle, the **viewport**, in pixels of the bitmap.

*fill*, *border*
: Accepted, evaluated and ignored.

#### Description

After `VIEW (x1, y1)-(x2, y2)`, the drawing statements' coordinates are measured from the viewport's top left
corner: the point (0, 0) is the pixel (x1, y1). A drawing can be moved about the screen by changing one
statement.

With `SCREEN`, the coordinates stay the bitmap's own, and the viewport matters only to a `WINDOW`.

A `WINDOW` set afterwards spreads its coordinates over the viewport instead of over the whole bitmap, which is
the usual reason for a viewport: a graph in one corner of the screen.

`VIEW` alone makes the viewport the whole bitmap again.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | No graphics mode is set |
| 13 | `type mismatch` | An argument is a string |

#### Notes

Unlike QuickBASIC's, the viewport does not clip. A point outside it is still drawn, as long as it is on the
bitmap, and the viewport is neither cleared nor outlined.

#### See also

`WINDOW`, `SCREEN`, `POINT`.

#### Example

```text
> SCREEN 13 : VIEW (100, 100)-(200, 200) : PSET (0, 0), 13 : PRINT POINT(0, 0)
 13
> VIEW : PRINT POINT(100, 100); POINT(0, 0)
 13  0
> SCREEN 0
```

## WHILE ... WEND

A statement block: a loop that repeats while a condition holds.

#### Syntax

```text
WHILE condition
    [statements]
WEND
```

#### Arguments

*condition*
: A numeric expression. It holds if its value is not 0.

#### Description

The condition is tested before each pass, the first included. While it holds, the statements run and control
goes back to the test; when it doesn't, the program goes on after `WEND`.

It is the same loop as `DO WHILE condition` ... `LOOP`, kept for older programs, with one difference: there is
no `EXIT` for it. `EXIT DO` inside a `WHILE` loop is an error unless a `DO` loop encloses it, and then it
leaves that one.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 29 | `WHILE without WEND` | A `WHILE` whose `WEND` isn't there. Found when the program is compiled |
| 30 | `WEND without WHILE` | A `WEND` with no loop open |
| 13 | `type mismatch` | The condition is a string |

#### See also

`DO`, `FOR`, `EXIT`.

#### Example

```session
> I = 0 : WHILE I < 3 : I = I + 1 : WEND : PRINT I : WHILE 0 : PRINT "never" : WEND
 3
```

## WIDTH

A statement: sets the width of the line that `PRINT` works to.

#### Syntax

```text
WIDTH n
```

#### Arguments

*n*
: A numeric expression from 1 to 255: the number of columns. 80 when HyBASIC starts.

#### Description

The width decides how many print zones fit on a line: `PRINT a, b, c` moves to a new line when the next zone
would start beyond it. It is HyBASIC's idea of the line, used for its own layout.

It does not change the terminal or the screen, and it does not break long text: a string longer than the width
is written whole, and the terminal wraps it wherever its own edge is.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is less than 1 or more than 255 |

#### See also

`PRINT`, `POS`, `TAB`.

#### Example

```session
> WIDTH 40 : PRINT 1, 2, 3, 4 : WIDTH 80
 1             2             3
 4
```

## WINDOW

A statement: gives the drawing statements coordinates of the program's own choosing.

#### Syntax

```text
WINDOW [[SCREEN] (x1, y1)-(x2, y2)]
```

#### Arguments

*x1*, *y1*, *x2*, *y2*
: Numeric expressions: the coordinates that the left edge, one horizontal edge, the right edge and the other
  horizontal edge of the drawing area are to have.

#### Description

After `WINDOW (x1, y1)-(x2, y2)`, a point's *x* runs from *x1* at the left edge of the bitmap to *x2* at the
right, and its *y* from *y1* at the **bottom** to *y2* at the top, as a graph is drawn on paper. Each point a
statement is given is worked out exactly in those coordinates and then rounded to a pixel. `WINDOW (-1, -1)-(1,
1)` puts (0, 0) at the middle of the screen.

With `SCREEN`, *y* runs the screen's way instead: *y1* is at the top and *y2* at the bottom.

The window covers the viewport, if `VIEW` has set one, and the whole bitmap otherwise. Every statement that
takes a point follows it, `POINT` included; a `CIRCLE`'s radius is in the window's x units.

`WINDOW` alone returns to pixels.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | No graphics mode is set |
| 13 | `type mismatch` | An argument is a string |

#### See also

`VIEW`, `SCREEN`, `POINT`, `CIRCLE`.

#### Example

The point (50, 50) of a window 100 units each way is the pixel in the middle of the bitmap:

```text
> SCREEN 13 : WINDOW (0, 0)-(100, 100) : PSET (50, 50), 12 : PRINT POINT(50, 50)
 12
> WINDOW : PRINT POINT(160, 120)
 12
> SCREEN 0
```

## WRITE

A statement: writes values in a form that `INPUT` can read back.

#### Syntax

```text
WRITE [#f,] [item [, item] ...]
```

#### Arguments

*item*
: An expression of either type.

*f*
: The number of a file opened `FOR OUTPUT`, `FOR APPEND` or `FOR BINARY`.

#### Description

The items are written on one line with a comma between each and nothing else: no spaces, no zones. A string is
put in double quotes. A number is written as its text with no space before or after it. The line is then ended.
`WRITE` with no items writes an empty line.

This is exactly the form `INPUT` and `INPUT #` take apart: the commas separate the fields, and the quotes keep
a string's own commas and spaces inside one field. A line written by `WRITE #` and read by `INPUT #` with the
same list of variables gives back the values that were written.

The items are separated by commas only. A semicolon is a syntax error here.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 2 | `syntax error` | A semicolon between items |
| 52 | `bad file number` | File *f* isn't open |
| 54 | `bad file mode` | File *f* is open `FOR INPUT` |

#### Notes

A string that itself contains a double quote can't be read back whole: the quote ends the field. There is no
way to write one that `INPUT` will accept.

#### See also

`PRINT`, `INPUT`, `OPEN`.

#### Example

```session
> WRITE 1, "two", 1/3, -4.5 : WRITE "a,b", 5
1,"two",1/3,-4.5
"a,b",5
> OPEN "/ram/w.txt" FOR OUTPUT AS #1 : WRITE #1, "Sam", 12, "x, y" : CLOSE
> OPEN "/ram/w.txt" FOR INPUT AS #1 : INPUT #1, N$, G, T$ : PRINT N$; G; "["; T$; "]" : CLOSE
Sam 12 [x, y]
```

## XOR

An operator: the bitwise exclusive OR of two integers.

#### Syntax

```text
a XOR b
```

#### Result

An integer. Each bit of it is 1 where the same bit of *a* and of *b* differ, and 0 where they are alike.

For two truth values, −1 and 0, the result is true if exactly one of them is.

Both operands are always evaluated, and they are rounded and treated as `AND`'s are: see `AND`. `XOR` binds
less tightly than `OR` and more tightly than `EQV`.

#### Example

```session
> PRINT 5 XOR 3; -1 XOR -1; (1 = 1) XOR (1 = 2); -1 XOR 1
 6  0 -1 -2
```
