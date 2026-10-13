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
