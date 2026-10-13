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

A function: numbers.

#### Syntax

```text
(+ x ...)
(- x ...)
(* x ...)
(/ x ...)
```

#### Description

Sum, difference, product, quotient (exact)

## <, >, <=, >=, cmp

A function: numbers.

#### Syntax

```text
(< x y)
(> x y)
(<= x y)
(>= x y)
(cmp x y)
```

#### Description

Comparisons; -1, 0 or 1.

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

A function: numbers.

#### Syntax

```text
(== x y)
(eq x y)
(neq x y)
```

#### Description

Equal; not equal.

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

A function: numbers.

#### Syntax

```text
(1+ x)
(1- x)
(abs x)
```

#### Description

Plus one; minus one; its size.

## alpha?, digit?, space?, upper?, lower?

A function: strings and characters.

#### Syntax

```text
(alpha? c)
(digit? c)
(space? c)
(upper? c)
(lower? c)
```

#### Description

Its kind.

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

A function: lists.

#### Syntax

```text
(any? f l)
(all? f l)
```

#### Description

Is *f* true of any item? Of every one?

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

A function: numbers.

#### Syntax

```text
(base)
(base b)
```

#### Description

The base; set it (giving the one before)

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

A function: numbers.

#### Syntax

```text
(bit-and n ...)
(bit-or n ...)
(bit-xor n ...)
(bit-not n)
```

#### Description

Bits.

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

A function: numbers.

#### Syntax

```text
(bytes s)
(from-bytes l)
```

#### Description

A string's bytes; a string from bytes.

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

A function: strings and characters.

#### Syntax

```text
(char-code c)
(code-char n)
```

#### Description

A character's code; the character for a code.

## close

A function: files and streams.

#### Syntax

```text
(close s)
```

#### Description

Close it.

## complex

A function: numbers.

#### Syntax

```text
(complex re im)
```

#### Description

A complex number.

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

A function: lists.

#### Syntax

```text
(cons x l)
```

#### Description

*x* in front of *l*.

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

A function: numbers.

#### Syntax

```text
(digits)
(digits n)
```

#### Description

The math functions' precision; set it.

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

A function: lists.

#### Syntax

```text
(elem? x l)
(in? x l)
```

#### Description

Is *x* in *l*?

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

A function: numbers.

#### Syntax

```text
(fib n)
```

#### Description

The *n*th Fibonacci number.

## filter

A function: lists.

#### Syntax

```text
(filter f l)
```

#### Description

The items *f* is true of.

## find, count

A function: lists.

#### Syntax

```text
(find f l)
(count f l)
```

#### Description

The first item *f* is true of; how many.

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

A function: lists.

#### Syntax

```text
(foldl f init l)
(foldr f init l)
```

#### Description

Combine the items, from the left, from the right.

## format

A function: strings and characters.

#### Syntax

```text
(format text x ...)
```

#### Description

The text, each `{}` the next value.

## from#

A function: hashes.

#### Syntax

```text
(from# h)
```

#### Description

A hash as a list of entries.

## fst, snd, thd, last, nth

A function: lists.

#### Syntax

```text
(fst l)
(snd l)
(thd l)
(last l)
(nth n l)
```

#### Description

An item (from 0)

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

A function: strings and characters.

#### Syntax

```text
(gensym [prefix])
```

#### Description

A new symbol, unlike any other.

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

A function: lists.

#### Syntax

```text
(head l)
(tail l)
```

#### Description

A list of the first item; all but the first.

## hex, bin

A function: numbers.

#### Syntax

```text
(hex n [width])
(bin n [width])
```

#### Description

Its digits, in hex or binary.

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

A function: strings and characters.

#### Syntax

```text
(index-of s sub)
(last-index-of s sub)
```

#### Description

Where *sub* first, last, is (or `NIL`)

## init, end

A function: lists.

#### Syntax

```text
(init l)
(end l)
```

#### Description

All but the last; a list of the last.

## item-at

A function: lists.

#### Syntax

```text
(item-at l i)
```

#### Description

Item *i*, as it's written.

## join

A function: lists.

#### Syntax

```text
(join l ...)
```

#### Description

The lists joined.

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

A function: lists.

#### Syntax

```text
(len l)
```

#### Description

Its length (a string's too)

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

A function: lists.

#### Syntax

```text
(list x ...)
```

#### Description

A list of the values.

## lo, hi, word

A function: numbers.

#### Syntax

```text
(lo n)
(hi n)
(word lo hi)
```

#### Description

Its low byte, its next; two bytes made a word.

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

A function: lists.

#### Syntax

```text
(map f l)
```

#### Description

*f* of each item.

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

A function: lists.

#### Syntax

```text
(min x ...)
(max x ...)
```

#### Description

The smallest; the largest.

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

A function: numbers.

#### Syntax

```text
(neg? x)
(pos? x)
(zero? x)
(one? x)
(not x)
```

#### Description

Tests.

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

## number-bytes, bytes-number

A function: numbers.

#### Syntax

```text
(number-bytes n)
(bytes-number b)
```

#### Description

A number as bytes, and back.

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

## random

A function: numbers.

#### Syntax

```text
(random n)
```

#### Description

A random whole number from 0 to *n* - 1.

## range

A function: lists.

#### Syntax

```text
(range to)
(range from to [step])
```

#### Description

Numbers, up to (not including) *to*.

## rational.n, rational.d

A function: numbers.

#### Syntax

```text
(rational.n x)
(rational.d x)
```

#### Description

Its top; its bottom.

## read

A function: strings and characters.

#### Syntax

```text
(read text)
```

#### Description

The expressions in the text, not worked out.

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

A function: lists.

#### Syntax

```text
(reverse l)
```

#### Description

Backwards (a string's too)

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

A function: numbers.

#### Syntax

```text
(shl n k)
(shr n k)
(bit? n k)
```

#### Description

Shifts; a bit.

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

A function: lists.

#### Syntax

```text
(sort l [less])
```

#### Description

In order (by `cmp`, or by *less*)

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

A function: numbers.

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

#### Description

The math functions.

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

A function: strings and characters.

#### Syntax

```text
(str-pad-left s n [c])
(str-pad-right s n [c])
```

#### Description

Padded to *n* characters.

## str-replace, str-repeat

A function: strings and characters.

#### Syntax

```text
(str-replace s old new)
(str-repeat s n)
```

#### Description

Replaced; repeated.

## str-split, str-join, str-chars

A function: strings and characters.

#### Syntax

```text
(str-split s [sep])
(str-join l [sep])
(str-chars s)
```

#### Description

Split; join; its characters.

## str-upper, str-lower, str-trim

A function: strings and characters.

#### Syntax

```text
(str-upper s)
(str-lower s)
(str-trim s)
```

#### Description

Capitals; small letters; trimmed.

## subset

A function: lists.

#### Syntax

```text
(subset l i [n])
```

#### Description

*n* items from *i*.

## substring, char-at

A function: strings and characters.

#### Syntax

```text
(substring s i [n])
(char-at s i)
```

#### Description

Part of a string; a character.

## sum, product

A function: lists.

#### Syntax

```text
(sum l)
(product l)
```

#### Description

All the items added; multiplied.

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

## take, drop

A function: lists.

#### Syntax

```text
(take n l)
(drop n l)
```

#### Description

The first *n*; all but them.

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

A function: numbers.

#### Syntax

```text
(to-fixed x [places])
(to-rational x)
```

#### Description

As a fixed decimal; as a fraction.

## to-str, repr

A function: strings and characters.

#### Syntax

```text
(to-str x [base])
(repr x)
```

#### Description

As `print` shows it; as the prompt shows it.

## to-sym, to-atom

A function: strings and characters.

#### Syntax

```text
(to-sym x)
(to-atom x)
```

#### Description

A symbol; an atom.

## truncate

A function: numbers.

#### Syntax

```text
(truncate x)
```

#### Description

The whole part (toward zero)

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

## use

A function of the library, written in hylang.

#### Syntax

```text
(use "name")
```

#### Description

Load a library once.

## val

A function: numbers.

#### Syntax

```text
(val s)
```

#### Description

The number a string holds.

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

