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
