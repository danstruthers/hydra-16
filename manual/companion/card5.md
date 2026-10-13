# Card 5. hylang

Chapter 21 teaches it, and Appendix B lists every function. Start it with `hylang`; leave with `(exit)`; run a file
with `hylang file.hl`.

### The shapes of things

| Written | Is |
|---|---|
| `(f a b)` | A call: work out `a` and `b`, then call `f` with them |
| `{a b c}` | A list, left as it is: data, or code to run later |
| `[a b c]` | A list of the *values* of `a`, `b` and `c` |
| `42`, `2/3`, `1.5`, `#xFF` | Numbers: whole, fractions, decimals, in other bases |
| `"text"` | A string (`\n` inside is a new line) |
| `:name` | An atom: a name that stands for itself |
| `T`, `NIL` | True; false (and "nothing", and the empty list) |
| `; text` | A comment |

### Names and functions

```text
(def {x} 10)                       ; a global name          short: :(x 10)
(fun {square n} {* n n})           ; a named function
(fn {n} {* n 2})                   ; a function with no name  short: @({n} {* n 2})
(let {{a 1} {b 2}} (+ a b))        ; names for one expression
(do =(a 5) (print a) (* a a))      ; several things in turn: =( makes a name of this function's own
(set! {x} 11)                      ; change a name where it was made
```

### Numbers and tests

| Functions | What they give |
|---|---|
| `(+ a b ...)` `(- a b)` `(* a b ...)` `(/ a b)` | Exact answers: `(/ 10 4)` is `5/2` |
| `(mod a b)`, `(div a b)` | The remainder; the whole part |
| `(sqrt x)`, `(pow x y)`, `(abs x)` | |
| `(to-fixed x 2)`, `(truncate x)`, `(round x)` | A decimal to 2 places; the whole part; the nearest whole number |
| `(random n)` | A whole number from 0 to *n* − 1, by chance |
| `(== a b)` `(!= a b)` `(< a b)` `(> a b)` `(<= a b)` `(>= a b)` | `T` or `NIL` |
| `(and a b)` `(or a b)` `(not a)` | |

### Decisions and loops

```text
(if (> x 5) "big" "small")
(cond {(< x 0) "negative"} {(== x 0) "zero"} {T "positive"})
(dotimes {i 3} (print i))                    ; 0 1 2
(each {x {1 2 3}} (print x))
(while (< x 20) (set! {x} (+ x 1)))
(fun {fact n} {if (< n 2) 1 (* n (fact (- n 1)))})    ; a function that calls itself
```

### Lists

| Functions | What they give |
|---|---|
| `(len l)`, `(fst l)`, `(last l)`, `(nth 2 l)` | How many; the first; the last; item 2, counting from 0 |
| `(head l)`, `(tail l)`, `(cons x l)`, `(join l m)` | A list of the first; the rest; *x* in front; two joined |
| `(range 5)`, `(range 1 11)` | `{0 1 2 3 4}`; `{1 2 ... 10}` |
| `(map f l)`, `(filter f l)` | *f* of every item; the items *f* is true of |
| `(sum l)`, `(sort l)`, `(reverse l)`, `(elem? x l)` | |

### Strings, hashes, printing

| Functions | What they do |
|---|---|
| `(+ "a" "b" 3)` | Join: `"ab3"` (a string must come first) |
| `(len s)`, `(substring s 0 5)`, `(str-upper s)` | |
| `(str-split s " ")`, `(str-join l " ")` | A string to a list of words, and back |
| `(format "{} is {}" a b)` | Fill each `{}` with the next value |
| `(val s)`, `(to-str x)` | A string to a number; anything to a string |
| `(def {h} #({{:name "Rex"} {:legs 4}}))` | A hash: values under names |
| `(h :name)`, `(hash-put h {:legs 3})`, `(hash-keys h)` | Look up; change; the names |
| `(print a b)`, `(write a b)` | Print with a new line; without |
| `(read-line)` | A line typed at the keyboard, as a string |

### When it goes wrong (section 21.13)

| You see | It means |
|---|---|
| `Unbound Symbol 'x'` | A name hylang doesn't know: a spelling slip? |
| A prompt of spaces | A `(`, `{` or `"` isn't closed yet |
| `<function>` for an answer | Too few arguments |
| `Unbound Symbol 'n'`, defining a function | The body is in `( )`: it should be in `{ }` |
