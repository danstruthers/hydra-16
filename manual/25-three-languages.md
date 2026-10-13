# Chapter 25. One Program, Three Languages

You've now met three programming languages. Which one is best? None of them: each has its own way of thinking
about a problem, and a problem that's awkward in one is easy in another. The clearest way to see the difference is
to write the same program in all three, and put them side by side. That's what this chapter does.

> **IN THIS CHAPTER:** You'll read one small game written three times, compare how each language does the same seven
> jobs, measure how fast each one runs, and get some help in choosing between them. **You need:** the "Learning"
> chapter of at least one of the languages (Chapter 19, 21 or 23). You'll follow your own language's version closely,
> and the other two well enough.

## 25.1 The game

The Hydra thinks of a number from 1 to 100, and you try to guess it. After each guess, it tells you whether you
were too small or too big, and when you get it right, it tells you how many guesses you took.

It's a small game, but it has a bit of everything. Whatever the language, the program has to:

1. choose a number by chance, and remember it;
2. print a message;
3. read a number that you type;
4. compare two numbers, and decide what to say;
5. go round again until you're right;
6. count the guesses.

Each program is in a file. Type in the one for your language (in `/ram`, with `edit`), or all three.

## 25.2 In HyBASIC

<!-- listing samples/guess.bas -->

```text
' guess.bas - guess the number, in HyBASIC
RANDOMIZE TIMER
SECRET = INT(RND * 100) + 1
TRIES = 0
PRINT "I'm thinking of a number from 1 to 100."
DO
  INPUT "Your guess"; GUESS
  TRIES = TRIES + 1
  IF GUESS < SECRET THEN
    PRINT "Too small."
  ELSEIF GUESS > SECRET THEN
    PRINT "Too big."
  END IF
LOOP UNTIL GUESS = SECRET
PRINT "Right! You took"; TRIES; "guesses."
```

<!-- session
cd /ram
!file samples/guess.bas
-->

```session Your_guess?_
/ram> basic guess.bas
I'm thinking of a number from 1 to 100.
Your guess? 50
Too big.
Your guess? 25
Too big.
Your guess? 12
Too big.
Your guess? 6
Too small.
Your guess? 9
Too big.
Your guess? 7
Right! You took 6 guesses.
```

HyBASIC's version reads from top to bottom, like a recipe: do this, then this, then go back and do it again
`UNTIL` the guess is right. `INPUT` does three jobs in one: it prints the question, waits for you, and turns what
you typed into a number. The values live in **variables** that any line can change: `TRIES = TRIES + 1` means "work
out `TRIES + 1`, and put the answer back in `TRIES`".

(Your secret number will be different from the book's: `RANDOMIZE TIMER` starts the random numbers from the clock.)

## 25.3 In hylang

<!-- listing samples/guess.hl -->

```text
; guess.hl - guess the number, in hylang

(def {secret} (+ 1 (random 100)))
(def {tries} 0)

(fun {ask} {do
  (write "Your guess? ")
  (val (read-line))})

(fun {play} {do
  =(guess (ask))
  (set! {tries} (+ tries 1))
  (cond
    {(< guess secret) (do (print "Too small.") (play))}
    {(> guess secret) (do (print "Too big.") (play))}
    {T (print "Right! You took" tries "guesses.")})})

(print "I'm thinking of a number from 1 to 100.")
(play)
```

<!-- session
cd /ram
!file samples/guess.hl
-->

```session Your_guess?_
/ram> hylang guess.hl
I'm thinking of a number from 1 to 100.
Your guess? 50
Too big.
Your guess? 25
Too big.
Your guess? 12
Too small.
Your guess? 18
Too small.
Your guess? 21
Right! You took 5 guesses.
```

hylang's version is made of **functions**. `ask` prints the question, reads a line, and gives back its value as a
number. `play` plays one turn: it asks, counts, and then `cond` picks one of three cases. Look at how it goes
round again: there's no loop. When the guess is wrong, `play` simply calls `play`, to play another turn. That's
**recursion** (section 21.9), and it's the way a Lisp programmer says "again".

`=(guess (ask))` makes a name that belongs to this turn only. The count has to last for the whole game, so
`tries` is a global name, made with `def`, and changed with `set!`.

## 25.4 In HyForth

<!-- listing samples/guess.fs -->

```text
\ guess.fs - guess the number, in HyForth
lib random

variable secret
variable tries

: ask ( -- n )                  \ A number, typed at the keyboard
  ." Your guess? " pad 8 accept
  0 0 rot pad swap >number 2drop drop ;

: judge ( n -- flag )           \ Say how a guess did; true if it's right
  dup secret @ < if drop ." Too small." cr false exit then
      secret @ > if      ." Too big." cr false exit then
  true ;

: play ( -- )
  100 random 1+ secret !  0 tries !
  ." I'm thinking of a number from 1 to 100." cr
  begin  ask  1 tries +!  judge  until
  ." Right! You took " tries @ . ." guesses." cr ;

play
```

<!-- session
cd /ram
!file samples/guess.fs
-->

```session Your_guess?_
/ram> forth guess.fs
I'm thinking of a number from 1 to 100.
Your guess? 50
Too big.
Your guess? 25
Too big.
Your guess? 12
Too small.
Your guess? 18
Too big.
Your guess? 15
Too big.
Your guess? 13
Too small.
Your guess? 14
Right! You took 7 guesses.
```

HyForth's version is made of small **words**, each built on the ones before, and each passing its values to the
next on the stack. `ask` leaves a number on the stack. `judge` takes it, says how the guess did, and leaves a
flag, which is just what `until` wants: so the loop in `play` reads almost like a sentence, "begin: ask, count,
judge, until right".

Forth has no `INPUT`, so `ask` does the job in two steps. `pad 8 accept` reads up to eight characters that you type
into a scratch area called `pad`, and leaves how many there were. `>number` then turns those characters into a
number. (Its stack diagram is in Appendix A; the `0 0` gives it a zero to start from, and the `2drop drop` throws
away the parts of its answer that this program doesn't need.) Every Forth programmer writes a word like `ask`
once, and keeps it.

## 25.5 Side by side

Here's each of the game's jobs, in each language:

| The job | HyBASIC | hylang | HyForth |
|---|---|---|---|
| A number from 1 to 100, by chance | `INT(RND * 100) + 1` | `(+ 1 (random 100))` | `100 random 1+` |
| Remember it | `SECRET = ...` | `(def {secret} ...)` | `variable secret` and `secret !` |
| Print a message | `PRINT "Too big."` | `(print "Too big.")` | `." Too big." cr` |
| Read a number | `INPUT "Your guess"; GUESS` | `(val (read-line))` | `pad 8 accept`, then `>number` |
| Decide | `IF` ... `ELSEIF` ... `END IF` | `(cond {test value} ...)` | `if` ... `then` |
| Go round again | `DO` ... `LOOP UNTIL` | `play` calls `play` | `begin` ... `until` |
| Count | `TRIES = TRIES + 1` | `(set! {tries} (+ tries 1))` | `1 tries +!` |

And here's what the differences come to:

| | HyBASIC | hylang | HyForth |
|---|---|---|---|
| A program is | A list of **statements**, done in order | **Expressions**, each with a value | **Words**, each made from other words |
| Values are kept | In variables | In names, and passed to functions | On the stack |
| It's written | Like a sentence: `PRINT A + B` | Function first: `(print (+ a b))` | Function last: `a b + .` |
| Its numbers | The Hydra's: any size, exact fractions | The Hydra's: any size, exact fractions | 16 bits; the Hydra's with a library |
| It's best at | Getting something working quickly, and being read by someone else | Lists, text and anything that's built up from pieces | Speed, small size, and getting close to the machine |
| The hard part | Big programs get tangled, unless you use `SUB`s | Counting the parentheses | Keeping track of the stack |

## 25.6 How fast?

Here's a race. Each language counts to ten thousand, one at a time, and says how long it took.

HyBASIC's `TIMER` is the number of seconds since midnight:

```session
> 10 T = TIMER : C = 0
> 20 FOR I = 1 TO 10000
> 30   C = C + 1
> 40 NEXT I
> 50 PRINT C; "in"; TIMER - T; "seconds"
> RUN
 10000 in 2.725 seconds
```

hylang's `(clock)` is the number of seconds since hylang started:

```session
hylang> (fun {count-up n} {do =(c 0) (dotimes {i n} (set! {c} (+ c 1))) c})
=> NIL
hylang> (def {t0} (clock))
=> NIL
hylang> (count-up 10000)
=> 10000
hylang> (- (clock) t0)
=> 2.435
```

And HyForth's `sys-ticks`, from the `hydra` library, counts the Hydra's **ticks**: there are 200 in a second.

```session
/> lib hydra
/> : count-up ( n -- count ) 0 swap 0 do 1+ loop ;
/> : timed ( -- ) sys-ticks 10000 count-up . sys-ticks swap - . ." ticks" cr ;
/> timed
10000 40 ticks
```

HyForth takes about a fifth of a second; the other two take more than ten times as long. That's the price of
their numbers. Each time HyBASIC or hylang adds 1, it's working with a number that could have a hundred digits, or
be a fraction, and it has to check. HyForth just adds two 16-bit cells, which the 65C02 can do in a few
instructions.

But don't choose a language by one race. For most programs, all three are fast enough, and the time that matters
is *yours*: how long the program takes to write, and to get right. And when a program really does need speed,
there's something faster than all three: the processor's own language, which is the subject of Part Five. A loop
like this one, in machine code, takes less than a tenth of a second.

## 25.7 Choosing

| If you want | Try |
|---|---|
| Your first program, today | HyBASIC |
| A calculation with huge numbers or exact fractions | HyBASIC or hylang |
| A program a friend can read without learning anything first | HyBASIC |
| To work on lists, words and text | hylang |
| To build something big out of small, tested pieces | hylang or HyForth |
| Speed, without learning machine code | HyForth |
| To control the Hydra's devices as you type | HyForth: it's the shell, so it's always there |
| To understand how a computer really runs a program | HyForth, then Part Five |

All three can read and write files, run other programs, play music and draw on the Vera X's screen: the second
chapter of each (Chapters 20, 22 and 24) shows how. And you don't have to choose only one. Programmers who know
several languages write better programs in each of them, because each language teaches a different habit: HyBASIC,
to say plainly what happens next; hylang, to think in values and functions; HyForth, to build big things from
small ones.
