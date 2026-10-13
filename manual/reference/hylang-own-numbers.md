## +, -, *, /

Functions: arithmetic. `+` also joins values to a string.

#### Syntax

```text
(+ x ...)
(- x ...)
(* x ...)
(/ x ...)
(+ s x ...)
```

#### Arguments

*x*
: Numbers of any kind: integers, fractions, decimals, complex numbers. There may be any number of them.

*s*
: A string. When `+`'s first argument is a string, the rest may be any values.

#### Result

With two or more numbers, the sum, the difference, the product or the quotient, worked from the left:
`(- 10 3 2)` is 10 - 3 - 2. With one number, `+` and `*` give it back, `-` gives its negative, and `/` gives its
reciprocal, 1 over it. `(+)` is 0.

#### Description

The arithmetic is exact (Chapter 18). An integer has as many digits as it needs. A division that does not come
out whole gives a fraction in its lowest terms, never a rounded decimal, and decimals add and multiply without
error: `(+ 0.1 0.2)` is exactly 0.3. A result is given in the simplest kind that holds it: a fraction and a
decimal make a fraction, and a result with no fractional part is an integer, so `(* 1.5 2)` is 3.

**Joining to a string.** If `+`'s first argument is a string, every other argument is turned to text, as
`print` would show it, and joined on: the result is a string. This is the usual way to build a message. Only
the *first* argument decides: a number first and a string later is an error.

#### Errors

| Message | Cause |
|---|---|
| `Division by zero.` | `/` with a divisor of 0 |
| `'+' adds numbers, or joins values to a string` | `+` with a first argument that is a number, and a later one that is not |
| `All parameters to operator '-' must be numbers.` | `-`, `*` or `/` (each with its own sign in the message) given anything but numbers |

#### Notes

An imaginary number is written with digits before its `i`: `2i`, `1+1i`. `1+i` is read as a symbol, and is an
error unless something has that name.

#### See also

`1+`, `sum`, `product`, `div`, `mod`, `pow`, `format`, `str-join`.

#### Example

```session
hylang> (+ 1 2 3)
=> 6
hylang> (- 5)
=> -5
hylang> (/ 1 3)
=> 1/3
hylang> (/ 6 4)
=> 3/2
hylang> (+ 0.1 0.2)
=> 0.3
hylang> (* 99999999999 99999999999)
=> 9999999999800000000001
hylang> (* 2i 2i)
=> -4
hylang> (+ "n = " 1/2 " and " {1 2})
=> "n = 1/2 and {1 2}"
hylang> (+ 1 "a")
=> Error: '+' adds numbers, or joins values to a string
```

## 1+, 1-, abs

Functions: a number plus one; minus one; and without its sign.

#### Syntax

```text
(1+ x)
(1- x)
(abs x)
```

#### Arguments

*x*
: A number.

#### Result

`1+` gives *x* + 1 and `1-` gives *x* - 1, for any kind of number. `abs` gives *x* if it is 0 or more, and its
negative if it is below 0.

#### Errors

| Message | Cause |
|---|---|
| `Cannot perform 'abs' on non-number` | `abs` of something that is not a number |
| `All parameters to operator '-' must be numbers.` | `1-` of something that is not a number |

#### Notes

`1+` is `+` with a 1, so `(1+ "a")` is not an error: it is the string `"a1"`.

`abs` does not give a complex number's size. It treats one as it treats any number, by its order: a complex
number whose real part is negative comes back with both its signs changed, and any other comes back as it is.

#### Example

```session
hylang> (1+ 5)
=> 6
hylang> (1- 5)
=> 4
hylang> (1+ 1/2)
=> 3/2
hylang> (abs -5)
=> 5
hylang> (abs -2.5)
=> 2.5
```

## truncate

A function: the whole part of a number.

#### Syntax

```text
(truncate x)
```

#### Arguments

*x*
: A real number: an integer, a fraction or a decimal.

#### Result

An integer: *x* with whatever follows the point taken away. The result is the nearest whole number toward
zero, so a negative number moves up: `(truncate -2.7)` is -2.

#### Errors

| Message | Cause |
|---|---|
| `'truncate' expects a real number` | *x* is complex, or not a number |

#### See also

`floor`, `ceil` and `round` in the library, which round down, up and to the nearest; `trunc`, which is another
name for `truncate`.

#### Example

```session
hylang> (truncate 2.7)
=> 2
hylang> (truncate -2.7)
=> -2
hylang> (truncate 7/2)
=> 3
```

## to-fixed, to-rational

Functions: a number as a decimal; and as a fraction.

#### Syntax

```text
(to-fixed x [places])
(to-rational x)
```

#### Arguments

*x*
: A real number.

*places*
: How many digits to keep after the point: 0 to 10,000. Without it, 10.

#### Result

`to-fixed` gives *x* as a decimal with at most *places* digits after the point. Digits beyond that are cut
off, not rounded: `(to-fixed 2/3 2)` is 0.66. A number that needs fewer places is not padded with zeros, and a
whole number stays an integer.

`to-rational` gives *x* as a fraction in its lowest terms, exactly. A whole number stays an integer.

#### Description

A fraction such as 1/3 has no exact decimal, so `to-fixed` is where exactness is given up, and it is the usual
last step before a number is shown to a person. `to-rational` loses nothing.

#### Errors

| Message | Cause |
|---|---|
| `'to-fixed' expects a real number` | *x* is complex, or not a number (and `'to-rational'` for `to-rational`) |
| `'to-fixed': -1 places is outside 0 to 10000` | *places* is out of range |

#### See also

`round`, `fdiv`, `to-str`, `format`.

#### Example

```session
hylang> (to-fixed 1/3)
=> 0.3333333333
hylang> (to-fixed 2/3 2)
=> 0.66
hylang> (to-fixed 1.23456 2)
=> 1.23
hylang> (to-rational 0.25)
=> 1/4
hylang> (to-rational 1.5)
=> 3/2
```

## rational.n, rational.d

Functions: the top and the bottom of a fraction.

#### Syntax

```text
(rational.n x)
(rational.d x)
```

#### Arguments

*x*
: A real number.

#### Result

`rational.n` gives the numerator and `rational.d` the denominator of *x* written as a fraction in its lowest
terms. The sign is the numerator's. An integer's denominator is 1, and a decimal is turned to a fraction
first: 0.75 gives 3 and 4.

#### Errors

| Message | Cause |
|---|---|
| `'rational.n' expects a real number` | *x* is complex, or not a number (and `'rational.d'`) |

#### Example

```session
hylang> (rational.n 3/4)
=> 3
hylang> (rational.d 3/4)
=> 4
hylang> (rational.d 5)
=> 1
hylang> (rational.n 0.75)
=> 3
```

## complex

A function: a complex number from its two parts.

#### Syntax

```text
(complex re im)
```

#### Arguments

*re*
: The real part: a real number.

*im*
: The imaginary part: a real number.

#### Result

The number *re* + *im*i. Each part keeps its own kind, and the two need not match. If *im* is 0 the result is
the real number *re*.

#### Errors

| Message | Cause |
|---|---|
| `One or more parameters passed to 'complex' is not a Number` | A part is not a number |

#### Example

```session
hylang> (complex 1 2)
=> 1+2i
hylang> (complex 1/2 0.5)
=> 1/2+0.5i
hylang> (complex 1 0)
=> 1
```

## val

A function: the number that a string spells.

#### Syntax

```text
(val s)
```

#### Arguments

*s*
: A string holding one number, written as hylang itself reads numbers. Spaces before and after it are allowed.

#### Result

The number. Any form that can be typed can be read: an integer, a fraction, a decimal, a complex number, or a
number with a base prefix. A number with no prefix is read in the current base.

#### Description

`val` is how text from a person, or from a file, becomes a number. The whole string must be the number: text
after it is an error, not ignored, so a program can tell a bad entry from a good one with `try`.

#### Errors

| Message | Cause |
|---|---|
| `'val': "abc" isn't a number` | The string is not a number, is empty, or has something after the number |
| `'val' expects a String` | *s* is not a string |

#### See also

`to-str`, `read`, `base`.

#### Example

```session
hylang> (val "42")
=> 42
hylang> (val "2/3")
=> 2/3
hylang> (val " 12 ")
=> 12
hylang> (val "#xFF")
=> 255
hylang> (val "12abc")
=> Error: 'val': "12abc" isn't a number
hylang> (try (val "abc") 0)
=> 0
```

## fib

A function: a Fibonacci number.

#### Syntax

```text
(fib n)
```

#### Arguments

*n*
: A whole number, 0 or more.

#### Result

The *n*th number of the series 0, 1, 1, 2, 3, 5, 8 ..., in which each is the sum of the two before it:
`(fib 0)` is 0 and `(fib 1)` is 1. The result is exact however long it is.

#### Errors

| Message | Cause |
|---|---|
| `'fib' parameter 'n' cannot be negative` | *n* is below 0 |
| `'fib': n must be a whole number, not 1.5` | *n* is not a whole number |

#### Example

```session
hylang> (fib 10)
=> 55
hylang> (fib 100)
=> 354224848179261915075
```

## random

A function: a random whole number.

#### Syntax

```text
(random n)
```

#### Arguments

*n*
: A whole number, 1 or more. It may be as large as any integer.

#### Result

A whole number from 0 to *n* - 1, chosen at random. `(random 6)` is one of 0 to 5: add 1 for the
throw of a die.

#### Errors

| Message | Cause |
|---|---|
| `'random' expects a number above 0` | *n* is 0 or negative |
| `'random' expects an integer` | *n* is not a whole number |

#### Example

The answers differ from run to run:

```text
hylang> (random 6)
=> 2
hylang> (1+ (random 6))
=> 4
```

## base

A function: the base that numbers are read and written in.

#### Syntax

```text
(base)
(base b)
```

#### Arguments

*b*
: A base, as a string: one of Chapter 18's base names, such as `"d"` (decimal), `"x"` (hex), `"b"` (binary) or
  `"o"` (octal), or a radix such as `"#16r"`.

#### Result

`(base)` gives the current base, as a string. `(base b)` sets it, and gives the base that was in force before,
so that a program can put it back.

#### Description

The base setting belongs to this run of hylang, and starts as decimal. While it is something else, every
number without a prefix of its own is *read* in that base, and every number is *written* in it. That includes
the numbers in the lines typed afterward: in base `"x"`, `10` is sixteen, and in base `"b"` a `5` is not a
number at all.

A number written with a prefix (`#xFF`, `#b101`) is read in its own base whatever the setting.

#### Errors

| Message | Cause |
|---|---|
| `'base' expects a string: a base ("x", "#16r", "c" ...)` | *b* is not a string. `(base 16)` is this error: write `(base "x")` |
| `'base': Invalid base specifier zz` | The string names no base |

#### See also

`hex`, `bin`, `to-str`, `val`; sections 18.3 to 18.9.

#### Example

```session
hylang> (base "x")
=> "d"
hylang> (+ 9 1)
=> A
hylang> (+ 8 8)
=> 10
hylang> (base "d")
=> "x"
hylang> (base)
=> "d"
```

## digits

A function: the precision of the math functions.

#### Syntax

```text
(digits)
(digits n)
```

#### Arguments

*n*
: A whole number from 1 to 100.

#### Result

`(digits)` gives the current precision. `(digits n)` sets it, and gives the one that was in force before.

#### Description

`sqrt`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, `pi` and `pow` give an exact answer when there is one, and
otherwise a decimal of this many significant digits. The setting starts as 12. It has no effect on `+`, `-`,
`*` and `/`, which are always exact.

#### Errors

| Message | Cause |
|---|---|
| `'digits' takes a whole number from 1 to 100` | *n* is out of range, or not a whole number |

#### Example

```session
hylang> (sqrt 2)
=> 1.41421356237
hylang> (digits 30)
=> 12
hylang> (sqrt 2)
=> 1.41421356237309504880168872421
hylang> (digits 12)
=> 30
```

## sqrt, exp, log, sin, cos, tan, atan, pi, pow

Functions: square root, exponential, natural logarithm, the trigonometric functions, pi, and powers.

#### Syntax

```text
(sqrt x)
(exp x)
(log x)
(sin x)
(cos x)
(tan x)
(atan x)
(pi)
(pow x y)
```

#### Arguments

*x*
: A real number. For `sin`, `cos` and `tan` it is an angle in radians. For `pow` with a whole *y* it may be
  any number, complex too.

*y*
: The power: any real number.

#### Result

| Function | Result |
|---|---|
| `sqrt` | The square root. Of a negative number, an imaginary one: `(sqrt -4)` is `2i` |
| `exp` | *e* to the power *x* |
| `log` | The natural logarithm (base *e*). Of a negative number, a complex one |
| `sin`, `cos`, `tan` | The sine, cosine and tangent of the angle *x* |
| `atan` | The angle, in radians, whose tangent is *x* |
| `pi` | Pi, to the precision in force. It is a function, called with no arguments |
| `pow` | *x* to the power *y* |

#### Description

Where the answer is exact, it is given exactly: `(sqrt 16)` is 4, `(sqrt 1/4)` is 1/2, `(sqrt 2.25)` is 1.5,
and `pow` with a whole power is exact for any *x*, so `(pow 2 100)` has all its digits and `(pow 2 -1)` is
1/2. A power that is a fraction is exact when the root is, as in `(pow 4 1/2)`.

Otherwise the answer is a decimal with the number of significant digits that `digits` has set, 12 to begin
with. Such an answer is rounded, so results that should be 0 can come out as a very small number:
`(sin (pi))` is not exactly 0.

Each function takes exactly the arguments shown: `log` has no second argument for a base (divide two
logarithms), and `atan` has no two-argument form.

#### Errors

| Message | Cause |
|---|---|
| `'sqrt' expects a number` | The argument is not a number (each function with its own name) |
| `'sqrt' expects a real number` | The argument is complex |
| `'log': the logarithm of 0` | `(log 0)` |
| `'pow': a negative number to a power that isn't whole` | A negative *x* with a fractional *y* |
| `Division by zero.` | 0 to a negative power |

#### See also

`digits`, `square`, `cube`, `math.pi`.

#### Example

```session
hylang> (sqrt 16)
=> 4
hylang> (sqrt 2)
=> 1.41421356237
hylang> (sqrt -4)
=> 2i
hylang> (pow 2 10)
=> 1024
hylang> (pow 2 1/2)
=> 1.41421356237
hylang> (exp 1)
=> 2.71828182846
hylang> (log 10)
=> 2.30258509299
hylang> (* 4 (atan 1))
=> 3.141592653588
hylang> (pi)
=> 3.14159265359
```

## number-bytes, bytes-number

Functions: a number as the bytes it is stored in; and back.

#### Syntax

```text
(number-bytes n)
(bytes-number b)
```

#### Arguments

*n*
: A number of any kind.

*b*
: A list of bytes (whole numbers from 0 to 255), or a buffer, holding one number in the stored format.

#### Result

`number-bytes` gives a list of bytes: the number in the Hydra's compact stored format, which every language
shares (HyBASIC's `MKN$` and `CVN` write and read the same bytes). `bytes-number` gives the number back,
exactly, in the kind it had.

#### Description

These are for keeping numbers in a file or a buffer, or passing them to a program in another language, without
turning them to text. The format has a different length for different numbers: the integers from -64 to 63
take one byte, and a long integer takes as many as it needs.

#### Errors

| Message | Cause |
|---|---|
| `'bytes-number': not a number in the stored format` | The bytes are not a whole stored number |

#### See also

`bytes`, `write-bytes`, `buffer`.

#### Example

```session
hylang> (number-bytes 0)
=> {0}
hylang> (number-bytes 258)
=> {129 2 1}
hylang> (bytes-number (number-bytes 258))
=> 258
hylang> (bytes-number (number-bytes 1/2))
=> 1/2
```

## ==, eq, neq

Functions: are two values equal, or not?

#### Syntax

```text
(== x y)
(eq x y)
(neq x y)
```

#### Arguments

*x*, *y*
: Any two values.

#### Result

`==` and `eq`, which are the same function, give `T` if the values are equal and `NIL` if not. `neq` gives the
opposite. The library adds two more names for `neq`: `<>` and `!=`.

#### Description

Two values of different kinds are never equal: the number 1 is not the string `"1"`. Within a kind:

| Kind | Equal when |
|---|---|
| Numbers | They have the same value, whatever their form: 1, 1.0 and 2/2 are equal, and so are 1/2 and 0.5 |
| Strings, characters | They have the same characters. Capitals and small letters differ |
| Atoms, symbols | They have the same name |
| Lists | They have the same length, and each item equals the other's. `NIL` equals `{}` |
| Functions | They have the same formals and the same body |

Each takes exactly two arguments.

#### See also

`cmp`, `<`, `elem?`.

#### Example

```session
hylang> (== 1 1.0)
=> T
hylang> (== 1/2 0.5)
=> T
hylang> (== "a" "A")
=> NIL
hylang> (== {1 2} {1 2})
=> T
hylang> (== 1 "1")
=> NIL
hylang> (neq 1 2)
=> T
```

## <, >, <=, >=, cmp

Functions: compare two values.

#### Syntax

```text
(< x y)
(> x y)
(<= x y)
(>= x y)
(cmp x y)
```

#### Arguments

*x*, *y*
: Any two values.

#### Result

`<`, `>`, `<=` and `>=` give `T` or `NIL`: is *x* before *y*, after it, not after it, not before it? `cmp`
gives -1 if *x* comes before *y*, 0 if they are equal, and 1 if it comes after.

#### Description

Any two values can be compared: there is no error for values of different kinds. The order is by kind first:
numbers, then characters, strings, atoms, symbols, Q-expressions, S-expressions, `T`, functions, hashes,
buffers, streams and errors. Within a kind:

| Kind | Order |
|---|---|
| Numbers | By value, whatever their form |
| Strings | Character by character, by character code: capitals come before small letters |
| Atoms, symbols | By name |
| Lists | Item by item. A list that is the start of another comes first |

This is the order `sort`, `min` and `max` use.

Each takes exactly two arguments: `(< 1 2 3)` is an error. To test a range, use `and`, or the library's
`between?`.

#### See also

`==`, `sort`, `min`, `<=>`, `between?`.

#### Example

```session
hylang> (< 1 2)
=> T
hylang> (<= 1 1)
=> T
hylang> (< 1/3 0.5)
=> T
hylang> (< "apple" "pear")
=> T
hylang> (cmp 2 1)
=> 1
hylang> (cmp {1 2} {1 3})
=> -1
hylang> (< 1 "a")
=> T
```

## neg?, pos?, zero?, one?, not

Functions: tests of a number's sign and value; and the opposite of a truth value.

#### Syntax

```text
(neg? x)
(pos? x)
(zero? x)
(one? x)
(not x)
```

#### Arguments

*x*
: Any value.

#### Result

`T` or `NIL`:

| Function | `T` when |
|---|---|
| `neg?` | *x* is a number below 0 |
| `pos?` | *x* is a number above 0 |
| `zero?` | *x* is equal to 0 (0.0 is) |
| `one?` | *x* is equal to 1 |
| `not` | *x* is `NIL`, or the empty list, which is the same |

#### Description

The four number tests are comparisons with 0 or 1, and since any two values can be compared, they give no
error for something that is not a number: `(zero? "a")` is simply `NIL`. The library's `positive?` and
`negative?` are the strict ones, which give an error for a non-number.

`not` follows hylang's rule of truth: only `NIL` is false. `(not 0)` and `(not "")` are `NIL`, because 0 and
the empty string are true.

#### See also

`positive?`, `negative?`, `odd?`, `even?`, `=0`, `and`, `or`.

#### Example

```session
hylang> (neg? -1)
=> T
hylang> (pos? 0)
=> NIL
hylang> (zero? 0.0)
=> T
hylang> (not NIL)
=> T
hylang> (not 0)
=> NIL
```

## bit-and, bit-or, bit-xor, bit-not

Functions: the bitwise operations.

#### Syntax

```text
(bit-and n ...)
(bit-or n ...)
(bit-xor n ...)
(bit-not n)
```

#### Arguments

*n*
: An integer. `bit-and`, `bit-or` and `bit-xor` take any number of them.

#### Result

An integer. Each bit of the result is worked out from the same bit of the arguments: `bit-and` gives a 1
where all have a 1, `bit-or` where any has, and `bit-xor` where an odd number have. `bit-not` turns every bit
over.

#### Description

Integers have no fixed width, so a negative number is treated as two's complement with its sign bit going on
for ever: -1 is all ones. That is why `(bit-not 0)` is -1 and `(bit-not 5)` is -6, and why
`(bit-and -1 255)` is 255. To keep a result to one byte or one word, `bit-and` it with 255 or 65535.

#### Errors

| Message | Cause |
|---|---|
| `'bit-and''s argument must be an integer` | An argument is a fraction, a decimal, or not a number (each function with its own name) |

#### See also

`shl`, `shr`, `bit?`, `lo`, `hi`, `hex`, `bin`; section 18.10.

#### Example

```session
hylang> (bit-and 12 10)
=> 8
hylang> (bit-or 12 10)
=> 14
hylang> (bit-xor 12 10)
=> 6
hylang> (bit-not 5)
=> -6
hylang> (bin (bit-and #b1100 #b1010))
=> "1000"
```

## shl, shr, bit?

Functions: shift an integer's bits left or right; and test one bit.

#### Syntax

```text
(shl n k)
(shr n k)
(bit? n k)
```

#### Arguments

*n*
: An integer.

*k*
: For the shifts, how many places: an integer. For `bit?`, which bit, bit 0 being the lowest.

#### Result

`shl` gives *n* shifted left *k* places, which is *n* times 2 to the power *k*; nothing is lost off the top,
because integers grow. `shr` gives *n* shifted right *k* places, which is *n* divided by 2 to the power *k*,
rounded down (toward minus infinity, so the sign is kept). A negative *k* shifts the other way.

`bit?` gives `T` if bit *k* of *n* is 1, and `NIL` if it is 0. A negative number is two's complement, with
all the bits above its top one set.

#### Errors

| Message | Cause |
|---|---|
| `'shr''s count must be an integer` | *k* is not an integer (each function with its own name) |

#### Example

```session
hylang> (shl 1 4)
=> 16
hylang> (shr 256 4)
=> 16
hylang> (shr -8 1)
=> -4
hylang> (shl 1 100)
=> 1267650600228229401496703205376
hylang> (bit? 5 0)
=> T
hylang> (bit? 5 1)
=> NIL
```

## hex, bin

Functions: an integer's digits in hexadecimal, or in binary, as a string.

#### Syntax

```text
(hex n [width])
(bin n [width])
```

#### Arguments

*n*
: An integer.

*width*
: The least number of digits: the string is filled out with zeros on the left to this length. A number that
  needs more digits has them all.

#### Result

A string of the digits, with no prefix. Hex digits above 9 are capitals. A negative number is a `-` and the
digits of its size: `(hex -1)` is `"-1"`, not a row of F's. For the byte or word a negative number would be in
memory, `bit-and` it first: `(hex (bit-and -1 255))` is `"FF"`.

#### Description

These give text, whatever the base setting is. To have every number written in another base, set `base`; to
turn one number to text in any base, use `to-str` with a base.

#### Errors

| Message | Cause |
|---|---|
| `'hex''s number must be an integer` | *n* is not an integer (and `'bin'`) |
| `A width must be an integer` | *width* is not an integer |

#### See also

`to-str`, `base`, `val`.

#### Example

```session
hylang> (hex 255)
=> "FF"
hylang> (hex 255 4)
=> "00FF"
hylang> (bin 5)
=> "101"
hylang> (bin 5 8)
=> "00000101"
hylang> (hex (bit-and -1 255))
=> "FF"
```

## lo, hi, word

Functions: the two bytes of a 16-bit word; and a word from two bytes.

#### Syntax

```text
(lo n)
(hi n)
(word lo hi)
```

#### Arguments

*n*
: An integer.

*lo*, *hi*
: Integers: the low byte and the high byte.

#### Result

`lo` gives the low byte of *n*, bits 0 to 7, and `hi` the next byte, bits 8 to 15: each a number from 0 to
255. Bits above 15 are ignored, and a negative number is taken as two's complement, so `(lo -1)` is 255.

`word` puts two bytes together: *lo* + 256 × *hi*. It is the opposite of `lo` and `hi`, and is how an address
read from memory a byte at a time is made whole (the 65C02 keeps the low byte first).

#### Errors

| Message | Cause |
|---|---|
| `'lo''s number must be an integer` | *n* is not an integer (and `'hi'`) |
| `A low byte must be an integer` | `word`'s *lo* is not an integer |

#### See also

`peek`, `peek-word`, `bit-and`, `shr`.

#### Example

```session
hylang> (lo #x1234)
=> 52
hylang> (hi #x1234)
=> 18
hylang> (hex (word #x34 #x12))
=> "1234"
hylang> (lo -1)
=> 255
```

## bytes, from-bytes

Functions: a string's bytes as a list of numbers; and a string from such a list.

#### Syntax

```text
(bytes s)
(from-bytes l)
```

#### Arguments

*s*
: A string.

*l*
: A list of whole numbers from 0 to 255.

#### Result

`bytes` gives a list of the character codes of *s*, one number for each byte, in order: `NIL` for the empty
string. `from-bytes` gives the string made of those bytes: the empty string for an empty list.

#### Errors

| Message | Cause |
|---|---|
| `'bytes' expects a String` | *s* is not a string |
| `256 isn't a byte` | An item of *l* is not a whole number from 0 to 255 |

#### See also

`char-code`, `code-char`, `str-chars`, `buffer`.

#### Example

```session
hylang> (bytes "AB")
=> {65 66}
hylang> (from-bytes {72 105})
=> "Hi"
hylang> (from-bytes (map 1+ (bytes "HAL")))
=> "IBM"
```
