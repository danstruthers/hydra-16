# Lab 25. Your First Machine Code

> **THIS LAB:** Manual: Chapter 27, sections 27.1 to 27.9, and 27.18. About an hour. You need a Hydra, and Lab 24
> done. Work in `/ram` while you experiment, and copy what you want to keep to `/sd/0/work`.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Type in `stars.s` from section 27.1, with `edit`. Assemble it, and run it:

```text
/ram> as stars.s
/ram> ./stars
```

   ✎ What did `as` print? What does that mean?

<div class="write"></div>

2. ✎ Find each of these in your `stars.s`, and copy the line.

| Find | The line |
|---|---|
| A comment | |
| A constant being made | |
| A variable being made | |
| A label | |
| A system call | |
| The instruction that ends the program | |

3. Change `ROWS` to 8, assemble, and run. Then change the `'*'` to another character. ✎ How many lines did you
   have to change each time? Why is `ROWS` a constant, and not just a 5 in the code?

<div class="write l2"></div>

4. Break it on purpose, one mistake at a time, and see what `as` says. (Put each one right before the next.)

| The mistake | What `as` said |
|---|---|
| Spell `lda` as `lad` | |
| Take the `#` off `lda #1` | (It assembles! What does the program do now, and why?) |
| Jump to a label that isn't there: `jsr lime` | |
| Leave out `.include "hydra.inc"` | |

5. Ask for a labels file, and look at it:

```text
/ram> as -l stars.s
/ram> cat stars.lbl
```

   ✎ At what address does `main` start? And `line`? Why isn't `main` at $0800, where a program starts?

<div class="write l2"></div>

6. ✎ How big is the program `stars`, in bytes (`ls -l`)? How big is its source, `stars.s`? What's in the source
   that isn't in the program?

<div class="write l2"></div>

7. Make it your own: change `stars.s` so that the triangle points the other way, with the longest row first.
   ✎ Write the lines you changed.

<div class="write l5"></div>

### Questions

1. ✎ What does an assembler do? What goes in, and what comes out?

<div class="write l2"></div>

2. ✎ Why does `line` keep its count in the variable `count`, and not in the X register?

<div class="write l2"></div>

3. ✎ `row` and `count` are on the zero page. How does the source say so? Where exactly do they end up? (Your
   labels file knows.)

<div class="write l2"></div>

4. ✎ What's a system call? Which one does `stars` use, and what does it need in which register?

<div class="write l2"></div>

5. ✎ `lda #1` and `lda 1` both assemble. Why is the second one almost always a bug?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | type in a source file, assemble it and run it |
| ☐ | read `as`'s error messages, and fix what they point at |
| ☐ | make and read a labels file |
| ☐ | use constants, variables, labels and a subroutine |
| ☐ | call HydraOS to print a character |

> **CHALLENGE:** ★ Write `count.s`, which prints the digits 0 to 9 on one line. (A digit's character code is $30
> plus the digit.) Then make it print 00 to 99, ten to a line. You'll want a subroutine that prints one digit,
> and two counters.
