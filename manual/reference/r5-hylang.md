# Chapter 5. hylang Forms

An expression in parentheses is evaluated: its first element names a function or a special form, and the rest
are its arguments. A function's arguments are evaluated before it is called, left to right. A **special form**
receives its arguments as they were written, and decides for itself which to evaluate and when; each entry for
one says exactly that. An expression in braces, a Q-expression, is data: it is not evaluated until something
runs it.

A name is looked up in the current **scope**, then in the scope that encloses it, and so on out to the global
scope. Each call of a function has a scope of its own, enclosed by the scope the function was made in.

An error is a value: `Error: ` and a message. An expression that is given an error as an argument returns it at
once, so an error passes outward through whatever contains it until something catches it or it reaches the
prompt.

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
