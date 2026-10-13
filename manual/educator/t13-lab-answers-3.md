# Topic 13. Labs 16 to 23

## Lab 16. HyForth: The Stack

**1.** (Each line is followed here by enough `drop`s to clear the stack.)

```session
/> 1 2 3 .s
<3> 1 2 3
/> drop drop drop 1 2 3 swap .s
<3> 1 3 2
/> drop drop drop 1 2 3 rot .s
<3> 2 3 1
/> drop drop drop 1 2 over .s
<3> 1 2 1
/> drop drop drop 5 dup * .s
<1> 25
/> drop 10 2 3 + * .s
<1> 50
/> drop 10 2 * 3 + .s
<1> 23
```

**2.**

```session
/> 4 5 + .
9
/> 4 5 + 2 * .
18
/> 100 7 8 * - .
44
/> 12 4 + 9 5 - / .
4
/> 100 7 mod .
2
```

**3.** It's a comment: a **stack diagram**, saying that `square` takes one number and leaves one. HyForth skips
everything between `(` and `)`. It's for the reader.

```session
/> : square ( n -- n*n ) dup * ;
/> 9 square .
81
/> see square
: square dup * ;
```

**4.** For example:

```session
/> : double ( n -- 2n ) 2 * ;
/> : cube ( n -- n^3 ) dup dup * * ;
/> : average ( a b -- avg ) + 2 / ;
/> : hello ( -- ) ." Hello from Sam" cr ;
/> 21 double . 3 cube . 10 20 average .
42 27 15
/> hello
Hello from Sam
```

**5.** The prompt becomes a row of spaces: HyForth is still inside the definition, compiling. A `;` ends it.

**6.**

```session
/> drop
stack underflow
/> 1 0 /
division by zero
```

**Questions.**

1. A pile of numbers. A number typed goes on the top, and words take their numbers from the top and put their
   answers back there: the last one on is the first one off, like a stack of plates.
2. The order of the words *is* the order things are done in, so no brackets are ever needed, and no rules about
   which operation comes first. (And it makes the language tiny: every word works the same way.)
3. `.` takes the top number off the stack and prints it. `.s` shows the whole stack, and leaves it alone.
4. There's no space before the `;`. Forth reads `2*;` as one word, doesn't know it, and gives up on the
   definition:

```session
/> : double 2*;
2*; ?
```

5. 32,767 (or 65,535, if it's read as unsigned). `32767 1 + .` prints `-32768`: the sum has gone past the largest
   positive number a 16-bit cell can hold, and wrapped round to the most negative.

**Challenge.**

```session
/> : f>c ( f -- c ) 32 - 5 * 9 / ;
/> 212 f>c . 32 f>c . 98 f>c .
100 0 36
/> : c>f ( c -- f ) 9 * 5 / 32 + ;
/> 100 c>f . 37 c>f .
212 98
```

98 °F is 36.67 °C, and `/` throws the remainder away. To keep one more digit, work in tenths: multiply by 50, not
by 5, and read the answer 366 as 36.6.

## Lab 17. HyForth: Decisions, Loops and Programs

**1.** `-1`, `0`, `-1`, `-1`. True is -1 (every bit a 1), and false is 0.

```session
/> 5 3 > . 5 3 < . 7 7 = . 0 0= .
-1 0 -1 -1
```

**2.** `0<` uses up the number it tests, so `dup` makes a copy for it. That leaves the original on the stack, and
the `drop` at the end throws it away, to keep the promise of the diagram ( *n* -- ). Without the `drop`, every
call would leave a number behind.

```session
/> : sign ( n -- ) dup 0< if ." negative" else ." not negative" then drop cr ;
/> 5 sign
not negative
/> -5 sign
negative
/> 0 sign
not negative
```

**3.**

```session
/> : big? ( n -- ) 100 > if ." big" else ." small" then cr ;
/> 500 big?
big
/> 5 big?
small
```

**4.**

```session
/> : a 5 0 do i . loop ;
/> a
0 1 2 3 4
/> : b 11 1 do i . loop ;
/> b
1 2 3 4 5 6 7 8 9 10
/> : c 10 0 do i . 2 +loop ;
/> c
0 2 4 6 8
/> : d 3 0 do ." Hip! " loop ." Hooray!" ;
/> d
Hip! Hip! Hip! Hooray!
```

**5.**

```session
/> : stars ( n -- ) 0 ?do [char] * emit loop cr ;
/> : triangle ( n -- ) 1+ 1 do i stars loop ;
/> 4 triangle
*
**
***
****
```

**6.** `!` stores a number at an address; `@` fetches the number that's there; `+!` adds to it. Typing `score`
alone leaves the variable's *address* on the stack: a number saying where in memory it is.

```session
/> variable score
/> 0 score !
/> : hit ( -- ) 10 score +! ;
/> hit hit hit
/> score @ .
30
```

**7.** Yes. The file is on the SD card, and `include` reads it as if it were typed again.

**Questions.**

1. "Then" as in "and then, afterwards": it marks where the two paths join up again. Everything after `then` is
   done whichever way the `if` went.
2. The loop stops *before* its limit: `10 0 do` runs for 0 to 9, ten times. `0 0 do` runs 65,536 times, all the
   way round. `?do` skips the loop when the limit and the start are equal.
3. `do` only works while a definition is being compiled. It has to be inside a `:` ... `;`.
4. A variable's name gives its address, and it's read and written with `@` and `!`. A constant's name gives its
   value, which never changes. A value's name gives its value too, and `to` changes it.
5. Each small word can be tested alone (`6 say`), so when the whole thing is wrong, you know which part to
   suspect. And each has a name that says what it does, so the last word reads almost like a sentence.

**Challenge.** One way:

```session
/> : divides? ( a b -- flag ) mod 0= ;
/> : prime? ( n -- flag ) dup 2 ?do dup i divides? if drop false unloop exit then loop drop true ;
/> : primes ( n -- ) 1+ 2 do i prime? if i . then loop cr ;
/> 50 primes
2 3 5 7 11 13 17 19 23 29 31 37 41 43 47
```

Trying every divisor is slow: `1000 primes` takes a while, and beyond a few thousand it's a wait. Stopping at
the square root of *n*, and skipping the even numbers, each help a great deal.

## Lab 18. hylang: Expressions and Functions

**1.**

```session
hylang> (+ 1 2 3 4)
=> 10
hylang> (* 2 (+ 3 4))
=> 14
hylang> (- 10 (* 2 3))
=> 4
hylang> (/ 10 4)
=> 5/2
hylang> (> 5 3)
=> T
hylang> (> 3 5)
=> NIL
hylang> (+ "Hydra" "-" 16)
=> "Hydra-16"
```

**2.**

```session
hylang> (* (+ 4 5) 2)
=> 18
hylang> (- 100 (* 7 8))
=> 44
hylang> (/ (+ 12 4) (- 9 5))
=> 4
hylang> (+ 1/2 1/3 1/6)
=> 1
```

**3.** In `def`, the name mustn't be worked out: `def` wants the *name itself*, not its value, and braces mean
"leave this as it is". Afterwards, when the name is used, its value is exactly what's wanted, so it's written
bare.

```session
hylang> (def {age} 12)
=> NIL
hylang> (* age 365)
=> 4380
hylang> (def {name} "Sam")
=> NIL
hylang> (print "Hello," name)
Hello, Sam
=> NIL
```

**4.** The name is `square`. Its formal is `n`. Its body is `{* n n}`.

```session
hylang> (fun {square n} {* n n})
=> NIL
hylang> (square 9)
=> 81
hylang> (square (square 3))
=> 81
```

**5.** For example:

```session
hylang> (fun {double n} {* n 2})
=> NIL
hylang> (fun {cube n} {* n n n})
=> NIL
hylang> (fun {average a b} {/ (+ a b) 2})
=> NIL
hylang> (fun {greet who} {print (+ "Hello, " who)})
=> NIL
hylang> [(double 21) (cube 3) (average 3 4)]
=> {42 27 7/2}
hylang> (greet "Sam")
Hello, Sam
=> NIL
```

**6.**

<!-- session
(fun {square n} {* n n})
-->

```session
hylang> (squre 5)
=> Error: Unbound Symbol 'squre'
hylang> (square 1 2)
=> Error: The function takes 1 argument, not 2
hylang> (square)
=> <function>(fn {n} {* n n})
hylang> (+ 1 "a")
=> Error: '+' adds numbers, or joins values to a string
```

**7.** hylang shows a prompt of spaces, and waits. Typing the missing `)` finishes the expression.

**Questions.**

1. Take any number of arguments: `(+ 1 2 3 4)`. With the sign in the middle, as at school, it can only ever
   have two.
2. True, and false. `NIL` also means "nothing", and the empty list.
3. `{* n n}` is a list of three things, kept as it is: code for later. `(* n n)` is a call, worked out at once.
   A function's body is in braces so that it's worked out when the function is *called*, not when it's defined.
4. `(to-fixed (/ 10 4) 1)` gives 2.5, and `(truncate (/ 10 4))` gives 2.
5. Making a new function out of an old one by filling in some of its arguments now, and the rest later:

```session
hylang> (fun {add a b} {+ a b})
=> NIL
hylang> (def {add5} (add 5))
=> NIL
hylang> (add5 10)
=> 15
```

**Challenge.** hylang keeps the exact fraction, where HyForth could only give 36:

```session
hylang> (fun {celsius f} {/ (* (- f 32) 5) 9})
=> NIL
hylang> (celsius 98)
=> 110/3
hylang> (to-fixed (celsius 98) 1)
=> 36.6
hylang> (fun {hypotenuse a b} {sqrt (+ (* a a) (* b b))})
=> NIL
hylang> (hypotenuse 3 4)
=> 5
hylang> (hypotenuse 1 1)
=> 1.41421356237
```

## Lab 19. hylang: Lists, Decisions and Programs

**1.**

```session
hylang> (def {l} {5 3 8 1})
=> NIL
hylang> (len l)
=> 4
hylang> (fst l)
=> 5
hylang> (tail l)
=> {3 8 1}
hylang> (sort l)
=> {1 3 5 8}
hylang> (reverse l)
=> {1 8 3 5}
hylang> (sum l)
=> 17
hylang> (range 5)
=> {0 1 2 3 4}
hylang> (range 1 11)
=> {1 2 3 4 5 6 7 8 9 10}
```

**2.**

```session
hylang> (map (fn {n} {* n 2}) {1 2 3})
=> {2 4 6}
hylang> (filter (fn {n} {> n 2}) {1 2 3 4})
=> {3 4}
hylang> (sum (map (fn {n} {* n n}) (range 1 11)))
=> 385
```

**3.** For example:

```session
hylang> (sum (range 1 101))
=> 5050
hylang> (map (fn {n} {* n 2}) (range 1 11))
=> {2 4 6 8 10 12 14 16 18 20}
```

(`(range 2 21 2)` and `(filter even? (range 2 21))` give the same list.)

**4.**

```session
hylang> (fun {size n} {if (> n 100) "big" "small"})
=> NIL
hylang> (map size {5 500 50 5000})
=> {"small" "big" "small" "big"}
```

**5.**

```session
hylang> (dotimes {i 3} (print "Hip!"))
Hip!
Hip!
Hip!
=> NIL
hylang> (each {x {1 2 3}} (print (* x x)))
1
4
9
=> NIL
hylang> (fun {fact n} {if (< n 2) 1 (* n (fact (- n 1)))})
=> NIL
hylang> (fact 5)
=> 120
hylang> (fact 30)
=> 265252859812191058636308480000000
```

**6.** The hash's three names. `:name` is an **atom**: a name that stands for itself, used here as a key.

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (pet :name)
=> "Rex"
hylang> (hash-put pet {:colour "brown"})
=> NIL
hylang> (hash-keys pet)
=> {:name :legs :colour}
```

**7.** For example, in `lab19.hl`:

```text
; lab19.hl - sizes, and a factorial
(fun {size n} {if (> n 100) "big" "small"})
(fun {fact n} {if (< n 2) 1 (* n (fact (- n 1)))})
(print "10 factorial is" (fact 10) "which is" (size (fact 10)))
```

**Questions.**

1. `{+ 1 2}` is a list of three items, left alone. `(+ 1 2)` is a call, worth 3. `eval` takes a list and runs it
   as code: `(eval {+ 1 2})` is 3. So a program can build a piece of program, and then run it.
2. HyForth's numbers are 16-bit cells: nothing bigger than 65,535. hylang's whole numbers can be any size, and
   30 factorial has 33 digits.
3. Each call is for a smaller number, and when the number is below 2 the `if` gives 1 without calling `fact`
   again. Every recursive function needs a case like that, and every call must move towards it.
4. The loop: going through the list, calling the function on each item, and collecting the answers.
5. A value can be used again: printed, put in a list, tested, passed to `map`. Something that's only been
   printed is gone. And a function that prints can't be tested by a program, only by eye.

**Challenge.** One way:

```session
hylang> (fun {prime? n} {and (> n 1) (not (any? (fn {d} {== 0 (mod n d)}) (range 2 n)))})
=> NIL
hylang> (fun {primes n} {filter prime? (range 2 (+ n 1))})
=> NIL
hylang> (primes 30)
=> {2 3 5 7 11 13 17 19 23 29}
```

It's short, and it's slow: `(primes 100)` takes a good while, much longer than HyForth's. hylang's numbers can be
any size, and that costs time on every sum (the manual's section 25.6).

## Lab 20. HyBASIC: First Programs

**1.**

```session
> PRINT 2 + 3 * 4
 14
> PRINT (2 + 3) * 4
 20
> PRINT 10 / 4
 5/2
> PRINT 10 \ 4
 2
> PRINT 10 MOD 4
 2
> PRINT 2 ^ 10
 1024
> PRINT "Hy" + "dra"
Hydra
```

**2.** `;` puts the next item straight after the last (a number brings its own spaces); `,` moves to the next
column, 14 characters on. `AGE = AGE + 1` means: work out `AGE + 1`, and put the answer in `AGE`. It's a command,
"becomes", not a statement that two things are equal.

```session
> AGE = 12
> NAME$ = "Sam"
> PRINT NAME$; " is"; AGE
Sam is 12
> PRINT NAME$, AGE
Sam            12
> AGE = AGE + 1
> PRINT AGE
 13
```

**3.** Typing a line again replaces it, and a new number adds a line:

```session
> 10 FOR I = 1 TO 5
> 20 PRINT I; "times 7 is"; I * 7
> 30 NEXT I
> 10 FOR I = 1 TO 12
> 5 PRINT "The seven times table"
> LIST
5 PRINT "The seven times table"
10 FOR I = 1 TO 12
20 PRINT I; "times 7 is"; I * 7
30 NEXT I
```

**4.** `40 PRINT "Done."`, and something like `25 PRINT "-----"`. Any number from 21 to 29 will do. Numbering in
tens leaves room for nine new lines between any two.

**5.** The first `LIST` shows nothing: `NEW` threw the program away. The second shows it all, back from the file.

**6.** HyBASIC says `?Redo from start`, and asks again: `A` is a number variable, and a word isn't a number.

**7.** For example:

```text
30 IF A < 13 THEN
32   PRINT "Hello, young "; N$
34 ELSEIF A < 20 THEN
36   PRINT "Hello, "; N$
38 ELSE
39   PRINT "Good day, "; N$
40 END IF
45 PRINT "Next year you'll be"; A + 1
```

**Questions.**

1. With a number, the line is kept as part of the program, to be done when the program is `RUN`. Without, it's
   done at once, and not kept.
2. That the variable holds a string (text), not a number.
3. It's 0 (or an empty string). So a mistyped name isn't reported: it's quietly a *new* variable, worth 0, and
   the program runs on with the wrong answer.
4. `INPUT`, then the sum, then `PRINT`. BASIC does a program's lines in order, from the top, one at a time: a
   value has to be fetched before it's used, and worked out before it's printed.
5. `NEW` throws the program away. `LIST` shows it. `RUN` runs it. `SAVE` keeps it in a file, and `LOAD` brings it
   back from one.

**Challenge.** One way, as a file:

```text
DO
  INPUT "Which table"; T
  FOR I = 1 TO 12
    PRINT USING "###"; I;
    PRINT " times"; T; "is";
    PRINT USING "#####"; I * T
  NEXT I
  INPUT "Another (y or n)"; A$
LOOP WHILE A$ = "y"
```

## Lab 21. HyBASIC: Bigger Programs

**1.** A `FOR` loop needs to know how many times to go round, and here that's the very thing the program is
finding out. A `DO` loop goes on until its test says stop.

<!-- session
cd /ram
!file samples/double.bas
-->

```session
/ram> basic double.bas
 20 doublings make 1048576
```

**2.**

```session
> A$ = "Hydra-16"
> PRINT LEN(A$)
 8
> PRINT LEFT$(A$, 5)
Hydra
> PRINT RIGHT$(A$, 2)
16
> PRINT MID$(A$, 2, 3)
ydr
> PRINT UCASE$(A$)
HYDRA-16
> PRINT INSTR(A$, "-")
 6
```

**3.** One way:

```session Word?_
> 10 INPUT "Word"; W$
> 20 FOR I = LEN(W$) TO 1 STEP -1
> 30   PRINT MID$(W$, I, 1);
> 40 NEXT I
> 50 PRINT
> RUN
Word? Hydra
ardyH
```

**4.** `38/5`: exact, where a calculator would say 7.6. For a decimal, change the `PRINT` to
`PRINT "The average is"; FIXED(TOTAL / 5, 2)`.

<!-- session
cd /ram
!file samples/marks.bas
-->

```session
/ram> basic marks.bas
The average is 38/5
```

**5.** For example, before the `DATA` line:

```text
HIGH = MARK(1) : LOW = MARK(1)
FOR I = 2 TO 5
  IF MARK(I) > HIGH THEN HIGH = MARK(I)
  IF MARK(I) < LOW THEN LOW = MARK(I)
NEXT I
PRINT "The highest is"; HIGH; "and the lowest is"; LOW
```

**6.** `ROWS` and `COLS` are the `SUB`'s **parameters**: its own names for the two values it's given. In the
first call they're 3 and 5; in the second, 2 and 10.

<!-- session
cd /ram
!file samples/box.bas
-->

```session
/ram> basic box.bas
*****
*****
*****

**********
**********

```

**7.** Add the function, and a `PRINT` after each `BOX`:

```text
BOX 3, 5
PRINT "Area:"; AREA(3, 5)

FUNCTION AREA (ROWS, COLS)
  AREA = ROWS * COLS
END FUNCTION
```

**Questions.**

1. `FOR`: when the number of times is known before the loop starts. `DO WHILE`: when it isn't, and the loop might
   need to run no times at all. `DO` ... `LOOP UNTIL`: when it isn't, and the loop must run at least once (asking
   a question until the answer is acceptable).
2. Six: `MARK(0)` to `MARK(5)`.
3. A `FUNCTION` gives back a value, and is used inside an expression: `PRINT AREA(3, 5)`. A `SUB` does something,
   and is used as a statement: `BOX 3, 5`.
4. Two procedures can each use `I`, or `TOTAL`, without spoiling the other's. Whoever writes one doesn't need to
   know the names used in the rest of the program.
5. So that people can see at a glance which lines belong to which loop or procedure. HyBASIC doesn't care.

**Challenge.** Project 1 is this challenge, grown up: Topic 15.

## Lab 22. Your Language Meets the Hydra

There are many right answers to each job. These are one for each language, the first three and the sixth as
the Hydra ran them.

**In HyForth.**

```session
/> s" date" sh-out type
2000-01-01 00:00:04
/> s" /ram/log.txt" w/o create-file throw constant f
/> s" one" f write-line throw s" two" f write-line throw s" three" f write-line throw
/> f close-file throw
/> variable g
/> : show ( -- ) 1 begin pad 80 g @ read-line throw while over . pad swap type cr 1+ repeat 2drop ;
/> s" /ram/log.txt" r/o open-file throw g !
/> show
1 one
2 two
3 three
/> g @ close-file throw
/> lib facility
/> : ready ( -- ) ." Ready..." cr 2000 ms ." Go!" cr ;
/> ready
Ready...
Go!
```

```text
lib facility
: name ( -- ) page 34 12 at-xy yellow color ." Sam" plain cr ;

lib sound
: ceg ( -- ) 0 0 snd-patch  0 60 snd-note 400 ms  0 64 snd-note 400 ms  0 67 snd-note 400 ms  0 snd-off ;

lib video
320 8 bitmap  12 pen  60 40 260 200 box  160 120 60 circle
```

**In hylang.**

```session
hylang> (sh-out "date")
=> "2000-01-01 00:00:05\n"
hylang> (write-file "/ram/log.txt" "one\ntwo\nthree")
=> NIL
hylang> (def {n} 0)
=> NIL
hylang> (each {l (read-lines "/ram/log.txt")} (do (set! {n} (+ n 1)) (print n l)))
1 one
2 two
3 three
=> NIL
hylang> (do (print "Ready...") (sleep 2) (print "Go!"))
Ready...
Go!
=> NIL
```

```text
(use "screen")
(do (cls) (at 34 12) (color :yellow) (print "Sam") (plain))

(use "snd")
(tune {{"C4" 1} {"E4" 1} {"G4" 1}})

(use "video")
(do (bitmap 320 8) (pen 12) (box 60 40 260 200) (circle 160 120 60))
```

**In HyBASIC.**

```session
> SHELL "date"
2000-01-01 00:00:05
> OPEN "/ram/log.txt" FOR OUTPUT AS #1
> PRINT #1, "one" : PRINT #1, "two" : PRINT #1, "three"
> CLOSE #1
> 10 OPEN "/ram/log.txt" FOR INPUT AS #1
> 20 N = 0
> 30 DO UNTIL EOF(1)
> 40   LINE INPUT #1, L$
> 50   N = N + 1
> 60   PRINT N; L$
> 70 LOOP
> 80 CLOSE #1
> 90 PRINT "Ready..." : SLEEP 2 : PRINT "Go!"
> RUN
 1 one
 2 two
 3 three
Ready...
Go!
```

```text
CLS : LOCATE 12, 34 : COLOR 14 : PRINT "Sam"

PLAY "o4 l4 c e g"

SCREEN 13 : LINE (60, 40)-(260, 200), 12, B : CIRCLE (160, 120), 60, 14
```

**Questions.**

1. The student's own view, with a reason. Typically: HyBASIC has a statement for nearly everything; hylang has
   the shortest lines for files and lists; HyForth makes you build the most yourself.
2. HyForth needs a library for nearly everything here (`facility`, `sound`, `video`, loaded with `lib`); hylang
   for the screen, the sound and the video (loaded with `use`); HyBASIC for nothing: it's all built in.
3. Yes: all three can open `/dev/sndctl` or `/dev/vid/draw` and write the same lines of text that `echo` sent.
   The libraries are a convenience. Underneath, they do exactly that.
4. Each language reports it in its own way, and each lets a program catch it: HyForth's file words leave an
   error code (and `throw` and `catch` deal with it); hylang gives an error value, and `try` catches it; HyBASIC
   stops with a message, unless `ON ERROR GOTO` is set.

## Lab 23. Three Languages

**2.** Guess the middle of what's left, every time: 50, then 25 or 75, and so on. Each guess halves the
possibilities, and 100 halved seven times is less than 1 (2 to the power 7 is 128, which is more than 100). So
seven guesses are always enough.

**3.** In HyBASIC, for example: change the loop's last line, and add after it:

```text
LOOP UNTIL GUESS = SECRET OR TRIES = 7
IF GUESS = SECRET THEN
  PRINT "Right! You took"; TRIES; "guesses."
ELSE
  PRINT "No more guesses. It was"; SECRET
END IF
```

**4.** From the manual's section 25.5, whichever two the student didn't type.

**5 and 6.** On the manual's Hydra, counting to 10,000 takes HyForth 40 ticks (a fifth of a second), hylang about
2.4 seconds, and HyBASIC about 2.7. Counting to 40,000 takes each about four times as long. The surprise is
HyForth's answer:

```session
/> : count-up ( n -- count ) 0 swap 0 do 1+ loop ;
/> 40000 count-up .
-25536
/> 40000 count-up u.
40000
```

40,000 doesn't fit in a signed 16-bit cell, whose largest value is 32,767. The count is right, bit for bit, but
`.` reads the top bit as a minus sign. `u.` prints the same cell as an unsigned number.

**Questions.**

1. `play` calls itself: when the guess is wrong, the function's last act is to call `play` again. That's
   recursion, doing a loop's job.
2. `INPUT` prints the question, reads the line, and turns it into a number, all in one statement. In hylang,
   `read-line` reads and `val` converts. In HyForth, `accept` reads the characters, and `>number` converts them,
   and the program has to manage the buffer and the stack between the two.
3. HyForth's numbers are 16-bit cells, which the 65C02 adds in a few instructions. hylang and HyBASIC use numbers
   of any size, checked and handled by a library, at every sum. What's given up: big numbers, fractions and
   decimals (without a library), and a good deal of convenience.
4. That for most programs, the hours a person spends writing and correcting them matter far more than the
   seconds the computer spends running them. Choose the language that gets you a *correct* program soonest, and
   look for speed only where it's really needed.
5. A quiz: HyBASIC (strings, `INPUT` and `DATA` make it easy, and anyone can read it). The 200th Fibonacci number:
   hylang or HyBASIC (the number has 42 digits, far beyond HyForth's cells). A light a thousand times a second:
   HyForth, or machine code: the others can't get round a loop that fast.

**Challenge.** One way, in HyBASIC:

```session s,_b_or_r?_
> 10 LO = 1 : HI = 100
> 20 DO
> 30   G = (LO + HI) \ 2
> 40   PRINT "Is it"; G
> 50   INPUT "s, b or r"; A$
> 60   IF A$ = "s" THEN LO = G + 1
> 70   IF A$ = "b" THEN HI = G - 1
> 80 LOOP UNTIL A$ = "r"
> 90 PRINT "I knew it."
> RUN
Is it 50
s, b or r? s
Is it 75
s, b or r? b
Is it 62
s, b or r? s
Is it 68
s, b or r? r
I knew it.
```
