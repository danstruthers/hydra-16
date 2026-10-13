# Lab 21. HyBASIC: Bigger Programs

> **THIS LAB:** Manual: Chapter 23, sections 23.8 to 23.14. About an hour. You need a Hydra, and Lab 20 done. The
> programs in this lab have no line numbers: write each in a file with `edit`, and run it from the shell with
> `basic` and its name.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. A loop that doesn't know how long it'll go on. Make `/sd/0/work/double.bas`:

```text
' double.bas - how many doublings to a million?
N = 1
COUNT = 0
DO WHILE N < 1000000
  N = N * 2
  COUNT = COUNT + 1
LOOP
PRINT COUNT; "doublings make"; N
```

   Run it with `basic /sd/0/work/double.bas`. ✎ What did it print? Why is a `DO` loop better here than a `FOR`?

<div class="write l2"></div>

2. Strings. Start `basic`, and try these. ✎ What does each print?

| I typed | It printed |
|---|---|
| `A$ = "Hydra-16"` | (nothing) |
| `PRINT LEN(A$)` | |
| `PRINT LEFT$(A$, 5)` | |
| `PRINT RIGHT$(A$, 2)` | |
| `PRINT MID$(A$, 2, 3)` | |
| `PRINT UCASE$(A$)` | |
| `PRINT INSTR(A$, "-")` | |

3. ✎ Write a program that asks for a word, and prints it backwards. (You'll need a `FOR` loop that counts *down*,
   with `STEP -1`, and `MID$`.)

<div class="write l5"></div>

4. An array, and data to fill it. Make `/sd/0/work/marks.bas`:

```text
' marks.bas - the average of some marks
DIM MARK(5)
TOTAL = 0
FOR I = 1 TO 5
  READ MARK(I)
  TOTAL = TOTAL + MARK(I)
NEXT I
PRINT "The average is"; TOTAL / 5
DATA 7, 9, 4, 8, 10
```

   ✎ What does it print? Is that what you'd expect from a calculator? Change it to print a decimal, to two places
   (`FIXED`, in Appendix C).

<div class="write l2"></div>

5. ✎ Add to `marks.bas`: after the average, print the highest mark, and the lowest. Write the lines you added.

<div class="write l5"></div>

6. A procedure of your own. Make `/sd/0/work/box.bas`:

```text
' box.bas - boxes of stars
BOX 3, 5
BOX 2, 10
END

SUB BOX (ROWS, COLS)
  FOR R = 1 TO ROWS
    PRINT STRING$(COLS, "*")
  NEXT R
  PRINT
END SUB
```

   ✎ What does it print? What are `ROWS` and `COLS`, inside the `SUB`?

<div class="write l2"></div>

7. ✎ Add a `FUNCTION AREA(ROWS, COLS)` to `box.bas`, and make the program print each box's area under it.

<div class="write l4"></div>

### Questions

1. ✎ `FOR`, `DO WHILE`, `DO` ... `LOOP UNTIL`: when would you choose each?

<div class="write l3"></div>

2. ✎ `DIM MARK(5)` makes how many places? (It isn't five.)

<div class="write"></div>

3. ✎ What's the difference between a `SUB` and a `FUNCTION`?

<div class="write l2"></div>

4. ✎ A variable inside a `SUB` is the `SUB`'s own. Why is that a good thing in a big program?

<div class="write l2"></div>

5. ✎ Why do the programs in this lab have their lines moved in by two spaces inside loops and `SUB`s? Does HyBASIC
   care?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | choose the right kind of loop |
| ☐ | take a string apart, and put strings together |
| ☐ | keep a list of values in an array, and fill it from `DATA` |
| ☐ | write a `SUB` and a `FUNCTION`, and pass them values |
| ☐ | write a program in a file, without line numbers, and run it |

> **CHALLENGE:** ★ Write a quiz. Keep five questions and their answers in `DATA` lines. Ask each question, check
> the answer (make `Paris`, `paris` and `PARIS` all count: `UCASE$`), keep score, and at the end say how many were
> right, and something kind or cheeky depending on the score.
