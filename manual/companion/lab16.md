# Lab 16. HyForth: The Stack

> **THIS LAB:** Manual: Chapter 19, sections 19.1 to 19.6. About an hour. You need a Hydra at its `/>` prompt:
> that prompt *is* HyForth.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Be the computer. For each line, write what you think the stack holds afterwards (the top on the right), then
   type the line, and then `.s` to see. Clear the stack between lines with as many `drop`s as it takes.

| The line | I think the stack is | `.s` says |
|---|---|---|
| `1 2 3` | | |
| `1 2 3 swap` | | |
| `1 2 3 rot` | | |
| `1 2 over` | | |
| `5 dup *` | | |
| `10 2 3 + *` | | |
| `10 2 * 3 +` | | |

2. Turn these sums into Forth, and get the Hydra to print the answers. ✎ Write the line you typed.

| The sum | In Forth | The answer |
|---|---|---|
| 4 + 5 | | |
| (4 + 5) × 2 | | |
| 100 − (7 × 8) | | |
| (12 + 4) ÷ (9 − 5) | | |
| The remainder when 100 is divided by 7 | | |

3. Define a word, and use it:

```text
/> : square ( n -- n*n ) dup * ;
/> 9 square .
/> see square
```

   ✎ What does `( n -- n*n )` mean? Does HyForth do anything with it?

<div class="write l2"></div>

4. ✎ Now write your own. Test each one with at least two numbers, and write the definition here.

| The word | What it does | My definition |
|---|---|---|
| `double` | ( *n* -- *2n* ) | |
| `cube` | ( *n* -- *n³* ) | |
| `average` | ( *a b* -- *the number half way between* ) | |
| `hello` | ( -- ) prints a greeting | |

5. Start a definition, and don't finish it: type `: oops 1 2` and press <kbd>RETURN</kbd>. ✎ What happened to the
   prompt? How do you get out?

<div class="write"></div>

6. Type `drop` when the stack is empty. Then type `1 0 /`. ✎ What does HyForth say each time?

<div class="write"></div>

### Questions

1. ✎ What is the stack? Why is it called that?

<div class="write l2"></div>

2. ✎ In Forth, `2 3 +` adds, where school writes 2 + 3. What's the advantage of putting the `+` last?

<div class="write l2"></div>

3. ✎ What's the difference between `.` and `.s`?

<div class="write"></div>

4. ✎ What goes wrong with `: double 2*;`? (Look closely.)

<div class="write"></div>

5. ✎ A number on Forth's stack is a 16-bit cell. What's the biggest number it holds? What does `32767 1 + .`
   print, and why?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | say what the stack holds after a line of Forth |
| ☐ | turn a sum into Forth |
| ☐ | use `dup`, `drop`, `swap`, `over` and `rot` |
| ☐ | define a word of my own, with a stack diagram |
| ☐ | get out of an unfinished definition |

> **CHALLENGE:** ★ Write `f>c` ( *fahrenheit* -- *celsius* ), which turns a temperature in Fahrenheit into
> Celsius: take away 32, multiply by 5, divide by 9. Check it with 212 (100) and 32 (0). Then write `c>f`. What
> happens with 98? Why isn't the answer exact, and how could you get one more digit out of it?
