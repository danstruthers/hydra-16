# Chapter 23. Learning HyBASIC

**BASIC** (Beginner's All-purpose Symbolic Instruction Code) was invented in 1964, at Dartmouth College, to let
students who weren't scientists use a computer. In the 1980s nearly every home computer had a BASIC built in: it was
the first language of millions of people. **HyBASIC** is the Hydra's BASIC, in the style of Microsoft's QuickBASIC:
it has line numbers if you want them, but also named procedures, records and structured loops, so programs can be
written clearly. And it has the Hydra's numbers.

> **IN THIS CHAPTER:** You'll learn HyBASIC from the beginning: `PRINT`, variables, programs, `INPUT`, decisions,
> loops, strings, arrays, and procedures of your own. **You need:** Chapter 6. You don't need to have programmed
> before: this is the gentlest of the three languages.

## 23.1 Starting HyBASIC

Type `basic`. HyBASIC's prompt is `>`. A line typed without a number is done at once (**direct mode**); `SYSTEM` (or
`BYE`) leaves HyBASIC.

```session
/> basic
> PRINT "Hello, world!"
Hello, world!
> PRINT 2 + 3
 5
```

You can type keywords in capitals or small letters: HyBASIC turns them into capitals. `?` is a short way to type
`PRINT`. This book prints HyBASIC in capitals, as the books of the 1980s did, but `print 2 + 3` works just as
well.

> **TRY THIS:** Use HyBASIC as a calculator. What's `PRINT 365 * 24`: the hours in a year? How many seconds is
> that? And what does `PRINT 1 / 3 + 1 / 6` give? (HyBASIC keeps fractions exact: Chapter 18.)

## 23.2 PRINT

`PRINT` shows its items. A number is shown with a space in front (where a minus sign would go). Items separated by
`;` are shown one after another; items separated by `,` are shown in **zones**, 14 characters wide, like a table's
columns. A `;` at the end of a `PRINT` keeps the next `PRINT` on the same line.

```session
> PRINT 2 ^ 64
 18446744073709551616
> ? 10 / 4
 5/2
> A = 5
> B = A * 2
> PRINT A; B
 5  10
> PRINT A, B
 5             10
```

HyBASIC uses Chapter 18's numbers, so 2 to the 64th is exact, and 10 / 4 is exactly 5/2.

## 23.3 Variables

A **variable** is a name for a value. A name is letters and digits (and `_` or `.`), starting with a letter. A name
that ends in `$` holds a **string**: text, in double quotes. Any other holds a number.

```session
> NAME$ = "Sam"
> PRINT "Hello, "; NAME$
Hello, Sam
```

(`LET NAME$ = "Sam"` means the same: `LET` is optional, as in all modern BASICs.) A number variable that hasn't been
given a value is 0, and a string one is `""`, the empty string.

## 23.4 Programs

A line typed with a **line number** in front isn't done at once: it's kept, as part of the **program**. The lines are
kept in order of their numbers, so you can type them in any order, and put a line between two others by giving it a
number between theirs. That's why lines are usually numbered in tens. `RUN` runs the program, and `LIST` shows it:

```session
> 10 FOR I = 1 TO 5
> 20 PRINT I; I * I
> 30 NEXT I
> RUN
 1  1
 2  4
 3  9
 4  16
 5  25
> LIST
10 FOR I = 1 TO 5
20 PRINT I; I * I
30 NEXT I
```

| Command | What it does |
|---|---|
| `RUN` | Run the program |
| `LIST` [*from*][`-`*to*] | Show the program (or some of its lines) |
| `NEW` | Throw the program away (and its variables) |
| `SAVE "`*file*`"` | Keep it in a file |
| `LOAD "`*file*`"` | Bring one back |
| `DELETE` *from*`-`*to* | Remove lines |
| `EDIT` | Edit the program in the screen editor (Chapter 11) |
| `CONT` | Carry on after a `STOP`, or <kbd>CTRL</kbd>-<kbd>C</kbd> |

To change a line, type it again with the same number. To put a line between two others, give it a number between
theirs. To remove one, type just its number:

```session
> 10 PRINT "one"
> 20 PRINT "two"
> 15 PRINT "one and a half"
> LIST
10 PRINT "one"
15 PRINT "one and a half"
20 PRINT "two"
> 20
> LIST
10 PRINT "one"
15 PRINT "one and a half"
```

A program in memory is gone when you leave HyBASIC, or type `NEW`. `SAVE` keeps it in a file, and `LOAD`
brings it back:

<!-- session
10 PRINT "one"
15 PRINT "one and a half"
-->

```session
> SAVE "/ram/count.bas"
> NEW
> LIST
> LOAD "/ram/count.bas"
> RUN
one
one and a half
```

(Remember section 6.7: `/ram` forgets. Save the programs you want to keep on the SD card, as
`"/sd/0/work/count.bas"`.) Program files usually have names ending in `.bas`.

**Programs without line numbers.** Line numbers are only needed for typing a program in at the `>` prompt. A
program in a file doesn't need them, and is easier to read without them. Write it with the screen editor, and run it
from the shell with `basic` and the file's name:

```text
/> edit /ram/dice.bas
```

<!-- listing samples/dice.bas -->

```text
' dice.bas - throw two dice
RANDOMIZE TIMER
A = INT(RND * 6) + 1
B = INT(RND * 6) + 1
PRINT A; B; "makes"; A + B
```

<!-- session
cd /ram
!file samples/dice.bas
-->

```session
/ram> basic dice.bas
 5  6 makes 11
```

A line that starts with `'` (or `REM`) is a **comment**: a note for people, which HyBASIC skips. `RND` gives a
number between 0 and 1, chosen by chance, so `INT(RND * 6) + 1` is a whole number from 1 to 6: a die. And
`RANDOMIZE TIMER` shakes the dice with the clock, so that they don't fall the same way every time.

> **TIP:** For longer programs, `EDIT` is much easier than typing numbered lines: it opens the program in the screen
> editor, and when you close the editor, HyBASIC takes the program back. In the editor, you don't need line numbers
> at all.

## 23.5 INPUT

`INPUT` asks a question, and waits for the answer. The answer goes into the variable:

```session What_is_your_name?_
> 10 INPUT "What is your name"; N$
> 20 PRINT "Hello, "; N$; "!"
> RUN
What is your name? Sam
Hello, Sam!
```

`LINE INPUT` takes a whole line, commas and all.

## 23.6 Arithmetic

| Operator | Means | Operator | Means |
|---|---|---|---|
| `+` `-` `*` `/` | Add, subtract, multiply, divide | `^` | A power |
| `\` | Divide, keeping only the whole part | `MOD` | The remainder |
| `=` `<>` `<` `>` `<=` `>=` | Comparisons | `AND` `OR` `XOR` `NOT` | Logic |

The **order** follows school mathematics: `^` first, then `*` and `/`, then `+` and `-`. Parentheses change it.

```session
> PRINT INT(7.8), FIX(-7.8), 7 MOD 3, 7 \ 2
 7            -7             1             3
> PRINT SQR(2), SQR(-1)
 1.41421356237               1i
> PRINT 1/3 + 1/6
 1/2
> PRINT FIB(50)
 12586269025
```

A comparison gives -1 for true and 0 for false.

## 23.7 Decisions

`IF` ... `THEN` does something only if a test is true, and `ELSE` gives something to do if it's false. On one line:

```session
> A = 5
> IF A > 3 THEN PRINT "big" ELSE PRINT "small"
big
```

For more than one line, an `IF` **block** ends with `END IF`, and can have `ELSEIF`s:

```text
IF SCORE >= 90 THEN
  PRINT "Excellent!"
ELSEIF SCORE >= 50 THEN
  PRINT "Well done."
ELSE
  PRINT "Try again."
END IF
```

`SELECT CASE` chooses between many cases:

```session
> 10 FOR D = 0 TO 6
> 20   SELECT CASE D
> 30     CASE 0, 6: PRINT D; "weekend"
> 40     CASE ELSE: PRINT D; "weekday"
> 50   END SELECT
> 60 NEXT D
> RUN
 0 weekend
 1 weekday
 2 weekday
 3 weekday
 4 weekday
 5 weekday
 6 weekend
```

A `CASE` can be a list of values, a range (`CASE 1 TO 5`), or a test (`CASE IS > 100`).

## 23.8 Loops

`FOR` ... `NEXT` counts, as in section 23.4; `STEP` gives the size of each step (`FOR I = 10 TO 0 STEP -2`).
`DO` ... `LOOP` repeats while (or until) a test is true; the test can be at the start or the end:

```session
> X = 1 : DO WHILE X < 100 : X = X * 2 : LOOP : PRINT X
 128
```

(A `:` puts several statements on one line.)

| Loop | Repeats |
|---|---|
| `FOR` *v* `=` *a* `TO` *b* [`STEP` *s*] ... `NEXT` | With *v* from *a* to *b* |
| `DO WHILE` *test* ... `LOOP` | While the test is true (tested first) |
| `DO UNTIL` *test* ... `LOOP` | Until it's true |
| `DO` ... `LOOP WHILE` *test* | While it's true (tested at the end: always at least once) |
| `DO` ... `LOOP UNTIL` *test* | Until it's true (tested at the end) |
| `WHILE` *test* ... `WEND` | While it's true (the old way) |
| `EXIT FOR`, `EXIT DO` | Leave the loop at once |

## 23.9 Strings

| Function | Gives |
|---|---|
| `LEN(s$)` | Its length |
| `LEFT$(s$, n)`, `RIGHT$(s$, n)` | Its first, or last, *n* characters |
| `MID$(s$, i [, n])` | *n* characters from the *i*th (counting from 1) |
| `INSTR([i,] s$, t$)` | Where *t$* is in *s$* (0 if it isn't) |
| `UCASE$(s$)`, `LCASE$(s$)` | In capitals, in small letters |
| `LTRIM$(s$)`, `RTRIM$(s$)` | Without spaces at its start, at its end |
| `CHR$(n)`, `ASC(s$)` | The character with code *n*; the code of the first character |
| `STR$(x)`, `VAL(s$)` | A number as a string; the number a string holds |
| `SPACE$(n)`, `STRING$(n, c)` | *n* spaces; *n* of a character |

```session
> PRINT LEN("Hydra"), LEFT$("Hydra", 3), MID$("Hydra", 2, 3), UCASE$("hi")
 5            Hyd           ydr           HI
> PRINT CHR$(65); ASC("A")
A 65
```

`+` joins strings: `"Hydra" + "-16"` is `"Hydra-16"`.

## 23.10 Arrays

An **array** is a numbered list of values, under one name. `DIM` makes one, giving its highest number:

```session
> DIM SCORES(5)
> SCORES(1) = 10 : SCORES(2) = 20
> PRINT SCORES(1) + SCORES(2)
 30
```

`DIM SCORES(5)` makes `SCORES(0)` to `SCORES(5)`. Arrays can have more than one dimension, like a grid: `DIM
BOARD(8, 8)`. `REDIM` changes an array's size (`REDIM PRESERVE` keeps what's in it).

## 23.11 DATA and READ

`DATA` lines hold values inside the program; `READ` takes them, one by one:

```session
> 10 READ N$, A
> 20 IF N$ = "end" THEN END
> 30 PRINT N$; " is"; A
> 40 GOTO 10
> 50 DATA "Sam", 12, "Ana", 13, "end", 0
> RUN
Sam is 12
Ana is 13
```

`GOTO` jumps to a line, and `END` ends the program. (`RESTORE` starts the `DATA` again from the beginning.)

## 23.12 SUBs and FUNCTIONs

Big programs are easier to write, and to read, in pieces. A **SUB** is a named piece of program; a **FUNCTION** is
one that gives back a value. Each has **parameters**: names for the values it's given.

```session
> 10 PRINT SQUARE(7)
> 20 CALL GREET("Sam")
> 30 END
> 40 FUNCTION SQUARE(N)
> 50   SQUARE = N * N
> 60 END FUNCTION
> 70 SUB GREET(WHO$)
> 80   PRINT "Hello, "; WHO$; "!"
> 90 END SUB
> RUN
 49
Hello, Sam!
```

A FUNCTION's value is what's given to its name. The variables in a SUB or FUNCTION are its own: they don't change the
main program's variables of the same names. (`SHARED` lets it use them, and `STATIC` keeps its own between calls.)
`GOSUB` and `RETURN`, the older way, jump to a line and back.

> **TIP:** A program with `SUB`s and `FUNCTION`s reads better without line numbers (section 23.14). Keep it in a
> file, and run it from the shell with `basic` and its name.

## 23.13 Errors

When something goes wrong, HyBASIC stops the program, and says where. A program can catch its errors with `ON ERROR
GOTO`: the error jumps to a **handler**, where `ERR` is its number and `ERR$` its message, and `RESUME NEXT` carries on
after the statement that failed:

```session
> 10 ON ERROR GOTO 100
> 20 PRINT 1 / 0
> 30 PRINT "after"
> 40 END
> 100 PRINT "error"; ERR; ERR$
> 110 RESUME NEXT
> RUN
error 11 division by zero
after
```

Here are the mistakes everyone makes when they start, and what to do about each:

```session
> PRNT 5
syntax error
> PRINT "five" + 5
type mismatch
> GOTO 999
label not defined
> NEXT
NEXT without FOR
```

| What you see | What happened | What to do |
|---|---|---|
| `syntax error` | HyBASIC couldn't make sense of the line: a keyword spelled wrong, a missing quote or parenthesis | Read the line again, a word at a time |
| `type mismatch` | A string where a number should be, or the other way round | A string's name ends in `$`; `VAL` and `STR$` change one into the other |
| `label not defined` | A `GOTO` or `GOSUB` to a line that isn't there | `LIST` the program, and check the number |
| `NEXT without FOR` | A `NEXT` with no `FOR` before it, or loops that cross each other | Each `FOR` needs its own `NEXT`, inside any loop it's in |
| `?Redo from start` | `INPUT` wanted a number, and you typed something else | Type a number |
| `duplicate definition` | Two `SUB`s or `FUNCTION`s with the same name | Give one of them another name |
| `0`, or an empty line, where you expected a value | A variable's name spelled two ways: `SCORE` and `SCROE` are two variables, and the new one is 0 | Check the names |
| A program that never stops | A loop whose test never comes true | Press <kbd>CTRL</kbd>-<kbd>C</kbd>; then `CONT` carries on, or `LIST` shows the program |

When a program stops with an error, HyBASIC says which line it was in. `LIST` that line, and look at it hard.

## 23.14 Whole programs

Here are three small programs that use what this chapter has taught. Type them in, run them, and then change
them.

### A times table

A loop inside a loop: for each row `T`, the inner loop prints ten numbers. `PRINT USING "####"` prints each number
in a column four characters wide, so the columns line up, and the `;` at the end keeps them on one line. The
`PRINT` at line 50 ends each row:

```session
> 10 FOR T = 1 TO 9
> 20   FOR I = 1 TO 10
> 30     PRINT USING "####"; T * I;
> 40   NEXT I
> 50   PRINT
> 60 NEXT T
> RUN
   1   2   3   4   5   6   7   8   9  10
   2   4   6   8  10  12  14  16  18  20
   3   6   9  12  15  18  21  24  27  30
   4   8  12  16  20  24  28  32  36  40
   5  10  15  20  25  30  35  40  45  50
   6  12  18  24  30  36  42  48  54  60
   7  14  21  28  35  42  49  56  63  70
   8  16  24  32  40  48  56  64  72  80
   9  18  27  36  45  54  63  72  81  90
```

> **TRY THIS:** Change the program to ask which table you want (`INPUT "Which table"; T`), and print only that
> one, as "7 times 3 is 21", a line for each.

### Dice with a score

`dice.bas` threw two dice. This version throws them until they make a double, and counts the throws. It uses a
`DO` loop, because nobody knows how many times it will go round:

```session
> 10 RANDOMIZE TIMER
> 20 THROWS = 0
> 30 DO
> 40   A = INT(RND * 6) + 1 : B = INT(RND * 6) + 1
> 50   THROWS = THROWS + 1
> 60   PRINT A; B
> 70 LOOP UNTIL A = B
> 80 PRINT "A double, after"; THROWS; "throws."
> RUN
 1  2
 1  3
 4  4
A double, after 3 throws.
```

(Yours will fall differently.)

### FizzBuzz, in a file

In the game of FizzBuzz, you count aloud, but say "Fizz" instead of every number that 3 goes into, "Buzz" for
those that 5 goes into, and "FizzBuzz" for both. Go to `/ram`, start `edit fizz.bas`, and type:

<!-- listing samples/fizz.bas -->

```text
' fizz.bas - the game of FizzBuzz: count, but say Fizz for the threes,
' Buzz for the fives, and FizzBuzz for both
FOR N = 1 TO 15
  IF N MOD 15 = 0 THEN
    PRINT "FizzBuzz ";
  ELSEIF N MOD 3 = 0 THEN
    PRINT "Fizz ";
  ELSEIF N MOD 5 = 0 THEN
    PRINT "Buzz ";
  ELSE
    PRINT N;
  END IF
NEXT N
PRINT
```

Save it, leave the editor, and run it:

<!-- session
cd /ram
!file samples/fizz.bas
-->

```session
/ram> basic fizz.bas
 1  2 Fizz  4 Buzz Fizz  7  8 Fizz Buzz  11 Fizz  13  14 FizzBuzz
```

`N MOD 3` is the remainder when `N` is divided by 3: if it's 0, then 3 goes into `N`. The test for 15 has to come
first. (Why?) The lines inside the loop and inside the `IF` are moved in by two spaces. HyBASIC doesn't mind
either way, but **indenting** like this shows at a glance which lines belong to which loop or test.

> **TRY THIS:** Change `fizz.bas` so that numbers 7 goes into say "Whizz". What should 21 say? And 35? Then make it
> ask how far to count, with `INPUT`.

Chapter 24 shows how to use HyBASIC to work the Hydra: files, the terminal, sound and graphics. Appendix C lists every
keyword.
