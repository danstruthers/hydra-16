# Project 7. Faster, with Machine Code

> **THIS PROJECT:** HyBASIC with 65C02 assembly language inside it. After Labs 21, 24 and 25. Two sittings. You'll
> write the slow part of a program in the processor's own language, and measure what that buys.

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The starting point

HyBASIC can hold assembly language between `ASM` and `END ASM` (section 27.20). This program counts to ten
thousand twice: once in HyBASIC, and once in machine code. Type it into `/sd/0/work/fast.bas`.

<!-- listing samples/fast.bas -->

```text
' fast.bas - the same job in HyBASIC and in machine code: count to 10,000
T = TIMER
C = 0
FOR I = 1 TO 10000 : C = C + 1 : NEXT
PRINT "HyBASIC counted to"; C; "in"; TIMER - T; "seconds"

ASM
count:  ldx #0          ; X and Y together: a 16-bit count
        ldy #0
again:  inx             ; Add 1 to the low byte ...
        bne skip
        iny             ; ... and when it goes round to 0, to the high byte
skip:   cpx #$10        ; 10,000 is $2710: stop when X is $10 and Y is $27
        bne again
        cpy #$27
        bne again
        rts
END ASM

T = TIMER
CALL ASM count
RREG A, X, Y, P
PRINT "Machine code counted to"; Y * 256 + X; "in"; TIMER - T; "seconds"
```

<!-- session
cd /ram
!file samples/fast.bas
-->

```session
/ram> basic fast.bas
HyBASIC counted to 10000 in 2.73 seconds
Machine code counted to 10000 in 0.055 seconds
```

### Understand it first

1. ✎ How many times faster is the machine code, on your Hydra?

<div class="write"></div>

2. ✎ The 65C02's registers hold 8 bits, and 10,000 doesn't fit in 8 bits. How does the routine count that high?
   What are X and Y, together?

<div class="write l2"></div>

3. ✎ Run the routine by hand for its first three times round. When does `iny` first happen?

<div class="write l2"></div>

4. ✎ Why is $2710 the number to stop at? How does the routine test for it? Why does it check X *and* Y?

<div class="write l2"></div>

5. ✎ What does `RREG A, X, Y, P` do? How does the HyBASIC line after it turn X and Y back into one number?

<div class="write l2"></div>

### Make it yours

| ☐ | Change |
|---|---|
| ☐ | Count to 50,000 in both. Predict both times first |
| ☐ | Pass the number to count to *into* the routine: `CALL ASM count, 0, LOW, HIGH` puts values in X and Y |
| ☐ | Write a routine that prints a row of *X* stars (section 27.20 has one), and use it to draw a triangle |
| ☐ | Write a routine that adds up the numbers 1 to *X* in A, and check it against HyBASIC for X = 10 |
| ☐ | Find the biggest X that routine gets right. What goes wrong after that, and which flag would tell you? |

✎ Write one of your routines here, with a comment on every line.

<div class="write l8"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | Every routine has been run by hand, on paper, for a small case |
| ☐ | Both versions give the same answer, and you've timed them |
| ☐ | You can say what each register is used for in your routine |

> **CHALLENGE:** ★ The race in section 25.6 gave HyForth about a fifth of a second for this job, and machine code
> is faster still. But HyBASIC took seconds to *write*, and the routine took much longer. For each of these, say
> which you'd use, and why: a loop that runs once when a program starts; a loop that draws every dot of a
> picture; a program you'll run once and throw away.
