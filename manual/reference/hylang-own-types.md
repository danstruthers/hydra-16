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
