## GCD

A numeric function: the greatest common divisor of two integers.

#### Syntax

```text
GCD(a, b)
```

#### Arguments

*a*, *b*
: Numeric expressions whose values are real. Each is rounded to the nearest integer, a half going to the even
  one.

#### Result

The largest integer that divides both *a* and *b* exactly. It is never negative: the signs of *a* and *b* make
no difference. `GCD(a, 0)` is the size of *a*, and `GCD(0, 0)` is 0. The integers can be of any length.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | An argument is complex |
| 13 | `type mismatch` | An argument is a string |
| 2 | `syntax error` | One argument only |

#### Notes

The least common multiple of two positive integers is `a * b / GCD(a, b)`.

Rationals are always kept in lowest terms, so there is no need for `GCD` to reduce a fraction.

#### See also

`MOD`, `NUMERATOR`, `DENOMINATOR`, `FIB`.

#### Example

```session
> PRINT GCD(12, 18); GCD(-12, 18); GCD(0, 5); GCD(17, 5); GCD(2 ^ 64, 2 ^ 40)
 6  6  5  1  1099511627776
```

## GET

A statement: reads a value from a `BINARY` file, at a given position.

#### Syntax

```text
GET [#]f, [position], v
```

#### Arguments

*f*
: A numeric expression: the number of a file opened `FOR BINARY`.

*position*
: A numeric expression: where to read, counted in bytes from 1 for the file's first byte. If it is left out
  (the two commas are still written) or is less than 1, the read starts where the last `GET` or `PUT` on the
  file ended.

*v*
: A variable: where the value goes. Its type and, for a string, its present length decide how many bytes are
  read.

#### Description

**A string variable** is filled with as many bytes from the file as it holds now. To read ten bytes, make the
variable ten bytes long first, with `SPACE$(10)` say. If the file ends first, the bytes that weren't there are
left as zero bytes; the variable keeps its length.

**A numeric variable** reads one number in the stored format, the form `PUT` writes and `MKN$` makes. A number
takes as many bytes as it needs, which the number's own first bytes say, so numbers written one after another
are read back one after another without their lengths being known.

After the read, the file's position is just past what was read.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 52 | `bad file number` | File *f* isn't open |
| 54 | `bad file mode` | File *f* was not opened `FOR BINARY` |
| 62 | `input past end of file` | A number was to be read, and the file had ended |

#### Notes

There are no records and no `FIELD`: QuickBASIC's random files are not here. `GET` and `PUT` always work in
bytes, and a program that wants records of a fixed size works out each one's position itself.

#### See also

`PUT`, `OPEN`, `SEEK`, `LOC`, `LOF`, `CVN`, `MKN$`, `INPUT$`.

#### Example

```session
> OPEN "/ram/b.dat" FOR BINARY AS #1 : A$ = "HELLO" : PUT #1, 1, A$ : N = 1/3 : PUT #1, , N
> PRINT LOF(1) : B$ = SPACE$(3) : GET #1, 2, B$ : PRINT B$
 8
ELL
> GET #1, 6, M : PRINT M; EOF(1) : CLOSE
 1/3 -1
```

## GOSUB

A statement: calls a subroutine, which is a part of the program that ends with `RETURN`.

#### Syntax

```text
GOSUB {line | label}
```

#### Arguments

*line*
: A line number.

*label*
: A label: a name with a colon after it at the start of a line that has no number.

#### Description

Control goes to the line named, and the place to come back to is remembered. The next `RETURN` that runs comes
back to the statement after the `GOSUB`.

Subroutines can call others, and themselves; each `RETURN` goes back to the most recent `GOSUB` that hasn't
been returned from.

A subroutine is not a block: it is only lines of the program, and nothing stops control from running into them
from above. The usual arrangement is to put the subroutines at the end of the program, with an `END` before
them.

A subroutine shares every variable with the code that called it. A `SUB` or a `FUNCTION`, with parameters and
variables of its own, is nearly always the better tool.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 8 | `label not defined` | No line has that number or that label. Found when the program is compiled |
| 3 | `RETURN without GOSUB` | A `RETURN` ran with no `GOSUB` to go back to |
| 81 | `out of stack space` | Too many `GOSUB`s without a `RETURN` |

#### Notes

A `GOSUB` inside a `SUB` or `FUNCTION` can only reach lines of that same procedure.

#### See also

`RETURN`, `GOTO`, `ON`, `SUB`, `FUNCTION`.

#### Example

```session
> 10 GOSUB 100 : PRINT "back" : END
> 100 PRINT "in the subroutine" : RETURN
> RUN
in the subroutine
back
```

## GOTO

A statement: goes to another line.

#### Syntax

```text
GOTO {line | label}
```

#### Description

Control goes to the first statement of the line with that number or that label, and carries on from there.

The same jump is written inside an `IF` as `IF c THEN 100` or `IF c GOTO 100`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 8 | `label not defined` | No line has that number or that label. Found when the program is compiled |

#### Notes

A label marks a line that has no number. In a program typed with line numbers, the targets are numbers; in a
program written in a file without them, they are labels, and a line may still be given a number so that an old
listing's `GOTO 100` works.

`GOTO` typed at the prompt can't jump into the program: a line typed there is compiled alone, and the program's
lines are not known to it (`label not defined`).

#### See also

`GOSUB`, `ON`, `IF`, `DO`, `EXIT`.

## GPRINT

A statement: writes text on the bitmap.

#### Syntax

```text
GPRINT [STEP] (x, y), text$
```

#### Arguments

*x*, *y*
: Numeric expressions: where the top left corner of the first character goes. With `STEP`, an offset from the
  last point referenced.

*text$*
: A string expression. Its first 40 characters are drawn.

#### Description

Needs a graphics mode. The characters are drawn as dots, in the console's font and the pen's colour.

This is text that is part of the picture. It is not the console's text, which `PRINT` writes and which lies
over the bitmap in its own layer; `CLS` clears both.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | No graphics mode is set |
| 258 | `invalid argument` | The point is far off the bitmap |
| 13 | `type mismatch` | *text$* is a number |

#### See also

`PRINT`, `LOCATE`, `COLOR`, `SCREEN`.

## HEX$

A string function: a number written in hexadecimal.

#### Syntax

```text
HEX$(x)
```

#### Arguments

*x*
: A numeric expression of any kind.

#### Result

The text of *x* in base 16, as `STR$(x, "x")` would give it, with no space in front. The digits above 9 are the
capitals A to F.

This is the number itself written in another base, not the contents of a machine word. A negative number has a
minus sign, so `HEX$(-1)` is `-1`, where QuickBASIC gives `FFFF`. A number that isn't an integer is written as
what it is: 2.5 is `2.8` (two and eight sixteenths), and 1/3 is `1/3`. An integer can be of any length.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *x* is a string |

#### Notes

For the two's complement form of a negative number in some number of bits, mask it first: `HEX$(-1 AND
&HFFFF)` is `FFFF`.

To read hexadecimal text back, put `&H` before it and use `VAL`.

#### See also

`OCT$`, `STR$`, `BASE`, `VAL`.

#### Example

```session
> PRINT HEX$(255); " "; HEX$(-1); " "; HEX$(2 ^ 40); " "; HEX$(2.5); " "; HEX$(-1 AND &HFFFF)
FF -1 10000000000 2.8 FFFF
```

## IF

A statement: runs statements only if a condition holds. It has a one-line form and a block form.

#### Syntax

```text
IF condition THEN statements [ELSE statements]
IF condition THEN line
IF condition GOTO line

IF condition THEN
    [statements]
[ELSEIF condition THEN
    [statements]] ...
[ELSE
    [statements]]
END IF
```

#### Arguments

*condition*
: A numeric expression. It holds if its value is not 0. A comparison gives −1 or 0, but any number will do: 1/2
  holds.

*line*
: A line number or a label, which is jumped to.

#### Description

**The one-line form** is all on one line. If the condition holds, the statements after `THEN` run, up to the
`ELSE` or the end of the line; if it doesn't, the statements after `ELSE` run, if there is an `ELSE`. Either
part can be several statements with colons between them, and they all belong to the `IF`: nothing on the line
after `THEN` runs when the condition fails, except what follows `ELSE`.

A one-line `IF` can hold another. An `ELSE` belongs to the nearest `IF` before it that has none.

**The block form** has nothing after `THEN` on the first line. The conditions are tested in order, the `IF`'s
and then each `ELSEIF`'s, and the statements under the first one that holds are run; the rest are skipped, their
conditions not evaluated. If none holds, the statements under `ELSE` run. Control then goes on after `END IF`.

Block `IF`s nest to any depth, and can hold loops and other blocks, each closed inside the part it began in.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | The condition is a string. (Two strings can be compared: the comparison is a number) |
| 80 | `block without its start or end` | A block `IF` with no `END IF`; `ELSE`, `ELSEIF` or `END IF` with no block `IF` |
| 8 | `label not defined` | `THEN` or `GOTO` with a line that isn't there |

#### Notes

At the prompt only the one-line form can be used, since a line typed there is compiled alone.

Both sides of an `AND` or an `OR` in a condition are always evaluated. To test one thing only if another holds,
use two `IF`s.

#### See also

`SELECT`, `AND`, `OR`, `NOT`, `GOTO`.

#### Example

```session
> 10 IF 1 THEN PRINT "a"; : PRINT "b" ELSE PRINT "c"; : PRINT "d"
> 20 IF 0 THEN PRINT "a"; : PRINT "b" ELSE PRINT "c"; : PRINT "d"
> 30 IF 5 THEN IF 0 THEN PRINT "x" ELSE PRINT "inner else"
> 40 X = 3
> 50 IF X = 1 THEN
> 60   PRINT "one"
> 70 ELSEIF X = 3 THEN
> 80   PRINT "three"
> 90 ELSE
> 100  PRINT "other"
> 110 END IF
> RUN
ab
cd
inner else
three
```

## IMAG

A numeric function: the imaginary part of a number.

#### Syntax

```text
IMAG(z)
```

#### Arguments

*z*
: A numeric expression of any kind.

#### Result

The imaginary part of *z*, a real number of the kind it has in *z*. 0 if *z* is real.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *z* is a string |

#### See also

`REAL`, `COMPLEX`.

#### Example

```session
> PRINT IMAG(1+2i); REAL(1+2i); IMAG(5); IMAG(1/2+0.5i)
 2  1  0  0.5
```

## IMP

An operator: bitwise implication.

#### Syntax

```text
a IMP b
```

#### Result

An integer. Each bit of it is 0 where that bit is 1 in *a* and 0 in *b*, and 1 everywhere else. It is `(NOT a)
OR b`.

For two truth values, −1 and 0, the result is false only when *a* is true and *b* is false: "if *a*, then
*b*".

The operands are evaluated, rounded and treated as `AND`'s are: see `AND`. `IMP` binds less tightly than every
other operator.

#### Example

```session
> PRINT -1 IMP 0; 0 IMP 0; 0 IMP -1; -1 IMP -1; 5 IMP 3
 0 -1 -1 -1 -5
```

## INCLUDE

A statement: reads another file's lines into the program at this point.

#### Syntax

```text
INCLUDE "file"
'$INCLUDE: 'file'
REM $INCLUDE: 'file'
```

#### Arguments

*file*
: The file's name, written out: a literal, not an expression.

#### Description

The lines of *file* are compiled as if they were written where the `INCLUDE` line is. The statement must be on
a line of its own. The second and third forms are QuickBASIC's, in which the statement is disguised as a
comment; they do the same.

A name that starts with `/` is used as it is. Any other name is looked for in the directory the program itself
came from, and then in `/lib/basic`.

An included file can include others, four deep. A program can include eight files in all.

An error in an included file is reported with that file's name and its own line number there.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 53 | `file not found` | The file is in neither place |
| 12 | `illegal in direct mode` | `INCLUDE` typed at the prompt |
| 2 | `syntax error` | `INCLUDE` with other statements on its line |

#### Notes

Including is done when the program is compiled, not when control reaches the line, so a file can't be chosen
by the running program. To run another program from a program, use `SHELL "basic file"`.

#### See also

`LOAD`, `SHELL`, `DECLARE`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo 'PRINT "included"' >inc.bas
% echo 'PRINT "main"' >main.bas
% echo 'INCLUDE "inc.bas"' >>main.bas
% echo 'PRINT "after"' >>main.bas
% basic main.bas
main
included
after
```

## INKEY$

A string function: a key that has been pressed, without waiting for one.

#### Syntax

```text
INKEY$
```

#### Result

The empty string if no key is waiting. Otherwise one key, which is taken out of the keyboard's queue:

* An ordinary key is a string of one byte, the key's character.
* A cursor key or a function key is a string of two bytes: `CHR$(0)`, then a code for the key. The codes are
  QuickBASIC's: 72 is up, 80 is down, 59 is F1, and so on.

#### Description

`INKEY$` never waits. The usual use is in a loop that does something else as well, or one that waits for any
key: `DO : K$ = INKEY$ : LOOP UNTIL K$ <> ""`.

#### Notes

A loop that only polls `INKEY$` takes all the processor time it can get. Put a `SLEEP` of a short time in it if
other tasks are running.

To wait for a set number of keys, use `INPUT$`.

#### See also

`INPUT$`, `INPUT`, `LINE INPUT`, `SLEEP`.

## INPUT

A statement: reads a line and assigns its fields to variables. The line comes from the console, or from a file.

#### Syntax

```text
INPUT [;] ["prompt" {; | ,}] v [, v] ...
INPUT #f, v [, v] ...
```

#### Arguments

*prompt*
: A string in quotes, written before the line is read. It must be a literal: a string variable there is a syntax
  error. After it, `;` adds a question mark and a space, and `,` adds nothing. With no prompt at all, `? ` is
  written.

*v*
: A numeric or string variable, or an array's element. Each takes one field of the line.

*f*
: The number of a file opened `FOR INPUT` or `FOR BINARY`.

#### Description

**From the console.** The prompt is written and one line is read, with the console's line editing (section 13.2).
Only its first 255 bytes are used. The line is split into fields at its commas, and each variable takes a field,
in order:

* A numeric variable's field is read as a number, in any form a program's own numbers may take: `12`, `-7/2`,
  `2.5`, `1e3`, `&HFF`. An empty field is 0.
* A string variable's field is the text up to the next comma, with the spaces before and after it dropped. A
  field that starts with a double quote runs to the closing quote instead, and can hold commas and spaces.

If the line has fewer fields than there are variables, or more, or a numeric field that isn't a number, HyBASIC
writes `?Redo from start` and the statement starts again from its prompt. It is never an error.

**From a file.** One line of file *f* is read, and split and assigned in the same way, with three differences.
There is no prompt and nothing is asked again: a numeric field that isn't a number is 0. If the line runs out of
fields before the variables do, the next line is read and its fields carry on. Fields left on the line after the
last variable is filled are dropped: the next `INPUT #` starts at the next line.

**The semicolon after `INPUT`** matters only when input doesn't come from the console (`basic prog <answers`).
The console echoes the Return that ends a line; a file doesn't, so HyBASIC writes a new line itself after reading
one. `INPUT ;` leaves it out, and what is printed next follows the prompt on the same line.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 62 | `input past end of file` | No line is left: in file *f*, or in standard input when it isn't the console |
| 52 | `bad file number` | File *f* isn't open |
| 54 | `bad file mode` | File *f* is open `FOR OUTPUT` or `FOR APPEND` |
| 2 | `syntax error` | The prompt isn't a string literal |

<kbd>CTRL</kbd>-<kbd>C</kbd> at the prompt stops the program, as it does anywhere else (`Break`).

#### Notes

To read a whole line, commas and all, into one string, use `LINE INPUT`. To read keys without waiting for
Return, use `INKEY$` or `INPUT$`.

#### See also

`LINE INPUT`, `INPUT$`, `INKEY$`, `OPEN`, `EOF`, `READ`.

#### Example

```session ?_ Two_numbers?_ Name,_age:_
> 10 INPUT "Two numbers"; A, B
> 20 PRINT A + B
> 30 INPUT "Name, age: ", N$, G
> 40 PRINT "["; N$; "]"; G
> 50 INPUT X, Y$
> 60 PRINT X; "["; Y$; "]"
> RUN
Two numbers? 3
?Redo from start
Two numbers? 1/2, 2.5
 3
Name, age: Sam Smith , 12
[Sam Smith] 12
? , "x, y"
 0 [x, y]
```

Fields that carry on from one line of a file to the next:

```session
> 10 OPEN "/ram/n.txt" FOR OUTPUT AS #1
> 20 PRINT #1, "1" : PRINT #1, "2, 3" : PRINT #1, "4"
> 30 CLOSE #1 : OPEN "/ram/n.txt" FOR INPUT AS #1
> 40 INPUT #1, A, B : PRINT A; B
> 50 INPUT #1, C : PRINT C
> 60 INPUT #1, D
> RUN
 1  2
 4
line 60: input past end of file
```

## INPUT$

A string function: a set number of keys from the keyboard, or of bytes from a file.

#### Syntax

```text
INPUT$(n)
INPUT$(n, [#]f)
```

#### Arguments

*n*
: A numeric expression: how many keys or bytes.

*f*
: A numeric expression: the number of a file opened `FOR INPUT` or `FOR BINARY`.

#### Result

**From the keyboard**, `INPUT$(n)` waits until *n* keys have been typed and returns them as a string of *n*
bytes. Nothing is echoed and Return is a key like any other, so this is the way to read a key press, a password,
or a menu choice without the user pressing Return. When standard input is not a console, it reads *n* bytes
from it instead.

**From a file**, `INPUT$(n, #f)` returns the next *n* bytes of file *f*, whatever they are, line ends included,
and moves the file's position past them.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 62 | `input past end of file` | The file, or standard input, ends before *n* bytes |
| 52 | `bad file number` | File *f* isn't open |
| 54 | `bad file mode` | File *f* is open `FOR OUTPUT` or `FOR APPEND` |

#### Notes

To read a whole file into a string, ask for its length: `INPUT$(LOF(1), #1)`.

#### See also

`INKEY$`, `INPUT`, `LINE INPUT`, `GET`, `LOF`.

#### Example

```session
> OPEN "/ram/t.txt" FOR OUTPUT AS #1 : PRINT #1, "abcdef" : CLOSE
> OPEN "/ram/t.txt" FOR INPUT AS #1 : PRINT INPUT$(3, #1); "|"; INPUT$(2, 1); "|"; LOC(1)
abc|de| 5
> CLOSE
```

## INSTR

A numeric function: where one string is found inside another.

#### Syntax

```text
INSTR([start,] s$, t$)
```

#### Arguments

*start*
: A numeric expression, an integer from 1 up: the position in *s$* to start looking from. 1 if it is left out.

*s$*
: A string expression: the string to look in.

*t$*
: A string expression: the string to look for.

#### Result

The position in *s$*, counting its first byte as 1, of the first place at or after *start* where *t$* appears
whole. 0 if it appears nowhere from there on, which includes the cases where *start* is beyond the end of
*s$* and where *t$* is longer than what is left.

If *t$* is empty, the result is *start*, as long as *start* is within *s$*.

The comparison is of bytes: capitals and small letters are different.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *start* is less than 1, or is not an integer |
| 13 | `type mismatch` | *s$* or *t$* is a number |

#### Notes

To find something without regard to case, compare capitals: `INSTR(UCASE$(s$), UCASE$(t$))`.

To find every place, call it again with *start* one past the last result.

#### See also

`MID$`, `LEFT$`, `RIGHT$`, `LEN`.

#### Example

```session
> PRINT INSTR("hello", "l"); INSTR(4, "hello", "l"); INSTR("hello", "z"); INSTR("Hello", "h")
 3  4  0  0
> PRINT INSTR("hello", ""); INSTR(9, "hello", "l")
 1  0
```

## INT

A numeric function: a real number rounded down.

#### Syntax

```text
INT(x)
```

#### Arguments

*x*
: A numeric expression whose value is real.

#### Result

The largest integer that is not more than *x*. For a negative number that isn't whole this is the next integer
*below* it: `INT(-2.5)` is −3. The result is an exact integer of whatever length.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### Notes

The three ways to make an integer differ only for numbers that aren't whole: `INT` rounds down, `FIX` rounds
toward zero, and `CINT` rounds to the nearest.

#### See also

`FIX`, `CINT`, `MOD`, `FIXED`.

#### Example

```session
> PRINT INT(2.5); INT(-2.5); INT(7/2); INT(-7/2); INT(9)
 2 -3  3 -4  9
```

## IS

Part of `SELECT CASE`: `CASE IS < 10` tests the selected value with a comparison. See `SELECT`.

## KILL

A statement: removes a file.

#### Syntax

```text
KILL path$
```

#### Arguments

*path$*
: A string expression: the file's name. It is one name, not a pattern.

#### Description

The file is removed from its directory, and the space it took is free. There is no way to get it back.

A directory can be removed with `KILL` too, if it is empty, though `RMDIR` says more clearly what is meant.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 53 | `file not found` | There is no such file |
| 263 | `busy` | The file is open, in this program or any other |
| 70 | `permission denied` | The file is read-only, or its disk is |
| 13 | `type mismatch` | *path$* is a number |

#### See also

`NAME`, `RMDIR`, `FILES`, `OPEN`.

#### Example

```session
> OPEN "/ram/u.txt" FOR OUTPUT AS #1 : KILL "/ram/u.txt"
busy
> CLOSE : KILL "/ram/u.txt" : KILL "/ram/u.txt"
file not found
```

## LBOUND, UBOUND

Numeric functions: the lowest and the highest index of an array.

#### Syntax

```text
LBOUND(array [, dimension])
UBOUND(array [, dimension])
```

#### Arguments

*array*
: An array's name, with no parentheses.

*dimension*
: A numeric expression: which dimension, counting the first as 1. 1 if it is left out.

#### Result

The lowest, or the highest, index the array allows in that dimension, as `DIM` set them.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *dimension* is less than 1 or more than the array has |

#### Notes

If the name has not been used as an array, asking for its bounds makes it one, with the usual indexes 0 to 10:
`UBOUND` of a misspelled name is 10, not an error.

A loop over a whole array is best written `FOR I = LBOUND(A) TO UBOUND(A)`, which goes on being right if the
`DIM` is changed.

#### See also

`DIM`, `REDIM`, `OPTION`.

#### Example

```session
> DIM A(2 TO 5, 7) : PRINT LBOUND(A); UBOUND(A); LBOUND(A, 2); UBOUND(A, 2)
 2  5  0  7
```

## LCASE$, UCASE$

String functions: a string with its letters made small, or made capital.

#### Syntax

```text
LCASE$(s$)
UCASE$(s$)
```

#### Arguments

*s$*
: A string expression.

#### Result

A string of the same length. `LCASE$` turns each of the capitals A to Z into its small letter; `UCASE$` turns
each of a to z into its capital. Every other byte is copied as it is, including the accented letters above 127
in the console's character sets.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *s$* is a number |

#### See also

`INSTR`, `LTRIM$`.

#### Example

```session
> PRINT LCASE$("Hello World 123"); " "; UCASE$("Hello World 123")
hello world 123 HELLO WORLD 123
```

## LEFT$, RIGHT$

String functions: the first, or the last, bytes of a string.

#### Syntax

```text
LEFT$(s$, n)
RIGHT$(s$, n)
```

#### Arguments

*s$*
: A string expression.

*n*
: A numeric expression, 0 or more: how many bytes. One that isn't an integer is rounded to the nearest, a half
  going to the even one.

#### Result

`LEFT$` is the first *n* bytes of *s$*, and `RIGHT$` the last *n*. If *s$* has fewer than *n* bytes, the result
is all of it. If *n* is 0, the result is the empty string.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is negative |
| 13 | `type mismatch` | *s$* is a number, or *n* a string |

#### See also

`MID$`, `LEN`, `INSTR`.

#### Example

```session
> PRINT "["; LEFT$("hello", 2); "]["; RIGHT$("hello", 2); "]["; LEFT$("hello", 9); "]["; RIGHT$("hi", 0); "]"
[he][lo][hello][]
```

## LEN

A numeric function: the length of a string.

#### Syntax

```text
LEN(s$)
```

#### Arguments

*s$*
: A string expression.

#### Result

The number of bytes in *s$*: 0 for the empty string. Every byte counts, zero bytes and line ends included.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *s$* is a number. `LEN` does not give the size of a numeric variable |

#### Notes

`LEN = n` at the end of an `OPEN` statement is a different thing: see `OPEN`.

#### See also

`LEFT$`, `MID$`, `INSTR`, `STR$`.

#### Example

```session
> PRINT LEN("hello"); LEN(""); LEN(CHR$(0) + CHR$(0)); LEN(STRING$(1000, "x"))
 5  0  2  1000
```

## LET

A statement: gives a variable a value.

#### Syntax

```text
[LET] variable = expression
```

#### Arguments

*variable*
: A variable, an array's element, a record or one of its fields.

*expression*
: An expression of the variable's type.

#### Description

The expression is evaluated, and its value becomes the variable's. The word `LET` is optional and changes
nothing.

A numeric variable takes any number as it is: an integer of any length, a rational, a decimal or a complex
number. Nothing is rounded or converted on the way in. A string variable takes a string of any length up to
64K.

Assigning to a function's own name, inside the function, sets the value it will return.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | A string for a numeric variable, or a number for a string variable |
| 10 | `duplicate definition` | The name is a constant's |
| 9 | `subscript out of range` | An array's element that isn't there |

#### Notes

`=` is also the comparison "is equal to". Which one is meant depends on where it stands: the first `=` of an
assignment assigns, and any after it compare, so `A = B = C` gives `A` the truth of `B = C`.

A line typed at the prompt that starts with a number is taken as a line of the program, whatever follows the
number.

#### See also

`CONST`, `DIM`, `SWAP`, `READ`.

#### Example

```session
> B = 2 : C = 2 : A = B = C : PRINT A : A = B = 3 : PRINT A
-1
 0
> LET N = 2 ^ 100 / 3 : PRINT N
 1267650600228229401496703205376/3
```

## LINE

A statement: draws a line, a box, or a filled box. (`LINE INPUT`, which reads a line of text, is the next
entry.)

#### Syntax

```text
LINE [[STEP] (x1, y1)] - [STEP] (x2, y2) [, [c] [, {B | BF}]]
```

#### Arguments

*x1*, *y1*
: Numeric expressions: where the line starts. If the first point is left out, the line starts at the last point
  referenced: `LINE -(x, y)` carries on from where the last statement stopped.

*x2*, *y2*
: Numeric expressions: where it ends.

`STEP`
: Before either point: its coordinates are offsets from the last point referenced. For the second point, that is
  the first point.

*c*
: A numeric expression, 0 to 255: the colour. The pen's colour if it is left out.

`B`, `BF`
: `B` draws the box that has the two points as opposite corners, in outline. `BF` draws it filled.

#### Description

Needs a graphics mode. Both end points are drawn. The second point becomes the last point referenced, so a
chain of `LINE -(x, y)` statements draws a connected path.

A line that goes off the bitmap is drawn as far as the edge.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | No graphics mode is set; or *c* is not 0 to 255 |
| 13 | `type mismatch` | An argument is a string |

#### See also

`PSET`, `CIRCLE`, `DRAW`, `PAINT`, `COLOR`, `SCREEN`.

#### Example

```text
> SCREEN 13 : LINE (0, 0)-(10, 0), 3 : LINE -(10, 10), 4 : PRINT POINT(5, 0); POINT(10, 5)
 3  4
> LINE (30, 30)-(40, 40), 6, B : PRINT POINT(30, 35); POINT(35, 35)
 6  0
> LINE (50, 50)-(60, 60), 7, BF : PRINT POINT(55, 55)
 7
> SCREEN 0
```

## LINE INPUT

A statement: reads a whole line into a string variable.

#### Syntax

```text
LINE INPUT [;] ["prompt";] v$
LINE INPUT #f, v$
```

#### Arguments

*prompt*
: A string in quotes, written before the line is read. Nothing is added to it: no question mark.

*v$*
: A string variable, or an array's element.

*f*
: The number of a file opened `FOR INPUT` or `FOR BINARY`.

#### Description

One line is read and assigned to *v$* as it stands: commas, quotes and spaces at either end are all kept, where
`INPUT` would split the line at its commas and trim it. The line's end is not part of the string. A line
longer than 255 bytes is cut to 255.

From the console the line is typed with the console's line editing (section 13.2). From a file, the next line
of the file is read, and the file's position moves past its line end.

The semicolon after `INPUT` has the meaning it has for `INPUT`: it matters only when input is not a console.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 62 | `input past end of file` | No line is left: in file *f*, or in standard input when it isn't the console |
| 13 | `type mismatch` | The variable is numeric |
| 52 | `bad file number` | File *f* isn't open |
| 54 | `bad file mode` | File *f* is open `FOR OUTPUT` or `FOR APPEND` |

#### Notes

`LINE` with an opening parenthesis after it is the graphics statement that draws a line.

#### See also

`INPUT`, `INPUT$`, `EOF`, `OPEN`.

#### Example

```session
> Q$ = CHR$(34) : OPEN "/ram/t.txt" FOR OUTPUT AS #1 : PRINT #1, "  two, "; Q$; "words"; Q$; "  " : CLOSE
> OPEN "/ram/t.txt" FOR INPUT AS #1 : LINE INPUT #1, L$ : PRINT "["; L$; "]"; EOF(1)
[  two, "words"  ]-1
> LINE INPUT #1, L$
input past end of file
```

## LIST

A command of the prompt: shows the program's lines.

#### Syntax

```text
LIST [from][-[to]]
```

#### Arguments

*from*, *to*
: Line numbers or labels.

#### Description

| Typed | Lists |
|---|---|
| `LIST` | The whole program |
| `LIST 100` | Line 100 alone |
| `LIST 100-` | From line 100 to the end |
| `LIST -100` | From the start to line 100 |
| `LIST 100-200` | Lines 100 to 200 |

A number need not be a line that exists: the lines in the range are listed, and if there are none nothing is.
The lines are shown as they were typed or loaded, with their spacing and their case.

<kbd>CTRL</kbd>-<kbd>C</kbd> stops a long listing.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 89 | `only at the prompt` | `LIST` in a program's line |

#### See also

`EDIT`, `DELETE`, `SAVE`, `LOAD`.

#### Example

```session
> 10 PRINT "one"
> 20 PRINT "two"
> 30 PRINT "three"
> LIST 20-
20 PRINT "two"
30 PRINT "three"
> LIST 15-25
20 PRINT "two"
```

## LOAD

A command of the prompt: reads a program from a file, in place of the one in memory.

#### Syntax

```text
LOAD "file"
```

#### Arguments

*file*
: The file's name, in quotes. Nothing is added to it: a program saved as `game.bas` is loaded as `"game.bas"`.

#### Description

The program in memory is dropped first, with its variables and any files it had open, as `NEW` does; then the
file's lines become the program. The file is plain text, a line of the program to each line, with or without
line numbers. A carriage return before a line feed is dropped, so a file written on a PC loads as it is.

The program is not run. `RUN` runs it; `basic file`, typed at the shell, loads and runs a file in one step.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 288 | `not found` | There is no such file. The message is preceded by the file's name |
| 89 | `only at the prompt` | `LOAD` in a program's line |

#### Notes

The old program is dropped before the file is opened. If the `LOAD` then fails, there is no program in memory
at all: `SAVE` first.

#### See also

`SAVE`, `NEW`, `RUN`, `INCLUDE`, `EDIT`.

#### Example

```session
> 10 PRINT "saved"
> SAVE "/ram/s.bas"
> NEW
> LOAD "/ram/s.bas"
> LIST
10 PRINT "saved"
> LOAD "/ram/nothere.bas"
/ram/nothere.bas: not found
> LIST
```

## LOC, LOF

Numeric functions: an open file's position, and its length.

#### Syntax

```text
LOC(f)
LOF(f)
```

#### Arguments

*f*
: A numeric expression: the number of an open file, in any mode.

#### Result

`LOC(f)` is the number of bytes before the file's position: how many have been read or written so far, in a
file used from start to end. It is 0 when a file has just been opened, except `FOR APPEND`, where it is the
file's length.

`LOF(f)` is the file's length in bytes, as it is now, including what this program has written to it.

`SEEK(f)` is `LOC(f) + 1`: the position counted from 1, which is the form the `SEEK` and `GET` statements take.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 52 | `bad file number` | File *f* isn't open |

#### See also

`SEEK`, `EOF`, `OPEN`, `GET`, `PUT`.

#### Example

```session
> OPEN "/ram/l.txt" FOR OUTPUT AS #1 : PRINT #1, "abcdef" : PRINT LOC(1); LOF(1); SEEK(1) : CLOSE
 7  7  8
```

## LOCATE

A statement: moves the cursor, and shows or hides it.

#### Syntax

```text
LOCATE [row] [, [column] [, cursor]]
```

#### Arguments

*row*, *column*
: Numeric expressions, from 1: the top line of the screen is row 1 and its left edge column 1. Each is rounded
  to the nearest integer. One that is left out keeps its present value.

*cursor*
: A numeric expression: 0 hides the cursor and 1 shows it.

#### Description

The cursor is moved by writing an escape sequence to standard output (section 13.7), and the next thing printed
starts there. `POS(0)` follows the move.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *row* or *column* is less than 1, or more than 255 |
| 13 | `type mismatch` | An argument is a string |

#### Notes

The screen's real size isn't checked. A position beyond the edge of the terminal is the terminal's to deal
with; most stop at the edge.

#### See also

`CSRLIN`, `POS`, `CLS`, `PRINT`, `TAB`.

#### Example

The transcript shows what was printed, not where on the screen it went. `POS(0)` is read after `CSRLIN` has
been printed, so it is 17, not 14:

```session
> LOCATE 5, 10 : PRINT "here"; : PRINT CSRLIN; POS(0)
here 5  17
```

## LOG

A numeric function: the natural logarithm, of any number but 0.

#### Syntax

```text
LOG(x)
```

#### Arguments

*x*
: A numeric expression, complex too and not 0.

#### Result

The logarithm of *x* to the base e. `LOG(1)` is the integer 0. Any other result is a decimal, rounded to the
number of significant digits that `DIGITS` has set (12 unless it has been changed).

For a negative *x* the result is complex: the logarithm of the size of *x*, plus π i. `LOG(-1)` is π i.

For a complex *x*, the principal value: the logarithm of its size, plus i times its angle (from above −π to π).
`LOG(1i)` is π/2 i, and a part that is exactly 0 (the size of 0.6+0.8i is 1) is given as 0.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is 0 |
| 13 | `type mismatch` | *x* is a string |

#### Notes

A logarithm to another base is a quotient: `LOG(x) / LOG(10)` is the logarithm to base 10.

#### See also

`EXP`, `SQR`, `DIGITS`.

#### Example

```session
> PRINT LOG(1); LOG(10); LOG(1/2); LOG(2 ^ 100); LOG(-1)
 0  2.30258509299 -0.69314718056  69.314718056  3.14159265359i
> PRINT LOG(0)
illegal function call
> PRINT LOG(1i); LOG(1+1i)
 1.57079632679i  0.34657359028+0.785398163397i
```

## LOOP

The line that ends a `DO` loop, with or without a condition. See `DO`.

## LTRIM$, RTRIM$

String functions: a string without the spaces at its start, or at its end.

#### Syntax

```text
LTRIM$(s$)
RTRIM$(s$)
```

#### Arguments

*s$*
: A string expression.

#### Result

`LTRIM$` is *s$* from its first byte that is not a space; `RTRIM$` is *s$* up to its last byte that is not a
space. Only the space, byte 32, is removed: tabs and line ends are kept. A string that is all spaces gives the
empty string.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *s$* is a number |

#### Notes

`STR$` puts a space in front of a number that isn't negative. `LTRIM$(STR$(n))` is the number's digits alone.

#### See also

`STR$`, `LEFT$`, `RIGHT$`, `SPACE$`.

#### Example

```session
> PRINT "["; LTRIM$("  a b  "); "]["; RTRIM$("  a b  "); "]["; LTRIM$("   "); "]"; LTRIM$(STR$(42)); "|"
[a b  ][  a b][]42|
```
