# Appendix C. HyBASIC Reference

This appendix lists HyBASIC's keywords, in alphabetical order. In the syntax, words in `CAPITALS` are typed as they
are, *italic* words stand for something you choose, `[ ]` marks something you can leave out, and `|` separates
choices. *n* and *x* are numbers, *s$* a string, *v* a variable, *f* a file number.

## C.1 The rules

* **Lines.** A line is a statement, or several separated by `:`. With a number first, it's part of the program;
  without, it's done at once. A label (a name and a `:`) can mark a line instead of a number, for `GOTO` and `GOSUB`.
* **Names.** Letters, digits, `_` and `.`, a letter first; capitals and small letters are the same. A name ending in
  `$` is a string's; any other a number's (the endings `%`, `&`, `!` and `#` are allowed, and mean a number).
* **Numbers.** Chapter 18's, in any of its forms, and `&H`, `&O`, `&B` and `E` notation. Every number variable can hold
  any kind of number: `INTEGER`, `LONG`, `SINGLE`, `DOUBLE` and `NUMBER` all mean "a number".
* **Strings.** In double quotes. `+` joins them.
* **Comments.** `REM`, or `'`, to the end of the line.
* **Truth.** A comparison gives -1 (true) or 0 (false); any number but 0 counts as true.
* **Operators**, from first to last: `^`; `-` (negation); `*` `/`; `\`; `MOD`; `+` `-`; `=` `<>` `<` `>` `<=` `>=`;
  `NOT`; `AND`; `OR`; `XOR`; `EQV`; `IMP`.

## C.2 The keywords

| Keyword | Syntax | What it does |
|---|---|---|
| `ABS` | `ABS(x)` | The size of *x* |
| `ABSOLUTE` | `CALL ABSOLUTE (addr)` | Call machine code |
| `AND` | *a* `AND` *b* | True if both are; bit by bit on numbers |
| `APPEND` | `OPEN ... FOR APPEND` | Open a file to write on to its end |
| `ARG$` | `ARG$(n)` | A program's argument *n* (0: its file's name) |
| `AS` | | In `DIM`, `OPEN`, `NAME` and `TYPE` |
| `ASC` | `ASC(s$)` | The code of the first character |
| `ASM` | `ASM` ... `END ASM` | Assembly language in the program (Chapter 27); `CALL ASM` *label* calls it |
| `ATN` | `ATN(x)` | The arctangent |
| `BANK` | `BANK n`, `BANK()` | Which RAM bank `PEEK` and `POKE` see at $8000 |
| `BASE` | `BASE b$`; `OPTION BASE 0` \| `1` | The base numbers are read and shown in; arrays' lowest index |
| `BASE$` | `BASE$` | The base's string |
| `BEEP` | `BEEP` | Ring the bell |
| `BINARY` | `OPEN ... FOR BINARY` | Open a file for bytes |
| `BIT` | `BIT(x, n)` | Bit *n* of *x*: 0 or 1 |
| `BYE` | `BYE [n]` | Leave HyBASIC (status *n*) |
| `CALL` | `CALL` *name*[`(`*args*`)`] | Call a SUB (the `CALL` can be left out) |
| `CASE` | `SELECT CASE` *x* / `CASE` *items* / `CASE ELSE` / `END SELECT` | Choose by value: items are values, *a* `TO` *b*, or `IS` *op value* |
| `CDBL`, `CSNG` | `CDBL(x)` | *x* (no change: there's one kind of number) |
| `CHDIR` | `CHDIR d$` | Change directory |
| `CHR$` | `CHR$(n)` | The character with code *n* |
| `CINT`, `CLNG` | `CINT(x)` | The nearest whole number (a half to the even one) |
| `CIRCLE` | `CIRCLE [STEP](x, y), r [, c [, start [, end [, aspect [, F]]]]]` | A circle (`F`: filled) |
| `CLEAR` | `CLEAR` | (At the prompt) forget the variables |
| `CLOSE` | `CLOSE [[#]f, ...]` | Close files (all, if none named) |
| `CLS` | `CLS` | Clear the screen (and a bitmap) |
| `COLOR` | `COLOR [fg] [, bg]` | Colours (with a bitmap: the pen) |
| `COMMAND$` | `COMMAND$` | A program's arguments |
| `COMPLEX` | `COMPLEX(re, im)` | A complex number |
| `CONST` | `CONST` *name* `=` *value*, ... | Constants |
| `CONT` | `CONT` | Carry on after `STOP` or <kbd>CTRL</kbd>-<kbd>C</kbd> |
| `COS` | `COS(x)` | The cosine |
| `CSRLIN` | `CSRLIN` | The cursor's row |
| `CVN` | `CVN(s$)` | The number whose bytes *s$* is (`MKN$`'s opposite) |
| `DATA` | `DATA` *items* | Values for `READ` |
| `DATE$` | `DATE$` | The date: `yyyy-mm-dd` |
| `DECLARE` | `DECLARE SUB` \| `FUNCTION` *name*`(`*params*`)` | (Allowed; not needed) |
| `DEFINT` ... `DEFSTR` | `DEFSTR` *a*`-`*z*, ... | Names starting with these letters are strings' (`DEFSTR`) or numbers' (the rest) |
| `DELETE` | `DELETE` *from*`-`*to* | Remove program lines |
| `DENOMINATOR` | `DENOMINATOR(x)` | A fraction's bottom |
| `DIGITS` | `DIGITS n`, `DIGITS()` | The math functions' precision |
| `DIM` | `DIM [SHARED]` *name*[`(`*bounds*`)`] [`AS` *type*], ... | Arrays, and variables of a type |
| `DIR$` | `DIR$(path$)`, `DIR$` | A directory's first name; the next |
| `DO` | `DO [WHILE` \| `UNTIL` *c*`]` ... `LOOP [WHILE` \| `UNTIL` *c*`]` | A loop |
| `DRAW` | `DRAW s$` | Draw with moves (section 24.6) |
| `EDIT` | `EDIT [label]` | Edit the program in the screen editor |
| `ELSE`, `ELSEIF` | | In `IF` |
| `END` | `END`; `END IF` \| `SELECT` \| `SUB` \| `FUNCTION` \| `TYPE` | End the program; end a block |
| `ENV$`, `ENVIRON$` | `ENV$(name$)` | An environment variable |
| `ENVIRON` | `ENVIRON "name=value"` | Set one (no value: remove it) |
| `EOF` | `EOF(f)` | -1 at the file's end |
| `EQV` | *a* `EQV` *b* | `NOT (a XOR b)` |
| `ERASE` | `ERASE` *array*, ... | Drop arrays |
| `ERL`, `ERR`, `ERR$` | | The last error's line, number and message |
| `ERROR` | `ERROR n` | Make error *n* |
| `EXIT` | `EXIT FOR` \| `DO` \| `SUB` \| `FUNCTION` | Leave a loop or procedure at once |
| `EXP` | `EXP(x)` | *e* to the power *x* |
| `FIB` | `FIB(n)` | The *n*th Fibonacci number |
| `FILES` | `FILES [path$]` | List a directory |
| `FIX` | `FIX(x)` | The whole part (toward zero) |
| `FIXED` | `FIXED(x, n)` | *x* as a fixed decimal of *n* places |
| `FOR` | `FOR` *v* `=` *a* `TO` *b* `[STEP` *s*`]` ... `NEXT [`*v*`]` | A counting loop |
| `FRE` | `FRE(0)` | Free memory, in bytes |
| `FREEFILE` | `FREEFILE` | A file number not in use |
| `FUNCTION` | `FUNCTION` *name*`(`*params*`)` ... `END FUNCTION` | A function: give its name the value |
| `GCD` | `GCD(a, b)` | The greatest common divisor |
| `GET` | `GET [#]f, [pos], v` | Read from a binary file |
| `GOSUB` | `GOSUB` *line* | Jump to a line, and come back at `RETURN` |
| `GOTO` | `GOTO` *line* | Jump to a line |
| `GPRINT` | `GPRINT [STEP](x, y), text$` | Text on the bitmap |
| `HEX$` | `HEX$(x)` | Its digits in hex |
| `IF` | `IF` *c* `THEN` *...* `[ELSE` *...*`]`; or a block to `END IF` | A decision |
| `IMAG` | `IMAG(z)` | A complex number's imaginary part |
| `IMP` | *a* `IMP` *b* | `(NOT a) OR b` |
| `INCLUDE` | `INCLUDE "file"` | Another file's lines, here |
| `INKEY$` | `INKEY$` | The key pressed, or `""` |
| `INPUT` | `INPUT [;] ["prompt" ; \| ,] v, ...`; `INPUT #f, v, ...` | Read values |
| `INPUT$` | `INPUT$(n [, #f])` | *n* keys, or bytes of a file |
| `INSTR` | `INSTR([i,] s$, t$)` | Where *t$* is in *s$* (0: not there) |
| `INT` | `INT(x)` | Round down |
| `IS` | `CASE IS > 5` | In `CASE` |
| `KILL` | `KILL f$` | Remove a file |
| `LBOUND`, `UBOUND` | `LBOUND(a [, d])` | An array's lowest and highest index |
| `LCASE$`, `UCASE$` | `LCASE$(s$)` | Small letters; capitals |
| `LEFT$`, `RIGHT$` | `LEFT$(s$, n)` | The first, last, *n* characters |
| `LEN` | `LEN(s$)` | Its length |
| `LET` | `[LET]` *v* `=` *x* | Give a variable a value |
| `LINE` | `LINE [[STEP](x1, y1)]-[STEP](x2, y2) [, c [, B \| BF]]` | A line, box or filled box |
| `LINE INPUT` | `LINE INPUT ["prompt";] v$`; `LINE INPUT #f, v$` | Read a whole line |
| `LIST` | `LIST [from][-[to]]` | Show the program |
| `LOAD` | `LOAD "file"` | Load a program |
| `LOC`, `LOF` | `LOC(f)`, `LOF(f)` | The place in a file; its length |
| `LOCATE` | `LOCATE [row] [, [col] [, cursor]]` | Move the cursor (from 1) |
| `LOG` | `LOG(x)` | The natural logarithm |
| `LOOP` | | In `DO` |
| `LTRIM$`, `RTRIM$` | `LTRIM$(s$)` | Without spaces at the start; at the end |
| `MID$` | `MID$(s$, i [, n])` | *n* characters from the *i*th |
| `MKDIR`, `RMDIR` | `MKDIR d$` | Make, remove a directory |
| `MKN$` | `MKN$(x)` | A number's bytes, as a string |
| `MOD` | *a* `MOD` *b* | The remainder |
| `NAME` | `NAME old$ AS new$` | Rename a file |
| `NEW` | `NEW` | Throw the program away |
| `NEXT` | | In `FOR` |
| `NOT` | `NOT` *a* | The opposite |
| `NUMERATOR` | `NUMERATOR(x)` | A fraction's top |
| `OCT$` | `OCT$(x)` | Its digits in octal |
| `ON` | `ON n GOTO` \| `GOSUB` *lines*; `ON ERROR GOTO` *line* | Jump to the *n*th line; catch errors (`GOTO 0`: stop catching) |
| `OPEN` | `OPEN path$ FOR INPUT \| OUTPUT \| APPEND \| BINARY AS [#]f [LEN = n]` | Open a file |
| `OPTION` | `OPTION BASE 0` \| `1` | Arrays' lowest index |
| `OR` | *a* `OR` *b* | True if either is |
| `OUTPUT` | `OPEN ... FOR OUTPUT` | Open a file to write |
| `PAINT` | `PAINT [STEP](x, y) [, c [, border]]` | Fill an area |
| `PALETTE` | `PALETTE n, rgb` | Change a colour |
| `PEEK`, `POKE` | `PEEK(addr)`, `POKE addr, b` | Memory |
| `PI` | `PI` | *π*, to the precision |
| `PLAY` | `PLAY [ch,] mml$`; `PLAY file$` | Music |
| `POINT` | `POINT(x, y)` | A dot's colour (-1: off the bitmap) |
| `POS` | `POS(0)` | The cursor's column |
| `PRESERVE` | `REDIM PRESERVE` | Keep an array's contents |
| `PRESET`, `PSET` | `PSET [STEP](x, y) [, c]` | Clear, set a dot |
| `PRINT` | `PRINT [#f,] [USING fmt$;] items` | Print (`;` together, `,` in zones) |
| `PUT` | `PUT [#]f, [pos], x` | Write to a binary file |
| `RANDOMIZE` | `RANDOMIZE [n]` | Start the random numbers (none: from the clock) |
| `RATIONAL` | `RATIONAL(x)` | As a fraction |
| `READ` | `READ v, ...` | The next `DATA` values |
| `REAL` | `REAL(z)` | A complex number's real part |
| `REDIM` | `REDIM [PRESERVE]` *arrays* | Make arrays again |
| `REM` | `REM` *text* | A comment |
| `RESTORE` | `RESTORE [line]` | `READ` from the first `DATA` again |
| `RESUME` | `RESUME [0 \| NEXT \| line]` | After an error: again; the next statement; a line |
| `RETURN` | `RETURN [line]`; `RETURN` *value* | Back from `GOSUB`; a FUNCTION's value |
| `RND` | `RND`, `RND(x)` | A random number from 0 to 1 |
| `RREG` | `RREG [a] [, x] [, y] [, p]` | The registers after the last `SYS` |
| `RUN` | `RUN` | Run the program |
| `SAVE` | `SAVE "file"` | Keep the program |
| `SCREEN` | `SCREEN n` | A graphics mode: 0, 1, 2, 7, 12 or 13 (section 24.6) |
| `SEEK` | `SEEK [#]f, pos`; `SEEK(f)` | The place in a file |
| `SELECT` | `SELECT CASE` *x* | (`CASE`) |
| `SGN` | `SGN(x)` | -1, 0 or 1 |
| `SHARED` | `SHARED` *names* | In a procedure: use the main program's variables |
| `SHELL` | `SHELL [line$]` | Run a command line |
| `SHELL$` | `SHELL$(line$)` | A command line's output |
| `SHL`, `SHR` | `SHL(x, n)` | Shift bits left, right |
| `SIN` | `SIN(x)` | The sine |
| `SLEEP` | `SLEEP [s]` | Wait (none: until a key) |
| `SOUND` | `SOUND ch [, note [, patch [, vol]]]`; `SOUND "text"` | A note; a sound command |
| `SPACE$` | `SPACE$(n)` | *n* spaces |
| `SPC` | `PRINT SPC(n)` | *n* spaces, in `PRINT` |
| `SPRITE` | `SPRITE n, x, y [, image$]`; `SPRITE n OFF` | Sprites |
| `SQR` | `SQR(x)` | The square root |
| `STATIC` | `STATIC` *names* | In a procedure: variables kept between calls |
| `STATUS` | `STATUS` | The last `SHELL`'s status |
| `STEP` | | In `FOR`, and graphics' points |
| `STOP` | `STOP` | Stop the program (`CONT` carries on) |
| `STR$` | `STR$(x [, base$])` | A number's text |
| `STRING`, `STRING$` | `AS STRING [* n]`; `STRING$(n, c)` | A string type; *n* of a character |
| `SUB` | `SUB` *name*`(`*params*`)` ... `END SUB` | A procedure |
| `SWAP` | `SWAP a, b` | Exchange two variables' values |
| `SYS` | `SYS addr \| "NAME" [, a [, x [, y]]]` | Machine code; a system call |
| `SYSTEM` | `SYSTEM [n]` | Leave HyBASIC |
| `TAB` | `PRINT TAB(n)` | To column *n* |
| `TAN` | `TAN(x)` | The tangent |
| `THEN` | | In `IF` |
| `TIME$`, `TIMER` | | The time; seconds since midnight |
| `TO` | | In `FOR` and `CASE` |
| `TYPE` | `TYPE` *name* / *field* `AS` *type* / `END TYPE` | A record type |
| `UNTIL` | | In `DO` |
| `USING` | `PRINT USING fmt$;` | A number's form |
| `VAL` | `VAL(s$)` | The number at the start of *s$* (0 if none) |
| `VIEW` | `VIEW [[SCREEN] (x1, y1)-(x2, y2)]` | Draw in only part of the bitmap |
| `WHILE`, `WEND` | `WHILE c` ... `WEND` | A loop |
| `WIDTH` | `WIDTH n` | The screen's width |
| `WINDOW` | `WINDOW [[SCREEN] (x1, y1)-(x2, y2)]` | Coordinates of your own |
| `WRITE` | `WRITE [#f,] items` | Items, strings in quotes, commas between |
| `XOR` | *a* `XOR` *b* | True if one is, not both |

## C.3 Examples

Each of these was typed into HyBASIC, as it is here. The ones with line numbers are programs, run with `RUN`.

### Numbers and printing

```session
> PRINT 7 / 2; 7 \ 2; 7 MOD 2
 7/2  3  1
> PRINT 2 ^ 100
 1267650600228229401496703205376
> PRINT FIXED(22 / 7, 4), RATIONAL(0.75)
 3.1428        3/4
> PRINT INT(-2.5); FIX(-2.5); CINT(2.5); SGN(-9); ABS(-9)
-3 -2  2 -1  9
> PRINT SQR(16); SQR(-16); GCD(12, 18); FIB(30)
 4  4i  6  832040
> PRINT HEX$(255); " "; OCT$(8); " "; STR$(255, "b")
FF 10 11111111
> PRINT USING "###.##"; 3.14159
  3.14
> PRINT "A"; TAB(10); "B"; SPC(3); "C"
A        B   C
> WRITE "Sam", 12
"Sam",12
```

### Strings

```session
> A$ = "Hydra-16"
> PRINT LEN(A$); LEFT$(A$, 5); " "; RIGHT$(A$, 2); " "; MID$(A$, 2, 3)
 8 Hydra 16 ydr
> PRINT INSTR(A$, "-"); UCASE$(A$); " "; LCASE$(A$)
 6 HYDRA-16 hydra-16
> PRINT ASC("A"); CHR$(66); VAL("42 apples") + 1; STR$(7) + "!"
 65 B 43  7!
> PRINT STRING$(5, "*"); SPACE$(2); LTRIM$("   x")
*****  x
```

### Decisions and loops

```session
> 10 FOR I = 10 TO 1 STEP -3 : PRINT I; : NEXT : PRINT
> 20 N = 1 : DO WHILE N < 50 : N = N * 2 : LOOP : PRINT N
> 30 SELECT CASE N
> 40   CASE 1 TO 9 : PRINT "small"
> 50   CASE IS > 60 : PRINT "big"
> 60   CASE ELSE : PRINT "middling"
> 70 END SELECT
> 80 ON 2 GOSUB 100, 200 : END
> 100 PRINT "first" : RETURN
> 200 PRINT "second" : RETURN
> RUN
 10  7  4  1
 64
big
second
```

### Arrays, data and records

```session
> 10 DIM A(4)
> 20 FOR I = 0 TO 4 : READ A(I) : NEXT
> 30 DATA 5, 3, 8, 1, 9
> 40 IF A(0) > A(1) THEN SWAP A(0), A(1)
> 50 PRINT A(0); A(1); LBOUND(A); UBOUND(A)
> 60 TYPE PET
> 70   KIND AS STRING
> 80   LEGS AS INTEGER
> 90 END TYPE
> 100 DIM P AS PET
> 110 P.KIND = "dog" : P.LEGS = 4
> 120 PRINT "A "; P.KIND; " has"; P.LEGS; "legs"
> RUN
 3  5  0  4
A dog has 4 legs
```

### Procedures

```session
> 10 PRINT TWICE(21)
> 20 SHOUT "hello"
> 30 END
> 40 FUNCTION TWICE(N)
> 50   TWICE = N * 2
> 60 END FUNCTION
> 70 SUB SHOUT(S$)
> 80   PRINT UCASE$(S$); "!"
> 90 END SUB
> RUN
 42
HELLO!
```

### Files

```session
> OPEN "/ram/pets.txt" FOR OUTPUT AS #1
> PRINT #1, "Rex" : PRINT #1, "Max"
> CLOSE #1
> OPEN "/ram/pets.txt" FOR INPUT AS #1
> LINE INPUT #1, A$ : PRINT A$; EOF(1)
Rex 0
> LINE INPUT #1, A$ : PRINT A$; EOF(1)
Max-1
> CLOSE
> FILES "/ram"
bin/            lib/            pets.txt
> KILL "/ram/pets.txt"
```

### Catching errors

```session
> 10 ON ERROR GOTO 100
> 20 PRINT 1 / 0
> 30 PRINT "carried on"
> 40 END
> 100 PRINT "error"; ERR; "in line"; ERL; ": "; ERR$
> 110 RESUME NEXT
> RUN
error 11 in line 20 : division by zero
carried on
```

### The system, and bits

```session
> PRINT DATE$; " "; TIME$
2000-01-01 00:00:04
> SHELL "echo hello from the shell"
hello from the shell
> PRINT STATUS
 0
> A$ = SHELL$("date") : PRINT LEFT$(A$, 10)
2000-01-01
> ENVIRON "colour=blue" : PRINT ENV$("colour")
blue
> PRINT HEX$(PEEK(65533)); FRE(0) > 0
E0-1
> PRINT BIT(5, 0); SHL(1, 4); 12 AND 10; 12 OR 10; 12 XOR 10
 1  16  8  14  6
```

## C.4 Errors

HyBASIC's error messages name what went wrong; `ERR` gives its number, `ERR$` its message, and `ERL` its line. A
system error's number is 256 plus its code from Appendix G.
