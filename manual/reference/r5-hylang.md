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
including 0 and the empty string. `T` is the usual value for true. The two may be written in small letters
too, `t` and `nil`, and for that reason neither can be the name of a variable or of a function's formal:
`(fn {t} ...)` is an error.

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

A function of the library, written in hylang: the first of its arguments that is not `NIL`.

#### Syntax

```text
(?? a ...)
```

#### Arguments

*a*
: Any values: at least one.

#### Result

The first argument that is not `NIL`. If all are `NIL`, `NIL`.

#### Description

`??` supplies a value where one may be missing: `(?? (env "editor") "edit")` is the variable's value if
there is one, and `"edit"` if not. Only `NIL` (and the empty list) counts as missing: 0 and the empty string
are values, and are returned.

It is a function, so all its arguments are evaluated before it chooses. Where a later argument must not be
evaluated unless it is needed, use `or`, which stops at the first true value.

#### See also

`or`, `if`, `nil?`.

#### Example

```session
hylang> (?? NIL NIL 3 4)
=> 3
hylang> (?? 0 1)
=> 0
hylang> (?? (env "nothere") "a default")
=> "a default"
```

## =0, =1, >0, <0, >=0, <=0, !=0, !=1

Functions of the library: comparisons with 0 and with 1, each of one argument.

#### Syntax

```text
(=0 n)   (=1 n)   (!=0 n)   (!=1 n)
(>0 n)   (<0 n)   (>=0 n)   (<=0 n)
```

#### Arguments

*n*
: A number.

#### Result

`T` or `NIL`: is *n* equal to 0, equal to 1, not 0, not 1, above 0, below 0, 0 or more, 0 or less?

#### Description

Each is the comparison its name spells, with the 0 or 1 already given. They save writing a function where
one is wanted as an argument: `(filter >0 l)` keeps the positive numbers of a list. `=0` and `=1` are other
names for `zero?` and `one?`.

They are plain comparisons, and give no error for a value that is not a number: such a value is ordered by
its kind, after every number, so `(>0 "a")` is `T`.

#### See also

`zero?`, `one?`, `pos?`, `neg?`, `<`.

#### Example

```session
hylang> (=0 0)
=> T
hylang> (>0 -5)
=> NIL
hylang> (filter >0 {3 -1 0 7})
=> {3 7}
hylang> (count =0 {0 1 0})
=> 2
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

Functions: the task's RAM banks: how many it has, taking some, giving them back, and reading and writing
them.

#### Syntax

```text
(banks)
(bank-alloc n)
(bank-free bank n)
(bank-read bank offset n)
(bank-write bank offset bytes)
```

#### Arguments

*n*
: For `bank-alloc` and `bank-free`, a number of banks, 1 or more. For `bank-read`, a number of bytes.

*bank*
: A bank's number: the one `bank-alloc` gave, or one of those after it in the same run.

*offset*
: A position in the bank, 0 to 8,191.

*bytes*
: A list of whole numbers from 0 to 255.

#### Result

`banks` gives the number of RAM banks the task has in all: 16 for each RAM module fitted, each bank 8K.

`bank-alloc` takes *n* banks that are next to one another and gives the number of the first: the others are
that number plus 1, plus 2 and so on. The banks are the task's until it gives them back or ends.

`bank-free` gives *n* banks back, from *bank* on, and gives `NIL`.

`bank-read` gives a list of *n* bytes from the bank, starting at *offset*. `bank-write` stores the bytes in
the bank from *offset* on, and gives `NIL`. The bytes read or written must all be inside the one bank:
*offset* plus the count can't pass 8,192.

#### Description

A task has far more RAM in its banks than the 64K it can see at once (384K, with three modules). These
functions let a hylang program keep data there, outside hylang's own memory: a large table, a picture, a
file's contents. hylang reaches the bank only for the moment of the read or the write, so nothing a program
keeps in a bank is disturbed by hylang, or disturbs it.

hylang keeps some of the task's banks for its own values. `bank-alloc` gives only banks that are free.

#### Errors

| Message | Cause |
|---|---|
| `'bank-read' was given a number out of its range` | The offset is past the bank's end, or the bytes would run past it |
| `'bank-write' expects a list of bytes (0-255)` | *bytes* is not such a list: a string or a buffer must be made a list first, with `bytes` |
| *n*`: invalid argument` | `bank-alloc` of no banks; `bank-free` of banks that aren't taken |
| `'bank-alloc' was given a number out of its range` | More banks than a task can have |

#### See also

`free`, `buffer`, `seg-create`, `peek`.

#### Example

The bank's number depends on what hylang has taken already:

```text
hylang> (banks)
=> 48
hylang> (def {bk} (bank-alloc 2))
=> NIL
hylang> bk
=> 7
hylang> (bank-write bk 0 {1 2 3})
=> NIL
hylang> (bank-read bk 0 4)
=> {1 2 3 0}
hylang> (bank-free bk 2)
=> NIL
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

## between?, ><?, not-between?

Functions of the library, written in hylang: is a value in a range?

#### Syntax

```text
(between? lo hi x)
(><? lo hi x)
(not-between? lo hi x)
```

#### Arguments

*lo*, *hi*
: The two ends of the range. They may be given in either order.

*x*
: The value to test.

#### Result

`between?` gives `T` if *x* is at one of the ends or anywhere between them, and `NIL` if it is outside.
`><?` is another name for it. `not-between?` gives the opposite, and has another name too, `!><?`.

#### Description

The comparison is `<=`'s, so it works on anything that has an order: numbers, strings, characters.

Three more go through a list: `(all-between? lo hi l)` is `T` when every item of the list *l* is in the
range, `(all-not-between? lo hi l)` when none is, and `(not-all-between? lo hi l)` when at least one is
outside.

#### See also

`<`, `and`, `min`, `max`.

#### Example

```session
hylang> (between? 1 10 5)
=> T
hylang> (between? 1 10 10)
=> T
hylang> (between? 10 1 5)
=> T
hylang> (between? 1 10 11)
=> NIL
hylang> (between? "a" "m" "c")
=> T
hylang> (all-between? 0 9 {3 5 7})
=> T
```

## bind, mount, unmount, ns

Functions: change, and list, the task's namespace.

#### Syntax

```text
(bind new old [:before | :after] [:create])
(mount dev old [spec] [:before | :after] [:create])
(unmount old [new])
(ns)
```

#### Arguments

*new*
: The directory, or file, to make appear at *old*.

*old*
: The place in the tree that is changed. It must be there already.

*dev*
: A device's name, such as `"#f"`, with *spec* choosing which of the device's trees: `"x"` is the ROM disk.

`:before`, `:after`
: Make a union: *old* shows what it showed before and *new*'s contents too, with *new* looked in first
  (`:before`) or last (`:after`). With neither, *new* replaces what *old* showed.

`:create`
: In a union, a file created at *old* is made in this member.

#### Result

`bind`, `mount` and `unmount` give `NIL`.

`unmount` undoes what was bound or mounted at *old*: all of it, or with *new*, only that member of a union.

`ns` gives the namespace as a list of hashes, one for each bind and mount, in the kernel's order: `:old`, the
place; `:new`, what is there, as a device's path such as `"#fx/lib"`; and `:create`, `T`, on a member that
takes creates.

#### Description

These are the system's `BIND`, `MOUNT` and `UNMOUNT` calls, and work as rc's `bind`, `mount` and `unmount`
commands do (Chapter 9 of the manual explains namespaces, and Chapter 2 of this book has the commands'
entries).

The namespace is this task's. Programs that hylang starts afterward see the same tree; the shell that
started hylang does not see the changes, and they are gone when hylang ends. `(newns)` puts the default
namespace back.

In a union with no member that takes creates, a new file is made in the directory that was there first.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | *new* or *old* isn't there; or `unmount` of a place where nothing is bound |
| `'bind' expects :before, :after or :create` | Another atom was given |
| *dev*`: no such device` | `mount` of a device that isn't there |

#### See also

`newns`, `ls`, `cd`; the commands `bind`, `mount`, `unmount` and `ns` (Chapter 2 of this book).

#### Example

```session
hylang> (mkdir "/ram/a")
=> NIL
hylang> (mkdir "/ram/b")
=> NIL
hylang> (write-file "/ram/a/one" "1")
=> NIL
hylang> (write-file "/ram/b/two" "2")
=> NIL
hylang> (bind "/ram/a" "/ram/b" :after)
=> NIL
hylang> (ls "/ram/b")
=> {"one" "two"}
hylang> (unmount "/ram/b")
=> NIL
hylang> (ls "/ram/b")
=> {"two"}
hylang> (bind "/ram/a" "/ram/b")
=> NIL
hylang> (ls "/ram/b")
=> {"one"}
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

## buffer

A function: makes a buffer, a block of bytes that can be changed in place.

#### Syntax

```text
(buffer n [fill])
(buffer s)
(buffer l)
(buffer b)
```

#### Arguments

*n*
: The buffer's size in bytes: a whole number from 0 to 8,184.

*fill*
: The value every byte starts with: 0 to 255, or a character. Without it, 0.

*s*
: A string: the buffer holds its bytes.

*l*
: A list of bytes, each a whole number from 0 to 255 or a character.

*b*
: Another buffer: the result is a copy of it.

#### Result

A buffer. It prints as `<buffer>` and its bytes.

#### Description

A **buffer** is a row of bytes of a fixed length. Unlike a string or a list, it is changed where it is:
`buffer-put`, `buffer-fill`, `buffer-copy` and `read-buffer` alter the buffer they are given, and every
name bound to that buffer sees the change. That makes it the value to use for binary data, for a file read a
block at a time, and for a table that is updated many times, where making a new list for each change would
be slow.

A buffer has a length (`len`), is equal to another buffer with the same bytes (`==`), and can be called with
an index, `(b i)`, to read a byte. `bytes` gives its bytes as a list and `from-bytes` as a string. It is not
a list: `map`, `nth` and the other list functions do not take one.

#### Errors

| Message | Cause |
|---|---|
| `'buffer': -1 bytes is outside 0 to 8184` | *n* is negative, or more than a buffer can hold |
| `'buffer': a size must be a whole number, not 1.5` | The argument is not a size, a string, a list or a buffer |
| `'buffer': 256 isn't a byte (0-255)` | *fill*, or an item of the list, is not a byte |

#### See also

`buffer-get`, `buffer-fill`, `read-buffer`, `write-bytes`, `bytes`, `from-bytes`.

#### Example

```session
hylang> (buffer 4)
=> <buffer>{0 0 0 0}
hylang> (buffer 4 255)
=> <buffer>{255 255 255 255}
hylang> (buffer "Hi")
=> <buffer>{72 105}
hylang> (buffer {1 2 3})
=> <buffer>{1 2 3}
hylang> (len (buffer 100))
=> 100
hylang> (from-bytes (buffer {72 105}))
=> "Hi"
```

## buffer-cmp

A function of the library, written in hylang: compares part of a buffer with a string, or with another
buffer.

#### Syntax

```text
(buffer-cmp b at x [i [n]])
```

#### Arguments

*b*
: A buffer.

*at*
: Where in *b* the comparison starts.

*x*
: What to compare with: a string, or a buffer.

*i*
: Where in *x* to start. Without it, 0.

*n*
: How many bytes of *x* to compare. Without it, all of them from *i* on.

#### Result

-1, 0 or 1, as `cmp` gives for two strings: is the part of *b* before, the same as, or after the part of
*x*?

#### Description

At most as many bytes of *b* are looked at as *x* has. So the result is 0 when *b*, from *at*, *begins*
with *x*: this is the test for "is this text here in the buffer?", such as a file's signature at its start.
If *b* ends before *x* does, and they agree as far as *b* goes, the result is -1.

#### Errors

| Message | Cause |
|---|---|
| `'buffer-cmp' expects a buffer, not 5` | *b* is not a buffer |
| `'buffer-cmp' expects a buffer or a string, not 5` | *x* is neither |
| `'buffer-cmp': index 5 is outside the buffer` | *at* or *i* is negative, or past the end |

#### See also

`cmp`, `==`, `index-of`.

#### Example

```session
hylang> (def {b} (buffer "HYX2 and more"))
=> NIL
hylang> (buffer-cmp b 0 "HYX2")
=> 0
hylang> (buffer-cmp b 0 "HYX3")
=> -1
hylang> (buffer-cmp b 5 "and")
=> 0
```

## buffer-fill, buffer-copy

Functions: set many bytes of a buffer to one value; and copy bytes from one buffer to another.

#### Syntax

```text
(buffer-fill b byte [i [n]])
(buffer-copy to at from [i [n]])
```

#### Arguments

*b*
: The buffer to fill.

*byte*
: The value to fill with: 0 to 255, or a character.

*to*, *at*
: The buffer to copy into, and the position in it where the first byte goes.

*from*
: The buffer to copy from.

*i*
: The position of the first byte to fill, or to copy from *from*. Without it, 0.

*n*
: How many bytes. Without it, all the bytes from *i* to the end.

#### Result

`NIL`. The buffer *b*, or *to*, is changed in place.

#### Description

A count that goes past the end of a buffer is not an error: the fill stops at the end of *b*, and the copy
stops when *from* runs out or *to* is full, whichever comes first.

*to* and *from* may be the same buffer, and the two parts may overlap: the bytes are copied as if they were
all read first.

Both arguments of `buffer-copy` must be buffers. To put a string's or a list's bytes in a buffer, make a
buffer of them first: `(buffer-copy b 0 (buffer "xy"))`.

#### Errors

| Message | Cause |
|---|---|
| `'buffer-fill': index 5 is outside the buffer` | *i*, or *at*, is negative or past the end |
| `'buffer-fill': 300 isn't a byte (0-255)` | *byte* is not a byte |
| `'buffer-copy' expects a buffer, not xy` | *to* or *from* is not a buffer |

#### See also

`buffer`, `buffer-put`, `read-buffer`.

#### Example

```session
hylang> (def {b} (buffer 6))
=> NIL
hylang> (buffer-fill b 7)
=> NIL
hylang> (buffer-fill b 1 4)
=> NIL
hylang> b
=> <buffer>{7 7 7 7 1 1}
hylang> (buffer-copy b 1 (buffer "abcdef") 2 3)
=> NIL
hylang> b
=> <buffer>{7 99 100 101 1 1}
```

## buffer-get, buffer-put

Functions: read one byte of a buffer; and set one.

#### Syntax

```text
(buffer-get b i)
(b i)
(buffer-put b i byte)
```

#### Arguments

*b*
: A buffer.

*i*
: The byte's position, from 0 to one less than the buffer's length.

*byte*
: The new value: a whole number from 0 to 255, or a character, whose code is stored.

#### Result

`buffer-get` gives the byte at *i*, a number from 0 to 255. Calling the buffer itself, `(b i)`, does the
same.

`buffer-put` stores *byte* at *i*, in the buffer itself, and gives the byte that was there before.

#### Errors

| Message | Cause |
|---|---|
| `'buffer-get': index 4 is outside the buffer` | *i* is negative, or the buffer's length or more (and `'buffer-put'`) |
| `'buffer-get': an index must be a whole number, not 1.5` | *i* is not a whole number |
| `'buffer-put': 256 isn't a byte (0-255)` | *byte* is not a byte |
| `'buffer-get' expects a buffer, not {1}` | *b* is not a buffer |

#### See also

`buffer`, `buffer-fill`, `peek`, `poke`.

#### Example

```session
hylang> (def {b} (buffer 4))
=> NIL
hylang> (buffer-put b 0 65)
=> 0
hylang> (buffer-put b 0 66)
=> 65
hylang> (b 0)
=> 66
hylang> (buffer-get b 1)
=> 0
hylang> (buffer-get b 4)
=> Error: 'buffer-get': index 4 is outside the buffer
```

## buffer?

A function: is a value a buffer?

#### Syntax

```text
(buffer? x)
```

#### Arguments

*x*
: Any value.

#### Result

`T` if *x* is a buffer, and `NIL` if it is anything else: a list of bytes is not a buffer, and nor is a
string.

#### See also

`buffer`, `type-of`.

#### Example

```session
hylang> (buffer? (buffer 4))
=> T
hylang> (buffer? {1 2})
=> NIL
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

## cd, cwd

Functions: set, and ask, the current directory.

#### Syntax

```text
(cd [path])
(cwd)
```

#### Arguments

*path*
: A directory. Without it, `/`.

#### Result

`cd` gives `NIL`, and makes *path* the current directory: the one that names not starting with `/` are
looked for in. `cwd` gives the current directory's full path, as a string.

#### Description

The current directory is this task's. Programs that hylang starts afterward begin in it; the shell that
started hylang is not affected.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such directory |
| `'cd' expects a path` | *path* is not a string |

#### Example

```session
hylang> (cd "/rom/lib")
=> NIL
hylang> (cwd)
=> "/rom/lib"
hylang> (exists? "shell")
=> T
hylang> (cd)
=> NIL
hylang> (cwd)
=> "/"
```

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

## clock library

A library, loaded with `(use "clock")`: setting the clock, and the clock chip.

#### Functions

| Form | What it does |
|---|---|
| `(set-date text)` | Sets the date and time, from a string in the form `"2026-10-13 09:41:00"`. With a clock chip fitted, the chip is set too. `NIL` |
| `(rtc)` | What the system found for a clock chip as it started: a list holding `:running`, `:stopped` (the chip is there and needs setting) or `:none`, and `:battery-low` as well if its battery is running down |

#### Description

Reading the clock needs no library: `time`, `date` and `date-parts` are always there. This one writes
`/dev/time` and reads `/dev/rtc` (section 17.5).

#### Errors

| Message | Cause |
|---|---|
| `invalid argument` | The text is not a date and time in that form |

#### See also

`time`, `date`, `seconds-of`; section 17.5.

#### Example

```session
hylang> (use "clock")
=> NIL
hylang> (rtc)
=> {:none}
hylang> (set-date "2026-10-13 09:41:00")
=> NIL
hylang> (substring (date) 0 16)
=> "2026-10-13 09:41"
hylang> (set-date "nonsense")
=> Error: invalid argument
```

## close

A function: closes a stream.

#### Syntax

```text
(close s)
```

#### Arguments

*s*
: A stream.

#### Result

`NIL`. The file is closed, and what was written to it is safely in it.

#### Description

Closing a stream that is already closed does nothing, and is not an error. Reading or writing a closed
stream is one: `Cannot read from this stream`, `Cannot write this stream`.

#### Errors

| Message | Cause |
|---|---|
| `'close' expects a stream` | *s* is not a stream |

#### See also

`open`.

#### Example

```session
hylang> (def {s} (open "/rom/lib/shell"))
=> NIL
hylang> (close s)
=> NIL
hylang> (read-line s)
=> Error: Cannot read from this stream
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

## cond, case

Functions of the library, written in hylang: choose one of several results.

#### Syntax

```text
(cond {test value} ...)
(case x {match result} ...)
```

#### Arguments

*{test value}*
: A Q-expression of two expressions. Neither is evaluated until `cond` reaches it.

*x*
: A value to look for.

*{match result}*
: A Q-expression of two expressions.

#### Result

`cond` evaluates each *test* in turn, and at the first that is true it evaluates that pair's *value* and
gives it. No later pair is looked at. A last pair with the test `T` is the "otherwise".

`case` evaluates each *match* in turn, and at the first that equals *x*, as `==` compares, it evaluates that
pair's *result* and gives it.

#### Description

Both are a chain of `if`s written flat. For a choice on the sign of a number, the special form `<=>` is
shorter.

Neither has a silent way out: if no test is true, or nothing matches, the result is an error. Give `cond` a
last `{T ...}`. `case` has no "anything else" pair, so check the value first, or catch the error with `try`.

#### Errors

| Message | Cause |
|---|---|
| `Selection not found` | `cond`: no test was true |
| `Case not found` | `case`: nothing equalled *x* |

#### See also

`if`, `<=>`, `and`, `or`, `try`.

#### Example

```session
hylang> (def {n} 5)
=> NIL
hylang> (cond {(< n 0) "negative"} {(== n 0) "zero"} {T "positive"})
=> "positive"
hylang> (case n {1 "one"} {5 "five"} {9 "nine"})
=> "five"
hylang> (case n {1 "one"})
=> Error: Case not found
hylang> (try (case n {1 "one"}) "many")
=> "many"
```

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

## cons library

A library, loaded with `(use "cons")`: the console's windows.

#### Functions

| Form | What it does |
|---|---|
| `(window)` | The number of the window hylang is running in |
| `(window-size)` | That window's size: a list, the columns and the rows |
| `(windows)` | The numbers of all the windows there are: a list |
| `(shown-window)` | The number of the window that is shown now |
| `(show-window n)` | Shows window *n*. `NIL` |
| `(new-window [line])` | Makes a window and shows it, running the command line in it; with none, a shell. It is the `new-window` command, and gives its exit code |
| `(new-group [line])` | The same, in a note group of its own (`new-window -g`) |
| `(window-label [s])` | With a string, sets this window's title (`""` gives it its program's name again), `NIL`. With none, gives the title |
| `(window-status s)` | Sets the text of this window's status line. `NIL` |
| `(window-ctl s)` | Writes any line to the window's control file, `/dev/wctl`. `NIL` |
| `(snarf)`, `(snarf! s)` | The console's cut buffer: its text; and set it |
| `(raw-on)`, `(raw-off)` | Turns raw keys on: each key comes as it is pressed, and is not shown. And off again |
| `(beep)` | Rings the bell |

#### Description

The library works through the console's files in `/dev` (Chapters 10 and 13). The cut buffer is shared by
all the windows: what one program puts there, <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>Y</kbd> pastes into another.

`raw-on` lasts until `raw-off`: while it is on, the prompt's own line editing is off too, so a program must
turn it off before it ends. For single keys, `key` is simpler, and looks after this itself.

#### See also

`key`, the `screen` library; the command `new-window` (Chapter 2 of this book); Chapter 10.

#### Example

```session
hylang> (use "cons")
=> NIL
hylang> (window)
=> 0
hylang> (window-size)
=> {80 24}
hylang> (windows)
=> {0}
hylang> (snarf! "cut text")
=> NIL
hylang> (snarf)
=> "cut text"
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

## dev library

A library, loaded with `(use "dev")`: the few functions that the device libraries are built from.

#### Functions

| Form | What it does |
|---|---|
| `(dev.read path [n])` | One read of the file: a string of at most *n* bytes, or 256 |
| `(dev.lines path)` | One read, as a list of its lines, without the empty ones |
| `(dev.num path)` | One read, as a number: the text is trimmed and given to `val` |
| `(dev.write path data)` | One write to the file, of a string or a list of bytes. `NIL` |
| `(dev.with path mode f)` | Opens the file (*mode* 0 to read, 1 to write, 2 for both), calls `(f fd)` with its file descriptor, closes the file, and gives *f*'s result |
| `(dev.bytes s)` | A string's bytes, as a list |
| `(dev.hex n w)` | A number in hex, *w* digits, in small letters: the way devices are named in their directories |

#### Description

A device's file is not like a disk's: each read is one request to the driver (a pin's level, a
transaction's bytes, a control file's text), and each write is one command. `read-file` may make several
reads and `write-file` several writes; these make exactly one, which is what a device wants. `dev.with` is
for anything longer, such as a seek and then a read, with the `sys-` functions.

Whatever goes wrong, the file is closed before the error is passed on.

These are the pieces to use for a device that has no library of its own, and the libraries themselves, in
`/lib/hylang`, are short examples of their use.

#### See also

`sys`, `read-file`, `write-file`; Chapter 12.

#### Example

```session
hylang> (use "dev")
=> NIL
hylang> (dev.read "/dev/rtc")
=> "none\n"
hylang> (dev.lines "/dev/serctl")
=> {"b9600" "x1" "pc 0 req, 0 cut short, 0 unanswered, 0 damaged, 0 nak, 0 gone" "line 0 overruns, 0 framing errors"}
hylang> (dev.write "/dev/gpio/ctl" "out 2")
=> NIL
hylang> (dev.num "/dev/gpio/2")
=> 1
hylang> (dev.write "/dev/gpio/ctl" "in 2")
=> NIL
```

## dice library

A library, loaded with `(use "dice")`: dice for games.

#### Functions

| Form | What it does |
|---|---|
| `(die sides)` | One throw of a die with that many sides: 1 to *sides* |
| `(die+ sides mod)` | A throw, with *mod* added |
| `(d3)`, `(d4)`, `(d6)`, `(d8)`, `(d10)`, `(d12)`, `(d20)`, `(d24)`, `(d30)`, `(d60)`, `(d100)` | A throw of the die named |
| `(d6+ mod)`, and the same for each of those | A throw of it, with *mod* added |
| `(dice n sides)`, `(dice+ n sides mod)` | The total of *n* throws; and with *mod* added |
| `(roll n d)` | A list of *n* throws: *d* is a Q-expression, run for each, as `repeat`'s is. `(roll 4 {d6})` |
| `(sumrolls l)` | Throws every die that the list describes, and gives the total. An item is a number of sides, or a list: `{sides}`, `{n sides}` or `{n sides mod}` |
| `(best-of-rolls a b d)`, `(worst-of-rolls a b d)` | The best, or the worst, *a* of *b* throws of *d*: a list |
| `(oe-die sides)`, `(oe-dice n sides)`, and each with `+` and a *mod* | Open-ended throws: a throw of the highest face is thrown again and added |
| `(special-die sides cases)` | A throw looked up with `case`: *cases* is a list of `{face result}` pairs |
| `(dnd-attr-rolls [n])` | Scores for a role-playing character: a list of *n* sets (one, if *n* isn't given), each set the best six of eight scores, greatest first, and each score the total of the best three of four six-sided dice |

#### Description

Every throw comes from `random`, so no two runs give the same numbers.

#### See also

`random`, `repeat`, `best-of`, `sum`.

#### Example

The numbers differ from run to run:

```text
hylang> (use "dice")
=> NIL
hylang> (d6)
=> 4
hylang> (dice 3 6)
=> 11
hylang> (roll 4 {d6})
=> {2 6 3 5}
hylang> (sum (best-of-rolls 3 4 {d6}))
=> 14
hylang> (sumrolls {6 {2 8} {1 4 3}})
=> 17
hylang> (dnd-attr-rolls)
=> {{18 16 16 14 13 12}}
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

## disk library

A library, loaded with `(use "disk")`: the disks.

#### Functions

| Form | What it does |
|---|---|
| `(disks)` | The disks that have been started: a list of hashes, each with `:disk`, its letter, a string; `:kind`, an atom such as `:sdhc`, `:rom`, `:ram` or `:sram`; `:blocks`, its size in 512-byte blocks; and `:label`, if it holds a file system |
| `(df)` | Each disk's room: a list of hashes with `:disk`, `:label`, and `:free` and `:size` in kilobytes |
| `(cards)` | The SPI devices that have an SD card: a list of numbers. Asking starts each card that is there |
| `(disk-start d)` | Starts a card again, as is needed after one is changed. `NIL` |
| `(disk-stop d)` | Stops a RAM disk, and gives its memory back. `NIL` |

#### Arguments

*d*
: A disk: its letter, as a string (`"0"` to `"f"` for the card on that SPI device, `"x"` the ROM disk, `"r"` the
  RAM disk, `"s"` the shared RAM disk); or, for a card, its SPI device's number.

#### Description

The library works through the files of `/dev/sd` (Chapter 14). `df` gives a program the numbers that the `df`
command prints.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such disk |

#### See also

Chapter 14; the commands `df`, `fsck`, `mkfs` and `label` (Chapter 2 of this book).

#### Example

```session
hylang> (use "disk")
=> NIL
hylang> (map (fn {d} {d :disk}) (disks))
=> {"0" "r" "s" "x"}
hylang> (map (fn {d} {list (d :label) (d :size)}) (df))
=> {{"HYDRA-16" 16380} {"RAM" 255} {"SRAM" 511} {"ROM" 623}}
hylang> (cards)
=> {0}
```

## div, mod, divmod, fdiv

Functions of the library, written in hylang: division with a whole quotient and a remainder; and division
to a decimal.

#### Syntax

```text
(div a b)
(mod a b)
(divmod a b)
(fdiv a b [places])
```

#### Arguments

*a*, *b*
: Real numbers.

*places*
: For `fdiv`: how many digits to keep after the point. Without it, 10.

#### Result

`div` gives the quotient rounded down to a whole number: the `floor` of *a* / *b*. `mod` gives what is left
over: *a* - *b* × `(div a b)`. `divmod` gives both, as a list of two.

`fdiv` gives *a* / *b* as a decimal of at most *places* places, cut off, not rounded: it is `to-fixed` of
the quotient.

`%` is another name for `mod`.

#### Description

Because the quotient is rounded *down*, not toward zero, the remainder always has the sign of *b*:
`(mod -7 2)` is 1, and `(mod 7 -2)` is -1. With a positive *b*, `mod` is therefore always from 0 up to, but
not including, *b*, which is what is wanted for wrapping a position round a table or an angle round a
circle.

The numbers need not be whole: `(mod 7.5 2)` is 1.5.

#### Errors

| Message | Cause |
|---|---|
| `Cannot perform 'div' on non-numbers` | An argument is not a number (each function with its own name) |
| `Division by zero.` | *b* is 0 |

#### See also

`/`, `floor`, `to-fixed`, `odd?`.

#### Example

```session
hylang> (div 7 2)
=> 3
hylang> (mod 7 2)
=> 1
hylang> (div -7 2)
=> -4
hylang> (mod -7 2)
=> 1
hylang> (divmod 7 2)
=> {3 1}
hylang> (fdiv 1 3 4)
=> 0.3333
```

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

Functions: read, set and remove an environment variable.

#### Syntax

```text
(env [name])
(setenv name value)
(unsetenv name)
$name
```

#### Arguments

*name*
: The variable's name, a string.

*value*
: A string; a list of strings, for a variable of several words; or a number, which is turned to text.

#### Result

`(env name)` gives the variable's value: a string, or a list of strings if it holds several words, as rc's
`$path` does. If there is no such variable, `NIL`. `$name` is another way to write `(env "name")`.

`(env)`, with no name, gives a hash of all the variables, with their names, as strings, for keys.

`setenv` and `unsetenv` give `NIL`. Removing a variable that isn't there is not an error.

#### Description

The environment is this task's own set of variables (section 17.7). hylang starts with a copy of the
environment of the shell that started it, and the programs it starts get a copy of its own: so `setenv` is
how a setting is passed down to a program run with `run` or `sh`. Nothing set here goes back up to the
shell.

Environment values are text. A number put in comes back as a string: use `val` to make it a number again.

#### Errors

| Message | Cause |
|---|---|
| `'setenv' expects a path` | *name* is not a string |

#### See also

`sh`, `run`, `val`.

#### Example

```session
hylang> (env "path")
=> {"." "/bin"}
hylang> (setenv "colour" "red")
=> NIL
hylang> $colour
=> "red"
hylang> (sh "echo my colour is $colour")
my colour is red
=> 0
hylang> (unsetenv "colour")
=> NIL
hylang> (env "colour")
=> NIL
```

## error

A function: makes an error.

#### Syntax

```text
(error message [code])
```

#### Arguments

*message*
: The error's message: a string, or any value, which is turned to text as `print` shows it.

*code*
: An atom that names the kind of error, for the program that catches it. Without it the error has no code.

#### Result

An error value. It prints as `Error: ` and the message.

#### Description

An error is a value, but one that most functions will not accept: a function given an error as an argument
does not run, and the error is its result. So the error passes outward through every enclosing call until it
is caught, or reaches the prompt, which prints it. `error` is how a program reports that it cannot go on;
`try` is how another part of the program catches the report.

The few forms that do accept an error are `try`; `def`, `set` and `set!`, so that an error can be kept in a
variable; `error?`, `type-of`, `error-message` and `error-code`; and `print`.

The errors that the system's calls give have codes too. Each is the name of the system's error code (Chapter
3 of this book lists them) in small letters, without its `E_`: `:noent` for `E_NOENT`, a file that is not found.

#### Errors

| Message | Cause |
|---|---|
| `An error's code must be an atom` | *code* is not an atom |

#### See also

`try`, `error-message`, `error?`.

#### Example

```session
hylang> (error "out of cheese")
=> Error: out of cheese
hylang> (+ 1 (error "inside"))
=> Error: inside
hylang> (fun {half n} {if (odd? n) (error (format "{} is odd" n) :odd) (/ n 2)})
=> NIL
hylang> (half 10)
=> 5
hylang> (half 7)
=> Error: 7 is odd
hylang> (try (half 7) (list &err &code))
=> {"7 is odd" :odd}
```

## error-message, error-code

Functions: the message and the code of an error.

#### Syntax

```text
(error-message e)
(error-code e)
```

#### Arguments

*e*
: An error, usually one kept in a variable, or the value of an expression that may fail.

#### Result

`error-message` gives the message, as a string, without the `Error: ` that is printed before it.
`error-code` gives the code, an atom, or `NIL` for an error that has none. For a value that is not an error,
both give `NIL`.

#### Description

These two are given errors without passing them on, so `(error-code (open path))` is a way to ask why a
call failed without `try`. For an error from the system the code is the system's name for it: `:noent` (not
found), `:exist` (already exists), `:isdir` (is a directory), `:rofs` (read-only), and so on.

#### See also

`error`, `try`, `error?`, `errstr`.

#### Example

```session
hylang> (def {e} (error "out of cheese" :cheese))
=> NIL
hylang> (error-message e)
=> "out of cheese"
hylang> (error-code e)
=> :cheese
hylang> (error-code (open "/nothere"))
=> :noent
hylang> (error-message (open "/nothere"))
=> "/nothere: not found"
hylang> (error-code 5)
=> NIL
```

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

Functions: is a path there, and is it a directory or a file?

#### Syntax

```text
(exists? path)
(dir? path)
(file? path)
```

#### Arguments

*path*
: A string.

#### Result

`T` or `NIL`. `exists?` is `T` if the path names anything; `dir?` if it names a directory; `file?` if it
names something that is not a directory. For a path that names nothing, all three are `NIL`: none of them
gives an error for a missing file.

#### See also

`stat`, `ls`, `open`.

#### Example

```session
hylang> (exists? "/rom/lib/shell")
=> T
hylang> (file? "/rom/lib/shell")
=> T
hylang> (dir? "/rom/lib")
=> T
hylang> (exists? "/rom/nothere")
=> NIL
```

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

## first, second, third, fourth, fifth, sixth, seventh, eighth, ninth, tenth

Functions of the library: an item of a list, by its place.

#### Syntax

```text
(first l)
(second l)
...
(tenth l)
```

#### Arguments

*l*
: A list long enough to have the item.

#### Result

The item: `first` is `(nth 0 l)`, `second` is `(nth 1 l)`, and so on to `tenth`, which is `(nth 9 l)`. The
item is given as it is in the list, not evaluated.

`first`, `second` and `third` are the same as `fst`, `snd` and `thd`.

#### Errors

| Message | Cause |
|---|---|
| `'nth': the list has no such item` | The list is too short |

#### See also

`nth`, `fst`, `last`.

#### Example

```session
hylang> (first {1 2 3})
=> 1
hylang> (third {1 2 3})
=> 3
hylang> (fourth {1 2 3})
=> Error: 'nth': the list has no such item
```

## flip, comp, unpack, apply, curry, pack, uncurry

Functions of the library, written in hylang: functions that work on functions.

#### Syntax

```text
(flip f a b)
(comp f g x)
(unpack f l)
(pack f x ...)
```

#### Arguments

*f*, *g*
: Functions.

*a*, *b*, *x*
: Values.

*l*
: A list of arguments.

#### Result

| Function | Result |
|---|---|
| `flip` | `(f b a)`: *f* called with its two arguments the other way round |
| `comp` | `(f (g x))`: *g*, and then *f* of its result |
| `unpack` | *f* called with the items of *l* as its arguments: `(unpack + {1 2 3})` is `(+ 1 2 3)`. `apply` and `curry` are other names for it, and `.(f l)` is another way to write the call |
| `pack` | *f* called with one argument, the list of all the values that follow it: `(pack len 1 2 3)` is `(len {1 2 3})`. `uncurry` is another name for it |

#### Description

Each is most useful with fewer arguments than it takes, because the result is then a new function (see
`fn`): `(comp 1+ square)` is a function that squares a number and adds one, and `(flip - 1)` is one that
subtracts 1 from its argument.

`unpack` is how a function that takes separate arguments is given a list that a program has built.

#### Errors

| Message | Cause |
|---|---|
| `'cons' expects a list second` | `unpack`'s second argument is not a list |

#### See also

`fn`, `map`, `eval`.

#### Example

```session
hylang> (flip - 1 10)
=> 9
hylang> (comp 1+ square 3)
=> 10
hylang> (def {inc-sq} (comp 1+ square))
=> NIL
hylang> (map inc-sq {1 2 3})
=> {2 5 10}
hylang> (unpack + {1 2 3})
=> 6
hylang> (apply max {3 9 2})
=> 9
hylang> (pack len 1 2 3)
=> 3
```

## floor, ceil, round, trunc

Functions of the library, written in hylang: a number made whole.

#### Syntax

```text
(floor a)
(ceil a)
(round a)
(trunc a)
```

#### Arguments

*a*
: A real number.

#### Result

An integer:

| Function | Result | 2.7 | -2.7 | 2.5 | -2.5 |
|---|---|---|---|---|---|
| `floor` | The nearest whole number that is not above *a* | 2 | -3 | 2 | -3 |
| `ceil` | The nearest whole number that is not below *a* | 3 | -2 | 3 | -2 |
| `round` | The nearest whole number; a half goes away from zero | 3 | -3 | 3 | -3 |
| `trunc` | The whole part: toward zero. Another name for `truncate` | 2 | -2 | 2 | -2 |

A number that is already whole is returned as it is.

#### Errors

| Message | Cause |
|---|---|
| `'truncate' expects a real number` | *a* is complex, or not a number (all four are built on `truncate`) |

#### See also

`truncate`, `to-fixed`, `div`.

#### Example

```session
hylang> (floor -2.7)
=> -3
hylang> (ceil 2.1)
=> 3
hylang> (round 2.5)
=> 3
hylang> (round -2.5)
=> -3
hylang> (round 7/2)
=> 4
```

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

A function with no formals needs nothing more before it can run, and the prompt treats it so: when the value
of a line typed there is such a function, the prompt calls it, and shows what it returns. So the name of a
function of no formals, typed alone, runs it, with or without parentheses. Inside a larger expression the
function is an ordinary value: `(list f)` is a list holding the function.

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

A function: a hash as a list.

#### Syntax

```text
(from# h)
```

#### Arguments

*h*
: A hash.

#### Result

A list of the hash's entries, each `{key value tag ...}`, in the order they were put, followed by the hash's
own tags. A value that is a hash is given as such a list too. The list is in the form `hash-create` takes, so
`(to# (from# h))` makes a hash like *h*.

#### Description

Every entry is in the list, private ones too, with their tags: `from#` is how a hash is examined, saved or
sent somewhere.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'from#' must be a hash` | *h* is not a hash |

#### See also

`hash-keys`, `hash-values`, `save`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (from# pet)
=> {{:name "Rex"} {:legs 4}}
hylang> (map fst (from# pet))
=> {:name :legs}
```

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

## gpio library

A library, loaded with `(use "gpio")`: the eight pins of the I/O header J27, and its two handshake lines.

#### Functions

| Form | What it does |
|---|---|
| `(gpio pin)` | The pin's level now: 0 or 1 |
| `(gpio! pin level)` | Makes the pin an output and sets it: low for a *level* of 0, high for anything else. `NIL` |
| `(gpio-in pin)`, `(gpio-out pin)` | Makes the pin an input, or an output. `NIL` |
| `(gpio-port)` | All eight pins' levels as one byte: bit *n* is pin *n* |
| `(gpio-port! byte)` | Sets all the outputs at once from a byte. Pins that are inputs stay inputs. `NIL` |
| `(gpio-ddr! byte)` | Sets all eight directions at once: bit *n* is 1 for an output, 0 for an input. `NIL` |
| `(gpio-ca1! edge)` | Chooses the change of CA1 that is counted: `:rise` (0 to 1) or `:fall` (1 to 0). `NIL` |
| `(gpio-ca2! v)` | CA2: 0 makes it an output that is low, 1 an output that is high, and `:in` an input. `NIL` |
| `(gpio-wait)` | Waits for CA1's next counted change, and gives the number of changes counted so far |
| `(gpio-state)` | The state of everything, as a hash |

#### Arguments

*pin*
: 0 to 7: PA0 to PA7 of the VIA, on J27's pins 3 to 10.

#### Description

The library works through the files of `/dev/gpio` (section 17.2 has the pins, the cautions about what may
be connected to them, and the files themselves). Every pin is an input when the Hydra starts, and an input
with nothing connected reads 1.

Pins 0 and 1 are also the I²C bus: while a program is using the bus, they are not the pins' to set.

`gpio-state` gives a hash with a key for each pin, 0 to 7, whose value is a list of its direction and its
level, such as `{:out 1}`; `:ca1`, a list of the edge counted and the count, such as `{:fall 0}`; and `:ca2`,
its level, or `:in`.

`gpio-wait` does not return until the line changes: <kbd>CTRL</kbd>-<kbd>C</kbd> ends the wait.

#### Errors

| Message | Cause |
|---|---|
| `/dev/gpio/8: not found` | A pin that is not 0 to 7, given to `gpio` or `gpio!` |
| `invalid argument` | A pin that is not 0 to 7, given to `gpio-in` or `gpio-out`; a byte that is not 0 to 255 |
| `'gpio-ca1!' expects :rise or :fall` | Another value for the edge |

#### See also

Section 17.2; the `dev` library; `sleep`.

#### Example

```session
hylang> (use "gpio")
=> NIL
hylang> (gpio 2)
=> 1
hylang> (gpio! 2 0)
=> NIL
hylang> (gpio 2)
=> 0
hylang> ((gpio-state) 2)
=> {:out 0}
hylang> (gpio-in 2)
=> NIL
hylang> (dotimes {i 3} (do (gpio! 3 0) (sleep 0.1) (gpio! 3 1) (sleep 0.1)))
=> NIL
hylang> (gpio-in 3)
=> NIL
```

## hash-add-tag, hash-tag?

Functions: mark a hash, or one of its entries, with a tag; and test for one.

#### Syntax

```text
(hash-add-tag h tag ...)
(hash-add-tag h {key tag} ...)
(hash-tag? h tag)
(hash-tag? h {key tag})
```

#### Arguments

*h*
: A hash.

*tag*
: An atom: a tag for the hash itself.

*{key tag}*
: A key and an atom: a tag for that key's entry.

#### Result

`hash-add-tag` gives `T` if the tag was added, and `NIL` if it was already there. With several tags, a list
of those answers. `hash-tag?` gives `T` if the hash, or the entry, has the tag, and `NIL` if not.

#### Description

A **tag** is a label a program puts on a hash or on an entry, to mean whatever the program wants: a kind of
record, a field to be saved, a unit. Tags are no part of the values. They show after an entry's value, and
after the last entry for the hash's own, when the hash is printed or listed with `from#`, and they can be
given in the same places when a hash is made.

Four tags are the system's, and change what can be done with a hash: `hash-lock` and the functions beside it
put them on.

#### Errors

| Message | Cause |
|---|---|
| `Failed to add tag: invalid parameters "notatom"` | A tag is not an atom, or the list is not a key and one tag |
| `hash-add-tag failed: entry not found` | The key is not in the hash |

#### See also

`hash-lock`, `hash-locked?`, `from#`.

#### Example

```session
hylang> (def {h} #({{:a 1} {:b 2}}))
=> NIL
hylang> (hash-add-tag h :big)
=> T
hylang> (hash-add-tag h {:a :metres})
=> T
hylang> h
=> <hash>{{:a 1 :metres} {:b 2} :big}
hylang> (hash-tag? h :big)
=> T
hylang> (hash-tag? h {:b :metres})
=> NIL
```

## hash-call

A function: call a method of a hash. Calling the hash with the key and arguments does the same.

#### Syntax

```text
(hash-call h key x ...)
(h key x ...)
```

#### Arguments

*h*
: A hash.

*key*
: The key of an entry whose value is a function.

*x*
: The function's arguments.

#### Result

What the function returns.

#### Description

A function stored in a hash is a **method**. When it is called through the hash, the symbol `&0` is bound,
inside it, to the hash, so the method can read and change the hash's other entries: `(&0 :n)`,
`(hash-put &0 {:n 1})`. A hash with data and methods is an object, and `hash-clone` makes more like it.

Through `&0` a method reaches every entry, private ones too. That is what a private entry is: one that only
the hash's methods can use.

The short form `(h key x ...)` is the usual way to write the call. With no arguments after the key it still
calls the method: `(counter :inc)`. To get the function itself, without calling it, use `hash-get`.

Given fewer arguments than it has formals, a method, like any function, returns a function of the rest.

#### Errors

| Message | Cause |
|---|---|
| `Second parameter to 'hash-call' must be a key to a member function` | The key isn't there, or its value isn't a function |
| `:v isn't a method: its value isn't a function` | The short form, with arguments after a key whose value isn't a function |
| `First parameter to 'hash-call' must be a hash` | *h* is not a hash |

#### See also

`hash-get`, `hash-clone`, `hash-make-private`, `fn`.

#### Example

```session
hylang> (def {counter} #({{:n 0} {:inc (fn {} {hash-put &0 {:n (1+ (&0 :n))}})} {:plus (fn {x} {+ x (&0 :n)})}}))
=> NIL
hylang> (counter :inc)
=> 0
hylang> (counter :inc)
=> 1
hylang> (counter :n)
=> 2
hylang> (counter :plus 10)
=> 12
hylang> (hash-call counter :plus 10)
=> 12
```

## hash-clone

A function: a copy of a hash.

#### Syntax

```text
(hash-clone h [override ...])
```

#### Arguments

*h*
: A hash.

*override*
: An entry, `{key value tag ...}`, to put in the copy: it replaces the entry of that key, or is added. An
  atom on its own is a tag for the copy. Unlike `hash-create`'s, an override's value is taken as it is
  written, not evaluated.

#### Result

A new hash with the same entries and tags, and then the overrides. The two are separate from then on: a
change to one is not seen in the other. A value that is itself a hash is copied too, so the copy shares
nothing with the original.

#### Description

This is how one object is made from another: clone a hash that has the methods and the usual values, giving
the entries that differ.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'hash-clone' must be a hash` | *h* is not a hash |

#### See also

`hash-create`, `hash-call`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (def {twin} (hash-clone pet {:name "Max"}))
=> NIL
hylang> (twin :name)
=> "Max"
hylang> (pet :name)
=> "Rex"
hylang> (hash-put twin {:legs 3})
=> 4
hylang> (pet :legs)
=> 4
```

## hash-create, to#

Functions: make a hash.

#### Syntax

```text
(hash-create [entries])
(to# entries)
#(entries)
```

#### Arguments

*entries*
: A list of entries. Each entry is a list: a key, a value, and after them any tags, `{key value tag ...}`.
  Without the argument, the hash is empty.

#### Result

A hash holding the entries, in the order given. It prints as `<hash>` and its entries.

#### Description

A **hash** is a table of values found by key. A key is an atom, a string, or an integer (a decimal with a
whole value, such as 3.0, is the integer). A value is anything, another hash or a function too.

Each entry's value is evaluated as the hash is made, so `{:legs (+ 2 2)}` stores 4; its key and its tags are
taken as written. If a key is given twice, the later entry replaces the earlier.

An item of the list that is an atom on its own, not an entry, is a tag on the hash itself (see
`hash-add-tag`).

`to#` is the same function under a shorter name, and `#(entries)` is another way to write the call.

A hash is small and searched from its first entry to its last: it is meant for records and objects, tens of
entries, not for thousands.

#### Errors

| Message | Cause |
|---|---|
| `A hash's entries are a list` | *entries* is not a list |
| `A hash's entry is {key value tag...}, not 1` | An item of the list is not an entry: usually one pair of braces is missing |
| `A hash key must be an atom, a string or an integer, not 1.5` | A key of another kind |

#### See also

`hash-put`, `hash-get`, `from#`, `hash-clone`.

#### Example

```session
hylang> (def {pet} (hash-create {{:name "Rex"} {:legs (+ 2 2)}}))
=> NIL
hylang> pet
=> <hash>{{:name "Rex"} {:legs 4}}
hylang> (len pet)
=> 2
hylang> #({{1 "one"} {"two" 2}})
=> <hash>{{1 "one"} {"two" 2}}
hylang> (hash-create {:a 1})
=> Error: A hash's entry is {key value tag...}, not 1
```

## hash-get

A function: the value at a key of a hash. Calling the hash itself does the same.

#### Syntax

```text
(hash-get h key)
(h key)
```

#### Arguments

*h*
: A hash.

*key*
: An atom, a string or an integer.

#### Result

The value stored at the key, or `NIL` if the hash has no such key. Since a stored value can itself be `NIL`,
use `hash-key?` to tell the two apart.

#### Description

A hash can stand where a function does: `(h key)` looks the key up. The two forms differ only when the value
is a function. `hash-get` gives the function as a value. `(h key)` calls it, as a method, with `&0` bound to
the hash (see `hash-call`).

`(h)`, with no key, is the hash itself. `hash-get` of `NIL` is `NIL`, so a lookup in a hash that was never
made does not stop a program.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'hash-get' must be a hash` | *h* is neither a hash nor `NIL` |
| `hash-get error: cannot access private hash entry` | The entry is private (see `hash-make-private`): only the hash's own methods can read it |

#### See also

`hash-put`, `hash-key?`, `hash-call`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (pet :name)
=> "Rex"
hylang> (hash-get pet :legs)
=> 4
hylang> (pet :wings)
=> NIL
```

## hash-key?, hash-keys, hash-values

Functions: is a key in a hash? And all its keys, and all its values.

#### Syntax

```text
(hash-key? h key)
(hash-keys h)
(hash-values h)
```

#### Arguments

*h*
: A hash.

*key*
: An atom, a string or an integer.

#### Result

`hash-key?` gives `T` if the hash has an entry with that key, whatever its value, and `NIL` if not.
`hash-keys` gives a list of the keys and `hash-values` a list of the values, both in the order the entries
were put, so the two lists match item for item. For an empty hash each is `NIL`.

Private entries are listed with the rest.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'hash-keys' must be a hash` | *h* is not a hash (each function with its own name) |

#### See also

`from#`, `hash-get`, `len`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (hash-key? pet :name)
=> T
hylang> (hash-key? pet :wings)
=> NIL
hylang> (hash-keys pet)
=> {:name :legs}
hylang> (hash-values pet)
=> {"Rex" 4}
```

## hash-lock, hash-make-const, hash-make-private, hash-make-not-nil

Functions: protect a hash, or some of its entries.

#### Syntax

```text
(hash-lock h [key ...])
(hash-make-const h [key ...])
(hash-make-private h [key ...])
(hash-make-not-nil h [key ...])
```

#### Arguments

*h*
: A hash.

*key*
: The keys of the entries to protect. With none, the protection is put on the hash as a whole.

#### Result

`T` if the protection was added, `NIL` if it was already there; a list of those for several keys.

#### Description

Each puts one of the system's four tags on the hash or on the entries named, and the tag changes what
`hash-put`, `hash-get` and `hash-remove` will do:

| Function | Tag | On the hash | On an entry |
|---|---|---|---|
| `hash-lock` | `:__locked` | No entry can be added or removed. The values of the entries it has can still be changed | |
| `hash-make-const` | `:__read-only` | Nothing can be changed, added or removed | The entry can't be changed or removed |
| `hash-make-private` | `:__private` | | The entry can be read and changed only by the hash's own methods, through `&0` |
| `hash-make-not-nil` | `:__not_nil` | | The entry can't be given the value `NIL` (it can still be removed) |

A protection, once put on, stays: there is no function that takes one off. A copy made with `from#` and
`to#` carries the tags with it.

A private entry is hidden from `hash-get` and from `hash-put`, but not from `hash-keys`, `hash-values` and
`from#`, which list it with the rest: the protection is against accidents, not against a determined program.

The same tags can be written in an entry as a hash is made: `{:a 1 :__read-only}`.

#### Errors

| Message | Cause |
|---|---|
| `hash-add-tag failed: entry not found` | A key is not in the hash |
| `First parameter to 'hash-lock' must be a hash` | *h* is not a hash (each function with its own name) |

The errors that a protection causes are listed under `hash-put`, `hash-get` and `hash-remove`.

#### See also

`hash-locked?`, `hash-add-tag`, `hash-call`.

#### Example

```session
hylang> (def {acct} #({{:owner "Ann"} {:pin 1234} {:check (fn {n} {== n (&0 :pin)})}}))
=> NIL
hylang> (hash-make-private acct :pin)
=> T
hylang> (acct :pin)
=> Error: hash-get error: cannot access private hash entry
hylang> (acct :check 1234)
=> T
hylang> (hash-make-const acct :owner)
=> T
hylang> (hash-put acct {:owner "Bob"})
=> Error: hash-put error: cannot modify read-only hash entry
hylang> (hash-lock acct)
=> T
hylang> (hash-put acct {:extra 1})
=> Error: hash-put error: cannot add new entries to a locked hash
```

## hash-locked?, hash-private?, hash-const?

Functions: is a hash, or one of its entries, protected?

#### Syntax

```text
(hash-locked? h [key])
(hash-private? h [key])
(hash-const? h [key])
```

#### Arguments

*h*
: A hash.

*key*
: An entry's key. Without it, the question is about the hash as a whole.

#### Result

`T` if the hash, or the entry, has the protection that `hash-lock`, `hash-make-private` or
`hash-make-const` puts on, and `NIL` if not.

The hash and its entries are separate: `(hash-const? h)` is `NIL` for a hash that has constant entries but
was not itself made constant.

#### Errors

| Message | Cause |
|---|---|
| `Key :v not found when looking up tag for hash entry` | The key is not in the hash |

#### See also

`hash-lock`, `hash-tag?`.

#### Example

```session
hylang> (def {k} #({{:a 1} {:b 2}}))
=> NIL
hylang> (hash-make-const k :a)
=> T
hylang> (hash-const? k :a)
=> T
hylang> (hash-const? k :b)
=> NIL
hylang> (hash-const? k)
=> NIL
hylang> (hash-locked? k)
=> NIL
```

## hash-put

A function: add entries to a hash, or change them.

#### Syntax

```text
(hash-put h {key value} ...)
```

#### Arguments

*h*
: A hash.

*{key value}*
: An entry, as `hash-create` takes: the value is evaluated, and tags may follow it. There may be several
  entries.

#### Result

The value the key had before, or `NIL` if the key is new. With several entries, a list of those values, one
for each.

#### Description

The hash itself is changed: every name bound to it sees the change. An entry whose key is already there
takes the old one's place in the order; a new key goes at the end.

Each entry needs its own braces. `(hash-put h :a 1)` is not an entry, and gives errors for `:a` and for 1.

#### Errors

| Message | Cause |
|---|---|
| `A hash's entry is {key value tag...}, not :a` | An argument is not an entry |
| `hash-put error: cannot add new entries to a locked hash` | The hash is locked, and the key is new (see `hash-lock`) |
| `hash-put error: cannot modify read-only hash` | The hash is constant (see `hash-make-const`) |
| `hash-put error: cannot modify read-only hash entry` | That entry is constant |
| `hash-put error: cannot access private hash entry` | The entry is private, and this is not one of the hash's methods |
| `Cannot set non-nillable field :v to NIL.` | The entry must not be `NIL` (see `hash-make-not-nil`) |

#### See also

`hash-get`, `hash-remove`, `hash-create`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (hash-put pet {:legs 3})
=> 4
hylang> (hash-put pet {:colour "brown"} {:age 2})
=> {NIL NIL}
hylang> (hash-keys pet)
=> {:name :legs :colour :age}
```

## hash-remove

A function: take an entry out of a hash.

#### Syntax

```text
(hash-remove h key)
```

#### Arguments

*h*
: A hash.

*key*
: The entry's key.

#### Result

The value the entry had. If there was no such key, `NIL`, and nothing is changed.

#### Errors

| Message | Cause |
|---|---|
| `hash-remove error: cannot remove an entry from a locked hash` | The hash is locked, or the entry is constant |

#### See also

`hash-put`, `hash-lock`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (hash-remove pet :legs)
=> 4
hylang> (hash-remove pet :legs)
=> NIL
hylang> (hash-keys pet)
=> {:name}
```

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

## i2c library

A library, loaded with `(use "i2c")`: the I²C bus on the I/O header.

#### Functions

| Form | What it does |
|---|---|
| `(i2c-devices)` | A list of the addresses of the chips that answer on the bus |
| `(i2c-read addr n [reg])` | Reads *n* bytes, 64 at most, from the chip at *addr*: a list. With *reg*, from that register on |
| `(i2c-write addr bytes [reg])` | Sends bytes, a list or a string's, to the chip. With *reg*, to that register. `NIL` |
| `(i2c-speed khz)` | Sets the bus's speed, in kilohertz: 1 to 100. `NIL` |
| `(i2c-reg-size n)` | Says how a register's number is sent: 0, not at all (plain bytes); 1, as one byte; 2, as two, high byte first. `NIL` |

#### Arguments

*addr*
: The chip's address on the bus, a number: `#x08` to `#x77`.

*reg*
: A register inside the chip: used only after `(i2c-reg-size 1)` or `2`, and 0 if it isn't given.

#### Description

The library works through the files of `/dev/i2c` (section 17.3). Each read or write is one transaction on
the bus. With a register size set, the register's number is sent first, and a read follows it with a
repeated start, which is what most chips with registers expect.

The settings are the bus's, not the program's: they stay as they are left.

#### Errors

| Message | Cause |
|---|---|
| `i/o error` | The chip did not answer: nothing is at that address |
| `invalid argument` | A speed or a register size out of range |

#### See also

Section 17.3; the `gpio` and `dev` libraries.

#### Example

With nothing on the bus:

```session
hylang> (use "i2c")
=> NIL
hylang> (i2c-devices)
=> NIL
hylang> (i2c-read #x50 2)
=> Error: i/o error
hylang> (i2c-speed 40)
=> NIL
```

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

Functions: the next key pressed, without waiting for a line; and whether one is waiting.

#### Syntax

```text
(key)
(key?)
```

#### Result

`key` waits for a key and gives it: a character for an ordinary key, and an atom for one of the terminal's
named keys: `:up`, `:down`, `:left`, `:right`, `:home`, `:end`, `:ins`, `:del`, `:pgup`, `:pgdn`, and `:f1` to
`:f12`. <kbd>ENTER</kbd> is the character `\cr`. At the end of the input it gives `NIL`.

`key?` gives `T` if a key has been pressed and not yet read, and `NIL` if not. It does not wait.

#### Description

Keys are read raw: as they are pressed, without being shown on the screen, and without the line editing
that `read-line` and the prompt give. The keyboard goes back to lines when the next line is read.

Together they make the loop of a game or an editor: `(if (key?) (key))` takes a key when there is one, and
lets the program go on when there isn't.

<kbd>CTRL</kbd>-<kbd>C</kbd> pressed while a program waits in `key` is the error `interrupted`.

#### See also

`read-line`, `on-note`, `sleep`.

#### Example

Here <kbd>x</kbd> and then the up arrow were pressed:

```text
hylang> (key)
=> \x
hylang> (key)
=> :up
```

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

Functions: the names in a directory; their details; one file's details; and the paths that match a pattern.

#### Syntax

```text
(ls [path])
(dir [path])
(stat path)
(glob pattern)
```

#### Arguments

*path*
: A directory, or a file. For `ls` and `dir`, the current directory if it isn't given.

*pattern*
: A path in which `*` stands for any run of characters, `?` for any one character, and `[...]` for any one of
  the characters in the brackets (`a-z` is a range, and `~` first means any character *not* listed). Each
  part of the path between slashes can be a pattern.

#### Result

`ls` gives a list of the names in the directory, as strings, sorted. Given a file, it gives a list of that
one name.

`stat` gives a hash of the file's details:

| Key | Value |
|---|---|
| `:name` | Its name, without its directory |
| `:length` | Its length in bytes (0 for a directory) |
| `:dir` | `T`, for a directory. A file has no such entry |
| `:mtime` | When it was last changed, in seconds since the start of the year 2000, as `time` counts |
| `:mode` | Its permission bits, as a number (438 is octal 666: it can be read and written) |
| `:qid` | The file system's own number for the file |
| `:dev` | The letter of the device it is on, a character |

`dir` gives a list of such hashes, one for each name `ls` would give.

`glob` gives a sorted list of the paths that match, each written as the pattern was (with the directory it
gave), and `NIL` if none does.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such file or directory |

#### See also

`exists?`, `date`, `cd`.

#### Example

```session
hylang> (mkdir "/ram/sub")
=> NIL
hylang> (write-file "/ram/sub/f1" "1")
=> NIL
hylang> (write-file "/ram/sub/f2.hl" "22")
=> NIL
hylang> (ls "/ram/sub")
=> {"f1" "f2.hl"}
hylang> (glob "/ram/sub/*.hl")
=> {"/ram/sub/f2.hl"}
hylang> ((stat "/ram/sub/f2.hl") :length)
=> 2
hylang> ((stat "/ram/sub") :dir)
=> T
hylang> (map (fn {d} {d :name}) (dir "/ram/sub"))
=> {"f1" "f2.hl"}
```

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

Constants of the library: four numbers, to many places.

#### Syntax

```text
math.pi
math.e
math.phi
math.gamma
```

#### Result

Each is a variable holding a decimal:

| Name | The number | Places |
|---|---|---|
| `math.pi` | Pi, the ratio of a circle's circumference to its diameter: 3.14159... | 125 |
| `math.e` | The base of the natural logarithms: 2.71828... | 50 |
| `math.phi` | The golden ratio: 1.61803... | 50 |
| `math.gamma` | Euler's constant: 0.57721... | 20 |

#### Description

These are ordinary decimals, so arithmetic with them is exact arithmetic on the digits given, and a result
carries every one of them: `(* 2 math.pi)` has 125 places. That is seldom what a program wants. `(pi)` gives
pi to the precision `digits` has set, 12 places to begin with, and `to-fixed` cuts any number to a length.

#### See also

`pi`, `digits`, `to-fixed`, `exp`.

#### Example

```session
hylang> math.e
=> 2.71828182845904523536028747135266249775724709369995
hylang> (to-fixed math.pi 5)
=> 3.14159
hylang> (to-fixed (* 2 math.pi) 8)
=> 6.2831853
```

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

Functions of the library, written in hylang: the least and the greatest item of a list; and its items in
falling order.

#### Syntax

```text
(minimum l)
(maximum l)
(sort-desc l)
(best-of n l)
(worst-of n l)
```

#### Arguments

*l*
: A list.

*n*
: A count.

#### Result

`minimum` and `maximum` give the least and the greatest item of the list, in `cmp`'s order: they are `min`
and `max` given the list's items as their arguments.

`sort-desc` gives the list sorted with the greatest first. `best-of` gives the *n* greatest items, greatest
first, and `worst-of` the *n* least, least first; if the list has fewer than *n*, all of them. `top` and
`bottom` are other names for `best-of` and `worst-of`.

#### Notes

Each takes one list. `(minimum 3 1)` is an error: use `min` for separate arguments.

`minimum` or `maximum` of an empty list is not an error and not `NIL`: it is the function `min`, or `max`,
waiting for an argument. Test for an empty list first.

#### See also

`min`, `max`, `sort`, `take`, `remove-min`.

#### Example

```session
hylang> (minimum {3 1 2})
=> 1
hylang> (maximum {3 1 2})
=> 3
hylang> (sort-desc {3 1 2})
=> {3 2 1}
hylang> (best-of 2 {3 1 4 2})
=> {4 3}
hylang> (worst-of 2 {3 1 4 2})
=> {1 2}
```

## mkdir, remove, rename, copy-file

Functions: make a directory; remove a file or a directory; rename a file; copy a file.

#### Syntax

```text
(mkdir path)
(remove path)
(rename path name)
(copy-file from to)
```

#### Arguments

*path*
: A string: what to make, remove or rename.

*name*
: The new name.

*from*, *to*
: The file to copy, and the file to make.

#### Result

Each gives `NIL` when it has done its work.

#### Description

`mkdir` makes one directory. The directory it is to be in must be there already: it doesn't make a path of
several.

`remove` takes away a file, or a directory that is empty.

`rename` changes a file's name, in the directory it is in. It does not move a file: only the last part of
*name* is used, and any directory written before it is ignored. To move a file to another directory, copy it
and remove the original.

`copy-file` makes *to*, or empties it if it is there, and writes *from*'s bytes into it. *to* must be a
file's name, not a directory's.

#### Errors

| Message | Cause |
|---|---|
| *path*`: already exists` | `mkdir` of something that is there |
| *path*`: not found` | The file isn't there; or, for `mkdir`, the directory it is to go in |
| *path*`: directory not empty` | `remove` of a directory that still holds something |
| *path*`: is a directory` | `copy-file` to a directory |

#### See also

`ls`, `exists?`, `write-file`.

#### Example

```session
hylang> (mkdir "/ram/t1")
=> NIL
hylang> (write-file "/ram/t1/a" "hello")
=> NIL
hylang> (rename "/ram/t1/a" "b")
=> NIL
hylang> (copy-file "/ram/t1/b" "/ram/t1/c")
=> NIL
hylang> (ls "/ram/t1")
=> {"b" "c"}
hylang> (remove "/ram/t1")
=> Error: /ram/t1: directory not empty
hylang> (remove "/ram/t1/b")
=> NIL
hylang> (remove "/ram/t1/c")
=> NIL
hylang> (remove "/ram/t1")
=> NIL
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

Functions: send a note to a task, or to a group of tasks; and choose what hylang does when one arrives.

#### Syntax

```text
(note task n)
(note-group group n)
(on-note f)
```

#### Arguments

*task*
: A task's number.

*group*
: A note group's number, as `task-info`'s `:group` gives it.

*n*
: The note: one of the atoms `:interrupt`, `:kill`, `:hangup`, `:alarm` and `:brk`, or a note's number, 1 to
  31. The numbers 16 to 31 are for programs' own purposes.

*f*
: A function of one argument, or `NIL`.

#### Result

Each gives `NIL`.

#### Description

A **note** is a short message from one task to another, or from the system: <kbd>CTRL</kbd>-<kbd>C</kbd> is
the note `:interrupt`, sent to every task in the window's group. `note` sends one to a task, and
`note-group` to every task of a group, hylang itself included if it is in it.

What a note does to the task that gets it is that task's choice. Left alone, hylang treats `:interrupt` as
the error `interrupted`, which stops what it is doing and returns to the prompt, and ends when any other
note arrives, as most programs do.

`on-note` changes that. From then on, each note that arrives is passed to *f*, as an atom for the named
notes and a number for the others. If *f* returns a true value, the note has been dealt with and the
program goes on. If it returns `NIL`, the note has its usual effect. `(on-note NIL)` goes back to the usual
effect for all of them.

*f* is not called at the instant the note arrives, but at hylang's next step after it, so it can do anything
a function can. The `:kill` note can't be caught: it always ends the task.

A task ended by a note has the exit code 128 plus the note's number: 130 for `:interrupt`, 137 for `:kill`.

#### Errors

| Message | Cause |
|---|---|
| *task*`: no such task` | There is no such task |
| `'note' expects a note: :interrupt, :kill, :hangup, :alarm, :brk, or 1-31` | *n* is not a note |
| `'on-note' expects a function or NIL` | *f* is neither |

#### See also

`kill`, `wait`, `sleep`, `key`; the commands `kill` and `slay` (Chapter 2 of this book).

#### Example

```session
hylang> (def {got} {})
=> NIL
hylang> (on-note (fn {n} {do (set! {got} (cons n got)) T}))
=> NIL
hylang> (note (pid) :alarm)
=> NIL
hylang> (note (pid) 20)
=> NIL
hylang> got
=> {20 :alarm}
hylang> (def {k} (spawn "sleep" "30"))
=> NIL
hylang> (note k :interrupt)
=> NIL
hylang> (wait k)
=> 130
```

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

Functions of the library, written in hylang: tests of a number, which give an error for anything else.

#### Syntax

```text
(odd? n)
(even? n)
(positive? a)
(negative? a)
```

#### Arguments

*n*, *a*
: A number.

#### Result

`T` or `NIL`. `odd?` is `T` when *n* leaves 1 on division by 2, and `even?` when it leaves 0; a number that
is not whole is neither. `positive?` is `T` when *a* is above 0, and `negative?` when it is below: 0 is
neither.

#### Description

`positive?` and `negative?` are `pos?` and `neg?` with a check: those two answer `NIL` for a value that is
not a number, and these give an error.

#### Errors

| Message | Cause |
|---|---|
| `Cannot perform 'odd?' on non-number` | `odd?` or `even?` (with its own name) of something that is not a number |
| `Only numbers can be positive` | `positive?` of something that is not a number (and `negative` for `negative?`) |

#### See also

`pos?`, `neg?`, `zero?`, `mod`.

#### Example

```session
hylang> (odd? 3)
=> T
hylang> (even? 3)
=> NIL
hylang> (odd? -3)
=> T
hylang> (filter even? (range 10))
=> {0 2 4 6 8}
hylang> (positive? 0)
=> NIL
```

## open

A function: opens a file, giving a stream.

#### Syntax

```text
(open path [mode])
```

#### Arguments

*path*
: The file's name, a string. A name that doesn't start with `/` is looked for from the current directory.

*mode*
: An atom: how the file will be used. Without it, `:read`.

| Mode | The file | The stream |
|---|---|---|
| `:read` | Must be there | Is read from its start |
| `:write` | Is made, or emptied if it is there | Is written from its start |
| `:append` | Is made if it isn't there, and nothing in it is lost | Is written at its end |
| `:update` | Must be there. Nothing is emptied | Is read and written in place, from its start |

#### Result

A stream: a value that stands for the open file and its current place. It prints as `<stream>`.

#### Description

A stream is passed to the reading functions (`read-line`, `read-byte`, `read-all`, `read-bytes`,
`read-buffer`), the writing functions (`print-to`, `write-to`, `write-bytes`), `seek` and `tell`, and then to
`close`. Each read or write moves the stream's place on by what was read or written.

Any file can be opened, a device's too: `(open "/dev/time")` is a stream on the clock.

A program should close what it opens: a task can have only so many files open at once. For a whole file at a
time, `read-file`, `read-lines` and `write-file` open and close it themselves.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | `:read` or `:update` of a file that isn't there, or a directory in the path that isn't |
| *path*`: is a directory` | The path names a directory. Use `ls` or `dir` |
| `'open' mode must be :read, :write, :append or :update, not :bogus` | Another atom was given as the mode |
| `'open' mode must be :read, :write, :append or :update` | The mode is not an atom |
| `'open' expects a String` | *path* is not a string |

Other failures give the system's message after the path (Chapter 3 of this book lists them), and its code as the error's.

#### See also

`close`, `read-line`, `print-to`, `seek`, `read-file`, `write-file`, `exists?`.

#### Example

```session
hylang> (def {w} (open "/ram/d.txt" :write))
=> NIL
hylang> (print-to w "hello" 42)
=> NIL
hylang> (close w)
=> NIL
hylang> (def {s} (open "/ram/d.txt"))
=> NIL
hylang> (read-line s)
=> "hello 42"
hylang> (close s)
=> NIL
hylang> (open "/ram/nothere")
=> Error: /ram/nothere: not found
```

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

## pc library

A library, loaded with `(use "pc")`: is the PC's file server there?

#### Functions

| Form | What it does |
|---|---|
| `(pc?)` | `T` if the PC tool answers on the serial line, so that `/pc` can be used. If nothing answers within a second, `NIL` |

#### Description

`/pc` is a folder, or a disk image, on the PC at the other end of the serial cable, served by the PC tool
(section 9.9). Its files are ordinary files: `read-file`, `ls`, `open` and the rest work on them, and need no
library. This one function lets a program find out, before it tries, whether the tool is running.

#### See also

`exists?`, `ls`; section 9.9.

## peek, poke, peek-word, poke-word

Functions: read and write the task's memory.

#### Syntax

```text
(peek addr)
(poke addr byte)
(peek-word addr)
(poke-word addr n)
```

#### Arguments

*addr*
: An address, 0 to 65,535 (`#xFFFF`).

*byte*
: A whole number from 0 to 255.

*n*
: A whole number from 0 to 65,535.

#### Result

`peek` gives the byte at the address, and `peek-word` the 16-bit word there: the byte at *addr* is its low
half and the byte at *addr* + 1 its high half, as the 65C02 keeps a word. `poke` and `poke-word` store a
byte, or a word in the same order, and give `NIL`.

#### Description

The memory is this task's own 64K, as it is while hylang runs: the task's RAM from `$0000` to `$7FFF`, the
RAM bank it has chosen at `$8000`, a bank of hylang's own code at `$A000`, and the kernel's ROM and the I/O
ports from `$E000` up (Appendix F). Other tasks' memory can't be reached.

Reading is always safe. Writing is not: hylang itself lives in this memory, with its stack, its variables
and its values, and a `poke` into any of them is likely to stop it. `poke` is for the places a program
knows to be its own, such as a block it has set aside, and for the hardware's registers. For a block of
bytes to work in, a buffer, or a RAM bank from `bank-alloc`, is the safe choice.

#### Errors

| Message | Cause |
|---|---|
| `'peek' was given a number out of its range` | The address, or the value, is too big (each function with its own name) |
| `'peek' expects a whole number` | An argument is negative, or not a whole number |

#### See also

`buffer`, `bank-read`, `lo`, `hi`, `word`, `hex`.

#### Example

```session
hylang> (hex (peek-word #xFFFC) 4)
=> "E000"
hylang> (hex (peek #xFFFC) 2)
=> "00"
hylang> (hex (peek #xFFFD) 2)
=> "E0"
```

(`$FFFC` holds the address the 65C02 starts from when it is reset.)

## pid

A function: this task's number.

#### Syntax

```text
(pid)
```

#### Result

The number of the task hylang is running in, 1 to 15. It is the number `ps` shows for it, and the name of
its directory in `/proc`.

#### See also

`task-info`, `ps`, `note`.

#### Example

```session
hylang> (pid)
=> 2
hylang> ((task-info (pid)) :name)
=> "hylang"
```

## platform, hydra?

Functions: which system hylang is running on.

#### Syntax

```text
(platform)
(hydra?)
```

#### Result

On the Hydra-16, `platform` gives the atom `:hydra`, and `hydra?` gives `T`.

#### Description

hylang is the Hydra's version of a language that also runs on a PC (danlang). A program meant for both asks
these before it uses anything that only the Hydra has: its devices, its RAM banks, its system calls.

#### Example

```session
hylang> (platform)
=> :hydra
hylang> (hydra?)
=> T
```

## print

A function: prints values, and ends the line.

#### Syntax

```text
(print x ...)
```

#### Arguments

*x*
: Any values. There may be none.

#### Result

`NIL`. The values are written to standard output with one space between them, and then a new line. With no
arguments, only the new line.

#### Description

Each value is written as text meant for a person. A string is written as its characters, without quotes, and
a character as itself. A number is written in the current base. A list is written in braces, and the strings
and characters *inside* a list keep their quotes and backslashes, so that the items can be told apart.

`print` accepts an error, and prints it as the prompt would.

To write values as they would be typed, so that they can be read back, use `save`. To write without the new
line or the spaces, use `write`. To write to a file, use `print-to`.

#### See also

`write`, `save`, `print-to`, `format`, `output-of`.

#### Example

```session
hylang> (print "one" 2 {3})
one 2 {3}
=> NIL
hylang> (print "a" \b :c 1/2)
a b :c 1/2
=> NIL
hylang> (print {1 "two" \3})
{1 "two" \3}
=> NIL
```

## print-to, write-to

Functions: `print` and `write`, to a stream.

#### Syntax

```text
(print-to s x ...)
(write-to s x ...)
```

#### Arguments

*s*
: A stream open for writing: `:write`, `:append` or `:update`.

*x*
: Any values.

#### Result

`NIL`. `print-to` writes what `print` would show: the values with spaces between, then a line feed.
`write-to` writes what `write` would: the values with nothing between or after them.

#### Errors

| Message | Cause |
|---|---|
| `'print-to' expects a stream first` | *s* is not a stream (and `'write-to'`) |
| `Cannot write this stream` | The stream was opened with `:read`, or has been closed |

#### See also

`print`, `write`, `write-bytes`, `write-file`, `open`.

#### Example

```session
hylang> (def {w} (open "/ram/d.txt" :write))
=> NIL
hylang> (print-to w "hello" 42)
=> NIL
hylang> (write-to w "no" "nl")
=> NIL
hylang> (close w)
=> NIL
hylang> (read-file "/ram/d.txt")
=> "hello 42\nnonl"
```

## proc library

A library, loaded with `(use "proc")`: a look inside any task.

#### Functions

| Form | What it does |
|---|---|
| `(task-args task)` | Its program's arguments: a list of strings |
| `(task-cwd task)` | Its current directory: a string |
| `(task-env task)` | Its environment: a hash, each variable's name, a string, and its value, a string (several words with spaces between) |
| `(task-ns task)` | Its namespace: a list of strings, the `bind` and `mount` lines that made it |
| `(task-regs task)` | The processor's registers as they were when the task last stopped running: a hash of numbers, `:pc`, `:a`, `:x`, `:y`, `:s`, `:p`, `:w`, `:u`, `:ram` and `:rom` |
| `(task-mem task addr n)` | *n* bytes of the task's memory from *addr*, as the task sees its 64K: a list |
| `(task-ram task bank offset n)` | *n* bytes of one of the task's RAM banks, from *offset*: a list |

#### Arguments

*task*
: A task's number.

#### Description

The library reads the files of `/proc` (section 17.8). With `ps` and `task-info`, which are always there, it
is what a program needs to watch another: the debugger `db` reads the same files.

The registers and the memory of the kernel's task and of the drivers can't be read.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such task |
| `not allowed` | `task-regs`, `task-mem` or `task-ram` of the kernel's task, or a driver's |

#### See also

`ps`, `task-info`, `pid`, `peek`; section 17.8.

#### Example

```session
hylang> (use "proc")
=> NIL
hylang> (task-args 1)
=> {"-l"}
hylang> (task-cwd (pid))
=> "/"
hylang> ((task-env (pid)) "prog")
=> "/bin/hylang"
hylang> (fst (task-ns (pid)))
=> "bind '#/' /"
hylang> (hex ((task-regs 1) :pc) 4)
=> "EAA0"
```

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

A function: reads bytes from a stream into a buffer.

#### Syntax

```text
(read-buffer s b [at [n]])
```

#### Arguments

*s*
: A stream open for reading.

*b*
: The buffer to read into.

*at*
: Where in the buffer the first byte goes. Without it, 0.

*n*
: The most bytes to read. Without it, as many as fit from *at* to the end of the buffer.

#### Result

The number of bytes read, which is less than was asked for when the file has fewer left; and `NIL` at the
end of the file. The bytes are in the buffer from *at* on, and the rest of the buffer is as it was.

#### Description

This is the way to read a file a block at a time without making a new value for each block: one buffer is
used over and over, and the loop ends when the result is `NIL`. `write-bytes` writes a buffer, or a part of
one, back to a stream.

#### Errors

| Message | Cause |
|---|---|
| `'read-buffer' expects a stream` | *s* is not a stream |
| `Cannot read from this stream` | The stream is not open for reading |

#### See also

`read-bytes`, `write-bytes`, `open`, `buffer`.

#### Example

```session
hylang> (write-file "/ram/bf" "0123456789")
=> NIL
hylang> (def {s} (open "/ram/bf"))
=> NIL
hylang> (def {r} (buffer 4))
=> NIL
hylang> (read-buffer s r)
=> 4
hylang> r
=> <buffer>{48 49 50 51}
hylang> (read-buffer s r)
=> 4
hylang> (read-buffer s r)
=> 2
hylang> (read-buffer s r)
=> NIL
hylang> (close s)
=> NIL
```

## read-bytes, write-bytes

Functions: read and write a stream's bytes as numbers.

#### Syntax

```text
(read-bytes s n)
(write-bytes s bytes)
(write-bytes s buffer [i [n]])
```

#### Arguments

*s*
: A stream, open for reading (`read-bytes`) or for writing (`write-bytes`).

*n*
: How many bytes: a whole number, 0 or more.

*bytes*
: A list of whole numbers from 0 to 255.

*buffer*
: A buffer. With *i*, its bytes from position *i* on are written; with *n* too, only *n* of them.

#### Result

`read-bytes` gives a list of the next *n* bytes. Near the end of the file the list is shorter than was asked
for, and at the end it is `NIL`.

`write-bytes` gives `NIL`. The bytes are written as they are, with nothing added: this is how a file that is
not text is made.

#### Errors

| Message | Cause |
|---|---|
| `A count must be an integer` | *n* is negative, or not a whole number |
| `'write-bytes' expects a list of bytes` | The second argument is neither a list nor a buffer. For a string's bytes, use `write-to`, or `(bytes s)` |
| `256 isn't a byte` | An item of the list is not a whole number from 0 to 255 |
| `'write-bytes' takes a part (i n) only of a buffer` | *i* was given with a list |
| `Cannot write this stream` | The stream is not open for writing |

#### See also

`read-byte`, `read-buffer`, `bytes`, `buffer`, `number-bytes`.

#### Example

```session
hylang> (def {w} (open "/ram/data" :write))
=> NIL
hylang> (write-bytes w {72 105 33})
=> NIL
hylang> (close w)
=> NIL
hylang> (def {s} (open "/ram/data"))
=> NIL
hylang> (read-bytes s 2)
=> {72 105}
hylang> (read-bytes s 100)
=> {33}
hylang> (read-bytes s 100)
=> NIL
hylang> (close s)
=> NIL
```

## read-file, read-lines

Functions: a whole file, as one string or as a list of lines.

#### Syntax

```text
(read-file path)
(read-lines path)
```

#### Arguments

*path*
: The file's name, a string.

#### Result

`read-file` gives the file's contents as one string, line ends and all. `read-lines` gives a list of its
lines, each a string without its line end; a carriage return before a line feed is dropped, and a last line
with no line end still counts. An empty file is `""`, and `NIL`.

#### Description

The file is opened, read and closed in the one call. The whole of it must fit in hylang's memory: for a
large file, `open` it and read it a line at a time.

Devices are files, so these read them too: `(read-file "/dev/time")` is the date and time.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such file (and the system's other messages, for its other errors) |
| `'read-file' expects a path` | *path* is not a string |

#### See also

`write-file`, `open`, `read-line`, `str-split`, `load`.

#### Example

```session
hylang> (write-file "/ram/a.txt" "line one\nline two\n")
=> NIL
hylang> (read-file "/ram/a.txt")
=> "line one\nline two\n"
hylang> (read-lines "/ram/a.txt")
=> {"line one" "line two"}
hylang> (read-lines "/rom/lib/shell")
=> {"/bin/forth -l"}
hylang> (read-file "/ram/nothere")
=> Error: /ram/nothere: not found
```

## read-line, read-byte, read-all

Functions: read from the keyboard, or from a stream.

#### Syntax

```text
(read-line [s])
(read-byte [s])
(read-all [s])
```

#### Arguments

*s*
: A stream open for reading (see `open`). Without it, standard input: the keyboard, unless hylang's input
  was redirected.

#### Result

| Function | Result | At the end |
|---|---|---|
| `read-line` | The next line, as a string, without the line end that closed it | `NIL` |
| `read-byte` | The next byte, as a number from 0 to 255 | `NIL` |
| `read-all` | Everything from the current place to the end, as one string | `""` |

#### Description

A line ends at a line feed, and a carriage return before it is dropped. The last line of a file is given
whether or not it ends with one.

Because `read-line` gives `NIL` only at the end, and an empty line is `""`, which is true, a loop can read
until `NIL`. From the keyboard, the end is <kbd>CTRL</kbd>-<kbd>D</kbd> at the start of a line.

The keyboard gives its characters a line at a time, when <kbd>ENTER</kbd> is pressed, so `read-byte` from
the keyboard waits for a whole line and then gives its bytes one by one, the line feed last. For keys one at
a time as they are pressed, use `key`.

#### Errors

| Message | Cause |
|---|---|
| `Cannot read from this stream` | The stream was opened for writing only, or has been closed |

#### See also

`open`, `read-file`, `read-lines`, `read-bytes`, `key`.

#### Example

```session
hylang> (write-file "/ram/a.txt" "line one\nline two\n")
=> NIL
hylang> (def {s} (open "/ram/a.txt"))
=> NIL
hylang> (read-line s)
=> "line one"
hylang> (read-byte s)
=> 108
hylang> (read-all s)
=> "ine two\n"
hylang> (read-line s)
=> NIL
hylang> (close s)
=> NIL
```

## remove-fst, remove-all, remove-min, remove-max

Functions of the library, written in hylang: a list without some of its items.

#### Syntax

```text
(remove-fst x l)
(remove-all x l)
(remove-min l)
(remove-max l)
```

#### Arguments

*x*
: A value to look for. It is a value, not a test: items are compared with it as `==` compares.

*l*
: A list.

#### Result

A new list. `remove-fst` leaves out the first item equal to *x*, and `remove-all` every item equal to it;
if there is none, the list is returned as it was. `remove-min` leaves out the least item, and `remove-max`
the greatest: one of them, the first, if several are equal.

To remove the items that pass a test, use `filter` with the opposite test.

#### See also

`filter`, `distinct`, `minimum`, `drop`.

#### Example

```session
hylang> (remove-fst 2 {1 2 3 2})
=> {1 3 2}
hylang> (remove-all 2 {1 2 3 2})
=> {1 3}
hylang> (remove-min {3 1 2 1})
=> {3 2 1}
hylang> (remove-max {3 1 4 4})
=> {3 1 4}
```

## repeat, times

Functions of the library, written in hylang: a list made by evaluating something several times.

#### Syntax

```text
(repeat n d)
(times n d)
```

#### Arguments

*n*
: How many items: a whole number, 0 or more.

*d*
: What each item is to be. It is evaluated afresh for each item, as `eval` evaluates: a Q-expression is run
  as code, and anything else is itself.

#### Result

A list of *n* items; `NIL` for 0. `times` is another name for `repeat`.

#### Description

Because *d* is evaluated each time, a Q-expression that calls `random`, or reads a line, gives a different
item each time: `(repeat 3 {random 6})` is three throws of a die. A number or a string simply appears *n*
times.

To have a list itself as each item, wrap it once more: `(repeat 2 {{1 2}})`.

#### See also

`range`, `dotimes`, `map`, `str-repeat`.

#### Example

```session
hylang> (repeat 3 7)
=> {7 7 7}
hylang> (repeat 2 {+ 1 2})
=> {3 3}
hylang> (repeat 2 {{1 2}})
=> {{1 2} {1 2}}
hylang> (len (repeat 5 {random 6}))
=> 5
```

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

Functions: run a program and wait for it; start one without waiting; wait for one that was started; end one.

#### Syntax

```text
(run prog x ...)
(spawn prog x ...)
(wait task)
(kill task)
```

#### Arguments

*prog*
: The program: a path, or a bare name, which is looked for in `/bin`.

*x*
: The program's arguments. Each value is turned to text as `print` shows it, and is one argument, whatever
  spaces it holds.

*task*
: A task's number, as `spawn` gave it.

#### Result

`run` starts the program, waits until it ends, and gives its exit code: 0 for success, by the system's
custom, and something else for a failure.

`spawn` starts the program and returns at once with its task number. The program runs alongside hylang, with
the same standard input and output, so what it prints appears on the same screen.

`wait` waits until the task ends, and gives its exit code. Each task can be waited for once, and only by the
program that started it.

`kill` sends the task the kill note, which ends it, and gives `NIL`. The exit code that `wait` then gives is
137 (128 + 9, the kill note's number).

#### Description

The program is started directly, with no shell: no pattern is expanded and nothing is redirected. For a
command line with pipes, patterns or redirection, use `sh`.

A program that reports its own failure still ran, so `run` gives its exit code. `run` gives an *error* only
when the program could not be started.

#### Errors

| Message | Cause |
|---|---|
| *prog*`: not found` | There is no such program |
| `'run' expects a path` | *prog* is not a string |
| `A task must be an integer` | *task* is not a whole number |
| *task*`: no such child` | `wait` for a task that this program did not start, or has already waited for |
| *task*`: no such task` | `kill` of a task that isn't there |

#### See also

`sh`, `sh-out`, `note`, `pid`, `ps`.

#### Example

```session
hylang> (run "echo" "hello" "there")
hello there
=> 0
hylang> (run "ls" "/nothere")
ls: /nothere: not found
=> 1
hylang> (run "nothere")
=> Error: nothere: not found
hylang> (def {tk} (spawn "sleep" "60"))
=> NIL
hylang> (kill tk)
=> NIL
hylang> (wait tk)
=> 137
```

## save

A function: writes a value as it would be typed, to the screen or to a file.

#### Syntax

```text
(save x)
(save x path [mode])
```

#### Arguments

*x*
: Any value.

*path*
: A file to write to. Without it, the value is printed.

*mode*
: What to do if the file is already there: `:overwrite`, to replace its contents, or `:append`, to add to
  them.

#### Result

`NIL`.

#### Description

The value is written in the form that reads back as the same value: a string with its quotes and escapes, a
character with its backslash, a hash as `#(...)`, and a new line after it. This is the opposite of `read` and
`load`: a file of saved values is a file of hylang source, and `load` gives the value of the last one in it.

For a file that does not exist, no mode is needed. For one that does, a mode must be given, so that a file is
never replaced by accident.

#### Errors

| Message | Cause |
|---|---|
| `File exists but neither ':append' nor ':overwrite' were specified` | The file is there, and no mode was given |
| The system's message, after the path | The file could not be made or written |

#### See also

`print`, `repr`, `load`, `read`, `write-file`.

#### Example

```session
hylang> (save "a string")
"a string"
=> NIL
hylang> (save {1 "two" \3 :four 5/6})
{1 "two" \3 :four 5/6}
=> NIL
hylang> (save {1 2} "/ram/saved.hl")
=> NIL
hylang> (save {3 4} "/ram/saved.hl" :append)
=> NIL
hylang> (read-file "/ram/saved.hl")
=> "{1 2}\n{3 4}\n"
hylang> (load "/ram/saved.hl")
=> {3 4}
```

## screen library

A library, loaded with `(use "screen")`: the terminal's cursor, colours and clearing.

#### Functions

| Form | What it does |
|---|---|
| `(cls)` | Clears the screen, and puts the cursor at its top left |
| `(at col row)` | Moves the cursor: columns and rows count from 1, from the top left |
| `(clear-line)` | Clears from the cursor to the end of its line |
| `(color fg [bg])` | Sets the colour of the text written from now on, and its background's if given: each one of the atoms `:black`, `:red`, `:green`, `:yellow`, `:blue`, `:magenta`, `:cyan`, `:white` |
| `(bold)`, `(plain)` | Bold text; and back to plain text in the usual colours |
| `(cursor-off)`, `(cursor-on)` | Hides the cursor; shows it |

#### Description

Each writes one of the terminal's control sequences to standard output (section 13.7 lists them), so they
work on the Hydra's console, in any of its windows, and on a PC's terminal alike. They act on the window
hylang is writing to.

What they set stays set after the program ends: a program that changes the colour or hides the cursor
should finish with `(plain)` and `(cursor-on)`.

#### See also

`write`, `key`, the `cons` library; section 13.7.

#### Example

A title in the middle of a cleared screen:

```text
(use "screen")
(cls)
(at 30 12)
(color :yellow :blue)
(write " The Hydra-16 ")
(plain)
(at 1 24)
```

## seek, tell

Functions: set, and ask, a stream's place in its file.

#### Syntax

```text
(seek s n)
(tell s)
```

#### Arguments

*s*
: A stream.

*n*
: The place to move to: a number of bytes from the start of the file, from 0 to the file's length.

#### Result

Both give the stream's place, as a number of bytes from the start: for `seek`, the place it has just set.
The next read or write happens there.

#### Description

A place is counted in bytes, the first being 0. `(seek s 0)` goes back to the start. To find a file's
length, ask `stat`.

With a stream opened `:update`, `seek` is how a part of a file is changed in place: writing there replaces
the bytes that were there, and does not move the ones after.

#### Errors

| Message | Cause |
|---|---|
| `Cannot seek past end of stream` | *n* is more than the file's length |
| `Cannot seek before beginning of stream` | *n* is negative |
| `'seek' expects a stream` | *s* is not a stream |

#### See also

`open`, `stat`, `read-bytes`.

#### Example

```session
hylang> (write-file "/ram/d.txt" "hello 42\n")
=> NIL
hylang> (def {u} (open "/ram/d.txt" :update))
=> NIL
hylang> (seek u 6)
=> 6
hylang> (read-line u)
=> "42"
hylang> (tell u)
=> 9
hylang> (seek u 0)
=> 0
hylang> (write-to u "J")
=> NIL
hylang> (close u)
=> NIL
hylang> (read-file "/ram/d.txt")
=> "Jello 42\n"
```

## seg-create, seg-attach, seg-detach, seg-read, seg-write

Functions: shared memory segments, which several tasks can use together.

#### Syntax

```text
(seg-create banks)
(seg-attach seg)
(seg-detach seg)
(seg-read seg bank offset n)
(seg-write seg bank offset bytes)
```

#### Arguments

*banks*
: The segment's size, in 8K banks: 1 to 128.

*seg*
: A segment's number, as `seg-create` gave it.

*bank*
: Which of the segment's banks, the first being 0.

*offset*
: A position in that bank, 0 to 8,191.

*n*
: A number of bytes.

*bytes*
: A list of whole numbers from 0 to 255.

#### Result

`seg-create` makes a segment of the shared RAM and gives its number. `seg-attach` and `seg-detach` join the
task to a segment and take it away again, and give `NIL`. `seg-read` gives a list of *n* bytes from one of
the segment's banks, and `seg-write` stores bytes there and gives `NIL`. As with a task's own banks, a read
or a write must stay inside one bank.

#### Description

A **segment** is a piece of the Hydra's shared RAM (section 17.9). Unlike a task's own banks, a segment can
be attached by any task that knows its number, so it is how two programs share a large amount of data
without copying it through a pipe: one creates the segment and passes its number, as an argument or in a
file, and both attach it. A segment lasts while any task has it attached, or while it has a name in
`/dev/seg`.

A task must be attached to a segment to read or write it. The task that creates one is attached to it
already; any other attaches it with `seg-attach`.

#### Errors

| Message | Cause |
|---|---|
| *seg*`: invalid argument` | A read or a write of a segment that isn't attached |
| *seg*`: out of range` | *bank* is not one of the segment's banks |
| `'seg-read' was given a number out of its range` | The bytes would run past the bank's end; or a number is too big for what it is |
| *banks*`: invalid argument` | `seg-create` of no banks |

#### See also

`bank-alloc`, `free`; section 17.9.

#### Example

The segment's number depends on what is already in use:

```text
hylang> (def {sg} (seg-create 1))
=> NIL
hylang> sg
=> 14
hylang> (seg-attach sg)
=> NIL
hylang> (seg-write sg 0 0 {1 2 3})
=> NIL
hylang> (seg-read sg 0 0 3)
=> {1 2 3}
hylang> (seg-detach sg)
=> NIL
```

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

Functions: run a command line with the shell; and the same, giving what it printed.

#### Syntax

```text
(sh line [input])
(sh-out line [input])
```

#### Arguments

*line*
: A command line, as it would be typed at rc's prompt: pipes, patterns, redirection and variables all work.

*input*
: Text to give the command as its standard input. Without it, the command reads what hylang itself would:
  the keyboard.

#### Result

`sh` runs the line (with `rc -c`) and gives its exit status, a number: 0 for success. What the command
prints goes to the screen.

`sh-out` runs it the same way but collects what it writes to standard output, and gives that as a string,
line ends included. What it writes to standard error still goes to the screen, and its exit status is not
given.

#### Description

These are the bridge to everything else on the Hydra: any program, and any pipeline of them, can do part of
a hylang program's work. `str-split` with `"\n"`, or `read-lines` on a file the command wrote, turns the
output into a list.

The command gets a copy of hylang's environment, so a variable set with `setenv` is `$name` in the line.

#### See also

`run`, `output-of`, `setenv`, `str-split`.

#### Example

```session
hylang> (sh "echo hi | wc -c")
      3
=> 0
hylang> (sh "ls /nothere")
ls: /nothere: not found
=> 1
hylang> (sh-out "echo hi")
=> "hi\n"
hylang> (sh-out "cat" "from input")
=> "from input"
hylang> (str-split (str-trim (sh-out "ls /rom/lib | head -3")) "\n")
=> {"as/" "basic/" "edit/"}
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

## sleep, sleep-until, yield

Functions: wait for a time; wait until a tick; and give other tasks a turn.

#### Syntax

```text
(sleep s)
(sleep-until tick)
(yield)
```

#### Arguments

*s*
: A number of seconds, 0 or more. It may be a fraction or a decimal: `(sleep 1/10)`.

*tick*
: A value of the tick count, as `ticks` gives it, at most 32,767 ticks ahead.

#### Result

Each gives `NIL`, when the wait is over. While hylang sleeps it uses none of the processor's time, and the
other tasks run.

`sleep-until` waits until the tick count reaches *tick*. A loop that adds a fixed step to a tick value and
sleeps until it keeps an even beat, however long each turn of the loop takes: `sleep`, which waits from when
it is called, would drift.

`yield` does not wait: it ends hylang's turn at once, so that any other task that is ready runs before
hylang goes on.

#### Description

A note that arrives during a sleep ends it early. <kbd>CTRL</kbd>-<kbd>C</kbd> does: the sleep's result is
then the error `interrupted`.

#### Errors

| Message | Cause |
|---|---|
| `'sleep' expects a number of seconds` | *s* is negative, or not a number |

#### See also

`ticks`, `clock`, `on-note`, `key?`.

#### Example

```session
hylang> (sleep 1/2)
=> NIL
hylang> (dotimes {i 3} (do (write i " ") (sleep 0.1)))
0 1 2 => NIL
```

## snd library

A library, loaded with `(use "snd")`: notes and sounds from the YM2151 and, with a Vera X, its PSG.

#### Functions

| Form | What it does |
|---|---|
| `(snd-claim ch ...)`, `(snd-release ch ...)` | Claims channels, so that no other program's sounds reach them; and gives them back |
| `(snd-patch ch n)` | Loads an instrument into the channel: one of the 163 patches, 0 to 162 |
| `(snd-note ch note)` | Starts a note: a MIDI note number, 60 being middle C |
| `(snd-off ch)` | Ends the channel's note (it dies away as its instrument does) |
| `(snd-level ch v)` | The channel's own volume: 0 to 127 |
| `(snd-pan ch pan)` | Which speakers: `:left`, `:right` or `:both` |
| `(snd-bend ch n)` | Bends the pitch, by *n* 64ths of a semitone, up or down |
| `(snd-drum ch n)` | Plays a drum: a General MIDI drum number (35 is a kick) |
| `(snd-freq ch hz)` | Starts a note at a frequency, in hertz; 0 ends it |
| `(snd-glide ch note)` | Moves the sounding note to another pitch without starting it again |
| `(snd-wave ch w [width])` | For a PSG channel: its waveform, `:pulse`, `:saw`, `:triangle` or `:noise`, and a pulse's width, 0 to 63 |
| `(snd-volume v)` | The master volume for all sound: 0 to 200, 100 being as written |
| `(snd-reset)` | Silences the chip and clears every setting |
| `(snd-lfo rate pmd amd wave)` | The YM2151's low-frequency oscillator: its rate, its depths of pitch and of loudness (0 to 127), and its wave (0 saw, 1 square, 2 triangle, 3 noise) |
| `(snd-sens ch pms ams)` | How much the oscillator moves a channel: its pitch (0 to 7) and its loudness (0 to 3) |
| `(snd-noise n)` | Noise on channel 7, at frequency 0 to 31; a negative number turns it off |
| `(snd-reg reg value ...)` | Writes the chip's own registers: a register and its value, in pairs |
| `(snd-regs)` | The chip's 256 registers as they were last written: a buffer |
| `(note-of name)` | A note's MIDI number from its name: `"C4"` is 60, `"C#4"` and `"Db4"` 61, `"A4"` 69 |
| `(tune notes [ch [tempo]])` | Plays a list of notes, each `{note beats}`, in time. A note is a name, a number, or `NIL` for a rest. The channel is 0 and the tempo 120 beats a minute if they aren't given |
| `(play path [times])` | Plays a song file (ZSM) with the `play` program: its exit code |
| `(snd-mml ch mml)`, `(snd-chord ch notes)` | Plays a line of the score language (MML) on a channel, or its notes together as a chord, with the `play` program: its exit code |

#### Arguments

*ch*
: A channel: 0 to 7 are the YM2151's eight voices, and 8 to 23, with a Vera X, the PSG's sixteen.

#### Description

The library works through the sound driver's files, `/dev/snd` and `/dev/sndctl` (Chapter 15 describes the
driver, its channels, the patches and MML). Each function gives `NIL` unless the table says otherwise.

A note sounds until it is ended: `snd-note`, a `sleep`, and `snd-off` make one note of a chosen length.
`tune` does that for a whole list, keeping time by the system's tick.

A channel a program has claimed is its own until it is released or the program ends. `play`, `snd-mml` and
`snd-chord` run another program, which needs the channel free: release it first, or they report
`channels busy`.

#### Errors

| Message | Cause |
|---|---|
| `'snd-pan' expects :left, :right or :both` | Another value for the speakers |
| `'note-of' expects a note: C4, C#4, Db4 ...` | A name that is not a note's |

#### See also

Chapter 15; section 22.6; the command `play` (Chapter 2 of this book).

#### Example

```session
hylang> (use "snd")
=> NIL
hylang> (note-of "A4")
=> 69
hylang> (snd-patch 0 5)
=> NIL
hylang> (snd-note 0 (note-of "C4"))
=> NIL
hylang> (sleep 1/2)
=> NIL
hylang> (snd-off 0)
=> NIL
hylang> (tune {{"C4" 1} {"E4" 1} {NIL 1} {"G4" 2}} 0 240)
=> NIL
```

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

## spi library

A library, loaded with `(use "spi")`: the sixteen SPI devices.

#### Functions

| Form | What it does |
|---|---|
| `(spi d bytes)` | One transaction: the device is selected, the bytes (a list, or a string's) are sent, and the device is let go. The result is a list of the bytes the device sent back meanwhile, one for each byte sent |
| `(spi-read d n)` | Clocks in *n* bytes, sending `$FF` for each: a list |
| `(spi-mode d mode)` | The clock's mode for the device: 0 (the clock rests low) or 3 (it rests high). `NIL` |

#### Arguments

*d*
: The device, 0 to 15: 0 to 7 are the board's headers J18 to J25, and 8 to 15 the slots' cards.

#### Description

The library works through the files of `/dev/spi` (section 17.4). A device with nothing connected sends back
`$FF`, 255, for every byte.

One program at a time can use a device, and a device that is an SD card the system has started can't be used
this way at all: the error is `busy`.

#### Errors

| Message | Cause |
|---|---|
| `invalid argument` | A mode that is not 0 or 3 |
| `busy` | Another program has the device, or it is a card's |
| *path*`: not found` | A device that is not 0 to 15 |

#### See also

Section 17.4; section 14.2; the `dev` library.

#### Example

With nothing connected to device 3:

```session
hylang> (use "spi")
=> NIL
hylang> (spi-mode 3 0)
=> NIL
hylang> (spi 3 {1 2 3})
=> {255 255 255}
hylang> (spi-read 3 4)
=> {255 255 255 255}
```

## split, zip, flatten, distinct, avg

Functions of the library, written in hylang: five more ways to work on lists.

#### Syntax

```text
(split n l)
(zip a b)
(flatten l)
(distinct l)
(avg l)
```

#### Arguments

*l*, *a*, *b*
: Lists.

*n*
: A count, 0 or more.

#### Result

| Function | Result |
|---|---|
| `split` | A list of two lists: the first *n* items of *l*, and the rest. It is `(list (take n l) (drop n l))` |
| `zip` | A list of pairs: the first items of *a* and *b* together, then the second, and so on. It is as long as the shorter of the two |
| `flatten` | One list of all the items, with every list inside *l* opened, however deep |
| `distinct` | The list without its repeats: each value appears once, where it first appeared |
| `avg` | The average of a list of numbers: their sum over their count, exactly |

#### Description

These are written in hylang, and call themselves once for each item, so they are for lists of modest
length: tens or hundreds of items, not thousands.

`zip` is how two lists are walked together: `(map f (zip a b))` calls *f* with a pair from each place.

#### Errors

| Message | Cause |
|---|---|
| `Division by zero.` | `avg` of an empty list |

#### See also

`take`, `drop`, `join`, `map`, `sum`, `sort`.

#### Example

```session
hylang> (split 2 {1 2 3 4 5})
=> {{1 2} {3 4 5}}
hylang> (zip {1 2 3} {"a" "b" "c"})
=> {{1 "a"} {2 "b"} {3 "c"}}
hylang> (flatten {1 {2 {3 4}} 5})
=> {1 2 3 4 5}
hylang> (distinct {1 2 1 3 2})
=> {1 2 3}
hylang> (avg {1 2 3 4})
=> 5/2
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

## square, cube, recip, inverse

Functions of the library, written in hylang: a number's square, its cube, and its reciprocal.

#### Syntax

```text
(square n)
(cube n)
(recip n)
(inverse n)
```

#### Arguments

*n*
: A number.

#### Result

`square` gives *n* × *n*, `cube` gives *n* × *n* × *n*, and `recip` gives 1 / *n*, each exactly. `inverse` is
another name for `recip`.

#### Errors

| Message | Cause |
|---|---|
| `Division by zero.` | `recip` of 0 |
| `All parameters to operator '*' must be numbers.` | `square` or `cube` of something that is not a number |

#### See also

`pow`, `sqrt`, `/`, `fdiv`.

#### Example

```session
hylang> (square 5)
=> 25
hylang> (square 1/2)
=> 1/4
hylang> (cube 3)
=> 27
hylang> (recip 4)
=> 1/4
```

## starts-with?, ends-with?, contains?

Functions of the library, written in hylang: is a piece of text at the start of a string, at its end, or
anywhere in it?

#### Syntax

```text
(starts-with? s sub)
(ends-with? s sub)
(contains? s sub)
```

#### Arguments

*s*
: A string.

*sub*
: The text to look for: a string.

#### Result

`T` or `NIL`. An empty *sub* is found everywhere: all three give `T` for it.

#### Description

`starts-with?` and `contains?` are `index-of` with its answer tested, so capitals and small letters are
different, and a character may be given in place of a one-character string (`ends-with?` needs a string).
`contains?` also takes a list for *s*, and then says
whether *sub* is one of its items, as `elem?` does with its arguments the other way round.

#### Errors

| Message | Cause |
|---|---|
| `'index-of' expects a String and a String` | *s* is a string, and *sub* is neither a string nor a character; or *s* is not a string or a list |

#### See also

`index-of`, `substring`, `elem?`.

#### Example

```session
hylang> (starts-with? "hello" "he")
=> T
hylang> (ends-with? "hello" "lo")
=> T
hylang> (contains? "hello" "ell")
=> T
hylang> (contains? "hello" "z")
=> NIL
hylang> (filter (fn {f} {ends-with? f ".hl"}) {"a.hl" "b.txt" "c.hl"})
=> {"a.hl" "c.hl"}
```

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

## sys

A function: makes a system call. Each call also has a function of its own, named `sys-` and the call's
name.

#### Syntax

```text
(sys name x ...)
(sys-name x ...)
```

#### Arguments

*name*
: The call's name in small letters, with `-` where the call has `_`: an atom, a symbol or a string. `OPEN` is
  `:open`, and `SLEEP_UNTIL` is `:sleep-until`.

*x*
: The call's inputs, in the order its entry in Chapter 3 of this book gives them. A string is passed as the address of
  its text.

#### Result

The call's output: a number, or a list of them if the call gives several. A call with no output gives `NIL`.
If the call fails, the result is an error, with the system's message, and its code as the error's code.

#### Description

`(sys :getpid)` and `(sys-getpid)` are the same call, written two ways. The first form takes the name as a
value, so it can be chosen as the program runs; the second reads like any other function.

These reach everything the kernel offers, at the level a program in assembly language works at: file
descriptors are numbers, and nothing is closed for you. Most programs never need them, because the
functions of this chapter (`open`, `read-line`, `spawn`, `sleep` and the rest) are built on the same calls
and look after the details. They are for the calls that have no function of their own.

"From the languages", in each call's entry in Chapter 3 of this book, says whether a call can be made this way.

#### Errors

| Message | Cause |
|---|---|
| `'sys' has no call :bogus` | There is no call of that name |
| `'sys-open' takes at least 1 argument, not 0` | Too few inputs for the call |
| The system's message | The call failed |

#### See also

Chapter 3 of this book; `error-code`, `errstr`.

#### Example

```session
hylang> (sys-getpid)
=> 2
hylang> (sys :getppid)
=> 1
hylang> (def {fd} (sys-open "/rom/lib/shell" 0))
=> NIL
hylang> fd
=> 3
hylang> (sys-close fd)
=> NIL
hylang> (error-code (sys-open "/nothere" 0))
=> :noent
```

## sysinfo, mods, ps, task-info, errstr, free

Functions: facts about the system: the machine, the ROM's modules, the tasks, an error's message, and the
memory.

#### Syntax

```text
(sysinfo)
(mods)
(ps)
(task-info task)
(errstr code)
(free)
```

#### Arguments

*task*
: A task's number, 0 to 15.

*code*
: A system error code, as a number, 0 to 255.

#### Result

| Function | Result |
|---|---|
| `sysinfo` | A hash: `:abi`, the version of the system's calls; `:ram-modules`, how many RAM modules are fitted; `:free-tasks`, a list of the task numbers not in use |
| `mods` | A list of hashes, one for each module in the paged ROM: `:name`, `:type` (`:program`, `:driver` or `:library`), `:bank` (its first ROM bank) and `:banks` (how many it has) |
| `ps` | A list of hashes, one for each task in use, as `task-info` gives |
| `task-info` | A hash for one task: `:task`, its number; `:name`; `:state`; `:parent`, the task that started it (no entry if none did); `:cpu`, the processor time it has had, in ticks; `:group`, its note group; `:args`, its arguments, a list of strings (no entry if it has none) |
| `errstr` | The system's message for the error code, a string |
| `free` | A hash of memory sizes, in kilobytes: `:ram`, this task's own RAM banks; `:shared`, the shared RAM that segments can use; `:used` and `:free`, how much of that is and isn't in segments; `:segments`, how many segments there are |

A task's `:state` is one of `:ready`, `:wait` (for a child to end), `:call` (in a driver's call), `:idle` (a
driver with nothing to do), `:new`, `:sleep`, `:blocked` and `:event`.

#### Description

These give, as values a program can use, what the commands `mods`, `ps` and `free` print. Being lists of
hashes, they are searched and sorted with the list functions.

#### Errors

| Message | Cause |
|---|---|
| *task*`: no such task` | `task-info` of a task number that isn't in use |
| `'task-info' was given a number out of its range` | *task* is not 0 to 15 |
| `'errstr' expects an error's code` | *code* is not 0 to 255 |

#### See also

`pid`, `banks`, `error-code`; the commands `ps`, `mods` and `free` (Chapter 2 of this book).

#### Example

```session
hylang> ((sysinfo) :ram-modules)
=> 3
hylang> (errstr 32)
=> "not found"
hylang> ((task-info 0) :name)
=> "kernel"
hylang> (map (fn {p} {p :name}) (ps))
=> {"kernel" "forth" "hylang" "snd" "kdev" "storage" "cons"}
hylang> (map (fn {m} {m :name}) (filter (fn {m} {== (m :type) :driver}) (mods)))
=> {"cons" "storage" "kdev" "snd" "vid"}
hylang> (free)
=> <hash>{{:ram 384} {:shared 1024} {:used 512} {:free 512} {:segments 1}}
```

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

Functions: the system's tick count, how many ticks there are in a second, and the time since hylang started.

#### Syntax

```text
(ticks)
(tick-rate)
(clock)
```

#### Result

`ticks` gives the system's tick count: a number from 0 to 32,767, which goes up by one at each tick of the
system's timer and starts again from 0 when it passes 32,767.

`tick-rate` gives the number of ticks in a second: 200.

`clock` gives the number of seconds since this hylang started, as a decimal, to the nearest tick (a two
hundredth of a second).

#### Description

For timing a piece of a program, take `(clock)` before and after, and subtract.

`ticks` is the raw count, which a few system calls and `sleep-until` use. It wraps every 164 seconds, so the
difference of two readings must be taken with that in mind: `clock` does that work, and is the one to use
for anything longer than a moment.

#### See also

`time`, `sleep`, `sleep-until`.

#### Example

The numbers depend on the moment:

```text
hylang> (tick-rate)
=> 200
hylang> (def {t0} (clock))
=> NIL
hylang> (fib 5000)
=> 3878968454388325633701916308325905312082127714646245106160597214895550139044037097010822916462210669
...
hylang> (- (clock) t0)
=> 1.385
```

## time, date, date-parts, seconds-of

Functions: the clock, and conversions between a count of seconds and a date.

#### Syntax

```text
(time)
(date [t])
(date-parts [t])
(seconds-of year month day [hour minute second])
```

#### Arguments

*t*
: A time, as a number of seconds since the start of 1 January 2000. Without it, now.

*year*, *month*, *day*
: A date: the year 2000 or later, the month 1 to 12, the day 1 to the month's last.

*hour*, *minute*, *second*
: A time of day. Without them, midnight.

#### Result

| Function | Result |
|---|---|
| `time` | The clock, now: the number of seconds since 2000-01-01 00:00:00 |
| `date` | The time as a string, `"2026-10-13 09:41:00"`: the form `/dev/time` uses |
| `date-parts` | The time as a hash: `:year`, `:month`, `:day`, `:hour`, `:minute`, `:second`, and `:weekday`, which is 0 for Sunday to 6 for Saturday |
| `seconds-of` | The number of seconds since 2000 of the date and time given |

#### Description

A time is a plain number, so times are compared with `<` and subtracted with `-`: the difference is in
seconds. A file's `:mtime`, from `stat`, is a time of the same kind.

The Hydra's clock starts at the beginning of 2000 each time it is switched on, unless it has a clock chip or
its time has been set (section 17.5): so on a board without one, `(time)` is the number of seconds since it
was started.

#### Errors

| Message | Cause |
|---|---|
| *year*`: invalid argument` | `seconds-of` given a date that doesn't exist, or a year before 2000 |
| `a time must be an integer` | *t* is not a whole number |

#### See also

`ticks`, `clock`, `sleep`, `stat`.

#### Example

```session
hylang> (date 0)
=> "2000-01-01 00:00:00"
hylang> (seconds-of 2026 10 13 9 41 0)
=> 845199660
hylang> (date 845199660)
=> "2026-10-13 09:41:00"
hylang> ((date-parts 845199660) :weekday)
=> 2
hylang> (- (seconds-of 2026 12 25) (seconds-of 2026 10 13))
=> 6307200
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
error's code, an atom: the system's name for the error if it came from the system (`:noent`), or the code the
program gave to `error`. It is `NIL` for an error that has none.

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

A function of the library, written in hylang: loads a library, once.

#### Syntax

```text
(use name)
```

#### Arguments

*name*
: The library's name, a string: the name of a file of hylang source, as `load` takes it. A bare name such as
  `"gpio"` is the file `gpio.hl` in `/lib/hylang`.

#### Result

`NIL`.

#### Description

The first time a name is used, the file is loaded: its expressions are evaluated, which defines the
library's functions. The name is then remembered, in the list `used`, and a later `use` of the same name
does nothing. So every file that needs a library can say so, and the library is still loaded only once.

`load` always loads: use it for a file of your own that you are changing.

The Hydra's libraries, each with an entry of its own in this chapter:

| Name | What it is for |
|---|---|
| `"gpio"`, `"i2c"`, `"spi"` | The I/O header's pins, the I²C bus, the SPI devices |
| `"clock"`, `"disk"`, `"proc"`, `"pc"`, `"dev"` | Setting the clock; the disks; the tasks; the PC's files; devices' control files |
| `"cons"`, `"screen"` | The console's windows; the terminal's cursor and colours |
| `"snd"`, `"video"` | Sound; the Vera X's graphics |
| `"dice"` | Dice, for games |

#### Errors

| Message | Cause |
|---|---|
| *name*`: not found` | There is no such file |

#### See also

`load`, `def`, `fun`.

#### Example

```session
hylang> (use "dice")
=> NIL
hylang> (use "dice")
=> NIL
hylang> used
=> {"dice"}
hylang> (use "nothere")
=> Error: nothere: not found
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

## video library

A library, loaded with `(use "video")`: drawing on a Vera X's screen, turtle graphics, and the chip's memory,
sprites and mouse.

#### Functions

**The bitmap, and drawing on it.**

| Form | What it does |
|---|---|
| `(bitmap w d)` | Turns on a bitmap under the console's text: *w* is 320 or 640 dots across, and *d* the bits for each dot, 1, 2, 4 or 8 (640 across allows only 1 or 2) |
| `(bitmap-off)` | Turns it off |
| `(pen c)` | The colour drawn in from now on: a palette number |
| `(plot x y)` | One dot |
| `(line x0 y0 x1 y1)` | A line |
| `(box x0 y0 x1 y1)`, `(bar x0 y0 x1 y1)` | A rectangle's outline; a filled rectangle |
| `(circle x y r)`, `(disc x y r)` | A circle's outline; a filled circle |
| `(text x y s)` | Text in the console's 8 × 8 font, 40 characters at most |
| `(clear)` | Clears the bitmap |
| `(border c)` | The colour of the screen's border |

**The turtle.**

| Form | What it does |
|---|---|
| `(cs)`, `(home)` | Clears the bitmap and sends the turtle home; sends it home only: the middle of the screen, facing up |
| `(fd n)`, `(bk n)` | Forward, or back, *n* dots, drawing a line if the pen is down |
| `(rt d)`, `(lt d)` | Turns right, or left, *d* degrees |
| `(pu)`, `(pd)` | Pen up (moves draw nothing); pen down |
| `(heading)`, `(seth d)` | The turtle's heading in degrees, 0 being up and 90 right; and set it |

**The chip.**

| Form | What it does |
|---|---|
| `(vpeek addr)`, `(vpoke addr v)` | A byte of the VERA's memory, at a 17-bit address; and set one |
| `(vram@ addr n)`, `(vram! addr bytes)` | *n* bytes of it, as a list; and a list of bytes written to it |
| `(palette! index rgb)` | Sets a palette entry to a colour, `#xRGB`: four bits each of red, green and blue |
| `(sprite! n bytes)` | Sets sprite *n*'s eight attribute bytes |
| `(sprite-at n x y)`, `(sprite-off n)` | Moves a sprite; hides it |
| `(vsync)` | Waits for the next frame |
| `(mouse)`, `(mouse-wait)` | The mouse now: a list of *x*, *y* and the buttons (1 left, 2 middle, 4 right, added together). And the same after waiting for its next change |

#### Description

The library works through the video driver's files in `/dev/vid` (Chapter 16), and needs a Vera X card; the
emulator has one when it is started with `--vera`. Each function gives `NIL` unless it says otherwise.

The bitmap lies under the console's text, so the prompt, and whatever a program prints, stay on the screen
over the drawing. Positions are in dots, from the top left corner; a dot outside the bitmap is not drawn,
and is not an error. The turtle keeps its place exactly, as a fraction, so a figure of many small steps
closes where it began.

The names are the same as those of HyForth's video library, and in both it is the driver that does the
drawing.

#### Errors

| Message | Cause |
|---|---|
| `invalid argument` | Drawing with no bitmap on; a width or depth that `bitmap` doesn't have |
| `busy` | Another program has claimed the chip |
| `/dev/vid/ctl: not found` | There is no Vera X |

#### See also

Chapter 16; section 22.7; the `screen` and `cons` libraries.

#### Example

A square, a filled circle, and a turtle's hexagon:

```text
hylang> (use "video")
=> NIL
hylang> (bitmap 320 8)
=> NIL
hylang> (pen 5)
=> NIL
hylang> (box 10 10 100 100)
=> NIL
hylang> (disc 200 60 30)
=> NIL
hylang> (cs)
=> NIL
hylang> (dotimes {i 6} (do (fd 40) (rt 60)))
=> NIL
hylang> (heading)
=> 0
hylang> (bitmap-off)
=> NIL
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

A function: prints values, with nothing added.

#### Syntax

```text
(write x ...)
```

#### Arguments

*x*
: Any values.

#### Result

`NIL`. The values are written to standard output as `print` writes each, but with nothing between them and
no new line after them.

#### Description

`write` is for building a line a piece at a time, and for prompts that the answer should follow on the same
line. A new line is written when the program asks for one, with `"\n"` in a string or with `(print)`.

#### See also

`print`, `write-to`, `format`.

#### Example

```session
hylang> (write "no new line")
no new line=> NIL
hylang> (write 1 2)
12=> NIL
hylang> (do (write "a" "b") (print))
ab
=> NIL
```

## write-file, append-file

Functions: write values to a file, replacing it or adding to it.

#### Syntax

```text
(write-file path x ...)
(append-file path x ...)
```

#### Arguments

*path*
: The file's name, a string.

*x*
: Any values. Each is written as `print` shows it, with nothing between them and no line end added: put
  `"\n"` where a line should end.

#### Result

`NIL`. `write-file` makes the file, or empties it if it is there, and writes the values. `append-file` makes
it if it isn't there, and writes the values after what it holds. With no values, `write-file` leaves an
empty file.

#### Description

The file is opened, written and closed in the one call. Devices are files, so this is also how a command is
sent to a device's control file (Chapter 12 of the manual).

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | A directory in the path isn't there (and the system's other messages, for its other errors) |

#### See also

`read-file`, `save`, `open`, `print-to`.

#### Example

```session
hylang> (write-file "/ram/b.txt" "x = " 42 "\n")
=> NIL
hylang> (append-file "/ram/b.txt" "y = " {1 2} "\n")
=> NIL
hylang> (read-file "/ram/b.txt")
=> "x = 42\ny = {1 2}\n"
```

## xor, <>, !=

Functions of the library: exclusive or; and two more names for "not equal".

#### Syntax

```text
(xor a b)
(<> a b)
(!= a b)
```

#### Arguments

*a*, *b*
: Any two values.

#### Result

`xor` gives `T` when exactly one of *a* and *b* is true (not `NIL`), and `NIL` when both are or neither is.
Both arguments are always evaluated: it cannot stop early as `and` and `or` do.

`<>` and `!=` are other names for `neq`: `T` when the two values are not equal, as `==` compares.

#### See also

`and`, `or`, `not`, `==`, `bit-xor`.

#### Example

```session
hylang> (xor T NIL)
=> T
hylang> (xor T T)
=> NIL
hylang> (<> 1 2)
=> T
hylang> (!= "a" "a")
=> NIL
```

