## **Numbers: one number system, one format and one library for every language**

A plan (October 2026) for the user's request: BASIC on hylang's number system instead of Microsoft's 40-bit floating point, and the same numbers in HyForth.  The simplest way to give every language the same numbers is the one the user named: **shared libraries** in the paged ROM that every language calls (`numbers`, the number system, and `math`, its functions), and **a new compact binary format** that every language stores numbers in and passes them in.  BASIC is written again on top of them: a new BASIC for the Hydra, QuickBASIC's kind, line numbers optional and labels to branch to, as the user asked ([BASIC.md](BASIC.md), its plan).  And every language gets all of it, as the user asked: all of danlang's and hylang's number system, its 19 named bases and every other way of writing a number included; numbers written in any of those formats; **a current format**, set by the program, that numbers are shown in and read in; fractions and fixed decimals in every format; and behind the scenes, every number stored in the new compact format, whatever format it was written or is shown in.

### **Contents**
1. [What each language has now](#what-each-language-has-now)
2. [The target](#the-target)
3. [All of it, in every language](#all-of-it-in-every-language)
4. [The current format: what numbers are shown and read in](#the-current-format-what-numbers-are-shown-and-read-in)
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
* **A current format** in each language, the program's to set: any of danlang's (a named base, a radix, digits of its own, balanced, least digit first ...), used to show numbers and to read them; fractions and fixed decimals in it as well as integers.
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

**Writing**: as danlang's `print` writes it (`42`, `-1.25`, `2/3`, `1+2i`), or in any of those bases (`to-str`'s: `#xFF`; a rational `#x1/#x3`; a fixed decimal as a rational; a complex number has no form in another base).  danlang reads no complex number from text (`(val "1+2i")` is an error, and `1+2i` is a symbol): they're made by `complex` and by arithmetic (a question below).

**The rest**: the arithmetic and comparisons; `abs`, `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `random`, `fib`, `pow`; the tests (`num?`, `int?`, `fixed?`, `rational?`, `complex?`); the bits, on integers of any size in two's complement (`bit-and`, `bit-or`, `bit-xor`, `bit-not`, `shl`, `shr`, `bit?`, `hex`, `bin`, `lo`, `hi`, `word`, `bytes`, `from-bytes`); and the math functions (below).

| | hylang | BASIC | HyForth | C |
| :- | :- | :- | :- | :- |
| A number in a program's text | Every form, as now | Every form (`X = #xFF + #b0.1`), but a `#` that names a file (`PRINT #1, X`); and QuickBASIC's too: `&HFF`, `&O17`, `&B101`, exponents (`1E6`, `2.5E-3`, exact) | A word Forth doesn't read as a cell or a double (`42`, `$FF`, `123.` stay what they are) but danlang reads as a number: `1.25`, `2/3`, `#xFF`, `#16r1F`, `#c+-0`, `100000000000000000000`, onto the number stack | Text, through `num_parse` |
| Text to a number | `(val s)` | `VAL(S$)` | `>n ( c-addr u -- )` | `num_parse` |
| A number written | `print`, `(to-str x)` | `PRINT`, `STR$(X)` | `n.`, `n>str` | `num_print` |
| In a base | `(to-str x base)` | `STR$(X, B$)` (`STR$(255, "x")` is `#xFF`) | `n.base ( c-addr u -- )` (`s" x" n.base`) | `num_print`'s base |
| The tower's functions | As now | A function each (named in [BASIC.md](BASIC.md)'s reference) | `nabs`, `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `nrandom`, `nfib`, `npow` | `num_abs`, `num_truncate`, `num_to_fixed` ... |
| The tests | `num?` `int?` `fixed?` `rational?` `complex?` | A function each | `int?` `fixed?` `rational?` `complex?` (the number stack's top, a flag) | `num_kind` |
| The bits | As now | `AND`, `OR`, `NOT` on integers of any size, and a function each for the rest | `nand`, `nor`, `nxor`, `ninvert`, `nlshift`, `nrshift`, `nbit?`, `nbytes`, `nfrom-bytes` | `num_and` ... |

### **The current format: what numbers are shown and read in**

Each language has a current format, which the program sets and reads: any base danlang can name (the 19 letters, `#16r`, `#[01]` and the rest, with the modifiers), as `to-str`'s base strings name them (`"x"`, `"#16r"`, `"c"`, `"#<x"`, `"#[01]"`).  Decimal at the start.

* **Shown**: every number written without a base given (`print`, `PRINT`, `n.`, `num_print` with none, the REPL's values, `STR$(X)`) is written in the current format.  A base given (`(to-str x "b")`, `STR$(X, "b")`, `n.base`) still wins.
* **Read**: every number read from text without a `#` of its own (`val`, `VAL`, `INPUT`, `>n`, `num_parse`, `read-line`'s numbers as a program converts them) is read in the current format; a number with a `#` of its own (`#xFF`) in its own.  Whether a program's own text (BASIC's lines, hylang's source) is read in it too is a question below: Forth's is (the current format is to the number stack what `BASE` is to cells), but a library read while a program has set hexadecimal would mean something else.
* **Integers, fractions and fixed decimals, in every format**: `ff`, `-1A`; a rational, its parts in the format (`1/3`, in hexadecimal `1/3`, in binary `1/11`); a number with a radix point (`1.8` in hexadecimal is `3/2`, `0.1` in binary is `1/2`), read exactly.  A fixed decimal shown in a format other than decimal: with a radix point when it ends in that base (`0.5` in binary is `0.1`, in hexadecimal `0.8`), else as a fraction (`0.1` in hexadecimal is `1/A`): exact either way, so what's shown reads back as the same number.  (danlang writes a fixed decimal in another base as a rational only: the radix point is new, danlang's first.)
* **Balanced and signed formats**: a balanced format's numbers have no sign (`#c+-0` is -2: its digits least first, 1 - 3 + 0), a negative base's none either; a radix point in them as danlang reads one.
* **The prefix**: shown as `FF`, as Forth shows a number in `HEX`, so that it reads back in the same format; or as `#xFF`, which reads back in any (a question below).
* **Complex numbers**: shown in decimal whatever the format, as danlang has no complex number in another base (`1+2i`).

The settings, a name each (named alike in every language, as the rest are; the names a question below):

| | hylang | BASIC | HyForth | C |
| :- | :- | :- | :- | :- |
| Set it | `(number-base "x")` | `NBASE "x"` | `s" x" nbase!` | `num_base ("x")` |
| Read it | `(number-base)`: `"x"` | `NBASE$` | `nbase@ ( -- c-addr u )` | `num_base (0)` |

The libraries keep no setting: each call to read or write text is given its format, and a language passes its current one.

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

**Every number stored is stored so**: in BASIC's variables, arrays and heap, on HyForth's number stack and in its variables, in hylang's heap, in a C program's buffers, and in what `SAVE`, `save` and the like write as binary.  The one place a number isn't bytes of the format is hylang's fixnum: a small integer is its 16-bit value itself (bit 0 set), as now, so that its loops stay quick; it becomes the format's bytes (`00`-`7F`, `80`-`81 ...`) the moment it's handed to the libraries (a question below).

### **The libraries**

**`numbers`** (a library module, `HT_LIBRARY`, a bank of the paged ROM): the tower, from hylang's `numreg.inc`, `numval.inc`, `numtext.inc`, `numbi.inc` and `numbits.inc`, taken out of hylang and made to work on the format instead of hylang's heap:

* Arithmetic: add, subtract, multiply, divide (exact), negate, absolute value, compare (-1, 0, 1), sign, equal; whole powers; quotient and remainder of integers; gcd.
* Conversions: integer part (`truncate`), floor, round; `to-fixed` (places), `to-rational`; a rational's numerator and denominator; `complex`, and a complex number's parts; to and from 16- and 32-bit integers (for cells, `PEEK`, array indexes).
* Text: a number read in danlang's grammar, every form of it, and written as danlang writes it, or in any base (the section above).  BASIC's exponents (`1E6`) as an option of the reader's, BASIC's alone.
* Bits, on integers of any size, in two's complement: and, or, xor, not, shifts, a bit's test.
* Random numbers: xorshift32 (hylang's), its state the caller's: an integer below n, or a fixed decimal from 0 to 1.

**`math`** (a second library module): the functions (next section).  It calls `numbers` for its arithmetic.

**The interface**, as the system calls are (and made from a specification the same way, `spec/numbers.def`, so that each language's bindings are made, not written):

* A library keeps a jump table after its header; a caller finds its bank with `MODINFO` as it starts (by name), and calls a routine with `XCALL` (`r15` the routine's address, `r14` the bank).
* The arguments are in `r0`-`r13`, as a system call's are: the operands' addresses (numbers in the format, wherever the caller has them: its RAM, or the bank it has at `$8000`), the result's address and how much room it has, the precision (for `math`), a base (for text).  The answer: C clear, and the result's length in `.A`; or C set, and an error in `.A` (too big, division by zero, not a number, no room for the result, a domain error).
* **Workspace**: the libraries work in registers, as hylang's number code does (a register is a 256-byte page, its bytes least first, with a length and a sign), so a sum or product of long numbers makes nothing till the result is written.  The caller lends the pages (eight, 2K) and names them in `r13`: hylang its reader's scratch pages, as now; BASIC and HyForth pages of their own.
* **Zero page**: the libraries use 16 bytes of the program's zero page, `$70`-`$7F`, as scratch during a call.  A program that calls them keeps nothing there across a call (the conventions to say so).
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
* **Otherwise a fixed decimal** of the precision's significant digits, correctly rounded: argument reduction, then series in fixed point with guard digits.  The default precision is a question below (12 significant digits would be three more than Microsoft's 9).
* **danlang first**: the same algorithms in danlang (C#, `System.Numerics.BigInteger`), so the cross-check (random expressions, run in both) stays exact to the last digit, as it is for the arithmetic now.
* A negative square root or logarithm is a complex number, in every language (the tower has them: `(sqrt -1)` is `i`).

### **hylang and danlang**

* **danlang**: the math functions and `digits` first, as the reference; and the format, as two built-ins (`(to-bytes n)` and `(from-bytes b)`, beside the bytes built-ins hylang has), so the cross-check can compare bytes.
* **hylang**: its number objects hold the format (a bignum's blob, a fixed decimal's, a rational's and a complex number's become one blob of the format's bytes, in its own banks), and its number built-ins call the libraries.  Its quick ways for fixnums stay in hylang (`+`, `-`, `*`, the comparisons, the native code's templates), so its loops are as fast as now; only numbers past a fixnum go to the library.  Its third bank gets most of its 12K back.  `numbers.dl` and the 2,100 random expressions must give the same as before, byte for byte, and the benchmarks no slower.
* The new built-ins: `sqrt`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, `pi`, `digits`; `pow` takes any real exponent.

### **BASIC**

A new BASIC, for the Hydra, as QuickBASIC was for the PC: line numbers optional, labels, blocks, `SUB` and `FUNCTION`; its own plan, [BASIC.md](BASIC.md).  Its numbers are these:

* **A value is 5 bytes**, holding the stored format's bytes when the number fits in 5 (most do: integers to 2^32, `0.5`, `3.14159`, `1/3`), else `$FF` and a reference (the number's address and length) to its bytes in the heap the strings use.  So variables, arrays and `FOR` take no more room than Microsoft's 5-byte floats did.
* **One number type** ([BASIC.md](BASIC.md)): QuickBASIC's suffixes and type names (`%`, `&`, `!`, `#`, `AS INTEGER` ...) are accepted, and every one is the same exact number, stored in the compact format.
* **Exact**: `10/4` is `5/2`, `0.1 + 0.2 = 0.3` is true, `2 ^ 100` is exact; `FOR x = 0 TO 1 STEP 0.1` runs 11 times; `PRINT` shows every number exactly (`1/3` as `1/3`).
* **Every format**: numbers in a program's text in every form danlang reads (the 19 named bases and the rest), QuickBASIC's `&HFF`, `&O17`, `&B101`, and exponents (`1E6`, `2.5E-3`, exact); `VAL` and `INPUT` read the same; `STR$(X, "x")` writes in a base; `NBASE "x"` sets the current format, which `PRINT`, `STR$`, `VAL` and `INPUT` use.
* **The functions**: `SQR`, `EXP`, `LOG`, `SIN`, `COS`, `TAN`, `ATN`, `^`, `PI` call `math` at `DIGITS` digits; `INT` (the floor), `FIX`, `ABS`, `SGN`, `RND` (the next number from 0 to 1, a fixed decimal; a negative argument seeds it), `MOD`, `\` (integer division), and the tower's other functions and tests (`TRUNCATE`, `TOFIXED`, `NUMERATOR` ..., named in BASIC.md's reference) call `numbers`.
* **The bits**: `AND`, `OR`, `XOR`, `NOT` on integers of any size, in two's complement (the library's), not 16-bit only; `PEEK`, `POKE` and the like take integers that fit.
* **Quick small integers**: those that fit a value are added, subtracted and compared without a library call (counters, `FOR` loops); the rest go to the libraries.

### **HyForth**

A library, **`lib numbers`** (`numbers.fl`), with `math`'s words in it or in a second, `lib math`:
* **A number stack**, as Forth's floating-point stack is a stack of its own: in a RAM bank of the task's (8K), each entry a number in the format, so a number's bytes are copied as Forth copies cells (no collector needed).
* **Words**, hylang's names where hylang has one: `n+`, `n-`, `n*`, `n/`, `nnegate`, `nabs`, `n=`, `n<`, `n0=`, `ncompare`; `ndup`, `ndrop`, `nswap`, `nover`, `nrot`, `ndepth`; `s>n`, `d>n`, `n>s`, `n>d` (to cells and doubles, if it fits); `n.`, `n.base`, `>n` and `n>str` (below); `truncate`, `to-fixed`, `to-rational`, `rational.n`, `rational.d`, `complex`, `nrandom`, `nfib`; `int?`, `fixed?`, `rational?`, `complex?`; the bits, `nand`, `nor`, `nxor`, `ninvert`, `nlshift`, `nrshift`, `nbit?`, `nbytes`, `nfrom-bytes`; `nsqrt`, `nexp`, `nlog`, `nsin`, `ncos`, `ntan`, `natan`, `npow`, `npi`, and `digits` (a variable).
* **Variables**: `nvariable`, `n@`, `n!`, `nconstant`, `nvalue`; a variable's number in `memory.fl`'s heap (`allocate`d, `resize`d as it grows).
* **Literals**: what Forth reads as a cell or a double stays one (`42`, `$FF`, `#10`, `123.`: HyForth reads a double only by a last `.`); any other word danlang reads as a number goes on the number stack: `1.25`, `2/3`, `#xFF`, `#16r1F`, `#c+-0`, `100000000000000000000`.  So every form of danlang's is a literal, and no Forth program changes.
* **Every base**: `n.` writes in decimal, as `print` does; `n.base ( c-addr u -- )` in the base a string names, as `to-str`'s (`s" x" n.base`); `>n ( c-addr u -- )` reads any form.
* **Optionally the standard's Floating-Point word set** on top (`f+`, `f@`, `fsqrt` ... over the same numbers, a float a fixed-size reference), so standard Forth programs with floats run: a question below, since it reverses October's "left out".

### **C and assembly**

* **C**: `num.h` and `lib/num.c`: `num_add (dst, room, a, b)`, `num_parse`, `num_print`, `num_sqrt (dst, room, a, digits)` ..., numbers in byte arrays in the format; each function an `XCALL` through a small piece of assembly.  A sample, `calc`, in `/rom/sample/c`.
* **Assembly**: `numbers.inc` in the SDK (`sdk/asm`), made from `spec/numbers.def`: the jump table's names and a macro for the call.
* **rc**: a `calc` tool could put the libraries at the prompt (`calc 2/3 + 0.5`): a question below.

### **Tests**

* **The format**: an encoder and decoder in JavaScript (`sim/tools/numfmt.js`) and in danlang, and the cross-check: random expressions, run in danlang and through the libraries (a test module, `t_num`, in the emulator), compared byte for byte, as hylang's 2,100 are now; then the math functions the same way at several precisions.
* **Every format**: random numbers of every kind (integers, fixed decimals, rationals) written in each of the 19 named bases, a few radixes, digits of their own and each modifier, then read back: the same number, and the same text in danlang and through the libraries; and each language's current format set and used to show and read.
* **hylang**: its suites (`hysuite`, `hylang`, `numbers.dl`), unchanged, must pass; `bench` and `hyspeed` no slower.
* **BASIC**: its new suite ([BASIC.md](BASIC.md)), the numbers among its checks.
* **HyForth**: a test file of the number words (as the standard's suite files are), and the Floating-Point suite's file if that word set is built.
* **C**: the `calc` sample at rc.

### **The order of work**

| Step | Work | Size |
| :--- | :--- | :--- |
| 1 | **danlang first, and the interface**: the stored format in danlang (`to-bytes`, `from-bytes`) and JavaScript (an encoder and decoder); danlang's current format (`number-base`) and its radix point in other bases; `spec/numbers.def`; the zero page and workspace conventions; `XCALL`'s cost measured | M |
| 2 | **`numbers`**: hylang's number code taken out into a library on the stored format, its text routines given a format each call (the radix point added); `t_num` and the cross-check | L |
| 3 | **hylang on `numbers`**: its objects in the stored format, its built-ins through the library, `number-base`; its suites, the cross-check and its benchmarks as before | M |
| 4 | **`math`**: danlang's functions first, then the library, and hylang's built-ins (`sqrt` ... `digits`) | M-L |
| 5 | **HyForth's `lib numbers`** (and `lib math`): the number stack, the words, literals in the current format, `nbase!` and `nbase@`, its test file | M |
| 6 | **C and assembly**: `num.h` (with `num_base`), `numbers.inc`, the `calc` sample | S |
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

1. **The precision of the math functions**: the default number of significant digits (12 is three more than Microsoft's 9; more costs time), and whether each language may change it (`DIGITS n`, `(digits n)`, `digits`)?
2. **Complex numbers from text**: danlang reads none (`1+2i` is a symbol, and `(val "1+2i")` an error), so in every language they're made by `complex` and by arithmetic (`SQR(-1)` is `i`).  Add a form to danlang's grammar (and so to every language's), or leave it?
3. **HyForth's Floating-Point word set** over these numbers, so standard Forth programs with floats run (reversing October's "left out")?
4. **hylang on the shared library** (one implementation, recommended), or left with its own copy of the code?
5. **A `calc` tool** at rc?
6. **The current format's names**: `number-base`, `NBASE`, `nbase!` and `nbase@`, `num_base` (the table above), or others?
7. **The current format and a program's own text**: does it read BASIC's lines and hylang's source too (as Forth's `BASE` reads Forth's), or only what a program reads as it runs (`VAL`, `INPUT`, `val`, `>n`), its text always decimal unless a number has a `#` of its own?  (Recommended: Forth's text yes, as `BASE`; BASIC's and hylang's no, so a library or a program means the same whatever was set.)
8. **The prefix when shown**: `FF` (reads back in the same format) or `#xFF` (reads back in any)?
9. **A fraction shown in another base**: with a radix point when it ends in that base and as a fraction when it doesn't (recommended: exact, and reads back), or always as a fraction (danlang's way now)?  (Exact either way: the user's answer, below, rules rounding out.)
10. **hylang's fixnums**: a small integer kept in hylang's 16-bit value as now (quick; the format's bytes when it leaves hylang), or the format's bytes even there?

### **Answers** (the user's, 7 October 2026)

1. **Every number is shown exactly**, in every language: a fraction as a fraction (`1/3`), a long integer whole, never rounded for display (it was question 2: how BASIC prints a fraction).
2. **BASIC has one number type** (BASIC.md's answers): QuickBASIC's number types accepted as names, all the same exact number.
