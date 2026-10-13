# Chapter 1. HyBASIC Keywords

HyBASIC compiles a program whole before it runs it: every line is read, every name resolved and every block
matched, and only then does the first statement execute. An error the compiler finds (a `FOR` with no `NEXT`, a
label that isn't there) is therefore reported before anything has run, with the line it was found on. A line typed
at the prompt without a number is compiled and run the same way, as a program of one line.

An error is reported as its message, in small letters. `ERR` then holds its number, `ERR$` the message and `ERL`
the line; the numbers given in each entry are `ERR`'s. A failure reported by the system (a disk that is full, a
name that isn't there) has HyBASIC's own number where there is one that fits (53, file not found), and otherwise
256 plus the system's code (Appendix G).

A number is an integer of any length, a rational, a decimal or a complex number, and arithmetic on them is exact
(Chapter 18). A string is a sequence of bytes, each 0 to 255, up to 64K of them.

## ABS

A numeric function: the absolute value of a real number.

#### Syntax

```text
ABS(x)
```

#### Arguments

*x*
: A numeric expression whose value is real: an integer, a rational or a decimal.

#### Result

*x* if *x* is zero or more, and −*x* if it is less. The result is the same kind of number as *x*, and exact: an
integer gives an integer, of whatever length; a rational gives a rational in lowest terms; a decimal gives a
decimal with the same digits. There is no largest value and no overflow.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex. A complex number has no sign, and `ABS` does not give its modulus |
| 13 | `type mismatch` | *x* is a string |
| 2 | `syntax error` | No argument, or more than one |

#### Notes

For the modulus of a complex number *z*, use `SQR(REAL(z) ^ 2 + IMAG(z) ^ 2)`.

#### See also

`SGN`, `INT`, `FIX`, `REAL`, `IMAG`.

#### Example

```session
> PRINT ABS(-7); ABS(7); ABS(-7/2); ABS(-2.5)
 7  7  7/2  2.5
> PRINT ABS(-2 ^ 100)
 1267650600228229401496703205376
> PRINT ABS(3+4i)
illegal function call
> PRINT ERR; SQR(REAL(3+4i) ^ 2 + IMAG(3+4i) ^ 2)
 5  5
```

## ASC

A numeric function: the code of a string's first byte.

#### Syntax

```text
ASC(s$)
```

#### Arguments

*s$*
: A string expression of one byte or more.

#### Result

An integer from 0 to 255: the value of the first byte of *s$*. The rest of the string is not looked at. A string
holds bytes, not characters of any one set, so the result is the byte itself; which character the console draws
for it depends on the character set in use (Appendix E). The codes 32 to 126 are ASCII's.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *s$* is the empty string |
| 13 | `type mismatch` | The argument is a number |

#### Notes

`ASC` is the inverse of `CHR$` for one byte: `ASC(CHR$(n))` is *n* for every *n* from 0 to 255, including 0 (a
zero byte doesn't end a string). To test a string that may be empty, test `LEN(s$)` first.

#### See also

`CHR$`, `LEN`, `MID$`, `VAL`.

#### Example

```session
> PRINT ASC("A"); ASC("Abc"); ASC(" "); ASC(CHR$(0) + "x"); ASC(CHR$(255))
 65  65  32  0  255
> PRINT ASC("")
illegal function call
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

## OPEN

A statement: opens a file, and gives it a number for the statements that read and write it.

#### Syntax

```text
OPEN path$ FOR {INPUT | OUTPUT | APPEND | BINARY} AS [#]f [LEN = n]
```

#### Arguments

*path$*
: A string expression: the file's name. A name that doesn't start with `/` is taken from the current directory
  (`CHDIR`). It can be any name in the program's namespace: a file on a disk, or a device such as `/dev/cons`.

*f*
: A numeric expression from 1 to 255: the number the file is known by until it is closed. The `#` is optional.

*n*
: Accepted, evaluated and ignored. QuickBASIC's record length has no meaning here: `GET` and `PUT` take a
  position in bytes.

#### Description

The mode decides how the file is opened, and which statements may then use it:

| Mode | A file that exists | A file that doesn't | Starts at | May then be used by |
|---|---|---|---|---|
| `INPUT` | Opened for reading | Error 53 | Its first byte | `INPUT #`, `LINE INPUT #`, `INPUT$`, `EOF`, `LOF`, `LOC`, `SEEK` |
| `OUTPUT` | Emptied, and opened for writing | Made | Its first byte | `PRINT #`, `WRITE #`, `LOF`, `LOC`, `SEEK` |
| `APPEND` | Opened for writing, its contents kept | Made | Its end | `PRINT #`, `WRITE #`, `LOF`, `LOC`, `SEEK` |
| `BINARY` | Opened for reading and writing | Made | Its first byte | All of the above, and `GET` and `PUT` |

Up to 8 files can be open at once. A file stays open until `CLOSE` names it, `CLOSE` with no number closes them
all, or a `RUN` starts (which closes every file first). Leaving HyBASIC closes them too.

`EOF(f)` is always true (−1) for a file opened `FOR OUTPUT` or `FOR APPEND`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 52 | `bad file number` | *f* is less than 1 or more than 255 |
| 55 | `file already open` | Number *f* is in use |
| 67 | `too many files` | 8 files are open already |
| 53 | `file not found` | `FOR INPUT`: there's no such file. Any mode: a directory in the path isn't there |
| 70 | `permission denied` | The file, or its disk (`/rom`), can't be written |
| 13 | `type mismatch` | *path$* is a number, or *f* a string |

The system's other refusals come through as its own errors (a full disk is 61, `disk full`).

#### Notes

The limit of 8 is HyBASIC's own. Each open file also takes one of the task's 16 file descriptors, three of which
are the console's.

One file can be open under two numbers at once, each with its own position: opening it `FOR INPUT` as `#2` while
it is open `FOR OUTPUT` as `#1` is allowed, and reads what has been written so far.

A file written with `PRINT #` holds exactly the bytes `PRINT` would have sent to the screen, each line ended by a
line feed (byte 10).

#### See also

`CLOSE`, `PRINT`, `WRITE`, `INPUT`, `LINE INPUT`, `GET`, `PUT`, `SEEK`, `EOF`, `LOF`, `LOC`, `KILL`, `NAME`.

#### Example

```session
> OPEN "/ram/log.txt" FOR OUTPUT AS #1 : PRINT #1, "one" : PRINT LOF(1); EOF(1) : CLOSE #1
 4 -1
> OPEN "/ram/log.txt" FOR APPEND AS #1 : PRINT #1, "two" : PRINT LOF(1) : CLOSE #1
 8
> OPEN "/ram/log.txt" FOR INPUT AS #1 : LINE INPUT #1, L$ : PRINT L$; EOF(1)
one 0
> PRINT #1, "three"
bad file mode
> OPEN "/ram/other.txt" FOR OUTPUT AS #1
file already open
> CLOSE : OPEN "/ram/nothere.txt" FOR INPUT AS #1
file not found
> PRINT ERR
 53
```
