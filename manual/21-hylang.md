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

`exit`, or `(exit)`, leaves hylang. (`(exit 3)` leaves it with the status 3, for a shell to see.)

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

To turn a sum from school into hylang, find the last thing you'd do, and write that first. In (3 + 4) × (5 − 2),
the last thing is the multiplying, so the `*` goes on the outside, with the two parts as its arguments:

```session
hylang> (* (+ 3 4) (- 5 2))
=> 21
```

Count the parentheses: every `(` needs its `)`. If you press <kbd>RETURN</kbd> before they're all closed, hylang
doesn't complain. It waits for the rest, with a prompt of spaces, until you type the missing `)`.

> **TRY THIS:** Work these out in your head, then ask hylang: `(- 10 (* 2 3))` and `(* (- 10 2) 3)`. Then write
> (8 − 2) × (1 + 1) in hylang. (It should say 12.) What does `(/ 1 3)` give? And `(+ (/ 1 3) (/ 1 6))`?

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
| False, and nothing | `NIL` | `:list` (it's the empty list) |
| Errors | `(error "oops")` | `:error` |

A **string** is text in double quotes. Inside it, `\n` is a new line, `\t` a tab, `\"` a double quote and `\\` a
backslash. A **here string**, in three double quotes, `"""` ... `"""`, can hold anything, quotes and new lines too.

An **atom** is a name that stands for itself: `:red` is just `:red`. Atoms are used for names of things, like the
keys of a hash.

`T` means true and `NIL` means false: every test gives one or the other, and anything that isn't `NIL` counts as true.
`NIL` also means "nothing", or an empty list. hylang always prints these two in capitals, so that they stand out;
you can type them either way.

```session
hylang> (> 5 3)
=> T
hylang> (> 3 5)
=> NIL
```

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

`def` makes **global** names, which can be used anywhere. A name is in small letters (hylang turns capitals into
small letters as it reads them), and it can have digits, and signs like `-` and `?`, in it: `high-score`, `even?`.

`def` is used so often that there's a shorter way to write it: `:(y 20)` means exactly the same as `(def {y} 20)`.
The `:` right against the parenthesis is a **prefix**. There are a few more prefixes, for other things that are
used a lot. You'll meet them as you go, and section 21.15 lists them all.

<!-- session
(def {x} 10)
-->

```session
hylang> :(y 20)
=> NIL
hylang> (+ x y)
=> 30
```

A name that hasn't been given a value is an error. This is the message you'll see most often while you're
learning, usually because of a slip of the finger:

<!-- session
(def {x} 10)
-->

```session
hylang> (+ x z)
=> Error: Unbound Symbol 'z'
```

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

**Too many, and too few.** Give a function more arguments than it has formals, and hylang says so. Give it fewer,
and something more interesting happens: you get back a *new function*, which is waiting for the rest.

<!-- session
(fun {square n} {* n n})
-->

```session
hylang> (square 1 2)
=> Error: The function takes 1 argument, not 2
hylang> (square)
=> <function>(fn {n} {* n n})
```

So if an answer comes back as `<function>` when you expected a number, count your arguments.

> **TRY THIS:** Write `(fun {double n} ...)`, and then `quadruple`, which uses `double` twice. Then write `average`,
> which takes two numbers and gives the number half way between them. `(average 3 4)` should be 7/2.

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
| `(index-of s sub)` | Where *sub* first appears (or -1) |
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

Here are the mistakes everyone makes when they start, and what to do about each:

| What you see | What happened | What to do |
|---|---|---|
| `Error: Unbound Symbol 'squre'` | A name hylang doesn't know: nearly always a slip of the finger | Check the spelling |
| A prompt of spaces | A `(`, a `{` or a `"` hasn't been closed yet | Type what's missing |
| `<function>(fn ...)` where you expected an answer | The function was given too few arguments | Count them |
| `Error: The function takes 1 argument, not 2` | It was given too many | Count them |
| `Error: Unbound Symbol 'n'`, as you define a function | The body is in parentheses, so hylang tried to work it out at once | Put the body in braces: `{* n n}` |
| `Error: '+' adds numbers, or joins values to a string` | `(+ 1 "a")`: a number first, then a string | To join, put a string first: `(+ "" 1 "a")` |
| `Error: 'head' expects a QExpr` | A list was wanted (a "QExpr" is a Q-expression: a list in braces) | Give it a list |
| A changed name that changes back | `=(` made a new name inside a loop or a function | Use `set!` (section 21.10) |

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

## 21.15 Prefixes: writing less

A **prefix** is a character right against an opening parenthesis, which stands for a function that's used a lot.
You've met `:(`, `=(`, `?(`, `@(` and `#(`. Here they all are:

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

```session
hylang> .(+ {1 2 3})
=> 6
hylang> ~("{} and {}" 1 2)
=> "1 and 2"
hylang> $path
=> {"." "/bin"}
```

Prefixes are only a shorter spelling. While you're learning, the long names say more clearly what's going on, and
you can always use them instead.

## 21.16 Whole programs

Here are three small programs that use what this chapter has taught. Type them in, run them, and then change
them.

### A times table

`range` makes the numbers 1 to 10, and `map` multiplies each of them by *n*: a whole row of the table at once,
with no loop in sight. Then `each` prints a row for each *n*:

```session
hylang> (fun {table n} {map (fn {i} {* n i}) (range 1 11)})
=> NIL
hylang> (table 7)
=> {7 14 21 28 35 42 49 56 63 70}
hylang> (each {n (range 1 4)} (print (table n)))
{1 2 3 4 5 6 7 8 9 10}
{2 4 6 8 10 12 14 16 18 20}
{3 6 9 12 15 18 21 24 27 30}
=> NIL
```

`table` doesn't *print* a row: it *gives* one, as a list, which is more useful. You could add its numbers up with
`(sum (table 7))`, or ask for `(nth 3 (table 7))`. In hylang, try to write functions that give values, and leave
the printing to whoever calls them.

### Dice

`(random n)` gives a whole number from 0 to *n* − 1, chosen by chance:

```session
hylang> (fun {die} {+ 1 (random 6)})
=> NIL
hylang> (fun {roll} {do =(a (die)) =(b (die)) (print a b "makes" (+ a b))})
=> NIL
hylang> (roll)
3 4 makes 7
=> NIL
hylang> (roll)
6 6 makes 12
=> NIL
```

(Yours will fall differently.) `roll` has to remember each die, to print it and then to add it, so it gives them
the names `a` and `b`, with `=(`. A function's body is one expression; `do` makes one out of several.

### FizzBuzz, in a file

In the game of FizzBuzz, you count aloud, but say "Fizz" instead of every number that 3 goes into, "Buzz" for
those that 5 goes into, and "FizzBuzz" for both. Go to `/ram`, start `edit fizz.hl`, and type:

<!-- listing samples/fizz.hl -->

```text
; fizz.hl - the game of FizzBuzz: count, but say Fizz for the threes,
; Buzz for the fives, and FizzBuzz for both

(fun {fizz n} {cond
  {(== 0 (mod n 15)) "FizzBuzz"}
  {(== 0 (mod n 3)) "Fizz"}
  {(== 0 (mod n 5)) "Buzz"}
  {T n}})

(fun {fizzbuzz n} {do
  (each {i (range 1 (+ n 1))} (write (fizz i) " "))
  (print)})

(fizzbuzz 15)
```

Save it, leave the editor, and run it:

<!-- session
cd /ram
!file samples/fizz.hl
-->

```session
/ram> hylang fizz.hl
1 2 Fizz 4 Buzz Fizz 7 8 Fizz Buzz 11 Fizz 13 14 FizzBuzz
```

* `cond` chooses between several cases. Each case is a test and a value, in braces; the first test that's true
  wins. The last case's test is `T`, which is always true: it catches everything the others didn't.
* `mod` gives a remainder. If the remainder of *n* divided by 15 is 0, then both 3 and 5 go into *n*. That test has
  to come first. (Why?)
* `fizz` gives a string or a number. It doesn't print anything, so it can be tried alone at the prompt:
  `(load "fizz.hl")`, then `(fizz 9)`.
* `fizzbuzz` does the printing. `write` prints like `print`, but doesn't start a new line; the `(print)` at the
  end does.

> **TRY THIS:** Change `fizz.hl` so that numbers 7 goes into say "Whizz". What should 21 say? And 35? And 105?
> Then, at the `hylang>` prompt, try `(map fizz (range 1 16))`: the whole game as a list.

Chapter 22 shows how to use hylang to work the Hydra, and Appendix B lists every function.
