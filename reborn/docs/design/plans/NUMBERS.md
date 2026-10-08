## **Numbers: one number system, one format and one library for every language**

A plan (October 2026) for the user's request: BASIC on hylang's number system instead of Microsoft's 40-bit floating point, and the same numbers in HyForth.  The simplest way to give every language the same numbers is the one the user named: **shared libraries** in the paged ROM that every language calls (`numbers`, the number system, and `math`, its functions), and **a new compact binary format** that every language stores numbers in and passes them in.  BASIC is written again on top of them: a new BASIC for the Hydra, QuickBASIC's kind, line numbers optional and labels to branch to, as the user asked ([BASIC.md](BASIC.md), its plan).  And every language gets all of it, as the user asked: all of danlang's and hylang's number system, its 19 named bases and every other way of writing a number included; numbers written in any of those formats; **a current base**, set by the program, that numbers are shown in and read in; fractions and fixed decimals in every format; and behind the scenes, every number stored in the new compact format, whatever format it was written or is shown in.

### **Contents**
1. [What each language has now](#what-each-language-has-now)
2. [The target](#the-target)
3. [All of it, in every language](#all-of-it-in-every-language)
4. [The base: what numbers are shown and read in](#the-base-what-numbers-are-shown-and-read-in)
5. [The format numbers are stored in](#the-format-numbers-are-stored-in)
6. [The libraries](#the-libraries)
7. [The math functions](#the-math-functions)
8. [hylang and danlang](#hylang-and-danlang)
9. [BASIC](#basic)
10. [HyForth](#hyforth)
11. [C and assembly](#c-and-assembly)
12. [Tests](#tests)
13. [The order of work](#the-order-of-work)
14. [Risks](#risks)
15. [Questions](#questions)
16. [Answers](#answers-the-users-7-october-2026)
17. [As built: step 1](#as-built-step-1)
18. [As built: step 2](#as-built-step-2)
19. [As built: step 3](#as-built-step-3)
20. [As built: step 4](#as-built-step-4)
21. [As built: step 5](#as-built-step-5)

---

### **What each language has now**

| Language | Numbers | Functions | Where the code is |
| :------- | :------ | :-------- | :---------------- |
| **BASIC** (`modules/basic`) | Microsoft's 40-bit binary floating point: 5 bytes (an exponent and a 32-bit mantissa), about 9 digits, 1E-38 to 1E38; `0.1` isn't exact, so `0.1 + 0.2 = 0.3` is false; `%` variables are 16-bit integers | `SQR`, `EXP`, `LOG`, `SIN`, `COS`, `TAN`, `ATN`, `^`, `INT`, `ABS`, `SGN`, `RND` (polynomial approximations) | `float.inc`, `trig.inc`, `rnd.inc` (1,767 lines), all through one accumulator (`FAC`, `ARG`) in the zero page |
| **hylang** (`modules/hylang`) | danlang's tower, exact: integers of any size (to 255 bytes, about 614 digits), fixed decimals (digits and places), rationals (in lowest terms), complex numbers; `(+ 0.1 0.2)` is `0.3`; `(/ 1 3)` is `1/3` | `+ - * /` (exact), `abs`, `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `val`, `random`, `fib`, `pow` (whole powers only), the bits; every base, to read and to write | `numreg.inc`, `numval.inc`, `numtext.inc`, `numbi.inc`, `numbits.inc` (5,500 lines), most of its third bank (12K) |
| **HyForth** (`modules/forth`) | 16-bit cells and 32-bit doubles (`double.fl`); no floating point (the Floating-Point word set was left out, October 2026) | None | |
| **C** (`sdk/c`) | cc65's `int` and `long` (16 and 32 bits); no `float` | None | |
| **danlang** (the reference, C#) | The same tower as hylang (`Numbers.cs`, `NumberParser.cs`) | As hylang: no square root, logarithm, exponential or trigonometry | |

So two languages have no numbers past integers, BASIC's numbers are inexact and short, and no language has `sqrt`, `log` or `sin` on exact numbers.

### **The target**

* **One number system, all of it**: danlang's and hylang's tower (integers of any size, fixed decimals, rationals, complex numbers), every way of reading and writing a number (the 19 named bases, any radix to 80, digits of one's own, balanced, least digit first, negative bases), every number built-in and the bits, in every language; with the tower's rules: a sum, difference or product is complex if either is, else rational if either is, else fixed if either is, else an integer; a quotient is exact; comparison is by value, whatever the kinds.
* **Math functions on it**: `sqrt`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, real powers, `pi`: exact where the answer is (`sqrt` of 9/4 is 3/2), else a fixed decimal to a precision the program sets.
* **A current base** in each language, the program's to set: any of danlang's (a named base, a radix, digits of its own, balanced, least digit first ...), used to show numbers and to read them; fractions and fixed decimals in it as well as integers.
* **One stored format**: a compact binary form of every number, the same in hylang's heap, in BASIC's variables, on HyForth's number stack, in a C program's buffers, in files, and between programs.  A number is stored the same whatever format it was written in or is shown in: the formats are text, the stored form is one.
* **Shared libraries**: `numbers` (the arithmetic, conversions, text in every base, bits) and `math` (the functions), modules of the paged ROM that every language calls with `XCALL`: one implementation, so every language gives the same answer to the same digit.
* **Every language on them**: hylang (its numbers moved out to the libraries), BASIC (rewritten on them), HyForth (a number stack and words), C and assembly (a header and an include file), each with the names it uses for everything else.

### **All of it, in every language**

Everything danlang's numbers have (its `reference.md`: their syntax in section 1, the built-ins under "Numbers"), every language has, through the libraries.

**Reading**: a number in danlang's grammar, whole: `42`, `-7`, `+5`, `1_000_000` (a `_` is passed over), `3.25` (a fixed decimal, with the places written), `720/84` (a rational, in lowest terms: `60/7`; an integer when it's whole), and a base:
* **The 19 named bases**, `#` and a letter.  The balanced and least-digit-first ones: `c` (`-0+`), `e` (`=-0+#`), `g` (`~=-0+#*`), `i` (`UON`), `j` (`WUONM`), `m` (`DanielStphrus`, negative).  Plain: `b` (2), `t` (3), `q` (4), `v` (5), `f` (6), `s` (7), `o` (8), `n` (9), `d` (10), `x` (16), `z` (36).  Balanced, most digit first: `k` (27: `ZYX...N0AB...M`) and `y` (53: `zy...a0AB...Z`).
* **Any radix** of 2 to 80 (`#16rFF`), its digits `0-9`, `A-Z`, `a-z`, then ``-=+`~!@#$%^&*,;:|?`` (case matters past 36); **digits of one's own** (`#[01]101`, 2 to 80 of them, each once).
* **The modifiers**: least digit first or most (`#<`, `#>`), balanced (`#=`: the middle digit is zero), a positive or negative base (`#+`, `#-`), a sign before `#` (`-#x10`).
* A fraction in base 10 is a fixed decimal; in any other base, a rational (`#b0.1` is `1/2`).

**Writing**: as danlang's `print` writes it (`42`, `-1.25`, `2/3`, `1+2i`), or in any of those bases (`to-str`'s: `#xFF`; a rational `#x1/#x3`; a fixed decimal with a radix point when it ends in that base, else as a fraction).  **Complex numbers read back too** (the user's answer; danlang's grammar first): a real part, `+` or `-`, an imaginary part and `i` (`1+2i`, `0.5-1/3i`), or an imaginary part alone (`2i`, `-1.5i`); `i` alone stays a name (`1i` is i).

**The rest**: the arithmetic and comparisons; `abs`, `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `random`, `fib`, `pow`; the tests (`num?`, `int?`, `fixed?`, `rational?`, `complex?`); the bits, on integers of any size in two's complement (`bit-and`, `bit-or`, `bit-xor`, `bit-not`, `shl`, `shr`, `bit?`, `hex`, `bin`, `lo`, `hi`, `word`, `bytes`, `from-bytes`); and the math functions (below).

| | hylang | BASIC | HyForth | C |
| :- | :- | :- | :- | :- |
| A number in a program's text | Every form; bare ones in the base | Every form (`X = #xFF + #b0.1 + 2i`), bare ones in the base; a `#` that names a file (`PRINT #1, X`) isn't one; QuickBASIC's `&HFF`, `&O17`, `&B101` and exponents (`1E6`, `2.5E-3`, exact) | A word Forth doesn't read as a cell or a double (`42`, `$FF`, `123.` stay what they are) but danlang reads as a number, in the base: `1.25`, `2/3`, `#xFF`, `2i`, `100000000000000000000`, onto the number stack | Text, through `num_parse` |
| Text to a number | `(val s)` | `VAL(S$)` | `>n ( c-addr u -- )` | `scanf ("%N", n)`, `num_parse` |
| A number written | `print`, `(to-str x)` | `PRINT`, `STR$(X)` | `n.`, `n>str` | `printf ("%N", n)`, `num_display` |
| In a base named, or by a format string | `(to-str x "x")`; `(format "{x} {}" a b)` | `STR$(X, "x")`; `PRINT USING "{x} {}"; A, B` | `n.base ( c-addr u -- )`; `nformat ( c-addr u -- )` (its numbers from the number stack) | `printf ("%{x}N %{#b}d", n, i)`; `num_display`'s base; `num_format` |
| The tower's functions | As now | A function each (named in [BASIC.md](BASIC.md)'s reference) | `nabs`, `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `nrandom`, `nfib`, `npow` | `num_abs`, `num_truncate`, `num_to_fixed` ... |
| The tests | `num?` `int?` `fixed?` `rational?` `complex?` | A function each | `int?` `fixed?` `rational?` `complex?` (the number stack's top, a flag) | `num_kind` |
| The bits | As now | `AND`, `OR`, `NOT` on integers of any size, and a function each for the rest | `nand`, `nor`, `nxor`, `ninvert`, `nlshift`, `nrshift`, `nbit?`, `nbytes`, `nfrom-bytes` | `num_and` ... |

### **The base: what numbers are shown and read in**

Every language has a **base** setting, as HyForth has `BASE`, and everything follows it, all the time, everywhere (the user's answer): the numbers a program shows, the numbers it reads as it runs, and the numbers in its own text.  A base is any of danlang's, named as `to-str`'s base strings name them (`"x"`, `"#16r"`, `"c"`, `"<x"`, `"[01]"` ...); decimal at the start.  The setting is the `numbers` library's (in its state block, below), so a base is selected one way and works the same way in every language.

| | hylang | BASIC | HyForth | C |
| :- | :- | :- | :- | :- |
| Set it | `(base "x")` | `BASE "x"` | `16 base !`, `hex`, `decimal` (a radix); `s" c" set-base` (any base) | `num_set_base ("x")` |
| Read it | `(base)`: `"x"` | `BASE$` | `base @` (a radix), `get-base ( -- c-addr u )` | `num_get_base ()` |

* **Shown**: every number written without a base named (`print`, `PRINT`, `n.`, the REPL's values, `STR$(X)`, `num_display`) is written in the base.  Whether its prefix shows is the base string's (the user's "a setting"): `"x"` shows `FF`, which reads back in that base; `"#x"` shows `#xFF`, which reads back in any.
* **Any base, when printing** (the user's): a print can name a base, by a parameter (`(to-str x "b")`, `STR$(X, "b")`, `n.base`, `num_display`'s) or in a format string: a placeholder `{}` in the base, `{x}`, `{#x}`, `{c}`, `{16r}`, `{[01]}` in that one, `{{` and `}}` braces.  The library's `num_format` fills them, and hylang's `format`, BASIC's `PRINT USING`, HyForth's `nformat` and C's `num_format` are calls of it; C's `printf` takes a base its own way (below): `PRINT USING "{x} is {} in decimal, {#b} in binary"; 255, 255, 255` shows `FF is 255 in decimal, #b11111111 in binary`.
* **Read as a program runs**: every number read without a `#` of its own (`val`, `VAL`, `INPUT`, `>n`, `num_parse`) is read in the base; a number with a `#` (`#xFF`) in its own.
* **And a program's own text**: BASIC's lines, hylang's source and Forth's (as `BASE` reads Forth's) are read in the base too.  So that names stay names, a bare number in a program's text starts with a digit, `0`-`9` (`0FF` in hexadecimal, as assemblers have it; `FF` is a name); a base whose digits aren't those (the balanced `c`, `-0+`; `i`, `UON`; `m` ...) is written with its `#` there.  A library or a program that must mean the same in any base writes its numbers with a `#` (`#d10`), or sets the base it wants and puts it back, as Forth's libraries do with `DECIMAL`.
* **Integers, fractions and fixed decimals, in every base**: `ff`, `-1A`; a rational, its parts in the base (`1/3`, in hexadecimal `1/3`, in binary `1/11`); a number with a radix point (`1.8` in hexadecimal is `3/2`, `0.1` in binary is `1/2`), read exactly.  A fixed decimal or a rational shown in a base other than decimal: with a radix point when it ends in that base (`0.5` in binary is `0.1`, in hexadecimal `0.8`), else as a fraction (`0.1` in hexadecimal is `1/A`): exact either way, so what's shown reads back as the same number (the user's answer).  (danlang writes a fixed decimal in another base as a rational only: the radix point is new, danlang's first.)
* **Balanced and signed bases**: a balanced base's numbers have no sign (`#c+-0` is -2: its digits least first, 1 - 3 + 0), a negative base's none either; a radix point in them as danlang reads one.
* **Complex numbers**: read and written in the base, each part in it (`1+2i`); in a base whose digits include `i` (`z`, a radix past 18), the form is settled in danlang first.

**The library's calls**, which every language's are:
* **`num_set_base`** selects the base (a base string), checking it; **`num_get_base`** gives it back.
* **`num_parse`** reads a number from a string in the base, or in a base named for the call (a number with a `#` of its own in its own): every kind, every form danlang reads; it gives back the number in the stored format and how many characters it used, so a caller reads numbers one after another (`INPUT a, b`, a program's text).
* **`num_display`** writes a number as text, exactly, in the base or in a base named for the call.
* **`num_format`** fills a format string's placeholders with numbers (and strings, as they are), each in the base or its placeholder's.
* **BASIC's `PRINT`** writes every number with `num_display` (BASIC adding only QuickBASIC's spaces around it), `PRINT USING` with `num_format`, `STR$` likewise; `VAL` and `INPUT` read with `num_parse`; its reader reads a program's numbers with it.  hylang's `print`, `repr`, `to-str`, `format`, `val` and its reader, HyForth's `n.`, `n.base`, `nformat`, `>n` and its interpreter, and C's functions are the same calls.

### **The format numbers are stored in**

A number is a tag byte and what the tag says follows.  Integers' bytes are least first (as hylang's bignums are).

| Tag | Kind | Then | Size |
| :-- | :--- | :--- | :--- |
| `$00`-`$7F` | An integer, -64 to 63: the tag itself, 7-bit two's complement | Nothing | 1 byte |
| `$80`-`$8F` | A positive integer of n bytes (the tag's low 4 bits are n - 1: 1 to 16 bytes) | Its magnitude | 2-17 bytes |
| `$90`-`$9F` | The same, negative | Its magnitude | 2-17 bytes |
| `$A0`, `$A1` | A positive or negative integer of 17 to 255 bytes | A length byte, then its magnitude | 19-257 bytes |
| `$B0`-`$BF` | A fixed decimal of 0 to 15 places (the tag's low 4 bits) | Its digits, an integer (one of the forms above) | 2 or more |
| `$C0` | A fixed decimal of 16 or more places | The places (2 bytes), then its digits, an integer | 4 or more |
| `$C1` | A rational | Its numerator, then its denominator, integers | 3 or more |
| `$C2` | A complex number | Its real part, then its imaginary part, real numbers (an integer, a fixed decimal or a rational) | 3 or more |
| `$C3`-`$FE` | Reserved | | |
| `$FF` | Never a number: free for a program's own use (BASIC's reference to a long number in its heap, below) | | |

Each number has one form: an integer in its shortest; a fixed decimal with no 0 at its digits' end (but `0.0`); a rational in lowest terms, its denominator above 1; a complex number's imaginary part not 0.  So two numbers of the same kind are equal exactly when their bytes are.  Numbers of different kinds can still be equal in value (`1` and `1.0`, a fixed decimal of no places, as danlang keeps it), so `=` asks the library.

Some numbers, and their sizes (Microsoft's floating point is always 5 bytes; hylang's heap a 2-byte value for a fixnum, else a cell and a blob):

| Number | Bytes | Size |
| :----- | :---- | :--- |
| `0`, `1`, `-1`, `63` | `00`, `01`, `7F`, `3F` | 1 |
| `100`, `-100` | `80 64`, `90 64` | 2 |
| `65535` | `81 FF FF` | 3 |
| `2147483648` (2^31) | `83 00 00 00 80` | 5 |
| `100000000000000000000` (10^20, which Microsoft's floating point can't hold exactly) | `88` and 9 bytes | 10 |
| `0.5`, `-2.5` | `B1 05`, `B1 67` | 2 |
| `3.14` | `B2 81 3A 01` | 4 |
| `1/3`, `2/3` | `C1 01 03`, `C1 02 03` | 3 |
| `1+2i`, `i` | `C2 01 02`, `C2 00 01` | 3 |
| pi to 11 places (`3.14159265359`) | `BB 84 4F F6 59 25 49` | 7 |

Most numbers a program uses are 1 to 5 bytes, as small as Microsoft's or smaller, and exact.

**Every number stored is stored so**: in BASIC's variables, arrays and heap, on HyForth's number stack and in its variables, in hylang's heap, in a C program's buffers, and in what `SAVE`, `save` and the like write as binary.  The one place a number isn't bytes of the format is hylang's fixnum: a small integer is its 16-bit value itself (bit 0 set), as now, so that its loops stay quick; it becomes the format's bytes (`00`-`7F`, `80`-`81 ...`) the moment it's handed to the libraries (the user's choice).

### **The libraries**

**`numbers`** (a library module, `HT_LIBRARY`, a bank of the paged ROM): the tower, from hylang's `numreg.inc`, `numval.inc`, `numtext.inc`, `numbi.inc` and `numbits.inc`, taken out of hylang and made to work on the format instead of hylang's heap:

* Arithmetic: add, subtract, multiply, divide (exact), negate, absolute value, compare (-1, 0, 1), sign, equal; whole powers; quotient and remainder of integers; gcd.
* Conversions: integer part (`truncate`), floor, round; `to-fixed` (places), `to-rational`; a rational's numerator and denominator; `complex`, and a complex number's parts; to and from 16- and 32-bit integers (for cells, `PEEK`, array indexes).
* Text: `num_parse` reads a number from a string in the base the library holds (`num_set_base`, `num_get_base`) or in one named for the call; `num_display` writes one, likewise; `num_format` fills a format string's placeholders (`{}`, `{x}`, `{#x}` ...): every form danlang reads, every kind, every base (the sections above).  BASIC's exponents (`1E6`) as an option of the reader's, BASIC's alone.
* Bits, on integers of any size, in two's complement: and, or, xor, not, shifts, a bit's test.
* Random numbers: hylang's generator (a 16-bit xorshift), its state in the libraries' bank: an integer below n, or a fixed decimal from 0 to 1.

**`math`** (a second library module): the functions (next section).  It calls `numbers` for its arithmetic.

**The interface**, as the system calls are (and made from a specification the same way, `spec/numbers.def`, so that each language's bindings are made, not written):

* A library keeps a jump table after its header; a caller finds its bank with `MODINFO` as it starts (by name), and calls a routine with `XCALL` (`r15` the routine's address, `r14` the bank).
* The arguments are in `r0`-`r13`, as a system call's are: the operands' addresses (numbers in the format, wherever the caller has them but its paged ROM, which is the library's while it runs: its RAM, or the bank it has at `$8000`), the result's address and how much room it has, the precision (for `math`), a format for the calls that name one.  The answer: C clear, and the result's length in `.A`/`.X`; or C set, and an error in `.A` (too big, division by zero, not a number, no room for the result, a domain error).
* **The libraries' bank** (step 2's change; the plan had the caller lend scratch pages in `r13` and a state block in `r12`): the caller gives the libraries a RAM bank of its own (`BANKS_ALLOC`, one bank, 8K) and names it in `r13`, the same bank for the program's life.  A call selects it at `$8000` (the caller's back as it ends) and the library keeps everything there: its registers, as hylang's number code has them (eight 256-byte pages, a number's bytes least first, with a length and a sign, so a sum or product of long numbers makes nothing till the result is written), its state between calls, and an arena for a call's numbers (its operands, the numbers it makes on the way, its result), with pages for a number's digits as it's written: the base (its string), the precision for `math` (12 digits at the start), the random generator's state.  `num_init` fills it (decimal, a seed from the clock and the ticks) as a program starts; every other entry refuses a bank it hasn't filled (`NE_INIT`).  Why a bank: a library has no RAM of its own, and a program's zero page and low RAM are its own (hylang's zero page is full); a bank holds the registers and the arena whole, out of every language's way.  The caller's operands are read and its results written a byte at a time, the caller's bank selected for the byte only when the address is in `$8000`-`$9FFF`.
* **Zero page**: the libraries work with 16 bytes of the zero page, `$70`-`$7F`, and save them as a call starts and put them back as it ends (some 200 cycles a call), so a program's zero page stays its own: hylang's is full, `$22`-`$7F`, HyForth's to `$7A` (step 1 looked).
* An abort point at each entry, as hylang's number code has (`n_enter`), so a result too big or no room goes back from however deep with its error.

The `numbers` code is about 11K (hylang's third bank is 12K, mostly numbers): a bank.  `math` adds perhaps 4-6K: a second bank.  ROM space is plentiful; what's scarce (BIOS page 0, the COMMON block) isn't touched.

### **The math functions**

| What | hylang | BASIC | HyForth | C |
| :--- | :----- | :---- | :------ | :- |
| Square root | `(sqrt x)` | `SQR(X)` | `nsqrt` | `num_sqrt` |
| e to the x | `(exp x)` | `EXP(X)` | `nexp` | `num_exp` |
| Natural logarithm | `(log x)` | `LOG(X)` | `nlog` | `num_log` |
| Sine, cosine, tangent (radians) | `(sin x)`, `(cos x)`, `(tan x)` | `SIN`, `COS`, `TAN` | `nsin`, `ncos`, `ntan` | `num_sin` ... |
| Arc tangent | `(atan x)` | `ATN(X)` | `natan` | `num_atan` |
| A power | `(pow x y)` (now any real `y`) | `X ^ Y` | `npow` | `num_pow` |
| Pi | `(pi)` | `PI` | `npi` | `num_pi` |
| The precision | `(digits)`, `(digits n)` | `DIGITS n` | `digits` (a variable) | an argument |

* **Exact when the answer is**: `sqrt` of a perfect square (of an integer or a rational) is exact (`(sqrt 9/4)` is `3/2`); `(exp 0)` is 1, `(log 1)` is 0, `(sin 0)` is 0, a power with a whole exponent is exact (as `pow` is now).
* **Otherwise a fixed decimal** of the precision's significant digits, correctly rounded: argument reduction, then series in fixed point with guard digits.  The default precision is 12 significant digits (three more than Microsoft's 9: the user's answer); each language may change it.
* **danlang first**: the same algorithms in danlang (C#, `System.Numerics.BigInteger`), so the cross-check (random expressions, run in both) stays exact to the last digit, as it is for the arithmetic now.
* A negative square root or logarithm is a complex number, in every language (the tower has them: `(sqrt -1)` is `i`).

### **hylang and danlang**

* **danlang**: the math functions and `digits` first, as the reference; and the format, as two built-ins (`(number-bytes n)` and `(bytes-number b)`: danlang's `bytes` and `from-bytes` were taken, a string's bytes), so the cross-check can compare bytes.
* **hylang**: its number objects hold the format (a bignum's blob, a fixed decimal's, a rational's and a complex number's become one blob of the format's bytes, in its own banks), and its number built-ins call the libraries.  Its quick ways for fixnums stay in hylang (`+`, `-`, `*`, the comparisons, the native code's templates), so its loops are as fast as now; only numbers past a fixnum go to the library.  Its third bank gets most of its 12K back.  `numbers.dl` and the 2,100 random expressions must give the same as before, byte for byte, and the benchmarks no slower.
* The new built-ins: `sqrt`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, `pi`, `digits`; `pow` takes any real exponent.

### **BASIC**

A new BASIC, for the Hydra, as QuickBASIC was for the PC: line numbers optional, labels, blocks, `SUB` and `FUNCTION`; its own plan, [BASIC.md](BASIC.md).  Its numbers are these:

* **A value is 5 bytes**, holding the stored format's bytes when the number fits in 5 (most do: integers to 2^32, `0.5`, `3.14159`, `1/3`), else `$FF` and a reference (the number's address and length) to its bytes in the heap the strings use.  So variables, arrays and `FOR` take no more room than Microsoft's 5-byte floats did.
* **One number type** ([BASIC.md](BASIC.md)): QuickBASIC's suffixes and type names (`%`, `&`, `!`, `#`, `AS INTEGER` ...) are accepted, and every one is the same exact number, stored in the compact format.
* **Exact**: `10/4` is `5/2`, `0.1 + 0.2 = 0.3` is true, `2 ^ 100` is exact; `FOR x = 0 TO 1 STEP 0.1` runs 11 times; `PRINT` shows every number exactly (`1/3` as `1/3`).
* **Every base**: numbers in a program's text in every form danlang reads (the 19 named bases and the rest; bare ones in the base `BASE` sets, starting with a digit: `0FF`), QuickBASIC's `&HFF`, `&O17`, `&B101`, and exponents (`1E6`, `2.5E-3`, exact); `VAL` and `INPUT` read in the base; `PRINT` and `STR$(X)` show in it; `STR$(X, "x")` and `PRINT USING "{x}"; X` in any.
* **The functions**: `SQR`, `EXP`, `LOG`, `SIN`, `COS`, `TAN`, `ATN`, `^`, `PI` call `math` at `DIGITS` digits; `INT` (the floor), `FIX`, `ABS`, `SGN`, `RND` (the next number from 0 to 1, a fixed decimal; a negative argument seeds it), `MOD`, `\` (integer division), and the tower's other functions and tests (`TRUNCATE`, `TOFIXED`, `NUMERATOR` ..., named in BASIC.md's reference) call `numbers`.
* **The bits**: `AND`, `OR`, `XOR`, `NOT` on integers of any size, in two's complement (the library's), not 16-bit only; `PEEK`, `POKE` and the like take integers that fit.
* **Quick small integers**: those that fit a value are added, subtracted and compared without a library call (counters, `FOR` loops); the rest go to the libraries.

### **HyForth**

A library, **`lib numbers`** (`numbers.fl`), with `math`'s words in it or in a second, `lib math`:
* **A number stack**, as Forth's floating-point stack is a stack of its own: in a RAM bank of the task's (8K), each entry a number in the format, so a number's bytes are copied as Forth copies cells (no collector needed).
* **Words**, hylang's names where hylang has one: `n+`, `n-`, `n*`, `n/`, `nnegate`, `nabs`, `n=`, `n<`, `n0=`, `ncompare`; `ndup`, `ndrop`, `nswap`, `nover`, `nrot`, `ndepth`; `s>n`, `d>n`, `n>s`, `n>d` (to cells and doubles, if it fits); `n.`, `n.base`, `>n` and `n>str` (below); `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `nrandom`, `nfib`; `int?`, `fixed?`, `rational?`, `complex?`; the bits, `nand`, `nor`, `nxor`, `ninvert`, `nlshift`, `nrshift`, `nbit?`, `nbytes`, `nfrom-bytes`; `nsqrt`, `nexp`, `nlog`, `nsin`, `ncos`, `ntan`, `natan`, `npow`, `npi`, and `digits` (a variable).
* **Variables**: `nvariable`, `n@`, `n!`, `nconstant`, `nvalue`; a variable's number in `memory.fl`'s heap (`allocate`d, `resize`d as it grows).
* **Literals**: what Forth reads as a cell or a double stays one (`42`, `$FF`, `#10`, `123.`); any other word danlang reads as a number, read in the base, goes on the number stack: `1.25`, `2/3`, `#xFF`, `#16r1F`, `2i`, `100000000000000000000`.  So every form of danlang's is a literal, and no Forth program changes.
* **The base**: Forth's `BASE` is the base of cells and the number stack alike: `hex`, `decimal` and `16 base !` set the library's base too; `s" c" set-base` selects any base, and `get-base ( -- c-addr u )` gives it.  With a base that isn't a plain radix (balanced, least digit first, digits of its own, past 36), cells are shown and read through the library too, so they follow it as everything else does.  `n.` writes in the base; `n.base ( c-addr u -- )` in a base named; `nformat ( c-addr u -- )` a format string (`{x}` ...), its numbers from the number stack; `>n ( c-addr u -- )` reads.
* **No Floating-Point word set** (the user's choice): the n-words are HyForth's numbers.

### **C and assembly**

* **C**: `num.h` and `lib/num.c`: `num_add (dst, room, a, b)`, `num_set_base`, `num_parse`, `num_display (dst, room, a, base)` (`NULL`: the base), `num_format (dst, room, fmt, ...)` (its placeholders' numbers after it), `num_sqrt (dst, room, a, digits)` ..., numbers in byte arrays in the format; each function an `XCALL` through a small piece of assembly.
* **C's `printf` takes a base** (the user's): the C library's `printf` family (`printf`, `fprintf`, `sprintf`, `snprintf` and their `v` forms) gains `%N`, a number in the stored format (a pointer to its bytes), shown in the base; and a base in braces before a conversion, for `%N` and for C's integers alike: `%{x}N`, `%{#b}d` (`#b101`), `%{c}ld` (an `int` or a `long` in balanced ternary), `%{16r}u`; `%{*}N` takes the base, a string, from the arguments, as `*` takes a width.  Width, `-` and `0` pad the whole text, as C's do.  Without braces C's own conversions are C's (`%d` decimal, `%x` hexadecimal), so C programs read as they always did.  `scanf` the same way for reading: `%N` reads a number in the base into a buffer, `%{x}d` an `int` in base x.  The C library's own `printf` core does it (cc65's, with these added), each number written by `num_display`.
* **Assembly**: `numbers.inc` in the SDK (`sdk/asm`), made from `spec/numbers.def`: the jump table's names and a macro for the call.
* **`calc`**, a tool at rc (the user's answer): `calc 2/3 + 0.5` shows `7/6`, `calc sqrt 2`, `calc -b x 255` shows `FF`; an expression on its command line, or one a line on its input, in the base `-b` gives (decimal without it); in C, over `num.h`, in `/bin`.

### **Tests**

* **The format**: an encoder and decoder in JavaScript (`sim/tools/numfmt.js`) and in danlang, and the cross-check: random expressions, run in danlang and through the libraries (a test module, `t_num`, in the emulator), compared byte for byte, as hylang's 2,100 are now; then the math functions the same way at several precisions.
* **Every base**: random numbers of every kind (integers, fixed decimals, rationals, complex numbers) written in each of the 19 named bases, a few radixes, digits of their own and each modifier, then read back: the same number, and the same text in danlang and through the libraries; format strings' placeholders; and each language's base set, then numbers shown, read as it runs, and read in its program text.
* **hylang**: its suites (`hysuite`, `hylang`, `numbers.dl`), unchanged, must pass; `bench` and `hyspeed` no slower.
* **BASIC**: its new suite ([BASIC.md](BASIC.md)), the numbers among its checks.
* **HyForth**: a test file of the number words (as the standard's suite files are), and the Floating-Point suite's file if that word set is built.
* **C**: `calc` at rc.

### **The order of work**

| Step | Work | Size |
| :--- | :--- | :--- |
| 1 | **danlang first, and the interface**: the stored format in danlang (`number-bytes`, `bytes-number`) and JavaScript (an encoder and decoder); danlang's `base` (its reader, its printing, `val`: all of it), the radix point in other bases, complex numbers read (`1+2i`, `2i`), format strings' bases (`{x}`); `spec/numbers.def`; the zero page, workspace and state block; `XCALL`'s cost measured | M |
| 2 | **`numbers`**: hylang's number code taken out into a library on the stored format; its bank, the base, `num_parse`, `num_display` and `num_format` (the radix point added); `t_num` and the cross-check | L |
| 3 | **hylang on `numbers`**: its objects in the stored format, its built-ins through the library, `base`; its suites, the cross-check and its benchmarks as before | M |
| 4 | **`math`**: danlang's functions first, then the library, and hylang's built-ins (`sqrt` ... `digits`) | M-L |
| 5 | **HyForth's `lib numbers`** (and `lib math`): the number stack, the words, literals, `BASE` and `set-base` for everything, `nformat`, its test file | M |
| 6 | **C, assembly and `calc`**: `num.h`, `printf` and `scanf` with `%N` and `%{base}`, `numbers.inc`, the `calc` tool | M |
| 7 | **BASIC**: the new language on the libraries, by its own plan ([BASIC.md](BASIC.md)) | L |
| 8 | **Documents**: `docs/basic.md` and the guides, the programmer's guide (a chapter on numbers), `status.md`, the guide and its PDF | S |

hylang goes first because its tests check every corner of the tower: the library taken out of it is right when hylang still passes them.  HyForth before BASIC because it's smaller and is the library's first new caller; BASIC, the largest, last.  BASIC's own first steps (its language, reading a program) can start at once; its core needs steps 1-4.

### **Risks**

* **Speed**: a library call costs an `XCALL` (measured in step 1) and copying a number's bytes; exact numbers grow (a rational's parts, a long product).  The answers: the quick ways for small integers stay in each language (hylang's fixnums, BASIC's 5-byte integers), and only the rest go to the library.  The math functions in decimal fixed point are slower than Microsoft's polynomials: measured, at 12 digits, against a budget set in step 4.
* **Exactness surprises**: a BASIC program that expects `1/3` to print as `.333333333`, or a loop `FOR X = 0 TO 1 STEP 0.1` (exact now, so it runs 11 times; with Microsoft's binary 0.1, whether the last step reaches 1 was a matter of rounding).
* **Memory**: long numbers in BASIC's heap; a value stays 5 bytes, and the collector is new and quick.
* **hylang**: moving its numbers out could break what works; step 3 is checked by its suites, and hylang keeps its own code till they pass.
* **The zero page**: `$70`-`$7F` must be free in hylang, BASIC and HyForth across a call; step 1 checks each.

### **Questions**

None open: the user answered them all (below).

### **Answers** (the user's, 7 October 2026)

1. **Every number is shown exactly**, in every language: a fraction as a fraction (`1/3`), a long integer whole, never rounded for display (it was question 2: how BASIC prints a fraction).
2. **BASIC has one number type** (BASIC.md's answers): QuickBASIC's number types accepted as names, all the same exact number.
3. **The numbers library holds the current base**, and has the parse and display functions that read and show numbers in it (`num_parse`, `num_display`); BASIC's `PRINT` uses them, and so does every language.
4. **The math functions' precision**: 12 significant digits by default; each language may change it (`DIGITS n`, `(digits n)`, `digits`).
5. **Complex numbers read back**: `1+2i`, `0.5-1/3i`, `2i`, in every language, danlang's grammar first.
6. **HyForth's numbers are the n-words**: no Floating-Point word set.
7. **hylang on the shared library**: one implementation of the number system.
8. **A `calc` tool** at rc.
9. **The setting is "base"** in every language, as HyForth's `BASE` (`(base "x")`, `BASE "x"`, `set-base`, `num_set_base`).
10. **The base everywhere, all the time**: what's shown, what's read as a program runs, and a program's own text, in every language.
11. **The prefix, a setting**: the base string's `#` (`"x"` shows `FF`, `"#x"` shows `#xFF`).
12. **A fraction in another base**: a radix point when it ends there, else a fraction.
13. **hylang's fixnums stay** in its 16-bit values, the format's bytes as they leave hylang.
14. **Printing in any base**: a base parameter, or a format string's placeholders (`num_format`).
15. **C's `printf` takes a base**: `%N` for a number, and `%{base}` before a conversion (`%{x}N`, `%{c}d`), for C's integers too; `scanf` the same for reading.

### **As built: step 1**

October 2026: danlang's branch `feature/numbers` (from `feature/speed`), and hydra-2's `reborn-numbers`.

* **danlang, the reference** (`8fc0da2`): `(base)` and `(base b)`, followed everywhere (print, `repr`, `to-str`, `format`'s `{}`, `val`, `read`, and a program's text from the next expression read: `load` now reads a file an expression at a time, each run before the next is read, as hylang's does); in a program's text a bare number starts with a digit (`0FF`); `save` writes a number with its prefix.  `to-str`'s base string says whether the prefix is written (`(to-str 255 "x")` is `FF` now, `"#x"` `#xFF`), and every base danlang reads it writes too (digits of their own, balanced radixes).  A fraction or a fixed decimal in another base: a radix point when it ends there (`#b0.1`), else a fraction (`1/A`); a complex number its parts (in a base whose digits have `+`, `-` or `i`, in decimal with `#d`).  Complex numbers read (`1+2i`, `0.5-1/3i`, `2i`; `1+i` stays a name).  `format`'s `{b}`.  `(number-bytes x)` and `(bytes-number l)`, the stored format, the second taking only a number's one form.  Its suite: 1,417 checks (80 new; four changed: `to-str`'s `"x"`, two fractions written with a point, a complex number in a base), none failing; `run.dl` reads each file in decimal.
* **The format's reference in JavaScript** (`sim/tools/numfmt.js`): `encode`, `decode` (strict: a number's one form, else an error), `parse` and `show` (danlang's decimal forms), its own checks (`--test`: the plan's table, the long forms, what `decode` refuses, 3,000 numbers encoded and decoded back).  Against danlang's `number-bytes`: 4,000 random numbers (703 integers, 1,048 fixed decimals, 1,500 rationals, 749 complex numbers), the same bytes and the same text, every one.
* **The libraries' calls** (`spec/numbers.def`): 41 entries (33 `numbers`, 8 `math`), each with its registers, errors and each language's name; `r12` the program's state block (`NUM_STATE`, 64 bytes: the base, the precision, the random state), `r13` the work pages (`NUM_PAGES`, 8), `r0`-`r3` the operands and the result's place and room, the result's length in `.A`/`.X`; the `NE_` errors and the `NK_` kinds.  `apigen.js` reads it in step 2.
* **What a call costs**: `XCALL`, counted from its code, about 116 cycles (the jump table's `jsr` to the caller's return), 130 with `r14` and `r15` set; the zero page's save and restore some 200 more.  So a quick way for small integers in each language (hylang's fixnums, BASIC's 5-byte integers) matters, as the plan has it.
* **Not yet**: hylang's copy of danlang's suite (`tests/hylang`) is danlang's `feature/speed`'s till hylang takes these (step 3).

### **As built: step 2**

Done, on `reborn-numbers` (not merged yet): the numbers library, all 33 of its entries.

* **The number system's reference in JavaScript** (`sim/tools/numref.js`): danlang's tower (`feature/numbers`) for the library's tests: the arithmetic, the conversions, the bits, `fib`, hylang's random generator step for step, and text, every base written and read.  `sim/tools/numxcheck.js` checks it against danlang: 11,243 cases from a fixed seed, none different.
* **The library's skeleton** (`modules/numbers`, a module of the paged ROM, `HT_LIBRARY`, after the others in `rom.txt`):
  * `spec/numbers.def` changed for the bank (`r13`, above), with `NE_INIT` (a bank `INIT` hasn't filled), `NE_TODO` (an entry not written yet), `NUM_MAX` (1,040: the longest number is 1,039 bytes, a complex number of two rationals of 255-byte integers) and the format's tags (`NT_`); `SEED` takes 16 bits, the generator's state.
  * `tools/apigen.js` reads it: `obj/sdk/numbers.inc` (each entry's address, `NUM_ADD` = `$A042` ..., `MATH_SQRT` ..., and the constants) and each library's jump table (`obj/gen/numbers_jt.inc`), which the module includes after its header.
  * The bank's layout (`nmbank.inc`): the registers at `$8000`-`$87FF`, then the state and the variables (612 bytes), then the arena to `$9FFF` (5,532 bytes), where a call's numbers are: its operands copied in, the numbers it makes on the way, its result, each in the stored format.  A number's digits use the arena's last eight pages while it's written (2,048: a 255-byte integer in base -2 is 2,042 digits; DISPLAY's one operand is at the arena's start), and PARSE's texts two of them.
  * A call (`nmcall.inc`): `nm_begin` selects the bank, keeps the caller's (`$00`) and its zero page `$70`-`$7F`, notes the stack, and refuses a bank `INIT` didn't fill; `nm_end` puts them back, the answer in C, `.A` and `.X`; `nm_fail` goes back to the entry's caller from however deep with an error.  The caller's numbers are measured by their tags as they're copied in (`NE_NOTNUM` for tags and lengths that aren't a number's), and results copied out with the room checked (`NE_ROOM`).
  * The first entries: `INIT`, `GET_BASE`, `SEED`, and `BYTES`, the format's one form checked as `numfmt.js`'s `decode` checks it, lowest terms too.
* **The registers and the tower** (`nmreg.inc`, `nmval.inc`; the entries `ADD`, `SUB`, `MUL`, `DIV`, `NEG`, `ABS`, `CMP`, `KIND` in `nmarith.inc`): hylang's `numreg.inc` and `numval.inc` with its heap made the arena.  A value is a number's address in the stored format, its parts found by their tags (`n_first`, `n_parts`, `n_size`); an integer is read into a register from its bytes (`r_load`) and written in its one form (`r_store`).  The complex numbers' machine keeps its values on a stack in the bank, and an operation on two values it made writes its result in their room, so the arena holds the worst case (a quotient of two complex numbers of rationals) with room to spare.  Where hylang's way was slow, the library's is quicker, with the same answers:
  * the greatest common divisor is binary (each number's twos shifted out, the less taken from the more, shifts and subtractions), not Euclid's way with a division a bit at a time at every step; but while one is far longer than the other (by more than a third of the shorter's bytes), the longer is made its remainder by the shorter, one division for what would be many subtractions;
  * a sum, difference, product or order of integers and fixed decimals is worked on their digits, scaled to the same places (a product's places both's), not as fractions over 10^places with a product and a long division after;
  * powers of ten are made a hundred at a time, and 10^615 or more (past 255 bytes) is too big at once;
  * a rational negated keeps its parts (they're in lowest terms) without its divisor worked out again.
  * Simple calls take 16,000 to 64,000 cycles each in the test (its own reading and writing of the card counted): `ADD 1, 2` 16,000, `ADD 0.1, 0.2` 20,000, `ADD 1/3, 1/7` 28,000, `MUL 1+2i, 3-4i` 32,000, `DIV 123456789, 987654321` 64,000.  Big rationals are slow: the binary divisor costs about 70 cycles a byte for each bit of the numbers, some 26M cycles for two of 255 bytes.  A quicker division for Euclid's way with big numbers (Knuth's, a byte of quotient at a time) is for later.
  * `CMP` can be too big (`NE_BIG`: it works the two as the sums are), so `numbers.def` says so.
* **Integers, conversions, bits, random numbers, Fibonacci** (`nmconv.inc`, `nmbits.inc`, `nmrand.inc`): `IDIV` (the quotient at `r2`, the remainder at `r5`, its room `r6`, both rooms checked before either's written), `GCD`, `POW` (numref.js's loop, `r` and `b` kept in two slots in the arena so a long power doesn't fill it), `TRUNCATE`, `FLOOR`, `ROUND` (a fixed decimal of 615 places or more is below 0.13: 0, or -1 for a negative one's floor, without 10^places), `TO_FIXED` (an integer as itself of no places, a fixed decimal by its digits cut, a rational a digit at a time as hylang did; past 10,000 places `NE_DOMAIN`), `TO_RATIONAL`, `NUMERATOR`, `DENOMINATOR`, `COMPLEX`, `PART`, `FROM_INT`, `TO_INT` (`r4`/`r5` the low 32 bits), `BITS` (hylang's two's complement; a shift left is too big only past 2040 bits, where hylang's refused some that fit), `RANDOM` (hylang's generator, and 0 for a fixed decimal of the precision's places, 12) and `FIB` (hylang's doubling: fib(2939) the last, as it makes fib(n + 1) too).  A number equal to an integer will do where one is needed (`3.0`).  `nm_begin` keeps `.A` and `.X` now (`TO_FIXED`'s places).  The library: 9,052 bytes of code.
* **Text** (`nmtext.inc`: `SET_BASE`, `DISPLAY`, `PARSE`, `FORMAT`): hylang's `numtext.inc`'s printer and its reader's digits and named bases, with what danlang's `feature/numbers` added, as `numref.js` has it.
  * A base's string is read as danlang's `NumberFormat.Of` reads it (`SET_BASE` refuses one that names no base: `NE_BASE`); a call's own base in `r4`, or the state's.  Two bases at once: the call's, and the one a number's read or written in (a number's own `#x`; decimal's, `#d`, for a complex number in a base whose digits have `+`, `-` or `i`).
  * `DISPLAY`: decimal without its prefix as danlang prints (hylang's printer, by pairs of digits); in another base a fraction in lowest terms, with a radix point if its denominator divides a power of the base (`n * (base^k / d)`, the power's part found a byte's gcd at a time, so only the numerator's digits must fit), else `n/d`; complex numbers by their parts; the prefix when the base string has its `#`.  A plain base's digits come a chunk a division (7 binary digits, 2 decimal); a balanced or negative base's a digit a division, as `q * size + rem` so nothing passes 255 bytes on the way.  The text is written as it's made (`NE_ROOM`: what there was room for written).
  * `PARSE`: `numref.js`'s `parse` (spaces at the ends and `_`s out, a `/`, signs unless the base has them as digits, the program's-text rule, a number's own `#` base by danlang's reader's rules, complex numbers by their `i`), on the longest start of the text that's a number (of its first 255 characters).  Each start is checked as text first, no arithmetic; only the longest that is one is made (its value can't change which starts are numbers, but for a `/0`, which is still found on the longest).
  * `FORMAT`: danlang's `format`: `{}` and `{base}` placeholders, `{{` and `}}`, numbers and strings as arguments (a table ending in `$FF`), `NE_FORMAT` (a new error) for too few or too many or a string for a `{base}`.
  * **A bug in danlang, found here**: a balanced base of an even size (`#=16r`, a balanced decimal) had its largest digit one past its digits (`MaxDigitVal` its 0's place).  Now `size - 1` less its 0's place, in danlang (`feature/numbers`, `76607f9`, 7 checks more: 1,424, none failing), `numref.js` and the library; `numxcheck.js` again: 11,243 cases, none different.  hylang's printer had the same flaw.
  * A bit test of a bit below 0 is 0 at once (it shifted left, and could be too big).
  * The library: 13,740 bytes of code, 84% of its bank.
* **The test** (`numbers`): `t_num` (`tests/mod/t_num`) makes the calls a card's file has (`num.in`, from `tests/numtest.js`) as a program makes them, and writes what each gave back to another (`num.out`), which the test's check compares with the reference; every call is checked to keep the caller's bank, its zero page `$70`-`$7F` and `r0`-`r3`, with its operands and results in the task's RAM and in its bank at `$8000`.  5,018 calls:
  * the state;
  * the format's forms one by one (each length's tags, the places, rationals, complex numbers, the longest number), 300 random numbers of every kind and each changed a little, room and counts;
  * the arithmetic: hand-picked cases (0.1 + 0.2, a division by 0 and by 0.0, complex quotients, the edges of 255 bytes), 840 random sums, differences, products and quotients (40 of them with big operands), 300 orders, 450 negations, absolute values and kinds, each against `numref.js`.
  * the integers' entries, the conversions, the bits (each operation, counts below 0 and past 16 bits, the edges of 2040 bits), random numbers after three seeds against `numref.js`'s generator, Fibonacci numbers to the edge of 255 bytes: 1,512 calls;
  * text: 79 base strings (every named base, modifiers, radixes, digits of one's own, and ones that name none) set and got back; 527 numbers written in every kind of base (from the state's base too, the edges of 2040 bits, no room); 485 texts read: what's written read back, with its `#` and bare in its base, a program's text, junk, numbers with text after them, spaces, past 255 characters; 50 formats; each against `numref.js` (and the model where it's too big);
  * `numref.js`'s integers have no end, the library's 255 bytes, so `numtest.js` has a model of the library's way (its registers' values as each is made, as `nmval.inc` makes them) that says where it's too big; where it isn't, the model's answer and `numref.js`'s must be the same, and are, for every call.
  * Every call's answer is the reference's, every `NE_BIG` where the model has it.  1,533M cycles (64 seconds).  Run with three other seeds too (7, 99, 2026): the same.

### **As built: step 3**

Done, on `reborn-numbers` (not merged yet): hylang's numbers are the library's.

* **A number's cell** (`hylang.inc`): a fixnum as ever (bit 0 set, -16384 to 16383), or a cell of one kind, `PK_NUMBER`: its length and its blob, the number's bytes in the stored format, in its one form.  The four kinds there were (`PK_BIGNUM`, `PK_FIXED`, `PK_RATIO`, `PK_COMPLEX`) are one; kinds 8-10 are free.  An integer in a fixnum's range is always a fixnum, so a value's form is still its number's (`eq`, hashes' keys).
* **The bridge** (`numlib.inc`, the third bank): the library found as hylang starts (`MODINFO`, the module `numbers`; without it hylang says so and stops), a RAM bank for its work (`r13`) and another for a number's text (8K: room for a complex number of rationals in binary).  A call's operands are staged in the reader's scratch (`NL_A`, `NL_B`, each `NUM_MAX` bytes: a fixnum's bytes made, a cell's blob copied), its result written after them (`NL_R`) and made a value (`nl_value`: a fixnum if it's an integer that fits, else a cell and a blob); a base's string in the RAM (`NL_S`).  `DISPLAY` writes into the text's bank and `out_byte` sends it on; past 8K, what fits; too big in its base (`NE_BIG`), in decimal with `#d`.  hylang's abort points (a built-in that runs out of room deep inside) came with it from `numreg.inc`.
* **Fixnums as before**: `+`, `-`, `*`, `/` and the comparisons on fixnums are worked where they were (the evaluator's quick built-ins, the bytecode machine's quick ops, the native code's templates), and only a number past a fixnum goes to the library.  While the base is plain decimal without its prefix (`SET_BASE`'s answer, below), hylang reads and prints fixnums itself too.
* **The built-ins** (`numbi.inc`, `numbits.inc`, rewritten): the arithmetic folds its arguments through `ADD` ... `DIV`; `abs`, `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `val`, `fib`, `random`, `to-str` and `format`'s `{base}` (a new placeholder in hylang's `format`), the bits (`bit-and`, `bit-or`, `bit-xor` quick on fixnums), `hex`, `bin`, `lo`, `hi`, `word`, `bytes`, `from-bytes`, the type tests, `range`, a hash's numeric key (`2.0` is the key `2`), and the system library's conversions (lengths, offsets, times, a call's registers).  New, as danlang has them: `(base)` and `(base b)`, the base everywhere (print, `repr`, `to-str`, `format`'s `{}`, `val`, `read`, a program's text from the next expression read, `save` with the prefix), and `number-bytes` and `bytes-number`, the stored format.  `numreg.inc`, `numval.inc` and `numtext.inc` are gone: 5,500 lines less.
* **A wider table of built-ins**: hylang's had 256 of 256 (a built-in is the value `BUILTIN0 + 2 * n`, `n` a byte), so the three new ones, and step 4's nine, need more.  The values `$0600`-`$07FF` (`BUILTIN2`, the immediates' pages now 8, two pages of the first bank of cells less) are a second table: built-ins 256 to 511, each an ordinary one whose code is in another bank (the `BI` macro checks it).  The first 256 are worked as they were, by their number, at no cost; the evaluator applies the second table's in a way of its own (`ev_bapply2`: the arguments counted, partially applied, a far call), and the bytecode machine and its compiler call them as any function, through the evaluator.  `base`, `number-bytes` and `bytes-number` are the first three there.
* **The library, for hylang**: `SET_BASE` gives back what the base is (`.A`: bit 0 decimal, bit 1 its prefix written); `PARSE`'s `.Y` bit 1 reads the whole text or nothing (`val`, a word of the reader's: `1/0x` isn't a number, as `1/0` is a division by zero).  A caller's number below `$8000` is copied in and out straight (each byte through the bank's test before: about 60 cycles a byte, 20 now), and the zero page `$70`-`$7F` kept unrolled: 224 cycles a call less.
* **The system calls**: hylang's `sys` puts each argument straight in its register (`r0`-`r15`) as it's converted, and a conversion past a fixnum calls the library, which uses `r0`-`r5` and `r13`-`r15` itself; so those conversions keep the call registers (`nl_rsave`, `nl_rrest`), and a fixnum is converted without the library.
* **The third bank**: 9,928 bytes (8,082 of code), 14,088 before: 4,160 bytes back, not most of its 12K, as the plan hoped: the built-ins, their error messages and the bridge stay there.  The library grew to 13,610 bytes.
* **Tests**:
  * `tests/hylang`'s `numbers.dl` and `run.dl` are danlang's (`feature/numbers`, `76607f9`): the suite is 1,424 checks (`hysuite5` 1,232), none failing.
  * The `hylang` test: two answers changed to danlang's (`(to-str 255 "x")` is `FF`, `(to-str 1/2 "#b")` is `#b0.1`); `heap`: 24 free pages, the immediates' 8.
  * The `numbers` test: 5,154 calls (136 more): `SET_BASE`'s answer for each of 79 base strings, and `PARSE`'s whole texts (junk, spaces, 255 characters and past, numbers written in a base and read back).
  * The whole suite: 87 tests, all passing.
  * **The cross-check** (`sim/tools/hyxcheck.js`, new): random number expressions (the tower's arithmetic past a fixnum and back, the edges of a fixnum, the order, the conversions and tests, the bits, numbers written in every base and read back, texts that may not be numbers, `format`'s placeholders, `fib`, `pow`, the stored format), printed a line each by danlang and by hylang in the emulator, from a card, compared byte for byte: 2,500 (its seed, 2100) and 4,000 more (seed 77), none different.
* **Speed**: the 20 benchmarks (`sim/bench.js --vs`, against step 2's tree): 17,690 ms, 17,730 before, each the same or quicker; none of them goes past a fixnum.  Past one, in ticks (200 a second), step 2's hylang and this one:

| Work | Before | Now |
| :--- | -----: | --: |
| 1/1 + 1/2 + ... + 1/39 | 299 | 137 |
| A product of 30 fixed decimals | 648 | 67 |
| 200!, in hexadecimal | 1,482 | 1,111 |
| 200!, in decimal | 806 | 963 |
| 300 sums of 21-digit integers | 246 | 314 |
| 300 orders of 20-digit integers | 108 | 152 |
| 200 shifts and xors of 31-digit integers | 153 | 248 |
| 20 numbers of 60 digits read | 323 | 373 |

  Rationals and fixed decimals are much quicker (the library's binary gcd, its scaled digits); an integer a little past a fixnum is a quarter to two-thirds slower, as the plan expected: each operation is a call (its operands staged and copied in, the call and the zero page, the result copied out and made a cell and a blob), where hylang's registers were its own.  Profiled (`(+ a b)`, `(< a b)`, 20 digits): the library a third of the time, half of it moving numbers in and out; the collector and the heap another third.

### **As built: step 4**

Done, on `reborn-numbers` (not merged yet), and danlang's `feature/numbers` (`8ba1680`): the math functions.

* **What they give**: `sqrt`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, `pi` and `pow` of any real power are exact when the answer is (a perfect square's root, of `x`'s kind: `(sqrt 9/4)` is `3/2`, `(sqrt 2.25)` `1.5`; `(exp 0)` 1, `(log 1)` 0, `(sin 0)` 0; a whole power as the tower multiplies; a power whose root is rational: `(pow 8 1/3)` is 2, `(pow 0.25 0.5)` 0.5); else a fixed decimal of the precision's significant digits, **correctly rounded**: `(pi)` is `3.14159265359`, `(exp 100)` `26881171418200000000000000000000000000000000` (12 significant digits, the rest 0s), `(exp -100)` 0.0000...372007597602.  `(digits)` is the precision, 12 at the start; `(digits n)` sets it, 1 to 100 (the plan had 255: past 100 a product of the working numbers can pass 255 bytes).  A negative number's square root or logarithm is complex (`(sqrt -4)` is `2i`, `(log -1)` `3.14159265359i`); `(log 0)`, a power of a negative number that isn't whole, and `exp` (or a power's `y log x`) more than 20000 from 0 are errors.  `pow` is a built-in now in both languages (`globals.dl`'s took whole powers only).
* **Correctly rounded, so every implementation agrees**: a function's value is worked in binary fixed point, an integer `A` standing for `A / 2^b` with a proven bound `E` on its error (and times `10^k` for `exp`'s reduction); it's rounded to the precision only if everything from `A - E` to `A + E` rounds the same, else worked again with more bits (Ziv's way).  So the digits are the true value's, rounded, whatever way it's worked: danlang's C# (`NumMath.cs`), the JavaScript reference (`numref.js`) and the library each have their own guard bits and their own ways (atan by halvings in danlang, by atan(1/m) in the library), and give the same digits.  The ways: `exp` reduces by `k log 10` (so a result's decimal exponent is `k`) and `exp(r/256)^256` by its series; `log` by `t log 2` and `2 atanh((f-1)/(f+1))`, `f` from 3/4 to 3/2; `sin` and `cos` by `k pi/2`, their series, the quadrant's; `tan` their quotient with its own bound; `atan` past 1 by `pi/2 - atan(1/x)`, then `atan(1/m) + atan((my-1)/(m+y))`; `pi` Machin's; `log 2` and `log 10` by `atanh(1/3)` and `atanh(1/9)`; `pow` `exp(y log x)` with `log x` at more bits; `sqrt` exactly in integers (`round(sqrt(PQ)/Q)`).
* **danlang first** (`NumMath.cs`, the built-ins, `math.dl`: 106 checks; the suite 1,530, none failing).  Checked two more ways: 1,500 random calls at 12 digits against the same at 45 rounded to 12 (none different), and against doubles (the differences all the doubles' own).  `numxcheck.js` has 1,208 math cases more (precisions 1 to 35): `numref.js` and danlang the same in all 12,451.
* **The math library** (`modules/math`, a library module of its own, `HT_LIBRARY`, its jump table spec/numbers.def's `math`): it assembles the numbers library's registers, tower and whole powers in its own bank (`numbers/nmcall.inc`, `nmreg.inc`, `nmval.inc`, and `nmpow.inc`, POW taken out of `nmconv.inc` for it), and works in the same RAM bank (`r13`): the state at its start is both's (the precision is the state's `st_digits`).  Its own work uses the digits' pages as registers R24-R31 as well (their lengths and signs: `NRALL`, 32).  `mtcore.inc`: the registers' arithmetic at a number of bits (a product shifted down, a 16-bit divisor), the constants, the rounding (`m_sig`, `m_round`) and the loop (`m_ziv`); `mtfun.inc`: the cores and the eight entries.  Its code: 15K of the bank's 16K.
* **Speed**, at 12 digits and 3.58 MHz, a call after the first (the test program's own reading and writing of each call counted): `pi` 40,000 cycles (11 ms), `log 2` 80,000, `atan` 140,000-160,000, `sqrt 2` 180,000, `log 10` 180,000, `sin` and `cos` 220,000 (62 ms), `exp` 280,000 (78 ms), `pow 2 0.5` 340,000, `tan` 380,000, `pow 1.5 2.5` 680,000 (190 ms).  The first version took up to six times as long (`exp 1` 1.3M cycles, `sin 1` 900,000, `pow 2 0.5` 2.2M); what made the difference:
  * Constants are kept: pi, log 2 and log 10 are made once, at 192 bits or more (the state, `st_const`, 312 bytes: INIT empties it), and shifted down to what a call needs.
  * A byte's product by quarter squares (`ab = f(a + b) - f(|a - b|)`, `f(n) = n^2/4`: a table of 512, 1K in each library's bank): `mul8`, both libraries', 2.5 times quicker, so every product.
  * `sin` and `cos` work only the series their quadrant needs (`tan` both), and divide a term by `(2i)(2i + 1)` at once; a divisor under 256 is the quicker byte division's.
  * Guard bits as the bounds need, not more (exp's 24, log's and atan's 14, trig's 16; a power's `log x` at `b + bits(y) + 32`), and `b` the precision's bits and 10.
  * The plan's budget for these was to be set here: these are it, the `numbers` test's math calls run at them.
* **hylang**: the ten built-ins (`sqrt` ... `pow`, `digits`) in the second table (269 built-ins now), on the math library (found as hylang starts, as `numbers` is), with danlang's messages.  The second table's entries are in hylang's second bank now (`bi_xinfo`: the first is full, 9 bytes left), and the bytecode machine's copy of the table is the first 256 alone.  `globals.hl` is danlang's `globals.dl` again (its `pow` gone).  Its suite has `math.dl`, and `run.dl` and `library.dl` danlang's: 1,530 checks (`hysuite5` 1,338), none failing.  `hyxcheck.js` has the math functions too (at precisions 1 to 30): 6,500 expressions, about 600 of them math, none different.
* **Tests**: the `numbers` test has 371 math calls more (`t_num` finds and calls the math library too: an op of `$80` and its slot): hand-picked ones (exact roots, errors, the edges), and random ones at precisions 1, 3, 12, 20 and 30, each against `numref.js`: 5,525 calls, every one the same.  Run apart, 5,131 random calls more (two seeds; every function, arguments to 30 digits and 30 places, precisions 1 to 40), against `numref.js` too: none different.  The whole suite: 87 tests, all passing.

### **As built: step 5**

Done, on `reborn-numbers` (not merged yet): HyForth's numbers, `lib numbers` (`forthlib/numbers.s`, `/lib/forth/numbers.fl`, 7,001 bytes), the math library's words in it too (one `lib` for both; a second library would have repeated the number stack's code).

* **The number stack**: a RAM bank of the task's (`BANKS_ALLOC`'d as the library loads, with the libraries' bank, `r13`), its numbers one after another from `$8000`, each in the stored format, a table of the library's saying where each starts (64 at most: past that, or past the bank's 8K, THROW -44; too few, -45).  A word selects the bank only while it works (the word that called it may be a definition's in a code bank), and gives the libraries the numbers where they are (`r0`, `r1` in the bank: the library reads the caller's bank at `$8000`), its result written after the top and moved down into place.  A string a word is given is copied out of the caller's memory first, as it may be in a bank too.
* **The words**: the plan's, and the other HyForth names in `spec/numbers.def` (`n/mod`, the integers' quotient and remainder; `ngcd`, `nfloor`, `nround`, `nseed`); `n>`, `n0<`, `n.s` and `nliteral` beside them.  `n>s` and `n>d` give the integer part (as `F>S` truncates): -32768 to 65535, or 32 bits signed or not, else THROW -11.  The comparisons and the tests (`int?` ...) take their numbers off.  `>n ( c-addr u -- flag )` gives a flag, as `>FLOAT` does (the plan's had none): the number pushed if the whole text is one, in the base (`1/0` THROWs -10).  `npow` is the math library's `RPOW` (any real power; without the library, `POW`'s whole powers).  `digits` is a variable, given the math library (`DIGITS`) before a math word or `nrandom` when it has changed (1 to 100, else -24).  `nvariable` is a cell: 0 (the number 0), or a block in `memory.fl`'s heap (`resize`d to each number stored: its length, its bytes), which `numbers.fl` loads if it isn't there.  `nconstant` and `nvalue` keep their number after their code, as `2constant` does; `to` knows an `nvalue` by its code, as it knows a `2value` (`coreext.s`, by the core's `num_dov` and `num_to`).
* **Literals**: the interpreter gives a word that isn't a word, a cell or a double to the library before it THROWs -13 (`num_call` 0, through `num_vec`, as `loc_vec` is the locals library's), which reads the whole word as a program's text (a bare number starts with a digit), in the base: it's pushed, or compiled, its bytes after a `jsr` (their length, then the bytes), or, in a code bank, after a `jsr` to `nlit_p` and their address, the bytes in the dictionary (as `S"`'s string is).  So `2/3`, `1.25`, `#xFF`, `#16r1F`, `2i` and `1+2i` are literals.  (`see` shows one as a call and bytes: it doesn't know the library's code.)  With the library loaded, a cell past 16 bits (`70000`: -32768 to 65535 are cells) or a double past 32 (`4294967296.`; -2147483648 at most below 0) is a number too, where Forth wrapped it: `>NUMBER`'s multiply notes the overflow (`num_ovf`).
* **The base**: BASE is the numbers' base too.  When it changes, and whenever `hex` or `decimal` is said (the core's `num_base` 0: so `s" <d" set-base` then `decimal` is plain decimal again), the library's base is set from it (`d`, `x`, `b`, `o`, `Nr`) before a number is next read or shown.  `set-base ( c-addr u -- )` gives the library any base string (not one: -24) and makes BASE its digits' count (36 at most).  If it isn't a radix as BASE has one (a prefix shown, a modifier, balanced, least digit first, digits of its own, past 36), the core's `number`, `n_text`, `u_text` and `d_text` (so `.`, `u.`, `.r`, `u.r`, `d.`, `d.r`, `.s`) ask the library (`num_custom`): cells are read and shown in it too, and a number with a Forth prefix (`$FF`, `%101`, `#10`) is Forth's as ever.  `get-base ( -- c-addr u )`.
* **Text**: `n.` and `n.base` write into the bank after the top and type it a buffer at a time; `n>str` gives it in a buffer of the library's (1,040 characters at most: -17); `nformat` counts its placeholders as `FORMAT` reads them (`{{` and `}}` braces, a `{` with no `}` after it itself), takes that many numbers (the deepest the first) and types the text.
* **Errors**: `NE_BIG` is -11 (result out of range), `NE_DIV0` -10, `NE_ROOM` -44 (number stack overflow), `NE_DOMAIN` -46 (outside the function's domain), the rest -24; too few numbers, -45.  The core has the four texts (-11, -44, -45, -46).
* **The core**: 289 bytes more (14,608 of its bank's 16K): `num_call` and its vectors (a `MARKER` that takes the library out clears them), the interpreter's hook, `number`'s ranges and hook, `n_text`'s and `u_text`'s hooks (and `double.fl`'s `d_text`'s), `set_base`'s `num_base`, `>NUMBER`'s overflow, the error texts.
* **Speed** (200 ticks a second): `1 s>n n+` 1.7 ms (two library calls, about 3,000 cycles each), `1/7 n+` on a rational 5.4 ms, `nsqrt` of 2 at 12 digits 49 ms (the math library's own time).
* **Tests**: `fnumbers` (new): `tests/forth/numberstest.fth`, in the Forth 2012 suite's way (tester.fr's `T{ ... -> ... }T`, a number checked by its text): every word, literals of every kind (and Forth's cells and doubles still Forth's), definitions with code banks and without, the bases (a radix, a prefix, balanced, least digit first, digits of one's own), the errors; then `n.`, `n.base`, `n.s`, `nformat` and cells in bases that aren't a radix, typed and compared.  The whole suite: 88 tests, all passing (`rom`'s budget 250M cycles: the ROM disk 7K more, 1,646 of its volume's 1,664 blocks used).
