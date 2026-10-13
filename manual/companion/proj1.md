# Project 1. A Quiz

> **THIS PROJECT:** HyBASIC. After Labs 20 and 21. Two or three sittings. You'll use `DATA` and `READ`, `LINE
> INPUT`, `IF` blocks, strings and a score.

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The starting point

Type this into `/sd/0/work/quiz.bas` with `edit`, and run it with `basic /sd/0/work/quiz.bas`.

<!-- listing samples/quiz.bas -->

```text
' quiz.bas - a quiz: its questions and answers are in the DATA lines
SCORE = 0
READ COUNT
FOR Q = 1 TO COUNT
  READ QUESTION$, ANSWER$
  PRINT Q; QUESTION$
  LINE INPUT "   Your answer: "; REPLY$
  IF UCASE$(REPLY$) = UCASE$(ANSWER$) THEN
    PRINT "   Right!"
    SCORE = SCORE + 1
  ELSE
    PRINT "   No: it's "; ANSWER$; "."
  END IF
NEXT Q
PRINT
PRINT "You scored"; SCORE; "out of"; COUNT

DATA 4
DATA "What chip is the Hydra's processor?", "65C02"
DATA "How many tasks can a Hydra run at once?", "16"
DATA "CTRL and which key stops a program?", "C"
DATA "What is the Hydra's shell called?", "HyForth"
```

A run looks like this:

<!-- session
cd /ram
!file samples/quiz.bas
-->

```session ___Your_answer:_
/ram> basic quiz.bas
 1 What chip is the Hydra's processor?
   Your answer: 65c02
   Right!
 2 How many tasks can a Hydra run at once?
   Your answer: 8
   No: it's 16.
 3 CTRL and which key stops a program?
   Your answer: c
   Right!
 4 What is the Hydra's shell called?
   Your answer: hyforth
   Right!

You scored 3 out of 4
```

### Understand it first

1. ✎ The first `DATA` line holds just a 4. What's it for? What must you remember when you add a question?

<div class="write l2"></div>

2. ✎ Why does the program compare `UCASE$(REPLY$)` with `UCASE$(ANSWER$)`, and not just the two strings?

<div class="write l2"></div>

3. ✎ Why `LINE INPUT`, and not `INPUT`? (Try an answer with a comma in it, both ways.)

<div class="write l2"></div>

### Make it yours

Do at least four of these. Tick each when it works.

| ☐ | Change |
|---|---|
| ☐ | Write your own questions, at least eight, about something you know well |
| ☐ | Ask the player's name at the start, and use it in the messages |
| ☐ | Give a second try before saying the answer |
| ☐ | Say something different at the end, depending on the score |
| ☐ | Let an answer be right if it's *in* the reply: "the 65C02" should count (`INSTR`) |
| ☐ | Ask the questions in a different order each time (`RND`, and an array) |
| ☐ | Keep the best score in a file, and say when it's been beaten (section 24.1) |
| ☐ | Take the time: how many seconds did the player need (`TIMER`)? |

✎ Which did you do? What was the hardest part, and how did you get past it?

<div class="write l5"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | Someone else has played it, without your help |
| ☐ | It copes with a silly answer, and with no answer at all |
| ☐ | The program has comments, and its lines are indented |
| ☐ | It's saved in `/sd/0/work` |

> **CHALLENGE:** ★ Take the questions out of the program altogether: keep them in a text file, one question and
> its answer to a line, so that a new quiz needs no programming. (`OPEN`, `LINE INPUT #` and `EOF`: section 24.1.)
