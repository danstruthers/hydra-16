# Card 6. HyBASIC

Chapter 23 teaches it, and Appendix C lists every keyword. Start it with `basic`; leave with `SYSTEM`; run a file
with `basic file.bas`.

### At the prompt

| Typed | What it does |
|---|---|
| `PRINT 2 + 3` (or `? 2 + 3`) | Do a line at once |
| `10 PRINT "Hi"` | Keep a line, as line 10 of the program |
| `10` | Remove line 10 |
| `LIST`, `RUN`, `NEW` | Show the program; run it; throw it away |
| `SAVE "file.bas"`, `LOAD "file.bas"` | Keep it in a file; bring it back |
| `EDIT` | Change it in the screen editor |
| <kbd>CTRL</kbd>-<kbd>C</kbd>, then `CONT` | Stop a running program; carry on |

### Values

| Written | Is |
|---|---|
| `SCORE = 10` | A number variable |
| `NAME$ = "Sam"` | A string variable: its name ends in `$` |
| `DIM A(10)` | An array: `A(0)` to `A(10)` |
| `+ - * /`, `\`, `MOD`, `^` | Arithmetic: `/` is exact (`10 / 4` is `5/2`); `\` keeps the whole part |
| `= <> < > <= >=`, `AND OR NOT` | Tests: -1 is true, 0 is false |
| `' text`, `REM text` | A comment |

### Printing and asking

```text
PRINT "Total:"; T          ' ; keeps things together   , moves to the next column
PRINT USING "###.##"; X    ' in a fixed shape
INPUT "How many"; N        ' ask for a number
LINE INPUT "Name: "; N$    ' ask for a whole line of text
```

### Decisions

```text
IF A > 3 THEN PRINT "big" ELSE PRINT "small"

IF SCORE >= 90 THEN
  PRINT "Excellent!"
ELSEIF SCORE >= 50 THEN
  PRINT "Well done."
ELSE
  PRINT "Try again."
END IF

SELECT CASE D
  CASE 0, 6 : PRINT "weekend"
  CASE ELSE : PRINT "weekday"
END SELECT
```

### Loops

```text
FOR I = 1 TO 10 STEP 2 : PRINT I : NEXT I

DO WHILE N < 100           ' tested first: may not run at all
  N = N * 2
LOOP

DO                         ' tested last: runs at least once
  INPUT "Guess"; G
LOOP UNTIL G = SECRET
```

`EXIT FOR` and `EXIT DO` leave a loop early.

### Strings and numbers

| Function | Gives |
|---|---|
| `LEN(S$)`, `LEFT$(S$, 3)`, `RIGHT$(S$, 3)`, `MID$(S$, 2, 3)` | Its length; its first 3; its last 3; 3 from the 2nd |
| `UCASE$(S$)`, `LCASE$(S$)`, `INSTR(S$, "x")` | Capitals; small letters; where `x` is (0: nowhere) |
| `STR$(N)`, `VAL(S$)`, `CHR$(65)`, `ASC("A")` | Number to string; string to number; code to character; and back |
| `INT(X)`, `ABS(X)`, `SQR(X)`, `FIXED(X, 2)` | Rounded down; its size; its square root; as a decimal to 2 places |
| `RND`, `INT(RND * 6) + 1`, `RANDOMIZE TIMER` | 0 to 1 by chance; a die; shake the dice |

### Data and procedures

```text
READ N$, AGE               ' takes the next values from the DATA lines
DATA "Sam", 12

SUB GREET (WHO$)           ' a procedure: GREET "Sam"
  PRINT "Hello, "; WHO$
END SUB

FUNCTION SQUARE (N)        ' a function: PRINT SQUARE(7)
  SQUARE = N * N
END FUNCTION
```

A program with a `SUB` or `FUNCTION` runs once for each time HyBASIC is started (section 23.12): keep it in a file,
and run it from the shell.

### When it goes wrong (section 23.13)

| You see | It means |
|---|---|
| `syntax error` | A word spelled wrong, or a quote or parenthesis missing |
| `type mismatch` | A string where a number should be, or the other way |
| `?Redo from start` | `INPUT` wanted a number |
| A 0 you didn't expect | A variable's name spelled two ways |
