# Lab 20. HyBASIC: First Programs

> **THIS LAB:** Manual: Chapter 23, sections 23.1 to 23.8. About an hour. You need a Hydra. Type `basic` to start
> HyBASIC, and `SYSTEM` to leave.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. HyBASIC as a calculator. First, guess. Then ask.

| I typed | My guess | HyBASIC said |
|---|---|---|
| `PRINT 2 + 3 * 4` | | |
| `PRINT (2 + 3) * 4` | | |
| `PRINT 10 / 4` | | |
| `PRINT 10 \ 4` | | |
| `PRINT 10 MOD 4` | | |
| `PRINT 2 ^ 10` | | |
| `PRINT "Hy" + "dra"` | | |

2. Variables. Type these, one at a time:

```text
> AGE = 12
> NAME$ = "Sam"
> PRINT NAME$; " is"; AGE
> PRINT NAME$, AGE
> AGE = AGE + 1
> PRINT AGE
```

   ✎ What's the difference between `;` and `,` in a `PRINT`? What does `AGE = AGE + 1` mean? (It isn't an equation!)

<div class="write l3"></div>

3. A program. Type the lines, then `LIST`, then `RUN`.

```text
> 10 FOR I = 1 TO 5
> 20 PRINT I; "times 7 is"; I * 7
> 30 NEXT I
```

   Now change it without typing it all again: make it go up to 12, and add a line 5 that prints a title.
   ✎ Write what you typed to make each change.

<div class="write l2"></div>

4. ✎ Add a line 40 that prints "Done." Then put a line *between* 20 and 30 that prints a row of dashes. What
   number did you give it? Why are lines usually numbered in tens?

<div class="write l2"></div>

5. Keep the program, clear it, and bring it back:

```text
> SAVE "/sd/0/work/seven.bas"
> NEW
> LIST
> LOAD "/sd/0/work/seven.bas"
> LIST
```

   ✎ What did the first `LIST` show? And the second?

<div class="write"></div>

6. A program that asks. Type `NEW`, and then:

```text
> 10 INPUT "What's your name"; N$
> 20 INPUT "How old are you"; A
> 30 IF A < 13 THEN PRINT "Hello, young "; N$ ELSE PRINT "Hello, "; N$
> 40 PRINT "Next year you'll be"; A + 1
```

   Run it twice, with different answers. ✎ What happens if you answer the second question with a word?

<div class="write"></div>

7. ✎ Change line 30 into a block `IF` ... `ELSEIF` ... `ELSE` ... `END IF` (section 23.7) with three cases: under 13,
   13 to 19, and over 19. Write your lines here.

<div class="write l6"></div>

### Questions

1. ✎ What's the difference between a line typed with a number in front and one without?

<div class="write l2"></div>

2. ✎ `NAME$` and `AGE` are both variables. What does the `$` mean?

<div class="write"></div>

3. ✎ A variable you've never given a value isn't an error. What is it? Why can that hide a mistake?

<div class="write l2"></div>

4. ✎ Which should come first in a program that works out an average: `INPUT`, the sum, or `PRINT`? What does that
   tell you about how BASIC reads a program?

<div class="write l2"></div>

5. ✎ What do `NEW`, `LIST`, `RUN`, `SAVE` and `LOAD` each do?

<div class="write l3"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | use HyBASIC as a calculator |
| ☐ | keep numbers and strings in variables |
| ☐ | type a program in, list it, run it, and change a line |
| ☐ | save a program, and load it again |
| ☐ | ask a question with `INPUT`, and decide with `IF` |

> **CHALLENGE:** ★ Write a program that asks for a number, and prints its times table from 1 to 12, lined up
> neatly with `PRINT USING` (section 23.14). Then make it ask "Another?" at the end, and go round again if the
> answer is yes.
