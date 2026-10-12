# Chapter 21. Learning hylang

**hylang** is the Hydra's version of **danlang**, a language in the family of **Lisp**, one of the oldest programming
languages there is (it was invented in 1958). In Lisp, a program is made of lists, and the language can work on its
own programs as easily as on any other list. hylang has the Hydra's whole number system, strings, lists and hashes,
and functions for the Hydra's files, devices and programs. This chapter teaches you the language.

> **IN THIS CHAPTER:** You'll learn hylang from the beginning: expressions in parentheses, values, names, functions,
> lists, decisions, loops, strings and hashes. **You need:** Chapter 6. You don't need to have programmed before.

## 21.1 Starting hylang

Type `hylang`. It says hello, and shows its prompt, `hylang>`. Type an **expression**, and hylang works out its
value, and shows it after `=>`:

```session
/> hylang
hylang (danlang on the Hydra-16)
Type 'exit' to Exit

hylang> (+ 1 2)
=> 3
```

`(exit)` leaves hylang. (`(exit 3)` leaves it with the status 3, for a shell to see.)

## 21.2 Expressions

In hylang, a **call** is written in parentheses: the function first, then its **arguments**, separated by spaces. So
`(+ 1 2)` calls the function `+` with the arguments 1 and 2. Calls can be **nested**: each argument can be a call of
its own, which is worked out first:

```session
hylang> (* 2 (+ 3 4))
=> 14
hylang> (/ 10 4)
=> 5/2
```

Because the function always comes first, there's never any doubt about the order things are done in, and `+` can
take as many arguments as you like: `(+ 1 2 3 4)` is 10. hylang has all of Chapter 18's numbers, so `(/ 10 4)` is
exactly 5/2.

## 21.3 Values

Every expression has a **value**. The kinds of value are:

| Kind | Looks like | `type-of` says |
|---|---|---|
| Numbers | `42`, `-3`, `2/3`, `3.14`, `1+2i`, `#xFF` | `:number` |
| Strings | `"Hello"` | `:string` |
| Characters | `\a`, `\space`, `\newline` | `:char` |
| Atoms | `:name`, `:red` | `:atom` |
| Symbols | `x`, `square`, `+` | `:symbol` |
| Lists | `{1 2 3}` | `:list` |
| Hashes | `#({{:name "Sam"}})` | `:hash` |
| Functions | `square`, `(fn {x} {* x x})` | `:function` |
| True | `T` | `:t` |
| False, and nothing | `NIL` | |
| Errors | `(error "oops")` | `:error` |

A **string** is text in double quotes. Inside it, `\n` is a new line, `\t` a tab, `\"` a double quote and `\\` a
backslash. A **here string**, in three double quotes, `"""` ... `"""`, can hold anything, quotes and new lines too.

An **atom** is a name that stands for itself: `:red` is just `:red`. Atoms are used for names of things, like the
keys of a hash.

`T` means true and `NIL` means false: every test gives one or the other, and anything that isn't `NIL` counts as true.
`NIL` also means "nothing", or an empty list.

## 21.4 Names

`def` gives a value a **name**. The name is written in braces (section 21.6 says why):

```session
hylang> (def {x} 10)
=> NIL
hylang> x
=> 10
hylang> (* x x)
=> 100
```

`def` makes **global** names, which can be used anywhere. A **prefix** is a shorter way to write some calls: a
character right against a parenthesis. `:(y 20)` means the same as `(def {y} 20)`:

<!-- session
(def {x} 10)
-->

```session
hylang> :(y 20)
=> NIL
hylang> (+ x y)
=> 30
```

| Prefix | Means | Example |
|---|---|---|
| `:(`*name value*`)` | `(def {`*name*`}` *value*`)` | `:(lives 3)` |
| `=(`*name value*`)` | `(set {`*name*`}` *value*`)`: a name in this function only | `=(total 0)` |
| `?(`*test then else*`)` | `(if` *test then else*`)` | `?((> x 5) "big" "small")` |
| `@(`*formals body*`)` | `(fn` *formals body*`)`: a function | `@({n} {* n 2})` |
| `#(`*entries*`)` | `(hash-create` *entries*`)`: a hash | `#({{:a 1}})` |
| `.(`*f list*`)` | `(unpack` *f list*`)`: *f* called with the list's items | `.(+ {1 2 3})` |
| `~(`*text values*`)` | `(format` *text values*`)` | `~("{} and {}" 1 2)` |
| `[`*a b*`]` | `(list` *a b*`)` | `[1 (+ 1 1) 3]` |
| `$`*name* | `(env "`*name*`")`: an environment variable | `$path` |

Names are in small letters: hylang turns capitals into small letters as it reads them.

## 21.5 Functions

`fun` makes a new function. Its first argument is a list of the function's name and its **formals** (the names its
arguments will have); the second is its **body**: what it does.

```session
hylang> (fun {square n} {* n n})
=> NIL
hylang> (square 7)
=> 49
```

A function's value is the value of its body. Functions can use other functions:

```session
hylang> (fun {greet name} {print (+ "Hello, " name "!")})
=> NIL
hylang> (greet "Sam")
Hello, Sam!
=> NIL
```

`print` prints its arguments, with spaces between them, and starts a new line; its value is `NIL`. `+` joins strings,
if its first argument is a string.

A function doesn't need a name. `fn` makes one without, which can be given a name later, or used straight away:

<!-- session
(fun {square n} {* n n})
-->

```session
hylang> (def {cube} (fn {n} {* n (square n)}))
=> NIL
hylang> (cube 3)
=> 27
hylang> (@({n} {* n 2}) 21)
=> 42
```

**Any number of arguments.** In a function, `&_` is a list of any arguments it was given beyond its formals, so a
function can take as many as you like:

```session
hylang> (fun {add-all} {sum &_})
=> NIL
hylang> (add-all 1 2 3)
=> 6
```

## 21.6 Lists, and code as lists

A list in braces, `{1 2 3}`, is a **Q-expression**: a list that's left as it is, not worked out. A list in
parentheses is an **S-expression**: a call, which is worked out. That's why names and bodies are written in braces:
`{* n n}` is kept as it is, to be run later, when the function is called.

```session
hylang> {1 2 3}
=> {1 2 3}
hylang> (head {1 2 3})
=> {1}
hylang> (tail {1 2 3})
=> {2 3}
```

`list`, or square brackets, makes a list of values that *are* worked out:

```session
hylang> [1 (+ 1 1) 3]
=> {1 2 3}
```

And `eval` runs a Q-expression as code:

```session
hylang> (eval {+ 1 2})
=> 3
```

So a hylang program can build code as a list, and then run it. Lisp programmers say "code is data".

| Function | What it gives |
|---|---|
| `(len l)` | The number of items |
| `(fst l)`, `(snd l)`, `(thd l)`, `(last l)` | The first, second, third, last item |
| `(nth n l)` | Item *n*, counting from 0 |
| `(head l)`, `(tail l)` | A list of the first item; all but the first |
| `(cons x l)` | *l* with *x* in front |
| `(join l1 l2 ...)` | The lists joined |
| `(take n l)`, `(drop n l)` | The first *n* items; all but them |
| `(reverse l)` | Backwards |
| `(range n)`, `(range a b [step])` | The numbers 0 to *n* - 1, or *a* up to (not including) *b* |
| `(sort l)` | In order |
| `(elem? x l)` | Is *x* in the list? |

## 21.7 Doing something to every item

Lisp's great strength is working on whole lists at once. `map` calls a function for each item, and makes a list of
the answers; `filter` keeps the items a test is true for; and `foldl` combines them all, from the left:

<!-- session
(fun {square n} {* n n})
-->

```session
hylang> (map square {1 2 3 4})
=> {1 4 9 16}
hylang> (filter (fn {n} {> n 2}) {1 2 3 4})
=> {3 4}
hylang> (foldl + 0 {1 2 3 4})
=> 10
hylang> (sum (range 101))
=> 5050
```

That last one adds up the numbers from 0 to 100, which the mathematician Gauss is said to have done in his head when
he was a schoolboy.

## 21.8 Decisions

`if` takes a test, a value for when it's true, and (if you like) one for when it's false:

<!-- session
(def {x} 10)
-->

```session
hylang> (if (> x 5) "big" "small")
=> "big"
hylang> ?((> x 50) "big" "small")
=> "small"
```

The comparisons are `==` (equal), `!=` (not equal), `<`, `>`, `<=` and `>=`, and `and`, `or` and `not` combine
tests. `<=>` chooses by whether a number is below, equal to, or above zero:

```session
hylang> (<=> (cmp 3 7) {< "less"} {= "same"} {> "more"})
=> "less"
```

## 21.9 Loops

| Loop | What it does |
|---|---|
| `(dotimes {i n} body ...)` | Runs the body *n* times, with *i* 0, 1 ... *n* - 1 |
| `(each {x list} body ...)` | Runs the body for each item of a list (or each character of a string) |
| `(while test body ...)` | Runs the body as long as the test is true |
| `(do x y ...)` | Works out each expression in turn; its value is the last one's |

```session
hylang> (each {i (range 3)} (print "item" i))
item 0
item 1
item 2
=> NIL
hylang> (dotimes {i 3} (print i))
0
1
2
=> NIL
```

But a Lisp programmer often uses **recursion** instead: a function that calls itself.

```session
hylang> (fun {fact n} {if (< n 2) 1 (* n (fact (- n 1)))})
=> NIL
hylang> (fact 20)
=> 2432902008176640000
```

## 21.10 Names inside functions

A function's formals are its own: they don't change any global name. For more names of its own, a function uses `=(`
(that's `set`): a name in the function's own **scope**, which is gone when it ends. `let` makes a scope of its own
too:

```session
hylang> (let {{a 1} {b 2}} (+ a b))
=> 3
```

Loops make a new scope each time round, so to change a name from inside a loop, use `set!`, which changes the name
where it was made:

```session
hylang> (fun {counter} {do =(c 0) (dotimes {i 5} (set! {c} (+ c i))) c})
=> NIL
hylang> (counter)
=> 10
```

## 21.11 Strings and characters

| Function | What it does |
|---|---|
| `(+ s ...)` | Joins strings (and anything else, as `print` shows it) |
| `(len s)` | Its length |
| `(substring s i [n])` | *n* characters from position *i* (counting from 0) |
| `(char-at s i)` | The character at *i* |
| `(index-of s sub)` | Where *sub* first appears (or `NIL`) |
| `(str-upper s)`, `(str-lower s)`, `(str-trim s)` | Capitals, small letters, spaces trimmed off |
| `(str-split s sep)`, `(str-join l sep)` | A string to a list, and back |
| `(str-replace s old new)` | Every *old* replaced by *new* |
| `(format text x ...)` | The text, with each `{}` replaced by the next value |
| `(to-str x)`, `(val s)` | A value as a string; the number a string holds |

```session
hylang> (str-split "a,b,c" ",")
=> {"a" "b" "c"}
hylang> (substring "Hydra-16" 0 5)
=> "Hydra"
hylang> (format "{} plus {} is {}" 2 3 (+ 2 3))
=> "2 plus 3 is 5"
```

## 21.12 Hashes

A **hash** is a set of **entries**, each a **key** and its **value**, like a dictionary: look up a word, and find its
meaning. The keys are usually atoms. `#(` makes one, from a list of entries, and the hash can be called like a
function, with a key, to look the key up:

```session
hylang> (def {h} #({{:name "Sam"} {:age 12}}))
=> NIL
hylang> (h :name)
=> "Sam"
hylang> (hash-keys h)
=> {:name :age}
hylang> (hash-put h {:city "Leeds"})
=> NIL
hylang> h
=> <hash>{{:name "Sam"} {:age 12} {:city "Leeds"}}
```

A value in a hash can be a function: a **method**. Calling the hash with its key calls it, and inside it, `&0` is the
hash itself. So hashes can be **objects**, with data and methods together:

```session
hylang> (def {dog} #({{:name "Rex"} {:speak (fn {} {print (+ (&0 :name) " says woof")})}}))
=> NIL
hylang> (dog :speak)
Rex says woof
=> NIL
```

## 21.13 Errors

An error is a value too. When something goes wrong, the error goes back through every call, until something catches
it, or it reaches the prompt:

```session
hylang> (error "my own error")
=> Error: my own error
hylang> (try (/ 1 0) (+ "oops: " &err))
=> "oops: Division by zero."
```

`try` works out its first argument; if that's an error, it works out its second instead, with `&err` the error's
message (and `&code` its code). `(error "message")` makes an error of your own.

## 21.14 Files of hylang

Write longer programs in a file (with `edit`), with a name ending in `.hl`, and run it with `hylang` and the file's
name, and any arguments: in the program, `args` is a list of the file's name and its arguments.

```session
/> echo '(print "args are" args)' >/ram/a.hl
/> hylang /ram/a.hl one two
args are {"/ram/a.hl" "one" "two"}
```

At the prompt, `(load "file")` runs a file's expressions, keeping its definitions. A file's first line can be
`#!/bin/hylang`, to run it by its name. Comments start with `;`.

Chapter 22 shows how to use hylang to work the Hydra, and Appendix B lists every function.
