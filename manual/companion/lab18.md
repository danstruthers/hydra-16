# Lab 18. hylang: Expressions and Functions

> **THIS LAB:** Manual: Chapter 21, sections 21.1 to 21.5. About an hour. You need a Hydra. Type `hylang` to start
> it, and `(exit)` to leave.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. First, guess. Then ask hylang.

| The expression | My guess | hylang says |
|---|---|---|
| `(+ 1 2 3 4)` | | |
| `(* 2 (+ 3 4))` | | |
| `(- 10 (* 2 3))` | | |
| `(/ 10 4)` | | |
| `(> 5 3)` | | |
| `(> 3 5)` | | |
| `(+ "Hydra" "-" 16)` | | |

2. Turn these sums into hylang. ✎ Write the expression, and its value.

| The sum | In hylang | Its value |
|---|---|---|
| (4 + 5) × 2 | | |
| 100 − (7 × 8) | | |
| (12 + 4) ÷ (9 − 5) | | |
| 1/2 + 1/3 + 1/6 | | |

3. Give a value a name, and use it:

```text
hylang> (def {age} 12)
hylang> (* age 365)
hylang> (def {name} "Sam")
hylang> (print "Hello," name)
```

   ✎ Why is the name in braces in `def`, but not when you use it?

<div class="write l2"></div>

4. Make a function, and use it:

```text
hylang> (fun {square n} {* n n})
hylang> (square 9)
hylang> (square (square 3))
```

   ✎ In `(fun {square n} {* n n})`, which part is the function's name? Its formal? Its body?

<div class="write l2"></div>

5. ✎ Now write your own. Test each with at least two values, and write the definition here.

| The function | What it gives | My definition |
|---|---|---|
| `(double n)` | Twice *n* | |
| `(cube n)` | *n* × *n* × *n* | |
| `(average a b)` | The number half way between | |
| `(greet who)` | Prints "Hello, " and the name | |

6. Make these mistakes on purpose. ✎ Write what hylang says.

| I typed | hylang said |
|---|---|
| `(squre 5)` | |
| `(square 1 2)` | |
| `(square)` | |
| `(+ 1 "a")` | |

7. Type `(+ 1 2` and press <kbd>RETURN</kbd>, without the last parenthesis. ✎ What happens? How do you finish?

<div class="write"></div>

### Questions

1. ✎ In hylang, the function comes first: `(+ 2 3)`. What can `+` do in hylang that it can't do in school
   arithmetic, because of that?

<div class="write l2"></div>

2. ✎ What do `T` and `NIL` mean?

<div class="write"></div>

3. ✎ What's the difference between `{* n n}` and `(* n n)`? When is each one worked out?

<div class="write l2"></div>

4. ✎ `(/ 10 4)` gives `5/2`. How would you get 2.5? How would you get just 2? (Appendix B.4.)

<div class="write l2"></div>

5. ✎ `(square)`, with no argument, gave you a function back. What might that be useful for?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | write a nested expression, and say what it's worth |
| ☐ | name a value with `def` |
| ☐ | define a function with `fun`, and call it |
| ☐ | read hylang's commonest error messages |
| ☐ | balance my parentheses |

> **CHALLENGE:** ★ Write `(celsius f)`, which turns Fahrenheit into Celsius: take away 32, multiply by 5, divide by
> 9. What does `(celsius 98)` give? Compare that with what HyForth gave you in Lab 16. Then write `(hypotenuse a b)`,
> using `sqrt`. What's `(hypotenuse 3 4)`? And `(hypotenuse 1 1)`?
