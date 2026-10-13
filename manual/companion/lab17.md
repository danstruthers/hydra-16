# Lab 17. HyForth: Decisions, Loops and Programs

> **THIS LAB:** Manual: Chapter 19, sections 19.7 to 19.17. About an hour. You need a Hydra, and Lab 16 done.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. A comparison leaves a **flag**. ✎ What does each print?

| The line | It prints | So the flag for "true" is |
|---|---|---|
| `5 3 > .` | | |
| `5 3 < .` | | |
| `7 7 = .` | | |
| `0 0= .` | | |

2. `if` uses a flag. Type this, and try it with 5, with -5 and with 0:

```text
/> : sign ( n -- ) dup 0< if ." negative" else ." not negative" then drop cr ;
```

   ✎ Why is the `dup` there? What would happen without the `drop`?

<div class="write l2"></div>

3. ✎ Write `big?` ( *n* -- ), which prints "big" for a number over 100, and "small" otherwise.

<div class="write l2"></div>

4. A counted loop: `do` ... `loop`. ✎ What does each of these print? Guess first.

| The definition, and the line | I guess | It prints |
|---|---|---|
| `: a 5 0 do i . loop ;` then `a` | | |
| `: b 11 1 do i . loop ;` then `b` | | |
| `: c 10 0 do i . 2 +loop ;` then `c` | | |
| `: d 3 0 do ." Hip! " loop ." Hooray!" ;` then `d` | | |

5. ✎ Write `stars` ( *n* -- ), which prints that many `*` on one line. (`[char] * emit` prints one.) Then use it
   to write `triangle` ( *n* -- ), which prints a triangle *n* rows high.

<div class="write l4"></div>

6. A variable keeps a number between words:

```text
/> variable score
/> 0 score !
/> : hit ( -- ) 10 score +! ;
/> hit hit hit
/> score @ .
```

   ✎ What do `!`, `@` and `+!` do? What's on the stack after you type just `score`?

<div class="write l2"></div>

7. Put your words from this lab in a file, `/sd/0/work/lab17.fs`, with `edit`. Then leave them behind: type `bye`
   in window 0 (a new shell starts), and bring them back with `include /sd/0/work/lab17.fs`. ✎ Did they come back?

<div class="write"></div>

### Questions

1. ✎ `if` ... `then`: in English, "then" comes before what you do. What does `then` mean in Forth?

<div class="write"></div>

2. ✎ `10 0 do` counts from 0 to 9. Why not to 10? What would `0 0 do` do, and what's the cure?

<div class="write l2"></div>

3. ✎ Typed at the prompt, `5 0 do i . loop` gives `do: compile only`. What does that mean?

<div class="write"></div>

4. ✎ What's the difference between a `variable`, a `constant` and a `value`?

<div class="write l2"></div>

5. ✎ FizzBuzz (section 19.17) is built from three small words and one loop. Why is that better than one long
   definition?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | compare numbers, and use the flag in an `if` |
| ☐ | write a `do` loop, and use `i` |
| ☐ | keep a value in a variable, and change it |
| ☐ | keep my words in a file, and `include` it |
| ☐ | test a small word by itself before using it in a bigger one |

> **CHALLENGE:** ★ Write `primes` ( *n* -- ), which prints the prime numbers up to *n*. Build it from small words:
> first `divides?` ( *a b* -- *flag* ), true if *b* goes into *a*; then `prime?` ( *n* -- *flag* ), which tries
> every number from 2 up; then the loop. How far can it go before it gets slow?
