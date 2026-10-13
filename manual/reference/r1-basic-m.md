## MID$

A string function, and a statement: a part of a string, taken out or replaced.

#### Syntax

```text
MID$(s$, start [, length])
MID$(v$, start [, length]) = new$
```

#### Arguments

*s$*
: A string expression.

*v$*
: A string variable, or an array's element.

*start*
: A numeric expression, 1 or more: the position of the first byte, counting the string's first byte as 1.

*length*
: A numeric expression, 0 or more: how many bytes. If it is left out, all the rest of the string.

*new$*
: A string expression.

Numbers that aren't integers are rounded to the nearest, a half going to the even one.

#### Result

**The function** is the *length* bytes of *s$* that start at *start*, or as many as there are if the string
ends sooner. It is the empty string if *start* is past the end, or *length* is 0.

#### Description

**The statement** overwrites part of the variable's string with the bytes of *new$*, starting at *start*. It
never changes the string's length. The number of bytes replaced is the smallest of three: *length* (if it is
given), the length of *new$*, and what is left of *v$* from *start* to its end. Bytes of *new$* that don't fit
are not used.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *start* is less than 1, or *length* is negative; in the statement, *start* is past the end of *v$* |
| 13 | `type mismatch` | A number where a string is wanted, or a string where a number is |

#### See also

`LEFT$`, `RIGHT$`, `INSTR`, `LEN`.

#### Example

```session
> PRINT "["; MID$("hello", 2, 3); "]["; MID$("hello", 4); "]["; MID$("hello", 4, 9); "]["; MID$("hello", 9); "]"
[ell][lo][lo][]
> A$ = "hello" : MID$(A$, 2, 2) = "XYZ" : PRINT A$ : MID$(A$, 4) = "12345" : PRINT A$; LEN(A$)
hXYlo
hXY12 5
```

## MKDIR, RMDIR

Statements: make a directory, and remove one.

#### Syntax

```text
MKDIR path$
RMDIR path$
```

#### Arguments

*path$*
: A string expression: the directory's name.

#### Description

`MKDIR` makes a new, empty directory. The directory it is to go in must exist already: one level is made at a
time.

`RMDIR` removes a directory, which must be empty.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 58 | `file already exists` | `MKDIR`: there is a file or a directory of that name |
| 53 | `file not found` | `MKDIR`: the directory it would go in isn't there. `RMDIR`: there is no such directory |
| 292 | `directory not empty` | `RMDIR`: the directory still has something in it |
| 70 | `permission denied` | The disk can't be written |

#### See also

`CHDIR`, `FILES`, `KILL`, `NAME`.

#### Example

```session
> MKDIR "/ram/d1" : MKDIR "/ram/d1/sub" : RMDIR "/ram/d1"
directory not empty
> RMDIR "/ram/d1/sub" : RMDIR "/ram/d1" : RMDIR "/ram/d1"
file not found
```

## MKN$

A string function: a number's bytes in the stored format.

#### Syntax

```text
MKN$(x)
```

#### Arguments

*x*
: A numeric expression of any kind.

#### Result

A string holding *x* in the form numbers are stored in: the form `PUT` writes for a numeric variable, and `CVN`
reads back. Its length depends on the number. A small integer takes one byte; larger integers, rationals,
decimals and complex numbers take as many as they need.

#### Description

`MKN$` and `CVN` are the pair that stands in place of QuickBASIC's `MKI$`, `MKL$`, `MKS$`, `MKD$` and their
`CV` functions. `CVN(MKN$(x))` is exactly *x*, for every number.

Because the length varies, a number can't be given a fixed place in a record by `MKN$` alone. Pad the string to
the width wanted, or keep the number as text.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *x* is a string |

#### See also

`CVN`, `PUT`, `GET`, `STR$`.

#### Example

```session
> PRINT LEN(MKN$(7)); LEN(MKN$(1/3)); LEN(MKN$(2 ^ 100)); CVN(MKN$(2 ^ 100))
 1  3  14  1267650600228229401496703205376
```

## MOD

An operator: the remainder of an integer division. The entry covers `\`, integer division, too.

#### Syntax

```text
a MOD b
a \ b
```

#### Arguments

*a*, *b*
: Numeric expressions whose values are real.

#### Result

Both operands are first rounded to the nearest integer, a half going to the even one. Then `a \ b` is the
quotient with its fraction dropped, which rounds toward zero, and `a MOD b` is what is left over: `a - (a \ b)
* b`. The remainder has the sign of *a*, or is 0.

The integers can be of any length.

#### Description

`\` binds more tightly than `MOD`, and both less tightly than `*` and `/` and more tightly than `+` and `-`. So
`A + B MOD C \ D` is `A + (B MOD (C \ D))`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 11 | `division by zero` | *b* is 0, or rounds to 0 |
| 13 | `type mismatch` | An operand is a string |

#### Notes

Because the operands are rounded first, `7.5 MOD 2` is 0: 7.5 rounds to 8. For the remainder of a division of
numbers that aren't whole, work it out: `x - INT(x / y) * y`.

`/` is exact division, and gives a rational when the integers don't divide: `7 / 2` is 7/2.

#### See also

`INT`, `FIX`, `GCD`.

#### Example

```session
> PRINT 7 MOD 3; -7 MOD 3; 7 MOD -3; 7 \ 2; -7 \ 2; 7.5 MOD 2; 7 / 2
 1 -1  1  3 -3  0  7/2
> PRINT 2 ^ 70 MOD 1000; 2 + 3 * 4 ^ 2 MOD 5 \ 2
 424  2
```

## NAME

A statement: renames a file.

#### Syntax

```text
NAME old$ AS new$
```

#### Arguments

*old$*
: A string expression: the file's present name.

*new$*
: A string expression: its new name.

#### Description

The file, or directory, *old$* is given the name *new$* in the directory it is already in. Only the last part
of *new$* is used, the name after its last `/`: a rename can't move a file to another directory, and a
directory written in *new$* is ignored without an error. To move a file, copy it and remove the original.

Renaming a file to the name it has does nothing.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 53 | `file not found` | There is no *old$* |
| 58 | `file already exists` | There is a *new$* in that directory already |
| 70 | `permission denied` | The disk can't be written |

#### See also

`KILL`, `FILES`, `MKDIR`, `SHELL`.

#### Example

```session
> OPEN "/ram/n1" FOR OUTPUT AS #1 : CLOSE : MKDIR "/ram/dd"
> NAME "/ram/n1" AS "/ram/dd/moved" : FILES "/ram" : FILES "/ram/dd"
bin/            lib/            moved           dd/
```

## NEW

A command of the prompt: drops the program.

#### Syntax

```text
NEW
```

#### Description

The program's lines are removed, with everything that belonged to it: its variables and arrays, its procedures
and labels, and any files it had open, which are closed. A stopped program is gone, so there is nothing to
`CONT`. The settings that are HyBASIC's and not the program's stay as they are: `BASE`, `DIGITS`, the current
directory.

There is no question first, and no way back. `SAVE` what is wanted.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 89 | `only at the prompt` | `NEW` in a program's line |

#### See also

`LOAD`, `SAVE`, `CLEAR`, `DELETE`.

## NEXT

The line that ends a `FOR` loop. See `FOR`.

## NOT

An operator: the bitwise complement of an integer, which is also the logical NOT of a truth value.

#### Syntax

```text
NOT a
```

#### Result

An integer: *a*, rounded to the nearest integer if it isn't one, with every bit turned over. In two's complement
that is −*a* − 1: `NOT 0` is −1, `NOT -1` is 0, and `NOT 5` is −6.

For the truth values −1 and 0, `NOT` gives the other one. For any other number it does not: 5 counts as true
in an `IF`, and so does `NOT 5`.

#### Description

`NOT` binds less tightly than the comparisons and more tightly than `AND`: `NOT A = B` is `NOT (A = B)`, and
`NOT A AND B` is `(NOT A) AND B`.

The operand is rounded and treated as `AND`'s are: see `AND`.

#### Example

```session
> PRINT NOT 0; NOT -1; NOT 5; NOT 1 = 2
-1  0 -6 -1
```

## NUMERATOR

A numeric function: the numerator of a real number written as a fraction in lowest terms.

#### Syntax

```text
NUMERATOR(x)
```

#### Result

An integer with the sign of *x*. A rational gives its numerator, an integer gives itself, and a decimal is
turned into the fraction it exactly equals first: −0.75 is −3/4, and gives −3. See `DENOMINATOR`, which has the
errors and an example.

## OCT$

A string function: a number written in octal.

#### Syntax

```text
OCT$(x)
```

#### Result

The text of *x* in base 8, with no space in front, as `STR$(x, "o")` would give it. As with `HEX$`, it is the
number itself in another base: a negative number has a minus sign, and a number that isn't an integer is written
as what it is. See `HEX$`.

#### Example

```session
> PRINT OCT$(8); " "; OCT$(255); " "; OCT$(-8); " "; OCT$(0.5)
10 377 -10 0.4
```

## ON

A statement: jumps to one of several lines by a number; or sets where errors go.

#### Syntax

```text
ON n GOTO line [, line] ...
ON n GOSUB line [, line] ...
ON ERROR GOTO line
ON ERROR GOTO 0
```

#### Arguments

*n*
: A numeric expression, rounded to the nearest integer.

*line*
: A line number or a label.

#### Description

**`ON n GOTO`** goes to the first line in the list if *n* is 1, to the second if it is 2, and so on. If *n* is
less than 1 or more than the number of lines listed, nothing happens: the program goes on with the next
statement. **`ON n GOSUB`** is the same, but calls the line as a subroutine, and its `RETURN` comes back to the
statement after the `ON`.

**`ON ERROR GOTO line`** names an error handler. From then on, an error that would have stopped the program
makes it go to *line* instead, with `ERR`, `ERL` and `ERR$` set. The handler ends with a `RESUME`, which says
where to carry on: at the statement that failed, at the one after it, or at another line.

While the handler is running, a second error is not sent to it again. It stops the program in the ordinary
way.

**`ON ERROR GOTO 0`** turns the handler off. Used inside a handler, it gives up: the error being handled stops
the program, with its message, as if there had been no handler.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 8 | `label not defined` | A line in the list, or the handler's, isn't in the program |
| 13 | `type mismatch` | *n* is a string |

#### Notes

<kbd>CTRL</kbd>-<kbd>C</kbd> is not an error, and no handler sees it.

#### See also

`RESUME`, `ERR`, `ERROR`, `GOTO`, `GOSUB`, `SELECT`.

#### Example

```session
> 10 ON ERROR GOTO 100
> 20 X = 1 / 0 : PRINT "after the error"
> 30 OPEN "/ram/nothere" FOR INPUT AS #1
> 40 PRINT "line 40" : ON ERROR GOTO 0 : ERROR 5
> 50 PRINT "not reached"
> 100 PRINT "handler:"; ERR; ERL
> 110 IF ERR = 11 THEN RESUME NEXT
> 120 RESUME 40
> RUN
handler: 11  20
after the error
handler: 53  30
line 40
line 40: illegal function call
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

## OPTION

A statement: sets the lowest index of arrays.

#### Syntax

```text
OPTION BASE {0 | 1}
```

#### Description

An array made with only its highest index given, as in `DIM A(10)`, has indexes from 0. After `OPTION BASE 1`
they are from 1, so that `DIM A(10)` has ten elements and no `A(0)`. `OPTION BASE 0` puts it back.

The setting applies to arrays made after the statement, whether by `DIM`, by `REDIM`, or by being used without
either (whose indexes then run from 1 to 10). Arrays that exist already keep the bounds they were made with. A
`DIM` that states its own lowest index, `DIM A(5 TO 9)`, is not affected.

The digit must be written out as 0 or 1: it is part of the statement, not an expression.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 2 | `syntax error` | Anything but 0 or 1 after `BASE` |

#### Notes

This `BASE` has nothing to do with the `BASE` statement, which sets the base numbers are written in.

#### See also

`DIM`, `REDIM`, `LBOUND`, `UBOUND`.

#### Example

```session
> OPTION BASE 1 : DIM A(3) : B(1) = 1 : PRINT LBOUND(A); UBOUND(A); LBOUND(B); UBOUND(B)
 1  3  1  10
> OPTION BASE 0 : DIM C(3) : PRINT LBOUND(C); LBOUND(A)
 0  1
```

## OR

An operator: the bitwise OR of two integers, which is also the logical OR of two truth values.

#### Syntax

```text
a OR b
```

#### Result

An integer. Each bit of it is 1 where the same bit is 1 in *a*, in *b*, or in both.

For two truth values, −1 and 0, the result is true if either is.

Both operands are always evaluated, and they are rounded and treated as `AND`'s are: see `AND`. `OR` binds less
tightly than `AND` and more tightly than `XOR`.

#### Example

```session
> PRINT 5 OR 3; 0 OR 0; (1 = 2) OR (2 = 2); 1.5 OR 0
 7  0 -1  2
```

## OUTPUT

A mode of `OPEN`: `OPEN path$ FOR OUTPUT AS #f` makes a file, or empties one, and opens it for writing. See
`OPEN`.

## PEEK, POKE

A function and a statement: read, and write, one byte of memory.

#### Syntax

```text
PEEK(address)
POKE address, value
```

#### Arguments

*address*
: A numeric expression: an address in HyBASIC's task, 0 to 65,535. A negative number from −32,768 to −1 is
  accepted too, and means the address 65,536 more: −1 is $FFFF.

*value*
: A numeric expression from 0 to 255, rounded to the nearest integer.

#### Result

`PEEK` is the byte at the address, 0 to 255.

#### Description

The addresses are those of HyBASIC's own task: its zero page and stack, the RAM that holds its state, the bank
window at $8000 to $9FFF, the paged ROM at $A000 to $DFFF and the BIOS above (Appendix F). Another task's
memory can't be reached.

For an address in the bank window, the bank is the one `BANK` has set. HyBASIC puts it there for the read or the
write and takes it away again, so the window is of no use to a program between the two: see `BANK`.

`POKE` does no checking beyond the two ranges. Writing into HyBASIC's own memory, which is most of what is
below $8000, will usually stop it working; and a write to an address that is ROM does nothing. The hardware's
registers are reached through the drivers' files (Chapter 12), not through `POKE`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | The address is outside −32,768 to 65,535; or the value is outside 0 to 255 |
| 13 | `type mismatch` | An argument is a string |

#### See also

`BANK`, `SYS`, `CALL`, `ASM`.

#### Example

```session
> BANK 40 : POKE &H8000, 65 : POKE &H8001, 66.5 : PRINT PEEK(&H8000); PEEK(&H8001); PEEK(-32768)
 65  66  65
> POKE &H8000, 256
illegal function call
```

## PI

A numeric function: π.

#### Syntax

```text
PI
```

#### Result

The ratio of a circle's circumference to its diameter, as a decimal rounded to the number of significant
digits that `DIGITS` has set: 3.14159265359 at the usual 12.

It takes no argument and no parentheses.

#### See also

`DIGITS`, `SIN`, `COS`, `TAN`, `ATN`.

#### Example

```session
> PRINT PI : DIGITS 30 : PRINT PI : DIGITS 12
 3.14159265359
 3.14159265358979323846264338328
```

## PLAY

A statement: plays music, and waits until it has finished.

#### Syntax

```text
PLAY [channel,] score$
PLAY file$
```

#### Arguments

*score$*
: A string expression: a line of the score language, MML (section 15.6).

*channel*
: A numeric expression: the sound channel to play it on (section 15.1). Channel 0 if it is left out.

*file$*
: A string expression ending in `.zsm` or `.mml`: the name of a song file, or of a file of score lines.

#### Description

HyBASIC does not make the sound itself. It runs the `play` program (section 15.7) with the string, and waits
for it to end: `play -m channel score` for a line of score, and `play file` for a file. Whatever `play` can
play, `PLAY` can. The program stops while the music lasts.

A string is taken as a file's name if it ends in `.zsm` or `.mml`, and as a score otherwise.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 511 | `play`'s own message | `play` could not play it: a mistake in the score, a file that isn't there. `play` prints its message itself first, with its name before it |
| 5 | `illegal function call` | A channel number that is not one |
| 13 | `type mismatch` | The score is a number |

#### Notes

This is not QuickBASIC's `PLAY` language, though simple tunes look alike in both: the letters `c` to `b` are
notes, `o` sets the octave and `l` the length. Section 15.6 has the whole language.

For a single tone, `SOUND` is simpler.

#### See also

`SOUND`, `BEEP`, `SHELL`.

#### Example

```session
> PLAY "t240 l16 c e g" : PRINT "played"
played
> PLAY "zzz!"
play: zzz!: channel 0: what is z
channel 0: what is z
> PRINT ERR
 511
```

## POS

A numeric function: the column the next character will be printed in.

#### Syntax

```text
POS(x)
POS
```

#### Arguments

*x*
: Any numeric expression. It is ignored; `POS(0)` is the usual form, from QuickBASIC, and `POS` alone means the
  same.

#### Result

The column, counted from 1 at the left edge. HyBASIC keeps count of what it has written since the last line
end, so this is the column of its own output: after `PRINT "ab";` it is 3.

#### Notes

The count is HyBASIC's, not the terminal's. It follows `LOCATE`, but it knows nothing of what other programs
have written, or of escape sequences a program prints for itself. The row is `CSRLIN`, which does ask the
terminal.

`TAB` and the print zones of a comma work from the same count.

#### See also

`CSRLIN`, `LOCATE`, `TAB`, `SPC`, `WIDTH`.

#### Example

```session
> PRINT POS(0); : PRINT "ab"; : PRINT POS(0)
 1 ab 6
```

## PRESERVE

Part of `REDIM`: `REDIM PRESERVE` changes an array's size and keeps what is in it. See `REDIM`.

## PRINT

A statement: writes values as text, to the screen or to a file.

#### Syntax

```text
PRINT [#f,] [item [{; | ,} item] ...] [; | ,]
PRINT [#f,] USING format$; item [{; | ,} item] ... [; | ,]
? ...
```

#### Arguments

*item*
: An expression of any type, or `TAB(n)` or `SPC(n)`.

*f*
: The number of a file opened `FOR OUTPUT`, `FOR APPEND` or `FOR BINARY`. The output goes to the file in place
  of the screen, byte for byte the same.

*format$*
: A string expression: a picture of how the items are to be written (below).

`?` is another spelling of `PRINT`.

#### Description

**Values.** A string is written as its bytes. A number is written as its text in the base that `BASE` has set,
with a space before it if it isn't negative (a negative number has its minus sign there instead) and a space
after it. The text is the number's exact value: an integer has all its digits however many there are, a
rational is written with a `/`, and a decimal with its point and every digit it holds. There is no
exponent form: a very large or very small number is written out in full.

**Between items.** A semicolon writes nothing: the next item follows at once. A comma moves to the next print
zone: the zones are 14 columns wide, starting at columns 1, 15, 29 and so on, and if there is no whole zone
left on the line the item starts a new line. Two commas skip a zone.

**At the end.** `PRINT` ends the line, by writing a line feed, unless the statement ends with a semicolon or a
comma: then the next `PRINT` carries on from there. `PRINT` with no items writes an empty line.

**`TAB(n)`** moves to column *n*, counting from 1, by writing spaces. If the output is already past column
*n*, it starts a new line and moves to column *n* there. **`SPC(n)`** writes *n* spaces. Both are used only
inside `PRINT`.

**The line's width** is 80 unless `WIDTH` has set another. It decides where the zones run out. Text longer
than the width is not broken by HyBASIC; the terminal wraps it.

#### PRINT USING

The format is text with **fields** in it. Each item is written by the next field; the text between fields is
written as it stands. If there are more items than fields, the format is used again from its start.

A number's field is a picture of the number:

| In the field | Means |
|---|---|
| `#` | A digit's place. The number is put at the right of its places, with spaces before it |
| `.` | The decimal point. The `#`s after it are decimal places, always filled, with zeros if need be |
| `,` | Before the point: put commas between each three digits |
| `+` | At the start: write the sign, `+` or `-`, always |
| `$$` | At the start: write a `$` just before the number |
| `**` | At the start: fill the spaces before the number with `*` |

The number is **rounded** to the field's decimal places, a half going away from zero: 2.5 in `###` is 3, and
−3.145 in `###.##` is −3.15. (This is not `CINT`'s rule, which sends a half to the even integer.) A minus sign
takes a place. If the number needs more places than the field has, it is written in full with a `%` before it
to show that the column is broken.

A string's field is one of three:

| In the field | Means |
|---|---|
| `!` | The first character of the string |
| `&` | The whole string |
| `\`, spaces, `\` | As many characters as the field is wide, counting both backslashes; a shorter string is padded with spaces |

Two more things can be in a format:

| In the format | Means |
|---|---|
| `{}` | The item as `PRINT` would write it, a number or a string, with no spaces added |
| `{`*base*`}` | A number in that base: `{x}` in hex, `{b}` in binary (a base string, section 18.4) |
| `_` | The next character is text, not part of a field: `_#` writes a `#` |

A semicolon or a comma between the items of `PRINT USING` makes no difference: the format decides the spacing.
At the end of the statement they hold the line open, as in `PRINT`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | `USING`: a string for a number's field, or a number for a string's |
| 5 | `illegal function call` | `USING`: the format has no field in it |
| 52 | `bad file number` | File *f* isn't open |
| 54 | `bad file mode` | File *f* is open `FOR INPUT` |

#### Notes

`PRINT #` writes to a file exactly what `PRINT` would have put on the screen, zones and spaces included. For
data that a program will read back with `INPUT #`, `WRITE #` is the better choice: it puts commas between the
items and quotes round the strings.

To print a number without its spaces, use `PRINT USING "{}"`, or `LTRIM$(STR$(n))`.

#### See also

`WRITE`, `WIDTH`, `LOCATE`, `POS`, `STR$`, `BASE`, `FIXED`, `OPEN`.

#### Example

```session
> PRINT 1; -2; 3 : PRINT "a"; "b"; 5; "c" : PRINT 1, 2, "x", -4
 1 -2  3
ab 5 c
 1             2            x             -4
> PRINT 1/3; 2.50; 2 ^ 64; 1+2i : PRINT "one"; : PRINT " line"
 1/3  2.5  18446744073709551616  1+2i
one line
> PRINT "A"; TAB(10); "B"; TAB(5); "C"; SPC(3); "D"
A        B
    C   D
> PRINT USING "###.##"; 3.14159 : PRINT USING "###.##|"; -3.145; 1234.5
  3.14
 -3.15|%1234.50|
> PRINT USING "#,###.# +##.# $$###.## **###.##"; 1234.56; 5; 12.5; 12.5
1,234.6  +5.0   $12.50 ***12.50
> PRINT USING "[!][&][\  \] {} {x} _#"; "hello"; "world"; "abcdefg"; 255; 255
[h][world][abcd] 255 FF #
```

## PUT

A statement: writes a value to a `BINARY` file, at a given position.

#### Syntax

```text
PUT [#]f, [position], value
```

#### Arguments

*f*
: A numeric expression: the number of a file opened `FOR BINARY`.

*position*
: A numeric expression: where to write, counted in bytes from 1 for the file's first byte. If it is left out
  (the two commas are still written) or is less than 1, the write starts where the last `GET` or `PUT` on the
  file ended.

*value*
: An expression of either type. A variable is the usual thing, but any expression will do.

#### Description

A string is written as its bytes, as many as it has, with nothing before or after them. A number is written in
the stored format, the form `MKN$` makes, which is as long as the number needs.

The bytes replace whatever was at that position. Writing past the end makes the file longer; writing at a
position beyond the end leaves the gap between filled with zero bytes.

After the write, the file's position is just past what was written.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 52 | `bad file number` | File *f* isn't open |
| 54 | `bad file mode` | File *f* was not opened `FOR BINARY` |
| 61 | `disk full` | No room left on the disk |

#### Notes

To write text to a file opened `FOR BINARY`, `PRINT #` works too, and adds its line end.

Since a number's stored form varies in length, a file of numbers written with `PUT` is read back in the same
order with `GET`; a number can't be changed in place for a longer one without overwriting what follows it.

#### See also

`GET`, `OPEN`, `SEEK`, `MKN$`, `CVN`, `PRINT`.

#### Example

```session
> OPEN "/ram/p.dat" FOR BINARY AS #1 : PUT #1, 1, "HELLO" : PUT #1, 3, "xy" : PUT #1, 9, "!" : PRINT LOF(1)
 9
> A$ = SPACE$(9) : GET #1, 1, A$ : PRINT A$; ASC(MID$(A$, 6)) : CLOSE
HExyO! 0
```
