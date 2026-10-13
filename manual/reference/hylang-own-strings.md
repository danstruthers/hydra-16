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
