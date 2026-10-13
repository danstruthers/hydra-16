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

## newns

A special form: rebuilds the namespace.

#### Syntax

```text
(newns)
```

#### Description

hylang's namespace is thrown away and the default one built again from `/lib/namespace`, as a login shell
does when it starts and as rc's `newns` does. Every `bind` and `mount` the program has made is undone.

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
