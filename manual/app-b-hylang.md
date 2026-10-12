# Appendix B. hylang Reference

This appendix lists all of hylang: its syntax, its special forms, its built-in functions, and its library. Arguments
in square brackets can be left out; `...` means as many as you like.

## B.1 Syntax

| Written | Is |
|---|---|
| `(f a b ...)` | An **S-expression**: a call of *f* with the values of *a*, *b* ... |
| `{a b ...}` | A **Q-expression**: a list, not worked out (code to run later, or names) |
| `[a b ...]` | `(list a b ...)`: a list of the values |
| `42`, `-3`, `2/3`, `1.25`, `2i`, `1+2i`, `1_000`, `#xFF` ... | Numbers, in any of Chapter 18's forms; read in the current base |
| `"text"` | A string; escapes `\n` `\t` `\r` `\0` `\a` `\b` `\f` `\v` `\e` `\\` `\"` and `\x`*HH* |
| `"""text"""` | A here string: the text as it is (as many quotes close it as opened it) |
| `\a`, `\space`, `\newline`, `\tab` | Characters: one, or one by its name |
| `:name` | An atom |
| `name` | A symbol (in small letters); `T` and `NIL` are true and false |
| `$name` | `(env "name")`: an environment variable |
| `; text` | A comment, to the end of the line |
| `:(`...`)` `=(`...`)` `?(`...`)` `@(`...`)` `#(`...`)` `.(`...`)` `~(`...`)` | `(def ...)`, `(set ...)`, `(if ...)`, `(fn ...)`, `(hash-create ...)`, `(unpack ...)`, `(format ...)` |
| `&_` | In a function: its arguments beyond its formals, as a list |
| `&1`, `&2` ... | In a function made with no formals: its arguments, by position |
| `&0` | In a method: the hash |

A prefix before `{` instead of `(` makes a Q-expression of the call: `?{a b c}` is `{if a b c}`.

## B.2 Special forms

These get their arguments as they're written, and decide themselves what to work out.

| Form | What it does |
|---|---|
| `(def {names} values ...)` | Give names values, globally; `NIL` |
| `(set {names} values ...)` | The same, in this scope |
| `(set! {names} values ...)` | The same, where each name was made |
| `(let {{name value} ...} body ...)` | The body in a scope of its own, with the names given values |
| `(fn {formals} {body})` | A function |
| `(fun {name formals ...} {body})` | Define a named function |
| `(fexpr {formals} {body})` | A function that gets its arguments as they're written |
| `(if test then [else])` | *then* if *test* isn't `NIL`, else *else* |
| `(do x ...)` | Each in turn; the last one's value |
| `(and x ...)`, `(or x ...)` | The first `NIL`, or the last value; the first value that isn't `NIL` |
| `(while test body ...)` | Repeat the body while *test* isn't `NIL` |
| `(each {x list} body ...)`, `(each f list)` | The body with *x* each item (or character); or *f* called with each |
| `(dotimes {i n} body ...)`, `(dotimes n f)` | The body *n* times, *i* 0 to *n* - 1 |
| `(<=> n {test value} ...)` | The value of the first case whose test (`=` `<>` `<` `>` `<=` `>=`) is true of *n*'s sign |
| `(try x [handler])` | *x*'s value; if that's an error, the handler's (with `&err` and `&code`), or `NIL` |
| `(eval x)` | Run a Q-expression as code |
| `(output-of x ...)` | What the expressions print, as a string |
| `(defined? x)`, `(expr? x)` | Is *x* a name in use here? Is it an expression? |
| `(hold body ...)` | The body, with no other task given a turn meanwhile |
| `(load path ...)` | Run each file's expressions |
| `(newns)` | Build the namespace again |
| `(exit [n])` | Leave hylang, with status *n* |

## B.3 Lists

| Function | Gives |
|---|---|
| `(list x ...)` | A list of the values |
| `(len l)` | Its length (a string's too) |
| `(head l)`, `(tail l)` | A list of the first item; all but the first |
| `(init l)`, `(end l)` | All but the last; a list of the last |
| `(fst l)` `(snd l)` `(thd l)` `(last l)`, `(nth n l)` | An item (from 0) |
| `(item-at l i)` | Item *i*, as it's written |
| `(subset l i [n])` | *n* items from *i* |
| `(cons x l)` | *x* in front of *l* |
| `(join l ...)` | The lists joined |
| `(take n l)`, `(drop n l)` | The first *n*; all but them |
| `(reverse l)` | Backwards (a string's too) |
| `(range to)`, `(range from to [step])` | Numbers, up to (not including) *to* |
| `(sort l [less])` | In order (by `cmp`, or by *less*) |
| `(elem? x l)`, `(in? x l)` | Is *x* in *l*? |
| `(map f l)` | *f* of each item |
| `(filter f l)` | The items *f* is true of |
| `(foldl f init l)`, `(foldr f init l)` | Combine the items, from the left, from the right |
| `(any? f l)`, `(all? f l)` | Is *f* true of any item? Of every one? |
| `(find f l)`, `(count f l)` | The first item *f* is true of; how many |
| `(sum l)`, `(product l)` | All the items added; multiplied |
| `(min x ...)`, `(max x ...)` | The smallest; the largest |

## B.4 Numbers

| Function | Gives |
|---|---|
| `(+ x ...)`, `(- x ...)`, `(* x ...)`, `(/ x ...)` | Sum, difference, product, quotient (exact) |
| `(1+ x)`, `(1- x)`, `(abs x)` | Plus one; minus one; its size |
| `(truncate x)` | The whole part (toward zero) |
| `(to-fixed x [places])`, `(to-rational x)` | As a fixed decimal; as a fraction |
| `(rational.n x)`, `(rational.d x)` | Its top; its bottom |
| `(complex re im)` | A complex number |
| `(val s)` | The number a string holds |
| `(fib n)` | The *n*th Fibonacci number |
| `(random n)` | A random whole number from 0 to *n* - 1 |
| `(base)`, `(base b)` | The base; set it (giving the one before) |
| `(digits)`, `(digits n)` | The math functions' precision; set it |
| `(sqrt x)` `(exp x)` `(log x)` `(sin x)` `(cos x)` `(tan x)` `(atan x)` `(pi)` `(pow x y)` | The math functions |
| `(number-bytes n)`, `(bytes-number b)` | A number as bytes, and back |
| `(== x y)`, `(eq x y)`, `(neq x y)` | Equal; not equal |
| `(< x y)` `(> x y)` `(<= x y)` `(>= x y)`, `(cmp x y)` | Comparisons; -1, 0 or 1 |
| `(neg? x)` `(pos? x)` `(zero? x)` `(one? x)` `(not x)` | Tests |
| `(bit-and n ...)` `(bit-or n ...)` `(bit-xor n ...)` `(bit-not n)` | Bits |
| `(shl n k)`, `(shr n k)`, `(bit? n k)` | Shifts; a bit |
| `(hex n [width])`, `(bin n [width])` | Its digits, in hex or binary |
| `(lo n)`, `(hi n)`, `(word lo hi)` | Its low byte, its next; two bytes made a word |
| `(bytes s)`, `(from-bytes l)` | A string's bytes; a string from bytes |

## B.5 Strings and characters

| Function | Gives |
|---|---|
| `(+ s x ...)` | The values joined, as `print` shows them |
| `(substring s i [n])`, `(char-at s i)` | Part of a string; a character |
| `(index-of s sub)`, `(last-index-of s sub)` | Where *sub* first, last, is (or `NIL`) |
| `(str-split s [sep])`, `(str-join l [sep])`, `(str-chars s)` | Split; join; its characters |
| `(str-upper s)`, `(str-lower s)`, `(str-trim s)` | Capitals; small letters; trimmed |
| `(str-replace s old new)`, `(str-repeat s n)` | Replaced; repeated |
| `(str-pad-left s n [c])`, `(str-pad-right s n [c])` | Padded to *n* characters |
| `(format text x ...)` | The text, each `{}` the next value |
| `(char-code c)`, `(code-char n)` | A character's code; the character for a code |
| `(alpha? c)` `(digit? c)` `(space? c)` `(upper? c)` `(lower? c)` | Its kind |
| `(to-str x [base])`, `(repr x)` | As `print` shows it; as the prompt shows it |
| `(to-sym x)`, `(to-atom x)` | A symbol; an atom |
| `(read text)` | The expressions in the text, not worked out |
| `(gensym [prefix])` | A new symbol, unlike any other |

## B.6 Types

`(type-of x)` gives `:number`, `:string`, `:char`, `:atom`, `:symbol`, `:function`, `:error`, `:t`, `:list`, `:sexpr`
or `:hash`. And each kind has a test: `t?`, `nil?`, `num?`, `int?`, `fixed?`, `rational?`, `complex?`, `atom?`,
`symbol?`, `string?`, `char?`, `function?`, `error?`, `qexpr?`, `sexpr?`.

## B.7 Hashes

| Function | What it does |
|---|---|
| `(hash-create [entries])`, `(to# entries)` | A hash, from a list of `{key value}` entries |
| `(from# h)` | A hash as a list of entries |
| `(h key)`, `(hash-get h key)` | A key's value (`NIL` if none) |
| `(hash-put h {key value} ...)` | Add or change entries |
| `(hash-key? h key)`, `(hash-keys h)`, `(hash-values h)` | Is the key there? The keys; the values |
| `(hash-remove h key)` | Take an entry out |
| `(hash-call h key x ...)`, `(h key x ...)` | Call a method (`&0` the hash) |
| `(hash-clone h [override ...])` | A copy |
| `(hash-add-tag h tag ...)`, `(hash-tag? h tag)` | Tags, on the hash or an entry |
| `(hash-lock h [key ...])`, `(hash-make-const ...)`, `(hash-make-private ...)`, `(hash-make-not-nil ...)` | Protect the hash, or entries |
| `(hash-locked? h [key])`, `(hash-private? ...)`, `(hash-const? ...)` | Are they? |

## B.8 Errors, output and input

| Function | What it does |
|---|---|
| `(error message [code])` | An error |
| `(error-message e)`, `(error-code e)` | Its message; its code |
| `(print x ...)` | Print the values, spaces between, and a new line |
| `(write x ...)` | Print them, without the new line |
| `(save x [path mode ...])` | Print *x* as it reads back, or write it to a file |
| `(read-line [s])`, `(read-byte [s])`, `(read-all [s])` | Read a line, a byte, the rest (from the keyboard, or a stream) |
| `(key)`, `(key?)` | The next key, as it's pressed; is one waiting? |

## B.9 Files and streams

| Function | What it does |
|---|---|
| `(open path [mode])` | A stream on a file: `:read`, `:write`, `:append` or `:update` |
| `(close s)` | Close it |
| `(print-to s x ...)`, `(write-to s x ...)` | Write to it |
| `(read-bytes s n)`, `(write-bytes s bytes [i [n]])` | Bytes |
| `(seek s n)`, `(tell s)` | Its place |
| `(read-file p)`, `(read-lines p)` | A whole file; its lines |
| `(write-file p x ...)`, `(append-file p x ...)` | Write a file; add to it |
| `(ls [p])`, `(dir [p])`, `(stat p)`, `(glob pattern)` | Names; their details; one's; those matching |
| `(exists? p)`, `(dir? p)`, `(file? p)` | Tests |
| `(mkdir p)`, `(remove p)`, `(rename p q)`, `(copy-file p q)` | Directories and files |
| `(cd [p])`, `(cwd)` | The current directory |

## B.10 Buffers

A **buffer** is a block of bytes, which can be called with an index for a byte: `(b 3)`.

| Function | What it does |
|---|---|
| `(buffer n [fill])`, `(buffer s)`, `(buffer l)` | A buffer of *n* bytes; of a string's; of a list's |
| `(buffer? x)` | Is it one? |
| `(buffer-get b i)`, `(buffer-put b i byte)` | A byte; set one |
| `(buffer-fill b byte [i [n]])`, `(buffer-copy to at from [i [n]])` | Fill; copy |
| `(read-buffer s b [at [n]])` | Read a stream into it |
| `(buffer-cmp b at x [i [n]])` | Compare with another buffer or string |

## B.11 Programs, time and the system

| Function | What it does |
|---|---|
| `(run prog x ...)`, `(spawn prog x ...)`, `(wait task)`, `(kill task)` | Programs |
| `(sh line [input])`, `(sh-out line [input])` | Command lines (their status; their output) |
| `(pid)` | This task |
| `(env [name])`, `(setenv name value)`, `(unsetenv name)` | The environment |
| `(time)`, `(date [t])`, `(date-parts [t])`, `(seconds-of y m d [h mi s])` | The clock |
| `(ticks)`, `(tick-rate)`, `(clock)` | Ticks (0-32,767), 200 a second; seconds since hylang started |
| `(sleep s)`, `(sleep-until tick)`, `(yield)` | Wait |
| `(platform)`, `(hydra?)` | `:hydra`; `T` |
| `(sysinfo)`, `(mods)`, `(ps)`, `(task-info task)`, `(errstr code)`, `(free)` | The system |
| `(peek a)` `(poke a b)` `(peek-word a)` `(poke-word a n)` | Memory |
| `(banks)` `(bank-alloc n)` `(bank-free b n)` `(bank-read b off n)` `(bank-write b off bytes)` | RAM banks |
| `(seg-create n)` `(seg-attach s)` `(seg-detach s)` `(seg-read s b off n)` `(seg-write s b off bytes)` | Shared segments |
| `(bind new old ...)`, `(mount dev old ...)`, `(unmount old [new])`, `(ns)` | The namespace (`:before`, `:after`, `:create`) |
| `(note task n)`, `(note-group g n)`, `(on-note f)` | Notes |
| `(sys name x ...)`, `(sys-`*name* `x ...)` | A system call (Appendix K) |

## B.12 The library

hylang's library (`globals`, loaded as it starts) adds:

| Function | What it does |
|---|---|
| `math.pi` `math.e` `math.phi` `math.gamma` | Constants, to many places |
| `(square n)`, `(cube n)`, `(recip n)` (`inverse`) | *n*², *n*³, 1/*n* |
| `(floor a)` `(ceil a)` `(round a)` `(trunc a)` | Whole numbers |
| `(div a b)`, `(mod a b)`, `(divmod a b)`, `(fdiv a b)` | Division, rounding down |
| `(odd? n)` `(even? n)` `(positive? a)` `(negative? a)` | Tests |
| `(xor a b)`, `(<> a b)` (`!=`) | Logic; not equal |
| `(=0 n)` `(=1 n)` `(>0 n)` `(<0 n)` `(>=0 n)` `(<=0 n)` `(!=0 n)` `(!=1 n)` | Comparisons with 0 and 1 |
| `first` ... `tenth` | Items 1 to 10 of a list |
| `(split n l)`, `(zip a b)`, `(flatten l)`, `(distinct l)`, `(avg l)` | Lists |
| `(minimum ...)`, `(maximum ...)`, `(sort-desc l)`, `(best-of n l)`, `(worst-of n l)` | Order |
| `(remove-fst f l)`, `(remove-all f l)`, `(remove-min ...)`, `(remove-max ...)` | Removing items |
| `(between? lo hi x)` (`><?`), `(not-between? ...)` and others | Is *x* between *lo* and *hi*? |
| `(starts-with? s sub)`, `(ends-with? s sub)`, `(contains? s sub)` | Strings |
| `(repeat n d)` (`times`) | A list of *d*'s value *n* times |
| `(cond {test value} ...)`, `(case x {value result} ...)` | Choices |
| `(flip f a b)`, `(comp f g x)`, `(unpack f l)` (`apply`, `curry`), `(pack f ...)` (`uncurry`) | Functions on functions |
| `(?? a ...)` | The first argument that isn't `NIL` |
| `(use "name")` | Load a library once |
| `(buffer-cmp ...)` | (B.10) |

And the libraries loaded with `use` (Chapter 22): `gpio`, `i2c`, `spi`, `clock`, `disk`, `cons`, `proc`, `pc`, `snd`,
`video`, `screen`, `dev`; and `dice` (with `load`): `(d6)`, `(d20)`, `(dice n sides)`, and more for games.
