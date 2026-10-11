# 6. The languages

The Hydra has three languages of its own, each a module of the paged ROM, each a shell too, and each with the whole
system a word away:

| Language | Command | What it is | Best for |
| :------- | :------ | :--------- | :------- |
| [**HyForth**](hyforth.md) | `forth` | Forth 2012, subroutine threaded, its word sets as libraries; the login shell | Speed, poking at hardware, small tools, interactive exploration |
| [**hylang**](hylang.md) | `hylang` | danlang, a lisp with integers of any size, exact fractions, hashes and closures; a bytecode machine and native code | Programs that work with lists, text and structure; scripting the system |
| [**HyBASIC**](hybasic.md) | `basic` | A structured BASIC in QuickBASIC's way: labels, `SUB`s and `FUNCTION`s, records; compiled to a stack machine | Programs that read like the classic 8-bit ones; graphics, sound, and learning |

All three share:

* **The numbers**: one number system, exact, of any size, in any base ([below](#the-numbers-they-share)).
* **The system**: every system call is a word or a function in each (`sys-open`, `(sys-open ...)`, `SYS "OPEN"`), and
  the devices are files each can open.
* **The shell rule**: as a login shell (`forth -l`, `hylang -l`, `basic -l`), a line that's the language's is run as
  the language, and any other line goes to rc ([the shell](../05-rc.md#the-shells-on-the-hydra)).
* **The devices' libraries**: the same names for GPIO, I2C, SPI, sound, video, the windows and the tasks in HyForth and
  hylang, and C's `hydra.h`, `snd.h` and `vera.h` follow them.

C and assembly are the [programming guide](../09-programming/README.md)'s.

## Contents

1. [HyForth](hyforth.md)
2. [hylang](hylang.md)
3. [HyBASIC](hybasic.md)

---

## The numbers they share

The Hydra has **one number system**, and every language has all of it:

* **Integers of any size**, to 255 bytes (614 digits): `2 ^ 100` is all 31 of its digits.
* **Fixed decimals**, exact: `0.1 + 0.2` is `0.3`, exactly.
* **Rationals**: `10 / 4` is `5/2`, in lowest terms.
* **Complex numbers**: `1+2i`; the square root of -4 is `2i`.
* **The math functions** (square root, exponential, logarithm, sine, cosine, tangent, arctangent, powers): exact when
  their answer is (`sqrt 9/4` is `3/2`), else correctly rounded to a precision, 12 significant digits at the start, up
  to 100.
* **Any base**, for reading and writing: `#xFF`, `#b0.1` (a half), `#c+-0` (balanced ternary), `#16r1F`; a base set
  once is followed everywhere numbers are read and written.

| Language | Its numbers |
| :------- | :---------- |
| hylang | Every number is one: `(/ 10 4)` is `5/2`, `(sqrt 2)`, `(base "x")` |
| HyForth | `lib numbers`: a number stack of its own (`n+`, `n.`, `nsqrt`, `nformat`); a number in the text that isn't a cell goes on it |
| HyBASIC | Every number is one, of one type: `PRINT 1 / 3` is `1/3`, `BASE "x"`, `DIGITS 30` |
| C | `num.h`: a function for each operation, and `printf`'s `%N` |
| Assembly | `numbers.inc`'s `NUMCALL` and `MATHCALL` |
| rc | `calc`: `calc 2/3 + 0.5` is `7/6`, `calc -b x 255` is `FF` |

They're all **the same code**: two library modules of the paged ROM, `numbers` and `math`, working on **the same stored
format**, so a number one language writes to a file is the same number when another reads it.  [Libraries](../09-programming/libraries.md#the-number-libraries)
describes the format and how to call them.  A language keeps its quickest numbers itself (hylang's 15-bit fixnums,
HyBASIC's 32-bit integers, HyForth's cells) and hands them to the libraries past that.

---

## How fast

Twenty benchmarks are written in each language (`/sd/0/bench` on HydraOS's card: `bench.fs`, `bench.hl`, `bench.bas`),
the same algorithms, sizes and results, each in the language's own way.  `node sim/bench.js` runs them in the emulator.
At 3.58 MHz, in October 2026, the time for all twenty, and the geometric means of the ratios:

| | HyForth | hylang | HyBASIC |
| :- | ---: | ---: | ---: |
| All twenty | 9.6 s | 17.7 s | 55.1 s |
| Against HyForth | 1x | 1.9x | 7.4x |
| Against hylang | | 1x | 3.2x |

By kind, HyBASIC's time against HyForth's: calls 14.9x, loops 10.1x, arithmetic 4.5x, arrays 6.2x, lists 5.8x, text
4.4x.  HyBASIC comes nearest where a statement does much (a number's text, a matrix), and is farthest where a step is
small (an empty `FOR` loop, a `FUNCTION`'s call).  For comparison, the Hydra's first BASIC (Microsoft's, by way of
EhyBASIC) took some 44 times HyForth's time.
