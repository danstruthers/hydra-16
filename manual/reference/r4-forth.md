# Chapter 4. HyForth Words

HyForth follows the Forth 2012 standard. A word that the standard defines does what the standard says, and its
entry here states that briefly and exactly; where the standard leaves a choice to the system, or calls a case
ambiguous, this chapter says what HyForth does. The words that are HyForth's own are described in full.

**The stack pictures** follow the standard's notation: what a word takes, `--`, what it leaves, the top of the
stack on the right. *n* is a signed cell, *u* an unsigned one, *x* any cell, *flag* a truth value, *c-addr u* the
address and length of a string, *d* and *ud* double-cell numbers, *xt* an execution token, *ior* an I/O result.
`R:` is the return stack and `N:` the numbers library's stack. *"name"* is a word taken from the input after
this one.

**Each entry says** which word set a word belongs to, and where it lives: in the ROM core, which is always
there, or in a library. The libraries `coreext`, `exception`, `file` and `tools` are loaded as HyForth starts,
and `shell` when it starts as a shell; any other is loaded with `lib` and its name. **Immediate** means the word
runs while a definition is being compiled; **compile only**, that it has no meaning outside one.

### What the standard leaves to the system

These are the facts the standard requires a system to state, as HyForth has them.

| | |
|---|---|
| A cell | 16 bits: a signed number is −32,768 to 32,767, an unsigned one 0 to 65,535 |
| A double | 32 bits, its high cell on top of the stack |
| A character | 8 bits; an address unit is a byte, and nothing needs aligning |
| True | −1, all bits set. Any value but 0 counts as true |
| Arithmetic | Two's complement, wrapping silently: `32767 1+` is −32768 |
| Division | Symmetric: `/`, `mod`, `/mod`, `*/` and `*/mod` round the quotient toward zero, and the remainder has the dividend's sign (`-7 2 /` is −3, remainder −1). `fm/mod` is there for floored division, and `sm/rem` for symmetric |
| Division by zero | The throw code −10 |
| Names | Capitals and small letters are the same: `DUP` is `dup`. The system's own names are in small letters |
| Numbers | Read in `base`, 2 to 36. A number can say its own base: `$FF` hex, `%1010` binary, `#10` decimal, `'A'` a character. One that ends in `.` is a double |
| `environment?` | Knows no queries: it answers false to every one |
| A word that isn't found | At the shell's prompt, the line is taken as a command and given to rc, whose message says it isn't found. In a file being loaded, the throw code −13: the word and a `?` |
| An empty stack | A word that takes more than the stack holds is the throw code −4, `stack underflow`, found when the line has been run |
| Errors | The standard's throw codes. An error that nothing catches prints its message, empties both stacks and returns to the prompt. A failure of the system is −512 minus its error code (Appendix G), which is what the file words' *ior*s are |
| Comments | `\` to the end of the line, and `(` to the next `)` |
| Code | New definitions are compiled into RAM banks of HyForth's own, so a program is not limited to the 32K of ordinary memory |

## %

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( "line" -- )

#### Description

The rest of the line is rc's.

## ', [']

Core word set; in the ROM core, always there. `[']` is immediate, and compile only.

#### Stack

```text
'     ( "name" -- xt )
[']   ( "name" -- )        compiling
      ( -- xt )            when the definition runs
```

#### Description

`'` ("tick") takes the next word of the line, finds it in the dictionary, and leaves its **execution
token**: a cell that stands for the word, which `execute` runs. On HyForth a token is the address of the
word's code.

`'` does its work when it runs. Inside a definition that would be when the definition runs, taking a name
from the line being read then. To put the token of a word named *in* the definition into it, use `[']`: it
finds the word as the definition is compiled, and compiles its token as a number.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −13 | *name* `?` | There is no such word |

#### See also

`execute`, `compile,`, `>body`, `defer`, `find`, `postpone`.

#### Example

```session
/> : square  dup * ;
/> 5 ' square execute .
25
/> : apply  ['] square execute ;
/> 6 apply .
36
/> ' nothere
nothere ?
```

## (

Core word set; in the ROM core, always there. Immediate.

#### Stack

```text
(   ( "text<paren>" -- )
```

#### Description

A comment: everything up to the next `)` is skipped. Like any word, `(` must have a space after it. It works
at the prompt and inside a definition, where its usual job is the word's stack picture.

#### See also

`\`, `.(`.

#### Example

```session
/> : square  ( n -- n*n )  dup * ;
/> 4 square ( this is skipped ) .
16
```

## */, */mod

Core word set; in the ROM core, always there.

#### Stack

```text
*/      ( n1 n2 n3 -- n4 )
*/mod   ( n1 n2 n3 -- n4 n5 )
```

#### Description

Multiplies *n1* by *n2*, and divides the product by *n3*. The product is kept as a 32-bit double, so it does
not overflow as `*` and then `/` would. `*/` leaves the quotient; `*/mod` leaves the remainder, and above it
the quotient.

This is the way to scale a number by a fraction with whole numbers only: `1000 314 100 */` is 3140, 1000
times 3.14. The division is symmetric, as `/` is.

The quotient itself must fit in a cell. One that does not is not reported: the result is wrong.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | *n3* is 0 |

#### See also

`m*/`, `/mod`, `um/mod`.

#### Example

```session
/> 1000 1000 100 */ .
10000
/> 1000 1000 * 100 / .
169
/> 100 7 3 */mod . .
233 1
```

## +, -, *

Core word set; in the ROM core, always there.

#### Stack

```text
+   ( n1 n2 -- n3 )
-   ( n1 n2 -- n3 )
*   ( n1 n2 -- n3 )
```

#### Description

The sum, the difference *n1* − *n2*, and the product of two cells. They are the same for signed and for
unsigned numbers.

A result that does not fit in 16 bits wraps round silently, and nothing reports it: `32767 1 +` is −32768,
and `300 300 *` is 24464, the low 16 bits of 90,000. Where a product or a total may be too big for a cell,
use the mixed words (`m*`, `um*`, `*/`), the double-number words, or the numbers library, whose integers
are of any length.

#### See also

`1+`, `m*`, `um*`, `*/`, `d+`, `n+`.

#### Example

```session
/> 3 4 + .
7
/> 3 4 - .
-1
/> 3 4 * .
12
/> 32767 1 + .
-32768
```

## +!

Core word set; in the ROM core, always there.

#### Stack

```text
+!   ( n a-addr -- )
```

#### Description

Adds *n* to the cell stored at the address: it is `dup @ rot + swap !` in one word. A negative *n* takes
away.

#### See also

`@`, `!`, `1+`.

#### Example

```session
/> variable total  10 total !
/> 5 total +!  -1 total +!
/> total @ .
14
```

## ,, c,

Core word set; in the ROM core, always there.

#### Stack

```text
,    ( x -- )
c,   ( char -- )
```

#### Description

`,` ("comma") puts a cell into data space at `here`, and moves `here` on by 2. `c,` puts one byte there, the
low byte of the cell given, and moves `here` on by 1.

After `create` they build a table of values, in order, as the program is loaded.

#### See also

`create`, `allot`, `here`, `@`.

#### Example

```session
/> create primes 2 , 3 , 5 , 7 ,
/> primes 2 cells + @ .
5
/> create msg 3 c, char a c, char b c, char c c,
/> msg count type
abc
```

## ., u.

Core word set; in the ROM core, always there.

#### Stack

```text
.    ( n -- )
u.   ( u -- )
```

#### Description

Print the number on top of the stack, in the current `base`, with one space after it, and remove it. `.`
("dot") prints it as a signed number, and `u.` as an unsigned one: the same cell that `.` shows as −5 is
65531 to `u.`. Use `u.` for addresses and for anything that may be above 32,767.

In a base above ten the digits past 9 are capital letters.

#### See also

`.r`, `d.`, `.s`, `?`, `base`, `<#`, `n.`.

#### Example

```session
/> 5 . -5 .
5 -5
/> -5 u.
65531
/> 40000 . 40000 u.
-25536 40000
/> 255 hex . decimal
FF
```

## ."

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
."   ( "text<quote>" -- )
```

#### Description

Inside a definition, compiles the text that follows, up to the next `"`, so that it is printed when the word
runs. One space must follow `."`, as after any word; that space is not part of the text, and everything
after it is, spaces too.

It can't be used at the prompt (`.": compile only`): there, use `.(`, or `s"` and `type`.

#### See also

`.(`, `s"`, `type`, `abort"`.

#### Example

```session
/> : greet  ." Hello, " type ." !" ;
/> s" Ann" greet
Hello, Ann!
```

## .(

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate.

#### Stack

```text
.(   ( "text<paren>" -- )
```

#### Description

Prints the text that follows, up to the next `)`, at once: at the prompt, or, since it is immediate, in the
middle of a definition as the definition is compiled. It is for messages from a file as it loads.

#### See also

`."`, `(`, `type`.

#### Example

```session
/> .( loading...)
loading...
```

## /, mod, /mod

Core word set; in the ROM core, always there.

#### Stack

```text
/      ( n1 n2 -- n3 )
mod    ( n1 n2 -- n3 )
/mod   ( n1 n2 -- n3 n4 )
```

#### Description

Signed division of *n1* by *n2*. `/` leaves the quotient, `mod` the remainder, and `/mod` both: the
remainder *n3*, and above it the quotient *n4*.

HyForth's division is symmetric: the quotient is rounded toward zero, and the remainder has the sign of
*n1*. So `-7 2 /` is −3, with the remainder −1. For division that rounds down, use `fm/mod`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | *n2* is 0 |

#### See also

`*/`, `fm/mod`, `sm/rem`, `um/mod`, `2/`.

#### Example

```session
/> 7 2 / .
3
/> -7 2 / .
-3
/> -7 2 mod .
-1
/> 7 2 /mod . .
3 1
/> 1 0 /
division by zero
```

## :

Core word set; in the ROM core, always there.

#### Stack

```text
:   ( "name" -- )
```

#### Description

Starts a definition. `:` takes the next word of the line as the name of a new word, and puts HyForth into
compiling: from here to the `;` that ends the definition, the words of the line are not run but compiled
into the new word. Immediate words are the exception: they run as they are met, and do the compiling of
`if`, `do`, `."` and the like.

The new word cannot be found until its `;`: inside its own definition, its name still means whatever it
meant before, so a new version of a word can call the old one. For a word that calls itself, use `recurse`.

A name can be given twice. The later definition is the one found from then on, and nothing reports it; words
already compiled keep using the earlier one.

A definition may run over several lines. HyForth compiles to machine code: a call to each word, with the
shortest words copied in place.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −16 | `no name` | Nothing follows `:` on the line |
| −13 | *word* `?` | A word in the definition isn't known, and isn't a number |

#### See also

`;`, `:noname`, `immediate`, `recurse`, `see`, `exit`.

#### Example

```session
/> : square  dup * ;
/> 7 square .
49
/> : square  square square ;
/> 3 square .
81
```

## ;

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
;   ( -- )
```

#### Description

Ends the definition that `:` or `:noname` began: it compiles the return, makes the new word findable, and
puts HyForth back into interpreting.

#### Notes

HyForth does not check that the definition's control structures are complete. An `if` with no `then`, or a
`do` with no `loop`, is not reported at `;`: the word is made, and misbehaves when it runs.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −14 | `;: compile only` | `;` with no definition open |

#### See also

`:`, `exit`, `immediate`.

#### Example

```session
/> : hello  ." Hello!" ;
/> hello
Hello!
/> ;
;: compile only
```

## <#, #, #s, hold, holds, sign, #>

Core word set; in the ROM core. Pictured numeric output: these build the text of a number, one character at a
time, from its last digit to its first.

#### Stack

```text
<#     ( -- )
#      ( ud1 -- ud2 )
#s     ( ud1 -- 0 0 )
hold   ( char -- )
sign   ( n -- )
#>     ( xd -- c-addr u )
```

#### Description

`<#` starts a new picture, in a buffer that belongs to these words. The number being converted is an unsigned
double on the stack throughout. `#` divides it by `base`, puts the remainder's digit in front of what the
picture holds so far, and leaves the quotient; `#s` does that until the quotient is 0, and at least once, so a
0 gives the one digit `0`. `hold` puts any character in front, and `sign` puts a minus sign there if its
argument is negative. `#>` drops the double and returns the finished text.

Since the text is built backwards, the words are written in the reverse of the order the characters will
appear: the digits after a decimal point first, then the point, then the rest.

The text is in a transient buffer, which the next picture, and the words that print numbers, use again. Use it
or copy it at once.

#### Notes

`.`, `u.`, `.r` and `d.` are written with these words. `holds` ( *c-addr u* -- ), in `coreext`, puts a whole
string in front.

#### Example

A number of pence as pounds and pence:

```session
/> : .money  s>d <# # # [char] . hold #s [char] $ hold #> type ;
/> 12345 .money
$123.45
```

## <>, u>, 0<>, 0>

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
<>    ( x1 x2 -- flag )
u>    ( u1 u2 -- flag )
0<>   ( x -- flag )
0>    ( n -- flag )
```

#### Description

`<>` is true when two cells differ. `u>` is true when *u1* is greater than *u2*, as unsigned numbers. `0<>`
is true when the cell is not zero: it turns any true value into the well-formed flag −1. `0>` is true when
the cell is greater than zero.

#### See also

`=`, `u<`, `0=`, `0<`.

#### Example

```session
/> 1 2 <> . 1 1 <> .
-1 0
/> -1 1 u> .
-1
/> 5 0<> . 0 0<> .
-1 0
/> 5 0> . -5 0> . 0 0> .
-1 0 0
```

## =, <, >, u<

Core word set; in the ROM core, always there.

#### Stack

```text
=    ( x1 x2 -- flag )
<    ( n1 n2 -- flag )
>    ( n1 n2 -- flag )
u<   ( u1 u2 -- flag )
```

#### Description

Compare two cells, and leave true (−1) or false (0). `=` is true when they are the same. `<` and `>` compare
them as signed numbers: is *n1* less than *n2*, or greater? `u<` compares them as unsigned numbers.

The difference matters for anything above 32,767, and for addresses, which are unsigned: −1 is less than 1,
but as the unsigned number 65,535 it is not.

There is no `<=` or `>=`: use `> 0=` and `< 0=`.

#### See also

`<>`, `u>`, `0=`, `0<`, `within`, `d<`, `compare`.

#### Example

```session
/> 1 1 = . 1 2 = .
-1 0
/> 1 2 < . 2 1 < .
-1 0
/> -1 1 < . -1 1 u< .
-1 0
```

## ?

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
?   ( a-addr -- )
```

#### Description

Prints the cell stored at an address: it is `@ .`. It is for looking at a variable at the prompt.

#### See also

`@`, `.`, `dump`.

#### Example

```session
/> variable v  42 v !
/> v ?
42
```

## @, !

Core word set; in the ROM core, always there.

#### Stack

```text
@   ( a-addr -- x )
!   ( x a-addr -- )
```

#### Description

`@` ("fetch") leaves the cell stored at an address, and `!` ("store") stores a cell there. A cell is two
bytes, the low byte first, as the 65C02 keeps a word.

An address is any number from 0 to 65,535: it is not checked. The addresses a program owns are those it got
from `variable`, `create`, `here` and `allot`, `buffer:` or `allocate`. Anywhere else in the task's 64K can
be read safely, and can be written only by a program that knows what is there: the task's own first pages
and HyForth's dictionary are in the same memory (Appendix F).

#### See also

`c@`, `2@`, `+!`, `variable`, `?`.

#### Example

```session
/> variable v
/> 42 v !
/> v @ .
42
/> $FFFC @ u.
57344
```

## [, ]

Core word set; in the ROM core, always there. `[` is immediate.

#### Stack

```text
[   ( -- )
]   ( -- )
```

#### Description

`[` stops compiling and starts interpreting, in the middle of a definition; `]` goes back to compiling.
What is written between them is run then and there, as the definition is compiled. With `literal` this
works a value out once, at compile time, where the same words left in the definition would work it out
every time the word ran.

#### See also

`literal`, `state`, `immediate`.

#### Example

```session
/> : day-minutes  [ 24 60 * ] literal ;
/> day-minutes .
1440
/> see day-minutes
: day-minutes 1440 ;
```

## \

Core Extension; in the ROM core, always there. Immediate.

#### Stack

```text
\   ( "text<end of line>" -- )
```

#### Description

A comment: the rest of the line is skipped. It needs a space after it.

#### See also

`(`, `.(`.

#### Example

```session
/> 1 2 + .  \ adds, and prints
3
```

## {:

Locals; in the `locals` library (`lib locals`). Immediate. Compile only.

#### Stack

```text
{: arg ... | local ... -- comment :}   ( x ... -- )
```

#### Description

Gives names to values, for the length of one definition. Between `{:` and `:}` come the names. When the word
runs, a value is taken from the stack for each name before the `|`, the last name getting the top of the
stack, so the names read in the order the arguments were pushed. The names after a `|`, if there are any,
are locals that start with no value set. Anything from `--` to `:}` is a comment, for the results.

In the rest of the definition, a local's name leaves its value, and `to` changes it.

Locals take the place of the `dup`, `swap`, `rot` and `>r` that a word with three or four arguments would
otherwise need. They are kept on the return stack, so the rules of `>r` apply to what the word does around
them; they can be used inside `do` loops.

#### See also

`to`, `value`, `>r`.

#### Example

```session
/> lib locals
/> : add3  {: a b c :}  a b + c + ;
/> 1 2 3 add3 .
6
/> : hypot2  {: x y | sum -- n :}  x x * y y * + to sum  sum ;
/> 3 4 hypot2 .
25
```

## 0=, 0<

Core word set; in the ROM core, always there.

#### Stack

```text
0=   ( x -- flag )
0<   ( n -- flag )
```

#### Description

`0=` is true when the cell is zero. As every value but zero counts as true, it is also the logical "not":
it turns any true value to false, and false to true.

`0<` is true when the cell is negative, which is to say when its top bit is set.

#### See also

`0<>`, `0>`, `invert`, `=`.

#### Example

```session
/> 0 0= . 5 0= .
-1 0
/> -1 0< . 1 0< . 0 0< .
-1 0 0
```

## 1+, 1-, 2*, 2/

Core word set; in the ROM core, always there.

#### Stack

```text
1+   ( n1 -- n2 )
1-   ( n1 -- n2 )
2*   ( x1 -- x2 )
2/   ( x1 -- x2 )
```

#### Description

`1+` adds one, and `1-` takes one away. `2*` shifts the cell one bit to the left, which doubles it. `2/`
shifts it one bit to the right and keeps the sign bit as it was, which halves it, rounding *down*: `-5 2/` is
−3, where `-5 2 /` is −2.

#### See also

`+`, `lshift`, `rshift`, `d2*`, `cell+`.

#### Example

```session
/> 5 1+ . 5 1- .
6 4
/> 5 2* . 5 2/ .
10 2
/> -5 2/ .
-3
```

## 2>r, 2r>, 2r@

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Compile only.

#### Stack

```text
2>r   ( x1 x2 -- )       R: ( -- x1 x2 )
2r>   ( -- x1 x2 )       R: ( x1 x2 -- )
2r@   ( -- x1 x2 )       R: ( x1 x2 -- x1 x2 )
```

#### Description

A pair of cells moved to the return stack, moved back from it, or copied from it, keeping their order: after
`2>r`, *x2* is on top of the return stack, as it was on top of the data stack. `2>r` is `swap >r >r`, not
`>r >r`.

They are for use inside a definition, and what is put on the return stack must be taken off again before the
definition ends.

## 2@, 2!

Core word set; in the ROM core, always there.

#### Stack

```text
2@   ( a-addr -- x1 x2 )
2!   ( x1 x2 a-addr -- )
```

#### Description

Fetch and store a pair of cells: a double number, or a string's address and length. *x2*, the cell on top
of the stack, is the one at *a-addr*, and *x1* is in the cell after it. For a double, that puts its high
cell first in memory.

#### See also

`@`, `2variable`, `2constant`.

#### Example

```session
/> create pair 2 cells allot
/> 100000. pair 2!
/> pair 2@ . .
1 -31072
/> pair @ .
1
```

## 2constant, 2variable, 2value, 2literal

Double-Number; in the `double` library (`lib double`).

#### Stack

```text
2constant   ( x1 x2 "name" -- )       name: ( -- x1 x2 )
2variable   ( "name" -- )             name: ( -- a-addr )
2value      ( x1 x2 "name" -- )       name: ( -- x1 x2 )
2literal    compiling: ( x1 x2 -- )   running: ( -- x1 x2 )
```

#### Description

`constant`, `variable`, `value` and `literal` for pairs of cells: a double number, usually. A `2variable`'s two
cells are read and written with `2@` and `2!`. A `2value` is changed with `to`. `2literal` is immediate and
compile only.

## 2dup, 2drop, 2swap, 2over

Core word set; in the ROM core.

#### Stack

```text
2dup    ( x1 x2 -- x1 x2 x1 x2 )
2drop   ( x1 x2 -- )
2swap   ( x1 x2 x3 x4 -- x3 x4 x1 x2 )
2over   ( x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2 )
```

#### Description

`dup`, `drop`, `swap` and `over` for pairs of cells. A pair is whatever takes two cells: a double number, or a
string's address and length.

## accept

Core word set; in the ROM core, always there.

#### Stack

```text
accept   ( c-addr +n1 -- +n2 )
```

#### Description

Reads a line from the keyboard into memory at *c-addr*, and leaves the number of characters it stored. The
person types, and can correct what is typed, as at the prompt; <kbd>ENTER</kbd> ends the line, and is not
stored. At most *+n1* characters are stored: anything typed beyond that is dropped.

#### See also

`key`, `refill`, `pad`, `read-line`, `>number`.

#### Example

Here `Hydra` was typed:

```text
/> : ask  ." name? " pad 20 accept  ." Hello, " pad swap type ;
/> ask
name? Hydra
Hello, Hydra
```

## again

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate, compile only.

#### Stack

Compiling: ( *dest* -- ). Running: ( -- ).

#### Description

Ends a loop that `begin` started, with a jump straight back to the `begin`: `begin ... again` repeats for ever.
The way out is `exit`, which leaves the whole definition, or a `throw`.

## ahead, cs-pick, cs-roll

Programming-Tools; in the `tools` library, which is loaded as HyForth starts. For writing new control-flow
words.

#### Stack

```text
ahead     compiling: ( -- orig )     running: ( -- )
cs-pick   compiling: ( destu ... dest0 u -- destu ... dest0 destu )
cs-roll   compiling: ( origu|destu ... u -- ... origu|destu )
```

#### Description

While a definition is compiled, the places that jumps come from and go to are kept on a control-flow stack:
`if` leaves an *orig*, which `then` resolves, and `begin` leaves a *dest*, which `until` or `again` uses.
`ahead` compiles a jump forward with nothing to test, leaving an *orig* for a later `then`. `cs-pick` copies
an item of the control-flow stack and `cs-roll` brings one to the top, as `pick` and `roll` do on the data
stack.

With them a control structure can be put together from parts: `while` is `if 1 cs-roll`, and `repeat` is
`again then`.

## align, aligned

Core word set; in the ROM core, always there.

#### Stack

```text
align     ( -- )
aligned   ( addr -- a-addr )
```

#### Description

On a machine where a cell must begin at an even address, `align` moves `here` up to the next one and
`aligned` rounds an address up to one. The 65C02 has no such rule: on HyForth `align` does nothing and
`aligned` leaves its address as it is. They are here so that programs written for other Forths load
unchanged.

#### See also

`here`, `cells`.

#### Example

```session
/> 7 aligned .
7
```

## allocate

Memory-Allocation; in the `memory` library (`lib memory`).

#### Stack

```text
allocate   ( u -- a-addr ior )
```

#### Description

Asks for *u* bytes of memory from the heap, and leaves their address, with an ior of 0. The bytes are not
cleared. If there is not that much free, the ior is −59, `allocate failed`, and the address means nothing.

The heap is a different place from data space: what is allocated can be given back, in any order, with
`free`, where `allot` only grows. It is for memory whose size is known only as the program runs, or which
is needed for a while.

#### See also

`free`, `resize`, `allot`, `buffer:`, `bank-window`.

#### Example

```session
/> lib memory
/> 100 allocate throw value m
/> m 100 char A fill  m 5 type
AAAAA
/> m free .
0
/> 40000 allocate nip .
-59
```

## allot

Core word set; in the ROM core, always there.

#### Stack

```text
allot   ( n -- )
```

#### Description

Sets aside *n* bytes of data space, by moving `here` on by *n*. The bytes are not cleared: they hold
whatever was there. A negative *n* gives space back.

With `create` it makes a table or a buffer: `create name 100 allot`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −8 | `dictionary full` | Fewer than *n* bytes are left |

#### See also

`here`, `create`, `buffer:`, `unused`, `cells`, `allocate`.

#### Example

```session
/> create buf 16 allot
/> buf 16 char * fill buf 16 type
****************
/> 40000 allot
dictionary full
```

## and, or, xor

Core word set; in the ROM core, always there.

#### Stack

```text
and   ( x1 x2 -- x3 )
or    ( x1 x2 -- x3 )
xor   ( x1 x2 -- x3 )
```

#### Description

The bitwise operations, on all 16 bits at once. Each bit of the result comes from the same bit of the two
cells: `and` gives 1 where both are 1, `or` where either is, and `xor` where they differ.

Because true is −1, all bits set, and false is 0, the same three words combine flags: `and` is "both", `or`
is "either". This works for well-formed flags only: 1 and 2 are both true, and `1 2 and` is 0. Make a value a
flag first, with `0<>`.

#### See also

`invert`, `lshift`, `0<>`, `nand`.

#### Example

```session
/> 12 10 and . 12 10 or . 12 10 xor .
8 14 6
/> $1234 $FF and .
52
/> 3 5 < 3 0 > and .
-1
```

## argc, arg

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( -- *n* ), ( *n* -- *c-addr u* )

#### Description

A script's arguments (0: its name)

## at-xy

Facility; in the `facility` library (`lib facility`).

#### Stack

( *u1 u2* -- )

#### Description

Cursor to column *u1*, row *u2* (from 0)

## bank!, bank@

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *bank* -- ), ( -- *bank* )

#### Description

Choose, and read, the bank there.

## bank-window

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( -- *addr* )

#### Description

$8000, where a RAM bank is seen.

## base

Core word set; in the ROM core, always there.

#### Stack

```text
base   ( -- a-addr )
```

#### Description

A variable: the number base, 2 to 36, in which numbers are read and printed. It starts as ten. `decimal` and
`hex` set it to 10 and 16, and any other base is stored: `2 base !`.

The base in force applies to what is typed too, the next `base !` included: after `hex`, `10 base !` leaves
the base at sixteen. To get back, use `decimal`, or write the number with its own base.

A number can carry its own base, whatever `base` is: `$FF` is hex, `%1010` binary, `#10` decimal, and `'A'`
a character's code. A `-` goes before the mark: `-$10`.

#### See also

`decimal`, `hex`, `.`, `>number`, `set-base`.

#### Example

```session
/> 255 hex . decimal
FF
/> 2 base ! 101 decimal .
5
/> $FF . %101 . #10 . -$10 .
255 5 10 -16
```

## beep

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Ring the bell.

## begin, until

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
begin   ( -- )
until   ( x -- )
```

#### Description

A loop that tests at its end. The words between `begin` and `until` are run; `until` then takes a flag from
the stack, and if it is false, 0, goes back to `begin`. When the flag is true the loop is over, and the words
after `until` are run. The words in the loop always run at least once.

`begin` also opens the other two loops: `begin` ... `while` ... `repeat`, which tests in the middle, and
`begin` ... `again`, which never ends by itself.

#### See also

`while`, `again`, `do`, `leave`, `exit`.

#### Example

```session
/> : countdown  begin dup . 1- dup 0= until drop ;
/> 5 countdown
5 4 3 2 1
```

## begin-structure, end-structure

Facility; in the `facility` library (`lib facility`).

#### Stack

( "name" -- *addr* 0 ), ( *addr n* -- )

#### Description

A structure's start and end.

## bin

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
bin   ( fam1 -- fam2 )
```

#### Description

In the standard, changes a way of opening into the same way for a binary file. The Hydra's files are all
plain bytes, with no difference between text and binary: `bin` leaves the value as it is. It is here so that
`r/o bin open-file` in a program from elsewhere works.

#### See also

`r/o`, `open-file`.

## bind, mount, unmount, newns

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( "args" -- )

#### Description

The namespace, as rc's (section 8.14)

## bitmap

The screen; in the `video` library (`lib video`).

#### Stack

( *width depth* -- )

#### Description

A bitmap under the text: 320 or 640 across, 1, 2, 4 or 8 bits a dot.

## bitmap-off

The screen; in the `video` library (`lib video`).

#### Stack

( -- )

#### Description

No bitmap.

## bl

Core word set; in the ROM core, always there.

#### Stack

```text
bl   ( -- char )
```

#### Description

A constant: 32, the code of a space ("blank"). It is what `word` and `fill` are most often given.

#### See also

`space`, `word`, `blank`.

#### Example

```session
/> bl .
32
```

## black, red, green, yellow, blue, magenta, cyan, white

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *c* )

#### Description

The colours 0 to 7.

## blank

String; in the `string` library (`lib string`).

#### Stack

```text
blank   ( c-addr u -- )
```

#### Description

Fills *u* bytes of memory with spaces: it is `bl fill`.

#### See also

`fill`, `erase`, `-trailing`.

## blk, scr

Block; in the `block` library (`lib block`).

#### Stack

```text
blk   ( -- a-addr )
scr   ( -- a-addr )
```

#### Description

Two variables. `blk` holds the number of the block being loaded, and is 0 when the source is not a block:
the keyboard, a file or a string. `scr` holds the number of the block most recently shown by `list`.

#### See also

`load`, `list`, `source-id`.

## block, buffer

Block; in the `block` library (`lib block`).

#### Stack

```text
block    ( u -- a-addr )
buffer   ( u -- a-addr )
```

#### Description

A **block** is 1,024 bytes of a file, the block file: block *u* is the bytes from *u* × 1,024 on. Blocks are
the old Forth way to keep source and data, a screen of 16 lines of 64 characters at a time, with no file
names.

`block` leaves the address of a buffer holding block *u*, reading it from the file if it is not in memory
already. A block beyond the end of the file reads as spaces. `buffer` leaves a buffer assigned to block *u*
without the reading, for a block that is about to be filled anyway.

There are two buffers, so the address is good only until the next `block` or `buffer` but one. After
changing what is in a buffer, say `update`, or the change is lost when the buffer is used for another
block.

The block file is `blocks.fb` in the directory that is current when a block is first asked for, made if it
is not there; `open-blocks` names another.

#### See also

`update`, `load`, `list`, `open-blocks`, `blk`.

#### Example

```session
/> lib block
/> s" /ram/work.fb" open-blocks
/> 0 block 1024 bl fill update flush
/> 1 block 1024 bl fill
/> s" .( block one ) 5 6 + ." 1 block swap move update flush
/> 1 load
block one 11
```

## >body

Core word set; in the ROM core, always there.

#### Stack

```text
>body   ( xt -- a-addr )
```

#### Description

Given the execution token of a word made by `create`, leaves the address of its data: the address the word
itself leaves when it is used. It is for code that has a word's token, from `'`, and wants its data.

For a word not made by `create` the result means nothing.

#### See also

`create`, `'`, `does>`.

#### Example

```session
/> create box 5 ,
/> ' box >body @ .
5
/> ' box >body box = .
-1
```

## bold, dim, underline, blink, reverse, plain

The terminal; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Styles; `plain` turns them all off.

## border

The screen; in the `video` library (`lib video`).

#### Stack

( *colour* -- )

#### Description

The border's colour.

## bright

Facility; in the `facility` library (`lib facility`).

#### Stack

( *c1* -- *c2* )

#### Description

A colour's bright version (8-15)

## buffer:

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
buffer:   ( u "name" -- )
name      ( -- a-addr )
```

#### Description

Defines a word that owns *u* bytes of data space, and leaves their address when it is used. The bytes are
not cleared. It is `create name u allot` in one word.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −8 | `dictionary full` | Fewer than *u* bytes are left |

#### See also

`create`, `allot`, `pad`, `allocate`.

#### Example

```session
/> 64 buffer: line
/> s" kept here" line swap move
/> line 9 type
kept here
```

## bye

Programming-Tools; in the ROM core, always there.

#### Stack

```text
bye   ( -- )
```

#### Description

Leaves HyForth, with the exit status 0. Everything defined in this run of it is gone.

When HyForth is the shell of the first window, as it is unless `/lib/shell` says otherwise, leaving it does
not leave the window empty: the system starts a fresh shell there.

#### See also

`exits`, `quit`, `marker`.

## c"

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate. Compile only.

#### Stack

```text
c"   ( "text<quote>" -- )       compiling
     ( -- c-addr )              when the definition runs
```

#### Description

Compiles the text up to the next `"` as a counted string: a byte holding its length, and then its
characters. When the word runs, the address of the length byte is left. `count` turns that into an address
and a length.

Counted strings are the older form, kept for `find` and `word`. New code uses `s"`.

#### See also

`s"`, `count`, `find`, `word`.

#### Example

```session
/> : label  c" counted" ;
/> label count type
counted
/> label c@ .
7
```

## c@, c!

Core word set; in the ROM core, always there.

#### Stack

```text
c@   ( c-addr -- char )
c!   ( char c-addr -- )
```

#### Description

`c@` leaves the byte at an address, as a number from 0 to 255: the cell's high byte is zero. `c!` stores the
low byte of the cell given, and ignores its high byte.

These are how strings, and any table of bytes, are read and written: a character is one byte.

#### See also

`@`, `count`, `c,`, `fill`.

#### Example

```session
/> variable v
/> $1234 v !
/> v c@ . v 1+ c@ .
52 18
/> 65 v c! v c@ emit
A
```

## case, of, endof, endcase

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate, compile only.

#### Stack

```text
case      ( -- )
of        ( x1 x2 -- | x1 )
endof     ( -- )
endcase   ( x -- )
```

#### Description

Chooses among several pieces of code by a value on the stack.

```text
case
   1 of ... endof
   2 of ... endof
   ...                  \ any other value: it is still on the stack here
endcase
```

Each `of` compares the value under it with the one given. If they are equal both are dropped, the code up to
`endof` runs, and control goes on after `endcase`. If not, the value given is dropped, the selector stays, and
the next `of` is tried. Code between the last `endof` and `endcase` runs when nothing matched, with the
selector still on the stack; `endcase` then drops it. That code must leave the selector, or something in its
place, for `endcase` to drop.

#### Example

```session
/> : name  case 1 of ." one" endof 3 of ." three" endof ." other" endcase ;
/> 3 name 7 name
threeother
```

## catch

Exception; in the `exception` library, which is loaded as HyForth starts.

#### Stack

```text
catch   ( i*x xt -- j*x 0 | i*x n )
```

#### Description

Runs the word whose token is given, as `execute` does, and watches for a `throw`.

If the word finishes in the ordinary way, `catch` leaves 0 above whatever the word left.

If a `throw` happens while the word runs, in it or in anything it calls, however deep, the word is
abandoned at that point: the return stack is put back as it was at the `catch`, and so is the data stack's
*depth*. `catch` then leaves the throw code. The cells that were the word's arguments are there in number,
but their values may have been changed: drop them.

So after `catch`, a zero means "it worked, and here are the results", and anything else is the reason it
did not.

Every error of the system is a throw, with a code of its own (see `throw`): `catch` is how a program
survives division by zero, a file that isn't there, or a word of its own that gives up.

#### See also

`throw`, `abort"`, `execute`, `quit`.

#### Example

```session
/> : safe/  ( n1 n2 -- n3 )  ['] / catch if 2drop 0 then ;
/> 10 2 safe/ .
5
/> 10 0 safe/ .
0
/> : risky  10 20 -99 throw ;
/> 1 2 ' risky catch .s
<3> 1 2 -99
```

## cd

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( "dir" -- )

#### Description

Change directory.

## cells, cell+, chars, char+

Core word set; in the ROM core, always there.

#### Stack

```text
cells   ( n1 -- n2 )
cell+   ( a-addr1 -- a-addr2 )
chars   ( n1 -- n2 )
char+   ( c-addr1 -- c-addr2 )
```

#### Description

Sizes, in bytes. `cells` gives the size of *n1* cells, which is twice *n1*, and `cell+` adds the size of one
cell, 2, to an address. `chars` gives the size of *n1* characters, which is *n1*, and `char+` adds 1.

Written with these, a program says what it means, and still works on a Forth whose cells are another size:
item *i* of a table of cells is at `table i cells +`.

#### See also

`allot`, `@`, `,`.

#### Example

```session
/> 1 cells . 3 cells . 5 chars .
2 6 5
/> 100 cell+ . 100 char+ .
102 101
```

## char, [char]

Core word set; in the ROM core, always there. `[char]` is immediate, and compile only.

#### Stack

```text
char     ( "name" -- char )
[char]   ( "name" -- )          compiling
         ( -- char )            when the definition runs
```

#### Description

`char` takes the next word of the line and leaves the code of its first character; the rest of the word is
ignored. `[char]` does the same inside a definition: it takes the next word as the definition is compiled,
and compiles the code as a number.

A character can also be written as a number in its own right, between single quotes: `'A'` is 65, at the
prompt and in a definition alike.

#### See also

`emit`, `bl`, `'`.

#### Example

```session
/> char A .
65
/> char Apple .
65
/> : star  [char] * emit ;
/> star
*
/> 'A' .
65
```

## circle, disc

The screen; in the `video` library (`lib video`).

#### Stack

( *x y r* -- )

#### Description

A circle, a filled circle.

## clear

The screen; in the `video` library (`lib video`).

#### Stack

( -- )

#### Description

Clear the bitmap.

## clear-line, clear-below

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Clear the rest of the line, of the screen.

## close-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
close-file   ( fileid -- ior )
```

#### Description

Closes a file. What was written to it is in it from then on, and the fileid means nothing any more. The ior
is 0, or −549, `bad file descriptor`, for a fileid that is not open.

#### See also

`open-file`, `flush-file`.

#### Example

```session
/> s" /rom/lib/shell" r/o open-file throw value fd
/> fd close-file .
0
/> fd close-file .
-549
```

## cmove, cmove>

String; in the `string` library (`lib string`).

#### Stack

```text
cmove    ( c-addr1 c-addr2 u -- )
cmove>   ( c-addr1 c-addr2 u -- )
```

#### Description

Copy *u* bytes from the first address to the second, one at a time in a set order: `cmove` from the lowest
address up, and `cmove>` from the highest down.

When the two areas don't overlap the order makes no difference. When they do, one order is right and the
other spreads a byte over the whole area: to move text toward higher addresses use `cmove>`, and toward
lower ones `cmove`. `move` chooses the right order by itself, and is the one to use unless the spreading is
what is wanted.

#### See also

`move`, `fill`.

#### Example

```session
/> lib string
/> create buf 8 allot
/> s" abcdefgh" buf swap cmove
/> buf buf 1+ 7 cmove>  buf 8 type
aabcdefg
/> buf buf 1+ 7 cmove  buf 8 type
aaaaaaaa
```

## code-banks

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *flag* -- )

#### Description

Compile definitions into banks (true, as it starts) or the dictionary (false)

## color, bgcolor

Facility; in the `facility` library (`lib facility`).

#### Stack

( *c* -- )

#### Description

The text's, the background's colour (0-15)

## compare

String; in the `string` library (`lib string`).

#### Stack

```text
compare   ( c-addr1 u1 c-addr2 u2 -- n )
```

#### Description

Compares two strings, character by character, by their codes. The result is 0 if they are the same, −1 if
the first comes before the second, and 1 if it comes after. When one string is the beginning of the other,
the shorter comes first.

The comparison is by code, so capitals come before small letters: `"ABC"` is before `"abc"`. `0=` after
`compare` is the test for equal strings.

#### See also

`search`, `=`, `ncompare`.

#### Example

```session
/> lib string
/> s" abc" s" abc" compare .
0
/> s" abc" s" abd" compare .
-1
/> s" abc" s" ab" compare .
1
```

## compile,

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
compile,   ( xt -- )
```

#### Description

Adds, to the definition being compiled, a call of the word whose token is given: what the compiler itself
does with each ordinary word it meets. It is used inside immediate words, which run while another
definition is being compiled.

#### See also

`postpone`, `[']`, `immediate`, `literal`.

#### Example

```session
/> : dup,  ['] dup compile, ; immediate
/> : square  dup, * ;
/> 6 square .
36
```

## [compile]

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate.

#### Stack

```text
[compile]   ( "name" -- )
```

#### Description

Compiles the next word of the definition as an ordinary call, even if it is immediate. It is the older way
to say what `postpone` says for an immediate word, and the standard keeps it only for old programs: write
`postpone`.

#### See also

`postpone`, `immediate`.

## complex

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *re im* -- *z* )

#### Description

A complex number.

## count

Core word set; in the ROM core, always there.

#### Stack

```text
count   ( c-addr1 -- c-addr2 u )
```

#### Description

Turns a counted string into an address and a length. A counted string is a byte holding a length, and then
that many characters. `count` leaves the address of the first character, and above it the length.

It is also a way to step through bytes: it leaves the next address, and the byte that was at this one.

#### See also

`type`, `word`, `c"`, `c@`.

#### Example

```session
/> create msg 3 c, char a c, char b c, char c c,
/> msg count type
abc
/> msg count . drop
3
```

## cr, space, spaces

Core word set; in the ROM core, always there.

#### Stack

```text
cr       ( -- )
space    ( -- )
spaces   ( n -- )
```

#### Description

`cr` starts a new line. `space` writes one space, and `spaces` writes *n* of them; for an *n* of 0 or less,
none.

#### See also

`emit`, `bl`, `.r`, `at-xy`.

#### Example

```session
/> 65 emit cr 66 emit
A
B
/> 67 emit 3 spaces 68 emit
C   D
```

## create

Core word set; in the ROM core, always there.

#### Stack

```text
create   ( "name" -- )
name     ( -- a-addr )
```

#### Description

Defines a word that, when it is used, leaves the address of the data space that follows it: the value
`here` had just after the word was made. `create` sets nothing aside itself. What follows it, `allot`, `,`
or `c,`, builds the word's data, and so a table, a buffer or a record is made.

With `does>`, `create` makes defining words: words that define other words, each with its own data and all
with the same behaviour.

#### See also

`does>`, `allot`, `,`, `variable`, `buffer:`, `>body`.

#### Example

```session
/> create primes 2 , 3 , 5 , 7 ,
/> : prime  cells primes + @ ;
/> 3 prime .
7
```

## ctl

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr1 u1 c-addr2 u2* -- )

#### Description

Write text *c-addr2 u2* to file *c-addr1 u1*, in one write.

## cursor-on, cursor-off

The terminal; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Show or hide the cursor.

## cursor-save, cursor-restore, cursor-off, cursor-on

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Save, restore, hide, show the cursor.

## cursor-up, cursor-down, cursor-right, cursor-left

Facility; in the `facility` library (`lib facility`).

#### Stack

( *n* -- )

#### Description

Move the cursor.

## d+, d-, m+, dnegate, dabs

Double-Number; in the `double` library (`lib double`).

#### Stack

```text
d+        ( d1 d2 -- d3 )
d-        ( d1 d2 -- d3 )
m+        ( d1 n -- d2 )
dnegate   ( d1 -- d2 )
dabs      ( d -- ud )
```

#### Description

Arithmetic on 32-bit numbers: the sum, the difference, a double plus a single cell, the negative, and the
absolute value. Results wrap at 32 bits without an error.

A number typed with a `.` at its end is a double: `100000. 1. d+ d.` prints 100001.

## d., d.r

Double-Number; in the `double` library (`lib double`).

#### Stack

```text
d.    ( d -- )
d.r   ( d n -- )
```

#### Description

`d.` prints a double in `base`, with a minus sign if it is negative and one space after it. `d.r` prints it at
the right of a field *n* characters wide, with no space after.

## d0=, d0<, d=, d<, du<, dmax, dmin

Double-Number; in the `double` library (`lib double`).

#### Stack

```text
d0=    ( d -- flag )
d0<    ( d -- flag )
d=     ( d1 d2 -- flag )
d<     ( d1 d2 -- flag )
du<    ( ud1 ud2 -- flag )
dmax   ( d1 d2 -- d3 )
dmin   ( d1 d2 -- d3 )
```

#### Description

The tests and comparisons of the cell words, for doubles: zero, negative, equal, less than (signed, and with
`du<` unsigned), and the larger and the smaller of two.

## d2*, d2/, d>s, m*/, 2rot

Double-Number; in the `double` library (`lib double`).

#### Stack

```text
d2*    ( d1 -- d2 )
d2/    ( d1 -- d2 )
d>s    ( d -- n )
m*/    ( d1 n1 +n2 -- d2 )
2rot   ( x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2 )
```

#### Description

`d2*` shifts a double left one bit, and `d2/` right one bit, keeping the sign. `d>s` is the double's low cell,
which is the same number if it fits in a cell. `m*/` multiplies a double by *n1* and divides by *n2*, which must
be positive, keeping a 48-bit product in between so that the scaling is exact. `2rot` is `rot` for pairs.

## decimal

Core word set; in the ROM core, always there.

#### Stack

```text
decimal   ( -- )
```

#### Description

Sets `base` to ten. It is the safe way back from any other base, because it does not depend on how a number
would be read.

#### See also

`hex`, `base`.

#### Example

```session
/> hex 10 decimal .
16
```

## defer, is, defer!, defer@, action-of

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
defer       ( "name" -- )
is          ( xt "name" -- )
defer!      ( xt2 xt1 -- )
defer@      ( xt1 -- xt2 )
action-of   ( "name" -- xt )
```

#### Description

`defer name` defines a word whose action can be changed. It should be given one before it is run.
`' word is name` makes *name* do what *word* does from then on; every definition that already calls *name*
follows the change, which is the point of it.

`action-of name` gives the execution token *name* now runs. `defer!` and `defer@` do the same as `is` and
`action-of` for a deferred word given by its execution token, *xt1*.

`is` and `action-of` work both at the prompt and inside a definition, where they take the name that follows
them in the definition.

#### Example

```session
/> defer greet  : hi ." hi " ;  : yo ." yo " ;
/> ' hi is greet greet  ' yo is greet greet
hi yo
```

## [defined], [undefined]

Programming-Tools; in the `tools` library, which is loaded as HyForth starts. Immediate.

#### Stack

```text
[defined]     ( "name" -- flag )
[undefined]   ( "name" -- flag )
```

#### Description

Take the next word of the line, and leave a flag: `[defined]` true if there is a word of that name, and
`[undefined]` true if there is not. Neither is an error for a name that isn't known.

With `[if]` they let a file supply a word only where the system lacks it, or load a library only if it is
needed.

#### See also

`[if]`, `find`, `lib`.

#### Example

```session
/> [defined] dup .
-1
/> [defined] nothere .
0
/> [undefined] 2rot [if] lib double [then]
/> [defined] 2rot .
-1
```

## definitions

Search-Order; in the `search` library (`lib search`).

#### Stack

( -- )

#### Description

New words go in the first word list of the order.

## delete-file, rename-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
delete-file   ( c-addr u -- ior )
rename-file   ( c-addr1 u1 c-addr2 u2 -- ior )
```

#### Description

`delete-file` removes the file named. `rename-file` gives the file named by the first string the name in
the second.

A file is renamed where it is: it stays in its directory, and only the last part of the new name is used.
To move a file to another directory, copy it and delete the original.

At the prompt, the two strings of `rename-file` can both be typed with `s"`: the two most recent such
strings are kept.

#### See also

`create-file`, `file-status`, the commands `rm` and `mv`.

#### Example

```session
/> s" /ram/a.txt" w/o create-file throw close-file throw
/> s" /ram/a.txt" s" b.txt" rename-file .
0
/> s" /ram/b.txt" file-status nip .
0
/> s" /ram/b.txt" delete-file .
0
/> s" /ram/b.txt" delete-file .
-544
```

## depth

Core word set; in the ROM core, always there.

#### Stack

```text
depth   ( -- +n )
```

#### Description

Leaves the number of cells that were on the stack before `depth` ran.

#### See also

`.s`, `ndepth`.

#### Example

```session
/> depth .
0
/> 10 20 30 depth .
3
```

## digits

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *addr* )

#### Description

The variable holding their precision (12)

## disasm

Bits, random numbers and the disassembler; in the `disasm` library (`lib disasm`).

#### Stack

( *addr u* -- )

#### Description

Disassemble *u* instructions from *addr* (`disasm`: Chapter 28)

## ?do

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate. Compile only.

#### Stack

```text
?do   ( n1 n2 -- )
```

#### Description

`do`, with a check: if the limit and the starting index are equal, the loop's words are not run at all, and
the program goes on after `loop`. Otherwise it is `do` exactly.

This is the one to use whenever the count comes from the program and might be 0, such as a length.

#### See also

`do`, `loop`, `leave`.

#### Example

```session
/> : stars  0 ?do [char] * emit loop ;
/> 5 stars
*****
/> 0 stars
```

## do, loop, +loop

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
do      ( n1 n2 -- )       n1 the limit, n2 the first index
loop    ( -- )
+loop   ( n -- )
```

#### Description

A counted loop. `do` takes a limit and a starting index. The words between `do` and `loop` are run with the
index at its starting value; `loop` then adds 1 to the index, and if it has not reached the limit, goes back
for another turn. So `10 0 do ... loop` runs ten times, with the index 0 to 9: the limit itself is never
used. Inside the loop, `i` leaves the index.

`+loop` takes its step from the stack each time round, in place of 1. With a positive step the loop ends
when the index reaches or passes the limit. With a negative step it counts down, and ends when the index
passes *below* the limit: this time the limit itself is used, so `0 10 do i . -3 +loop` shows 10, 7, 4, 1.

The index and the limit are kept on the return stack while the loop runs (see `>r`).

#### Notes

`do` always runs its words at least once. If the limit equals the starting index, as when a count turns out
to be 0, the loop does not skip: it runs 65,536 times, the index going all the way round. Use `?do` wherever
that can happen.

#### See also

`?do`, `i`, `leave`, `unloop`, `begin`.

#### Example

```session
/> : count-up  3 0 do i . loop ;
/> count-up
0 1 2
/> : by-threes  10 0 do i . 3 +loop ;
/> by-threes
0 3 6 9
/> : down  0 10 do i . -3 +loop ;
/> down
10 7 4 1
```

## does>

Core word set; in the ROM core. Immediate, compile only.

#### Stack

Running the word that contains it: ( -- ). Running a word it made: ( ... -- *addr* ... ).

#### Description

`does>` is used inside a defining word, after `create`, and splits the definition in two. What comes before it
runs when the defining word is used, and makes a new word; what comes after it is what every word made that way
does when it is run, starting with the address of that word's own data on the stack.

```text
: constant  create ,  does> @ ;
```

`5 constant five` runs `create ,`: a word `five` is made, with the cell 5 in its data. Running `five` then puts
the address of that cell on the stack and runs `@`.

The data address is the one `>body` gives for the word. `does>` changes the most recent word made by `create`.

#### Example

```session
/> : counter  create 0 ,  does> dup @ 1+ dup rot ! ;
/> counter hits  hits . hits . hits .
1 2 3
```

## drop

Core word set; in the ROM core, always there.

#### Stack

```text
drop   ( x -- )
```

#### Description

Removes the top cell of the stack. With nothing on the stack it is the error −4, `stack underflow`, reported
when the line has been run.

#### See also

`2drop`, `nip`, `dup`.

#### Example

```session
/> 1 2 drop .s
<1> 1
/> drop drop
stack underflow
```

## dump

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
dump   ( addr u -- )
```

#### Description

Shows *u* bytes of memory, from *addr*, eight to a line: the address of the line's first byte in hex, the
bytes in hex, and then the same bytes as characters, with a dot for any that can't be printed.

#### See also

`?`, `c@`, `type`, the command `xd`.

#### Example

```session
/> create msg  char H c, char y c, char d c, char r c, char a c, 0 c, 1 c, 255 c, 16 c,
/> msg 9 dump
3967: 48 79 64 72 61 00 01 FF  Hydra...
396F: 10                       .
```

## ?dup

Core word set; in the ROM core, always there.

#### Stack

```text
?dup   ( x -- 0 | x x )
```

#### Description

Copies the top cell if it is not zero, and leaves a zero as it is. It is for a value that is also a flag:
`?dup if ... then` runs the part between with the value, and leaves nothing behind when the value was zero,
where `dup if ... else drop then` would be needed without it.

#### See also

`dup`, `if`.

#### Example

```session
/> 5 ?dup .s
<2> 5 5
/> 2drop 0 ?dup .s
<1> 0
```

## dup

Core word set; in the ROM core, always there.

#### Stack

```text
dup   ( x -- x x )
```

#### Description

Makes a copy of the top cell of the stack, and leaves the copy on top of it. Almost every word uses up what
it takes, so `dup` is how a value is kept for a second use.

#### See also

`?dup`, `over`, `2dup`, `drop`.

#### Example

```session
/> 7 dup * .
49
/> 1 2 dup .s
<3> 1 2 2
```

## ekey, ekey?

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *u* ), ( -- *flag* )

#### Description

A key, special keys too; is one waiting?

## ekey>char, ekey>fkey

Facility; in the `facility` library (`lib facility`).

#### Stack

( *u* -- *u* false | *char* true ), ( *u* -- *u* false | *x* true )

#### Description

Is it a character? A special key?

## emit

Core word set; in the ROM core, always there.

#### Stack

```text
emit   ( x -- )
```

#### Description

Writes one character to standard output: the one whose code is *x*. Codes below 32 are the terminal's
control characters: 10 moves down a line, 7 rings the bell.

#### See also

`type`, `cr`, `char`, `key`.

#### Example

```session
/> 72 emit 105 emit
Hi
/> char A emit
A
```

## emit?

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *flag* )

#### Description

Can `emit` go on? (always)

## end-library

Search-Order; in the `search` library (`lib search`).

#### Stack

( *wid* -- )

#### Description

End it.

## environment?

Core word set; in the ROM core, always there.

#### Stack

```text
environment?   ( c-addr u -- false | i*x true )
```

#### Description

In the standard, asks the system about itself by name: the size of a counted string, the largest number,
which word sets are there. HyForth knows no queries: the result is false, whatever the string. The facts
themselves are in this chapter's introduction.

To find out whether a word is there, use `[defined]`.

#### See also

`[defined]`, `unused`, `libs`.

#### Example

```session
/> s" /counted-string" environment? .
0
```

## erase

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
erase   ( addr u -- )
```

#### Description

Sets *u* bytes of memory, from *addr* on, to zero: it is `0 fill`. Memory from `allot` is not cleared, so a
table that must start as zeros is erased first.

#### See also

`fill`, `blank`, `allot`.

#### Example

```session
/> create buf 8 allot
/> buf 8 char A fill
/> buf 4 erase
/> buf c@ . buf 3 + c@ . buf 4 + c@ .
0 0 65
```

## evaluate

Core word set; in the ROM core, always there.

#### Stack

```text
evaluate   ( i*x c-addr u -- j*x )
```

#### Description

Runs a string as a line of Forth. The string becomes the input for as long as it takes to interpret it, and
then the input goes back to what it was: the rest of the line that `evaluate` was on. What the string's words
do to the stack is what `evaluate` does to it.

The string is interpreted, or compiled if a definition is open, exactly as typed text would be: it can
define words, and a word in it that isn't known is the same error as at the prompt.

#### See also

`included`, `execute`, `source`, `parse`.

#### Example

```session
/> s" 2 3 + ." evaluate
5
/> : thirty  s" 10 20 +" evaluate ;
/> thirty .
30
/> s" nothere" evaluate
nothere ?
```

## execute

Core word set; in the ROM core, always there.

#### Stack

```text
execute   ( i*x xt -- j*x )
```

#### Description

Runs the word whose execution token is on top of the stack, with the rest of the stack as the word finds
it. A token comes from `'`, `[']`, `:noname`, `find` or `search-wordlist`.

Tokens are numbers, so they can be stored: a table of tokens, indexed and `execute`d, is how a program
chooses one of several actions without a chain of `if`s.

#### Notes

The token is not checked. A number that is not a word's token sends the processor to that address, and what
follows is most likely the end of this HyForth.

#### See also

`'`, `:noname`, `defer`, `compile,`, `catch`.

#### Example

```session
/> : hi  ." hi " ;  : bye-bye  ." bye " ;
/> create acts  ' hi ,  ' bye-bye ,
/> : act  cells acts + @ execute ;
/> 0 act 1 act
hi bye
```

## exit

Core word set; in the ROM core, always there. Compile only.

#### Stack

```text
exit   ( -- )
```

#### Description

Returns from the word being run, at once, to the word that called it. The rest of the definition is not
run. Inside a `do` loop, `unloop` must come first; inside a `begin` loop, nothing is needed.

What has been put on the return stack with `>r` must be taken off before `exit`, or the return goes astray.

#### Notes

Typed at the prompt, where there is no word to return from, `exit` ends HyForth itself.

#### See also

`unloop`, `leave`, `;`, `quit`, `bye`.

#### Example

```session
/> : half  dup 1 and if drop ." odd" exit then 2/ . ;
/> 10 half
5
/> 7 half
odd
```

## exits

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *n* -- )

#### Description

Leave HyForth with status *n*.

## +field, field:, cfield:

Facility; in the `facility` library (`lib facility`).

#### Stack

( *n1 n2* "name" -- *n3* ), ( *n1* "name" -- *n2* )

#### Description

Its fields.

## file-position, reposition-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
file-position     ( fileid -- ud ior )
reposition-file   ( ud fileid -- ior )
```

#### Description

A file's place is where its next read or write will happen, counted in bytes from its start. `file-position`
leaves it, and `reposition-file` sets it. The place is a double number, because a file can be longer than
65,535 bytes: write `0.` for the start.

#### See also

`file-size`, `read-file`, `d.`.

#### Example

```session
/> s" /rom/lib/shell" r/o open-file throw value fd
/> 5. fd reposition-file .
0
/> pad 5 fd read-file 2drop  pad 5 type
forth
/> fd file-position . . .
0 0 10
/> fd close-file drop
```

## file-size, resize-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
file-size     ( fileid -- ud ior )
resize-file   ( ud fileid -- ior )
```

#### Description

`file-size` leaves the length of an open file in bytes, as a double. `resize-file` sets it: a file made
shorter loses what was beyond the new end, and one made longer gains bytes whose contents are not defined.

#### See also

`file-position`, `file-status`, `delete-file`.

#### Example

```session
/> s" /ram/g.txt" w/o create-file throw value fd
/> s" 0123456789" fd write-file throw
/> fd file-size . . .
0 0 10
/> 4. fd resize-file .
0
/> fd file-size . . .
0 0 4
/> fd close-file drop
```

## file-status

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
file-status   ( c-addr u -- x ior )
```

#### Description

Asks about a file by name, without opening it. The ior is 0 if the file is there, and −544, `not found`, if
it is not: this is the test for whether a file exists. When it is there, *x* is its mode, the permission
bits the system keeps for it (438, octal 666, for a file that can be read and written).

#### See also

`open-file`, `file-size`, `open-dir`.

#### Example

```session
/> s" /rom/lib/shell" file-status nip .
0
/> s" /rom/nothere" file-status nip .
-544
```

## fill

Core word set; in the ROM core, always there.

#### Stack

```text
fill   ( c-addr u char -- )
```

#### Description

Stores the character in each of *u* bytes of memory, from *c-addr* on. With a *u* of 0 it does nothing.

#### See also

`erase`, `blank`, `move`.

#### Example

```session
/> create buf 8 allot
/> buf 8 char - fill
/> buf 3 char * fill
/> buf 8 type
***-----
```

## find

Core word set; in the ROM core, always there.

#### Stack

```text
find   ( c-addr -- c-addr 0 | xt 1 | xt -1 )
```

#### Description

Looks up a word whose name is given as a counted string. If it is not found, the string's address is left
with 0 above it. If it is found, its execution token is left, with 1 above it for an immediate word and −1
for an ordinary one.

`find` is the old way to search the dictionary. A counted string is awkward to make outside a definition;
`'` at the prompt, `[defined]` in a file, and `search-wordlist`, which takes an address and a length, are
easier.

#### See also

`'`, `[defined]`, `search-wordlist`, `c"`.

#### Example

```session
/> : name  c" dup" ;
/> name find . ' dup = .
-1 -1
/> : none  c" nothere" ;
/> none find . count type
0 nothere
```

## flush-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
flush-file   ( fileid -- ior )
```

#### Description

Makes sure that everything written to the file so far has reached the disk, without closing it. A program
that keeps a file open for a long time, a log for instance, calls it after each thing that must not be lost
if the power goes.

#### See also

`close-file`, `write-file`.

## form

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *rows cols* )

#### Description

The window's size.

## forth-wordlist

Search-Order; in the `search` library (`lib search`).

#### Stack

( -- *wid* )

#### Description

The FORTH word list.

## free

Memory-Allocation; in the `memory` library (`lib memory`).

#### Stack

```text
free   ( a-addr -- ior )
```

#### Description

Gives back memory that `allocate` or `resize` handed out. The address must be exactly one that they gave,
and it must not be used afterward. The ior is 0, or −60, `free failed`, for an address that is not an
allocated block's: freeing the same block twice is that error.

#### See also

`allocate`, `resize`.

## get-current, set-current

Search-Order; in the `search` library (`lib search`).

#### Stack

( -- *wid* ), ( *wid* -- )

#### Description

Where new words go.

## get-dir, set-dir

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr1 u1* -- *c-addr2 u2* ), ( *c-addr u* -- *ior* )

#### Description

The current directory.

## get-order, set-order

Search-Order; in the `search` library (`lib search`).

#### Stack

( -- *widn ... wid1 n* ), ( *widn ... wid1 n* -- )

#### Description

The search order.

## getenv, setenv, unsetenv

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *c-addr u* -- *c-addr2 u2* ), ( *c-addr1 u1 c-addr2 u2* -- ), ( *c-addr u* -- )

#### Description

Environment variables.

## gpio

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( *pin* -- *level* )

#### Description

A pin's level, 0 or 1.

## gpio!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( *pin level* -- )

#### Description

Make a pin an output, and set it.

## gpio-ca1!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( *rise?* -- )

#### Description

CA1 counts rises (true) or falls (false)

## gpio-ca2!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( *n* -- )

#### Description

CA2: 0 low, 1 high, -1 an input.

## gpio-ddr!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( *byte* -- )

#### Description

Set all eight directions (bit *n* 1: pin *n* an output)

## gpio-in, gpio-out

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( *pin* -- )

#### Description

Make a pin an input, or an output.

## gpio-port

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( -- *byte* )

#### Description

All eight pins at once.

## gpio-port!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( *byte* -- )

#### Description

Set all the outputs at once.

## gpio-state

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( -- *c-addr u* )

#### Description

The `ctl` file's text.

## gpio-wait

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

( -- *count* )

#### Description

Wait for CA1's next change: the changes so far.

## here

Core word set; in the ROM core, always there.

#### Stack

```text
here   ( -- addr )
```

#### Description

Leaves the address of the next free byte of data space: where `,`, `c,` and `allot` will put what they are
given, and where the next `create`d word's data will begin.

Data space is the dictionary, in the task's own memory: it holds every word's name, and the data of
variables and of `create`d words. It ends at `$7F00`, and `unused` says how much is left. The code of colon
definitions is not here: it is compiled into RAM banks of HyForth's own, so a long program doesn't use data
space up. Defining any word still moves `here`, by the size of its name.

#### See also

`allot`, `,`, `unused`, `create`, `code-banks`.

#### Example

```session
/> here 10 allot here swap - .
10
```

## hex

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
hex   ( -- )
```

#### Description

Sets `base` to sixteen: from then on numbers are read and printed in hexadecimal, until `decimal`. The
digits above 9 may be typed as capitals or small letters, and are printed as capitals.

For a single hex number among decimal ones, the `$` mark is simpler, and doesn't change the base.

#### See also

`decimal`, `base`, `dump`.

#### Example

```session
/> hex ff . 10 . decimal
FF 10
/> $ff .
255
```

## i, j

Core word set; in the ROM core, always there. Compile only.

#### Stack

```text
i   ( -- n )
j   ( -- n )
```

#### Description

Inside a `do` loop, `i` leaves the loop's index for this turn. Inside a loop that is itself inside another,
`j` leaves the index of the outer one.

They read the return stack, where the loops keep their numbers. So they are right only directly in the
loop's own words: not in a word the loop calls, and not after a `>r` in the loop that has not been matched
by its `r>`. Used outside a loop, `i` leaves whatever is on the return stack.

#### See also

`do`, `?do`, `r@`, `unloop`.

#### Example

```session
/> : grid  2 0 do 3 0 do j . i . space loop cr loop ;
/> grid
0 0  0 1  0 2
1 0  1 1  1 2
```

## if, else, then

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
if     ( x -- )
else   ( -- )
then   ( -- )
```

#### Description

A choice, inside a definition. `if` takes the top of the stack as a flag. If it is true, which is any value
but 0, the words between `if` and `else` are run, and then those after `then`. If it is 0, the words between
`else` and `then` are run instead. `else` and its part can be left out: then a false flag skips straight to
`then`.

`then` is where the two paths join, not what happens "next": it reads better as "and then, either way".
Choices can be put inside one another to any depth, each `if` with its own `then`.

The flag is used up. To test a value and keep it, `dup` it first, or use `?dup`.

#### Notes

These can't be used at the prompt, outside a definition: `if: compile only`. For a choice while a file is
loading, there is `[if]`.

An `if` with no `then` is not reported when the definition ends, and the word goes wrong when it is run.

#### See also

`case`, `?dup`, `0=`, `[if]`, `begin`.

#### Example

```session
/> : sign  dup 0< if ." negative" else 0= if ." zero" else ." positive" then then ;
/> -5 sign
negative
/> 0 sign
zero
/> : warn  100 > if ." too big!" then ;
/> 50 warn 500 warn
too big!
```

## [if], [else], [then]

Programming-Tools; in the `tools` library, which is loaded as HyForth starts. Immediate.

#### Stack

```text
[if]     ( flag -- )
[else]   ( -- )
[then]   ( -- )
```

#### Description

A choice made as the text is read, not when a word runs. `[if]` takes a flag. If it is true, the text goes
on being interpreted, and at `[else]` everything is skipped up to `[then]`. If it is false, everything up
to `[else]` (or to `[then]`, if there is no `[else]`) is skipped, as if it were not there.

What is skipped is not looked at beyond finding the matching `[else]` and `[then]`, so it may hold
definitions, and words the system does not have. In a file the parts may run over many lines, and the three
may be nested.

They work at the prompt and in a file. Inside a colon definition, they choose which text is *compiled*.

#### See also

`[defined]`, `if`, `included`.

#### Example

```session
/> 1 [if] .( yes) [else] .( no) [then]
yes
/> 0 [if] .( yes) [else] .( no) [then]
no
/> 0 [if] .( skipped) [then] .( after)
after
```

## immediate

Core word set; in the ROM core, always there.

#### Stack

```text
immediate   ( -- )
```

#### Description

Marks the word most recently defined as immediate. An immediate word is run, not compiled, when the
compiler meets it inside a definition: so it can do work at compile time, and can compile whatever it
chooses into the definition. `if`, `do`, `."` and `;` are immediate words.

It is written straight after the `;` of the word it marks.

#### See also

`postpone`, `[`, `literal`, `compile,`, `[compile]`.

#### Example

```session
/> : shout  ." compiling! " ; immediate
/> : quiet  shout ;
compiling!
/> quiet
```

## include-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
include-file   ( i*x fileid -- j*x )
```

#### Description

Loads Forth source from a file that is already open: its lines are interpreted from the file's current
place to its end, and the file is then closed. `included`, which takes a name, is the usual way; this one is
for a file that the program has opened and looked at first.

#### See also

`included`, `open-file`, `evaluate`.

#### Example

```session
/> s" /ram/inc.fs" w/o create-file throw value fd
/> s" .( from the file ) 1 2 + ." fd write-line throw  fd close-file throw
/> s" /ram/inc.fs" r/o open-file throw include-file
from the file 3
```

## included, include

File Access; in the ROM core, always there.

#### Stack

```text
included   ( i*x c-addr u -- j*x )
include    ( i*x "name" -- j*x )
```

#### Description

Load a file of Forth source: each of its lines is interpreted, in turn, as if it had been typed. `included`
takes the file's name as a string, and `include` takes it from the line. When the file ends, the input goes
back to where it was.

A file can include others, to eight deep. A line of a file can be up to 128 characters long.

An error in the file stops the loading and is reported with the file's name and the number of the line:
`/ram/bad.fs:1: nothere ?`. What the file had defined before the error stays defined.

Each loading runs the file again and defines its words again. To load a file only if it has not been loaded
already, use `required`, or for a library, `lib`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −512 − *code* | *name*`: not found`, or the system's other messages | The file can't be opened |
| −258 | `sources nested too deeply` | More than eight files deep |

#### See also

`required`, `lib`, `evaluate`, `marker`, `include-file`.

#### Example

```session
/> s" /ram/t.fs" w/o create-file drop
/> dup s" : loaded 77 ; .( loading) " rot write-line drop close-file drop
/> include /ram/t.fs
loading
/> loaded .
77
/> include /ram/nothere.fs
/ram/nothere.fs: not found
```

## int?, fixed?, rational?, complex?

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *flag* ) N: ( *x* -- )

#### Description

Its kind.

## invert

Core word set; in the ROM core, always there.

#### Stack

```text
invert   ( x1 -- x2 )
```

#### Description

Turns every bit of the cell over. Of a well-formed flag it is "not": `true invert` is false. Of any other
number it is not: `5 invert` is −6, which is still true. For "is zero", which is the "not" of any value, use
`0=`.

#### See also

`0=`, `negate`, `xor`.

#### Example

```session
/> 0 invert . 5 invert .
-1 -6
/> 3 5 < invert .
0
```

## ior>text

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *ior* -- *c-addr u* )

#### Description

An error's message.

## k-resize, k-focus

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *x* )

#### Description

(Not keys: the window was resized; another window of the group was chosen)

## k-shift-mask, k-ctrl-mask, k-alt-mask

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *x* )

#### Description

Added to a key typed with them held.

## k-up, k-down, k-left, k-right, k-home, k-end, k-prior, k-next, k-insert, k-delete, k-f1, k-f12

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *x* )

#### Description

The special keys, as `ekey>fkey` gives them.

## key

Core word set; in the ROM core, always there.

#### Stack

```text
key   ( -- char )
```

#### Description

Waits for a key, and leaves its character's code. The key is not shown on the screen, and no
<kbd>ENTER</kbd> is needed: the keyboard is read a key at a time.

Only keys that send one character are whole in one `key`: an arrow or a function key sends several, and
each `key` gives the next. For those, the Facility library has `ekey`.

#### See also

`key?`, `ekey`, `accept`, `emit`.

#### Example

Here <kbd>A</kbd> was pressed:

```text
/> key .
65
```

## key?

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *flag* )

#### Description

Is a key waiting?

## leave, unloop

Core word set; in the ROM core, always there. Compile only.

#### Stack

```text
leave    ( -- )
unloop   ( -- )
```

#### Description

`leave` ends the innermost `do` loop at once: the loop's numbers are thrown away, and the program goes on
after `loop`. It is usually inside an `if`.

`unloop` throws away the numbers of the innermost loop and nothing more. It is needed before `exit` inside a
loop, because the loop's numbers are on the return stack, on top of the address `exit` must return to. For
a loop inside a loop, one `unloop` for each.

#### See also

`do`, `exit`, `i`.

#### Example

```session
/> : to-five  10 0 do i 5 = if leave then i . loop ." done" ;
/> to-five
0 1 2 3 4 done
/> : find-five  10 0 do i 5 = if unloop exit then i . loop ." not reached" ;
/> find-five
0 1 2 3 4
```

## -lib

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
-lib   ( "name" -- )
```

#### Description

Hides a loaded library: its words are no longer found, though it stays in memory, and words already
compiled that use it go on working. `lib` with the same name brings them back.

This settles a clash, when two libraries have a word of the same name and the other one's is wanted.

#### See also

`lib`, `libs`, `marker`.

#### Example

```session
/> lib bits
/> -lib bits
/> libs
forth coreext exception file tools shell (bits)
```

## lib

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
lib   ( "name" -- )
```

#### Description

Makes a library's words available. If the library is not loaded, it is loaded: `lib` looks in `/lib/forth`
for *name*`.fl`, a library compiled in advance, and failing that for *name*`.fs`, a file of Forth source. If
it is loaded already, nothing is loaded again; and if `-lib` had hidden it, its words are found once more.

So `lib` can be written at the top of every file that needs a library, at no cost.

Most of HyForth is in libraries: the ROM core holds only the Core word set, and each other word set
(`double`, `string`, `facility`, `search` ...) and each of the Hydra's own (`hydra`, `numbers`, `sound`,
`video`, `gpio` ...) is loaded when it is asked for. Every entry of this chapter names its word's library.
A compiled library is small and loads at once.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −512 − *code* | *name*`.fs: not found` | There is no library of that name |
| −259 | `not this forth's library` | A `.fl` file made for another version of HyForth |

#### See also

`libs`, `-lib`, `required`, `included`.

#### Example

```session
/> lib bits
/> lib nothere
nothere.fs: not found
```

## library

Search-Order; in the `search` library (`lib search`).

#### Stack

( "name" -- *wid* )

#### Description

Start a library of your own: a new word list, first in the order.

## libs

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
libs   ( -- )
```

#### Description

Lists the libraries that are loaded, in the order they were loaded. `forth` is the ROM core itself. A
library that `-lib` has hidden is shown in parentheses.

#### See also

`lib`, `-lib`, `words`.

#### Example

```session
/> libs
forth coreext exception file tools shell
/> lib bits
/> libs
forth coreext exception file tools shell bits
```

## line, box, bar

The screen; in the `video` library (`lib video`).

#### Stack

( *x0 y0 x1 y1* -- )

#### Description

A line, a rectangle, a filled rectangle.

## list

Block; in the `block` library (`lib block`).

#### Stack

```text
list   ( u -- )
```

#### Description

Shows block *u* as a screen: sixteen numbered lines, 0 to 15, of 64 characters each. It also stores *u* in
`scr`.

#### See also

`block`, `load`, `scr`.

## literal

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
literal   ( x -- )         compiling
          ( -- x )         when the definition runs
```

#### Description

Takes a number from the stack as the definition is compiled, and compiles it into the definition, so that
the word leaves that number when it runs. The number is usually worked out between `[` and `]`.

#### See also

`[`, `2literal`, `sliteral`, `nliteral`, `[']`.

#### Example

```session
/> : five  [ 2 3 + ] literal ;
/> five .
5
```

## load, thru

Block; in the `block` library (`lib block`).

#### Stack

```text
load   ( i*x u -- j*x )
thru   ( i*x u1 u2 -- j*x )
```

#### Description

`load` interprets block *u* as Forth source: its 1,024 characters are read as one long line (a `\` comment
ends at the end of its 64-character line). `thru` loads each block from *u1* to *u2* in turn.

Block 0 can't be loaded: by custom it holds a description of the file. `0 load` is the error −35.

#### See also

`block`, `list`, `included`, `blk`.

#### Example

```session
/> lib block
/> s" /ram/two.fb" open-blocks
/> 0 block 1024 bl fill update flush
/> 1 block 1024 bl fill  s" .( one ) " 1 block swap move update flush
/> 2 block 1024 bl fill  s" .( two ) " 2 block swap move update flush
/> 1 2 thru
one two
```

## lshift, rshift

Core word set; in the ROM core, always there.

#### Stack

```text
lshift   ( x1 u -- x2 )
rshift   ( x1 u -- x2 )
```

#### Description

Shift the bits of *x1* by *u* places, left or right, filling with zeros. `rshift` is a logical shift: the
sign bit is not kept, so `-1 1 rshift` is 32767. A shift of 16 places or more leaves 0.

For a right shift of a signed number by one place, keeping its sign, use `2/`.

#### See also

`2*`, `2/`, `and`, `nlshift`.

#### Example

```session
/> 1 4 lshift . 256 4 rshift .
16 16
/> -1 1 rshift .
32767
/> $1234 8 rshift .
18
```

## marker

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
marker   ( "name" -- )
name     ( -- )
```

#### Description

Defines a word that remembers the dictionary as it is at that moment. When that word is run, everything
defined after it, and the word itself, is forgotten, and the space is free again.

A program being worked on begins with a marker: running the marker and loading the file again replaces the
old version without piling one copy on another.

#### See also

`words`, `included`, `-lib`.

#### Example

```session
/> marker fresh
/> : temp  99 ;
/> temp .
99
/> fresh
/> ' temp
temp ?
```

## min, max

Core word set; in the ROM core, always there.

#### Stack

```text
min   ( n1 n2 -- n3 )
max   ( n1 n2 -- n3 )
```

#### Description

The lesser, and the greater, of two signed numbers. `0 max` keeps a number from going below zero, and
`limit min` from going above a limit.

The comparison is signed: 65535 is −1, and is less than 1.

#### See also

`<`, `within`, `dmin`, `dmax`.

#### Example

```session
/> 3 9 min . 3 9 max .
3 9
/> -1 1 min .
-1
/> 150 0 max 100 min .
100
```

## =mkdir

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr u mode* -- *ior* )

#### Description

Make a directory.

## move

Core word set; in the ROM core, always there.

#### Stack

```text
move   ( addr1 addr2 u -- )
```

#### Description

Copies *u* bytes from *addr1* to *addr2*. The two parts may overlap, in either direction: the bytes arrive
as they were before the copy began, as if they had all been read first.

#### See also

`cmove`, `cmove>`, `fill`.

#### Example

```session
/> create buf 8 allot
/> s" abcdefgh" buf swap move
/> buf buf 2 + 6 move
/> buf 8 type
ababcdef
```

## ms

Facility; in the `facility` library (`lib facility`).

#### Stack

( *u* -- )

#### Description

Wait *u* milliseconds.

## >n

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *c-addr u* -- *flag* ) N: ( -- *x* | )

#### Description

Read a number from a string.

## n+, n-, n*, n/

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *x y* -- *z* )

#### Description

Exactly (`n/` gives fractions)

## n., n.base

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *x* -- ), ( *c-addr u* -- ) N: ( *x* -- )

#### Description

Print in the base; in a base named.

## n.s

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- )

#### Description

Show the number stack.

## n/mod

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *x y* -- *rem quot* )

#### Description

Whole-number division.

## n=, n<, n>, n0=, n0<

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *flag* ) N: ( ... -- )

#### Description

Tests.

## n>r, nr>

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
n>r   ( i*x +n -- ) ( R: -- i*x +n )
nr>   ( -- i*x +n ) ( R: i*x +n -- )
```

#### Description

Move several cells to the return stack and back, in one go. `n>r` takes a count and that many cells below
it, and puts them all, with the count, on the return stack; `nr>` brings them back as they were, the count
on top.

They follow the rules of `>r` and `r>`: a pair in the same definition, not across a loop's boundary.

#### Notes

They are for use inside a definition only. Typed at the prompt, `n>r` upsets the interpreter's own return
stack, and HyForth stops.

#### See also

`>r`, `2>r`, `save-input`.

#### Example

```session
/> : hide-three  1 2 3 3 n>r depth . nr> .s ;
/> hide-three
0 <4> 1 2 3 3
```

## n>s, n>d

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *n* ) N: ( *x* -- ), ( -- *d* ) N: ( *x* -- )

#### Description

To Forth's stack.

## n>str

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *c-addr u* ) N: ( *x* -- )

#### Description

Takes the number on top of the number stack and gives its text, in the base that is set, as `n.` would print
it but without the space after it. The text is in a transient buffer: use it or copy it before the next number
is converted.

## name>string, name>interpret, name>compile

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
name>string      ( nt -- c-addr u )
name>interpret   ( nt -- xt | 0 )
name>compile     ( nt -- x xt )
```

#### Description

Take a name token, as `traverse-wordlist` gives, apart.

`name>string` leaves the word's name, as a string in the dictionary itself.

`name>interpret` leaves the execution token for the word's behaviour at the prompt.

`name>compile` leaves what is needed to compile the word: a cell *x* and a token *xt*, such that `execute`
of *xt*, with *x* under it, does what the compiler does on meeting the word. For an ordinary word that is
its own token and `compile,`; for an immediate word, its token and `execute`.

On HyForth a name token is the address of the word's header in the dictionary.

#### See also

`traverse-wordlist`, `find`, `compile,`, `execute`.

## nand, nor, nxor, ninvert

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N:

#### Description

Bits.

## nbit?

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *u* -- *flag* ) N: ( *x* -- )

#### Description

A bit.

## nbytes, nfrom-bytes

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *c-addr u* ) N: ( *x* -- ), ( *c-addr u* -- ) N: ( -- *x* )

#### Description

The stored form's bytes.

## ncompare

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- -1 | 0 | 1 ) N: ( *x y* -- )

#### Description

Compare.

## ndepth

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *u* )

#### Description

How many numbers.

## ndrop, ndup, nover, nswap, nrot

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
ndrop   N: ( x -- )
ndup    N: ( x -- x x )
nover   N: ( x1 x2 -- x1 x2 x1 )
nswap   N: ( x1 x2 -- x2 x1 )
nrot    N: ( x1 x2 x3 -- x2 x3 x1 )
```

#### Description

`drop`, `dup`, `over`, `swap` and `rot` for the number stack. A number there is one item whatever its size: an
integer of a thousand digits, a fraction, a complex number.

## negate, abs

Core word set; in the ROM core, always there.

#### Stack

```text
negate   ( n1 -- n2 )
abs      ( n -- u )
```

#### Description

`negate` leaves the number with its sign changed: 0 − *n1*. `abs` leaves the number without its sign.

−32768 has no positive counterpart in 16 bits: `negate` and `abs` both leave it as it is, and what `abs`
leaves is right as the unsigned number 32,768.

#### See also

`dnegate`, `dabs`, `invert`.

#### Example

```session
/> 5 negate . -5 abs .
-5 5
/> -32768 abs u.
32768
```

## nfib

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *n* -- *f* )

#### Description

A Fibonacci number.

## nformat

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *c-addr u* -- ) N: ( *x1 ... xn* -- )

#### Description

Print a format, each `{}` a number.

## nip, tuck

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
nip    ( x1 x2 -- x2 )
tuck   ( x1 x2 -- x2 x1 x2 )
```

#### Description

`nip` removes the second cell: it is `swap drop`. `tuck` puts a copy of the top cell under the second: it is
`swap over`.

#### See also

`drop`, `over`, `swap`.

#### Example

```session
/> 1 2 nip .s
<1> 2
/> drop 1 2 tuck .s
<3> 2 1 2
```

## nliteral, nconstant, nvariable, nvalue, n@, n!

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nliteral    compiling: N: ( x -- )    running: N: ( -- x )
nconstant   ( "name" -- ) N: ( x -- )           name: N: ( -- x )
nvariable   ( "name" -- )                       name: ( -- addr )
nvalue      ( "name" -- ) N: ( x -- )           name: N: ( -- x )
n@          ( addr -- ) N: ( -- x )
n!          ( addr -- ) N: ( x -- )
```

#### Description

The number stack's forms of `literal`, `constant`, `variable`, `value`, `@` and `!`. A number written inside a
definition is compiled by itself, as a cell's is; `nliteral`, which is immediate and compile only, compiles one
that was worked out while compiling. An `nvariable` holds one number of any size, which `n@` copies to the
number stack and `n!` stores from it. An `nvalue` is changed with `to`.

#### Example

```session
/> lib numbers
/> 1/3 nconstant third  nvariable acc  third third n+ acc n!  acc n@ n.
2/3
/> : big 100000000000 ;  big big n* n.
10000000000000000000000
```

## nlshift, nrshift

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *u* -- ) N: ( *x* -- *y* )

#### Description

Shift.

## nnegate, nabs, ngcd, npow

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N:

#### Description

Negate; size; greatest common divisor; a power.

## :noname

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
:noname   ( -- xt )
```

#### Description

Starts a definition with no name. It is compiled as a `:` definition is, up to `;`, and when it is finished
its execution token is left on the stack. The token is all there is to run it by: keep it in a constant, a
table or a `defer`red word, or `execute` it.

#### See also

`:`, `execute`, `defer`, `'`.

#### Example

```session
/> :noname  dup * ;  constant squarer
/> 7 squarer execute .
49
```

## note, note-group

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *task n* -- ), ( *group n* -- )

#### Description

Send a note.

## note-of

Sound; in the `sound` library (`lib sound`).

#### Stack

( *c-addr u* -- *n* )

#### Description

A note's number from its name: `s" A4" note-of` is 69.

## nrandom, nseed

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *n* -- *r* ), ( *u* -- )

#### Description

Random numbers.

## nsqrt, nexp, nlog, nsin, ncos, ntan, natan, npi

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N:

#### Description

The math functions.

## >number

Core word set; in the ROM core. Converts the digits at the start of a string, adding them to a double.

#### Stack

```text
>number  ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
```

#### Description

*ud1* is the number so far: 0 to start a new one. Characters are taken from the left of the string for as long
as each is a digit in the current `base`, and for each one *ud* becomes *ud* × `base` + the digit's value. The
digits are `0` to `9`, then the letters `A` to `Z` for 10 to 35; small letters count as capitals. A character
whose value is not less than `base` is not a digit, and conversion stops at it: in decimal `a` stops it, in hex
`g` does.

*ud2* is the result, and *c-addr2 u2* is what was not converted: the address of the first character that isn't
a digit, and the number of characters from there to the end. *u2* is 0 if the whole string was digits.

Nothing else is understood. A sign, a prefix (`$`, `#` or `%`) and a decimal point all stop the conversion like
any other non-digit; an empty string converts nothing and returns *ud1* unchanged. Signs and prefixes are the
text interpreter's business: it deals with them itself, and calls this word's code for the digits. A string
with no digits is not an error.

#### Notes

A value that passes 32 bits wraps: only its low 32 bits are kept, and no error is raised. (The standard calls
this an ambiguous condition.) To read a number of any length, use the numbers library (`lib numbers`).

#### Example

`nip` drops the address, leaving the double (its low cell first, as `.s` shows it) and the count of characters
not converted:

```session
/> 0 0 s" 123abc" >number nip .s
<3> 123 0 3
/> drop 2drop
/> hex 0 0 s" ff zz" >number nip .s decimal
<3> FF 0 3
/> drop 2drop
/> 0 0 s" -5" >number nip .s
<3> 0 0 2
/> drop 2drop
/> 0 0 s" 99999999999" >number nip .s
<3> 2839 -20518 0
```

## on-note

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *xt* -- )

#### Description

*xt* ( *n* -- *flag* ) handles notes (0: none)

## only, also, previous, forth, order

Search-Order; in the `search` library (`lib search`).

#### Stack

All ( -- ).

#### Description

The search order is the list of word lists in which a name is looked up, first to last. `only` makes it the
smallest that will work: the `forth` word list alone. `also` puts a second copy of its first list on the front,
ready to be replaced. A word list's name, such as `forth`, replaces the first list with itself. `previous`
takes the first list off. `order` prints the order, first list first, and then the current word list, the one
new definitions go into.

`also name` is therefore the way to add the word list *name* to the front of the order.

#### Example

```session
/> lib search
/> only forth order
forth  current: forth
```

## open-blocks

Block; in the `block` library (`lib block`). A HyForth word, not the standard's.

#### Stack

```text
open-blocks   ( c-addr u -- )
```

#### Description

Chooses the block file: from now on, `block`, `load` and the rest work on the file named. If the file is not
there it is made, empty. `flush` first, so that changes to the blocks of the file in use are written to it.

Without `open-blocks` the block file is `blocks.fb` in the current directory.

#### See also

`block`, `flush`.

## open-dir, read-dir, close-dir

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
open-dir    ( c-addr u -- dirid ior )
read-dir    ( c-addr u1 dirid -- u2 flag ior )
close-dir   ( dirid -- ior )
```

#### Description

A directory's names, one at a time. `open-dir` opens the directory named by the string. Each `read-dir` puts
the next name into the buffer at *c-addr*, which has room for *u1* characters, and returns its length *u2* and
a true *flag*; when there are no more, the flag is false. `close-dir` gives the directory up.

The names come in the directory's own order. An *ior* is 0 for success.

## open-file, create-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
open-file     ( c-addr u fam -- fileid ior )
create-file   ( c-addr u fam -- fileid ior )
```

#### Arguments

*c-addr u*
: The file's name. One that doesn't start with `/` is looked for from the current directory.

*fam*
: `r/o`, `w/o` or `r/w`.

#### Description

`open-file` opens a file that exists. `create-file` makes a new, empty file; if there is one of that name
already, it is emptied.

Both leave a **fileid**, which is what every other file word takes, and above it an **ior**, an I/O result:
0 if all went well, and the reason if not. The fileid is the system's file descriptor, a small number, and
is only good if the ior is 0. The usual way to write a call is with `throw` after it, which does nothing for
0, and reports any failure.

A device's file is opened the same way: `s" /dev/time" r/o open-file`.

A file should be closed when it is finished with: a task can have only so many open at once.

#### Errors

The ior is −512 minus the system's error code (Appendix G), and as a throw code it prints the system's
message: −544 is `not found`, −547 `is a directory`, −553 `read-only`.

#### See also

`close-file`, `read-line`, `write-line`, `throw`, `ior>text`, `included`.

#### Example

```session
/> s" /ram/f.txt" w/o create-file throw value fd
/> s" line one" fd write-line throw
/> fd close-file throw
/> s" /ram/nothere" r/o open-file . .
-544 0
/> s" /ram/nothere" r/o open-file throw
not found
```

## output-of

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *xt* -- *c-addr u* )

#### Description

What a word prints.

## over

Core word set; in the ROM core, always there.

#### Stack

```text
over   ( x1 x2 -- x1 x2 x1 )
```

#### Description

Copies the second cell to the top.

#### See also

`dup`, `pick`, `2over`, `tuck`.

#### Example

```session
/> 1 2 over .s
<3> 1 2 1
```

## pad

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
pad   ( -- c-addr )
```

#### Description

Leaves the address of a scratch area of 100 bytes, for a program to use as it likes: to build a string, or
hold a line.

The area is at a fixed place, not above `here`, so it does not move as words are defined.

#### See also

`buffer:`, `here`, `s"`.

#### Example

```session
/> s" Hydra" pad swap move
/> pad 5 type
Hydra
```

## page

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Clear the screen.

## parse, parse-name

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
parse        ( char "text<char>" -- c-addr u )
parse-name   ( "name" -- c-addr u )
```

#### Description

Take text from the input that follows the word being run: this is how a word reads its own arguments from
the line, as `:` and `char` do.

`parse` takes everything up to the next occurrence of the character given, and leaves it as a string; the
character itself is used up and is not part of the string. If the character isn't there, the string is the
rest of the line. Nothing is skipped first: the space after the word that called `parse` has gone, but any
further spaces are in the string.

`parse-name` skips spaces, and takes the next word: everything up to the following space or the end of the
line. At the end of the line the string's length is 0.

The string is in the input line itself, and is good only until the line is finished: copy it if it must
last.

#### See also

`word`, `source`, `>in`, `char`.

#### Example

```session
/> : two-words  parse-name type ." |" parse-name type ." |" ;
/> two-words one   two
one|two|
/> : to-comma  [char] , parse type ." |" ;
/> to-comma a b,
a b|
```

## pause

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( -- )

#### Description

Let the other tasks have a turn.

## pen

The screen; in the `video` library (`lib video`).

#### Stack

( *colour* -- )

#### Description

The colour to draw in.

## pick

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
pick   ( xu ... x1 x0 u -- xu ... x1 x0 xu )
```

#### Description

Removes *u*, and copies the cell that is then *u* places down from the top: `0 pick` is `dup`, and `1 pick`
is `over`.

#### Notes

*u* is not checked against the depth of the stack. A *u* that reaches below the bottom of the stack gives
whatever is in the memory there, and is not an error.

#### See also

`roll`, `dup`, `over`.

#### Example

```session
/> 10 20 30 1 pick .
20
/> 0 pick .
30
```

## piped

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *xt c-addr u* -- )

#### Description

Runs a command line, given as a string, with what the word *xt* prints as its standard input. The word is run
with its output going into a pipe, the command reads the pipe, and `piped` waits for the command to end.
`' words s" grep file" piped` shows the words with `file` in their names.

## plain, bold, dim, underline, blink, reverse

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Styles (`plain`: all off)

## plot

The screen; in the `video` library (`lib video`).

#### Stack

( *x y* -- )

#### Description

A dot.

## postpone

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
postpone   ( "name" -- )
```

#### Description

Takes the next word of the definition and compiles it so that its *compiling* action happens later, when
the word being defined runs. It is how one immediate word is built from others.

For an immediate word, that means it is compiled as an ordinary call, where it would otherwise have run on
the spot. For an ordinary word, it means that when the word being defined runs, it compiles a call of that
word into whatever definition is then being compiled.

#### See also

`immediate`, `compile,`, `[compile]`, `literal`.

#### Example

`twice` compiles `dup +` into the definition that uses it:

```session
/> : twice  postpone dup postpone + ; immediate
/> : quad  twice twice ;
/> 3 quad .
12
/> see quad
: quad dup + dup + ;
```

## prompt, prompt2

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *c-addr u* -- )

#### Description

The prompt's format; the second prompt's.

## quit, abort, abort"

Core word set; in the ROM core.

#### Stack

```text
quit     ( -- )                R: ( i*x -- )
abort    ( i*x -- )            R: ( j*x -- )
abort"   ( i*x flag -- | i*x )    compile only, immediate; "text" follows it
```

#### Description

`quit` empties the return stack and goes back to reading lines from the keyboard, without touching the data
stack and without printing `ok` or anything else. `abort` empties the data stack too: it is `-1 throw`, so a
`catch` can stop it.

`abort" text"` is used inside a definition. When it runs, it takes a flag: if the flag is true it prints the
text and aborts (`-2 throw`), and if false it does nothing. It is the short way to check an argument.

#### Example

```session
/> : half  dup 1 and abort" odd number" 2/ ;
/> 10 half .  7 half .
5 odd number
```

## .r, u.r

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
.r    ( n1 n2 -- )
u.r   ( u n -- )
```

#### Description

Print a number set to the right of a field *n2* (or *n*) characters wide, with spaces before it and none
after: `.r` signed, `u.r` unsigned. A number too long for the field is printed in full, and the field is
simply overrun. This is how columns of figures are lined up.

#### See also

`.`, `d.r`, `spaces`, `<#`.

#### Example

```session
/> : row  dup 4 .r dup dup * 6 .r cr ;
/> 7 row 12 row
   7    49
  12   144
/> 12345 2 .r
12345
```

## >r, r>, r@

Core word set; in the ROM core, always there. Compile only.

#### Stack

```text
>r   ( x -- ) ( R: -- x )
r>   ( -- x ) ( R: x -- )
r@   ( -- x ) ( R: x -- x )
```

#### Description

Move a cell between the data stack and the return stack. `>r` takes the top of the data stack and puts it on
the return stack; `r>` brings it back; `r@` copies it without removing it.

The return stack is where a value can be put out of the way while the words that follow work on what is
under it. But it is also where each word keeps the address it will return to, and where `do` loops keep
their numbers, so its use has rules:

* everything a word puts there, it must take off again before its `;` or an `exit`;
* what is put there outside a loop can't be reached inside it, and what is put there inside a loop must be
  taken off in the same turn of the loop, before `loop`;
* `i` is wrong while something put there inside the loop is still there.

They work only inside a definition.

#### See also

`2>r`, `n>r`, `i`, `exit`, `{:`.

#### Example

```session
/> : sum-and-keep  >r r@ + r> ;
/> 10 5 sum-and-keep . .
5 15
```

## r/o, w/o, r/w

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
r/o   ( -- fam )
w/o   ( -- fam )
r/w   ( -- fam )
```

#### Description

The three ways to open a file, for `open-file` and `create-file`: read only, write only, and read and write.
They are the constants 0, 1 and 2, which are the system's own numbers for the three.

#### See also

`open-file`, `create-file`, `bin`.

#### Example

```session
/> r/o . w/o . r/w .
0 1 2
```

## rand, rand32

Bits, random numbers and the disassembler; in the `random` library (`lib random`).

#### Stack

( -- *u* ), ( -- *ud* )

#### Description

A random number (`random`)

## random

Bits, random numbers and the disassembler; in the `random` library (`lib random`).

#### Stack

( *u1* -- *u2* )

#### Description

One from 0 to *u1* - 1.

## read-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
read-file   ( c-addr u1 fileid -- u2 ior )
```

#### Description

Reads up to *u1* bytes from the file, from its current place, into memory at *c-addr*, and moves the file's
place on by what was read. *u2* is the number of bytes read: less than *u1* when the file has fewer left,
and 0 at its end, which is not an error.

The bytes are read as they are, line ends included.

#### See also

`read-line`, `write-file`, `reposition-file`, `file-size`.

#### Example

```session
/> s" /rom/lib/shell" r/o open-file throw value fd
/> pad 5 fd read-file . .
0 5
/> pad 5 type
/bin/
/> pad 100 fd read-file . .
0 9
/> pad 100 fd read-file . .
0 0
/> fd close-file drop
```

## read-line

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
read-line   ( c-addr u1 fileid -- u2 flag ior )
```

#### Description

Reads the next line of the file into memory at *c-addr*, without its line end, and leaves its length, *u2*,
and a flag: true if there was a line, and false at the end of the file, where *u2* is 0.

At most *u1* characters are stored. A longer line is not lost: it comes in pieces, the rest of it being
given by the next `read-line`s. The buffer should have room for two characters more than *u1*, which the
standard lets `read-line` use for the line end.

An empty line is a length of 0 with a true flag; the end of the file is 0 with a false one. So a loop reads
while the flag is true.

#### See also

`read-file`, `write-line`, `refill`, `accept`.

#### Example

```session
/> s" /ram/f.txt" w/o create-file throw value fd
/> s" line one" fd write-line throw  s" two" fd write-line throw  fd close-file throw
/> s" /ram/f.txt" r/o open-file throw to fd
/> pad 80 fd read-line . . .
0 -1 8
/> pad 8 type
line one
/> pad 80 fd read-line . . .
0 -1 3
/> pad 80 fd read-line . . .
0 0 0
/> fd close-file drop
```

## recurse

Core word set; in the ROM core. Immediate, compile only.

#### Stack

Compiling: ( -- ). Running: whatever the definition's own is.

#### Description

Compiles a call to the definition that is being compiled. A definition's own name can't be used for this,
since the name is not findable until the `;`: that rule is what lets a new definition of a name use the old
one.

The depth of recursion is limited by the return stack.

#### Example

```session
/> : fact  dup 1 > if dup 1- recurse * then ;
/> 5 fact .
120
```

## refill, source-id, save-input, restore-input

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
refill          ( -- flag )
source-id       ( -- 0 | -1 | fileid )
save-input      ( -- xn ... x1 n )
restore-input   ( xn ... x1 n -- flag )
```

#### Description

The input source is where the text interpreter is taking its words from. `source-id` says which: 0 for the
keyboard, −1 for a string being `evaluate`d, and a file's id while a file is being loaded.

`refill` reads the next line of the input source into the input buffer and sets `>in` to 0, so that parsing
goes on from its start. Its flag is true if there was a line, and false at the end of a file, and always for a
string. A word that reads text across several lines, such as a multi-line comment, uses it.

`save-input` puts the input source's present position on the stack as *n* cells and their count, and
`restore-input` goes back to it; its flag is true if it could **not**.

## replaces, substitute, unescape

String; in the `string` library (`lib string`).

#### Stack

```text
replaces     ( c-addr1 u1 c-addr2 u2 -- )
substitute   ( c-addr1 u1 c-addr2 u2 -- c-addr2 u3 n )
unescape     ( c-addr1 u1 c-addr2 -- c-addr2 u2 )
```

#### Description

Text substitution, as the standard defines it. `replaces` records that the name *c-addr2 u2* stands for the
text *c-addr1 u1*. `substitute` copies the string *c-addr1 u1* into the buffer *c-addr2 u2*, putting in place of
each name written between two `%` signs the text recorded for it; `%%` gives one `%`. It returns the result
and *n*, the number of substitutions made, or a negative number if the result did not fit.

`unescape` copies a string, doubling every `%` in it, so that `substitute` will pass it through unchanged. Its
buffer must have room for twice the string.

## required, require

Core word set; in the ROM core.

#### Stack

```text
required   ( i*x c-addr u -- j*x )
require    ( i*x "name" -- j*x )
```

#### Description

`included` and `include`, once only: the file is loaded if it has not been loaded already, and otherwise
nothing happens. `required` takes the file's name as a string and `require` takes it from the input.

A file is known by its name as it was found. One that could not be opened, or that stopped with an error part
way, is not counted as loaded, and the next `require` of it tries again.

A name that does not start with `/` is looked for in the current directory and then in `/lib/forth`.

## resize

Memory-Allocation; in the `memory` library (`lib memory`).

#### Stack

```text
resize   ( a-addr1 u -- a-addr2 ior )
```

#### Description

Changes the size of an allocated block to *u* bytes. The block may have to move: the address left is where
it is now, and the old address must not be used again. Its contents, up to the smaller of the old and new
sizes, are kept.

If it can't be done the ior is −61, `resize failed`, and the block is as it was, at its old address.

#### See also

`allocate`, `free`.

#### Example

```session
/> lib memory
/> 100 allocate throw value m
/> m 100 char A fill
/> m 200 resize throw to m
/> m 5 type
AAAAA
/> m free drop
```

## roll

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
roll   ( xu xu-1 ... x0 u -- xu-1 ... x0 xu )
```

#### Description

Removes *u*, takes the cell that is then *u* places down out of the stack, and puts it on top; the cells that
were above it each move down one. `1 roll` is `swap`, `2 roll` is `rot`, and `0 roll` does nothing.

#### See also

`pick`, `rot`.

#### Example

```session
/> 1 2 3 2 roll .s
<3> 2 3 1
```

## rot

Core word set; in the ROM core, always there.

#### Stack

```text
rot   ( x1 x2 x3 -- x2 x3 x1 )
```

#### Description

Brings the third cell to the top, moving the other two down. `rot rot` turns the three the other way: it
puts the top cell under the other two.

#### See also

`roll`, `swap`, `2rot`.

#### Example

```session
/> 1 2 3 rot .s
<3> 2 3 1
/> rot rot .s
<3> 1 2 3
```

## rseed

Bits, random numbers and the disassembler; in the `random` library (`lib random`).

#### Stack

( *ud* -- )

#### Description

Start the random numbers.

## run

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr u* -- *status* )

#### Description

Run a program and its arguments, and wait.

## .s

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
.s   ( -- )
```

#### Description

Shows the whole data stack, and changes nothing: the number of cells in angle brackets, and then each cell
as a signed number in the current base, the top of the stack last.

It is the most useful word there is for finding out what a line or a definition really leaves behind.

#### See also

`depth`, `.`, `n.s`, `dump`.

#### Example

```session
/> 1 2 3 .s
<3> 1 2 3
/> 2drop drop .s
<0>
```

## s"

Core word set; in the ROM core, always there. Immediate.

#### Stack

```text
s"   ( "text<quote>" -- )       compiling
     ( -- c-addr u )            when the definition runs, or at the prompt
```

#### Description

A string. The text that follows, up to the next `"`, becomes a string, and its address and length are left
on the stack. One space follows `s"` and is not part of the string.

Inside a definition the text is compiled into the word, and stays as long as the word does.

At the prompt the text is put in a buffer of HyForth's own, and its address is good only for a while. There
are two such buffers, used in turn, so the last two strings typed are both intact, which is what a word that
takes two strings (such as `rename-file`) needs; a third overwrites the first. Each holds 100 characters. A
string that must last is copied somewhere of the program's own.

There is no way to put a `"` in the text: for that, and for control characters, use `s\"`.

#### See also

`s\"`, `c"`, `."`, `type`, `sliteral`, `pad`.

#### Example

```session
/> s" hello" type
hello
/> s" hello" nip .
5
/> : name  s" Hydra-16" ;
/> name type
Hydra-16
```

## s>d

Core word set; in the ROM core, always there.

#### Stack

```text
s>d   ( n -- d )
```

#### Description

Makes a signed cell a signed double of the same value: a cell of 0 is put above it for a number that is 0 or
more, and a cell of −1, all ones, for a negative one.

For an unsigned cell, the high cell is always 0: put a `0` on the stack.

#### See also

`d>s`, `m*`, `um/mod`.

#### Example

```session
/> 3 s>d . .
0 3
/> -3 s>d . .
-1 -3
```

## s>n, d>n

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *n* -- ) N: ( -- *x* ), ( *d* -- ) N: ( -- *x* )

#### Description

From Forth's stack.

## s\"

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate.

#### Stack

```text
s\"   ( "text<quote>" -- )      compiling
      ( -- c-addr u )           when the definition runs, or at the prompt
```

#### Description

`s"`, with escapes: in the text, a backslash and the character after it stand for a character that can't be
typed.

| Written | Is | Written | Is |
|---|---|---|---|
| `\n` | A new line | `\"` and `\q` | A double quote |
| `\t` | A tab | `\\` | A backslash |
| `\r` | A carriage return (13) | `\e` | Escape (27) |
| `\l` | A line feed (10) | `\a` | The bell (7) |
| `\m` | A carriage return and a line feed | `\b` | A backspace (8) |
| `\z` | A zero byte | `\f`, `\v` | A form feed (12), a vertical tab (11) |
| `\x` and two hex digits | The character with that code | | |

#### See also

`s"`, `emit`, `type`.

#### Example

```session
/> s\" one\ntwo" type
one
two
/> s\" a \"quoted\" word" type
a "quoted" word
```

## sbit, cbit

Bits, random numbers and the disassembler; in the `bits` library (`lib bits`).

#### Stack

( *n b* -- *n'* )

#### Description

Set, clear bit *b*.

## search

String; in the `string` library (`lib string`).

#### Stack

```text
search   ( c-addr1 u1 c-addr2 u2 -- c-addr3 u3 flag )
```

#### Description

Looks for the second string inside the first. If it is found, the flag is true, and the string left is the
rest of the first string from the place of the match on. If it is not found, the flag is false, and the
first string is left as it was.

An empty second string is found at the start.

#### See also

`compare`, `/string`.

#### Example

```session
/> lib string
/> s" hello world" s" o w" search . type
-1 o world
/> s" hello world" s" xyz" search . type
0 hello world
```

## search-wordlist

Search-Order; in the `search` library (`lib search`).

#### Stack

( *c-addr u wid* -- 0 | *xt* 1 | *xt* -1 )

#### Description

Look a word up in a word list.

## see

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
see   ( "name" -- )
```

#### Description

Shows how a word is made. For a colon definition, the source is rebuilt from the compiled code: the words
it calls, by name, with its numbers and strings. A constant is shown as its value, and a word made by
`create` or `variable` as `create`.

The rebuilding is of the code, not of what was typed: comments are gone, a value worked out between `[` and
`]` is a plain number, and the branches of `if` and `begin` appear as `?branch` and `branch` with the
addresses they jump to. `do` loops and strings come back as they were written. `immediate` follows the `;`
of an immediate word.

A word of the ROM that is written in assembly language has no Forth inside it. With the `disasm` library
loaded (`lib disasm`), `see` shows its machine code; without it, just its name.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −13 | *name* `?` | There is no such word |

#### See also

`words`, `disasm`, `dump`, `'`.

#### Example

```session
/> : square  ( n -- n*n )  dup * ;
/> see square
: square dup * ;
/> : count-up  10 0 do i . loop ;
/> see count-up
: count-up 10 0 do i . loop ;
/> : pick-one  if 1 else 2 then ;
/> see pick-one
: pick-one ?branch $804B 1 branch $8054 2 ;
/> 5 constant five  see five
: five 5 ;
```

## seg-bank!

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *seg n* -- *ior* )

#### Description

A shared segment's bank there.

## send

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( "n line" -- )

#### Description

Type a line into window *n*.

## set-base, get-base

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *c-addr u* -- ), ( -- *c-addr u* )

#### Description

The base (any of Chapter 18's)

## sgr

Facility; in the `facility` library (`lib facility`).

#### Stack

( *n* -- )

#### Description

Any `ESC [`*n*`m`.

## sh

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr u* -- *status* )

#### Description

Run a command line with rc, and wait.

## sh-out

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *c-addr u* -- *c-addr2 u2* )

#### Description

A command's output.

## sliteral

String; in the `string` library (`lib string`). Immediate. Compile only.

#### Stack

```text
sliteral   ( c-addr u -- )      compiling
           ( -- c-addr2 u )     when the definition runs
```

#### Description

Takes a string from the stack as a definition is compiled, and compiles a copy of it into the definition,
so that the word leaves the copy's address and length when it runs. It is `literal` for a string, and what
`s"` uses inside a definition.

#### See also

`literal`, `s"`, `2literal`.

#### Example

```session
/> lib string
/> : motto  [ s" compiled in" ] sliteral ;
/> motto type
compiled in
```

## snd-bend, snd-glide

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch n* -- )

#### Description

Bend its pitch; glide to note *n*.

## snd-chord

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch c-addr u* -- *status* )

#### Description

Play a chord.

## snd-claim, snd-release

Sound; in the `sound` library (`lib sound`).

#### Stack

( *mask* -- )

#### Description

Claim or release channels 0 to 7 (bit *n*: channel *n*); `snd-claim-psg` and `snd-release-psg` for 8 to 23.

## snd-drum

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch n* -- )

#### Description

Play General MIDI drum *n*.

## snd-freq

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch hz* -- )

#### Description

Play a frequency (0: stop)

## snd-level

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch v* -- )

#### Description

The channel's volume, 0 to 127.

## snd-mml

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch c-addr u* -- *status* )

#### Description

Play a line of MML (section 15.6)

## snd-note

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch n* -- )

#### Description

Play note *n* (60: middle C)

## snd-off

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch* -- )

#### Description

Let it go.

## snd-pan

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch pan* -- )

#### Description

Its speakers: 1 left, 2 right, 3 both.

## snd-patch

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch p* -- )

#### Description

Give channel *ch* patch *p*.

## snd-play

Sound; in the `sound` library (`lib sound`).

#### Stack

( *c-addr u times* -- *status* )

#### Description

Play a song file *times* times (0: until <kbd>CTRL</kbd>-<kbd>C</kbd>)

## snd-reset

Sound; in the `sound` library (`lib sound`).

#### Stack

( -- )

#### Description

Silence everything.

## snd-sens, snd-lfo, snd-noise, snd-reg

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-sens    ( ch pms ams -- )
snd-lfo     ( rate pmd amd wave -- )
snd-noise   ( n -- )
snd-reg     ( reg val -- )
```

#### Description

The sound driver's less common commands (section 15.3), each the driver's command of the same name with its
numbers in the same order. `snd-sens` sets how strongly the low-frequency oscillator bends a channel's pitch
(*pms*) and its loudness (*ams*). `snd-lfo` sets the oscillator: its rate, how deeply it works on pitch and on
loudness, and its wave. `snd-noise` sets the noise generator's pitch. `snd-reg` writes a value straight to one
of the FM chip's registers.

## snd-volume

Sound; in the `sound` library (`lib sound`).

#### Stack

( *v* -- )

#### Description

The master volume, 0 to 200.

## snd-wave

Sound; in the `sound` library (`lib sound`).

#### Stack

( *ch w width* -- )

#### Description

A PSG channel's wave.

## spawn

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *c-addr u* -- *task* )

#### Description

Start a command, not waiting.

## state

Core word set; in the ROM core, always there.

#### Stack

```text
state   ( -- a-addr )
```

#### Description

Leaves the address of a cell that is zero while HyForth is interpreting and not zero while it is compiling
a definition. An immediate word reads it to do one thing in a definition and another at the prompt.

A program must not store into it: `[` and `]`, `:` and `;` are what change it.

#### See also

`[`, `]`, `immediate`.

#### Example

```session
/> state @ .
0
/> : compiling?  state @ . ; immediate
/> : test  compiling? ;
-1
```

## status

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( -- *n* )

#### Description

The last command's status.

## /string

String; in the `string` library (`lib string`).

#### Stack

```text
/string   ( c-addr1 u1 n -- c-addr2 u2 )
```

#### Description

Cuts *n* characters off the front of a string: the address goes up by *n*, and the length down by *n*. It is
how a program steps along a string.

*n* is not checked. One greater than the length leaves a negative length, and a negative *n* moves the
start back.

#### See also

`-trailing`, `search`, `count`.

#### Example

```session
/> lib string
/> s" hello world" 6 /string type
world
```

## swap

Core word set; in the ROM core, always there.

#### Stack

```text
swap   ( x1 x2 -- x2 x1 )
```

#### Description

Exchanges the top two cells.

#### See also

`over`, `rot`, `2swap`, `nip`, `tuck`.

#### Example

```session
/> 1 2 swap .s
<2> 2 1
/> 10 3 swap - .
-7
```

## synonym

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
synonym   ( "newname" "oldname" -- )
```

#### Description

Defines a new name for a word that exists. The new word behaves exactly as the old one does, at the prompt
and when it is compiled, and is immediate if the old one is.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −13 | *oldname* `?` | There is no such word |

#### See also

`defer`, `:`, `'`.

#### Example

```session
/> synonym plus +
/> 2 3 plus .
5
```

## sys

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *addr a x y* -- *a x y p* )

#### Description

Call machine code, with registers.

## sys-

The Hydra; in the `hydra` library (`lib hydra`). A family of words, one for each system call.

#### Stack

( *inputs* -- *outputs* [*ior*] )

#### Description

For each system call that a program can make there is a word named `sys-` and the call's name in small letters,
with `-` for `_`: `sys-getpid`, `sys-open`, `sys-banks-alloc`. It takes the call's inputs from the stack in the
order Chapter 3 lists them, the first deepest; makes the call; and leaves its outputs in the same order. If the
call can fail, an *ior* is left on top: 0 for success, and otherwise −512 minus the system's error code.

An input that is an address is given as an address. A path is the address of a string that ends with a zero
byte: `>z` ( *c-addr u* -- *z-addr* ) makes one from an ordinary string.

#### Example

```session
/> lib hydra
/> sys-getpid .
1
```

## tbit

Bits, random numbers and the disassembler; in the `bits` library (`lib bits`).

#### Stack

( *n b* -- *n flag* )

#### Description

Is bit *b* of *n* set? (`bits`)

## text

The screen; in the `video` library (`lib video`).

#### Stack

( *x y c-addr u* -- )

#### Description

Write text on it.

## throw

Exception; in the `exception` library, which is loaded as HyForth starts.

#### Stack

```text
throw   ( k*x n -- k*x | i*x n )
```

#### Description

With 0 on the stack, `throw` removes it and does nothing more. With any other number, it abandons what is
running and returns to the most recent `catch` that is still waiting, which leaves that number as its
result. So a word can write `throw` after anything that leaves an error code, zero meaning "none": the file
words leave such a code, and `open-file throw` is the usual way to use them.

If no `catch` is waiting, the throw goes to HyForth itself: its message is printed, both stacks are
emptied, and the prompt returns.

**The codes.** A program may use any positive number for errors of its own. The negative ones are the
system's:

| Code | Message | Meaning |
|---|---|---|
| −1 | (none) | `abort` |
| −2 | The text given | `abort"` |
| −3 | `stack overflow` | More than 32 cells on the data stack |
| −4 | `stack underflow` | A word took more than the stack held |
| −8 | `dictionary full` | No data space left |
| −10 | `division by zero` | |
| −11 | `result out of range` | |
| −13 | The word, and `?` | A word that isn't known |
| −14 | The word, and `compile only` | A word that works only in a definition, used at the prompt |
| −16 | `no name` | A defining word with no name after it |
| −17 | `number too long` | Pictured numeric output with too many digits |
| −21 | `unsupported operation` | |
| −22 | `control structure mismatch` | |
| −24 | `invalid numeric argument` | |
| −28 | `interrupt` | <kbd>CTRL</kbd>-<kbd>C</kbd> |
| −44, −45 | `number stack overflow`, `underflow` | The numbers library's stack |
| −46 | `outside the function's domain` | The numbers library: the logarithm of 0, and the like |
| −49, −50 | `search order full`, `empty` | |
| −59, −60, −61 | `allocate failed`, `free failed`, `resize failed` | The memory library |
| −258 | `sources nested too deeply` | Files that include files, too deep |
| −259 | `not this forth's library` | A library file made for another HyForth |
| −512 − *code* | The system's message | A system call failed: *code* is its error code (Appendix G). −544 is `not found` |

Any other number is reported as `error` and the number.

#### See also

`catch`, `abort"`, `ior>text`, `quit`.

#### Example

```session
/> : check  ( n -- )  100 > if 1 throw then ;
/> 50 ' check catch .
0
/> 500 ' check catch . drop
1
/> -10 throw
division by zero
/> 99 throw
error 99
/> -544 throw
not found
```

## time&date

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *sec min hour day month year* )

#### Description

The date and time.

## to-fixed

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *u* -- ) N: ( *x* -- *y* )

#### Description

A fixed decimal of *u* places.

## to-rational, rational.n, rational.d

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N:

#### Description

A fraction; its top; its bottom.

## -trailing

String; in the `string` library (`lib string`).

#### Stack

```text
-trailing   ( c-addr u1 -- c-addr u2 )
```

#### Description

Shortens a string so that it does not end in spaces: the address stays, and the length becomes that of the
string without them. A string of nothing but spaces becomes one of length 0.

#### See also

`/string`, `blank`.

#### Example

```session
/> lib string
/> s" hi   " -trailing . drop
2
```

## traverse-wordlist

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
traverse-wordlist   ( i*x xt wid -- j*x )
```

#### Description

Calls a word once for each name in a word list, newest first. The word, given by its token *xt*, is called
with the name's **name token** on the stack, above whatever it left from the call before, and must leave a
flag: true to go on to the next name, false to stop there.

A name token, *nt*, stands for one entry in the dictionary. The words below take it apart.

`forth-wordlist`, in the `search` library, gives the word list that holds the system's words.

#### See also

`name>string`, `words`, `forth-wordlist`, `search-wordlist`.

#### Example

```session
/> lib search
/> : count-it  ( n nt -- n' flag )  drop 1+ true ;
/> : first-three  ( n nt -- n' flag )  name>string type space  1+ dup 3 < ;
/> 0 ' first-three forth-wordlist traverse-wordlist drop
first-three count-it end-library
```

## true, false

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
true    ( -- true )
false   ( -- false )
```

#### Description

The two flags, as constants: `true` is −1, a cell with every bit set, and `false` is 0.

#### See also

`0=`, `and`, `if`.

#### Example

```session
/> true . false .
-1 0
```

## truncate, nfloor, nround

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *x* -- *n* )

#### Description

To a whole number.

## tune

Sound; in the `sound` library (`lib sound`).

#### Stack

( *c-addr u ch tempo* -- )

#### Description

Play a tune: note names and their beats.

## type

Core word set; in the ROM core, always there.

#### Stack

```text
type   ( c-addr u -- )
```

#### Description

Writes a string to standard output: *u* characters, starting at the address. With a *u* of 0, nothing.

A string in Forth is an address and a length, on the stack as two cells: `s"` leaves one in that form, and
so do `count`, `parse` and the file words.

#### See also

`s"`, `count`, `emit`, `."`, `dump`.

#### Example

```session
/> s" hello" type
hello
/> s" hello" drop 4 type
hell
```

## um*, m*

Core word set; in the ROM core, always there.

#### Stack

```text
um*   ( u1 u2 -- ud )
m*    ( n1 n2 -- d )
```

#### Description

Multiply two cells and leave the whole 32-bit product, which cannot overflow. `um*` takes the cells as
unsigned and leaves an unsigned double; `m*` takes them as signed and leaves a signed double.

#### See also

`*`, `um/mod`, `*/`, `d.`, `m*/`.

#### Example

```session
/> lib double
/> 40000 3 um* d.
120000
/> -2 3 m* d.
-6
/> 300 300 m* d.
90000
```

(`d.`, which prints a double, is in the `double` library.)

## um/mod, fm/mod, sm/rem

Core word set; in the ROM core, always there.

#### Stack

```text
um/mod   ( ud u1 -- u2 u3 )
fm/mod   ( d n1 -- n2 n3 )
sm/rem   ( d n1 -- n2 n3 )
```

#### Description

Divide a double by a cell, and leave the remainder, and above it the quotient, each a cell.

| Word | The numbers | The quotient | The remainder |
|---|---|---|---|
| `um/mod` | Unsigned | | |
| `fm/mod` | Signed | Rounded down, toward minus infinity ("floored") | Has the divisor's sign, or is 0 |
| `sm/rem` | Signed | Rounded toward zero ("symmetric") | Has the dividend's sign, or is 0 |

For numbers of the same sign the two signed words agree. They differ when the signs differ: −7 by 2 is −4
remainder 1 floored, and −3 remainder −1 symmetric. HyForth's `/` and `mod` are symmetric; `fm/mod` is the
one for a remainder that is never negative with a positive divisor.

The quotient must fit in a cell: one that does not is not reported.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | The divisor is 0 |

#### See also

`/mod`, `um*`, `m*`, `s>d`.

#### Example

```session
/> 100000. 7 um/mod . .
14285 5
/> -7. 2 fm/mod . .
-4 1
/> -7. 2 sm/rem . .
-3 -1
```

## unused

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
unused   ( -- u )
```

#### Description

Leaves the number of bytes of data space still free: from `here` to the dictionary's end.

That is the room for names and data. The code of colon definitions has its own room, in RAM banks, which
`code-banks` reports.

#### See also

`here`, `allot`, `code-banks`.

#### Example

The number depends on what has been loaded:

```text
/> unused u.
17785
```

## update, save-buffers, flush, empty-buffers

Block; in the `block` library (`lib block`).

#### Stack

```text
update          ( -- )
save-buffers    ( -- )
flush           ( -- )
empty-buffers   ( -- )
```

#### Description

`update` marks the block most recently given by `block` or `buffer` as changed, so that it will be written
back. `save-buffers` writes every changed block to the block file. `flush` does the same and then lets the
buffers go. `empty-buffers` lets them go *without* writing: changes not yet saved are thrown away.

Changed blocks are also written when HyForth ends.

#### Notes

A block file grows a block at a time. A block can be written only if every block before it is in the file:
in a new file, write block 0 first, then 1, and so on, with a `flush` after each, since several changed
blocks are not always written in their order. Writing block 4 to a file of three blocks is the error
`end of file`.

#### See also

`block`, `open-blocks`.

## value, to

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
value   ( x "name" -- )
name    ( -- x )
to      ( x "name" -- )
```

#### Description

`value` takes a number from the stack and defines a word that leaves that number when it is used, as a
constant does. `to` changes it: `20 to name` stores 20 as the value's new number. `to` works at the prompt
and inside a definition.

A value reads like a constant and can be changed like a variable, without a `@` at every use.

#### Notes

`to` is only for a name that `value` made (or `2value`, `nvalue`, or a local of `{:`). It
does not check: `to` followed by a constant's or a variable's name is not reported, and stores the number
somewhere it does not belong.

#### See also

`variable`, `constant`, `2value`, `{:`.

#### Example

```session
/> 10 value limit
/> limit .
10
/> 20 to limit  limit .
20
/> : raise  limit 5 + to limit ;
/> raise limit .
25
```

## variable, constant

Core word set; in the ROM core, always there.

#### Stack

```text
variable   ( "name" -- )
constant   ( x "name" -- )
name       ( -- a-addr )       a variable
name       ( -- x )            a constant
```

#### Description

`variable` defines a word that owns one cell of data space, which starts as 0. When the word is used it
leaves the address of the cell, for `@` and `!`.

`constant` takes a value from the stack and defines a word that leaves that value when it is used.

A constant's value is not meant to change. For a value that changes now and then and is read often, `value`
is neater than a variable.

#### See also

`value`, `create`, `2variable`, `2constant`, `@`, `!`.

#### Example

```session
/> variable score
/> score @ .
0
/> 100 score ! score @ .
100
/> 60 constant minute
/> minute 5 * .
300
```

## wait

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *task* -- )

#### Description

Wait for a background task.

## while, repeat

Core word set; in the ROM core, always there. Immediate. Compile only.

#### Stack

```text
while    ( x -- )
repeat   ( -- )
```

#### Description

A loop that tests before its work: `begin` *test* `while` *work* `repeat`. The words of the test are run, and
`while` takes a flag from the stack. If it is true, the work is done and `repeat` goes back to `begin` for
the test again. If it is false, the loop is over, and the words after `repeat` are run.

Unlike `begin` ... `until`, the work may be done no times at all, when the test fails the first time.

#### See also

`begin`, `until`, `again`, `do`.

#### Example

```session
/> : countdown  begin dup while dup . 1- repeat drop ;
/> 5 countdown
5 4 3 2 1
/> 0 countdown
```

## within

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

```text
within   ( n1 n2 n3 -- flag )
```

#### Description

True when *n1* is in the range that starts at *n2* and stops just before *n3*: *n2* ≤ *n1* < *n3*. The
lower limit is in the range, and the upper one is not.

The three may all be signed numbers or all unsigned ones, and the test is right for either. It is worked
out as a single unsigned comparison, (*n1* − *n2*) `u<` (*n3* − *n2*), so if the limits are given in the
wrong order the result is not simply false: it is true for the numbers *outside* the range between them.

#### See also

`<`, `u<`, `min`, `max`.

#### Example

```session
/> 5 1 10 within . 1 1 10 within . 10 1 10 within .
-1 -1 0
/> -1 -5 5 within .
-1
```

## word, source, >in

Core word set; in the ROM core.

#### Stack

```text
word     ( char "<chars>ccc<char>" -- c-addr )
source   ( -- c-addr u )
>in      ( -- a-addr )
```

#### Description

The text interpreter's own tools, for words that read what follows them in the input.

`source` gives the address and length of the input buffer: the line, or the string, being interpreted. `>in`
is a variable holding how far into it the interpreter has got, in characters; storing into it moves the place
the next word will be read from.

`word` skips any of *char* at the place, reads up to the next *char* or the end of the line, and returns the
address of a **counted string**: a length byte, then the characters. The string is in a transient buffer. `bl
word` reads the next name.

`word` is the older tool. `parse-name` and `parse`, which return an address and a length within the input
buffer and copy nothing, are simpler to use.

## wordlist

Search-Order; in the `search` library (`lib search`).

#### Stack

( -- *wid* )

#### Description

A new, empty word list.

## words

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

```text
words   ( -- )
```

#### Description

Lists the words of the first word list in the search order, the newest first: for each, its execution token
in hex, three marks, and its name, as many to a line as the screen's width allows.

The marks say what kind of word it is: an `l` for a word that is a number compiled in place (a constant
such as `true`), an `i` for an immediate word, and then `a` for a word written in assembly language or `f`
for one written in Forth.

The list is long: the ROM core and the libraries loaded at the start have about 270 words. Words defined
since are at the top.

#### See also

`see`, `libs`, `order`, `traverse-wordlist`, `[defined]`.

#### Example

The first lines, after two words were defined:

```text
/> : square dup * ;  : cube dup square * ;
/> words
 3974 --f cube            3967 --f square          3756 --a unsetenv
 36E7 --a setenv          3671 --a getenv          365F --a wait
 3641 -ia exit            361D --a exits           3609 --a status
...
```

## write-file, write-line

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

```text
write-file   ( c-addr u fileid -- ior )
write-line   ( c-addr u fileid -- ior )
```

#### Description

Write a string to a file, at its current place, and move the place on. `write-file` writes the *u* bytes
and nothing else. `write-line` writes them and then a line end, a line feed.

The file must have been opened `w/o` or `r/w`: writing to one opened `r/o` is the ior −549.

#### See also

`read-file`, `read-line`, `flush-file`, `create-file`, `type`.

#### Example

```session
/> s" /ram/f.txt" w/o create-file throw value fd
/> s" one" fd write-line .
0
/> s" two" fd write-file .
0
/> fd close-file drop
/> cat /ram/f.txt
one
two
```

## >z, zcount

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr u* -- *z-addr* ), ( *z-addr* -- *c-addr u* )

#### Description

To and from a zero-ended string.

