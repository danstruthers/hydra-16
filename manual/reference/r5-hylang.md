# Chapter 5. hylang Forms

hylang is a Lisp. A program is a series of expressions, and each is evaluated to a value.

**Evaluation.** A number, a string, a character or an atom is its own value. A symbol's value is what it is
bound to, and an unbound one is an error. An expression in parentheses, an **S-expression**, is a call: its
first element is evaluated to find a function, the rest are evaluated left to right as its arguments, and the
function is applied to them. An expression in braces, a **Q-expression**, is data: a list, which is not
evaluated until something runs it. `[a b]` is short for `(list a b)`.

**Special forms** are the exception to "the arguments are evaluated first". Each receives its arguments as
they were written and decides for itself which to evaluate, and when: `if` evaluates one of its two branches,
`def` doesn't evaluate the names it is given. Their entries say exactly what each one does with each argument.
Everything else in this chapter is a function, and its arguments are values.

**Truth.** `NIL` is false, and so is the empty list, which is the same thing. Everything else is true,
including 0 and the empty string. `T` is the usual value for true.

**Scopes.** A name is looked up in the current scope, then in the one that encloses it, and so on out to the
global scope. A call of a function has a scope of its own, enclosed by the scope the function was made in, not
by the caller's. A function can therefore go on using the names that were in force where it was made, after
that code has finished: a closure.

**Functions and their arguments.** A function called with fewer arguments than it has formals is not an error.
The result is another function, which takes the rest: `((add 1) 5)` is `(add 1 5)`. Called with more than it
has, it is an error, unless the function uses `&_`, which is the list of the extra ones. A function made with
no formals refers to its arguments as `&1`, `&2` and so on.

**Errors.** An error is a value, printed as `Error: ` and a message. A function that is given an error as an
argument does not run: the error is its result. So an error travels outward through every expression that
contains it until `try` catches it or it reaches the prompt. `(error "text")` makes one.

**Numbers** are those of Chapter 18: integers of any length, exact fractions, decimals, complex numbers.

Each entry says what kind of thing it describes: a special form, a function, or a function of the library,
which is written in hylang itself and loaded as hylang starts.

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

## <=>

A special form: chooses by the sign of a number.

#### Syntax

```text
(<=> n {test value} ...)
```

#### Arguments

*n*
: An expression whose value is a number.

*test*
: One of `=`, `<>`, `<`, `>`, `<=`, `>=`.

*value*
: An expression.

#### Description

*n* is evaluated, and compared with 0. The cases are taken in order, and the first whose test is true of that
comparison has its *value* evaluated and returned: `{< x}` if *n* is negative, `{= x}` if it is zero, `{>= x}`
if it is zero or positive, and so on. If no case's test is true, the result is `NIL`.

It is the three-way branch for the result of a subtraction or of a comparison function.

#### Example

```session
hylang> (<=> -5 {< "negative"} {= "zero"} {> "positive"})
=> "negative"
hylang> (<=> 0 {< "negative"} {>= "not negative"})
=> "not negative"
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

## ??

A function of the library, written in hylang.

#### Syntax

```text
(?? a ...)
```

#### Description

The first argument that isn't `NIL`.

## =0, =1, >0, <0, >=0, <=0, !=0, !=1

A function of the library, written in hylang.

#### Syntax

```text
(=0 n)
(=1 n)
(>0 n)
(<0 n)
(>=0 n)
(<=0 n)
(!=0 n)
(!=1 n)
```

#### Description

Comparisons with 0 and 1.

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

## alpha?, digit?, space?, upper?, lower?

Functions: tests of a character's kind.

#### Syntax

```text
(alpha? c)
(digit? c)
(space? c)
(upper? c)
(lower? c)
```

#### Arguments

*c*
: A character, or a string.

#### Result

`T` or `NIL`:

| Function | `T` for |
|---|---|
| `alpha?` | A letter, A to Z or a to z |
| `digit?` | A digit, 0 to 9 |
| `space?` | A space, a tab, a line feed, a carriage return or a form feed |
| `upper?` | A capital letter |
| `lower?` | A small letter |

For a string, the test must pass for every character of it: `(digit? "12")` is `T` and `(alpha? "a1")` is
`NIL`. An empty string gives `NIL`.

#### Errors

| Message | Cause |
|---|---|
| `'alpha?' expects a String` | *c* is neither a character nor a string (each function with its own name) |

#### Example

```session
hylang> (alpha? \a)
=> T
hylang> (digit? \a)
=> NIL
hylang> (digit? "12")
=> T
hylang> (upper? "Ab")
=> NIL
hylang> (filter digit? (str-chars "a1b22"))
=> {\1 \2 \2}
```

## and, or

Special forms: the logical operators, which stop as soon as the answer is known.

#### Syntax

```text
(and x ...)
(or x ...)
```

#### Description

`and` evaluates its arguments from the left. At the first whose value is `NIL` it stops, and returns `NIL`;
the rest are not evaluated. If none is, it returns the last one's value. With no arguments it returns `T`.

`or` evaluates its arguments from the left. At the first whose value is not `NIL` it stops, and returns that
value. If all are `NIL`, or there are none, it returns `NIL`.

Because they return a value and not just `T`, they also serve as "the first of these that exists" (`or`) and
"this, provided that" (`and`).

#### Example

```session
hylang> (and 1 NIL (print "not reached"))
=> NIL
hylang> (or NIL 2 (print "not reached"))
=> 2
hylang> (list (and) (or))
=> {T NIL}
```

## any?, all?

Functions: does a test pass for any item of a list, or for every item?

#### Syntax

```text
(any? f l)
(all? f l)
```

#### Arguments

*f*
: A function of one argument: the test.

*l*
: A list.

#### Result

`any?` gives `T` if *f* returns a true value for at least one item, and `NIL` if not. `all?` gives `T` if *f*
returns a true value for every item, and `NIL` if not. The result is always `T` or `NIL`, whatever *f* itself
returned.

For an empty list `any?` is `NIL` (there is no item that passes) and `all?` is `T` (there is none that fails).

#### Description

The items are tested in order, and the testing stops as soon as the answer is known: at the first item that
passes for `any?`, and at the first that fails for `all?`. *f* is not called for the items after it.

#### Errors

| Message | Cause |
|---|---|
| `'any?' expects a function and a list` | *l* is not a list (and `'all?'` for `all?`) |

#### See also

`find`, `count`, `filter`, `elem?`.

#### Example

```session
hylang> (any? odd? {2 4 5})
=> T
hylang> (any? odd? {2 4})
=> NIL
hylang> (all? odd? {1 3})
=> T
hylang> (all? odd? {})
=> T
```

## banks, bank-alloc, bank-free, bank-read, bank-write

A function: programs, time and the system.

#### Syntax

```text
(banks)
(bank-alloc n)
(bank-free b n)
(bank-read b off n)
(bank-write b off bytes)
```

#### Description

RAM banks.

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

## between?, ><?, not-between?

A function of the library, written in hylang.

#### Syntax

```text
(between? lo hi x)
><?
(not-between? ...)
```

#### Description

Is *x* between *lo* and *hi*?

## bind, mount, unmount, ns

A function: programs, time and the system.

#### Syntax

```text
(bind new old ...)
(mount dev old ...)
(unmount old [new])
(ns)
```

#### Description

The namespace (`:before`, `:after`, `:create`)

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

## buffer

A function: buffers.

#### Syntax

```text
(buffer n [fill])
(buffer s)
(buffer l)
```

#### Description

A buffer of *n* bytes; of a string's; of a list's.

## buffer-cmp

A function: buffers.

#### Syntax

```text
(buffer-cmp b at x [i [n]])
```

#### Description

Compare with another buffer or string.

## buffer-fill, buffer-copy

A function: buffers.

#### Syntax

```text
(buffer-fill b byte [i [n]])
(buffer-copy to at from [i [n]])
```

#### Description

Fill; copy.

## buffer-get, buffer-put

A function: buffers.

#### Syntax

```text
(buffer-get b i)
(buffer-put b i byte)
```

#### Description

A byte; set one.

## buffer?

A function: buffers.

#### Syntax

```text
(buffer? x)
```

#### Description

Is it one?

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

## cd, cwd

A function: files and streams.

#### Syntax

```text
(cd [p])
(cwd)
```

#### Description

The current directory.

## char-code, code-char

Functions: a character's code; and the character with a code.

#### Syntax

```text
(char-code c)
(code-char n)
```

#### Arguments

*c*
: A character, or a string of exactly one character.

*n*
: A whole number from 0 to 255.

#### Result

`char-code` gives the character's code, 0 to 255 (Appendix E of the manual lists them). `code-char` gives the
character. The prompt shows a character with a backslash, and shows the ones that can't be printed by name:
`\space`, `\lf` for code 10, `\null` for 0.

#### Errors

| Message | Cause |
|---|---|
| `'char-code' expects one character` | *c* is a string that is empty, or longer than one character |
| `'char-code' expects a String` | *c* is neither a string nor a character |
| `'code-char': 256 isn't a character's code (0-255)` | *n* is out of range |
| `'code-char': a number must be a whole number, not 1.5` | *n* is not a whole number |

#### See also

`bytes`, `from-bytes`, `alpha?`.

#### Example

```session
hylang> (char-code \a)
=> 97
hylang> (char-code "A")
=> 65
hylang> (code-char 65)
=> \A
hylang> (code-char 32)
=> \space
```

## close

A function: files and streams.

#### Syntax

```text
(close s)
```

#### Description

Close it.

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

## cond, case

A function of the library, written in hylang.

#### Syntax

```text
(cond {test value} ...)
(case x {value result} ...)
```

#### Description

Choices.

## cons

A function: a list with one more item at its front.

#### Syntax

```text
(cons x l)
```

#### Arguments

*x*
: Any value: the new first item.

*l*
: A list. `NIL`, the empty list, is one.

#### Result

A list whose first item is *x* and whose remaining items are *l*'s. *l* itself is not changed.

#### Description

*x* goes in as one item, whatever it is: a list given as *x* becomes a list inside the result. To put two lists
end to end, use `join`.

#### Errors

| Message | Cause |
|---|---|
| `'cons' expects a list second` | *l* is not a list |

#### See also

`join`, `list`.

#### Example

```session
hylang> (cons 1 {2 3})
=> {1 2 3}
hylang> (cons {1} {2 3})
=> {{1} 2 3}
hylang> (cons 1 {})
=> {1}
hylang> (cons 1 2)
=> Error: 'cons' expects a list second
```

## def

A special form: binds names in the global scope.

#### Syntax

```text
(def {name ...} value ...)
(def name value)
```

#### Arguments

*name*
: Symbols, in a Q-expression so that they are not evaluated. One name can be written without the braces.

*value*
: Expressions, as many as there are names. Each is evaluated.

#### Result

`NIL`.

#### Description

Each name is bound to the value in the same position. The binding is made in the global scope wherever `def`
is used: inside a function, inside a `let`, it still defines a global name. A name that is bound already is
bound again to the new value.

To bind a name in the scope the code is in, use `set`. To change a name where it already is, use `set!`.

#### Errors

| Message | Cause |
|---|---|
| `'def' passed too many arguments or symbols.  Expected` *n*`, got` *m* | The names and the values don't pair up |

#### See also

`set`, `set!`, `let`, `fun`.

#### Example

```session
hylang> (def {a b} 1 2)
=> NIL
hylang> (fun {inside} {def {made-inside} 5})
=> NIL
hylang> (inside)
=> NIL
hylang> (list a b made-inside)
=> {1 2 5}
hylang> (def {a} 1 2)
=> Error: 'def' passed too many arguments or symbols.  Expected 1, got 2
```

## defined?, expr?

Special forms: tests on a name and on an expression.

#### Syntax

```text
(defined? {name})
(expr? x)
```

#### Description

`defined?` returns `T` if the name is bound, in the current scope or any that encloses it, and `NIL` if it is
not. The name is written in braces, so that it is not looked up, which would be an error for exactly the names
the test is for.

`expr?` returns `T` if its argument is something that can be evaluated.

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

## div, mod, divmod, fdiv

A function of the library, written in hylang.

#### Syntax

```text
(div a b)
(mod a b)
(divmod a b)
(fdiv a b)
```

#### Description

Division, rounding down.

## do

A special form: evaluates expressions in order.

#### Syntax

```text
(do x ...)
```

#### Description

Each expression is evaluated in turn, and the value of the last is returned; the others are evaluated for
what they do. With no expressions the result is `NIL`. It is the way to put several expressions where one is
expected, such as a branch of an `if`.

## dotimes

A special form: repeats a body a number of times.

#### Syntax

```text
(dotimes {i n} body ...)
(dotimes n f)
```

#### Description

In the first form, *n* is evaluated once, and the body is evaluated *n* times with the symbol *i* bound to 0,
then 1, up to *n* − 1. In the second form, the function *f* is called *n* times with those numbers as its
argument.

The result is `NIL`.

#### Example

```session
hylang> (dotimes {k 3} (print k))
0
1
2
=> NIL
```

## each

A special form: does something with every item of a list, or every character of a string.

#### Syntax

```text
(each {x list} body ...)
(each f list)
```

#### Description

In the first form, *list* is evaluated once, and the body is evaluated once for each of its items, in order,
with the symbol *x* bound to the item. In the second form, the function *f* is called with each item.

A string in place of the list gives its characters, one at a time.

The result is `NIL`: `each` is for what the body does. To collect results, use `map`.

#### Example

```session
hylang> (each {x {1 2 3}} (print (* x x)))
1
4
9
=> NIL
hylang> (each print "ab")
a
b
=> NIL
```

## elem?, in?

Functions: is a value one of a list's items?

#### Syntax

```text
(elem? x l)
(in? x l)
```

#### Arguments

*x*
: Any value.

*l*
: A list.

#### Result

`T` if an item of *l* is equal to *x*, and `NIL` if none is. The two names are the same function.

#### Description

Items are compared as `==` compares: numbers by value, so 1.0 is found in a list holding 1; strings exactly,
capitals and small letters being different; lists item by item.

Only the list's own items are looked at, not the items of lists inside it.

#### Errors

| Message | Cause |
|---|---|
| `'elem?' expects a list` | *l* is not a list. To look for a character or a piece of text in a string, use `index-of` or `contains?` |

#### See also

`find`, `index-of`, `any?`.

#### Example

```session
hylang> (elem? 2 {1 2 3})
=> T
hylang> (elem? 5 {1 2 3})
=> NIL
hylang> (elem? 1.0 {1 2})
=> T
hylang> (elem? {1} {{1} {2}})
=> T
hylang> (elem? "A" {"a"})
=> NIL
```

## env, setenv, unsetenv

A function: programs, time and the system.

#### Syntax

```text
(env [name])
(setenv name value)
(unsetenv name)
```

#### Description

The environment.

## error

A function: errors, output and input.

#### Syntax

```text
(error message [code])
```

#### Description

An error.

## error-message, error-code

A function: errors, output and input.

#### Syntax

```text
(error-message e)
(error-code e)
```

#### Description

Its message; its code.

## eval

A special form: evaluates a list as code.

#### Syntax

```text
(eval x)
```

#### Description

The argument is evaluated in the ordinary way, and if the result is a list, that list is then evaluated as an
S-expression: its first item as the function and the rest as its arguments. A Q-expression is thus "code not
yet run", and `eval` runs it. A result that is not a list is returned as it is.

#### Example

```session
hylang> (def {later} {+ 1 2})
=> NIL
hylang> (eval later)
=> 3
hylang> (eval (list * 6 7))
=> 42
```

## exists?, dir?, file?

A function: files and streams.

#### Syntax

```text
(exists? p)
(dir? p)
(file? p)
```

#### Description

Tests.

## exit

A special form: leaves hylang.

#### Syntax

```text
(exit [n])
exit
```

#### Description

hylang ends, with the exit code *n*, or 0 if none is given. At the prompt the bare word `exit` does the same.
The exit code is what the shell sees as the command's status.

## fexpr

A special form: makes a function that receives its arguments unevaluated.

#### Syntax

```text
(fexpr {formals} {body})
```

#### Description

A function made by `fexpr` is called like any other, but its arguments are not evaluated first: each formal is
bound to the expression that was written in that place, as data. The body can look at the expressions, and
evaluate them, or not, with `eval`, as many times as it likes.

This is how a program adds control structures of its own: a form that evaluates an argument twice, or only
under some condition, can't be an ordinary function, because by the time an ordinary function runs its
arguments have all been evaluated once already.

#### Example

```session
hylang> (def {twice} (fexpr {x} {do (eval x) (eval x)}))
=> NIL
hylang> (twice (print "hi"))
hi
hi
=> NIL
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

## filter

A function: the items of a list that pass a test.

#### Syntax

```text
(filter f l)
```

#### Arguments

*f*
: A function of one argument: the test.

*l*
: A list.

#### Result

A list of the items for which *f* returned a true value (anything but `NIL`), in their original order. If none
passes, `NIL`.

#### Description

*f* is called with each item in turn. If a call's result is an error, `filter` stops, and the error is its
result.

#### Errors

| Message | Cause |
|---|---|
| `'filter' expects a function and a list` | *l* is not a list |
| `S-Expression starts with incorrect type. Got Number, Expected Function.` | *f* is not a function |

#### See also

`map`, `find`, `count`, `remove-all`.

#### Example

```session
hylang> (filter odd? {1 2 3 4 5})
=> {1 3 5}
hylang> (filter (fn {x} {> x 2}) {1 2 3 4})
=> {3 4}
hylang> (filter odd? {2 4})
=> NIL
```

## find, count

Functions: the first item of a list that passes a test; and how many pass.

#### Syntax

```text
(find f l)
(count f l)
```

#### Arguments

*f*
: A function of one argument: the test.

*l*
: A list.

#### Result

`find` gives the first item for which *f* returns a true value, and `NIL` if there is none. `count` gives the
number of items for which it does: 0 if there are none.

#### Description

`find` stops at the first item that passes, and *f* is not called for the items after it. `count` tests every
item.

`find` gives the item, not its position. Its `NIL` cannot tell "nothing was found" from "the item found was
`NIL`": where that matters, use `any?`.

#### Errors

| Message | Cause |
|---|---|
| `'count' expects a function and a list` | *l* is not a list (and `'find'` for `find`) |
| `S-Expression starts with incorrect type. Got Number, Expected Function.` | *f* is not a function |

#### See also

`filter`, `any?`, `elem?`.

#### Example

```session
hylang> (find odd? {2 3 4 5})
=> 3
hylang> (find odd? {2 4})
=> NIL
hylang> (count odd? {1 2 3})
=> 2
```

## first, tenth

A function of the library, written in hylang.

#### Syntax

```text
first
tenth
```

#### Description

Items 1 to 10 of a list.

## flip, comp, unpack, apply, curry, pack, uncurry

A function of the library, written in hylang.

#### Syntax

```text
(flip f a b)
(comp f g x)
(unpack f l)
apply
curry
(pack f ...)
uncurry
```

#### Description

Functions on functions.

## floor, ceil, round, trunc

A function of the library, written in hylang.

#### Syntax

```text
(floor a)
(ceil a)
(round a)
(trunc a)
```

#### Description

Whole numbers.

## fn

A special form: makes a function.

#### Syntax

```text
(fn {formals} {body})
```

#### Arguments

*formals*
: The symbols the arguments will be bound to, in order. They can be none.

*body*
: A Q-expression: the expression that the function evaluates.

#### Result

A function. It has no name; `def` or `fun` gives it one.

#### Description

Nothing in the body is evaluated when the function is made. When the function is called, a new scope is made,
enclosed by the scope `fn` was used in; the formals are bound there to the arguments' values; and the body is
evaluated there. Its value is the call's.

The body is one expression. For several, wrap them in `do`.

Called with fewer arguments than formals, the function returns a function of the remaining formals, with the
ones given already bound. Called with more, it is an error, unless the body refers to `&_`, which is bound to
the list of the arguments beyond the formals. With no formals at all, the arguments are `&1`, `&2` and so on.

A function prints as `<function>` and its definition.

#### Errors

| Message | Cause |
|---|---|
| `The function takes` *n* `arguments, not` *m* | Too many arguments, in a function that doesn't use `&_` |

#### See also

`fun`, `fexpr`, `let`, `def`.

#### Example

```session
hylang> (def {add} (fn {x y} {+ x y}))
=> NIL
hylang> (list (add 1 2) ((add 1) 5))
=> {3 6}
hylang> (add 1 2 3)
=> Error: The function takes 2 arguments, not 3
hylang> ((fn {x} {list x &_}) 1 2 3)
=> {1 {2 3}}
hylang> ((fn {} {* &1 &2}) 6 7)
=> 42
```

## foldl, foldr

Functions: combine all the items of a list into one value.

#### Syntax

```text
(foldl f init l)
(foldr f init l)
```

#### Arguments

*f*
: A function of two arguments.

*init*
: The value to start from. It is the result for an empty list.

*l*
: A list.

#### Result

The value that is left when every item has been combined.

#### Description

`foldl` works from the left. It calls *f* with the value so far and the next item, `(f acc x)`, and the result
is the value so far for the next call: for a list `{a b c}` the result is `(f (f (f init a) b) c)`.

`foldr` works from the right, and calls *f* with the item first, `(f x acc)`: the result is
`(f a (f b (f c init)))`.

For a function such as `+` the two give the same answer. For one such as `-` or `cons`, where the order of the
arguments matters, they do not.

#### Errors

| Message | Cause |
|---|---|
| `'foldl' expects a function, a value and a list` | *l* is not a list (and `'foldr'` for `foldr`) |

An error from *f* is the result.

#### See also

`sum`, `product`, `map`.

#### Example

```session
hylang> (foldl + 0 {1 2 3})
=> 6
hylang> (foldl - 0 {1 2 3})
=> -6
hylang> (foldr - 0 {1 2 3})
=> 2
hylang> (foldr cons {} {1 2 3})
=> {1 2 3}
hylang> (foldl max 0 {3 9 2})
=> 9
```

## format

A function: a string with values set into it.

#### Syntax

```text
(format text x ...)
~(text x ...)
```

#### Arguments

*text*
: A string. Each `{}` in it is a place for a value.

*x*
: The values: one for each place, in order.

#### Result

The text, with each `{}` replaced by the next value, as `print` shows it: a string without its quotes, a
number in the current base.

#### Description

A pair of braces with a base name between them, such as `{x}` or `{b}`, is a place for a number, which is
written in that base whatever the base setting is. The names are Chapter 18's, and a radix such as `{#16r}`
is one too. The braces cannot hold anything else: there are no widths and no positions. To set a value to a
width, pad it first with `str-pad-left`.

The count must match: a value for every place, and no values over.

`~(text x ...)` is another way to write the call.

#### Errors

| Message | Cause |
|---|---|
| `'format' has too few arguments for its {}s` | There are more places than values |
| `'format' has more arguments than {}s` | There are more values than places |
| `'format's {x} is a base, for a number` | The value for a base's place is not a number |
| `Invalid base specifier :5` | The braces hold something that is not a base |
| `'format' expects a String` | *text* is not a string |

#### See also

`+`, `str-join`, `to-str`, `print`.

#### Example

```session
hylang> (format "{} + {} = {}" 1 2 3)
=> "1 + 2 = 3"
hylang> (format "{} is {x} in hex" 255 255)
=> "255 is FF in hex"
hylang> (format "{} and {}" 1)
=> Error: 'format' has too few arguments for its {}s
```

## from#

A function: hashes.

#### Syntax

```text
(from# h)
```

#### Description

A hash as a list of entries.

## fst, snd, thd, last, nth

Functions: one item of a list.

#### Syntax

```text
(fst l)
(snd l)
(thd l)
(last l)
(nth n l)
```

#### Arguments

*l*
: A list.

*n*
: Which item, counting from 0: a whole number from 0 to one less than the list's length.

#### Result

The item itself: `fst` the first, `snd` the second, `thd` the third, `last` the last, `nth` item *n*.

#### Description

The item is given as it is in the list, and is not evaluated. An item of a Q-expression that was written as
code is still code: `(fst {(+ 1 2) x})` is the expression `(+ 1 2)`, not 3.

The library names ten more, `first` to `tenth`, which are `nth` with its number already given.

#### Errors

| Message | Cause |
|---|---|
| `'nth': the list has no such item` | The list is too short, or *n* is negative. `fst`, `snd`, `thd` and `last` give this message too, with `nth`'s name |
| `'nth' expects a list` | *l* is not a list (a string is not one) |
| `'nth': an index must be a whole number, not 1.5` | *n* is not a whole number |

#### See also

`item-at`, `head`, `end`, `first`.

#### Example

```session
hylang> (fst {1 2 3})
=> 1
hylang> (last {1 2 3})
=> 3
hylang> (nth 2 {1 2 3})
=> 3
hylang> (fst {(+ 1 2) x})
=> (+ 1 2)
hylang> (nth 3 {1 2 3})
=> Error: 'nth': the list has no such item
```

## fun

A special form: defines a named function.

#### Syntax

```text
(fun {name formal ...} {body})
```

#### Description

`(fun {name a b} {body})` is `(def {name} (fn {a b} {body}))`: the first symbol in the braces is the
function's name and the rest are its formals. The name is global, wherever `fun` is used. The result is `NIL`.

A function can call itself by its name. A call that is the last thing a function does takes no more stack than
a loop would, so recursion of that shape can go on as long as it likes.

#### Example

```session
hylang> (fun {count-down n} {if (> n 0) (count-down (- n 1)) "done"})
=> NIL
hylang> (count-down 2000)
=> "done"
```

## gensym

A function: a new symbol that no program has written.

#### Syntax

```text
(gensym [prefix])
```

#### Arguments

*prefix*
: A string to begin the name with. Without it, `g`.

#### Result

A symbol named by the prefix, two underscores and a number that goes up by one each time `gensym` is called:
`g__1`, `g__2`, `tmp__3`. The same name is not given twice in a run of hylang.

#### Description

A `fexpr`, or any code that builds code, uses `gensym` for the names it needs, so that they cannot be the
same as a name in the code it is given.

#### Errors

| Message | Cause |
|---|---|
| `'gensym' expects a String` | *prefix* is not a string |

#### Example

The numbers depend on how many have been made:

```text
hylang> (gensym)
=> g__1
hylang> (gensym "tmp")
=> tmp__2
```

## h, hash-get

A function: hashes.

#### Syntax

```text
(h key)
(hash-get h key)
```

#### Description

A key's value (`NIL` if none)

## hash-add-tag, hash-tag?

A function: hashes.

#### Syntax

```text
(hash-add-tag h tag ...)
(hash-tag? h tag)
```

#### Description

Tags, on the hash or an entry.

## hash-call, h

A function: hashes.

#### Syntax

```text
(hash-call h key x ...)
(h key x ...)
```

#### Description

Call a method (`&0` the hash)

## hash-clone

A function: hashes.

#### Syntax

```text
(hash-clone h [override ...])
```

#### Description

A copy.

## hash-create, to#

A function: hashes.

#### Syntax

```text
(hash-create [entries])
(to# entries)
```

#### Description

A hash, from a list of `{key value}` entries.

## hash-key?, hash-keys, hash-values

A function: hashes.

#### Syntax

```text
(hash-key? h key)
(hash-keys h)
(hash-values h)
```

#### Description

Is the key there? The keys; the values.

## hash-lock, hash-make-const, hash-make-private, hash-make-not-nil

A function: hashes.

#### Syntax

```text
(hash-lock h [key ...])
(hash-make-const ...)
(hash-make-private ...)
(hash-make-not-nil ...)
```

#### Description

Protect the hash, or entries.

## hash-locked?, hash-private?, hash-const?

A function: hashes.

#### Syntax

```text
(hash-locked? h [key])
(hash-private? ...)
(hash-const? ...)
```

#### Description

Are they?

## hash-put

A function: hashes.

#### Syntax

```text
(hash-put h {key value} ...)
```

#### Description

Add or change entries.

## hash-remove

A function: hashes.

#### Syntax

```text
(hash-remove h key)
```

#### Description

Take an entry out.

## head, tail

Functions: the first item of a list, as a list; and the rest of the list.

#### Syntax

```text
(head l)
(tail l)
```

#### Arguments

*l*
: A list with at least one item.

#### Result

`head` gives a list of one item, the first. `tail` gives the list without its first item; for a list of one
item that is `NIL`.

#### Description

`head` keeps the item inside a list so that it can be run: `(eval (head l))` evaluates the first item of a list
of code. For the item itself, use `fst`.

Neither works on a string: for that there are `substring` and `char-at`.

#### Errors

| Message | Cause |
|---|---|
| `'head' passed an empty list` | *l* is `NIL` (and `'tail'` for `tail`) |
| `'head' expects a QExpr` | *l* is not a list |

#### See also

`fst`, `init`, `take`, `drop`.

#### Example

```session
hylang> (head {1 2 3})
=> {1}
hylang> (tail {1 2 3})
=> {2 3}
hylang> (tail {1})
=> NIL
hylang> (eval (head {(+ 1 2) 3}))
=> 3
hylang> (head {})
=> Error: 'head' passed an empty list
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

## hold

A special form: evaluates expressions without letting other tasks run in between.

#### Syntax

```text
(hold body ...)
```

#### Description

The body's expressions are evaluated in order, as `do` evaluates them, and the last one's value is returned.
While they run, HydraOS does not switch to another task: the program keeps the processor. It is for the few
moments when two tasks share something, a segment of memory or a device, and must not see it half changed.

Every other task waits while the body runs, so the body should be short, and must not wait for anything itself.

## if

A special form: evaluates one of two expressions.

#### Syntax

```text
(if test then [else])
```

#### Description

*test* is evaluated. If its value is anything but `NIL`, *then* is evaluated and its value returned, and *else*
is not looked at. If it is `NIL`, *else* is evaluated and returned; with no *else*, the result is `NIL`.

0 and the empty string are not `NIL`, and count as true. The empty list is `NIL`.

Each branch is one expression. For several, use `do`. A branch written in braces is not run: a Q-expression's
value is itself, so `(if T {print "x"})` returns the list `{print "x"}` and prints nothing.

#### Example

```session
hylang> (if 0 "zero counts as true" "zero counts as false")
=> "zero counts as true"
hylang> (if {} "an empty list is true" "an empty list is NIL")
=> "an empty list is NIL"
hylang> (if NIL 1)
=> NIL
```

## index-of, last-index-of

Functions: where a piece of text is in a string, or an item in a list.

#### Syntax

```text
(index-of s sub)
(last-index-of s sub)
```

#### Arguments

*s*
: A string, or a list.

*sub*
: What to look for. In a string: a string or a character. In a list: any value, which is compared with each
  item as `==` compares.

#### Result

The position, from 0, of the first place (`index-of`) or the last place (`last-index-of`) where *sub* is found.
If it isn't found, the result is -1. An empty *sub* is found at 0.

#### Description

The result -1 is a number, and so is true: test it with `>=0`, or `(< i 0)`, not with `if` alone. The
library's `contains?`, `starts-with?` and `ends-with?` do that for strings.

#### Errors

| Message | Cause |
|---|---|
| `'index-of' expects a String and a String` | *s* is a string and *sub* is neither a string nor a character |

#### See also

`contains?`, `substring`, `elem?`, `find`.

#### Example

```session
hylang> (index-of "hello" "l")
=> 2
hylang> (last-index-of "hello" "l")
=> 3
hylang> (index-of "hello" "z")
=> -1
hylang> (index-of {"a" "b"} "b")
=> 1
hylang> (substring "a.b.c" (1+ (last-index-of "a.b.c" ".")))
=> "c"
```

## init, end

Functions: a list without its last item; and its last item, as a list.

#### Syntax

```text
(init l)
(end l)
```

#### Arguments

*l*
: A list with at least one item.

#### Result

`init` gives the list without its last item (`NIL` for a list of one). `end` gives a list of one item, the last.
They are `tail` and `head` seen from the other end.

#### Errors

| Message | Cause |
|---|---|
| `'init' passed an empty list` | *l* is `NIL` (and `'end'` for `end`) |
| `'init' expects a QExpr` | *l* is not a list |

#### See also

`head`, `tail`, `last`.

#### Example

```session
hylang> (init {1 2 3})
=> {1 2}
hylang> (end {1 2 3})
=> {3}
hylang> (init {1})
=> NIL
```

## item-at

A function: one item of a list, by its position.

#### Syntax

```text
(item-at l i)
```

#### Arguments

*l*
: A list.

*i*
: The item's position, from 0.

#### Result

Item *i*, as it is in the list, not evaluated.

#### Description

`item-at` is `nth` with its arguments the other way round, and with messages that name the position.

#### Errors

| Message | Cause |
|---|---|
| `'item-at': the list has no item 2` | *i* is the list's length or more, or negative |
| `First 'item-at' parameter must be a QExpr` | *l* is not a list |

#### See also

`nth`, `subset`.

#### Example

```session
hylang> (item-at {a (+ 1 2) c} 1)
=> (+ 1 2)
hylang> (item-at {a b} 2)
=> Error: 'item-at': the list has no item 2
```

## join

A function: lists put end to end.

#### Syntax

```text
(join l ...)
```

#### Arguments

*l*
: A list. There may be any number of them, and an empty one adds nothing.

#### Result

One list, of the first list's items, then the second's, and so on.

#### Description

Only the outer lists are opened: a list that is an item of one of them is still a list in the result.

`join` is for lists. Strings are joined with `+`, or with `str-join`.

#### Errors

| Message | Cause |
|---|---|
| `Invalid parameter passed to 'join'.  Expected QExpr.` | An argument is not a list |

#### See also

`cons`, `list`, `flatten`.

#### Example

```session
hylang> (join {1 2} {3} {} {4 5})
=> {1 2 3 4 5}
hylang> (join {1} {{2}})
=> {1 {2}}
hylang> (join {1} 2)
=> Error: Invalid parameter passed to 'join'.  Expected QExpr.
```

## key, key?

A function: errors, output and input.

#### Syntax

```text
(key)
(key?)
```

#### Description

The next key, as it's pressed; is one waiting?

## len

A function: the length of a list, a string, a hash or a buffer.

#### Syntax

```text
(len x)
```

#### Arguments

*x*
: A list, a string, a hash or a buffer.

#### Result

A whole number: a list's items (a list inside it counts as one), a string's characters, a hash's entries, a
buffer's bytes. The empty list and the empty string have length 0.

#### Errors

| Message | Cause |
|---|---|
| `'len' requires parameter of type string, list, hash or buffer` | *x* is a number, an atom, or anything else that has no length |

#### Example

```session
hylang> (len {1 2 3})
=> 3
hylang> (len {a {b c} d})
=> 3
hylang> (len "hello")
=> 5
hylang> (len {})
=> 0
hylang> (len 5)
=> Error: 'len' requires parameter of type string, list, hash or buffer
```

## let

A special form: evaluates expressions in a new scope, with names bound in it first.

#### Syntax

```text
(let {{name [value]} ...} body ...)
(let body)
```

#### Arguments

*name*
: A symbol.

*value*
: An expression: what the name is bound to. If it is left out, the name is bound to `NIL`.

*body*
: An expression. It can also be written as a Q-expression, `{print x}`, which is then run as code.

#### Result

The value of the last *body* expression.

#### Description

1. A new scope is made, enclosed by the scope the `let` is in.
2. The bindings are taken in order. For each, *value* is evaluated **in the new scope**, and *name* is bound to
   the result there. A value can therefore use the names bound before it in the same `let`; a name bound twice
   keeps its last value.
3. The *body* expressions are evaluated in order, in the new scope. The values of all but the last are discarded.
4. The new scope is dropped, unless something made in it still refers to it: a function made by `fn` in the body
   keeps the scope, and can go on using its names after the `let` has returned (a closure).

A name bound by `let` hides one of the same name outside it for as long as the body runs, and the outer one is
unchanged afterwards.

Inside the body, `set` binds or changes a name in the `let`'s own scope; `set!` changes a name in whichever
scope it was made, which may be outside; and `def` always binds globally.

The last body expression is evaluated in tail position: a function whose body ends in a `let` that ends in a
call to itself runs in constant stack, however many times it repeats.

With one argument only, that argument is the body: `(let body)` evaluates it in a new, empty scope. This means
`(let {{a 1}})` does not bind anything; its one argument is run as code, and the result is the list `{a 1}`.

#### Errors

| Message | Cause |
|---|---|
| `A 'let' binding is {name value}` | A binding that isn't a list, whose first element isn't a symbol, or that has more than two elements |
| `'let' supplied too few parameters` | No arguments at all |

An error from evaluating a *value* or a *body* expression ends the `let` at once, and is its result.

#### See also

`def`, `set`, `set!`, `fn`, `do`.

#### Example

```session
hylang> (let {{x 2} {y (+ x 1)}} (list x y))
=> {2 3}
hylang> (def {x} 10)
=> NIL
hylang> (let {{x 1}} (set {x} 5) x)
=> 5
hylang> x
=> 10
hylang> (def {add5} (let {{n 5}} (fn {a} {+ a n})))
=> NIL
hylang> (add5 1)
=> 6
hylang> n
=> Error: Unbound Symbol 'n'
hylang> (let {a 1} a)
=> Error: A 'let' binding is {name value}
```

## list

A function: makes a list of its arguments' values.

#### Syntax

```text
(list x ...)
[x ...]
```

#### Arguments

*x*
: Any value. There may be any number of them.

#### Result

A list of the values, in order. With no arguments, `NIL`, which is the empty list.

#### Description

`list` is how a list is made from values that have to be worked out. A Q-expression, `{a b}`, is a list of what
was written, with nothing evaluated; `(list a b)` is a list of what `a` and `b` evaluate to. Square brackets are
another way to write the call: `[a b]` is read as `(list a b)`.

An argument that is itself a list becomes one item of the result: `list` nests, where `join` flattens.

#### See also

`cons`, `join`, `range`.

#### Example

```session
hylang> (list 1 (+ 1 1) "three")
=> {1 2 "three"}
hylang> [1 (+ 1 1)]
=> {1 2}
hylang> (list (+ 1 2) {+ 1 2})
=> {3 {+ 1 2}}
hylang> (list)
=> NIL
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

## load

A special form: evaluates the expressions in files.

#### Syntax

```text
(load path ...)
```

#### Description

Each file is read and its expressions are evaluated in order. It is how a program is split across files, and
how a library is brought in.

A file that can't be read is an error, with the system's message and the file's name.

#### Example

```session
hylang> (load "/ram/nothere.hl")
=> Error: /ram/nothere.hl: not found
```

## ls, dir, stat, glob

A function: files and streams.

#### Syntax

```text
(ls [p])
(dir [p])
(stat p)
(glob pattern)
```

#### Description

Names; their details; one's; those matching.

## map

A function: a list of a function's results, one for each item of a list.

#### Syntax

```text
(map f l)
```

#### Arguments

*f*
: A function of one argument.

*l*
: A list.

#### Result

A list as long as *l*: *f* of its first item, *f* of its second, and so on. For `NIL` the result is `NIL`.

#### Description

*f* is called with each item in turn, from the first. If a call's result is an error, `map` stops there, and
the error is `map`'s result.

`map` takes exactly one list. To walk two lists together, `zip` them first.

#### Errors

| Message | Cause |
|---|---|
| `'map' expects a function and a list` | *l* is not a list (a string is not one) |
| `S-Expression starts with incorrect type. Got Number, Expected Function.` | *f* is not a function |
| `'map' takes 2 arguments, not 3` | More than one list was given |

#### See also

`each`, `filter`, `foldl`, `zip`.

#### Example

```session
hylang> (map 1+ {1 2 3})
=> {2 3 4}
hylang> (map (fn {x} {* x x}) {1 2 3})
=> {1 4 9}
hylang> (map len {"a" "bb" {1 2 3}})
=> {1 2 3}
hylang> (map (fn {x} {/ 1 x}) {1 0 2})
=> Error: Division by zero.
```

## math.pi, math.e, math.phi, math.gamma

A function of the library, written in hylang.

#### Syntax

```text
math.pi
math.e
math.phi
math.gamma
```

#### Description

Constants, to many places.

## min, max

Functions: the least of their arguments; and the greatest.

#### Syntax

```text
(min x ...)
(max x ...)
```

#### Arguments

*x*
: Any values: at least one.

#### Result

The argument that comes first in order (`min`) or last (`max`).

#### Description

The arguments are compared as `<` and `>` compare, which is `cmp`'s order: numbers by value whatever their
form, strings character by character, and values of different kinds by kind, numbers before strings before
atoms. So the arguments need not all be numbers.

These compare their *arguments*. Given one list, they have one argument, and return it: `(min {3 1 2})` is
`{3 1 2}`. For the least item of a list, use the library's `minimum` and `maximum`.

#### See also

`minimum`, `maximum`, `sort`, `cmp`.

#### Example

```session
hylang> (min 3 1 2)
=> 1
hylang> (max 3 1 2)
=> 3
hylang> (min 1/2 0.3)
=> 0.3
hylang> (min "b" "a")
=> "a"
hylang> (min {3 1 2})
=> {3 1 2}
```

## minimum, maximum, sort-desc, best-of, worst-of

A function of the library, written in hylang.

#### Syntax

```text
(minimum ...)
(maximum ...)
(sort-desc l)
(best-of n l)
(worst-of n l)
```

#### Description

Order.

## mkdir, remove, rename, copy-file

A function: files and streams.

#### Syntax

```text
(mkdir p)
(remove p)
(rename p q)
(copy-file p q)
```

#### Description

Directories and files.

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

## newns

A special form: rebuilds the namespace.

#### Syntax

```text
(newns)
```

#### Description

hylang's namespace is thrown away and the default one built again from `/lib/namespace`, as a login shell
does when it starts and as rc's `newns` does. Every `bind` and `mount` the program has made is undone.

## note, note-group, on-note

A function: programs, time and the system.

#### Syntax

```text
(note task n)
(note-group g n)
(on-note f)
```

#### Description

Notes.

## num?, int?, fixed?, rational?, complex?

Functions: is a value a number, and of which kind?

#### Syntax

```text
(num? x)
(int? x)
(fixed? x)
(rational? x)
(complex? x)
```

#### Arguments

*x*
: Any value.

#### Result

`T` or `NIL`:

| Function | `T` when *x* is |
|---|---|
| `num?` | A number of any kind |
| `int?` | An integer |
| `fixed?` | A decimal, such as 1.5. 1.0 is one, though its value is whole |
| `rational?` | A fraction, such as 1/2 |
| `complex?` | A complex number, with an imaginary part that isn't 0 |

#### Description

The four kinds go by the form the number is in, not by its value: 1.0 is `fixed?` and not `int?`, and 0.5 is
not `rational?`, though it equals 1/2. Arithmetic gives each result in the simplest form that holds it (see
`+`), so `(/ 4 2)` is an integer and `(/ 1 4)` is a fraction, and a complex number whose imaginary part comes
to 0 is real.

Nothing here is an error: a value that is not a number gives `NIL`.

#### See also

`type-of`, `zero?`, `positive?`, `to-fixed`, `to-rational`.

#### Example

```session
hylang> (num? 42)
=> T
hylang> (num? "42")
=> NIL
hylang> (int? 1.0)
=> NIL
hylang> (fixed? 1.0)
=> T
hylang> (rational? (/ 1 4))
=> T
hylang> (complex? 2i)
=> T
hylang> (filter num? {1 "a" 2/3 :b})
=> {1 2/3}
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

## odd?, even?, positive?, negative?

A function of the library, written in hylang.

#### Syntax

```text
(odd? n)
(even? n)
(positive? a)
(negative? a)
```

#### Description

Tests.

## open

A function: files and streams.

#### Syntax

```text
(open path [mode])
```

#### Description

A stream on a file: `:read`, `:write`, `:append` or `:update`.

## output-of

A special form: collects what expressions print.

#### Syntax

```text
(output-of x ...)
```

#### Description

The expressions are evaluated in order, with standard output going into a string in place of the screen. The
string is the result; the expressions' own values are discarded. Line ends are in the string as they were
printed.

#### Example

```session
hylang> (output-of (print "a") (print "b"))
=> "a\nb\n"
```

## peek, poke, peek-word, poke-word

A function: programs, time and the system.

#### Syntax

```text
(peek a)
(poke a b)
(peek-word a)
(poke-word a n)
```

#### Description

Memory.

## pid

A function: programs, time and the system.

#### Syntax

```text
(pid)
```

#### Description

This task.

## platform, hydra?

A function: programs, time and the system.

#### Syntax

```text
(platform)
(hydra?)
```

#### Description

`:hydra`; `T`.

## print

A function: errors, output and input.

#### Syntax

```text
(print x ...)
```

#### Description

Print the values, spaces between, and a new line.

## print-to, write-to

A function: files and streams.

#### Syntax

```text
(print-to s x ...)
(write-to s x ...)
```

#### Description

Write to it.

## qexpr?, sexpr?, function?, error?

Functions: is a value a list, an unevaluated call, a function, or an error?

#### Syntax

```text
(qexpr? x)
(sexpr? x)
(function? x)
(error? x)
```

#### Arguments

*x*
: Any value.

#### Result

`T` or `NIL`:

| Function | `T` when *x* is |
|---|---|
| `qexpr?` | A list, the empty one too |
| `sexpr?` | An S-expression that has not been evaluated: a call, as data |
| `function?` | A function: one made with `fn` or `fun`, a built-in one, or a special form |
| `error?` | An error |

#### Description

`error?` can be given an error: it answers `T`, where another function would not run, and would pass the
error on. With `try` it is one of the two ways to deal with an error.

An S-expression is only seen as one when it comes out of a list unevaluated, or from `read`.

There is a test for buffers too, `buffer?`.

#### See also

`type-of`, `try`, `error`, `eval`.

#### Example

```session
hylang> (qexpr? {1})
=> T
hylang> (qexpr? "a")
=> NIL
hylang> (sexpr? (fst {(+ 1 2)}))
=> T
hylang> (function? len)
=> T
hylang> (error? (error "x"))
=> T
hylang> (error? 5)
=> NIL
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

## range

A function: a list of evenly spaced numbers.

#### Syntax

```text
(range to)
(range from to [step])
```

#### Arguments

*from*
: The first number. With one argument, 0.

*to*
: Where to stop. *to* itself is never in the list.

*step*
: The difference between one number and the next: 1 if it isn't given. It may be negative, to count down, and
  it may be a fraction or a decimal, as *from* and *to* may be.

#### Result

A list of *from*, *from* + *step*, and so on, for as long as the number is before *to*: less than it for a
positive step, greater for a negative one. If the first number is already at or past *to*, the result is `NIL`.

#### Description

`(range n)` is the numbers 0 to *n* - 1: *n* of them, which are also the positions of a list of *n* items. The
arithmetic is exact, so a fractional step never drifts.

#### Errors

| Message | Cause |
|---|---|
| `'range' step cannot be 0` | *step* is 0 |
| `'range' expects Numbers` | An argument is not a number |
| `'range' takes at most 3 arguments, not 4` | Too many arguments |

#### See also

`dotimes`, `map`.

#### Example

```session
hylang> (range 5)
=> {0 1 2 3 4}
hylang> (range 2 6)
=> {2 3 4 5}
hylang> (range 5 0 -2)
=> {5 3 1}
hylang> (range 0 1 1/4)
=> {0 1/4 1/2 3/4}
hylang> (range 5 0)
=> NIL
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

## read

A function: the expressions that a string of text holds.

#### Syntax

```text
(read text)
```

#### Arguments

*text*
: A string of hylang source.

#### Result

A list of the expressions in the text, in order, as the reader makes them and with nothing evaluated: a call
written in parentheses is an S-expression in the list. Empty text gives `NIL`.

#### Description

`read` is the reader on its own: the first half of what `load` and the prompt do. `eval` is the second half.
Given the list, `eval` runs the expressions in turn and gives the last one's value; given one item of it,
`eval` runs that one.

#### Errors

| Message | Cause |
|---|---|
| `'read': missing )` | The text ends inside an expression (the message names what is missing) |
| `'read' expects a String` | *text* is not a string |

#### See also

`eval`, `load`, `val`, `repr`.

#### Example

```session
hylang> (read "(+ 1 2)")
=> {(+ 1 2)}
hylang> (read "1 2 3")
=> {1 2 3}
hylang> (eval (read "(+ 1 2)"))
=> 3
hylang> (read "(+ 1")
=> Error: 'read': missing )
```

## read-buffer

A function: buffers.

#### Syntax

```text
(read-buffer s b [at [n]])
```

#### Description

Read a stream into it.

## read-bytes, write-bytes

A function: files and streams.

#### Syntax

```text
(read-bytes s n)
(write-bytes s bytes [i [n]])
```

#### Description

Bytes.

## read-file, read-lines

A function: files and streams.

#### Syntax

```text
(read-file p)
(read-lines p)
```

#### Description

A whole file; its lines.

## read-line, read-byte, read-all

A function: errors, output and input.

#### Syntax

```text
(read-line [s])
(read-byte [s])
(read-all [s])
```

#### Description

Read a line, a byte, the rest (from the keyboard, or a stream)

## remove-fst, remove-all, remove-min, remove-max

A function of the library, written in hylang.

#### Syntax

```text
(remove-fst f l)
(remove-all f l)
(remove-min ...)
(remove-max ...)
```

#### Description

Removing items.

## repeat, times

A function of the library, written in hylang.

#### Syntax

```text
(repeat n d)
times
```

#### Description

A list of *d*'s value *n* times.

## reverse

A function: a list, or a string, backwards.

#### Syntax

```text
(reverse x)
```

#### Arguments

*x*
: A list or a string.

#### Result

A list of the same items in the opposite order, or a string of the same characters in the opposite order. Only
the outer list is reversed: a list inside it keeps its own order.

#### Errors

| Message | Cause |
|---|---|
| `'reverse' expects a QExpr or a String` | *x* is neither |

#### Example

```session
hylang> (reverse {1 2 3})
=> {3 2 1}
hylang> (reverse {1 {2 3} 4})
=> {4 {2 3} 1}
hylang> (reverse "hello")
=> "olleh"
```

## run, spawn, wait, kill

A function: programs, time and the system.

#### Syntax

```text
(run prog x ...)
(spawn prog x ...)
(wait task)
(kill task)
```

#### Description

Programs.

## save

A function: errors, output and input.

#### Syntax

```text
(save x [path mode ...])
```

#### Description

Print *x* as it reads back, or write it to a file.

## seek, tell

A function: files and streams.

#### Syntax

```text
(seek s n)
(tell s)
```

#### Description

Its place.

## seg-create, seg-attach, seg-detach, seg-read, seg-write

A function: programs, time and the system.

#### Syntax

```text
(seg-create n)
(seg-attach s)
(seg-detach s)
(seg-read s b off n)
(seg-write s b off bytes)
```

#### Description

Shared segments.

## set

A special form: binds names in the current scope.

#### Syntax

```text
(set {name ...} value ...)
```

#### Description

As `def`, but the binding is made in the scope the `set` is in: inside a function or a `let`, the name is
local to it, and a global of the same name is hidden, not changed. At the prompt, where the current scope is
the global one, `set` and `def` are the same.

If the name is already bound in the current scope, it is bound again to the new value. A name bound in an
enclosing scope is not touched: `set` makes a new one that hides it. To change the outer one, use `set!`.

The result is `NIL`.

#### Example

```session
hylang> (def {a} 5)
=> NIL
hylang> (fun {local} {do (set {a} 99) a})
=> NIL
hylang> (list (local) a)
=> {99 5}
```

## set!

A special form: changes names where they are already bound.

#### Syntax

```text
(set! {name ...} value ...)
```

#### Description

As `def`, but each name must be bound already, and it is changed in whichever scope holds it: the nearest one,
looking outward from the current scope. This is how a function changes a variable that belongs to the code
around it, or a global.

The result is `NIL`.

#### Errors

| Message | Cause |
|---|---|
| `Unbound Symbol '`*name*`'` | The name is not bound in any scope |

#### Example

```session
hylang> (def {total} 0)
=> NIL
hylang> (fun {add-to-total n} {set! {total} (+ total n)})
=> NIL
hylang> (do (add-to-total 5) (add-to-total 7) total)
=> 12
hylang> (set! {nosuch} 1)
=> Error: Unbound Symbol 'nosuch'
```

## sh, sh-out

A function: programs, time and the system.

#### Syntax

```text
(sh line [input])
(sh-out line [input])
```

#### Description

Command lines (their status; their output)

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

## sleep, sleep-until, yield

A function: programs, time and the system.

#### Syntax

```text
(sleep s)
(sleep-until tick)
(yield)
```

#### Description

Wait.

## sort

A function: a list in order.

#### Syntax

```text
(sort l [less])
```

#### Arguments

*l*
: A list.

*less*
: A function of two items, which returns a true value when the first must come before the second. Without it,
  the items are put in `cmp`'s order.

#### Result

A new list of the same items in order. *l* is not changed.

#### Description

Without *less* the order is that of `cmp`: by kind first (numbers before characters, then strings, atoms,
symbols and lists), then by value. Numbers are compared by value whatever their form, strings character by
character, and lists item by item, a list that is the start of another coming first. So a list of mixed kinds
sorts without an error.

The sort is a merge sort, and it is stable: items that *less* does not put in order keep the order they had.
To sort records by one field, give a *less* that compares that field.

#### Errors

| Message | Cause |
|---|---|
| `'sort' expects a QExpr` | *l* is not a list (a string is not one) |
| `Second 'sort' parameter must be a function` | *less* is not a function |

#### See also

`cmp`, `min`, `max`, `sort-desc`.

#### Example

```session
hylang> (sort {3 1 2})
=> {1 2 3}
hylang> (sort {3 1 2} >)
=> {3 2 1}
hylang> (sort {"pear" "apple" "fig"})
=> {"apple" "fig" "pear"}
hylang> (sort {3 "a" 1})
=> {1 3 "a"}
hylang> (sort {{2 "b"} {1 "a"}} (fn {a b} {< (fst a) (fst b)}))
=> {{1 "a"} {2 "b"}}
```

## split, zip, flatten, distinct, avg

A function of the library, written in hylang.

#### Syntax

```text
(split n l)
(zip a b)
(flatten l)
(distinct l)
(avg l)
```

#### Description

Lists.

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

## square, cube, recip, inverse

A function of the library, written in hylang.

#### Syntax

```text
(square n)
(cube n)
(recip n)
inverse
```

#### Description

*n*², *n*³, 1/*n*.

## starts-with?, ends-with?, contains?

A function of the library, written in hylang.

#### Syntax

```text
(starts-with? s sub)
(ends-with? s sub)
(contains? s sub)
```

#### Description

Strings.

## str-pad-left, str-pad-right

Functions: a string filled out to a length.

#### Syntax

```text
(str-pad-left s n [c])
(str-pad-right s n [c])
```

#### Arguments

*s*
: A string. A number must be made one first, with `to-str`.

*n*
: The length wanted: a whole number, 0 or more.

*c*
: What to fill with: a character, or a string of one character. Without it, a space.

#### Result

A string of at least *n* characters. `str-pad-left` puts the filling before *s*, which sets it to the right of
a column, as numbers are set; `str-pad-right` puts it after. A string already *n* long or longer is returned
whole: it is never cut.

#### Errors

| Message | Cause |
|---|---|
| `'str-pad-left' expects a String` | *s* is not a string (and `'str-pad-right'`) |
| `'str-pad-left' pads with one character` | *c* is a string of more than one character |
| `'str-pad-left' length cannot be negative` | *n* is below 0 |
| `'str-pad-left': a number must be a whole number, not 1.5` | *n* is not a whole number |

#### See also

`format`, `to-str`, `hex`.

#### Example

```session
hylang> (str-pad-left (to-str 42) 6)
=> "    42"
hylang> (str-pad-left "7" 3 "0")
=> "007"
hylang> (str-pad-right "ab" 5 "*")
=> "ab***"
hylang> (str-pad-left "1234" 3)
=> "1234"
```

## str-replace, str-repeat

Functions: a string with every copy of some text replaced; and a string repeated.

#### Syntax

```text
(str-replace s old new)
(str-repeat s n)
```

#### Arguments

*s*
: A string. For `str-repeat` it may be a character.

*old*
: The text to look for: a string that is not empty, or a character.

*new*
: The text to put in its place: a string, which may be empty, or a character.

*n*
: How many times: a whole number, 0 or more.

#### Result

`str-replace` gives *s* with every place where *old* is found replaced by *new*. The string is gone through
once, from the left, and what has been put in is not looked at again: replacing `"a"` by `"aa"` in `"aaa"`
gives six a's and stops. If *old* is not found, the result is *s*.

`str-repeat` gives *n* copies of *s*, one after another: the empty string for 0.

#### Errors

| Message | Cause |
|---|---|
| `'str-replace' cannot replace an empty String` | *old* is `""` |
| `'str-replace' expects a String` | An argument is not a string or a character |
| `'str-repeat' count cannot be negative` | *n* is below 0 |
| `'str-repeat': a number must be a whole number, not 1.5` | *n* is not a whole number |

#### Example

```session
hylang> (str-replace "a-b-c" "-" "+")
=> "a+b+c"
hylang> (str-replace "abc" "b" "")
=> "ac"
hylang> (str-replace "aaa" "a" "aa")
=> "aaaaaa"
hylang> (str-repeat "ab" 3)
=> "ababab"
```

## str-split, str-join, str-chars

Functions: a string cut into a list of strings; a list joined into a string; and a string's characters.

#### Syntax

```text
(str-split s [sep])
(str-join l [sep])
(str-chars s)
```

#### Arguments

*s*
: A string.

*sep*
: For `str-split`, the text to cut at: a string, or a list of strings, any of which cuts. Without it, a space.
  For `str-join`, the text to put between the items: without it, nothing.

*l*
: A list. Its items need not be strings: each is turned to text as `print` shows it.

#### Result

`str-split` gives a list of the pieces of *s* between the separators, without the separators. Two separators
together have an empty string between them, and the list always has at least one item: a string with no
separator in it gives a list of itself.

`str-join` gives one string: the items, with *sep* between each two. An empty list gives the empty string.

`str-chars` gives a list of the string's characters (`NIL` for the empty string). They are characters, not
strings of one character.

#### Errors

| Message | Cause |
|---|---|
| `First 'str-split' parameter must be a string` | *s* is not a string |
| `Second 'str-split' parameter must be a list of strings or a single string` | *sep* is something else: a character is not accepted |
| `'str-join' expects a QExpr` | *l* is not a list |
| `'str-chars' expects a String` | *s* is not a string |

#### See also

`read-lines`, `format`, `bytes`, `+`.

#### Example

```session
hylang> (str-split "one two three")
=> {"one" "two" "three"}
hylang> (str-split "a,b,,c" ",")
=> {"a" "b" "" "c"}
hylang> (str-split "a,b;c" {"," ";"})
=> {"a" "b" "c"}
hylang> (str-join {"a" "b" "c"} ", ")
=> "a, b, c"
hylang> (str-join {1 2 3} "-")
=> "1-2-3"
hylang> (str-chars "abc")
=> {\a \b \c}
hylang> (str-join (reverse (str-split "a b c")) " ")
=> "c b a"
```

## str-upper, str-lower, str-trim

Functions: a string in capitals; in small letters; and without the blank space at its ends.

#### Syntax

```text
(str-upper s)
(str-lower s)
(str-trim s)
```

#### Arguments

*s*
: A string. `str-upper` and `str-lower` also take a character, and give a character.

#### Result

A new string. `str-upper` and `str-lower` change the letters A to Z and leave every other character as it is.
`str-trim` removes the spaces, tabs and line ends from the start and the end, and leaves those in the middle.

#### Errors

| Message | Cause |
|---|---|
| `'str-upper' expects a String` | *s* is neither a string nor a character (each function with its own name) |

#### Example

```session
hylang> (str-upper "Hello 1")
=> "HELLO 1"
hylang> (str-lower "Hello 1")
=> "hello 1"
hylang> (str-trim "  hi  ")
=> "hi"
hylang> (str-upper \a)
=> \A
```

## string?, char?, atom?, symbol?

Functions: is a value a string, a character, an atom, or a symbol?

#### Syntax

```text
(string? x)
(char? x)
(atom? x)
(symbol? x)
```

#### Arguments

*x*
: Any value.

#### Result

`T` or `NIL`:

| Function | `T` when *x* is |
|---|---|
| `string?` | A string. A character is not a string, even a string of one character |
| `char?` | A character, such as `\a` |
| `atom?` | An atom, such as `:name` |
| `symbol?` | A symbol that has not been evaluated |

#### Description

A symbol written as an argument is evaluated before the function sees it, so `symbol?` is only `T` for a
symbol taken out of a list, made by `to-sym` or `gensym`, or read by `read`.

#### See also

`type-of`, `to-sym`, `to-atom`, `alpha?`.

#### Example

```session
hylang> (string? "a")
=> T
hylang> (string? \a)
=> NIL
hylang> (char? \a)
=> T
hylang> (atom? :a)
=> T
hylang> (symbol? (fst {a}))
=> T
hylang> (symbol? :a)
=> NIL
```

## subset

A function: part of a list.

#### Syntax

```text
(subset l i [n])
```

#### Arguments

*l*
: A list.

*i*
: Where the part starts, from 0. It may be the list's length, which gives `NIL`.

*n*
: How many items. Without it, or if it is more than there are, all the items from *i* to the end.

#### Result

A new list of the items.

#### Errors

| Message | Cause |
|---|---|
| `'subset': the list has no item 6` | *i* is more than the list's length, or negative |
| `First 'subset' parameter must be a QExpr` | *l* is not a list. For part of a string, use `substring` |

#### See also

`take`, `drop`, `substring`.

#### Example

```session
hylang> (subset {1 2 3 4 5} 1 3)
=> {2 3 4}
hylang> (subset {1 2 3 4 5} 3)
=> {4 5}
hylang> (subset {1 2 3 4 5} 2 99)
=> {3 4 5}
hylang> (subset {1 2 3 4 5} 5)
=> NIL
```

## substring, char-at

Functions: part of a string; and one character of it.

#### Syntax

```text
(substring s i [n])
(char-at s i)
```

#### Arguments

*s*
: A string.

*i*
: A position in the string, from 0. For `substring` it may be the string's length, which gives the empty
  string; for `char-at` it must be less.

*n*
: How many characters. Without it, or if it is more than there are, all the characters from *i* to the end.

#### Result

`substring` gives a string. `char-at` gives a character, which is a different kind of value from a string of
one character: `(char-at "abc" 0)` is `\a`, and is not equal to `"a"`. `to-str`, or `+` after a string, makes
a character a string.

#### Errors

| Message | Cause |
|---|---|
| `'substring': index 6 is outside the string` | *i* is negative, or past the end (and `'char-at'`) |
| `'substring': a count can't be negative` | *n* is below 0 |
| `'substring': an index must be a whole number, not 1.5` | *i* or *n* is not a whole number |
| `First 'substring' parameter must be a String` | *s* is not a string. For part of a list, use `subset` |
| `'char-at' expects a String` | `char-at`'s *s* is not a string |

#### See also

`index-of`, `str-chars`, `subset`, `nth`.

#### Example

```session
hylang> (substring "hello" 1 3)
=> "ell"
hylang> (substring "hello" 1)
=> "ello"
hylang> (substring "hello" 5)
=> ""
hylang> (char-at "hello" 1)
=> \e
hylang> (substring "hello" 6)
=> Error: 'substring': index 6 is outside the string
```

## sum, product

Functions: all the numbers of a list added together; and multiplied together.

#### Syntax

```text
(sum l)
(product l)
```

#### Arguments

*l*
: A list of numbers.

#### Result

The total, or the product, exact as all of hylang's arithmetic is. For an empty list `sum` is 0 and `product`
is 1.

#### Errors

| Message | Cause |
|---|---|
| `'sum' expects a list` | *l* is not a list (and `'product'` for `product`) |
| `'+' adds numbers, or joins values to a string` | `sum`: an item is not a number |
| `All parameters to operator '*' must be numbers.` | `product`: an item is not a number |

#### See also

`+`, `*`, `foldl`, `avg`.

#### Example

```session
hylang> (sum {1 2 3})
=> 6
hylang> (sum {1/2 1/3})
=> 5/6
hylang> (product {2 3 4})
=> 24
hylang> (sum {})
=> 0
```

## sys, sys-, x

A function: programs, time and the system.

#### Syntax

```text
(sys name x ...)
(sys-
x ...)
```

#### Description

A system call (Appendix K)

## sysinfo, mods, ps, task-info, errstr, free

A function: programs, time and the system.

#### Syntax

```text
(sysinfo)
(mods)
(ps)
(task-info task)
(errstr code)
(free)
```

#### Description

The system.

## t?, nil?

Functions: is a value `T`, or `NIL`?

#### Syntax

```text
(t? x)
(nil? x)
```

#### Arguments

*x*
: Any value.

#### Result

`t?` gives `T` only for `T` itself: other values that count as true, such as 1, are not `T`.

`nil?` gives `T` for `NIL` and for the empty list, which are the same value. 0 and the empty string are not
`NIL`. `nil?` does the same as `not`.

#### See also

`not`, `and`, `or`, `if`.

#### Example

```session
hylang> (t? T)
=> T
hylang> (t? 1)
=> NIL
hylang> (nil? {})
=> T
hylang> (nil? 0)
=> NIL
hylang> (nil? "")
=> NIL
```

## take, drop

Functions: the first items of a list; and the list without them.

#### Syntax

```text
(take n l)
(drop n l)
```

#### Arguments

*n*
: A count: a whole number, 0 or more.

*l*
: A list.

#### Result

`take` gives a list of the first *n* items, and `drop` the list without them. A count greater than the list's
length is not an error: `take` then gives the whole list, and `drop` gives `NIL`.

#### Errors

| Message | Cause |
|---|---|
| `'take': a count can't be negative` | *n* is less than 0 |
| `'take': a count must be a whole number, not 1.5` | *n* is not a whole number |
| `'take' expects a list` | *l* is not a list |

`drop`'s messages are the same, with its own name.

#### See also

`subset`, `head`, `tail`, `split`.

#### Example

```session
hylang> (take 2 {1 2 3})
=> {1 2}
hylang> (drop 2 {1 2 3})
=> {3}
hylang> (take 5 {1 2 3})
=> {1 2 3}
hylang> (drop 5 {1 2 3})
=> NIL
```

## ticks, tick-rate, clock

A function: programs, time and the system.

#### Syntax

```text
(ticks)
(tick-rate)
(clock)
```

#### Description

Ticks (0-32,767), 200 a second; seconds since hylang started.

## time, date, date-parts, seconds-of

A function: programs, time and the system.

#### Syntax

```text
(time)
(date [t])
(date-parts [t])
(seconds-of y m d [h mi s])
```

#### Description

The clock.

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

## to-str, repr

Functions: a value as text.

#### Syntax

```text
(to-str x [base])
(repr x)
```

#### Arguments

*x*
: Any value.

*base*
: For a number: the base to write it in, as a string (`"x"`, `"b"`, a radix). Without it, the current base.

#### Result

A string.

`to-str` gives the text that `print` would show: a string is itself, a character is the one character, and a
number is its digits.

`repr` gives the text as the prompt shows it, which is the text that reads back as the same value: a string
has its quotes, and its special characters are written as escapes; a character has its backslash.

For a number, a list, an atom, `T` or `NIL` the two are the same. A string inside a list is shown with its
quotes by both.

#### Errors

| Message | Cause |
|---|---|
| `Second 'to-str' parameter must be a String` | *base* is not a string: write `"x"`, not 16 |

#### See also

`val`, `read`, `format`, `hex`, `save`.

#### Example

```session
hylang> (to-str 42)
=> "42"
hylang> (to-str 255 "x")
=> "FF"
hylang> (to-str "a")
=> "a"
hylang> (repr "a")
=> "\"a\""
hylang> (to-str {1 "a"})
=> "{1 \"a\"}"
hylang> (len (to-str 1/3))
=> 3
```

## to-sym, to-atom

Functions: a symbol, or an atom, with a given name.

#### Syntax

```text
(to-sym x)
(to-atom x)
```

#### Arguments

*x*
: For `to-sym`, a string or an atom. For `to-atom`, a string, a symbol or a number.

#### Result

`to-sym` gives the symbol whose name is the string's text, or the atom's name without its colon. `to-atom`
gives the atom of that name.

#### Description

A symbol made this way is a value like any other until it is evaluated; `eval` then looks it up, as it would
a name written in the program. This is how a program reaches a function, or a variable, whose name it has
only as text.

#### Errors

| Message | Cause |
|---|---|
| `Only atoms and strings can be converted into symbols` | `to-sym` of anything else |
| `Only symbols, strings and numbers can be converted to atoms` | `to-atom` of anything else |

#### See also

`gensym`, `eval`, `def`.

#### Example

```session
hylang> (to-sym "abc")
=> abc
hylang> (to-atom "abc")
=> :abc
hylang> ((eval (to-sym "len")) "hello")
=> 5
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

## try

A special form: evaluates an expression, and catches an error.

#### Syntax

```text
(try x [handler])
```

#### Description

*x* is evaluated. If its value is not an error, that value is the result, and *handler* is not looked at.

If its value is an error, *handler* is evaluated and its value is the result; with no handler, the result is
`NIL`. While the handler is evaluated, `&err` is bound to the error's message, as a string, and `&code` to the
system's error code, a number, if the error came from the system, and to `NIL` if it did not.

#### Example

```session
hylang> (try (/ 1 0) (list "caught" &err &code))
=> {"caught" "Division by zero." NIL}
hylang> (try (+ 1 2) "not used")
=> 3
hylang> (try (error "mine"))
=> NIL
```

## type-of

A function: the kind of a value.

#### Syntax

```text
(type-of x)
```

#### Arguments

*x*
: Any value.

#### Result

An atom:

| Atom | The value is |
|---|---|
| `:number` | A number of any kind: an integer, a fraction, a decimal or a complex number |
| `:string` | A string |
| `:char` | A character |
| `:atom` | An atom |
| `:symbol` | A symbol that has not been evaluated |
| `:list` | A Q-expression: a list. `NIL`, the empty list, is one |
| `:sexpr` | An S-expression that has not been evaluated: a call, as data |
| `:t` | `T` |
| `:function` | A function: one made with `fn` or `fun`, a built-in one, or a special form |
| `:hash` | A hash |
| `:buffer` | A buffer |
| `:stream` | A stream |
| `:error` | An error |

#### Description

`type-of` is one of the few functions that can be given an error without the error becoming its result: it
answers `:error`. A symbol or an S-expression is only seen as one when it comes out of a list unevaluated, as
`(fst {a})` does; written as an argument, it is evaluated first, and `type-of` sees its value.

To tell the kinds of number apart, use `int?`, `fixed?`, `rational?` and `complex?`.

#### See also

`num?` and the other tests; `==`.

#### Example

```session
hylang> (type-of 42)
=> :number
hylang> (type-of "42")
=> :string
hylang> (type-of {4 2})
=> :list
hylang> (type-of NIL)
=> :list
hylang> (type-of len)
=> :function
hylang> (type-of (fst {a}))
=> :symbol
hylang> (type-of (error "x"))
=> :error
```

## use

A function of the library, written in hylang.

#### Syntax

```text
(use "name")
```

#### Description

Load a library once.

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

## while

A special form: repeats a body while a test holds.

#### Syntax

```text
(while test body ...)
```

#### Description

*test* is evaluated. If its value is `NIL`, the loop ends. Otherwise the body's expressions are evaluated in
order, and the test is evaluated again. The test is made before each pass, the first included, so the body may
not run at all.

The result is `NIL`. The body runs in the scope the `while` is in, which is how it changes the names its test
looks at.

#### Example

```session
hylang> (let {{i 0}} (while (< i 3) (set {i} (+ i 1))) i)
=> 3
```

## write

A function: errors, output and input.

#### Syntax

```text
(write x ...)
```

#### Description

Print them, without the new line.

## write-file, append-file

A function: files and streams.

#### Syntax

```text
(write-file p x ...)
(append-file p x ...)
```

#### Description

Write a file; add to it.

## xor, <>, !=

A function of the library, written in hylang.

#### Syntax

```text
(xor a b)
(<> a b)
!=
```

#### Description

Logic; not equal.

