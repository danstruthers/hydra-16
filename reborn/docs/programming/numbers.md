# Numbers

The Hydra has one number system, and every language has all of it: integers of any size (to 255 bytes, 614
digits), fixed decimals (`1.25`), rationals (`2/3`) and complex numbers (`1+2i`), all exact; the math functions
exact when their answer is, else correctly rounded to a precision (12 significant digits at the start); and any base,
for reading and writing.  hylang, HyForth (`lib numbers`), BASIC, C and assembly all work numbers by the same two
library modules, `numbers` and `math`, on the same bytes, so a number one writes is the same number in another.
This chapter is that layer; [../design/plans/NUMBERS.md](../design/plans/NUMBERS.md) is its design and why.

Contents: [The stored format](#the-stored-format) · [The libraries](#the-libraries) · [Calling them](#calling-them) ·
[The entries](#the-entries) · [Errors](#errors) · [Bases and text](#bases-and-text) ·
[In each language](#in-each-language)

## The stored format

A number is a tag byte, and what the tag says follows (`spec/numbers.def`; integers' bytes least first):

| Tag | A number | Then |
| :-- | :------- | :--- |
| `$00`-`$7F` | An integer -64 to 63: the tag itself, 7-bit two's complement | Nothing |
| `$80`-`$8F`, `$90`-`$9F` | A positive or negative integer of n bytes (the tag's low 4 bits: n - 1) | Its magnitude |
| `$A0`, `$A1` | A positive or negative integer of 17 to 255 bytes | Its length, then its magnitude |
| `$B0`-`$BF` | A fixed decimal of 0 to 15 places (the tag's low 4 bits) | Its digits: an integer |
| `$C0` | A fixed decimal of 16 to 65,535 places | Its places (2 bytes), then its digits |
| `$C1` | A rational, in lowest terms, its denominator above 1 | Its numerator, then its denominator |
| `$C2` | A complex number, its imaginary part not 0 | Its real part, then its imaginary part |
| `$FF` | Never a number: a program's own (BASIC's values use it) | |

Each number has one form (an integer in its shortest, a fixed decimal without 0s at its digits' end), so two numbers
of one kind are equal exactly when their bytes are; `1` and `1.0` are of two kinds and equal in value, so a program
asks the library (`CMP`).  Most numbers are 1 to 5 bytes: `100` is `80 64`, `0.5` is `B1 05`, `1/3` is `C1 01 03`,
`1+2i` is `C2 01 02`.  `NUM_MAX` (1,040) is the longest there can be.  This is what hylang's heap, HyForth's number
stack, BASIC's values and heap and a C program's `num_t` arrays hold.

## The libraries

* **`numbers`** (a library module, `HT_LIBRARY`, a bank of the paged ROM): the tower's arithmetic, conversions,
  text, the bits of integers of any size, random numbers, Fibonacci.
* **`math`** (a second library module): the functions (`SQRT`, `EXP`, `LOG`, `TRIG`, `ATAN`, `RPOW`, `PI`) and their
  precision (`DIGITS`).  It calls nothing of `numbers`'s: it assembles the same registers and tower in its own bank.

A library has no RAM of its own, and a program's zero page and low RAM are the program's, so **the libraries work in
a RAM bank the program gives them**: one bank (8K) of its own (`BANKS_ALLOC`), the same for its life, named in `r13`
at every call.  `INIT` fills it once (the base decimal, the precision 12 digits, the random generator seeded from the
clock), and every other entry refuses a bank `INIT` hasn't filled (`NE_INIT`).  A call selects that bank at `$8000`
and gives the caller's back as it ends; it keeps its registers there (eight 256-byte pages: a number's bytes, its
length and sign), its state between calls, and an arena for the call's numbers.  A call uses the zero page's
`$70`-`$7F` too, and saves them as it starts and puts them back as it ends, so the program's zero page stays its own.

## Calling them

A library is called with `XCALL` ([modules.md](modules.md)): `r15` the entry's address (`NUM_ADD` ...: the
library's jump table, after its header), `r14` the library's bank (`MODINFO` finds it by its name, `"numbers"` or
`"math"`), `r13` the libraries' RAM bank.  The rest are the entry's:

| Registers | |
| :-------- | :- |
| `r0`, `r1` | The operands: numbers' addresses, anywhere in the caller's memory but its paged ROM (at `$8000`-`$9FFF`: the bank the caller has there) |
| `r2`, `r3` | The result's place and the room it has (a result may be written over an operand: they're read first) |
| `r4` | A base string for the text entries (0: the base the library holds) |
| `.Y` | A few entries' options (`PARSE`'s `NPARSE_` flags, `BITS`'s operation, `TRIG`'s function) |
| Back | C = 0: `.A`/`.X` the result's length; C = 1: `.A` an error (`NE_`) |

In assembly, `numbers.inc` (`sdk/asm`, made from `spec/numbers.def` by the build) has each entry's address and its
registers, and the macros `NUMCALL entry` and `MATHCALL entry`, which load `r13`-`r15` from three bytes of the
program's: `num_bank`, `num_mod` and `math_mod`.  `numlib.s`'s `num_open` fills those (`MODINFO`, `BANKS_ALLOC`,
`INIT`) as the program starts.  The sample `nsum` (`sdk/asm/samples/nsum`) sums the numbers it's given:

```
            jsr         num_open                            ; The libraries (C = 1: .A the error)
            ...
            stz         r4                                  ; Read in the base (r4 = 0), the whole word a number
            stz         r4 + 1
            LDR         r2, num
            LDR         r3, NUM_MAX
            ldy         #NPARSE_WHOLE
            NUMCALL     NUM_PARSE                           ; (r0, r1: the text and its length)
            ...
            LDR         r0, sum                             ; sum = sum + num
            LDR         r1, num
            LDR         r2, sum
            NUMCALL     NUM_ADD
```

A call costs a few hundred cycles before its work (`XCALL` some 120, the zero page's saving some 200), so a language
keeps its quickest numbers itself where it can: hylang's fixnums, BASIC's integers of 32 bits, HyForth's cells, C's
`int`s, each handed to the library when it's past them.

## The entries

| | `numbers`'s |
| :- | :- |
| Its bank | `INIT`; `SET_BASE`, `GET_BASE` (the base, a string) |
| Text | `PARSE` (the longest start of a text that's a number, or with `NPARSE_WHOLE` the whole text), `DISPLAY` (a number written exactly), `FORMAT` (a format string's `{}` placeholders filled, danlang's `format`) |
| Arithmetic | `ADD`, `SUB`, `MUL`, `DIV` (exact: `10/4` is `5/2`), `NEG`, `ABS`, `CMP` (by value, whatever the kinds), `KIND` (`NK_`: an integer, a fixed decimal, a rational, a complex number; and its sign), `IDIV` (an integer quotient toward 0, and the remainder), `GCD`, `POW` (a whole power) |
| Conversions | `TRUNCATE`, `FLOOR`, `ROUND` (a half to the even one), `TO_FIXED` (places, cut short), `TO_RATIONAL`, `NUMERATOR`, `DENOMINATOR`, `COMPLEX`, `PART` (a complex number's), `FROM_INT` and `TO_INT` (16 or 32 bits: cells, `PEEK`, array sizes), `BYTES` (bytes checked as a number) |
| Bits | `BITS`: and, or, xor, not, shifts, a bit's test (`NBIT_`), on integers of any size in two's complement |
| Others | `RANDOM` (an integer below n, or a fixed decimal from 0 to 1) and `SEED`; `FIB` |

| | `math`'s |
| :- | :- |
| The functions | `SQRT`, `EXP`, `LOG`, `TRIG` (sine, cosine, tangent), `ATAN`, `RPOW` (any real power), `PI` |
| The precision | `DIGITS`: 1 to 100 significant digits, 12 at the start |

The functions are exact when their answer is (`SQRT` of `9/4` is `3/2`, `RPOW` of 8 and `1/3` is 2, `EXP` of 0 is
1), else a fixed decimal of the precision's significant digits, correctly rounded (a bound on the error, and more
bits till the rounding is sure), so the Hydra, danlang and the PC's reference (`sim/tools/numfmt.js`) agree to the
last digit.  A negative number's square root or logarithm is complex.  At 12 digits a sine takes some 220,000
cycles, an exponential 280,000.

## Errors

| Error | |
| :---- | :- |
| `NE_BIG` | Too big: an integer past 255 bytes, a fixed decimal past 65,535 places |
| `NE_DIV0` | Division by zero |
| `NE_NOTNUM` | Not a number: text that isn't one, bytes that aren't one in the stored format |
| `NE_ROOM` | No room for the result in the place given |
| `NE_DOMAIN` | Outside the function's domain (`LOG` of 0 ...) |
| `NE_BASE` | Not a base: a base string that names none |
| `NE_INT`, `NE_REAL` | An integer is needed; a real number, not a complex one |
| `NE_INIT` | The bank in `r13` isn't one `INIT` filled |
| `NE_FORMAT` | A format's placeholders and its arguments don't match |

Each language says them its own way: hylang's and HyForth's errors, BASIC's codes (`NE_DIV0` is its division by
zero, `NE_BIG` its overflow), C's `num_error` and `num_strerror`.

## Bases and text

The library holds the base numbers are read and written in (`SET_BASE`), and a text entry can name one of its own in
`r4`.  A base is a string, hylang's: `d` decimal, `x` hexadecimal written bare (`FF`), `#x` with its prefix (`#xFF`),
`b`, `o`, `c` (balanced ternary), `16r` (a radix), `<x` (least digit first), `[01]` (digits of its own), and the
rest of hylang's nineteen named bases and their modifiers ([../using/hylang.md](../using/hylang.md), "Numbers").

* **Reading** (`PARSE`): every form danlang reads, in the base: integers, fractions (`1/3`), a radix point (`0.8` in
  hexadecimal is a half), complex numbers (`1+2i`, `2i`), and a number with a base of its own (`#xFF`, `#b0.1`,
  `#16r1F`).  `NPARSE_PROGRAM` is a program's text's rule: a bare number starts with a digit (`0FF`), so `FF` is a
  name.
* **Writing** (`DISPLAY`): exactly, in the base: a fraction or a fixed decimal with a radix point when it ends in
  that base, else as a fraction (`0.1` in hexadecimal is `1/A`), so what's written reads back as the same number.
* **Formats** (`FORMAT`): `{}` a value in the base, `{x}`, `{#b}`, `{c}` ... in one named; hylang's `format`,
  HyForth's `nformat`, BASIC's `PRINT USING "{}"`.  C's `printf` writes numbers with `%N` and takes a base in braces
  (`%{x}N`, `%{#b}d`) on the same entries.

## In each language

| Language | Its numbers |
| :------- | :---------- |
| hylang | Every number is one: `(/ 10 4)` is `5/2`, `(sqrt 2)`, `(base "x")`; `number-bytes` and `bytes-number` the stored format ([../using/hylang.md](../using/hylang.md)) |
| HyForth | `lib numbers`: a number stack of its own (`n+`, `n.`, `nsqrt`, `nformat`); a word that's a number past a cell goes on it; `BASE` and `set-base` the library's base ([../using/hyforth.md](../using/hyforth.md)) |
| BASIC | Every number is one, of one type: `PRINT 1 / 3` is `1/3`, `BASE "x"`, `STR$(x, "b")`, `DIGITS 30` ([../using/basic.md](../using/basic.md)) |
| C | `num.h`: a function for each entry (`num_add (dst, room, a, b)` ...), `printf`'s `%N` and `scanf`'s ([../../sdk/c/README.md](../../sdk/c/README.md), "Numbers") |
| Assembly | `numbers.inc`'s `NUMCALL` and `MATHCALL`, `numlib.s`'s `num_open`; the sample `nsum` ([../../sdk/asm/README.md](../../sdk/asm/README.md)) |
| rc | `calc`: `calc 2/3 + 0.5` is `7/6`, `calc -b x 255` is `FF` ([../using/tools.md](../using/tools.md)) |
