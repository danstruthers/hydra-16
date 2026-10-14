## ABS

A numeric function: the absolute value of a number, its size.

#### Syntax

```text
ABS(x)
```

#### Arguments

*x*
: A numeric expression: an integer, a rational, a decimal or a complex number.

#### Result

*x* if *x* is zero or more, and −*x* if it is less. The result is the same kind of number as *x*, and exact: an
integer gives an integer, of whatever length; a rational gives a rational in lowest terms; a decimal gives a
decimal with the same digits. There is no largest value and no overflow.

For a complex number, its modulus: its distance from 0, the square root of its real part squared plus its
imaginary part squared. `ABS(3+4i)` is 5. It is exact when that root is, and otherwise a decimal of the digits
`DIGITS` has set.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *x* is a string |
| 2 | `syntax error` | No argument, or more than one |

#### See also

`SGN`, `INT`, `FIX`, `REAL`, `IMAG`.

#### Example

```session
> PRINT ABS(-7); ABS(7); ABS(-7/2); ABS(-2.5)
 7  7  7/2  2.5
> PRINT ABS(-2 ^ 100)
 1267650600228229401496703205376
> PRINT ABS(3+4i); ABS(1+1i)
 5  1.41421356237
```

## ABSOLUTE

Part of `CALL ABSOLUTE`, which calls machine code at an address. See `CALL`.

## AND

An operator: the bitwise AND of two integers, which is also the logical AND of two truth values.

#### Syntax

```text
a AND b
```

#### Arguments

*a*, *b*
: Numeric expressions whose values are real.

#### Result

An integer. Each bit of it is 1 where the same bit is 1 in both *a* and *b*, and 0 elsewhere.

#### Description

Both operands are evaluated, always: there is no short cut when the first is 0. Each is then rounded to the
nearest integer, a half going to the even one (1.5 and 2.5 both become 2), and the two integers are combined bit
by bit.

The integers can be of any length. A negative one is taken in two's complement with as many sign bits as are
needed, so −1 is all ones, and `-1 AND x` is *x* for every integer *x*.

A comparison gives −1 for true and 0 for false. Since −1 is all ones and 0 is none, `AND` of two comparisons is
their logical AND, and `IF a > 0 AND b > 0 THEN` means what it says. `AND` of a truth value and some other
number is a bit operation still: `(1 = 1) AND 2` is 2, which counts as true, but `1 AND 2` is 0, which is false,
though both of its operands count as true.

`AND` binds more tightly than `OR`, `XOR`, `EQV` and `IMP`, and less tightly than `NOT` and the comparisons:
`NOT a AND b = c OR d` is `((NOT a) AND (b = c)) OR d`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | An operand is a string |
| 5 | `illegal function call` | An operand is complex |

#### See also

`OR`, `XOR`, `NOT`, `EQV`, `IMP`, `BIT`, `SHL`.

#### Example

```session
> PRINT 12 AND 10; -1 AND 255; (1 < 2) AND (3 < 2); 1 AND 2
 8  255  0  0
> PRINT 2.5 AND 3; 1.5 AND 3; 2 ^ 40 + 5 AND 2 ^ 40 + 3
 2  2  1099511627777
> PRINT 0 AND (1 / 0)
division by zero
```

## APPEND

A mode of `OPEN`: `OPEN path$ FOR APPEND AS #f` opens a file for writing at its end. See `OPEN`.

## ARG$

A string function: one of the arguments the program was started with.

#### Syntax

```text
ARG$(n)
```

#### Arguments

*n*
: A numeric expression: which argument. 0 is the program's own name, 1 the first argument after it.

#### Result

The argument, as a string. The arguments are those of the command that started HyBASIC with a program to run:
after `basic prog.bas one two`, `ARG$(0)` is `prog.bas`, `ARG$(1)` is `one` and `ARG$(2)` is `two`. An argument
is passed whole, as the shell made it: one that was quoted there can hold spaces.

The empty string if there is no argument *n*: past the last one, for a negative *n*, and for every *n* when
HyBASIC was started without a program (at its prompt).

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *n* is a string |

#### Notes

There is no count of the arguments. Read them until one is empty, or split `COMMAND$`.

#### See also

`COMMAND$`, `ENVIRON$`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo 'PRINT "["; ARG$(0); "]["; ARG$(1); "]["; ARG$(2); "]["; ARG$(3); "]"' >args.bas
% basic args.bas one 'two words'
[args.bas][one][two words][]
```

## AS

Part of four statements, where it introduces a name, a number or a type: `DIM v AS type`, `OPEN path$ FOR mode AS
#f`, `NAME old$ AS new$`, and a field's line in `TYPE`. See those.

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

## ASM ... END ASM

A block: 65C02 assembly language inside a program, assembled when the program is compiled.

#### Syntax

```text
ASM
    assembly language lines
END ASM

CALL ASM label [, a [, x [, y]]]
```

#### Arguments

*label*
: A label defined in one of the program's `ASM` blocks, spelled as it is there. The assembler's names keep
  their case: `Add1` and `add1` are two labels.

*a*, *x*, *y*
: Numeric expressions: the values the A, X and Y registers hold as the routine is entered, a byte each. Each is
  0 if it is left out.

#### Description

`ASM` alone on a line starts a block and `END ASM` alone on a line ends it. The lines between are not HyBASIC:
they are in the language of the `as` assembler (Chapter 27), with its labels, directives and expressions. A
program can have several blocks; they are assembled together, as one source, in the order they appear, so a
label in one can be used in another.

The blocks are assembled when the program is compiled, before it runs. Their code goes into a RAM bank of its
own, 8K long, which is at $8000 to $9FFF while the code runs. Statements don't run "through" a block: when
control reaches `ASM`, it goes on after `END ASM`.

`CALL ASM label` calls the code at *label* as a subroutine (`jsr`), which returns with `rts`. `RREG` then gives
the registers it returned with.

The assembler is given some of the program's names as symbols. A global numeric variable's name is the address
of its value: five bytes, the first its kind (0 for an integer) and the next four an integer's value, low byte
first. An integer `CONST`'s name is its value. Each is defined in capitals and in small letters. The names `A`,
`X` and `Y` are left out, since they are the registers'.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 90 | `ASM: ` and the assembler's message | An error in a block, reported at its line when the program is compiled |
| 90 | `ASM: `*label*`: no such label in an ASM block` | `CALL ASM` names a label that no block defines |

#### Notes

The code's bank is at $8000 only while the code runs, so the code can't use `BANK`'s bank at the same time, and a
`PEEK` of $8000 from HyBASIC doesn't see the code.

A program can name at most 64 different labels in its `CALL ASM` statements.

The code runs as part of HyBASIC's task, with the whole machine open to it. A routine that doesn't return, or
that writes where it shouldn't, takes HyBASIC down with it.

#### See also

`CALL`, `SYS`, `RREG`, `PEEK`, `POKE`, `BANK`.

#### Example

```session
> 10 CALL ASM seven : RREG A : PRINT A
> 20 CALL ASM add1, 41 : RREG A : PRINT A
> 30 END
> 40 ASM
> 50 seven: lda #7
> 60        rts
> 70 add1:  inc a
> 80        rts
> 90 END ASM
> RUN
 7
 42
```

## ATN

A numeric function: the arctangent.

#### Syntax

```text
ATN(x)
```

#### Arguments

*x*
: A numeric expression, complex too.

#### Result

The angle, in radians, whose tangent is *x*: more than −π/2 and less than π/2. `ATN(0)` is the integer 0. Any
other result is a decimal, correct to the number of significant digits that `DIGITS` has set (12 unless it has
been changed), whatever kind of number *x* is.

For a complex *x*, the principal value, as mathematics defines it (Chapter 18 of the manual): each part correctly rounded, and `ATN(-x)` always `-ATN(x)`. `ATN(2i)` is π/2 + 0.549...i.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is i or −i, where the arctangent is infinite |
| 13 | `type mismatch` | *x* is a string |

#### Notes

`4 * ATN(1)` is the usual way to get π in a BASIC, but here it multiplies the rounding of `ATN(1)` by four. `PI`
gives π to the digits set, directly.

#### See also

`TAN`, `SIN`, `COS`, `PI`, `DIGITS`.

#### Example

```session
> PRINT ATN(1); ATN(0); ATN(-1); ATN(1/2)
 0.785398163397  0 -0.785398163397  0.463647609001
> DIGITS 30 : PRINT ATN(1) : DIGITS 12
 0.78539816339744830961566084582
> PRINT ATN(2i); ATN(1+1i)
 1.57079632679+0.549306144334i  1.0172219679+0.402359478109i
```

## BANK

A statement and a function: chooses, or reports, the RAM bank that `PEEK` and `POKE` see at $8000.

#### Syntax

```text
BANK n
BANK()
```

#### Arguments

*n*
: A numeric expression from 0 to 255: a RAM bank's number.

#### Result

`BANK()` is the number last set by `BANK`, or 0 if none has been.

#### Description

The addresses $8000 to $9FFF are a window, 8K wide, onto one RAM bank at a time (Appendix F). `BANK n` sets the
bank that a `PEEK` or `POKE` of an address in that range will use. It changes nothing else: the bank is put in
the window for the moment of each `PEEK` and `POKE` and then taken out again, because HyBASIC keeps its own data
in banks and uses the window itself.

HyBASIC takes RAM banks for its own use as it needs them, for the program's text, its strings and its arrays,
and `BANK` doesn't check for them: a `POKE` into a bank HyBASIC is using damages it. A program that wants a
bank to itself should take one high in the task's range, which HyBASIC reaches last if at all.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is less than 0 or more than 255 |
| 13 | `type mismatch` | *n* is a string |

#### Notes

*n* is not checked against the banks the task has. A task with three RAM modules has banks 0 to 47.

`BANK` has no effect on the bank an `ASM` block's code runs in.

#### See also

`PEEK`, `POKE`, `SYS`, `ASM`.

#### Example

```session
> PRINT BANK() : BANK 40 : POKE &H8000, 65 : PRINT PEEK(&H8000); BANK()
 0
 65  40
> BANK 41 : POKE &H8000, 66 : BANK 40 : PRINT PEEK(&H8000)
 65
```

## BASE

A statement: sets the base that numbers are written and read in.

#### Syntax

```text
BASE b$
```

#### Arguments

*b$*
: A string expression: a base string (sections 18.4 to 18.6). `"d"` is decimal, `"x"` hexadecimal, `"b"`
  binary, `"o"` octal; `"7r"` is base 7.

#### Description

From the next statement on, every number that HyBASIC converts to text is written in the base, and every number
it converts from text is read in it. That covers `PRINT`, `WRITE`, `STR$` and `PRINT #`; `INPUT`, `READ` and
`VAL`; and the numbers written in the program's own lines that are compiled after the statement has run, which
at the prompt means the lines typed after it.

A number with a prefix says its own base whatever the setting: `&HFF` and `&B1010` are 255 and 10 in every
base.

The setting stays until another `BASE`, and is `"d"` when HyBASIC starts.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *b$* isn't a base string |
| 13 | `type mismatch` | *b$* is a number |

#### Notes

`OPTION BASE`, which sets the lowest index of arrays, is a different statement: see `OPTION`.

To write one number in another base without changing the setting, use `HEX$`, `OCT$`, or `STR$` with a base
string.

#### See also

`BASE$`, `DIGITS`, `HEX$`, `OCT$`, `STR$`, `VAL`.

#### Example

On the second line the 10 and the 6 are read in hex, as sixteen and six, and their sum, twenty-two, is written
in hex:

```session
> X = 255 : BASE "x" : PRINT X
 FF
> PRINT 10 + 6; BASE$
 16 x
> BASE "d" : PRINT X
 255
```

## BASE$

A string function: the base string that `BASE` last set.

#### Syntax

```text
BASE$
```

#### Result

The string given to the last `BASE` statement, as it was given; `d` if there has been none.

#### See also

`BASE`.

#### Example

```session
> PRINT BASE$ : BASE "16r" : PRINT BASE$ : BASE "d"
d
16r
```

## BEEP

A statement: sounds the terminal's bell.

#### Syntax

```text
BEEP
```

#### Description

Writes the byte 7 (BEL) to standard output. A terminal sounds its bell, or flashes its window, as it is set to.
If output has been sent to a file, the byte goes into the file.

#### See also

`SOUND`, `PLAY`.

## BINARY

A mode of `OPEN`: `OPEN path$ FOR BINARY AS #f` opens a file for reading and writing bytes at any position. See
`OPEN`, `GET` and `PUT`.

## BIT

A numeric function: one bit of an integer.

#### Syntax

```text
BIT(x, n)
```

#### Arguments

*x*
: A numeric expression whose value is real. It is rounded to the nearest integer, a half going to the even one.

*n*
: A numeric expression: which bit. Bit 0 is the lowest, worth 1; bit *n* is worth 2 to the power *n*.

#### Result

1 if bit *n* of *x* is set, and 0 if it is clear.

A negative *x* is taken in two's complement with endless sign bits, so every bit of −1 is 1, however high. 0 if
*n* is negative.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | An argument is a string |
| 2 | `syntax error` | One argument only |

#### See also

`AND`, `OR`, `XOR`, `SHL`, `SHR`.

#### Example

```session
> PRINT BIT(5, 0); BIT(5, 1); BIT(5, 2); BIT(-1, 100); BIT(2 ^ 70, 70)
 1  0  1  1  1
```

## BYE

A statement: leaves HyBASIC. `BYE [n]` is another spelling of `SYSTEM [n]`. See `SYSTEM`.

## CALL

A statement: calls a `SUB`, machine code at an address, or a label in an `ASM` block.

#### Syntax

```text
[CALL] name [(] [argument [, argument] ...] [)]
CALL ABSOLUTE (address)
CALL ASM label [, a [, x [, y]]]
```

#### Arguments

*name*
: The name of a `SUB` defined anywhere in the program.

*argument*
: An expression, one for each of the `SUB`'s parameters, of the parameter's type.

*address*
: A numeric expression, 0 to 65,535: where the machine code is.

#### Description

**A `SUB`.** The word `CALL` is optional, and so are the parentheses round the arguments: `Show "x", 2`, `CALL
Show("x", 2)` and `CALL Show "x", 2` are the same call. The `SUB` runs, and the program goes on with the
statement after the call.

An argument that is a variable, or an array's element, is passed **by reference**: the parameter is another
name for it, and an assignment to the parameter changes it. Any other expression is passed by value. To pass a
variable by value, put it in parentheses of its own, which makes it an expression: `Inc (X)`.

**`CALL ABSOLUTE`** calls the machine code at *address* as a subroutine, with the A, X and Y registers 0. It is
`SYS address` by QuickBASIC's name. `RREG` gives the registers afterwards. See `SYS`.

**`CALL ASM`** calls a label of an `ASM` block. See `ASM`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 35 | `subprogram not defined` | No `SUB` has that name. Found when the program is compiled |
| 37 | `argument-count mismatch` | More arguments, or fewer, than the `SUB` has parameters |
| 13 | `type mismatch` | A string where the parameter is a number, or a number where it is a string |

#### Notes

A `SUB` can be called before the line that defines it: the whole program is compiled before any of it runs.

A `FUNCTION` is not called with `CALL`. It is used in an expression.

#### See also

`SUB`, `FUNCTION`, `DECLARE`, `SYS`, `ASM`, `RREG`.

#### Example

```session
> 10 SUB Inc (N) : N = N + 1 : END SUB
> 20 X = 1 : Inc X : PRINT X
> 30 CALL Inc((X)) : PRINT X
> 40 DIM A(3) : A(1) = 5 : CALL Inc(A(1)) : PRINT A(1)
> RUN
 2
 2
 6
```

## CASE

Part of `SELECT CASE`: `CASE` starts each of its choices. See `SELECT`.

## CDBL, CSNG

Numeric functions: their argument, unchanged.

#### Syntax

```text
CDBL(x)
CSNG(x)
```

#### Arguments

*x*
: A numeric expression.

#### Result

*x*. In QuickBASIC these convert to double and to single precision. HyBASIC has one kind of numeric variable,
which holds any number exactly, so there is nothing to convert: a rational stays a rational, and no digits are
lost or gained. They are here so that programs written for QuickBASIC run.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *x* is a string |

#### See also

`CINT`, `FIXED`, `RATIONAL`, `DIGITS`.

#### Example

```session
> PRINT CDBL(1/3); CSNG(2.5); CDBL(7)
 1/3  2.5  7
```

## CHDIR

A statement: changes the current directory.

#### Syntax

```text
CHDIR path$
```

#### Arguments

*path$*
: A string expression: the directory's name. One that doesn't start with `/` is taken from the current
  directory.

#### Description

Sets the current directory of HyBASIC's task, which is where a file name that doesn't start with `/` is looked
for by `OPEN`, `LOAD`, `SAVE`, `FILES`, `KILL` and the rest. The change lasts until the next `CHDIR` or until
HyBASIC ends. It doesn't change the directory of the shell that started HyBASIC.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 53 | `file not found` | There is no such name |
| 76 | `path not found` | The name is a file, not a directory |
| 13 | `type mismatch` | *path$* is a number |

#### See also

`DIR$`, `FILES`, `MKDIR`, `RMDIR`, `SHELL`.

#### Example

```session
> CHDIR "/ram" : OPEN "note.txt" FOR OUTPUT AS #1 : CLOSE : FILES
bin/            lib/            note.txt
> CHDIR "/nothere"
file not found
> CHDIR "/rom/bin/ls"
path not found
```

## CHR$

A string function: a string of one byte.

#### Syntax

```text
CHR$(n)
```

#### Arguments

*n*
: A numeric expression from 0 to 255. One that isn't an integer is rounded to the nearest, a half going to the
  even one.

#### Result

A string one byte long, whose byte is *n*. Any value can be made, 0 included, and a string can hold any of them
anywhere: `LEN(CHR$(0))` is 1.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *n* is less than 0 or more than 255 |
| 13 | `type mismatch` | *n* is a string |

#### Notes

What the console shows for a byte depends on the byte and on the character set (Appendix E). The codes below 32
are controls to a terminal: `CHR$(7)` rings its bell, `CHR$(10)` starts a new line, and `CHR$(27)` begins an
escape sequence (section 13.7).

#### See also

`ASC`, `STRING$`, `SPACE$`.

#### Example

```session
> PRINT CHR$(72) + CHR$(105); LEN(CHR$(0)); ASC(CHR$(65.5)); ASC(CHR$(66.5))
Hi 1  66  66
> PRINT CHR$(256)
illegal function call
```

## CINT, CLNG

Numeric functions: a real number rounded to the nearest integer.

#### Syntax

```text
CINT(x)
CLNG(x)
```

#### Arguments

*x*
: A numeric expression whose value is real.

#### Result

The integer nearest to *x*. A value exactly half way between two integers goes to the even one: 2.5 gives 2,
3.5 gives 4, and −2.5 gives −2. This is the rounding QuickBASIC's `CINT` does.

The two functions are the same. An integer here can be of any length, so there is no range to pass, where
QuickBASIC's `CINT` overflows beyond 32,767.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *x* is complex |
| 13 | `type mismatch` | *x* is a string |

#### See also

`INT` (rounds down), `FIX` (rounds toward zero), `FIXED`.

#### Example

```session
> PRINT CINT(2.5); CINT(3.5); CINT(-2.5); CINT(7/2); CINT(2.4); CINT(-2.6)
 2  4 -2  4  2 -3
> PRINT CLNG(2 ^ 40 + 1/2)
 1099511627776
```

## CIRCLE

A statement: draws a circle, or a filled disc, on the bitmap.

#### Syntax

```text
CIRCLE [STEP] (x, y), r [, c [, start [, end [, aspect [, F]]]]]
```

#### Arguments

*x*, *y*
: Numeric expressions: the centre (see `SCREEN` for coordinates). With `STEP`, they are an offset from the last
  point referenced.

*r*
: A numeric expression, 0 or more: the radius.

*c*
: A numeric expression, 0 to 255: the colour. A mode with fewer colours uses the number's low bits. If it is left
  out, the pen's colour.

*start*, *end*, *aspect*
: Accepted, evaluated and ignored. QuickBASIC draws arcs and ellipses with them; HyBASIC always draws a whole
  circle.

`F`
: The letter F: fill the circle, making a disc.

#### Description

Needs a graphics mode. The radius is in pixels, rounded to the nearest; with a `WINDOW` in force it is in the
window's x units. The centre becomes the last point referenced.

A radius of 0 draws the centre's one dot.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | No graphics mode is set; or *c* is not 0 to 255 |
| 258 | `invalid argument` | The driver refused it: a negative radius, or a centre far off the bitmap |
| 13 | `type mismatch` | An argument is a string |

#### See also

`SCREEN`, `COLOR`, `PSET`, `LINE`, `PAINT`, `WINDOW`, `POINT`.

#### Example

With a Vera X card. (`SCREEN` clears the console, so the graphics examples are shown as they were typed, not as
replayed transcripts.)

```text
> SCREEN 13 : CIRCLE (100, 100), 20, 200
> PRINT POINT(120, 100); POINT(100, 80); POINT(100, 100)
 200  200  0
> CIRCLE (100, 100), 10, 3, , , , F : PRINT POINT(100, 100); POINT(105, 100)
 3  3
> SCREEN 0
```

## CLEAR

A command of the prompt: forgets every variable.

#### Syntax

```text
CLEAR
```

#### Description

Every variable and array of the program is dropped, so that each is 0, empty or not yet made when it is next
used. The program's lines stay. A program that was stopped can no longer be continued with `CONT`.

`CLEAR` is one of the commands that work only when typed at the prompt, alone on its line. `RUN` clears the
variables itself as it starts.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 89 | `only at the prompt` | `CLEAR` in a program's line, or after other statements on a line typed at the prompt |

#### See also

`NEW`, `RUN`, `ERASE`, `CONT`.

## CLOSE

A statement: closes files.

#### Syntax

```text
CLOSE [[#]f [, [#]f] ...]
```

#### Arguments

*f*
: A numeric expression: the number of a file that `OPEN` opened.

#### Description

Each file named is closed, and its number is free to be used by another `OPEN`. With no numbers, every open file
is closed.

Closing a number that is not open is not an error: nothing happens. That holds for any number, 0 and numbers
above 255 included.

What a program has written with `PRINT #`, `WRITE #` or `PUT` is in the file as each statement finishes, not
held back until `CLOSE`; closing only gives up the file.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *f* is a string |

#### Notes

`RUN` closes every file as it starts, and files are closed when HyBASIC ends.

#### See also

`OPEN`, `FREEFILE`.

## CLS

A statement: clears the screen.

#### Syntax

```text
CLS
```

#### Description

Clears the console's text and puts the cursor at the top left. In a graphics mode (`SCREEN` 1 or more) the
bitmap is cleared to colour 0 as well, and the last point referenced becomes its middle.

If output is not a console, the bytes that would clear one (an escape sequence, section 13.7) go where output
goes.

#### See also

`LOCATE`, `SCREEN`, `COLOR`.

## COLOR

A statement: sets the colours of text, or in a graphics mode the colour drawn with.

#### Syntax

```text
COLOR [fg] [, bg]
```

#### Arguments

*fg*
: A numeric expression: the foreground colour.

*bg*
: A numeric expression: the background colour.

#### Description

**With no graphics mode** (`SCREEN 0`), `COLOR` sets the colours of the text written from then on. Each is 0 to
15: 0 to 7 are the terminal's eight colours (black, red, green, yellow, blue, magenta, cyan, white), and 8 to
15 are their bright forms. One that is left out is not changed. The colours are set by writing an escape
sequence (section 13.7), so what they look like is the terminal's choice.

**In a graphics mode** (`SCREEN` 1 or more), *fg* sets the pen: the colour used by the drawing statements when
they are given none. *bg* is accepted and ignored.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | A text colour that isn't 0 to 15 |
| 13 | `type mismatch` | An argument is a string |

#### See also

`SCREEN`, `PALETTE`, `CLS`, `PSET`, `LINE`, `CIRCLE`.

## COMMAND$

A string function: the arguments the program was started with, as one string.

#### Syntax

```text
COMMAND$
```

#### Result

The arguments after the program's name, joined with one space between each, 255 bytes at most. The empty string
if there were none, or if HyBASIC was started without a program.

An argument that held spaces of its own can't be told from two arguments here. `ARG$` gives each one whole.

#### See also

`ARG$`, `ENVIRON$`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo 'PRINT "["; COMMAND$; "]"' >cmd.bas
% basic cmd.bas one 'two words'
[one two words]
% basic cmd.bas
[]
```

## COMPLEX

A numeric function: a complex number made from its two parts.

#### Syntax

```text
COMPLEX(re, im)
```

#### Arguments

*re*, *im*
: Numeric expressions whose values are real: the real part and the imaginary part.

#### Result

The number *re* + *im* i. The parts keep their kinds: a rational real part with a decimal imaginary part is a
complex number of exactly those. If *im* is 0 the result is the real number *re*, not a complex number with
nothing in its imaginary part.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *re* or *im* is itself complex |
| 13 | `type mismatch` | An argument is a string |
| 2 | `syntax error` | One argument only |

#### Notes

A complex number can also be written directly, as `1+2i` or `0.5i`: digits with an `i` after them are an
imaginary number. The `i` must follow a number. `1+i` is 1 plus the variable `I`.

#### See also

`REAL`, `IMAG`, `SQR`.

#### Example

```session
> PRINT COMPLEX(1, 2); COMPLEX(1/2, 0.5); COMPLEX(3, 0); COMPLEX(0, 1) ^ 2
 1+2i  1/2+0.5i  3 -1
> PRINT COMPLEX(1+2i, 3)
illegal function call
```

## CONST

A statement: gives names to constant values.

#### Syntax

```text
CONST name = value [, name = value] ...
```

#### Arguments

*name*
: A name that has not been used: a string's if it ends in `$`, otherwise a number's.

*value*
: A constant expression of the name's type: numbers and strings written out, other constants, and operators
  and built-in functions applied to them.

#### Description

The value is worked out when the program is compiled, and the name stands for it from then on. A constant can
be used wherever a value can, and it is known in every `SUB` and `FUNCTION` without being `SHARED`.

A constant can't be assigned to, and its name can't be given to anything else.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 10 | `duplicate definition` | *name* is already a constant or a variable; or an assignment to a constant |
| 87 | `not a constant` | *value* uses a variable, an array or a `FUNCTION` |
| 13 | `type mismatch` | A string for a numeric name, or a number for a string name |

#### Notes

An integer constant's name is also a symbol in the program's `ASM` blocks, where it stands for its value.

#### See also

`DIM`, `LET`, `ASM`.

#### Example

```session
> CONST LIMIT = 5, UNIT$ = "cm", TWICE = LIMIT * 2 : PRINT LIMIT; UNIT$; TWICE
 5 cm 10
> LIMIT = 6
duplicate definition
> V = 3 : CONST LATER = V
not a constant
```

## CONT

A command of the prompt: goes on with a program that was stopped.

#### Syntax

```text
CONT
```

#### Description

A program stops, with its state kept, at a `STOP` statement or when <kbd>CTRL</kbd>-<kbd>C</kbd> is pressed.
`CONT` carries on from the statement after the one it stopped at, with every variable as it is now: variables
can be looked at, and changed, at the prompt in between.

A program can no longer be continued once it has ended, once one of its lines has been changed, added or
removed, or after `NEW`, `LOAD` or `CLEAR`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 17 | `cannot continue` | No program is stopped |
| 89 | `only at the prompt` | `CONT` in a program's line |

#### See also

`STOP`, `END`, `RUN`.

#### Example

```session
> 10 PRINT "one" : STOP : PRINT "two"
> 20 PRINT "three"
> RUN
one
Break in line 10
> CONT
two
three
> CONT
cannot continue
```

## COS

A numeric function: the cosine.

#### Syntax

```text
COS(x)
```

#### Arguments

*x*
: A numeric expression, complex too: an angle in radians.

#### Result

The cosine of *x*, from −1 to 1. `COS(0)` is the integer 1. Any other result is a decimal, correct to the number
of significant digits that `DIGITS` has set (12 unless it has been changed).

For a complex *x*, the principal value, as mathematics defines it (Chapter 18 of the manual): `COS(a + bi)` is cos *a* cosh *b* − i sin *a* sinh *b*, each part correctly rounded.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 13 | `type mismatch` | *x* is a string |

#### Notes

An angle in degrees is turned into radians by multiplying by `PI / 180`.

`PI` is itself rounded to the digits set, so `COS(PI / 2)` is not exactly 0. It is a very small number: the
cosine of the angle that `PI / 2` really is.

#### See also

`SIN`, `TAN`, `ATN`, `PI`, `DIGITS`.

#### Example

```session
> PRINT COS(0); COS(1); COS(PI); COS(60 * PI / 180)
 1  0.540302305868 -1  0.5
> PRINT COS(1i)
 1.54308063482
```

## CSRLIN

A numeric function: the row the cursor is on.

#### Syntax

```text
CSRLIN
```

#### Result

The cursor's row, counted from 1 at the top of the screen. 0 if HyBASIC has no console (its input or output is
a file or a pipe).

#### Description

HyBASIC does not keep track of the row. It asks the terminal each time, by writing a cursor position request
(section 13.7) and reading the terminal's reply, so the answer is right whatever has been written to the screen,
by this program or another.

#### Notes

The column is `POS(0)`.

#### See also

`POS`, `LOCATE`.

## CVN

A numeric function: the number that a string of bytes holds, in the stored format.

#### Syntax

```text
CVN(s$)
```

#### Arguments

*s$*
: A string expression: a number's bytes, as `MKN$` makes them and as `PUT` writes a numeric variable to a file.

#### Result

The number. It is exactly the number that was stored, of the same kind: `CVN(MKN$(x))` is *x* for every number
*x*.

#### Description

A number's stored form is as long as the number needs: a small integer takes one byte, a rational or a long
integer more.

It replaces QuickBASIC's `CVI`, `CVL`, `CVS` and `CVD`, as `MKN$` replaces `MKI$` and the rest: there is one
kind of number, so one pair of functions.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 5 | `illegal function call` | *s$* is not a number's stored form |
| 13 | `type mismatch` | *s$* is a number |

#### Notes

A string that was never a number may still be some number's stored form, and then there is no error: `CVN("x")`
is −8.

#### See also

`MKN$`, `GET`, `PUT`.

#### Example

```session
> PRINT CVN(MKN$(1/3)); CVN(MKN$(-7)); CVN(MKN$(2+3i)); LEN(MKN$(7)); LEN(MKN$(1/3))
 1/3 -7  2+3i  1  3
```
