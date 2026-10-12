# Chapter 18. Numbers and Bases

Most small computers can only count to 255, or 65,535, and get fractions slightly wrong. The Hydra's numbers are
different: they can be as big as you like, fractions are exact, and they can be written in any **base**, from binary
to base 80 and beyond. The same numbers work in hylang, in HyBASIC, in HyForth (with its numbers library), in the
`calc` command, and in assembly language. This chapter explains them, using `calc` for the examples, since it works
at the shell; each language's own chapter shows how it writes them.

> **IN THIS CHAPTER:** You'll meet the Hydra's four kinds of number (whole numbers of any size, exact fractions,
> decimals and complex numbers), and learn to write and print them in any base. **You need:** Chapter 6, and section
> 1.6. If you're in a hurry to program, read sections 18.1 to 18.3, and come back for the rest when you need it.

## 18.1 Four kinds of number

The Hydra has four kinds of number.

**Integers** are whole numbers: 0, 42, -17. They can have as many digits as you need (up to about 600).

```session
/> calc '2^100'
1267650600228229401496703205376
/> calc 'fib 100'
354224848179261915075
```

(`fib 100` is the hundredth **Fibonacci number**: each is the sum of the two before it, 1, 1, 2, 3, 5, 8 ...)

**Fixed decimals** are numbers with a decimal point, like 3.14 or 0.5. They're exact: each has a fixed number of
places after the point, and adding or multiplying them never loses any. So, unlike most computers, the Hydra knows
that 0.1 and 0.2 make exactly 0.3:

```session
/> calc '0.1 + 0.2'
0.3
/> calc '3.14 * 2'
6.28
```

**Rationals** are fractions: one integer over another, like 2/3. Dividing two integers that don't divide exactly
gives a rational, never a rounded decimal, and rationals are always kept in their lowest terms:

```session
/> calc '1/3 + 1/6'
1/2
/> calc '100 / 7'
100/7
/> calc '1/3 * 3'
1
```

**Complex numbers** have a real part and an imaginary part, which is written with `i`. `i` is the square root of -1,
a number that mathematicians invented so that every number has a square root:

```session
/> calc 'i^2'
-1
/> calc 'sqrt(-4)'
2i
/> calc '(1+2i) * (3-i)'
5+5i
```

When numbers of different kinds meet, the answer is the simplest kind that can hold it exactly. An integer divided by
an integer is a rational, unless it comes out whole; a rational and a decimal make a rational (`1/3 + 0.5` is `5/6`);
and anything with a complex number is complex, unless its imaginary part turns out to be 0.

## 18.2 Functions and precision

Some numbers can't be written exactly in any of these ways: the square root of 2, or pi, has digits that go on for
ever. For these, the **math library**'s functions (square roots, powers, logarithms, sines ...) work to a
**precision**: 12 digits unless you ask for more. `calc`'s `-d` asks for more:

```session
/> calc 'sqrt 2'
1.41421356237
/> calc -d 40 'sqrt 2'
1.41421356237309504880168872420969807857
/> calc -d 50 pi
3.1415926535897932384626433832795028841971693993751
```

The answers are fixed decimals, rounded at the last place. Each language has its own way to set the precision (up to
100 digits): Chapters 19, 21 and 23 show how.

## 18.3 Bases

When we write a number like 365, each digit's place means something: 3 hundreds, 6 tens and 5 ones. Each place is
worth ten times the place to its right. That's **base 10**, or **decimal**, and we use it because we have ten
fingers. But any number can be the base:

* In **base 2**, **binary**, each place is worth twice the one to its right: ones, twos, fours, eights ... and the
  only digits are 0 and 1. 1010 in binary is one eight, no fours, one two and no ones: ten. Computers work in
  binary inside, because a wire can be off (0) or on (1).
* In **base 16**, **hexadecimal** (or **hex**), each place is worth sixteen times the one to its right, so it needs
  sixteen digits: 0 to 9, then A to F for ten to fifteen. FF is fifteen sixteens and fifteen ones: 255. Programmers
  use hex because each hex digit is exactly four binary digits, so it's a short way to write binary.

| Decimal | Binary | Hex |
|---|---|---|
| 5 | 101 | 5 |
| 10 | 1010 | A |
| 15 | 1111 | F |
| 16 | 10000 | 10 |
| 255 | 11111111 | FF |

`calc -b` shows its answer in another base. The base is named by a **base string**: `b` for binary, `x` for hex,
and so on:

```session
/> calc -b b 10
1010
/> calc -b x 255
FF
```

## 18.4 The named bases

The Hydra has nineteen **named bases**, each named by a letter (capital or small: `X` and `x` are the same):

| Letter | Base | Digits | Called |
|---|---|---|---|
| `b` | 2 | 01 | Binary |
| `t` | 3 | 012 | Ternary |
| `q` | 4 | 0123 | Quaternary |
| `v` | 5 | 01234 | Quinary |
| `f` | 6 | 012345 | Senary |
| `s` | 7 | 0123456 | Septenary |
| `o` | 8 | 01234567 | Octal |
| `n` | 9 | 012345678 | Nonary |
| `d` | 10 | 0123456789 | Decimal |
| `x` | 16 | 0123456789ABCDEF | Hexadecimal |
| `z` | 36 | 0-9 and A-Z | Base 36 (all the digits and letters) |
| `c` | 3 | `-` `0` `+` | Balanced ternary, least digit first |
| `i` | 3 | `U` `O` `N` | Balanced ternary in letters, least digit first |
| `e` | 5 | `=` `-` `0` `+` `#` | Balanced quinary, least digit first |
| `j` | 5 | `W` `U` `O` `N` `M` | Balanced quinary in letters, least digit first |
| `g` | 7 | `~` `=` `-` `0` `+` `#` `*` | Balanced septenary, least digit first |
| `k` | 27 | `ZYXWVUTSRQPON` `0` `ABCDEFGHIJKLM` | Balanced base 27 |
| `y` | 53 | `zyx`...`a` `0` `ABC`...`Z` | Balanced base 53 |
| `m` | -13 | `DanielStphrus` | Balanced base -13, least digit first |

```session
/> calc -b t 10
101
/> calc -b o 10
12
/> calc -b z 1000000
LFLS
```

The first eleven are ordinary bases: their digits start at 0, and the biggest digit is one less than the base. The
others are stranger, and need some explaining.

**Balanced bases.** In a **balanced** base, the digits go below zero as well as above it. In balanced ternary, base
`c`, the three digits are `-` (minus one), `0` (zero) and `+` (plus one). There's no need for a minus sign: a
negative number just uses its digits differently. The places are still ones, threes, nines, twenty-sevens ...

**Least digit first.** The bases `c`, `i`, `e`, `j`, `g` and `m` write their digits the other way round: the ones
digit first, on the left. So in base `c`, 5 is `--+`:

| Place | Ones | Threes | Nines |
|---|---|---|---|
| Digit | `-` (-1) | `-` (-1) | `+` (+1) |
| Worth | -1 | -3 | +9 |

and -1 - 3 + 9 = 5. Base `i` is the same, with the letters `U` (minus one), `O` (zero) and `N` (plus one) as its
digits, so 5 is `UUN`.

```session
/> calc -b c 5
--+
/> calc -b c 10
+0+
/> calc -b i 5
UUN
```

Balanced ternary isn't just a curiosity: some early Russian computers, like the Setun of 1958, worked in it, and it's
the neatest way to weigh things with a balance scale.

**Negative bases.** In a **negative** base, the places are worth 1, the base, the base squared ..., and since the
base is negative, the places alternate between positive and negative. Base `m` is base -13, balanced, with thirteen
letters as its digits: `D` `a` `n` `i` `e` `l` are -6 to -1, `S` is 0, and `t` `p` `h` `r` `u` `s` are 1 to 6.
Capital and small letters are different digits here, as `S` and `s` both appear.

## 18.5 Radixes and digits of your own

Besides the named bases, a base string can name a **radix**, or a set of **digits of your own**.

**A radix** is a number from 2 to 80, then `r`: `16r` is base 16, the same as `x`. A radix's digits are the first
of these eighty:

```text
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-=+`~!@#$%^&*,;:|?
```

So `36r` is the same as `z`, and `80r` uses them all:

```session
/> calc -b 16r 255
FF
/> calc -b 80r 1000000
1;K0
```

**Digits of your own** are written in square brackets: from 2 to 80 characters, each used once, the first standing
for zero, the next for one, and so on. Any character can be a digit, except a space, a control character, and these:
`< > [ ] ( ) . / _ \ ' "`.

```session
/> calc -b '[ABCDEFGHIJ]' 123
BCD
```

(With the digits A to J for 0 to 9, 123 is BCD.) If no two of the digits differ only in being capital or small,
then capital and small letters are read as the same.

## 18.6 The modifiers

A base string can start with **modifiers**, which change how the base works:

| Modifier | What it does |
|---|---|
| `#` | Numbers written in this base show the base, as a prefix: `#xFF` instead of `FF` |
| `<` | Write the digits least first (ones on the left) |
| `>` | Write the digits most first (the usual way) |
| `=` | Balanced: the middle digit is zero (for a radix or digits of your own; a named base is balanced already, or not) |
| `-` | A negative base |
| `+` | A positive base (the usual way) |

They go in that order: `#`, then `<` or `>`, then `=`, then `+` or `-`, then the base.

```session
/> calc -b '#x' 255
#xFF
/> calc -b '<x' 4660
4321
/> calc -b '-d' 10
190
```

4660 is `1234` in hex, so least digit first it's `4321`. In base -10, 190 is 1 × 100 + 9 × -10 + 0: 10.

So one base can have several names, which all mean the same:

| Base | Some of its names |
|---|---|
| Binary | `b`, `2r`, `[01]` |
| Octal | `o`, `8r`, `[01234567]` |
| Decimal | `d`, `10r`, `[0123456789]` |
| Hexadecimal | `x`, `16r`, `[0123456789ABCDEF]` |
| Base 36 | `z`, `36r` |
| Balanced ternary | `c` (least first, with `-` `0` `+`), `>c` (most first), `=3r` (most first, with `0` `1` `2` for -1, 0 and +1) |

## 18.7 Writing a number in a base

Inside a program (or a `calc` expression), a number can say its own base: a `#`, the base string, and the digits.

| Written | Is |
|---|---|
| `#b1010` | 10, in binary |
| `#o17` | 15, in octal |
| `#xFF` | 255, in hex |
| `#16rFF` | 255, base 16 as a radix |
| `#z1Z` | 71, in base 36 |
| `#c+--` | -11, in balanced ternary, least digit first |
| `#[ABCDEFGHIJ]BCD` | 123, with digits of its own |
| `#<x4321` | 4660, in hex, least digit first |

```session
/> calc '#b1010 + #x0F'
25
/> calc '#[ABCDEFGHIJ]BCD'
123
```

Numbers can have `_`s in them, to make them easier to read: `1_000_000` is a million. And decimal numbers can be
written in all the forms of section 18.1: `-42`, `3.14`, `2/3`, `2i`, `1+2i`.

## 18.8 Fractions in other bases

A fraction can be written with a point in any base: in binary, `0.1` is a half, and `0.01` a quarter.

```session
/> calc -b b 0.75
0.11
/> calc '#x0.8'
1/2
```

But just as 1/3 never ends in decimal (0.3333...), some fractions never end in other bases. Then they're written as
a fraction, with the base's digits:

```session
/> calc -b b '1/3'
1/11
```

(1 over 11: 1 over three, in binary.)

## 18.9 The base setting

Each program has a **base setting**: the base it reads numbers in, and writes them in, when they don't say their
own. It starts as decimal. Every language can change it, and when it does, every number the program reads or writes
follows it:

| Language | Setting the base | Reading it |
|---|---|---|
| hylang | `(base "x")` | `(base)` |
| HyBASIC | `BASE "x"` | `BASE$` |
| HyForth | `s" x" set-base` (or `hex`, `decimal`, and `n base !`) | `get-base` |
| `calc` | `calc -b x ...` (for its answer) | |

In hylang, for example:

```session
/> hylang
hylang (danlang on the Hydra-16)
Type 'exit' to Exit

hylang> (base "x")
=> "d"
hylang> (+ 9 1)
=> A
hylang> (base "d")
=> "x"
```

In hex, 9 + 1 is A. Setting the base gives back the one before, so a program can put it back when it's done.

## 18.10 Bits

Integers can be treated as rows of **bits** (binary digits), however big they are. Each language has the operations
**and**, **or**, **xor**, **not**, and **shifts**, which move the bits left (multiplying by 2 each place) or right
(dividing by 2, rounding down). A negative integer acts as if it had an endless row of 1s on its left, as in most
computers.

| Operation | Each bit of the answer is 1 when |
|---|---|
| and | Both bits are 1 |
| or | Either bit is 1 |
| xor | The bits differ |
| not | The bit is 0 (and so not *x* is -*x* - 1) |

## 18.11 When numbers go wrong

| Message | Means |
|---|---|
| `division by zero` | Dividing by 0 |
| `too big` | An integer of more than 255 bytes (about 600 digits), or a decimal with more than 65,535 places |
| `not a number` | Text that isn't a number |
| `outside the function's domain` | A function given a number it can't take, like the logarithm of 0 |
| `not a base` | A base string that names no base |
| `an integer is needed` | A fraction where only a whole number will do |
| `a real number is needed` | A complex number where only a real one will do |

```session
/> calc 1/0
calc: division by zero
/> calc 'abs(3-4i)'
calc: a real number is needed
```
