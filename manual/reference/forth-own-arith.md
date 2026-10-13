## dup

Core word set; in the ROM core, always there.

#### Stack

```text
dup   ( x -- x x )
```

#### Description

Makes a copy of the top cell of the stack, and leaves the copy on top of it. Almost every word uses up what
it takes, so `dup` is how a value is kept for a second use.

#### See also

`?dup`, `over`, `2dup`, `drop`.

#### Example

```session
/> 7 dup * .
49
/> 1 2 dup .s
<3> 1 2 2
```

## ?dup

Core word set; in the ROM core, always there.

#### Stack

```text
?dup   ( x -- 0 | x x )
```

#### Description

Copies the top cell if it is not zero, and leaves a zero as it is. It is for a value that is also a flag:
`?dup if ... then` runs the part between with the value, and leaves nothing behind when the value was zero,
where `dup if ... else drop then` would be needed without it.

#### See also

`dup`, `if`.

#### Example

```session
/> 5 ?dup .s
<2> 5 5
/> 2drop 0 ?dup .s
<1> 0
```

## drop

Core word set; in the ROM core, always there.

#### Stack

```text
drop   ( x -- )
```

#### Description

Removes the top cell of the stack. With nothing on the stack it is the error −4, `stack underflow`, reported
when the line has been run.

#### See also

`2drop`, `nip`, `dup`.

#### Example

```session
/> 1 2 drop .s
<1> 1
/> drop drop
stack underflow
```

## swap

Core word set; in the ROM core, always there.

#### Stack

```text
swap   ( x1 x2 -- x2 x1 )
```

#### Description

Exchanges the top two cells.

#### See also

`over`, `rot`, `2swap`, `nip`, `tuck`.

#### Example

```session
/> 1 2 swap .s
<2> 2 1
/> 10 3 swap - .
-7
```

## over

Core word set; in the ROM core, always there.

#### Stack

```text
over   ( x1 x2 -- x1 x2 x1 )
```

#### Description

Copies the second cell to the top.

#### See also

`dup`, `pick`, `2over`, `tuck`.

#### Example

```session
/> 1 2 over .s
<3> 1 2 1
```

## rot

Core word set; in the ROM core, always there.

#### Stack

```text
rot   ( x1 x2 x3 -- x2 x3 x1 )
```

#### Description

Brings the third cell to the top, moving the other two down. `rot rot` turns the three the other way: it
puts the top cell under the other two.

#### See also

`roll`, `swap`, `2rot`.

#### Example

```session
/> 1 2 3 rot .s
<3> 2 3 1
/> rot rot .s
<3> 1 2 3
```

## depth

Core word set; in the ROM core, always there.

#### Stack

```text
depth   ( -- +n )
```

#### Description

Leaves the number of cells that were on the stack before `depth` ran.

#### See also

`.s`, `ndepth`.

#### Example

```session
/> depth .
0
/> 10 20 30 depth .
3
```

## nip, tuck

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
nip    ( x1 x2 -- x2 )
tuck   ( x1 x2 -- x2 x1 x2 )
```

#### Description

`nip` removes the second cell: it is `swap drop`. `tuck` puts a copy of the top cell under the second: it is
`swap over`.

#### See also

`drop`, `over`, `swap`.

#### Example

```session
/> 1 2 nip .s
<1> 2
/> drop 1 2 tuck .s
<3> 2 1 2
```

## pick

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
pick   ( xu ... x1 x0 u -- xu ... x1 x0 xu )
```

#### Description

Removes *u*, and copies the cell that is then *u* places down from the top: `0 pick` is `dup`, and `1 pick`
is `over`.

#### Notes

*u* is not checked against the depth of the stack. A *u* that reaches below the bottom of the stack gives
whatever is in the memory there, and is not an error.

#### See also

`roll`, `dup`, `over`.

#### Example

```session
/> 10 20 30 1 pick .
20
/> 0 pick .
30
```

## roll

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
roll   ( xu xu-1 ... x0 u -- xu-1 ... x0 xu )
```

#### Description

Removes *u*, takes the cell that is then *u* places down out of the stack, and puts it on top; the cells that
were above it each move down one. `1 roll` is `swap`, `2 roll` is `rot`, and `0 roll` does nothing.

#### See also

`pick`, `rot`.

#### Example

```session
/> 1 2 3 2 roll .s
<3> 2 3 1
```

## +, -, *

Core word set; in the ROM core, always there.

#### Stack

```text
+   ( n1 n2 -- n3 )
-   ( n1 n2 -- n3 )
*   ( n1 n2 -- n3 )
```

#### Description

The sum, the difference *n1* − *n2*, and the product of two cells. They are the same for signed and for
unsigned numbers.

A result that does not fit in 16 bits wraps round silently, and nothing reports it: `32767 1 +` is −32768,
and `300 300 *` is 24464, the low 16 bits of 90,000. Where a product or a total may be too big for a cell,
use the mixed words (`m*`, `um*`, `*/`), the double-number words, or the numbers library, whose integers
are of any length.

#### See also

`1+`, `m*`, `um*`, `*/`, `d+`, `n+`.

#### Example

```session
/> 3 4 + .
7
/> 3 4 - .
-1
/> 3 4 * .
12
/> 32767 1 + .
-32768
```

## /, mod, /mod

Core word set; in the ROM core, always there.

#### Stack

```text
/      ( n1 n2 -- n3 )
mod    ( n1 n2 -- n3 )
/mod   ( n1 n2 -- n3 n4 )
```

#### Description

Signed division of *n1* by *n2*. `/` leaves the quotient, `mod` the remainder, and `/mod` both: the
remainder *n3*, and above it the quotient *n4*.

HyForth's division is symmetric: the quotient is rounded toward zero, and the remainder has the sign of
*n1*. So `-7 2 /` is −3, with the remainder −1. For division that rounds down, use `fm/mod`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | *n2* is 0 |

#### See also

`*/`, `fm/mod`, `sm/rem`, `um/mod`, `2/`.

#### Example

```session
/> 7 2 / .
3
/> -7 2 / .
-3
/> -7 2 mod .
-1
/> 7 2 /mod . .
3 1
/> 1 0 /
division by zero
```

## */, */mod

Core word set; in the ROM core, always there.

#### Stack

```text
*/      ( n1 n2 n3 -- n4 )
*/mod   ( n1 n2 n3 -- n4 n5 )
```

#### Description

Multiplies *n1* by *n2*, and divides the product by *n3*. The product is kept as a 32-bit double, so it does
not overflow as `*` and then `/` would. `*/` leaves the quotient; `*/mod` leaves the remainder, and above it
the quotient.

This is the way to scale a number by a fraction with whole numbers only: `1000 314 100 */` is 3140, 1000
times 3.14. The division is symmetric, as `/` is.

The quotient itself must fit in a cell. One that does not is not reported: the result is wrong.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | *n3* is 0 |

#### See also

`m*/`, `/mod`, `um/mod`.

#### Example

```session
/> 1000 1000 100 */ .
10000
/> 1000 1000 * 100 / .
169
/> 100 7 3 */mod . .
233 1
```

## 1+, 1-, 2*, 2/

Core word set; in the ROM core, always there.

#### Stack

```text
1+   ( n1 -- n2 )
1-   ( n1 -- n2 )
2*   ( x1 -- x2 )
2/   ( x1 -- x2 )
```

#### Description

`1+` adds one, and `1-` takes one away. `2*` shifts the cell one bit to the left, which doubles it. `2/`
shifts it one bit to the right and keeps the sign bit as it was, which halves it, rounding *down*: `-5 2/` is
−3, where `-5 2 /` is −2.

#### See also

`+`, `lshift`, `rshift`, `d2*`, `cell+`.

#### Example

```session
/> 5 1+ . 5 1- .
6 4
/> 5 2* . 5 2/ .
10 2
/> -5 2/ .
-3
```

## negate, abs

Core word set; in the ROM core, always there.

#### Stack

```text
negate   ( n1 -- n2 )
abs      ( n -- u )
```

#### Description

`negate` leaves the number with its sign changed: 0 − *n1*. `abs` leaves the number without its sign.

−32768 has no positive counterpart in 16 bits: `negate` and `abs` both leave it as it is, and what `abs`
leaves is right as the unsigned number 32,768.

#### See also

`dnegate`, `dabs`, `invert`.

#### Example

```session
/> 5 negate . -5 abs .
-5 5
/> -32768 abs u.
32768
```

## min, max

Core word set; in the ROM core, always there.

#### Stack

```text
min   ( n1 n2 -- n3 )
max   ( n1 n2 -- n3 )
```

#### Description

The lesser, and the greater, of two signed numbers. `0 max` keeps a number from going below zero, and
`limit min` from going above a limit.

The comparison is signed: 65535 is −1, and is less than 1.

#### See also

`<`, `within`, `dmin`, `dmax`.

#### Example

```session
/> 3 9 min . 3 9 max .
3 9
/> -1 1 min .
-1
/> 150 0 max 100 min .
100
```

## s>d

Core word set; in the ROM core, always there.

#### Stack

```text
s>d   ( n -- d )
```

#### Description

Makes a signed cell a signed double of the same value: a cell of 0 is put above it for a number that is 0 or
more, and a cell of −1, all ones, for a negative one.

For an unsigned cell, the high cell is always 0: put a `0` on the stack.

#### See also

`d>s`, `m*`, `um/mod`.

#### Example

```session
/> 3 s>d . .
0 3
/> -3 s>d . .
-1 -3
```

## um*, m*

Core word set; in the ROM core, always there.

#### Stack

```text
um*   ( u1 u2 -- ud )
m*    ( n1 n2 -- d )
```

#### Description

Multiply two cells and leave the whole 32-bit product, which cannot overflow. `um*` takes the cells as
unsigned and leaves an unsigned double; `m*` takes them as signed and leaves a signed double.

#### See also

`*`, `um/mod`, `*/`, `d.`, `m*/`.

#### Example

```session
/> lib double
/> 40000 3 um* d.
120000
/> -2 3 m* d.
-6
/> 300 300 m* d.
90000
```

(`d.`, which prints a double, is in the `double` library.)

## um/mod, fm/mod, sm/rem

Core word set; in the ROM core, always there.

#### Stack

```text
um/mod   ( ud u1 -- u2 u3 )
fm/mod   ( d n1 -- n2 n3 )
sm/rem   ( d n1 -- n2 n3 )
```

#### Description

Divide a double by a cell, and leave the remainder, and above it the quotient, each a cell.

| Word | The numbers | The quotient | The remainder |
|---|---|---|---|
| `um/mod` | Unsigned | | |
| `fm/mod` | Signed | Rounded down, toward minus infinity ("floored") | Has the divisor's sign, or is 0 |
| `sm/rem` | Signed | Rounded toward zero ("symmetric") | Has the dividend's sign, or is 0 |

For numbers of the same sign the two signed words agree. They differ when the signs differ: −7 by 2 is −4
remainder 1 floored, and −3 remainder −1 symmetric. HyForth's `/` and `mod` are symmetric; `fm/mod` is the
one for a remainder that is never negative with a positive divisor.

The quotient must fit in a cell: one that does not is not reported.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | The divisor is 0 |

#### See also

`/mod`, `um*`, `m*`, `s>d`.

#### Example

```session
/> 100000. 7 um/mod . .
14285 5
/> -7. 2 fm/mod . .
-4 1
/> -7. 2 sm/rem . .
-3 -1
```

## and, or, xor

Core word set; in the ROM core, always there.

#### Stack

```text
and   ( x1 x2 -- x3 )
or    ( x1 x2 -- x3 )
xor   ( x1 x2 -- x3 )
```

#### Description

The bitwise operations, on all 16 bits at once. Each bit of the result comes from the same bit of the two
cells: `and` gives 1 where both are 1, `or` where either is, and `xor` where they differ.

Because true is −1, all bits set, and false is 0, the same three words combine flags: `and` is "both", `or`
is "either". This works for well-formed flags only: 1 and 2 are both true, and `1 2 and` is 0. Make a value a
flag first, with `0<>`.

#### See also

`invert`, `lshift`, `0<>`, `nand`.

#### Example

```session
/> 12 10 and . 12 10 or . 12 10 xor .
8 14 6
/> $1234 $FF and .
52
/> 3 5 < 3 0 > and .
-1
```

## invert

Core word set; in the ROM core, always there.

#### Stack

```text
invert   ( x1 -- x2 )
```

#### Description

Turns every bit of the cell over. Of a well-formed flag it is "not": `true invert` is false. Of any other
number it is not: `5 invert` is −6, which is still true. For "is zero", which is the "not" of any value, use
`0=`.

#### See also

`0=`, `negate`, `xor`.

#### Example

```session
/> 0 invert . 5 invert .
-1 -6
/> 3 5 < invert .
0
```

## lshift, rshift

Core word set; in the ROM core, always there.

#### Stack

```text
lshift   ( x1 u -- x2 )
rshift   ( x1 u -- x2 )
```

#### Description

Shift the bits of *x1* by *u* places, left or right, filling with zeros. `rshift` is a logical shift: the
sign bit is not kept, so `-1 1 rshift` is 32767. A shift of 16 places or more leaves 0.

For a right shift of a signed number by one place, keeping its sign, use `2/`.

#### See also

`2*`, `2/`, `and`, `nlshift`.

#### Example

```session
/> 1 4 lshift . 256 4 rshift .
16 16
/> -1 1 rshift .
32767
/> $1234 8 rshift .
18
```

## =, <, >, u<

Core word set; in the ROM core, always there.

#### Stack

```text
=    ( x1 x2 -- flag )
<    ( n1 n2 -- flag )
>    ( n1 n2 -- flag )
u<   ( u1 u2 -- flag )
```

#### Description

Compare two cells, and leave true (−1) or false (0). `=` is true when they are the same. `<` and `>` compare
them as signed numbers: is *n1* less than *n2*, or greater? `u<` compares them as unsigned numbers.

The difference matters for anything above 32,767, and for addresses, which are unsigned: −1 is less than 1,
but as the unsigned number 65,535 it is not.

There is no `<=` or `>=`: use `> 0=` and `< 0=`.

#### See also

`<>`, `u>`, `0=`, `0<`, `within`, `d<`, `compare`.

#### Example

```session
/> 1 1 = . 1 2 = .
-1 0
/> 1 2 < . 2 1 < .
-1 0
/> -1 1 < . -1 1 u< .
-1 0
```

## 0=, 0<

Core word set; in the ROM core, always there.

#### Stack

```text
0=   ( x -- flag )
0<   ( n -- flag )
```

#### Description

`0=` is true when the cell is zero. As every value but zero counts as true, it is also the logical "not":
it turns any true value to false, and false to true.

`0<` is true when the cell is negative, which is to say when its top bit is set.

#### See also

`0<>`, `0>`, `invert`, `=`.

#### Example

```session
/> 0 0= . 5 0= .
-1 0
/> -1 0< . 1 0< . 0 0< .
-1 0 0
```

## <>, u>, 0<>, 0>

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
<>    ( x1 x2 -- flag )
u>    ( u1 u2 -- flag )
0<>   ( x -- flag )
0>    ( n -- flag )
```

#### Description

`<>` is true when two cells differ. `u>` is true when *u1* is greater than *u2*, as unsigned numbers. `0<>`
is true when the cell is not zero: it turns any true value into the well-formed flag −1. `0>` is true when
the cell is greater than zero.

#### See also

`=`, `u<`, `0=`, `0<`.

#### Example

```session
/> 1 2 <> . 1 1 <> .
-1 0
/> -1 1 u> .
-1
/> 5 0<> . 0 0<> .
-1 0
/> 5 0> . -5 0> . 0 0> .
-1 0 0
```

## true, false

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
true    ( -- true )
false   ( -- false )
```

#### Description

The two flags, as constants: `true` is −1, a cell with every bit set, and `false` is 0.

#### See also

`0=`, `and`, `if`.

#### Example

```session
/> true . false .
-1 0
```

## within

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
within   ( n1 n2 n3 -- flag )
```

#### Description

True when *n1* is in the range that starts at *n2* and stops just before *n3*: *n2* ≤ *n1* < *n3*. The
lower limit is in the range, and the upper one is not.

The three may all be signed numbers or all unsigned ones, and the test is right for either. It is worked
out as a single unsigned comparison, (*n1* − *n2*) `u<` (*n3* − *n2*), so if the limits are given in the
wrong order the result is not simply false: it is true for the numbers *outside* the range between them.

#### See also

`<`, `u<`, `min`, `max`.

#### Example

```session
/> 5 1 10 within . 1 1 10 within . 10 1 10 within .
-1 -1 0
/> -1 -5 5 within .
-1
```
