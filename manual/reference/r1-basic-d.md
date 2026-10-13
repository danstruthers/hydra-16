## DATA

A statement: holds values in the program for `READ` to take.

#### Syntax

```text
DATA item [, item] ...
```

#### Arguments

*item*
: A value, written out. It is not an expression: nothing in it is evaluated.

#### Description

`DATA` does nothing when control passes over it. Its items are collected when the program is compiled: all the
`DATA` statements of the program, in the order of its lines, make one list, and `READ` takes the next item from
that list each time it fills a variable. `RESTORE` moves the place back.

An item is the text between two commas, with the spaces before and after it dropped. Text in double quotes is
taken as it stands, commas and spaces included, without the quotes. An empty item, two commas with nothing
between them, is allowed.

What an item means is decided by the variable that reads it. Read into a string variable, it is its text. Read
into a numeric variable, it is read as a number in any form a program's numbers may take (`12`, `-7/2`, `2.5`,
`&HFF`); text that isn't a number, and an empty item, give 0.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 4 | `out of DATA` | A `READ` when every item has been taken. The error is `READ`'s, at its line |

#### Notes

Because the items aren't expressions, `DATA 2 + 2` holds the five characters `2 + 2`: nothing adds them up.

A `DATA` statement can be anywhere in the program, before or after the `READ` that uses it.

#### See also

`READ`, `RESTORE`.

#### Example

```session
> 10 DATA 12, hello there , "a, b", 1/2, , &HFF
> 20 READ A, B$, C$, D, E$, F
> 30 PRINT A; "["; B$; "]["; C$; "]"; D; "["; E$; "]"; F
> 40 READ G
> RUN
 12 [hello there][a, b] 1/2 [] 255
line 40: out of DATA
```

## DATE$

A string function: today's date.

#### Syntax

```text
DATE$
```

#### Result

A string of ten characters, `yyyy-mm-dd`: the year, the month and the day of the system's clock. On a Hydra
with no clock chip, the clock starts at 2000-01-01 when the system starts.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 73 | `not done yet` | An assignment to `DATE$`. QuickBASIC sets the date that way; HyBASIC doesn't set the clock |

#### Notes

The form is not QuickBASIC's `mm-dd-yyyy`. Strings in this form sort in date order.

#### See also

`TIME$`, `TIMER`.

## DECLARE

A statement: accepted, and ignored.

#### Syntax

```text
DECLARE SUB name [(parameters)]
DECLARE FUNCTION name [(parameters)]
```

#### Description

QuickBASIC needs a `DECLARE` for a procedure that is used before it is defined. HyBASIC compiles the whole
program before it runs any of it, and finds every `SUB` and `FUNCTION` first, so a declaration tells it nothing.
The statement is read past without being checked: its parameter list need not match the procedure's.

It is here so that programs written for QuickBASIC load unchanged.

#### See also

`SUB`, `FUNCTION`.

## DEFINT, DEFLNG, DEFSNG, DEFDBL, DEFSTR

Statements: set the type of names by their first letter.

#### Syntax

```text
DEFSTR letters [, letters] ...
DEFINT letters [, letters] ...
```

#### Arguments

*letters*
: One letter, or a range of letters with a hyphen between: `S`, `A-C`.

#### Description

A name normally belongs to a string if it ends in `$` and to a number if it doesn't. After `DEFSTR`, a name
with no `$` whose first letter is one of those given is a string's too.

`DEFINT`, `DEFLNG`, `DEFSNG` and `DEFDBL` all do the opposite, and are the same statement: they make names that
start with those letters numbers' again. (In QuickBASIC they choose between four kinds of number. HyBASIC has
one.)

The statement takes effect where it stands in the program's text, for the lines after it, not when control
reaches it.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | A number assigned to a name that `DEFSTR` has made a string's, or the reverse |

#### See also

`DIM`, `OPTION`.

#### Example

```session
> 10 DEFSTR S-T : S = "str" : T1 = "t" : PRINT S; T1
> 20 DEFINT S : S2 = 5 : PRINT S2
> RUN
strt
 5
```

## DELETE

A command of the prompt: removes lines from the program.

#### Syntax

```text
DELETE from[-to]
```

#### Arguments

*from*, *to*
: Line numbers.

#### Description

The lines numbered *from* to *to*, both included, are removed. With *from* alone, that one line is. Numbers in
the range that no line has are passed over, so a range can name lines that aren't there, and a `DELETE` that
finds none is not an error.

A program that was stopped can't be continued after a `DELETE`.

Typing a line number with nothing after it removes that one line, which is usually quicker.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 2 | `syntax error` | No line number; or a range with no *from* |
| 89 | `only at the prompt` | `DELETE` in a program's line |

#### See also

`LIST`, `NEW`, `EDIT`.

#### Example

```session
> 10 PRINT 1
> 20 PRINT 2
> 30 PRINT 3
> 40 PRINT 4
> DELETE 20-30
> LIST
10 PRINT 1
40 PRINT 4
```

## DENOMINATOR

A numeric function: the denominator of a real number written as a fraction in lowest terms.

#### Syntax

```text
DENOMINATOR(x)
```

#### Arguments

*x*
: A numeric expression whose value is real.

#### Result

A positive integer. A rational gives its denominator, with any common factor already taken out: 6/8 is kept as
3/4, so it gives 4. An integer gives 1. A decimal is turned into the fraction it exactly equals first: 0.75 is
3/4, and gives 4.

`NUMERATOR(x) / DENOMINATOR(x)` is *x* for every real *x*; the sign is the numerator's.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### See also

`NUMERATOR`, `RATIONAL`, `FIXED`, `GCD`.

#### Example

```session
> PRINT NUMERATOR(-6/8); DENOMINATOR(-6/8); DENOMINATOR(7); NUMERATOR(0.75); DENOMINATOR(0.75)
-3  4  1  3  4
```

## DIGITS

A statement and a function: sets, or reports, how many significant digits the mathematical functions work to.

#### Syntax

```text
DIGITS n
DIGITS
```

#### Arguments

*n*
: A numeric expression from 1 to 100, rounded to the nearest integer.

#### Result

`DIGITS`, used as a function, is the current setting: 12 when HyBASIC starts.

#### Description

Addition, subtraction, multiplication and division of integers, rationals and decimals are exact, and `DIGITS`
has nothing to do with them. It governs the functions whose results generally can't be written exactly:
`SQR`, `EXP`, `LOG`, `SIN`, `COS`, `TAN`, `ATN`, `PI` and a power with a fractional exponent. Each of those
returns a decimal rounded to *n* significant digits. `RND` returns a decimal of *n* digits too.

A result that happens to be exact comes back exact whatever the setting: `SQR(16)` is the integer 4.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is less than 1 or more than 100 |
| 13 | `type mismatch` | *n* is a string |

#### Notes

A value that was worked out at one setting keeps the digits it has when the setting changes. To cut a number to
some number of decimal places, use `FIXED`.

#### See also

`BASE`, `FIXED`, `PI`, `SQR`, `RND`.

#### Example

```session
> PRINT DIGITS; SQR(2); 1/3
 12  1.41421356237  1/3
> DIGITS 5 : PRINT SQR(2); PI; 1/3 : DIGITS 12
 1.4142  3.1416  1/3
> DIGITS 0
illegal function call
```

## DIM

A statement: makes arrays, and declares variables with a type.

#### Syntax

```text
DIM [SHARED] name[(bounds [, bounds] ...)] [AS type] [, name ...] ...
```

where *bounds* is

```text
upper
lower TO upper
```

#### Arguments

*name*
: The array's or the variable's name.

*lower*, *upper*
: Numeric expressions, each rounded to the nearest integer: the lowest and the highest index in that dimension.
  If *lower* is left out it is 0, or 1 after `OPTION BASE 1`. An array can have from 1 to 8 dimensions.

*type*
: `INTEGER`, `LONG`, `SINGLE`, `DOUBLE` or `NUMBER`, which all mean a number; `STRING`; or the name of a `TYPE`,
  for a record. Without `AS`, the type is the name's own: a string if it ends in `$`.

#### Description

**An array** is made when the statement runs, so its bounds can be worked out by the program. Every element
starts as 0, or as the empty string. `DIM A(3)` has the four elements `A(0)` to `A(3)`.

An array that is used without having been made is made on the spot, with indexes 0 to 10 in each dimension it
is used with (from 1 after `OPTION BASE 1`). `DIM` is needed only for other sizes.

**A variable** with no bounds is declared with its type: `DIM Count AS INTEGER`, `DIM P AS Point`. A numeric
type is a statement of intent only. Every numeric variable holds any number, so a variable declared `INTEGER`
can be given 1/3 and keeps it.

`SHARED` makes the names known inside every `SUB` and `FUNCTION` as well as in the main program: see `SHARED`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 10 | `duplicate definition` | The array has been made already. `ERASE` it first, or use `REDIM` |
| 9 | `subscript out of range` | A negative *upper*; *lower* above *upper*; more than 8 dimensions. Also: an index outside an array's bounds, wherever the array is used |
| 7 | `out of memory` | No room for the array |
| 13 | `type mismatch` | A bound that is a string |

#### Notes

A name followed by parentheses is taken to be an array's unless it is a `FUNCTION`'s. A misspelled function
name is therefore not an error: it is a new array, whose elements are all 0.

#### See also

`REDIM`, `ERASE`, `OPTION`, `LBOUND`, `UBOUND`, `TYPE`, `SHARED`, `STATIC`.

#### Example

```session
> DIM A(3) : A(3) = 7 : PRINT A(0); A(3); LBOUND(A); UBOUND(A)
 0  7  0  3
> A(4) = 1
subscript out of range
> DIM A(5)
duplicate definition
> DIM C(1 TO 3, -2 TO 2) : C(3, -2) = 9 : PRINT C(3, -2); LBOUND(C, 2); UBOUND(C, 2)
 9 -2  2
> B(10) = 1 : PRINT UBOUND(B)
 10
> DIM Q AS INTEGER : Q = 1/3 : PRINT Q
 1/3
```

## DIR$

A string function: the names in a directory, one at a time.

#### Syntax

```text
DIR$(path$)
DIR$
```

#### Arguments

*path$*
: A string expression: a directory's name.

#### Result

`DIR$(path$)` is the first name in the directory, and each `DIR$` after it, with no argument, is the next.
A name that is itself a directory has a `/` after it. The empty string when there are no more, and for every
call after that. The names come in the directory's own order, which is not alphabetical.

The empty string, too, if *path$* is a file and not a directory.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 53 | `file not found` | There is no such name as *path$* |
| 13 | `type mismatch` | *path$* is a number |

#### Notes

*path$* names a directory. It is not a pattern: QuickBASIC's `DIR$("*.BAS")` has no meaning here. To pick out
some of the names, test each one as it comes.

#### See also

`FILES`, `CHDIR`, `OPEN`.

#### Example

```session
> CHDIR "/ram" : MKDIR "sub" : OPEN "a.txt" FOR OUTPUT AS #1 : CLOSE
> N$ = DIR$("/ram") : DO WHILE N$ <> "" : PRINT N$; " "; : N$ = DIR$ : LOOP : PRINT
bin/ lib/ sub/ a.txt
```

## DO ... LOOP

A statement block: a loop that repeats while, or until, a condition holds.

#### Syntax

```text
DO [{WHILE | UNTIL} condition]
    [statements]
LOOP

DO
    [statements]
LOOP [{WHILE | UNTIL} condition]
```

#### Arguments

*condition*
: A numeric expression. It holds if its value is not 0.

#### Description

With the condition after `DO`, it is tested before each pass, the first included, so the statements may not run
at all. With the condition after `LOOP`, it is tested after each pass, so the statements run at least once.

`WHILE` goes on for as long as the condition holds. `UNTIL` goes on for as long as it does not.

With no condition in either place the loop repeats for ever, until `EXIT DO` leaves it, an error stops it, or
<kbd>CTRL</kbd>-<kbd>C</kbd> is pressed. `EXIT DO` leaves the innermost `DO` loop at once, whatever its
condition, and goes on after its `LOOP`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 80 | `block without its start or end` | A `DO` with no `LOOP`, or a `LOOP` with no `DO`; a condition in both places; the loop and another block overlapping. Found when the program is compiled |
| 13 | `type mismatch` | The condition is a string |

#### Notes

At the prompt a loop must be whole on its line, since each line typed is compiled alone.

`WHILE` ... `WEND` is the same as `DO WHILE` ... `LOOP`.

#### See also

`EXIT`, `WHILE`, `FOR`.

#### Example

```session
> I = 0 : DO : I = I + 1 : LOOP UNTIL I = 3 : PRINT I
 3
> I = 5 : DO WHILE I < 3 : I = I + 1 : LOOP : PRINT I
 5
> I = 5 : DO : I = I + 1 : LOOP WHILE I < 3 : PRINT I
 6
```

## DRAW

A statement: draws with a string of pen movements, QuickBASIC's turtle language.

#### Syntax

```text
DRAW commands$
```

#### Arguments

*commands$*
: A string expression: movement commands, one after another. Spaces between them are optional, and capitals and
  small letters are the same.

#### Description

Needs a graphics mode. Drawing starts at the last point referenced, and each move draws a line from where the
pen is to where it goes, in the pen's colour, and leaves the pen there.

| Command | Moves the pen |
|---|---|
| `U` *n*, `D` *n*, `L` *n*, `R` *n* | Up, down, left, right by *n* units |
| `E` *n*, `F` *n*, `G` *n*, `H` *n* | Diagonally: up and right, down and right, down and left, up and left, *n* units each way |
| `M` *x*`,`*y* | To the point (*x*, *y*) |
| `M` `+`*x*`,`*y* or `M` `-`*x*`,`*y* | By that much from where it is: a sign on the first number makes both numbers offsets |

*n* is 1 if it is left out. A unit is one pixel to begin with.

| Before a move | Effect |
|---|---|
| `B` | Move without drawing |
| `N` | Draw, then put the pen back where it started |

| Setting | Effect |
|---|---|
| `C` *n* | The colour to draw in. It becomes the pen's colour, and stays so after the `DRAW` |
| `S` *n* | The scale: a unit is *n* / 4 pixels. 4 to begin with |
| `A` *n* | Turn every direction by *n* × 90 degrees, anticlockwise: *n* is 0 to 3 |
| `TA` *n* | Turn every direction by *n* degrees, anticlockwise |

The scale and the turn stay in force for the `DRAW` statements that follow, until they are set again. A line
that runs off the bitmap is drawn up to its edge.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | No graphics mode is set; or a letter in the string that is not a command |
| 13 | `type mismatch` | The argument is a number |

#### Notes

QuickBASIC's `X`, which runs another string, and `P`, which paints, are not here. Join strings with `+`, and use
`PAINT`.

#### See also

`LINE`, `PSET`, `COLOR`, `PAINT`, `SCREEN`.

#### Example

A square of side 10, and a line at 45 degrees:

```text
> SCREEN 13 : DRAW "BM150,100 C5 R10 D10 L10 U10"
> PRINT POINT(155, 100); POINT(160, 105); POINT(150, 105); POINT(155, 105)
 5  5  5  0
> DRAW "BM150,150 C7 TA45 R10" : PRINT POINT(157, 143)
 7
> SCREEN 0
```

## EDIT

A command of the prompt: opens the program in the screen editor.

#### Syntax

```text
EDIT [line]
```

#### Arguments

*line*
: A line number or a label: where the editor's cursor starts.

#### Description

The program's text is written to a file in `/ram`, and the screen editor, `edit` (Chapter 11), is started on
it with the cursor at *line*. With no *line*, the cursor starts at the line of the last error, if there has been
one.

When the editor is left, the file is read back as the program and compiled, and the first error found, if there
is one, is reported with its line. `EDIT` again goes to it.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 89 | `only at the prompt` | `EDIT` in a program's line |

#### See also

`LIST`, `LOAD`, `SAVE`, `DELETE`.

## ELSE, ELSEIF

Parts of `IF`: `ELSE` begins the statements for a condition that didn't hold, and `ELSEIF` tests another. `CASE
ELSE` is the last choice of a `SELECT CASE`. See `IF` and `SELECT`.

## END

A statement: ends the program. Also the first word of the lines that close a block.

#### Syntax

```text
END
END {IF | SELECT | SUB | FUNCTION | TYPE | ASM}
```

#### Description

`END` alone stops the program and returns to the prompt, or, when HyBASIC was started with a program to run,
ends HyBASIC. Nothing is printed. A program also ends when control runs off its last line, so `END` is needed
only to stop sooner, typically before the lines of a subroutine that `GOSUB` reaches.

A program that has ended can't be continued with `CONT`. To stop in a way that can be, use `STOP`.

The two-word forms each close the block of that name: see `IF`, `SELECT`, `SUB`, `FUNCTION`, `TYPE` and `ASM`.

#### See also

`STOP`, `SYSTEM`, `CONT`.

#### Example

```session
> 10 PRINT "a" : END : PRINT "b"
> RUN
a
> CONT
cannot continue
```

## ENV$, ENVIRON$

String functions: the value of a variable of the environment.

#### Syntax

```text
ENVIRON$(name$)
ENV$(name$)
```

#### Arguments

*name$*
: A string expression: the variable's name, in the case it has. The shell's own variables are in small letters.

#### Result

The variable's value. A value is a list of words (section 8.4); they are returned joined with one space between
each. The empty string if there is no such variable.

The two names are the same function.

#### Description

The environment is the set of named values a task starts with, copied from the task that started it: for
HyBASIC, usually a shell, whose variables they are: `path`, `prompt` and the rest. Each is also a file in `/env`
(section 17.7).

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *name$* is a number |

#### See also

`ENVIRON`, `ARG$`, `COMMAND$`, `SHELL`.

#### Example

```session
> PRINT ENVIRON$("path"); "|"; ENV$("nosuch"); "|"
. /bin||
```

## ENVIRON

A statement: sets, or removes, a variable of the environment.

#### Syntax

```text
ENVIRON "name=value"
```

#### Arguments

A string expression: the variable's name, an equals sign, and its value.

#### Description

The variable is set in HyBASIC's own environment. The value is one word, whatever spaces are in it. Programs
that HyBASIC starts afterwards with `SHELL` get a copy of the environment and see the variable; the shell that
started HyBASIC does not, since a task can't change its parent's environment.

With nothing after the equals sign, the variable is removed.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | No equals sign in the string |
| 13 | `type mismatch` | The argument is a number |

#### See also

`ENVIRON$`, `SHELL`.

#### Example

```session
> ENVIRON "colour=red" : PRINT ENVIRON$("colour") : SHELL "cat /env/colour; echo"
red
red
> ENVIRON "colour=" : PRINT "["; ENVIRON$("colour"); "]"
[]
```

## EOF

A numeric function: whether a file has been read to its end.

#### Syntax

```text
EOF(f)
```

#### Arguments

*f*
: A numeric expression: the number of an open file.

#### Result

−1 (true) if there is nothing more to read from file *f*, and 0 (false) if there is. For a file opened `FOR
OUTPUT` or `FOR APPEND`, always −1.

#### Description

The usual loop reads while there is something to read: `DO UNTIL EOF(1) : LINE INPUT #1, L$ : ... : LOOP`.
Testing first matters, because reading at the end is an error (62, `input past end of file`), and an empty file
is at its end from the start.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 52 | `bad file number` | File *f* isn't open |
| 13 | `type mismatch` | *f* is a string |

#### See also

`OPEN`, `INPUT`, `LINE INPUT`, `LOF`, `LOC`.

#### Example

```session
> OPEN "/ram/two.txt" FOR OUTPUT AS #1 : PRINT #1, "one" : PRINT #1, "two" : CLOSE
> OPEN "/ram/two.txt" FOR INPUT AS #1
> DO UNTIL EOF(1) : LINE INPUT #1, L$ : PRINT L$; EOF(1) : LOOP : CLOSE
one 0
two-1
```

## EQV

An operator: bitwise equivalence.

#### Syntax

```text
a EQV b
```

#### Result

An integer. Each bit of it is 1 where the same bit of *a* and of *b* are alike (both 0 or both 1), and 0 where
they differ. It is `NOT (a XOR b)`.

For two truth values, −1 and 0, the result is true when they are the same.

The operands are evaluated, rounded and treated as `AND`'s are: see `AND`. `EQV` binds less tightly than `AND`,
`OR` and `XOR`, and more tightly than `IMP`.

#### Example

```session
> PRINT 5 EQV 3; (1 = 1) EQV (2 = 3); 0 EQV 0
-7  0 -1
```

## ERASE

A statement: drops arrays.

#### Syntax

```text
ERASE array [, array] ...
```

#### Arguments

*array*
: An array's name, with no parentheses.

#### Description

Each array named is dropped, and the memory it took is free again. The name is then as if it had never been
used: a `DIM` can make it again with other bounds, and using it without one makes it afresh with indexes 0 to
10, every element 0 or empty.

A name that isn't an array's is passed over without an error.

#### See also

`DIM`, `REDIM`, `CLEAR`.

#### Example

```session
> DIM A(3) : A(3) = 7 : ERASE A : DIM A(50) : PRINT A(3); UBOUND(A)
 0  50
```

## ERL, ERR, ERR$

Functions: the line, the number and the message of the last error.

#### Syntax

```text
ERR
ERR$
ERL
```

#### Result

`ERR` is the number of the most recent error, and 0 if there has been none. The numbers are QuickBASIC's where
it has an error of the same meaning (5 is `illegal function call`, 53 is `file not found`); the Hydra's own
start at 80. An error reported by the system that has no number of HyBASIC's is 256 plus the system's code
(Appendix G).

`ERR$` is that error's message, as it is printed: in small letters, with no line number. `error 0` if there has
been none.

`ERL` is the number of the line the error was on. If that line has no number, it is the number of the nearest
numbered line before it; in a program with no line numbers at all, it is the line's position in its file,
counted from 1. 0 if the error was in a line typed at the prompt, or if there has been none.

#### Description

The three keep their values until the next error: a statement that succeeds doesn't clear them. They are meant
to be read in an error handler (`ON ERROR GOTO`), where they tell it what happened and where.

#### See also

`ON`, `RESUME`, `ERROR`.

#### Example

```session
> PRINT ERR; ERL; ERR$
 0  0 error 0
> 10 X = 1 / 0
> RUN
line 10: division by zero
> PRINT ERR; ERL; ERR$
 11  10 division by zero
```

## ERROR

A statement: raises an error.

#### Syntax

```text
ERROR n
```

#### Arguments

*n*
: A numeric expression from 1 to 255: the error's number.

#### Description

The program behaves exactly as if error *n* had happened at this statement. If an `ON ERROR GOTO` handler is in
force, control goes to it, with `ERR` equal to *n* and `ERL` this line; otherwise the program stops and the
error's message is printed.

If *n* is one of HyBASIC's error numbers, the message is that error's. Any other number from 1 to 255 can be
used for an error of the program's own; its message is `error` and the number.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is less than 1 or more than 255 |

#### See also

`ON`, `RESUME`, `ERR`.

#### Example

```session
> 10 ON ERROR GOTO 50
> 20 ERROR 200
> 30 PRINT "after"
> 40 END
> 50 PRINT "caught"; ERR; ERL; ERR$ : RESUME NEXT
> RUN
caught 200  20 error 200
after
```

## EXIT

A statement: leaves a loop or a procedure early.

#### Syntax

```text
EXIT {FOR | DO | SUB | FUNCTION}
```

#### Description

`EXIT FOR` and `EXIT DO` leave the innermost loop of that kind that the statement is inside, and the program
goes on with the statement after the loop's `NEXT` or `LOOP`. A `FOR` loop's variable keeps the value it has.
An `EXIT DO` inside a `FOR` loop that is itself inside a `DO` loop leaves both.

`EXIT SUB` and `EXIT FUNCTION` return from the procedure at once. A function returns the value last assigned to
its name, or 0 or the empty string if none has been.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 80 | `block without its start or end` | `EXIT FOR` or `EXIT DO` that is not inside such a loop |
| 85 | `not in a procedure` | `EXIT SUB` or `EXIT FUNCTION` outside a `SUB` or a `FUNCTION` |

#### Notes

There is no `EXIT WHILE`, and `EXIT DO` doesn't leave a `WHILE` ... `WEND` loop. A loop that needs an early way
out should be written with `DO`.

#### See also

`FOR`, `DO`, `SUB`, `FUNCTION`.

#### Example

```session
> 10 FOR I = 1 TO 100 : IF I * I > 50 THEN EXIT FOR
> 20 NEXT : PRINT I
> RUN
 8
```

## EXP

A numeric function: e raised to a power.

#### Syntax

```text
EXP(x)
```

#### Arguments

*x*
: A numeric expression whose value is real.

#### Result

e to the power *x*, where e is 2.71828... `EXP(0)` is the integer 1. Any other result is a decimal, rounded to
the number of significant digits that `DIGITS` has set (12 unless it has been changed).

There is no overflow. A large result is a decimal of that many significant digits followed by zeros, and is
printed in full.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### See also

`LOG`, `SQR`, `DIGITS`.

#### Example

```session
> PRINT EXP(0); EXP(1); EXP(-1); EXP(1/2)
 1  2.71828182846  0.367879441171  1.6487212707
> PRINT EXP(100)
 26881171418200000000000000000000000000000000
```

## FIB

A numeric function: a Fibonacci number.

#### Syntax

```text
FIB(n)
```

#### Arguments

*n*
: A numeric expression whose value is an integer, 0 or more.

#### Result

The *n*th Fibonacci number, an exact integer of whatever length: `FIB(0)` is 0, `FIB(1)` is 1, and each one
after is the sum of the two before it.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is negative, or is not an integer |
| 13 | `type mismatch` | *n* is a string |

#### See also

`GCD`.

#### Example

```session
> PRINT FIB(0); FIB(1); FIB(2); FIB(30); FIB(100)
 0  1  1  832040  354224848179261915075
```

## FILES

A statement: lists the names in a directory.

#### Syntax

```text
FILES [path$]
```

#### Arguments

*path$*
: A string expression: a directory's name. The current directory if it is left out.

#### Description

The names are printed across the screen, each in a column 16 characters wide, five to a line, in the
directory's own order. A name that is a directory has a `/` after it. Nothing else is shown: for sizes and
dates, use `SHELL "ls -l"`.

If *path$* is a file and not a directory, nothing is listed.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 53 | `file not found` | There is no such name as *path$* |
| 13 | `type mismatch` | *path$* is a number |

#### Notes

*path$* is a directory, not a pattern: `FILES "*.BAS"` looks for a directory of that name.

#### See also

`DIR$`, `CHDIR`, `KILL`, `NAME`, `SHELL`.

#### Example

```session
> FILES "/rom/lib"
as/             basic/          edit/           font/           forth/
hylang/         namespace       profile         shell           windows
```

## FIX

A numeric function: a real number with its fraction removed.

#### Syntax

```text
FIX(x)
```

#### Arguments

*x*
: A numeric expression whose value is real.

#### Result

The integer part of *x*: *x* rounded toward zero. For a positive number this is `INT(x)`. For a negative number
that isn't whole it is one more: `FIX(-2.5)` is −2, where `INT(-2.5)` is −3. The result is an exact integer of
whatever length.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### See also

`INT`, `CINT`, `FIXED`.

#### Example

```session
> PRINT FIX(2.5); FIX(-2.5); FIX(7/2); FIX(-7/2); INT(-2.5)
 2 -2  3 -3 -3
```

## FIXED

A numeric function: a real number as a decimal with a given number of places.

#### Syntax

```text
FIXED(x, n)
```

#### Arguments

*x*
: A numeric expression whose value is real.

*n*
: A numeric expression, 0 or more: the number of decimal places.

#### Result

A decimal: *x* with the digits after its *n*th decimal place cut off. The digits are cut, not rounded, so the
result is never further from zero than *x*: `FIXED(2/3, 2)` is 0.66, and `FIXED(-2/3, 2)` is −0.66. Zeros at
the end are not kept, so `FIXED(5, 2)` is 5.

#### Description

This is the way to turn a rational into a decimal. `1/3` is kept and printed as `1/3`; `FIXED(1/3, 6)` is
0.333333. *n* is not limited by `DIGITS`, which is about the mathematical functions.

To round instead of cutting, add half of the last place first: `FIXED(x + 0.005, 2)` rounds a positive *x* to
two places.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | An argument is a string |

#### See also

`RATIONAL`, `FIX`, `CINT`, `DIGITS`, `PRINT` (`USING`).

#### Example

```session
> PRINT FIXED(22/7, 4); FIXED(2/3, 2); FIXED(-2/3, 2); FIXED(5, 2); FIXED(1.239, 2)
 3.1428  0.66 -0.66  5  1.23
> PRINT FIXED(1/3, 30)
 0.333333333333333333333333333333
```

## FOR ... NEXT

A statement block: a loop that counts a variable from one value to another.

#### Syntax

```text
FOR v = start TO limit [STEP step]
    [statements]
NEXT [v [, v] ...]
```

#### Arguments

*v*
: The loop's counter: a simple numeric variable. An array's element can't be one.

*start*, *limit*, *step*
: Numeric expressions. *step* is 1 if `STEP` is left out.

#### Description

1. *start* is assigned to *v*, and *limit* and *step* are evaluated and kept. They are evaluated this once only:
   a variable they were worked out from can change inside the loop without changing them.
2. The test: if *step* is zero or more and *v* is greater than *limit*, or *step* is negative and *v* is less than
   *limit*, the loop ends, and the program goes on after its `NEXT`. The test is made before the first pass too,
   so the statements may not run at all; *v* then keeps the value *start*.
3. Otherwise the statements run.
4. At `NEXT`, *step* is added to *v*, and the loop goes back to the test at step 2.

When the loop ends at the test, *v* holds the first value that failed it: after `FOR I = 1 TO 3`, `I` is 4.

The addition is exact. A loop with a fractional step, `STEP 0.1` or `STEP 1/3`, reaches its limit on the last
pass, where a binary floating-point BASIC may stop one short.

The statements may assign to *v*; `NEXT` adds *step* to whatever it then holds.

`EXIT FOR` leaves the innermost loop at once, with *v* as it is.

`NEXT` with no variable closes the innermost open loop. `NEXT v` does the same, and the compiler checks that *v*
is that loop's counter. `NEXT j, i` is `NEXT j : NEXT i`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 26 | `FOR without NEXT` | A `FOR` whose `NEXT` isn't there. Found when the program is compiled: nothing has run |
| 1 | `NEXT without FOR` | A `NEXT` with no loop open, or naming a variable that isn't the innermost loop's counter |
| 80 | `block without its start or end` | The loop and another block overlap: a `NEXT` inside a one-line `IF`, say |
| 13 | `type mismatch` | *v* is a string variable, or *start*, *limit* or *step* a string |
| 2 | `syntax error` | *v* is not a simple variable |

#### Notes

With a *step* of 0 the counter never changes and the loop doesn't end by itself: only `EXIT FOR`, an error or
<kbd>CTRL</kbd>-<kbd>C</kbd> ends it.

At the prompt, a line is compiled alone, so a loop typed there must be whole on its line: `FOR I = 1 TO 3` by
itself is `FOR without NEXT`.

#### See also

`EXIT`, `DO`...`LOOP`, `WHILE`...`WEND`.

#### Example

```session
> FOR I = 1 TO 3 : PRINT I; : NEXT : PRINT "then"; I
 1  2  3 then 4
> FOR I = 5 TO 1 : PRINT I; : NEXT : PRINT "then"; I
then 5
> FOR X = 0 TO 1 STEP 1/3 : PRINT X; : NEXT : PRINT
 0  1/3  2/3  1
> N = 3 : FOR I = 1 TO N : N = 10 : PRINT I; : NEXT : PRINT
 1  2  3
> FOR I = 1 TO 2 : FOR J = 1 TO 2 : PRINT I; J; : NEXT J, I : PRINT
 1  1  1  2  2  1  2  2
> FOR I = 1 TO 2 : FOR J = 1 TO 2 : NEXT I
NEXT without FOR
```

## FRE

A numeric function: how much memory is free.

#### Syntax

```text
FRE()
FRE(x)
```

#### Arguments

*x*
: Any expression. It is evaluated and ignored; QuickBASIC's forms `FRE(0)` and `FRE("")` both work.

#### Result

The number of bytes left for the program's data: its strings, its arrays and its numbers too long for a
variable's own five bytes. The figure is the room in the RAM banks HyBASIC has not yet used, so it falls as
arrays are made and strings grow, and rises when they are dropped.

#### Notes

With three RAM modules a new HyBASIC has about 300K free.

#### See also

`DIM`, `ERASE`, `CLEAR`.

#### Example

The figure itself depends on the machine. An array of 5,001 numbers takes five bytes for each, and twelve for
the array:

```session
> A = FRE() : DIM BIG(5000) : PRINT A - FRE()
 25017
```

## FREEFILE

A numeric function: a file number that is not in use.

#### Syntax

```text
FREEFILE
```

#### Result

The lowest number, from 1 up, that no open file has. It is the number to give the next `OPEN` in a routine that
can't know which numbers the rest of the program is using.

#### Notes

`FREEFILE` finds a number. It doesn't reserve it: call it again only after the `OPEN`, or it returns the same
one.

Since 8 files can be open at once and numbers go to 255, there is always a free number. When 8 are open the
`OPEN` fails instead, with `too many files`.

#### See also

`OPEN`, `CLOSE`.

#### Example

```session
> PRINT FREEFILE : OPEN "/ram/x" FOR OUTPUT AS #1 : OPEN "/ram/y" FOR OUTPUT AS #3
 1
> PRINT FREEFILE : CLOSE #1 : PRINT FREEFILE : CLOSE
 2
 1
```

## FUNCTION ... END FUNCTION

A block: defines a function, which returns a value and is used in expressions.

#### Syntax

```text
FUNCTION name [(parameter [, parameter] ...)] [STATIC]
    [statements]
    name = value
END FUNCTION
```

#### Arguments

*name*
: The function's name. It returns a string if the name ends in `$`, and a number otherwise.

*parameter*
: A variable's name, optionally followed by `AS type`; or an array's, written with empty parentheses.

`STATIC`
: The function's variables keep their values from one call to the next. See `STATIC`.

#### Description

The function is called by using its name in an expression, with an argument for each parameter: `PRINT Area(3,
4)`. A function with no parameters is used as a bare name.

The value returned is whatever was last assigned to the function's name when the function ends, at `END
FUNCTION` or at an `EXIT FUNCTION`. If nothing was assigned, it is 0, or the empty string.

Arguments are passed as a `SUB`'s are (see `CALL`): a variable or an array's element by reference, so that
assigning to the parameter changes it; any other expression by value. To protect a variable, pass it in
parentheses of its own.

The variables used inside the function are its own. A name there is a new variable, 0 or empty at each call,
even if the main program has a variable of the same name; the main program's is not touched. Names declared
`SHARED`, and constants, are the exceptions.

A function may call itself. Each call has its own parameters and variables.

Functions are found when the program is compiled, wherever they are in its text, so one can be used on a line
before its definition. When control reaches a `FUNCTION` line in the ordinary run of the program, it skips to
after `END FUNCTION`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 37 | `argument-count mismatch` | A call with more arguments, or fewer, than the function has parameters |
| 10 | `duplicate definition` | Two procedures of one name |
| 13 | `type mismatch` | An argument of the wrong type; a string assigned to a numeric function's name |
| 80 | `block without its start or end` | `FUNCTION` with no `END FUNCTION` |
| 2 | `syntax error` | `FUNCTION` typed at the prompt: a function can be defined only in a program |

#### Notes

A name followed by parentheses that is not a function's is an array's. Calling a function that doesn't exist,
or misspelling one, is therefore not an error: it makes an array of that name and returns its element, 0.

#### See also

`SUB`, `CALL`, `EXIT`, `SHARED`, `STATIC`, `DECLARE`.

#### Example

```session
> 10 FUNCTION Fact (N)
> 20 IF N <= 1 THEN Fact = 1 ELSE Fact = N * Fact(N - 1)
> 30 END FUNCTION
> 40 FUNCTION Bump (V) : V = V + 1 : X = 99 : Bump = V : END FUNCTION
> 50 PRINT Fact(5); Fact(25)
> 60 X = 1 : Z = 1 : PRINT Bump(Z); Z; Bump((Z)); Z; X
> 70 PRINT Fcat(5)
> RUN
 120  15511210043330985984000000
 2  2  3  2  1
 0
```
