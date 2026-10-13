# Lab 15. Numbers and Bases

> **THIS LAB:** Manual: Chapter 18, sections 18.1 to 18.4, and 18.7. About 45 minutes. You need a Hydra. `calc`
> does the sums: put each sum in single quotes, so the shell leaves its signs alone.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. First, guess. Then ask the Hydra, and write what it really says.

| The sum | My guess | The Hydra says |
|---|---|---|
| `calc '2^10'` | | |
| `calc '2^64'` | | |
| `calc '10 / 4'` | | |
| `calc '1/3 + 1/6'` | | |
| `calc '0.1 + 0.2'` | | |
| `calc '1/3 * 3'` | | |
| `calc 'sqrt 16'` | | |
| `calc 'sqrt 2'` | | |
| `calc 'sqrt(-4)'` | | |

2. ✎ Most calculators say that 10 ÷ 4 is 2.5, and many computers say that 0.1 + 0.2 is 0.30000000000000004. What
   does the Hydra do differently?

<div class="write l2"></div>

3. `calc -d` sets how many digits the functions work to. ✎ Write the square root of 2 to 30 digits.

<div class="write"></div>

4. `calc -b` writes the answer in another base. Fill in the table.

| Decimal | In binary (`-b b`) | In hex (`-b x`) | In octal (`-b o`) |
|---|---|---|---|
| 10 | | | |
| 100 | | | |
| 255 | | | |
| 1000 | | | |

5. A number can *say* its own base, with `#` and a letter. ✎ What do these come to?

| The sum | The answer |
|---|---|
| `calc '#b1010'` | |
| `calc '#xFF'` | |
| `calc '#b1010 + #x0F'` | |
| `calc -b x '#xFF + 1'` | |

6. Bases don't stop at 16. ✎ Try `calc -b t 10` and `calc -b z 1000000`. What are these two bases, and what
   digits does each use? (Section 18.4.)

<div class="write l2"></div>

### Questions

1. ✎ The Hydra has four kinds of number. Name them, and give an example of each.

<div class="write l3"></div>

2. ✎ Why does 1/3 × 3 come to exactly 1 on the Hydra, when on many calculators it comes to 0.9999999?

<div class="write l2"></div>

3. ✎ In base 2 there are two digits, and in base 16 sixteen. How many digits does base 8 need? What's the biggest
   digit in base 8?

<div class="write"></div>

4. ✎ Programmers like hex because it lines up with binary. Show how: write $A5 in binary, a hex digit at a time.

<div class="write"></div>

5. ✎ What does `calc 1/0` say? What should a program do when that can happen?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | tell a whole number, a fraction, a decimal and a complex number apart |
| ☐ | get an exact answer, or a decimal one, as I choose |
| ☐ | write a number in binary, octal and hex, and read one back |
| ☐ | set the precision of a function like `sqrt` |

> **CHALLENGE:** ★ How many digits has 2 to the power 1000? (`calc '2^1000'` will show you the number. Counting
> them by eye isn't the best way: `wc -c` counts characters.) Then read about **balanced ternary** in section
> 18.4, where the digits are minus one, zero and plus one. Write 5, 6 and 7 in it by hand, and check with
> `calc -b c`.
