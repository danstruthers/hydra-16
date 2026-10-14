# Libraries

A **library module** is code in the paged ROM that other programs call, in their own tasks, with their own RAM: it has no
data, BSS or entries of its own.  The system has three: **`numbers`** and **`math`** (the number system every language
shares) and **`asm`** (the W65C02S's instructions: the assembler and the disassembler).  This page is how a library is
called, and each of the system's.

## Contents

1. [XCALL](#xcall)
2. [Writing a library](#writing-a-library)
3. [The number libraries](#the-number-libraries): the stored format, calling them, the entries, errors, bases and text
4. [The asm library](#the-asm-library)

---

## XCALL

**`XCALL`** calls a routine in any bank of the paged ROM, as the Commander X16's `jsrfar` does: `r15` the routine's
address, `r14` (its low byte) the bank it's in.  The caller's bank register (`$01`) is set to the routine's for the call
and back after it; `.A`, `.X`, `.Y`, the flags and `r0`-`r13` pass through both ways.  The routine runs in the caller's
task, with its RAM and zero page.

A program finds a library's bank in the module directory: **`MODINFO`** gives each entry's name and first bank
(`ME_NAME`, `ME_BANK`), so a program looks for `"numbers"` as it starts and keeps the bank.

```
            lda         lib_bank                            ; (MODINFO's ME_BANK, found as it started)
            sta         r14
            LDR         r15, LIB_ADD                        ; The library's routine ($A030: its jump table)
            lda         #3
            ldx         #4
            jsr         XCALL                               ; .A = 7
```

A note may come while the library runs: a program that makes far calls keeps its **note handler in its RAM**, since the
kernel calls it with whatever bank is at `$A000`.

---

## Writing a library

* Its header is `HYX2_LIBRARY "name", "segment", "memory"`; it's a module of its own, listed in `rom.txt`.
* **A jump table at its start**, after its header (at `$A030`), so its routines' addresses don't change as it does.
  `tests/mod/t_lib/t_lib.s` is one, and `t_xcall` calls it.
* It has no RAM of its own: it works in what its caller gives it (addresses in registers, a RAM bank named in a register,
  the caller's zero page saved and put back).
* The system's libraries are made from a specification (`reborn/spec/numbers.def`, `asm.def`): `reborn/tools/apigen.js`
  makes the library's jump table and each language's binding (assembly's `numbers.inc` and `asmlib.inc`, C's `numdefs.h`
  and `asmdefs.h`) from it.
* A program may also have libraries of its own, assembled with it, each a bank with a header (`HYX2_LIBRARY`), called with
  `FARN` ([modules of several banks](assembly.md#modules-of-several-banks)).

---

## The number libraries

The Hydra has one number system ([the numbers](../06-languages/README.md#the-numbers-they-share)), and every language
works it through the same two library modules:

* **`numbers`**: the arithmetic, conversions, text, the bits of integers of any size, random numbers, Fibonacci.
* **`math`**: the functions (`SQRT`, `EXP`, `LOG`, `TRIG`, `ATAN`, `RPOW`, `PI`) and their precision (`DIGITS`).

### The stored format

A number is a tag byte, and what the tag says follows (integers' bytes least significant first):

| Tag | A number | Then |
| :-- | :------- | :--- |
| `$00`-`$7F` | An integer -64 to 63: the tag itself, 7-bit two's complement | Nothing |
| `$80`-`$8F`, `$90`-`$9F` | A positive or negative integer of n bytes (the tag's low 4 bits: n - 1) | Its magnitude |
| `$A0`, `$A1` | A positive or negative integer of 17 to 255 bytes | Its length, then its magnitude |
| `$B0`-`$BF` | A fixed decimal of 0 to 15 places (the tag's low 4 bits) | Its digits: an integer |
| `$C0` | A fixed decimal of 16 to 65,535 places | Its places (2 bytes), then its digits |
| `$C1` | A rational, in lowest terms, its denominator above 1 | Its numerator, then its denominator |
| `$C2` | A complex number, its imaginary part not 0 | Its real part, then its imaginary part |
| `$FF` | Never a number: a program's own (HyBASIC's values use it) | |

Each number has **one form** (an integer in its shortest, a fixed decimal without 0s at its end), so two numbers of one
kind are equal exactly when their bytes are.  Most numbers are 1 to 5 bytes: `100` is `80 64`, `0.5` is `B1 05`, `1/3`
is `C1 01 03`, `1+2i` is `C2 01 02`.  `NUM_MAX` (1,040) is the longest there can be.  This is what hylang's heap,
HyForth's number stack, HyBASIC's values and a C program's `num_t` arrays hold.

### Calling them

A library has no RAM of its own, so **the libraries work in a RAM bank the program gives them**: one bank (8K) of its
own (`BANKS_ALLOC`), the same for its life, named in `r13` at every call.  `INIT` fills it once (the base decimal, the
precision 12 digits, the random generator seeded from the clock).  A call selects that bank at `$8000` and gives the
caller's back as it ends; it uses the zero page's `$70-$7F` too, saved and put back.

| Registers | |
| :-------- | :- |
| `r15`, `r14`, `r13` | The entry's address (`NUM_ADD` ...), the library's bank, the libraries' RAM bank |
| `r0`, `r1` | The operands: numbers' addresses, anywhere in the caller's memory but its paged ROM |
| `r2`, `r3` | The result's place and the room it has (a result may be written over an operand) |
| `r4` | A base string for the text entries (0: the base the library holds) |
| `.Y` | A few entries' options (`PARSE`'s flags, `BITS`'s operation, `TRIG`'s function) |
| Back | C = 0: `.A`/`.X` the result's length; C = 1: `.A` an error (`NE_`) |

In assembly, `numbers.inc` has each entry's address and the macros **`NUMCALL entry`** and **`MATHCALL entry`**, which
load `r13`-`r15`; **`numlib.s`'s `num_open`** finds the libraries, takes the bank and calls `INIT` as the program starts.
The sample `nsum` sums the numbers it's given:

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
keeps its quickest numbers itself where it can, and hands them to the library past that.  In C, `num.h` wraps every
entry ([C](c.md#numbers)).

### The entries

| | `numbers`'s |
| :- | :- |
| Its bank | `INIT`; `SET_BASE`, `GET_BASE` (the base, a string) |
| Text | `PARSE` (the longest start of a text that's a number, or with `NPARSE_WHOLE` the whole text), `DISPLAY` (a number written exactly), `FORMAT` (a format string's `{}` placeholders filled) |
| Arithmetic | `ADD`, `SUB`, `MUL`, `DIV` (exact: `10/4` is `5/2`), `NEG`, `ABS` (a real number's: `math`'s takes complex ones too), `CMP` (by value, whatever the kinds), `KIND` (an integer, a fixed decimal, a rational, a complex number; its sign), `IDIV` (an integer quotient toward 0, and the remainder), `GCD`, `POW` (a whole power) |
| Conversions | `TRUNCATE`, `FLOOR`, `ROUND` (a half to the even one), `TO_FIXED`, `TO_RATIONAL`, `NUMERATOR`, `DENOMINATOR`, `COMPLEX`, `PART`, `FROM_INT` and `TO_INT` (16 or 32 bits), `BYTES` (bytes checked as a number) |
| Bits | `BITS`: and, or, xor, not, shifts, a bit's test, on integers of any size in two's complement |
| Others | `RANDOM` (an integer below n, or a fixed decimal from 0 to 1) and `SEED`; `FIB` |

| | `math`'s |
| :- | :- |
| The functions | `SQRT`, `EXP`, `LOG`, `TRIG` (sine, cosine, tangent), `ATAN`, `RPOW` (any power), `PI`, `ABS` (a complex number's modulus too) |
| The precision | `DIGITS`: 1 to 100 significant digits, 12 at the start |

The functions are exact when their answer is (`SQRT` of `9/4` is `3/2`), else a fixed decimal of the precision's
significant digits, **correctly rounded**, so the Hydra, danlang and the PC's reference agree to the last digit.  A
negative number's square root or logarithm is complex.  A complex argument (or a negative number to a power that isn't
whole) gives the principal value, as mathematics defines it, each part correctly rounded: `sqrt(3+4i)` is `2+1i`,
`ABS` of `3-4i` is `5`, `i^i` is real.  These are worked in the library's third bank (`mtcplx.inc`), from the real
functions' cores.  At 12 digits a sine takes some 220,000 cycles.

### Errors

| Error | |
| :---- | :- |
| `NE_BIG` | Too big: an integer past 255 bytes, a fixed decimal past 65,535 places |
| `NE_DIV0` | Division by zero |
| `NE_NOTNUM` | Not a number: text that isn't one, bytes that aren't one in the stored format |
| `NE_ROOM` | No room for the result in the place given |
| `NE_DOMAIN` | Outside the function's domain (`LOG` of 0) |
| `NE_BASE` | Not a base |
| `NE_INT`, `NE_REAL` | An integer is needed; a real number, not a complex one |
| `NE_INIT` | The bank in `r13` isn't one `INIT` filled |
| `NE_FORMAT` | A format's placeholders and its arguments don't match |

### Bases and text

A base is a string, hylang's: `d` decimal, `x` hexadecimal written bare (`FF`), `#x` with its prefix (`#xFF`), `b`, `o`,
`c` (balanced ternary), `16r` (a radix), `<x` (least digit first), `[01]` (digits of its own), and the rest of hylang's
named bases.

* **Reading** (`PARSE`): integers, fractions (`1/3`), a radix point (`0.8` in hexadecimal is a half), complex numbers
  (`1+2i`), and a number with a base of its own (`#xFF`, `#b0.1`).  `NPARSE_PROGRAM` is a program's rule: a bare number
  starts with a digit (`0FF`), so `FF` is a name.
* **Writing** (`DISPLAY`): exactly, in the base: a fraction or a fixed decimal with a radix point when it ends in that
  base, else as a fraction (`0.1` in hexadecimal is `1/A`), so what's written reads back as the same number.
* **Formats** (`FORMAT`): `{}` a value in the base, `{x}`, `{#b}` ... in one named: hylang's `format`, HyForth's
  `nformat`, HyBASIC's `PRINT USING "{}"`, C's `printf` with `%N` and `%{x}N`.

---

## The asm library

**`asm`** (`reborn/modules/asm`, `reborn/spec/asm.def`) is the W65C02S's instructions as the assembler `as` writes them,
one table for every tool that reads or writes them: `db`'s `d`, HyForth's `disasm`, `dis`, `as`, and HyBASIC's `ASM`
blocks.  It's one bank: the assembler and the disassembler.

| Entry | What it does |
| :---- | :----------- |
| `DIS` | One instruction disassembled, as `as` writes it (with symbols, if given) |
| `FILE` | A source file assembled whole, as `as` does it |
| `BEGIN`, `PASS`, `DEFINE`, `LINE`, `END` | A source given a line at a time, for each pass, with the caller's own symbols: HyBASIC's way |
| `SYMBOL`, `IMAGE`, `DONE` | The assembled symbols and image, and the end |
| `ERROR` | The last error's text |

It has no RAM of its own: the assembler's state is in RAM its caller lends it while it runs (`ASM_RAM`, `$5000-$7DFF`),
and its zero page (`$22` on) is the caller's, put back as each call ends.  In assembly, `asmlib.inc` has the entries and
the macro `ASMCALL`; in C, `asm.h` has `dis_insn` and the symbol files' `lbl_load`.
