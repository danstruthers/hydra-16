# Chapter 19. Learning HyForth

**Forth** is a programming language invented by Charles Moore around 1970, to control radio telescopes. It's small,
fast and simple enough to fit in a tiny computer, which made it popular on the home computers of the 1980s. **HyForth**
is the Hydra's Forth: it follows the 2012 standard for the language, and adds words for the Hydra's own features.
This chapter teaches you Forth from the beginning. You've met HyForth already: it's the shell you type at.

> **IN THIS CHAPTER:** You'll learn Forth from the beginning: the stack, making new words, decisions, loops,
> variables, strings and memory. **You need:** Chapter 6. You don't need to have programmed before.

## 19.1 Talking to HyForth

At the `/>` prompt, any line that starts with a Forth word, or a number, is Forth (section 8.1); everything else is a
command for rc. If you'd rather every line was Forth, type `forth` to start a plain HyForth, which says `ok` after
each line it has done; type `bye` to leave it.

Forth reads a line as **words**, separated by spaces, and does each one in turn, from left to right. A word is any
group of characters without a space: `+`, `dup`, `."` and `2/` are all words. Capital and small letters are the same,
so `DUP` and `dup` are the same word. And a word that's a number puts that number on the **stack**.

## 19.2 The stack

The **stack** is the heart of Forth. It's a pile of numbers: each number you type goes on the top, and most words take
their numbers off the top, and put their answers back there. `.s` shows the whole stack, without changing it; the
number in angle brackets is how many are on it, and the top is on the right. `.` ("dot") takes the top number off,
and prints it:

```session
/> 1 2 3 .s
<3> 1 2 3
/> . . .
3 2 1
/> .s
<0>
```

The numbers come off in the opposite order to the one they went on: the last one on is the first one off. Think of a
pile of plates: you put plates on the top, and take them off the top.

## 19.3 Arithmetic

Since the numbers are on the stack, a word like `+` doesn't need anything after it: it takes two numbers off the
stack, adds them, and puts the answer back. So in Forth, you write the numbers first, and then what to do with them:

```session
/> 2 3 + .
5
/> 10 3 - .
7
/> 6 7 * .
42
/> 20 4 / .
5
/> 7 2 mod .
1
```

`/` divides, and throws away any remainder; `mod` gives the remainder; and `/mod` gives both, the remainder under
the answer:

```session
/> 7 2 /mod . .
3 1
```

This way of writing sums is called **postfix**, or **reverse Polish**, notation. It never needs parentheses, because
the order of the words says what's done first:

```session
/> 2 3 + 4 * .
20
/> 2 3 4 * + .
14
```

The first adds 2 and 3, then multiplies by 4. The second multiplies 3 by 4, then adds 2.

> **NOTE:** Each number on Forth's stack is a **cell**: 16 bits, so it holds -32,768 to 32,767 (or 0 to 65,535). A
> sum that goes past them wraps round: `32767 1 + .` prints `-32768`. Section 20.8 shows how HyForth's numbers library
> works with the Hydra's big numbers, fractions and decimals instead.

## 19.4 Moving things on the stack

These words rearrange the stack. Each comes with a **stack diagram**, in parentheses: what it takes from the stack,
`--`, then what it leaves, with the top on the right.

| Word | Stack | What it does |
|---|---|---|
| `dup` | ( *a* -- *a a* ) | Copy the top |
| `drop` | ( *a* -- ) | Throw away the top |
| `swap` | ( *a b* -- *b a* ) | Swap the top two |
| `over` | ( *a b* -- *a b a* ) | Copy the second to the top |
| `rot` | ( *a b c* -- *b c a* ) | Bring the third to the top |
| `nip` | ( *a b* -- *b* ) | Throw away the second |
| `tuck` | ( *a b* -- *b a b* ) | Copy the top under the second |
| `2dup` | ( *a b* -- *a b a b* ) | Copy the top two |
| `2drop` | ( *a b* -- ) | Throw away the top two |

```session
/> 1 2 swap .s
<2> 2 1
/> drop drop
/> 1 2 3 rot .s
<3> 2 3 1
```

## 19.5 Making new words

This is where Forth comes alive. You can make new words out of old ones, and use them just like the built-in ones. A
**definition** starts with `:` and the new word's name, and ends with `;`:

```session
/> : square dup * ;
/> 5 square .
25
```

Now `square` is a word, as good as `dup` or `*`. You can use it to make more words:

<!-- session
: square dup * ;
-->

```session
/> : cube ( n -- n^3 ) dup square * ;
/> 3 cube .
27
```

The `( n -- n^3 )` is a **comment**: Forth skips everything from `(` to `)`. It's good to give each word a comment
with its stack diagram, so you (and others) know what it takes and leaves. A `\` makes the rest of the line a comment.

`see` shows how a word was defined:

<!-- session
: square dup * ;
: cube ( n -- n^3 ) dup square * ;
-->

```session
/> see cube
: cube dup square * ;
```

A Forth program is just a collection of words, each built from the ones before, until the last one does the whole
job. Give your words names that say what they do, and keep each one short: a line or two. If you define a word again,
the new definition is used from then on, but the words already made with the old one keep using it.

## 19.6 Printing

| Word | Stack | What it does |
|---|---|---|
| `.` | ( *n* -- ) | Print a number, and a space |
| `."` *text*`"` | ( -- ) | Print the text (in a definition) |
| `.(` *text*`)` | ( -- ) | Print the text, at once (outside a definition) |
| `cr` | ( -- ) | Start a new line |
| `emit` | ( *char* -- ) | Print the character whose code is *char* |
| `space` | ( -- ) | Print a space |
| `spaces` | ( *n* -- ) | Print *n* spaces |
| `type` | ( *c-addr u* -- ) | Print a string (section 19.11) |

```session
/> : hello ." Hello, world!" cr ;
/> hello
Hello, world!
```

The space after `."` isn't part of the text: it just separates the word from the text.

## 19.7 True and false

A **comparison** takes two numbers, and leaves a **flag**: -1 for true, or 0 for false.

```session
/> 5 3 > .
-1
/> 5 3 < .
0
/> 5 5 = .
-1
```

| Word | Stack | True if |
|---|---|---|
| `=` | ( *a b* -- *flag* ) | *a* equals *b* |
| `<>` | ( *a b* -- *flag* ) | They differ |
| `<`, `>` | ( *a b* -- *flag* ) | *a* is less, or more, than *b* |
| `0=` | ( *a* -- *flag* ) | *a* is 0 |
| `0<`, `0>` | ( *a* -- *flag* ) | *a* is below, or above, 0 |
| `within` | ( *n lo hi* -- *flag* ) | *lo* ≤ *n* < *hi* |

`and`, `or` and `xor` combine flags (they work on each bit of the numbers), and `0=` turns true into false and back.

## 19.8 Making decisions

In a definition, `if` takes a flag from the stack. If it's true (anything but 0), the words up to `then` are done;
if it's false, they're skipped. With an `else`, the words between `else` and `then` are done when it's false:

```session
/> : test ( n -- ) 0< if ." negative" else ." not negative" then cr ;
/> -4 test
negative
/> 7 test
not negative
```

`then` is where the paths join up again: "if this, do that, *then* carry on". `if`, `else` and `then` only work
inside definitions.

To choose between several cases, use `case`:

```text
: day ( n -- )
  case
    0 of ." Sunday" endof
    6 of ." Saturday" endof
    ." a weekday"
  endcase cr ;
```

## 19.9 Loops

`do` ... `loop` counts. It takes two numbers: the end, and the start. `i` puts the count on the stack each time round:

```session
/> : count-up 10 0 do i . loop cr ;
/> count-up
0 1 2 3 4 5 6 7 8 9
```

The loop stops before it reaches the end: 10 0 counts from 0 to 9. `+loop` counts in steps of the number it takes:

```session
/> : evens 10 0 do i . 2 +loop cr ;
/> evens
0 2 4 6 8
```

`begin` ... `until` repeats until a flag is true:

```session
/> : countdown ( n -- ) begin dup . 1- dup 0= until drop cr ;
/> 5 countdown
5 4 3 2 1
```

And `begin` ... `while` ... `repeat` tests at the start of each time round: it goes on while the flag is true.

| Loop | How it works |
|---|---|
| *end start* `do` ... `loop` | Counts from *start* to *end* - 1; `i` is the count (`j` the outer loop's) |
| *end start* `do` ... *n* `+loop` | Counts in steps of *n* |
| `begin` ... *flag* `until` | Repeats until the flag is true |
| `begin` ... *flag* `while` ... `repeat` | Repeats while the flag is true |
| `begin` ... `again` | Repeats for ever (press <kbd>CTRL</kbd>-<kbd>C</kbd>) |
| `leave` | Leaves a `do` loop at once |

## 19.10 Variables and constants

A **variable** is a named place in memory that holds a number. `variable score` makes one. Saying its name puts its
**address** (where it is) on the stack; `!` ("store") puts a number there, and `@` ("fetch") gets it back:

```session
/> variable score
/> 10 score !
/> score @ .
10
/> 5 score +!
/> score ?
15
```

`+!` adds to a variable, and `?` prints it (it's the same as `@ .`).

A **constant** has a fixed value, which its name puts on the stack:

```session
/> 42 constant answer
/> answer .
42
```

A **value** is between the two: its name gives its value, like a constant, but `to` changes it:

```session
/> 0 value lives
/> 3 to lives
/> lives .
3
```

## 19.11 Characters and strings

Every character has a number, its **code** (Appendix E has them all). `char` gives a character's code, and `emit`
prints the character for a code:

```session
/> char A .
65
/> 65 emit cr
A
```

In a definition, `[char] A` does the same as `char A`:

```session
/> : stars ( n -- ) 0 do [char] * emit loop cr ;
/> 5 stars
*****
```

A **string** is a row of characters in memory. Forth keeps a string as two numbers on the stack: its address, and its
length (written *c-addr u* in stack diagrams). `s"` makes one, and `type` prints one:

```session
/> s" hello" type cr
hello
/> : greet ( c-addr u -- ) ." Hello, " type ." !" cr ;
/> s" Sam" greet
Hello, Sam!
```

## 19.12 Memory

A variable is one cell of memory. For more, `create` makes a name for the place where the next things go, and `,`
("comma") puts a number there, in the next cell:

```session
/> create primes 2 , 3 , 5 , 7 , 11 ,
/> primes 3 cells + @ .
7
```

`primes` gives the address of the first number; `3 cells +` moves three cells on (a cell is two bytes); `@` fetches
the fourth prime. Counting starts at 0, so that's item number 3.

| Word | Stack | What it does |
|---|---|---|
| `@`, `!` | ( *addr* -- *x* ), ( *x addr* -- ) | Fetch, store a cell |
| `c@`, `c!` | ( *addr* -- *char* ), ( *char addr* -- ) | Fetch, store a byte |
| `+!` | ( *n addr* -- ) | Add to a cell |
| `,`, `c,` | ( *x* -- ), ( *char* -- ) | Put a cell, a byte, in the next place |
| `allot` | ( *n* -- ) | Leave room for *n* bytes |
| `cells` | ( *n* -- *n×2* ) | How many bytes *n* cells take |
| `here` | ( -- *addr* ) | The next free place |
| `fill`, `erase` | ( *addr u char* -- ), ( *addr u* -- ) | Fill memory with a byte, or zeros |
| `move` | ( *from to u* -- ) | Copy *u* bytes |
| `dump` | ( *addr u* -- ) | Show memory, in hex |

## 19.13 Numbers in other bases

HyForth reads and prints numbers in the base in the variable `base`, usually 10. `hex` sets it to 16, and `decimal`
back to 10:

```session
/> 255 hex . decimal
FF
```

A number can also say its own base: `$` for hex, `%` for binary, `#` for decimal, and a character in single quotes
is its code:

```session
/> $FF . %1010 . #99 . 'A' .
255 10 99 65
```

(With the numbers library, section 20.8, HyForth reads all of Chapter 18's numbers too.)

## 19.14 Words that call themselves

A word can use itself, with `recurse`. The **factorial** of a number is it times every number below it: 5 × 4 × 3 ×
2 × 1. Or, put another way, the factorial of *n* is *n* times the factorial of *n* - 1:

```session
/> : fact ( n -- n! ) dup 1 > if dup 1- recurse * then ;
/> 5 fact .
120
```

## 19.15 When things go wrong

When HyForth doesn't know a word, it prints the word and a `?`:

```session
/> : oops nothere ;
nothere ?
```

Other mistakes give a message, and HyForth empties its stack and carries on with the next line:

```session
/> 1 0 / .
division by zero
```

A program can catch errors itself. `catch` runs a word (given by its **execution token**, which `'` or `[']` gives),
and leaves 0 if it worked, or the error's number if it **threw** one:

```session
/> : safe-div ( a b -- ) ['] / catch if 2drop ." can't divide" cr else . cr then ;
/> 10 2 safe-div
5
/> 10 0 safe-div
can't divide
```

`throw` ( *n* -- ) throws error *n* (unless it's 0). The standard's errors are -1 to -255, and HyForth's
system errors -512 and below, so give your own errors positive numbers.

## 19.16 Keeping your words

Words you type at the prompt are gone when HyForth ends. To keep them, write them in a file (with `edit`, Chapter
11), and **include** it:

```session
/> echo ': twice ( n -- 2n ) 2 * ;' >/ram/my.fs
/> echo '21 twice . cr' >>/ram/my.fs
/> include /ram/my.fs
42
/> 4 twice .
8
```

`include` reads the file as if you'd typed it. `require` does the same, but only once: a file that's been included
already isn't read again.

A file of Forth can also be run as a program, a **script**, with `forth file.fs`, followed by any arguments. Its
first line can be `#!/bin/forth`, and then it runs by its name, like any program. In a script, `argc` and `arg` give
its arguments (section 20.9), and `bye` (or reaching the end) ends it.

Put your Forth files on an SD card, so they're there next time. Chapter 20 shows how to use HyForth to work the
Hydra's devices, and Appendix A lists every word.
