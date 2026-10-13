# Lab 19. hylang: Lists, Decisions and Programs

> **THIS LAB:** Manual: Chapter 21, sections 21.6 to 21.16. About an hour. You need a Hydra, and Lab 18 done.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. A list is values in braces. ✎ What does each of these give?

| The expression | Its value |
|---|---|
| `(def {l} {5 3 8 1})` | |
| `(len l)` | |
| `(fst l)` | |
| `(tail l)` | |
| `(sort l)` | |
| `(reverse l)` | |
| `(sum l)` | |
| `(range 5)` | |
| `(range 1 11)` | |

2. `map` does something to every item, and `filter` keeps some of them. ✎ What does each give?

| The expression | Its value |
|---|---|
| `(map (fn {n} {* n 2}) {1 2 3})` | |
| `(filter (fn {n} {> n 2}) {1 2 3 4})` | |
| `(sum (map (fn {n} {* n n}) (range 1 11)))` | |

3. ✎ In one expression each: the sum of the numbers from 1 to 100; and the even numbers from 2 to 20, as a list.

<div class="write l2"></div>

4. A decision: `if` takes a test, then a value for true and a value for false.

```text
hylang> (fun {size n} {if (> n 100) "big" "small"})
hylang> (size 5)
hylang> (size 500)
hylang> (map size {5 500 50 5000})
```

   ✎ What did the last line give?

<div class="write"></div>

5. A loop, and a function that calls itself. ✎ What does each print or give?

| The expression | What it does |
|---|---|
| `(dotimes {i 3} (print "Hip!"))` | |
| `(each {x {1 2 3}} (print (* x x)))` | |
| `(fun {fact n} {if (< n 2) 1 (* n (fact (- n 1)))})`, then `(fact 5)` | |
| `(fact 30)` | |

6. A hash keeps values under names:

```text
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
hylang> (pet :name)
hylang> (hash-put pet {:colour "brown"})
hylang> (hash-keys pet)
```

   ✎ What did the last line give? What kind of thing is `:name`?

<div class="write l2"></div>

7. Put a program in a file, `/sd/0/work/lab19.hl`, with `edit`: two functions from this lab, and a line that
   prints something with them. Run it from the shell with `hylang /sd/0/work/lab19.hl`. ✎ Write your program here.

<div class="write l6"></div>

### Questions

1. ✎ `{+ 1 2}` and `(+ 1 2)` look alike. What's the difference? What does `(eval {+ 1 2})` do?

<div class="write l2"></div>

2. ✎ HyForth can't work out `(fact 30)`. Why can hylang?

<div class="write l2"></div>

3. ✎ `fact` calls itself. Why doesn't it go on for ever?

<div class="write l2"></div>

4. ✎ What does `map` save you from writing?

<div class="write"></div>

5. ✎ In FizzBuzz (section 21.16), the function `fizz` *gives* a value, and doesn't print anything. Why is that the
   better way?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | make a list, and take it apart |
| ☐ | use `map` and `filter` with a function of my own |
| ☐ | choose with `if` and `cond` |
| ☐ | write a function that calls itself, and say why it stops |
| ☐ | keep a program in a file, and run it |

> **CHALLENGE:** ★ Write `(primes n)`, which gives a list of the prime numbers up to *n*. One way: write
> `(prime? n)` first, using `any?` or `filter` over `(range 2 n)`, and then `filter` the numbers from 2 to *n* with
> it. How long does `(primes 100)` take? Compare with your HyForth version from Lab 17.
