## ndepth

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
ndepth   ( -- u )
```

#### Description

Leaves, on the data stack, how many numbers are on the number stack.

The **number stack** is the numbers library's own: it holds the Hydra's numbers (Chapter 18), which are of
any length and so can't be cells. With the library loaded, a number typed that a cell can't hold goes there
by itself: `2/3`, `1.25`, `#xFF`, `2i`, `100000000000000000000`. A number that fits a cell, such as `42`,
still goes on the data stack, and `s>n` moves it across.

The number stack is in a RAM bank of 8K, so it holds a great many small numbers or a few enormous ones.
Unlike the data stack, it is not emptied when an error returns HyForth to the prompt: what was on it is
still there.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −44 | `number stack overflow` | No room for another number (any word that adds one) |
| −45 | `number stack underflow` | A word took more numbers than there were |

#### See also

`n.s`, `ndrop`, `s>n`, `depth`.

#### Example

```session
/> lib numbers
/> 2/3 1.25 ndepth .
2
/> n.s
<2> 2/3 1.25
/> n+
/> n.
23/12
/> n+
number stack underflow
```

## n+, n-, n*, n/

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n+   ( N: x y -- z )
n-   ( N: x y -- z )
n*   ( N: x y -- z )
n/   ( N: x y -- z )
```

#### Description

Add, subtract, multiply and divide the top two numbers of the number stack, exactly. The numbers may be of
any kind and any size, and need not be of the same kind: integers, fractions, decimals, complex numbers.
`n-` leaves *x* − *y*, and `n/` leaves *x* over *y*.

A result is given in the simplest kind that holds it. `n/` of two integers that don't divide evenly is a
fraction in its lowest terms, never a rounded number; a fraction and a decimal make a fraction; a result
with no fractional part is an integer.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | `n/` by zero |
| −11 | `result out of range` | A number too big for the library: more than 255 bytes, about 600 digits |

#### See also

`n/mod`, `npow`, `s>n`, `+`, `d+`.

#### Example

```session
/> lib numbers
/> 1/3 1/6 n+ n.
1/2
/> 1.5 2 s>n n* n.
3
/> 7 s>n 2 s>n n/ n.
7/2
/> 123456789012345678901234567890 ndup n* n.
15241578753238836750495351562536198787501905199875019052100
```

## n/mod

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n/mod   ( N: x y -- rem quot )
```

#### Description

Divides one integer by another, and leaves the remainder and, on top, the whole quotient. The division is
symmetric, as HyForth's `/mod` is: the quotient is rounded toward zero, and the remainder has the sign of
*x*.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | A number that is not an integer |
| −10 | `division by zero` | *y* is zero |

#### See also

`n/`, `/mod`, `ngcd`.

#### Example

```session
/> lib numbers
/> 10 s>n 3 s>n n/mod n. n.
3 1
/> -7 s>n 2 s>n n/mod n. n.
-3 -1
```

## nnegate, nabs, ngcd, npow

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nnegate   ( N: x -- y )
nabs      ( N: x -- y )
ngcd      ( N: x y -- z )
npow      ( N: x y -- z )
```

#### Description

`nnegate` changes a number's sign. `nabs` leaves a real number without its sign; a complex number is an
error. `ngcd` leaves the greatest common divisor of two integers: the largest number that divides both.

`npow` leaves *x* to the power *y*. With a whole *y* the result is exact, whatever *x* is: `2 100` gives all
31 digits, and a negative power gives a fraction. With a fractional *y* the result is exact when the root
is, and otherwise a decimal to the precision in `digits`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | `nabs` of a complex number; `ngcd` of numbers that are not integers |
| −46 | `outside the function's domain` | `npow`: a negative number to a power that isn't whole |

#### See also

`n*`, `nsqrt`, `digits`, `negate`.

#### Example

```session
/> lib numbers
/> 12 s>n 18 s>n ngcd n.
6
/> 2 s>n 100 s>n npow n.
1267650600228229401496703205376
/> 2 s>n -1 s>n npow n.
1/2
/> 2 s>n 1/2 npow n.
1.41421356237
```

## ncompare

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
ncompare   ( -- n ) ( N: x y -- )
```

#### Description

Compares the top two numbers of the number stack, removes them, and leaves on the data stack −1 if *x* is
less than *y*, 0 if they are equal, and 1 if it is greater. Numbers are compared by value, whatever their
kind: 1/2 and 0.5 are equal.

#### See also

`n=`, `n<`, `compare`.

#### Example

```session
/> lib numbers
/> 1/2 0.5 ncompare .
0
/> 1/3 0.5 ncompare .
-1
```

## n=, n<, n>, n0=, n0<

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n=    ( -- flag ) ( N: x y -- )
n<    ( -- flag ) ( N: x y -- )
n>    ( -- flag ) ( N: x y -- )
n0=   ( -- flag ) ( N: x -- )
n0<   ( -- flag ) ( N: x -- )
```

#### Description

The tests of `=`, `<`, `>`, `0=` and `0<`, for the number stack. Each removes its numbers and leaves a flag
on the data stack, where `if` and the rest expect one. Numbers are compared by value: `1/2 0.5 n=` is true.

#### See also

`ncompare`, `int?`, `=`.

#### Example

```session
/> lib numbers
/> 1/2 0.5 n= .
-1
/> 1/3 1/2 n< .
-1
/> 0.0 n0= .
-1
```

## int?, fixed?, rational?, complex?

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
int?        ( -- flag ) ( N: x -- )
fixed?      ( -- flag ) ( N: x -- )
rational?   ( -- flag ) ( N: x -- )
complex?    ( -- flag ) ( N: x -- )
```

#### Description

Remove the top number and leave a flag: is it an integer, a decimal, a fraction, a complex number? The test
is of the form the number is in, not its value: 1.0 is a decimal, not an integer, though the two are equal.
Arithmetic gives its results in the simplest form, so `4/2` is an integer.

#### See also

`n=`, `to-fixed`, `to-rational`.

#### Example

```session
/> lib numbers
/> 1/2 rational? . 1.5 fixed? . 2i complex? .
-1 -1 -1
/> 1.0 int? . 4/2 int? .
0 -1
```

## s>n, d>n

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
s>n   ( n -- ) ( N: -- x )
d>n   ( d -- ) ( N: -- x )
```

#### Description

Move a number from the data stack to the number stack: `s>n` a signed cell, and `d>n` a signed double. This
is how a small number, which HyForth reads as a cell, joins the big ones: `2 s>n`.

#### See also

`n>s`, `>n`, `s>d`.

#### Example

```session
/> lib numbers
/> 100 s>n 10 s>n npow n.
100000000000000000000
/> 70000. d>n n.
70000
```

## n>s, n>d

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n>s   ( -- n ) ( N: x -- )
n>d   ( -- d ) ( N: x -- )
```

#### Description

Move a number from the number stack to the data stack, as a signed cell or a signed double. A number with a
fractional part loses it: the whole part, toward zero, is what arrives.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −11 | `result out of range` | The number doesn't fit a cell (`n>s`) or a double (`n>d`) |
| −24 | `invalid numeric argument` | A complex number |

#### See also

`s>n`, `truncate`, `d>s`.

#### Example

```session
/> lib numbers
/> 7/2 n>s .
3
/> 100000000000 n>s .
result out of range
```

## truncate, nfloor, nround

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
truncate   ( N: x -- n )
nfloor     ( N: x -- n )
nround     ( N: x -- n )
```

#### Description

Make a real number whole. `truncate` drops what follows the point, which rounds toward zero. `nfloor` rounds
down, toward minus infinity. `nround` rounds to the nearest whole number, and a number exactly halfway
between two goes to the even one: 0.5 to 0, 1.5 and 2.5 to 2, 3.5 to 4.

#### See also

`to-fixed`, `n>s`, `n/mod`.

#### Example

```session
/> lib numbers
/> 2.7 truncate n. -2.7 truncate n.
2 -2
/> -2.7 nfloor n.
-3
/> 2.5 nround n. 3.5 nround n.
2 4
```

## to-fixed

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
to-fixed   ( u -- ) ( N: x -- y )
```

#### Description

Makes the top number a decimal with at most *u* places after the point. Digits beyond that are cut off, not
rounded: two thirds to two places is 0.66. This is where a fraction is turned into something to print.

#### See also

`to-rational`, `nround`, `n.`.

#### Example

```session
/> lib numbers
/> 1 s>n 3 s>n n/ 4 to-fixed n.
0.3333
/> 2/3 2 to-fixed n.
0.66
```

## to-rational, rational.n, rational.d

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
to-rational   ( N: x -- y )
rational.n    ( N: x -- n )
rational.d    ( N: x -- n )
```

#### Description

`to-rational` makes a decimal the exact fraction it equals, in its lowest terms. `rational.n` leaves the top
of that fraction, the numerator, and `rational.d` the bottom, the denominator; an integer's denominator is
1.

#### See also

`to-fixed`, `n/`, `ngcd`.

#### Example

```session
/> lib numbers
/> 0.25 to-rational n.
1/4
/> 3/4 rational.n n. 3/4 rational.d n.
3 4
```

## complex

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
complex   ( N: re im -- z )
```

#### Description

Makes a complex number from two real ones: its real part, and above it its imaginary part. A complex number
can also be typed directly, with digits before its `i`: `1+2i`, `2i`.

#### See also

`complex?`, `n+`, `nsqrt`.

#### Example

```session
/> lib numbers
/> 1 s>n 2 s>n complex n.
1+2i
/> 5 s>n 1+2i n+ n.
6+2i
```

## nrandom, nseed

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nrandom   ( N: n -- r )
nseed     ( u -- )
```

#### Description

`nrandom` replaces a whole number *n* by a random whole number from 0 to *n* − 1. *n* may be as large as
any integer. With an *n* of 0 it gives a random decimal between 0 and 1.

`nseed` starts the generator from a number of the program's choosing. The same seed gives the same series
again, which is what testing a game or a simulation needs.

#### See also

`random`, `rand`, `nfib`.

#### Example

```session
/> lib numbers
/> 42 nseed 6 s>n nrandom n.
5
/> 42 nseed 6 s>n nrandom n.
5
```

## nfib

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nfib   ( N: n -- f )
```

#### Description

Replaces a whole number *n*, 0 or more, by the *n*th Fibonacci number: 0, 1, 1, 2, 3, 5 ... , exactly,
however long.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | *n* is negative, or not whole |

#### Example

```session
/> lib numbers
/> 30 s>n nfib n.
832040
/> 100 s>n nfib n.
354224848179261915075
```

## nand, nor, nxor, ninvert

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nand      ( N: x y -- z )
nor       ( N: x y -- z )
nxor      ( N: x y -- z )
ninvert   ( N: x -- y )
```

#### Description

The bitwise operations `and`, `or`, `xor` and `invert`, on integers of any length. (`nand` is the numbers'
"and", not "not and".) A negative integer is taken as two's complement, with its sign bit going on for ever:
`0 ninvert` is −1.

#### See also

`nlshift`, `nbit?`, `and`.

#### Example

```session
/> lib numbers
/> 12 s>n 10 s>n nand n. 12 s>n 10 s>n nor n. 12 s>n 10 s>n nxor n.
8 14 6
/> 0 s>n ninvert n.
-1
```

## nlshift, nrshift

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nlshift   ( u -- ) ( N: x -- y )
nrshift   ( u -- ) ( N: x -- y )
```

#### Description

Shift an integer's bits *u* places, the count coming from the data stack. `nlshift` multiplies by 2 to the
power *u*, and loses nothing, since the integer simply grows. `nrshift` divides by 2 to the power *u*,
rounding down, and keeps the sign.

#### See also

`nand`, `nbit?`, `lshift`.

#### Example

```session
/> lib numbers
/> 1 s>n 100 nlshift n.
1267650600228229401496703205376
/> -8 s>n 1 nrshift n.
-4
```

## nbit?

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nbit?   ( u -- flag ) ( N: x -- )
```

#### Description

Removes an integer from the number stack and leaves a flag: true if its bit *u* is 1. Bit 0 is the lowest.
A negative integer has every bit above its top one set.

#### See also

`nand`, `nrshift`, `tbit`.

#### Example

```session
/> lib numbers
/> 5 s>n 0 nbit? . 5 s>n 1 nbit? .
-1 0
```

## nbytes, nfrom-bytes

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nbytes        ( -- c-addr u ) ( N: x -- )
nfrom-bytes   ( c-addr u -- ) ( N: -- x )
```

#### Description

A number in the Hydra's stored format: the compact form, of a length that depends on the number, that every
language uses to keep a number in a file (hylang's `number-bytes`, HyBASIC's `MKN$`). `nbytes` removes a
number and leaves the address and length of its bytes; `nfrom-bytes` takes such bytes and puts the number
they hold back on the number stack.

The bytes `nbytes` leaves are in a buffer of the library's own: write or copy them before another number
word is used.

#### See also

`n>str`, `>n`, `write-file`.

#### Example

```session
/> lib numbers
/> 258 s>n nbytes nip .
3
/> 258 s>n nbytes nfrom-bytes n.
258
```

## n., n.base

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n.       ( N: x -- )
n.base   ( c-addr u -- ) ( N: x -- )
```

#### Description

Print the top number of the number stack, with a space after it, and remove it. `n.` prints in the current
base. `n.base` prints in the base named by the string, any of Chapter 18's: `s" x"` hex, `s" b"` binary,
`s" o"` octal, a radix such as `s" 16r"`; with a `#` before the radix, the number is printed with that
prefix.

#### See also

`n.s`, `n>str`, `nformat`, `set-base`, `.`.

#### Example

```session
/> lib numbers
/> 2/3 n.
2/3
/> 255 s>n s" x" n.base 255 s>n s" b" n.base
FF 11111111
/> 255 s>n s" #16r" n.base
#16rFF
```

## n.s

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n.s   ( -- )
```

#### Description

Shows the number stack, and changes nothing: how many numbers are on it, in angle brackets, and then each
one, the top last. It is `.s` for the number stack.

#### See also

`ndepth`, `n.`, `.s`.

#### Example

```session
/> lib numbers
/> 2/3 1.25 100000000000 n.s
<3> 2/3 1.25 100000000000
```

## >n

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
>n   ( c-addr u -- flag ) ( N: -- x | )
```

#### Description

Reads a number from a string. If the whole string is a number, in any of the forms that can be typed, the
number goes on the number stack and the flag is true. If it is not, nothing is added, and the flag is
false. Spaces around the number are allowed.

This is how a program turns what a person typed, or a line of a file, into a number, and finds out whether
it was one.

#### See also

`n>str`, `>number`, `accept`, `s>n`.

#### Example

```session
/> lib numbers
/> s" 2/3" >n . n.
-1 2/3
/> s" abc" >n . ndepth .
0 0
```

## set-base, get-base

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
set-base   ( c-addr u -- )
get-base   ( -- c-addr u )
```

#### Description

The base in which numbers are read and printed, by name. `set-base` takes any of Chapter 18's base strings,
which go far beyond what the variable `base` can say: `s" x"` and `s" b"`, but also bases with digits of
their own, and balanced ones. `get-base` leaves the name of the base in force, `d` for decimal.

Cells and the number stack's numbers share the one setting. Setting `base`, or saying `hex` or `decimal`,
sets it for both.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | The string names no base |

#### See also

`base`, `n.base`, `hex`; sections 18.3 to 18.9.

#### Example

```session
/> lib numbers
/> get-base type
d
/> s" x" set-base  255 s>n n.  decimal
255
/> 255 s>n s" x" set-base n. decimal
FF
```

## nformat

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nformat   ( c-addr u -- ) ( N: x1 ... xn -- )
```

#### Description

Prints a string in which each `{}` is replaced by a number from the number stack: the first `{}` by the
deepest of them, so the numbers appear in the order they were put there. As many numbers are removed as
there are places.

A place can name a base, as `{x}` does: that number is printed in it.

#### See also

`n.`, `n.base`, `."`.

#### Example

```session
/> lib numbers
/> 1/2 0.25 s" {} and {}" nformat
1/2 and 0.25
/> 255 s>n s" hex {x}" nformat
hex FF
```

## nsqrt, nexp, nlog, nsin, ncos, ntan, natan, npi

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nsqrt   nexp   nlog   nsin   ncos   ntan   natan   ( N: x -- y )
npi     ( N: -- x )
```

#### Description

The math functions: the square root, *e* to a power, the natural logarithm, the sine, cosine and tangent of
an angle in radians, the angle whose tangent is given, and pi.

Where there is an exact answer it is given: `16 nsqrt` is 4, and the square root of a negative number is an
imaginary one. Otherwise the result is a decimal with as many significant digits as the variable `digits`
holds, 12 unless a program changes it.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −46 | `outside the function's domain` | The logarithm of 0 |
| −24 | `invalid numeric argument` | A complex number |

#### See also

`digits`, `npow`, `to-fixed`.

#### Example

```session
/> lib numbers
/> 2 s>n nsqrt n.
1.41421356237
/> 16 s>n nsqrt n. -4 s>n nsqrt n.
4 2i
/> 1 s>n nexp n.
2.71828182846
/> npi n.
3.14159265359
```

## digits

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
digits   ( -- a-addr )
```

#### Description

A variable: the number of significant digits the math functions work to when an answer is not exact. It
starts as 12, and can be raised as far as 100. It makes no difference to `n+`, `n-`, `n*` and `n/`, which are
always exact.

#### See also

`nsqrt`, `npow`, `to-fixed`.

#### Example

```session
/> lib numbers
/> 30 digits !
/> 2 s>n nsqrt n.
1.41421356237309504880168872421
/> npi n.
3.14159265358979323846264338328
```
