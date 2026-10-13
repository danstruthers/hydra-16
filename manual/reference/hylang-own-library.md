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
