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

```text
%   ( "line" -- )
```

#### Description

Gives the rest of the line to rc as a command, whatever its first word is.

At HyForth's prompt a line is taken as Forth if its first word is a Forth word or a number, and as a command
for rc otherwise. So a program whose name is also a Forth word can't be run just by typing it: `words`,
`free` (once the memory library is loaded), or any word you have defined yourself. `%` settles it: what
follows is a command.

The command's exit code is then in `status`. A line that ends in `&` is not waited for, as in rc.

#### See also

`sh`, `status`, `wait`; section 8.14.

#### Example

```session
/> : free  ." my own word" ;
/> free
my own word
/> % free
ram     384 KB a task (3 modules)
shared  1024 KB, 512 KB in segments (1), 512 KB free
```

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

```text
argc   ( -- n )
arg    ( n -- c-addr u )
```

#### Description

The arguments HyForth was started with, for a program run as `forth file.fs a b`. `argc` leaves how many
there are, the file's name counting as the first: 3 in that case. `arg` leaves argument *n* as a string,
number 0 being the file's name; past the last, the string's length is 0.

At the prompt of an interactive HyForth there are none: `argc` is 0.

#### See also

`included`, `getenv`.

#### Example

```session
/> lib hydra
/> argc .
0
```

## at-xy

Facility; in the `facility` library (`lib facility`).

#### Stack

```text
at-xy   ( u1 u2 -- )
```

#### Description

Moves the cursor to column *u1* of row *u2*. Both count from 0, and 0 0 is the top left corner of the
window. What is printed next appears there.

#### See also

`page`, `form`, `cursor-up`, `clear-line`.

#### Example

A message in the middle of a cleared screen:

```text
lib facility
page  30 12 at-xy  ." The Hydra-16"  0 23 at-xy
```

## bank!, bank@

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
bank!   ( bank -- )
bank@   ( -- bank )
```

#### Description

`bank!` chooses which of the task's RAM banks is seen at `bank-window`, and `bank@` leaves the number of the
one chosen. A task has 16 banks of 8K for each RAM module fitted. A program asks for the ones it will use
with `sys-banks-alloc`, which leaves the number of the first of a run, and gives them back with
`sys-banks-free`.

With a bank chosen, the 8K at `bank-window` is ordinary memory, for `@`, `!`, `move` and the rest. This is
how a HyForth program keeps far more data than its dictionary has room for.

#### Notes

HyForth keeps the code of colon definitions in RAM banks too, at the same window. A definition that uses
`bank!` would switch away the very bank it is running from, so one compiled there is the error −21,
`unsupported operation`, when it runs. Say `false code-banks` before defining any word that uses `bank!` or
`seg-bank!`: such words are then compiled into the dictionary, and work. At the prompt `bank!` can be used
freely.

#### See also

`bank-window`, `code-banks`, `seg-bank!`, `allocate`.

#### Example

```session
/> lib hydra
/> 2 sys-banks-alloc throw value bk
/> false code-banks
/> : keep  ( n -- )  bk bank!  bank-window ! ;
/> : kept  ( -- n )  bk bank!  bank-window @ ;
/> true code-banks
/> 1234 keep kept .
1234
/> bk 2 sys-banks-free .
0
```

## bank-window

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
bank-window   ( -- addr )
```

#### Description

A constant: `$8000`, the address where the task's chosen RAM bank appears. The 8K from there to `$9FFF` is a
window on to one bank at a time, which `bank!` and `seg-bank!` choose.

#### See also

`bank!`, `seg-bank!`, `code-banks`; Appendix F.

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

Facility; in the `facility` library (`lib facility`). A HyForth word, not the standard's.

#### Stack

```text
beep   ( -- )
```

#### Description

Rings the terminal's bell.

#### See also

`emit`, `snd-note`.

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

```text
begin-structure   ( "name" -- struct-sys 0 )
end-structure     ( struct-sys +n -- )
name              ( -- +n )
```

#### Description

Describe a record: a group of named fields laid out one after another. `begin-structure` starts the
description and defines *name*; the field words between add fields; `end-structure` finishes it. Afterward
*name* leaves the size of the whole record in bytes, which is what `allot`, `buffer:` or `allocate` needs to
make one.

Nothing is set aside by the description itself. It only works out offsets: each field word, used later,
adds its field's offset to the address of a record.

#### See also

`+field`, `create`, `allot`.

#### Example

```session
/> lib facility
/> begin-structure point  field: p.x  field: p.y  cfield: p.flag  end-structure
/> point .
5
/> create here-i-am  point allot
/> 3 here-i-am p.x !  4 here-i-am p.y !
/> here-i-am p.x @ . here-i-am p.y @ .
3 4
```

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

```text
bind      ( "[-a|-b] [-c] new old" -- )
mount     ( "[-a|-b] [-c] #x old [spec]" -- )
unmount   ( "[new] old" -- )
newns     ( -- )
```

#### Description

Change HyForth's own namespace. Each takes the rest of the line as its arguments, written exactly as for
rc's commands of the same names, whose entries in Chapter 2 of this book describe them in full: `bind` puts
one directory at another's place, or joins it to what is there (`-a` after, `-b` before, `-c` taking new
files); `mount` does the same with a device; `unmount` undoes them; `newns` builds the default namespace
again.

They are Forth words because a command run by rc would change only the namespace of that command's task.
Commands started afterward see the namespace as these leave it.

A HyForth that is a window's first shell keeps the files in its `/ram` through a `newns`. One started from
another shell shares that shell's `/ram` until its first `newns`, which gives it an empty one of its own.

#### See also

`cd`; Chapter 9; `ns`, the command.

#### Example

```session
/> mkdir /ram/a /ram/b
/> echo 1 >/ram/a/one
/> bind /ram/a /ram/b
/> ls /ram/b
one
/> unmount /ram/b
/> ls /ram/b
```

## bitmap, bitmap-off

The screen; in the `video` library (`lib video`).

#### Stack

```text
bitmap       ( width depth -- )
bitmap-off   ( -- )
```

#### Description

`bitmap` turns on a bitmap on a Vera X's screen, under the console's text, for the drawing words. *width* is
320 or 640 dots across, giving 240 or 480 down; *depth* is the number of bits for each dot, 1, 2, 4 or 8,
which gives 2, 4, 16 or 256 colours. At 640 across only 1 or 2 bits are possible. `bitmap-off` turns it off.

The library needs a Vera X card (the emulator has one with `--vera`). Without one, its words are the error
`not found`. A drawing word used with no bitmap on is `invalid argument`.

#### See also

`pen`, `plot`, `clear`; Chapter 16.

#### Example

A square, a filled circle, and the turtle's hexagon:

```text
lib video
320 8 bitmap
5 pen  10 10 100 100 box  200 60 30 disc
cs  : hexagon  6 0 do 40 fd 60 rt loop ;  hexagon
bitmap-off
```

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

Facility; in the `facility` library (`lib facility`). HyForth's own words, not the standard's.

#### Stack

```text
black ... white   ( -- c )
```

#### Description

The eight colours, as constants, for `color` and `bgcolor`: `black` is 0, `red` 1, `green` 2, `yellow` 3,
`blue` 4, `magenta` 5, `cyan` 6 and `white` 7.

#### See also

`color`, `bright`.

#### Example

```session
/> lib facility
/> black . red . green . yellow . blue . magenta . cyan . white .
0 1 2 3 4 5 6 7
```

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

## bright

Facility; in the `facility` library (`lib facility`). A HyForth word, not the standard's.

#### Stack

```text
bright   ( c -- c' )
```

#### Description

Turns one of the eight colours into its bright version, by adding 8: `red bright` is 9. A colour that is
already bright is left as it is.

#### See also

`color`, `red`, `bold`.

#### Example

```session
/> lib facility
/> red bright . 9 bright .
9 9
```

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

```text
cd   ( "dir" -- )
```

#### Description

Changes HyForth's current directory to the one named on the rest of the line. It is a Forth word because it
has to be: a `cd` run by rc as a command would change only that command's own directory.

The name is written as for rc's `cd`: `'`...`'` quotes one with spaces in it, and `$name` is a variable's
value. With no name, the directory is the one in `$home`, if that is set.

The prompt shows the current directory. Inside a definition, use `set-dir`.

#### Errors

| Message | Cause |
|---|---|
| *dir*`: not found` | There is no such directory |

#### See also

`set-dir`, `get-dir`, `prompt`.

#### Example

```session
/> cd /rom/lib
/rom/lib> cd /nothere
/nothere: not found
/rom/lib> cd /
```

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

```text
circle   ( x y r -- )
disc     ( x y r -- )
```

#### Description

Draw a circle with its centre at *x*, *y* and radius *r*, in the pen's colour: `circle` its outline, and
`disc` filled.

#### See also

`box`, `plot`.

## clear

The screen; in the `video` library (`lib video`).

#### Stack

```text
clear   ( -- )
```

#### Description

Clears the bitmap: every dot becomes colour 0. The console's text over it is not touched.

#### See also

`cs`, `page`, `bitmap`.

## clear-line, clear-below

Facility; in the `facility` library (`lib facility`). HyForth's own words, not the standard's.

#### Stack

```text
clear-line    ( -- )
clear-below   ( -- )
```

#### Description

`clear-line` clears from the cursor to the end of its line. `clear-below` clears from the cursor to the end
of the window. Neither moves the cursor.

#### See also

`page`, `at-xy`.

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

```text
code-banks   ( flag -- )
```

#### Description

Says where the code of the colon definitions made from now on is to go. With a true flag it goes into RAM
banks of HyForth's own, as it does when HyForth starts: that leaves the dictionary for names and data, and
lets a program be much bigger than the dictionary. With a false flag it goes into the dictionary, in
ordinary memory.

Code in the dictionary is needed for words that change the bank window themselves, with `bank!` or
`seg-bank!`. Switch to false, define those words, and switch back.

#### See also

`bank!`, `here`, `unused`.

## color, bgcolor

Facility; in the `facility` library (`lib facility`). HyForth's own words, not the standard's.

#### Stack

```text
color     ( c -- )
bgcolor   ( c -- )
```

#### Description

Set the colour of the text printed from now on, and of its background. A colour is a number from 0 to 15:
the eight named colours, 0 to 7, and their bright versions, 8 to 15, which `bright` gives.

The colours stay set until they are changed or `plain` is used, after the program has ended too.

#### See also

`black`, `bright`, `plain`, `bold`.

#### Example

```text
lib facility
yellow color  blue bgcolor  ."  Warning "  plain
```

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

```text
complex   ( N: re im -- z )
```

#### Description

Makes a complex number from two real ones: its real part, and above it its imaginary part. A complex number
can also be typed directly, with digits before its `i`: `1+2i`, `2i`.

#### See also

`complex?`, `n+`, `nsqrt`.

#### Example

```session
/> lib numbers
/> 1 s>n 2 s>n complex n.
1+2i
/> 5 s>n 1+2i n+ n.
6+2i
```

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

## cs, home, fd, bk, rt, lt, pu, pd, heading, seth

The screen; in the `video` library (`lib video`).

#### Stack

```text
cs        ( -- )            home      ( -- )
fd        ( n -- )          bk        ( n -- )
rt        ( degrees -- )    lt        ( degrees -- )
pu        ( -- )            pd        ( -- )
heading   ( -- degrees )    seth      ( degrees -- )
```

#### Description

Turtle graphics, as in the language Logo. The turtle is a point on the bitmap with a heading and a pen.
`home` puts it in the middle of the screen, heading up, with its pen down, and `cs` clears the bitmap first.
`fd` moves it forward *n* dots along its heading, drawing a line in the pen's colour if the pen is down, and
`bk` moves it back. `rt` and `lt` turn it right and left by a number of degrees. `pu` lifts the pen, so that
moves draw nothing, and `pd` puts it down.

`heading` leaves the heading, 0 to 359 degrees, 0 being up and 90 to the right; `seth` sets it.

The turtle's place is kept to a sixteenth of a dot, so a figure of many small moves does not drift.

#### See also

`line`, `pen`, `bitmap`.

## ctl

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
ctl   ( c-addr1 u1 c-addr2 u2 -- )
```

#### Description

Writes the second string to the file named by the first, in a single write, and closes it. It is how a
command is sent to a device's control file: the same thing as `echo -n text >file` at the shell.

A failure is thrown, and reported with the file's name: the file isn't there, or the driver refused the
command.

#### See also

`write-file`, `sh`; Chapter 12.

#### Example

```session
/> lib hydra
/> s" /dev/gpio/ctl" s" out 2" ctl
/> s" /dev/gpio/ctl" s" in 2" ctl
/> s" /dev/gpio/ctl" s" fly" ctl
/dev/gpio/ctl: invalid argument
```

## cursor-save, cursor-restore, cursor-off, cursor-on

Facility; in the `facility` library (`lib facility`). HyForth's own words, not the standard's.

#### Stack

```text
cursor-save      ( -- )
cursor-restore   ( -- )
cursor-off       ( -- )
cursor-on        ( -- )
```

#### Description

`cursor-save` has the terminal remember where the cursor is, and `cursor-restore` puts it back there: a
program can write somewhere else on the screen, a status line for instance, and return. `cursor-off` hides
the cursor, and `cursor-on` shows it again.

A program that hides the cursor should show it again before it ends: the setting is the window's, and
outlasts the program.

#### See also

`at-xy`, `plain`.

## cursor-up, cursor-down, cursor-right, cursor-left

Facility; in the `facility` library (`lib facility`). HyForth's own words, not the standard's.

#### Stack

```text
cursor-up      ( n -- )
cursor-down    ( n -- )
cursor-right   ( n -- )
cursor-left    ( n -- )
```

#### Description

Move the cursor *n* places from where it is. It stops at the edge of the window.

#### See also

`at-xy`, `cursor-save`.

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

```text
definitions   ( -- )
```

#### Description

Makes the first word list in the search order the compilation word list too: from now on, new words go into
the list that is searched first. `only forth definitions` puts everything back as it starts.

#### See also

`set-current`, `only`, `also`, `order`.

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

```text
digits   ( -- a-addr )
```

#### Description

A variable: the number of significant digits the math functions work to when an answer is not exact. It
starts as 12, and can be raised as far as 100. It makes no difference to `n+`, `n-`, `n*` and `n/`, which are
always exact.

#### See also

`nsqrt`, `npow`, `to-fixed`.

#### Example

```session
/> lib numbers
/> 30 digits !
/> 2 s>n nsqrt n.
1.41421356237309504880168872421
/> npi n.
3.14159265358979323846264338328
```

## disasm

The disassembler; in the `disasm` library (`lib disasm`).

#### Stack

```text
disasm   ( addr u -- )
```

#### Description

Disassembles machine code: shows *u* instructions, starting at *addr*, a line each, with the instruction's
address, its bytes, and the instruction as the assembler `as` would write it. A call or a jump to a Forth
word has the word's name after it.

While the library is loaded, `see` uses it too: a word of the ROM that is written in assembly language is
shown as its instructions, between `code` and `end-code`.

The address of a word's code is its execution token, from `'`. For a colon definition whose code is in a
code bank, the token's address holds only the call that leads there.

#### See also

`see`, `dump`, `sys`; the commands `dis` and `db`; Chapter 28.

#### Example

```session
/> lib disasm
/> see dup
code dup
 A299  CA        dex
 A29A  B5 23     lda $23,x
 A29C  95 22     sta $22,x
 A29E  B5 43     lda $43,x
 A2A0  95 42     sta $42,x
 A2A2  60        rts
end-code
```

## disk-ctl, disk-cmd, disk-start, disk-stop, cards

The devices; in the `disk` library, a file of Forth in `/lib/forth` (`lib disk`).

#### Stack

```text
disk-ctl     ( disk -- c-addr u )
disk-cmd     ( c-addr u disk -- )
disk-start   ( disk -- )
disk-stop    ( disk -- )
cards        ( -- mask )
```

#### Description

The disks (Chapter 14). A disk is named by its letter, as a character: `char 0` to `char f` for the card on
that SPI device, `char x` for the ROM disk, `char r` for the RAM disk and `char s` for the shared one.

`disk-ctl` leaves the text of the disk's control file: what it is and its size, and its file system's label
and free space. `disk-cmd` sends that file a command, any of those Chapter 14 lists (`check`, `label` ...).
`disk-start` starts a card again, as is needed after one is changed, and `disk-stop` stops a RAM disk and
gives its memory back.

`cards` leaves a mask of the SD cards that are there: bit *n* is set for one on SPI device *n*.

#### See also

The commands `df`, `fsck` and `label`; Chapter 14.

#### Example

```session
/> lib disk
/> char x disk-ctl type
rom 4 MB 8192 blocks
hydrafs label=ROM
partition at block 1056
free 11 KB of 623 KB
/> cards .
1
```

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

```text
ekey    ( -- u )
ekey?   ( -- flag )
```

#### Description

`ekey` waits for a key and leaves a number for it, the special keys included: an arrow or a function key,
which sends several characters, comes back as one number. For an ordinary key the number is its character's
code; for a special key it is the value of one of the `k-` constants, `k-up`, `k-f1` and the rest, which are
128 and above.

`ekey?` is true if a key is waiting, as `key?` is.

#### See also

`ekey>char`, `ekey>fkey`, `k-up`, `key`.

#### Example

Here the up arrow was pressed:

```text
/> lib facility
/> ekey k-up = .
-1
```

## ekey>char, ekey>fkey

Facility; in the `facility` library (`lib facility`).

#### Stack

```text
ekey>char   ( u -- u false | char true )
ekey>fkey   ( u -- u false | u true )
```

#### Description

Sort what `ekey` gave. `ekey>char` leaves true above it if it is an ordinary character, and false if it is a
special key. `ekey>fkey` is the other way round: true if it is a special key, one of the `k-` values, and
false if it is a character. Either way the number itself stays underneath.

#### See also

`ekey`, `k-up`.

#### Example

Here <kbd>A</kbd> and then the up arrow were pressed:

```text
/> lib facility
/> : show-key  ekey ekey>fkey if ." special " else ." character " then . ;
/> show-key
character 65
/> show-key
special 128
```

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

```text
emit?   ( -- flag )
```

#### Description

In the standard, true if `emit` would not have to wait. On HyForth output never has to be asked about: the
result is always true.

#### See also

`emit`, `key?`.

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

```text
exits   ( n -- )
```

#### Description

Ends HyForth, with *n* as its exit code: what the program that started it sees as its status. `bye` ends it
with 0. A script run as a command uses `exits` to say whether it succeeded.

#### See also

`bye`, `status`, `exit`.

## +field, field:, cfield:

Facility; in the `facility` library (`lib facility`).

#### Stack

```text
+field    ( n1 n2 "name" -- n3 )
field:    ( n1 "name" -- n2 )
cfield:   ( n1 "name" -- n2 )
name      ( addr1 -- addr2 )
```

#### Description

Add a field to the structure being described. Each takes the offset reached so far, defines *name* as a
word that adds that offset to an address, and leaves the offset of whatever comes next. `+field` adds a
field of *n2* bytes: any size, for a string or a record inside a record. `field:` adds a cell, 2 bytes, and
`cfield:` a character, 1 byte.

Used later, a field's name turns the address of a record into the address of that field in it, ready for
`@`, `!`, `c@` or `move`. The first field's offset is 0.

#### See also

`begin-structure`, `cells`, `@`.

#### Example

```session
/> lib facility
/> begin-structure person  16 +field p.name  field: p.age  end-structure
/> person .
18
/> 0 p.name . 0 p.age .
0 16
```

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

The place may be set past the end of the file. Reading there finds the end; writing there makes the file
that long, and the bytes between, which were never written, read as zeros.

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

Facility; in the `facility` library (`lib facility`). A HyForth word, not the standard's.

#### Stack

```text
form   ( -- rows cols )
```

#### Description

Leaves the size of the window HyForth is running in: its number of rows, and above that its number of
columns. A program that draws on the screen asks this, not assuming 24 by 80: a window can be any size, and
is as small as the smaller of the terminals it is shown on.

#### See also

`at-xy`, `k-resize`.

#### Example

```session
/> lib facility
/> form . .
80 24
```

## forth-wordlist

Search-Order; in the `search` library (`lib search`).

#### Stack

```text
forth-wordlist   ( -- wid )
```

#### Description

Leaves the identifier of the word list that holds the system's words: the ROM core's, those of every
library loaded, and, unless a program arranges otherwise, the program's own definitions.

#### See also

`wordlist`, `get-order`, `forth`, `traverse-wordlist`.

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

```text
get-current   ( -- wid )
set-current   ( wid -- )
```

#### Description

The **compilation word list** is the one that new definitions are added to. `get-current` leaves its
identifier, and `set-current` makes another word list the one.

Where definitions go and where words are looked for are separate settings. A word defined into a word list
that is not in the search order can't be found by name until the list is added to the order.

#### See also

`definitions`, `get-order`, `wordlist`.

## get-dir, set-dir

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
get-dir   ( c-addr1 u1 -- c-addr2 u2 )
set-dir   ( c-addr u -- ior )
```

#### Description

`get-dir` puts the current directory's path into the buffer given, *c-addr1* and its size *u1*, and leaves
it as a string: as much of it as fits. `set-dir` makes the directory named the current one, and leaves 0, or
the reason it could not.

`set-dir` is the shell's `cd` in a form that works inside a definition. The current directory is where
names that don't begin with `/` are looked for.

#### See also

`cd`, `open-dir`, `=mkdir`.

#### Example

```session
/> lib hydra
/> s" /rom/lib" set-dir .
0
/rom/lib> pad 80 get-dir type
/rom/lib
/rom/lib> s" /nothere" set-dir .
-544
/rom/lib> s" /" set-dir drop
```

## get-order, set-order

Search-Order; in the `search` library (`lib search`).

#### Stack

```text
get-order   ( -- widn ... wid1 n )
set-order   ( widn ... wid1 n -- )
```

#### Description

The **search order** is the list of word lists that are searched for a name, in order: the first one that
has the name wins. `get-order` leaves the identifiers of all of them, the first to be searched on top, and
above them their number. `set-order` takes the same picture and makes it the search order. There can be
eight word lists in the order.

`-1 set-order` sets the least order a system can work with: the system's word list alone, as `only` does.

To add a word list at the front of the order: `get-order mine swap 1+ set-order`.

#### Notes

`0 set-order` empties the search order, and then *no* word can be found, `only` and `set-order` among them:
every line is answered with the word and a `?`. There is no way back but to end HyForth, with
<kbd>CTRL</kbd>-<kbd>D</kbd>.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −49 | `search order full` | More than eight word lists |

#### See also

`order`, `only`, `also`, `previous`, `definitions`.

#### Example

```session
/> lib search
/> wordlist constant mine
/> mine set-current  : secret 42 ;  definitions
/> get-order mine swap 1+ set-order
/> secret .
42
/> only forth
/> [defined] secret .
0
```

## getenv, setenv, unsetenv

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
getenv     ( c-addr1 u1 -- c-addr2 u2 )
setenv     ( c-addr1 u1 c-addr2 u2 -- )
unsetenv   ( c-addr u -- )
```

#### Description

The environment's variables, which HyForth shares with the commands it runs (section 17.7).

`getenv` leaves the value of the variable named, as a string. A variable that is a list of several words
comes with a space between them. If there is no such variable, the length is 0.

`setenv` sets the variable named by the first string to the second, as one word of at most 126 characters.
`unsetenv` removes a variable; one that isn't there is not an error.

The commands HyForth starts get a copy of its environment, so a variable set here is `$name` in a command
line typed afterward.

#### See also

`sh`, `%`, `arg`.

#### Example

```session
/> s" colour" s" red" setenv
/> s" colour" getenv type
red
/> echo my colour is $colour
my colour is red
/> s" colour" unsetenv
/> s" colour" getenv nip .
0
```

## gpio, gpio!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio    ( pin -- level )
gpio!   ( pin level -- )
```

#### Description

The eight pins of the I/O header J27, numbered 0 to 7 (section 17.2). `gpio` leaves a pin's level now, 0 or
1. `gpio!` makes the pin an output and sets it: low for a *level* of 0, high for anything else.

Every pin is an input when the Hydra starts, and an input with nothing connected reads 1.

The library's words work through the files of `/dev/gpio`, and a failure is thrown, with the word's name:
`gpio: not found` for a pin that is not 0 to 7.

#### See also

`gpio-in`, `gpio-port`, `gpio-state`, `ctl`.

#### Example

```session
/> lib gpio
/> 2 gpio .
1
/> 2 0 gpio!  2 gpio .
0
/> 2 gpio-in
```

## gpio-ca1!, gpio-ca2!, gpio-wait

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-ca1!   ( rise? -- )
gpio-ca2!   ( n -- )
gpio-wait   ( -- count )
```

#### Description

The header's two handshake lines. `gpio-ca1!` chooses which change of CA1 is counted: its rises, from 0 to
1, for a true flag, and its falls for a false one. `gpio-wait` waits for the next such change, and leaves
the number counted so far: this is how a program waits for a button without watching it.

`gpio-ca2!` sets CA2: 0 makes it an output that is low, 1 an output that is high, and −1 an input.

#### See also

`gpio-state`, `key?`; section 17.2.

## gpio-in, gpio-out, gpio-ddr!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-in     ( pin -- )
gpio-out    ( pin -- )
gpio-ddr!   ( byte -- )
```

#### Description

Set the pins' directions. `gpio-in` makes one pin an input, and `gpio-out` an output. `gpio-ddr!` sets all
eight at once: bit *n* of the byte is 1 to make pin *n* an output, and 0 for an input.

#### See also

`gpio!`, `gpio-port!`, `gpio-state`.

## gpio-port, gpio-port!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-port    ( -- byte )
gpio-port!   ( byte -- )
```

#### Description

All eight pins at once, as one byte: bit *n* is pin *n*. `gpio-port` leaves their levels. `gpio-port!` sets
the levels of the pins that are outputs; those that are inputs stay inputs, and their bits are ignored.

#### See also

`gpio`, `gpio-ddr!`.

#### Example

```session
/> lib gpio
/> 15 gpio-ddr!  5 gpio-port!
/> gpio-port .
245
/> 0 gpio-ddr!
```

## gpio-state

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-state   ( -- c-addr u )
```

#### Description

Leaves the text of the pins' control file: a line for each pin, with its number, its direction and its
level, then CA1's edge and its count, and CA2's setting. The text is in a buffer of the library's, good
until its next word is used.

#### See also

`gpio`, `gpio-ddr!`.

#### Example

```session
/> lib gpio
/> gpio-state type
0 in 1
1 in 1
2 in 1
3 in 1
4 in 1
5 in 1
6 in 1
7 in 1
ca1 fall 0
ca2 in
```

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

## i2c-read, i2c-write, i2c-devices, i2c?, i2c-speed, i2c-reg-size

The devices; in the `i2c` library, a file of Forth in `/lib/forth` (`lib i2c`).

#### Stack

```text
i2c-read       ( addr reg c-addr u -- )
i2c-write      ( addr reg c-addr u -- )
i2c-devices    ( -- )
i2c?           ( addr -- flag )
i2c-speed      ( khz -- )
i2c-reg-size   ( n -- )
```

#### Description

The I²C bus on the I/O header (section 17.3). *addr* is a chip's address on the bus, `$08` to `$77`.

`i2c-read` reads *u* bytes, 64 at most, from the chip into memory at *c-addr*, and `i2c-write` sends *u*
bytes from there. *reg* is the register inside the chip to start from; it is used only when a register size
has been set, and is otherwise given as 0.

`i2c-devices` prints the addresses of the chips that answer, in hex, and `i2c?` leaves true if one answers at
the address given.

`i2c-speed` sets the bus's speed in kilohertz, 1 to 100. `i2c-reg-size` says how a register's number is
sent before the data: 0 not at all, 1 as one byte, 2 as two, high byte first.

A chip that does not answer is the error `i/o error`, thrown with the word's name.

#### See also

`gpio`, `spi`, `ctl`.

#### Example

With nothing on the bus:

```session
/> lib i2c
/> $50 i2c? .
0
/> $50 0 pad 2 i2c-read
i2c-read: i/o error
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

```text
int?        ( -- flag ) ( N: x -- )
fixed?      ( -- flag ) ( N: x -- )
rational?   ( -- flag ) ( N: x -- )
complex?    ( -- flag ) ( N: x -- )
```

#### Description

Remove the top number and leave a flag: is it an integer, a decimal, a fraction, a complex number? The test
is of the form the number is in, not its value: 1.0 is a decimal, not an integer, though the two are equal.
Arithmetic gives its results in the simplest form, so `4/2` is an integer.

#### See also

`n=`, `to-fixed`, `to-rational`.

#### Example

```session
/> lib numbers
/> 1/2 rational? . 1.5 fixed? . 2i complex? .
-1 -1 -1
/> 1.0 int? . 4/2 int? .
0 -1
```

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

```text
ior>text   ( ior -- c-addr u )
```

#### Description

Leaves the system's message for an ior, the result that the file words and the `sys-` words leave: for −544,
`not found`. For a number that is not a system error's, the standard's throw codes among them, the string
is empty.

`throw` prints the same message. `ior>text` is for a program that wants to word its own report.

#### See also

`throw`, `open-file`, `sys-`.

#### Example

```session
/> lib hydra
/> -544 ior>text type
not found
/> s" /ram/nothere" r/o open-file nip ior>text type
not found
```

## k-resize, k-focus

Facility; in the `facility` library (`lib facility`). HyForth's own words, not the standard's.

#### Stack

```text
k-resize   ( -- u )
k-focus    ( -- u )
```

#### Description

Constants, 150 and 151: two more numbers that `ekey` can give, which are not keys but news from the console.
`k-resize` comes when the window's size has changed (ask `form` for the new one), and `k-focus` when the
window has been given, or has lost, the keyboard.

#### See also

`ekey`, `form`.

## k-shift-mask, k-ctrl-mask, k-alt-mask

Facility; in the `facility` library (`lib facility`).

#### Stack

```text
k-shift-mask   ( -- u )
k-ctrl-mask    ( -- u )
k-alt-mask     ( -- u )
```

#### Description

Constants, one bit each: 256, 512 and 1024. When a special key is pressed with <kbd>SHIFT</kbd>,
<kbd>CTRL</kbd> or <kbd>ALT</kbd> held, and the terminal reports it, `ekey`'s number has the matching bit
set as well as the key's own value. `and` with a mask tests for the modifier, and `255 and` leaves the key.

#### See also

`ekey`, `k-up`.

## k-up, k-down, k-left, k-right, k-home, k-end, k-prior, k-next, k-insert, k-delete, k-f1, k-f12

Facility; in the `facility` library (`lib facility`).

#### Stack

```text
k-up ... k-f12   ( -- u )
```

#### Description

Constants: the numbers `ekey` gives for the special keys. `k-prior` and `k-next` are
<kbd>PAGE UP</kbd> and <kbd>PAGE DOWN</kbd>. The function keys are `k-f1` to `k-f12`, twelve numbers in a
row.

| Key | Value | Key | Value |
|---|---|---|---|
| `k-up` | 128 | `k-end` | 133 |
| `k-down` | 129 | `k-insert` | 134 |
| `k-right` | 130 | `k-delete` | 135 |
| `k-left` | 131 | `k-prior` | 136 |
| `k-home` | 132 | `k-next` | 137 |
| `k-f1` ... `k-f12` | 138 ... 149 | | |

A program should compare with the names, not the numbers.

#### See also

`ekey`, `ekey>fkey`, `k-shift-mask`, `k-resize`.

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

```text
key?   ( -- flag )
```

#### Description

True if a key has been pressed and is waiting to be read; false if not. It does not wait, and it does not
take the key: `key` then reads it at once.

This is what lets a program do something else while it watches the keyboard: a game's loop, or a display
that updates until a key is pressed.

The first `key?` puts the keyboard into the mode where each key comes as it is pressed, unshown, and it
stays so until the next line is read.

#### See also

`key`, `ekey?`, `ms`.

#### Example

```text
lib facility
: spin  begin [char] . emit 100 ms key? until key drop ;
```

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

## library, end-library

Search-Order; in the `search` library (`lib search`). HyForth's own words, not the standard's.

#### Stack

```text
library       ( "name" -- wid )
end-library   ( wid -- )
```

#### Description

Bracket the definitions of a library that has a word list of its own. `library` makes a new word list,
defines *name* as a constant that leaves its identifier, puts the list first in the search order, and makes
it the one that definitions go into; it leaves the identifier of the word list that definitions went into
before. `end-library` takes that identifier and sends definitions back there. The library's word list stays
in the search order, so its words go on being found.

A file written this way keeps its names apart from the system's: they can be searched on their own with
`search-wordlist`, and taken out of the order, with `previous` or a `set-order`, without being lost.

#### See also

`wordlist`, `definitions`, `only`, `lib`.

#### Example

```session
/> lib search
/> library shapes
/> : square  dup * ;
/> end-library
/> 5 square .
25
/> s" square" shapes search-wordlist nip .
-1
```

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

```text
line   ( x0 y0 x1 y1 -- )
box    ( x0 y0 x1 y1 -- )
bar    ( x0 y0 x1 y1 -- )
```

#### Description

Draw, in the pen's colour: `line` a straight line from one point to the other; `box` the outline of the
rectangle with those two points as opposite corners; `bar` the same rectangle filled.

#### See also

`plot`, `circle`, `fd`.

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
/> 1 block 1024 bl fill  s" .( one ) " 1 block swap move update
/> 2 block 1024 bl fill  s" .( two ) " 2 block swap move update
/> flush
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

```text
=mkdir   ( c-addr u mode -- ior )
```

#### Description

Makes a directory. *mode* is the permission bits to give it; 0 leaves them to the system. The ior is 0, or
the reason: −545, `already exists`; −544 if the directory it is to go in isn't there.

The name has an `=` so that `mkdir` typed at the shell's prompt is still the `mkdir` program.

#### See also

`set-dir`, `open-dir`, `delete-file`.

#### Example

```session
/> lib hydra
/> s" /ram/newdir" 0 =mkdir .
0
/> s" /ram/newdir" 0 =mkdir .
-545
```

## mouse, mouse-wait

The screen; in the `video` library (`lib video`).

#### Stack

```text
mouse        ( -- x y buttons )
mouse-wait   ( -- x y buttons )
```

#### Description

The mouse of a Vera X. `mouse` leaves where the pointer is now, in the screen's dots, and which buttons are
held: 1 for the left, 2 the middle, 4 the right, added together. `mouse-wait` waits until the mouse moves or
a button changes, and then leaves the same three.

#### See also

`ekey`, `vsync`; section 16.8.

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

```text
ms   ( u -- )
```

#### Description

Waits at least *u* milliseconds. HyForth sleeps for the time, so other tasks have the processor meanwhile.
The system's clock ticks 200 times a second, so the wait is to the nearest 5 milliseconds.

<kbd>CTRL</kbd>-<kbd>C</kbd> ends a wait early.

#### See also

`pause`, `time&date`, `key?`.

#### Example

```session
/> lib facility
/> : slowly  3 0 do i . 200 ms loop ;
/> slowly
0 1 2
```

## >n

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
>n   ( c-addr u -- flag ) ( N: -- x | )
```

#### Description

Reads a number from a string. If the whole string is a number, in any of the forms that can be typed, the
number goes on the number stack and the flag is true. If it is not, nothing is added, and the flag is
false. Spaces around the number are allowed.

This is how a program turns what a person typed, or a line of a file, into a number, and finds out whether
it was one.

#### See also

`n>str`, `>number`, `accept`, `s>n`.

#### Example

```session
/> lib numbers
/> s" 2/3" >n . n.
-1 2/3
/> s" abc" >n . ndepth .
0 0
```

## n+, n-, n*, n/

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n+   ( N: x y -- z )
n-   ( N: x y -- z )
n*   ( N: x y -- z )
n/   ( N: x y -- z )
```

#### Description

Add, subtract, multiply and divide the top two numbers of the number stack, exactly. The numbers may be of
any kind and any size, and need not be of the same kind: integers, fractions, decimals, complex numbers.
`n-` leaves *x* − *y*, and `n/` leaves *x* over *y*.

A result is given in the simplest kind that holds it. `n/` of two integers that don't divide evenly is a
fraction in its lowest terms, never a rounded number; a fraction and a decimal make a fraction; a result
with no fractional part is an integer.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −10 | `division by zero` | `n/` by zero |
| −11 | `result out of range` | A number too big for the library: more than 255 bytes, about 600 digits |

#### See also

`n/mod`, `npow`, `s>n`, `+`, `d+`.

#### Example

```session
/> lib numbers
/> 1/3 1/6 n+ n.
1/2
/> 1.5 2 s>n n* n.
3
/> 7 s>n 2 s>n n/ n.
7/2
/> 123456789012345678901234567890 ndup n* n.
15241578753238836750495351562536198787501905199875019052100
```

## n., n.base

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n.       ( N: x -- )
n.base   ( c-addr u -- ) ( N: x -- )
```

#### Description

Print the top number of the number stack, with a space after it, and remove it. `n.` prints in the current
base. `n.base` prints in the base named by the string, any of Chapter 18's: `s" x"` hex, `s" b"` binary,
`s" o"` octal, a radix such as `s" 16r"`; with a `#` before the radix, the number is printed with that
prefix.

#### See also

`n.s`, `n>str`, `nformat`, `set-base`, `.`.

#### Example

```session
/> lib numbers
/> 2/3 n.
2/3
/> 255 s>n s" x" n.base 255 s>n s" b" n.base
FF 11111111
/> 255 s>n s" #16r" n.base
#16rFF
```

## n.s

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n.s   ( -- )
```

#### Description

Shows the number stack, and changes nothing: how many numbers are on it, in angle brackets, and then each
one, the top last. It is `.s` for the number stack.

#### See also

`ndepth`, `n.`, `.s`.

#### Example

```session
/> lib numbers
/> 2/3 1.25 100000000000 n.s
<3> 2/3 1.25 100000000000
```

## n/mod

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n/mod   ( N: x y -- rem quot )
```

#### Description

Divides one integer by another, and leaves the remainder and, on top, the whole quotient. The division is
symmetric, as HyForth's `/mod` is: the quotient is rounded toward zero, and the remainder has the sign of
*x*.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | A number that is not an integer |
| −10 | `division by zero` | *y* is zero |

#### See also

`n/`, `/mod`, `ngcd`.

#### Example

```session
/> lib numbers
/> 10 s>n 3 s>n n/mod n. n.
3 1
/> -7 s>n 2 s>n n/mod n. n.
-3 -1
```

## n=, n<, n>, n0=, n0<

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
n=    ( -- flag ) ( N: x y -- )
n<    ( -- flag ) ( N: x y -- )
n>    ( -- flag ) ( N: x y -- )
n0=   ( -- flag ) ( N: x -- )
n0<   ( -- flag ) ( N: x -- )
```

#### Description

The tests of `=`, `<`, `>`, `0=` and `0<`, for the number stack. Each removes its numbers and leaves a flag
on the data stack, where `if` and the rest expect one. Numbers are compared by value: `1/2 0.5 n=` is true.

#### See also

`ncompare`, `int?`, `=`.

#### Example

```session
/> lib numbers
/> 1/2 0.5 n= .
-1
/> 1/3 1/2 n< .
-1
/> 0.0 n0= .
-1
```

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

```text
n>s   ( -- n ) ( N: x -- )
n>d   ( -- d ) ( N: x -- )
```

#### Description

Move a number from the number stack to the data stack, as a signed cell or a signed double. A number with a
fractional part loses it: the whole part, toward zero, is what arrives.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −11 | `result out of range` | The number doesn't fit a cell (`n>s`) or a double (`n>d`) |
| −24 | `invalid numeric argument` | A complex number |

#### See also

`s>n`, `truncate`, `d>s`.

#### Example

```session
/> lib numbers
/> 7/2 n>s .
3
/> 100000000000 n>s .
result out of range
```

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

```text
nand      ( N: x y -- z )
nor       ( N: x y -- z )
nxor      ( N: x y -- z )
ninvert   ( N: x -- y )
```

#### Description

The bitwise operations `and`, `or`, `xor` and `invert`, on integers of any length. (`nand` is the numbers'
"and", not "not and".) A negative integer is taken as two's complement, with its sign bit going on for ever:
`0 ninvert` is −1.

#### See also

`nlshift`, `nbit?`, `and`.

#### Example

```session
/> lib numbers
/> 12 s>n 10 s>n nand n. 12 s>n 10 s>n nor n. 12 s>n 10 s>n nxor n.
8 14 6
/> 0 s>n ninvert n.
-1
```

## nbit?

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nbit?   ( u -- flag ) ( N: x -- )
```

#### Description

Removes an integer from the number stack and leaves a flag: true if its bit *u* is 1. Bit 0 is the lowest.
A negative integer has every bit above its top one set.

#### See also

`nand`, `nrshift`, `tbit`.

#### Example

```session
/> lib numbers
/> 5 s>n 0 nbit? . 5 s>n 1 nbit? .
-1 0
```

## nbytes, nfrom-bytes

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nbytes        ( -- c-addr u ) ( N: x -- )
nfrom-bytes   ( c-addr u -- ) ( N: -- x )
```

#### Description

A number in the Hydra's stored format: the compact form, of a length that depends on the number, that every
language uses to keep a number in a file (hylang's `number-bytes`, HyBASIC's `MKN$`). `nbytes` removes a
number and leaves the address and length of its bytes; `nfrom-bytes` takes such bytes and puts the number
they hold back on the number stack.

The bytes `nbytes` leaves are in a buffer of the library's own: write or copy them before another number
word is used.

#### See also

`n>str`, `>n`, `write-file`.

#### Example

```session
/> lib numbers
/> 258 s>n nbytes nip .
3
/> 258 s>n nbytes nfrom-bytes n.
258
```

## ncompare

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
ncompare   ( -- n ) ( N: x y -- )
```

#### Description

Compares the top two numbers of the number stack, removes them, and leaves on the data stack −1 if *x* is
less than *y*, 0 if they are equal, and 1 if it is greater. Numbers are compared by value, whatever their
kind: 1/2 and 0.5 are equal.

#### See also

`n=`, `n<`, `compare`.

#### Example

```session
/> lib numbers
/> 1/2 0.5 ncompare .
0
/> 1/3 0.5 ncompare .
-1
```

## ndepth

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
ndepth   ( -- u )
```

#### Description

Leaves, on the data stack, how many numbers are on the number stack.

The **number stack** is the numbers library's own: it holds the Hydra's numbers (Chapter 18), which are of
any length and so can't be cells. With the library loaded, a number typed that a cell can't hold goes there
by itself: `2/3`, `1.25`, `#xFF`, `2i`, `100000000000000000000`. A number that fits a cell, such as `42`,
still goes on the data stack, and `s>n` moves it across.

The number stack is in a RAM bank of 8K, so it holds a great many small numbers or a few enormous ones.
Unlike the data stack, it is not emptied when an error returns HyForth to the prompt: what was on it is
still there.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −44 | `number stack overflow` | No room for another number (any word that adds one) |
| −45 | `number stack underflow` | A word took more numbers than there were |

#### See also

`n.s`, `ndrop`, `s>n`, `depth`.

#### Example

```session
/> lib numbers
/> 2/3 1.25 ndepth .
2
/> n.s
<2> 2/3 1.25
/> n+
/> n.
23/12
/> n+
number stack underflow
```

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

```text
nfib   ( N: n -- f )
```

#### Description

Replaces a whole number *n*, 0 or more, by the *n*th Fibonacci number: 0, 1, 1, 2, 3, 5 ... , exactly,
however long.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | *n* is negative, or not whole |

#### Example

```session
/> lib numbers
/> 30 s>n nfib n.
832040
/> 100 s>n nfib n.
354224848179261915075
```

## nformat

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nformat   ( c-addr u -- ) ( N: x1 ... xn -- )
```

#### Description

Prints a string in which each `{}` is replaced by a number from the number stack: the first `{}` by the
deepest of them, so the numbers appear in the order they were put there. As many numbers are removed as
there are places.

A place can name a base, as `{x}` does: that number is printed in it.

#### See also

`n.`, `n.base`, `."`.

#### Example

```session
/> lib numbers
/> 1/2 0.25 s" {} and {}" nformat
1/2 and 0.25
/> 255 s>n s" hex {x}" nformat
hex FF
```

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

```text
nlshift   ( u -- ) ( N: x -- y )
nrshift   ( u -- ) ( N: x -- y )
```

#### Description

Shift an integer's bits *u* places, the count coming from the data stack. `nlshift` multiplies by 2 to the
power *u*, and loses nothing, since the integer simply grows. `nrshift` divides by 2 to the power *u*,
rounding down, and keeps the sign.

#### See also

`nand`, `nbit?`, `lshift`.

#### Example

```session
/> lib numbers
/> 1 s>n 100 nlshift n.
1267650600228229401496703205376
/> -8 s>n 1 nrshift n.
-4
```

## nnegate, nabs, ngcd, npow

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nnegate   ( N: x -- y )
nabs      ( N: x -- y )
ngcd      ( N: x y -- z )
npow      ( N: x y -- z )
```

#### Description

`nnegate` changes a number's sign. `nabs` leaves a real number without its sign, and a complex number's modulus,
its distance from 0 (`3-4i nabs` is 5). `ngcd` leaves the greatest common divisor of two integers: the largest
number that divides both.

`npow` leaves *x* to the power *y*. With a whole *y* the result is exact, whatever *x* is: `2 100` gives all
31 digits, and a negative power gives a fraction. With a fractional *y* the result is exact when the root
is, and otherwise a decimal to the precision in `digits`. A negative *x* to a fractional power, or a complex *x*
or *y*, gives the principal value (`-8 s>n 1/3 npow` is `1+1.73205080757i`, `1i 1i npow` is 0.207879576351).

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | `ngcd` of numbers that are not integers |
| −46 | `outside the function's domain` | `npow`: 0 to a complex power whose real part is 0 or less |

#### See also

`n*`, `nsqrt`, `digits`, `negate`.

#### Example

```session
/> lib numbers
/> 12 s>n 18 s>n ngcd n.
6
/> 2 s>n 100 s>n npow n.
1267650600228229401496703205376
/> 2 s>n -1 s>n npow n.
1/2
/> 2 s>n 1/2 npow n.
1.41421356237
```

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

```text
note         ( task n -- )
note-group   ( group n -- )
```

#### Description

Send note number *n* to a task, or to every task of a note group. The notes with meanings of their own are 1
(interrupt, which is what <kbd>CTRL</kbd>-<kbd>C</kbd> sends), 3 (hangup), 4 (alarm) and 9 (kill, which a
task cannot refuse); 16 to 31 are for programs to use between themselves.

What a note does is up to the task that gets it. Most programs end. A failure, such as a task that isn't
there, is thrown.

#### See also

`on-note`, `spawn`, `wait`; the command `kill`.

#### Example

```session
/> lib hydra
/> sleep 30 &
/> s" apid" getenv evaluate 9 note
/> 99 4 note
no such task
```

## note-of

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
note-of   ( c-addr u -- n )
```

#### Description

Leaves the MIDI number of a note given by name: a letter A to G, a `#` for a sharp or a `b` for a flat if
there is one, and the octave. `C4` is middle C, 60.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | The string is not a note's name |

#### See also

`snd-note`, `tune`.

#### Example

```session
/> lib sound
/> s" A4" note-of .
69
/> s" C#4" note-of . s" Db4" note-of .
61 61
```

## nrandom, nseed

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nrandom   ( N: n -- r )
nseed     ( u -- )
```

#### Description

`nrandom` replaces a whole number *n* by a random whole number from 0 to *n* − 1. *n* may be as large as
any integer. With an *n* of 0 it gives a random decimal between 0 and 1.

`nseed` starts the generator from a number of the program's choosing. The same seed gives the same series
again, which is what testing a game or a simulation needs.

#### See also

`random`, `rand`, `nfib`.

#### Example

```session
/> lib numbers
/> 42 nseed 6 s>n nrandom n.
5
/> 42 nseed 6 s>n nrandom n.
5
```

## nsqrt, nexp, nlog, nsin, ncos, ntan, natan, npi

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
nsqrt   nexp   nlog   nsin   ncos   ntan   natan   ( N: x -- y )
npi     ( N: -- x )
```

#### Description

The math functions: the square root, *e* to a power, the natural logarithm, the sine, cosine and tangent of
an angle in radians, the angle whose tangent is given, and pi.

Where there is an exact answer it is given: `16 nsqrt` is 4, and the square root of a negative number is an
imaginary one. Otherwise the result is a decimal with as many significant digits as the variable `digits`
holds, 12 unless a program changes it. A complex argument gives the principal value, each part so rounded:
`3+4i nsqrt` is `2+1i`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −46 | `outside the function's domain` | The logarithm of 0; the arc tangent of `1i` or `-1i` |

#### See also

`digits`, `npow`, `to-fixed`.

#### Example

```session
/> lib numbers
/> 2 s>n nsqrt n.
1.41421356237
/> 16 s>n nsqrt n. -4 s>n nsqrt n.
4 2i
/> 1 s>n nexp n.
2.71828182846
/> npi n.
3.14159265359
/> 3+4i nsqrt n. 1i nexp n.
2+1i 0.540302305868+0.841470984808i
```

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

```text
on-note   ( xt -- )
```

#### Description

Names the word that is to be given the notes this HyForth receives. The word must take the note's number
and leave a flag, ( *n* -- *flag* ): true if it has dealt with the note and the program should go on, false
if the note should be treated as <kbd>CTRL</kbd>-<kbd>C</kbd> is, and stop what is running (the throw code
−28).

The word is not called at the instant a note arrives, but at the next word the interpreter runs or the next
turn of a loop, so it may do anything a word can.

<kbd>CTRL</kbd>-<kbd>C</kbd> itself and the kill note are not passed to it. `0 on-note` takes the word
away again: with none, any note but <kbd>CTRL</kbd>-<kbd>C</kbd> ends HyForth.

#### See also

`note`, `catch`.

#### Example

```session
/> lib hydra
/> : heard  ( n -- flag )  ." note " . true ;
/> ' heard on-note
/> sys-getpid 20 note
note 20
/> 0 on-note
```

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

```text
output-of   ( xt -- c-addr u )
```

#### Description

Runs a Forth word, and leaves what it printed as a string: nothing appears on the screen. It is `sh-out` for
HyForth's own output. The string is in the free space above `here`, as `sh-out`'s is, so the word run must
not add to the dictionary.

If the word throws an error, the error is thrown again when the output has been put back.

#### See also

`sh-out`, `piped`, `type`.

#### Example

```session
/> : say  ." from forth" ;
/> ' say output-of nip .
10
/> ' say output-of type
from forth
```

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

```text
page   ( -- )
```

#### Description

Clears the window, and puts the cursor at its top left corner.

#### See also

`at-xy`, `clear-below`.

## palette!, sprite!, sprite-at, sprite-off, vsync

The screen; in the `video` library (`lib video`).

#### Stack

```text
palette!     ( index rgb -- )
sprite!      ( n c-addr -- )
sprite-at    ( n x y -- )
sprite-off   ( n -- )
vsync        ( -- )
```

#### Description

`palette!` sets a palette entry to a colour written as `$RGB`: four bits each of red, green and blue.
`sprite!` sets sprite *n*'s eight attribute bytes from memory at *c-addr*; `sprite-at` moves a sprite to
*x*, *y*; `sprite-off` hides it.

`vsync` waits for the start of the next frame, of which there are about sixty a second. A program that
moves things on the screen does so once a frame, after `vsync`, so that the movement is smooth.

#### See also

`vram!`, `pen`, `ms`.

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

```text
pause   ( -- )
```

#### Description

Gives up the rest of HyForth's turn of the processor, so that any other task that is ready runs now. HyForth
goes on at its next turn. A loop that is only waiting for something another task will do should `pause`
each time round, or better, wait with `ms`.

#### See also

`ms`, `key?`.

## pc?

The devices; in the `pc` library, a file of Forth in `/lib/forth` (`lib pc`).

#### Stack

```text
pc?   ( -- flag )
```

#### Description

True if the PC tool answers on the serial line, so that the files of `/pc` can be used; false, after a wait
of a second, if nothing answers. `/pc`'s files themselves are ordinary files, for the file words.

#### See also

`open-file`, `file-status`; section 9.9.

## pen, border

The screen; in the `video` library (`lib video`).

#### Stack

```text
pen      ( colour -- )
border   ( colour -- )
```

#### Description

`pen` sets the colour that the drawing words draw in from now on: a number of the palette, within what the
bitmap's depth allows. `border` sets the colour of the screen's border, outside the picture.

#### See also

`plot`, `palette!`, `bitmap`.

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

Facility; in the `facility` library (`lib facility`). HyForth's own words, not the standard's.

#### Stack

```text
plain   bold   dim   underline   blink   reverse   ( -- )
```

#### Description

The styles of the text printed from now on. `bold`, `dim`, `underline`, `blink` and `reverse` (the text's
and the background's colours exchanged) each turn a style on, and they add up. `plain` turns every style
off, and puts the colours back to the terminal's usual ones.

How each style looks depends on the terminal: not every one shows `dim` or `blink`.

#### See also

`color`, `sgr`, `cursor-off`.

## plot

The screen; in the `video` library (`lib video`).

#### Stack

```text
plot   ( x y -- )
```

#### Description

Draws one dot in the pen's colour. Positions count from 0 0 at the top left corner of the bitmap, *x*
across and *y* down. A dot outside the bitmap is not drawn, and is not an error.

#### See also

`line`, `pen`, `bitmap`.

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

```text
prompt    ( c-addr u -- )
prompt2   ( c-addr u -- )
```

#### Description

Set the text of the prompt. `prompt` sets the one shown when HyForth is ready for a line, and `prompt2` the
one shown while a definition that was begun on an earlier line is still open. Each takes a string of at
most 31 characters, in which these stand for something that changes:

| Written | Shows |
|---|---|
| `%p` | The whole path of the current directory |
| `%d` | The current directory on its disk |
| `%v` | The SD card the directory is on, as `0:`; nothing for a directory that is not on a card |
| `%t` | HyForth's task number |
| `%w` | Its window's number |
| `%%` | A `%` |

HyForth starts with the prompt `%v%d> `, which shows `/ram> ` in the RAM disk and `0:/games> ` on a card, and
with a tab as the second prompt.

#### See also

`cd`, `:`.

#### Example

```text
/> s" [task %t] " prompt
[task 1] s" %v%d> " prompt
/> s" ... " prompt2
/> : unfinished
... 1 2 + ;
/> unfinished .
3
```

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

Random numbers; in the `random` library (`lib random`).

#### Stack

```text
rand     ( -- u )
rand32   ( -- ud )
```

#### Description

The next random number from the library's generator: `rand` 16 bits of it, as an unsigned cell, and `rand32`
all 32, as an unsigned double.

The generator is seeded from the clock and the tick count as the library loads, so each run gives a
different series, unless `rseed` sets it.

#### See also

`random`, `rseed`, `nrandom`.

## random

Random numbers; in the `random` library (`lib random`).

#### Stack

```text
random   ( u -- u' )
```

#### Description

A random number from 0 to *u* − 1: `6 random 1+` is the throw of a die. For a *u* of 0 the result is 0.

#### See also

`rand`, `rseed`, `nrandom`.

#### Example

```session
/> lib random
/> 42. rseed  6 random . 6 random .
1 1
```

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

Random numbers; in the `random` library (`lib random`).

#### Stack

```text
rseed   ( ud -- )
```

#### Description

Starts the generator from a number of the program's choosing, a double. The same seed gives the same series
again, for testing. (A seed of 0 is taken as 1.)

#### See also

`rand`, `random`.

#### Example

```session
/> lib random
/> 42. rseed rand u.
17704
/> 42. rseed rand u.
17704
```

## run

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
run   ( c-addr u -- status )
```

#### Description

Runs a program, given as a string holding its name and its arguments with spaces between them, waits for it,
and leaves its exit code. The line is split at its spaces and nothing more: there is no quoting, no pattern
and no redirection. A name with no `/` in it is looked for in `/bin`.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −544 | `not found` | There is no such program |

#### See also

`sh`, `spawn`, `wait`.

#### Example

```session
/> lib hydra
/> s" echo one two" run .
one two
0
/> s" nothere" run .
not found
```

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

```text
s>n   ( n -- ) ( N: -- x )
d>n   ( d -- ) ( N: -- x )
```

#### Description

Move a number from the data stack to the number stack: `s>n` a signed cell, and `d>n` a signed double. This
is how a small number, which HyForth reads as a cell, joins the big ones: `2 s>n`.

#### See also

`n>s`, `>n`, `s>d`.

#### Example

```session
/> lib numbers
/> 100 s>n 10 s>n npow n.
100000000000000000000
/> 70000. d>n n.
70000
```

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

Bits; in the `bits` library (`lib bits`).

#### Stack

```text
sbit   ( n b -- n' )
cbit   ( n b -- n' )
```

#### Description

`sbit` leaves *n* with its bit *b* set to 1, and `cbit` with it cleared to 0. The other bits are as they
were.

#### See also

`tbit`, `or`, `and`, `lshift`.

#### Example

```session
/> lib bits
/> 0 3 sbit .
8
/> 15 0 cbit .
14
```

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

```text
search-wordlist   ( c-addr u wid -- 0 | xt 1 | xt -1 )
```

#### Description

Looks for a name, given as a string, in one word list, whether or not that list is in the search order. If
it is not there the result is 0. If it is, the word's execution token is left, with 1 above it for an
immediate word and −1 for any other.

Capitals and small letters are the same in a name.

#### See also

`find`, `'`, `wordlist`, `traverse-wordlist`.

#### Example

```session
/> lib search
/> s" dup" forth-wordlist search-wordlist . drop
-1
/> s" if" forth-wordlist search-wordlist . drop
1
/> s" nothere" forth-wordlist search-wordlist .
0
```

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

```text
seg-bank!   ( seg n -- ior )
```

#### Description

Puts bank *n* of a shared segment at `bank-window`: the bank is then ordinary memory there, as one of the
task's own is after `bank!`. The segment must be one this task is attached to: `sys-seg-create` makes one,
and attaches its maker, and `sys-seg-attach` attaches another task that has its number.

Like `bank!`, it can't be used in a definition compiled into the code banks: see `code-banks`.

#### See also

`bank!`, `bank-window`, `code-banks`; section 17.9.

## send

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
send   ( "n line" -- )
```

#### Description

Types a line into another window. The first word of the rest of the line is a window's number, and what
follows it is given to that window's shell as if it had been typed there, with <kbd>ENTER</kbd> after it.
A line can be 126 characters at most.

With it one window can drive another: start a long job there, or a program whose output should be kept
apart.

#### See also

`spawn`; Chapter 10; the command `new-window`.

## set-base, get-base

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
set-base   ( c-addr u -- )
get-base   ( -- c-addr u )
```

#### Description

The base in which numbers are read and printed, by name. `set-base` takes any of Chapter 18's base strings,
which go far beyond what the variable `base` can say: `s" x"` and `s" b"`, but also bases with digits of
their own, and balanced ones. `get-base` leaves the name of the base in force, `d` for decimal.

Cells and the number stack's numbers share the one setting. Setting `base`, or saying `hex` or `decimal`,
sets it for both.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | The string names no base |

#### See also

`base`, `n.base`, `hex`; sections 18.3 to 18.9.

#### Example

```session
/> lib numbers
/> get-base type
d
/> s" x" set-base  255 s>n n.  decimal
255
/> 255 s>n s" x" set-base n. decimal
FF
```

## set-date, rtc

The devices; in the `clock` library, a file of Forth in `/lib/forth` (`lib clock`).

#### Stack

```text
set-date   ( c-addr u -- )
rtc        ( -- c-addr u )
```

#### Description

`set-date` sets the clock from a string in the form `2026-10-13 09:41:00`; with a clock chip fitted, the
chip is set too. `rtc` leaves the text that says what the system found for a clock chip as it started:
`running`, `stopped` (it needs setting) or `none`, with `battery low` added if its battery is running down.

Reading the clock needs no library: `time&date`, in the Facility library, gives it.

#### See also

`time&date`; section 17.5.

#### Example

```session
/> lib clock
/> rtc type
none
/> s" nonsense" set-date
set-date: invalid argument
```

## sgr

Facility; in the `facility` library (`lib facility`). A HyForth word, not the standard's.

#### Stack

```text
sgr   ( n -- )
```

#### Description

Sends the terminal any one of its text attributes by number: the sequence `ESC [` *n* `m`. The words for the
styles and the colours are built on it; `sgr` reaches the attributes that have no word (section 13.7 lists
the ones the Hydra's console knows).

#### See also

`plain`, `color`.

## sh

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
sh   ( c-addr u -- status )
```

#### Description

Runs a command line, given as a string, with the shell rc (`rc -c`), waits for it to finish, and leaves its
exit status: 0 for success. Pipes, patterns, redirection and variables in the line are all rc's to deal
with.

This is the way to run a command from inside a definition, where a line can't simply be typed. For a single
program with plain arguments, `run` does without the shell.

#### See also

`run`, `sh-out`, `%`, `status`.

#### Example

```session
/> lib hydra
/> s" echo hi | wc -c" sh .
      3
0
/> s" ls /nothere" sh .
ls: /nothere: not found
1
```

## sh-out

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
sh-out   ( c-addr u -- c-addr2 u2 )
```

#### Description

Runs a command line with rc, as `sh` does, and leaves what it wrote to standard output as a string. Its exit
code is in `status`. What it wrote to standard error still goes to the screen.

The string is in the free space above `here`, and holds as much of the output as fits there. It is good
only until something is added to the dictionary: use it, or copy it, before defining anything.

#### See also

`sh`, `output-of`, `piped`, `status`.

#### Example

```session
/> s" echo hi" sh-out type
hi
/> s" ls /rom/lib/shell" sh-out nip .
15
```

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

```text
snd-bend    ( ch n -- )
snd-glide   ( ch n -- )
```

#### Description

`snd-bend` bends the pitch of a channel's note by *n* 64ths of a semitone, up for a positive number and
down for a negative one. `snd-glide` moves the sounding note to note *n* without starting it again: the
pitch changes, and the note's attack is not repeated.

#### See also

`snd-note`, `snd-freq`.

## snd-claim, snd-release, snd-claim-psg, snd-release-psg

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-claim         ( mask -- )
snd-release       ( mask -- )
snd-claim-psg     ( mask -- )
snd-release-psg   ( mask -- )
```

#### Description

Claim sound channels, so that no other program's sounds reach them, and give them back. The mask has a bit
for each channel: bit *n* is channel *n* of the YM2151's eight, and for the `-psg` pair, voice *n* of the
Vera X's PSG, which is channel 8 + *n*.

A claim lasts until it is released or HyForth ends. While HyForth has a channel, the `play` program can't
use it: `snd-mml`, `snd-chord` and `snd-play` want it released first.

#### See also

`snd-note`, `snd-reset`; Chapter 15.

## snd-drum

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-drum   ( ch n -- )
```

#### Description

Plays a drum on a channel: *n* is a General MIDI drum number, 35 being a kick drum and 38 a snare.

#### See also

`snd-note`, `tune`.

## snd-freq

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-freq   ( ch hz -- )
```

#### Description

Starts a note at a frequency given in hertz, for a pitch that is not one of the scale's notes. A frequency
of 0 ends it.

#### See also

`snd-note`, `snd-bend`.

## snd-level, snd-volume, snd-pan

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-level    ( ch v -- )
snd-volume   ( v -- )
snd-pan      ( ch pan -- )
```

#### Description

`snd-level` sets one channel's volume, 0 to 127. `snd-volume` sets the master volume for all sound, 0 to
200, where 100 is "as written". `snd-pan` chooses a channel's speakers: 1 the left, 2 the right, 3 both.

#### See also

`snd-note`, `snd-reset`.

## snd-mml, snd-chord, snd-play

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-mml     ( ch c-addr u -- status )
snd-chord   ( ch c-addr u -- status )
snd-play    ( c-addr u times -- status )
```

#### Description

Play music with the `play` program, and leave its exit status: 0 if all went well.

`snd-mml` plays a line of MML, the score language of section 15.6, on a channel. `snd-chord` plays the
notes of such a line together, as a chord, on that channel and the ones after it. `snd-play` plays a song
file, a ZSM, the number of times given; 0 plays its loop until <kbd>CTRL</kbd>-<kbd>C</kbd>.

Each waits until the music is over. `play` is another program, and needs the channels free: release any that
HyForth has claimed first.

#### See also

`tune`, `snd-claim`; the command `play`.

#### Example

```session
/> lib sound
/> 0 s" cde" snd-mml .
0
/> s" /nothere.zsm" 1 snd-play .
play: /nothere.zsm: not found
1
```

## snd-note, snd-off

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-note   ( ch n -- )
snd-off    ( ch -- )
```

#### Description

`snd-note` starts a note on a channel: *n* is a MIDI note number, 60 being middle C and each semitone one
more. `snd-off` ends the channel's note, which then dies away as its instrument does.

A note sounds until it is ended, so a note of a given length is `snd-note`, a wait, and `snd-off`.

#### See also

`note-of`, `snd-patch`, `snd-freq`, `tune`, `ms`.

#### Example

```session
/> lib sound
/> lib facility
/> 0 5 snd-patch
/> 0 60 snd-note  500 ms  0 snd-off
```

## snd-patch

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-patch   ( ch p -- )
```

#### Description

Loads an instrument into a channel: patch *p*, one of the 163 that the sound driver holds, 0 to 162
(Chapter 15). The notes played on the channel afterward have that instrument's sound.

A channel is 0 to 7 for the YM2151's voices, and 8 to 23 for the PSG's, with a Vera X.

#### See also

`snd-note`, `snd-wave`, `snd-level`.

## snd-regs

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-regs   ( c-addr -- )
```

#### Description

Copies the YM2151's 256 registers, as they were last written, into memory at *c-addr*, which must have room
for 256 bytes. The chip itself can't be read: these are the driver's record of what has been sent to it.

#### See also

`snd-reg`, `dump`.

#### Example

```session
/> lib sound
/> create regs 256 allot
/> 32 199 snd-reg  regs snd-regs  regs 32 + c@ .
199
```

## snd-reset

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-reset   ( -- )
```

#### Description

Silences the sound chips and clears every setting: patches, volumes, bends. The channels a program has
claimed stay claimed.

#### See also

`snd-off`, `snd-volume`, `snd-release`.

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

## snd-wave

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-wave   ( ch w width -- )
```

#### Description

Sets the waveform of a PSG channel, 8 to 23: *w* is 0 for a pulse, 1 a sawtooth, 2 a triangle, 3 noise.
*width* is a pulse's width, 0 to 63, where 63 is a square wave.

#### See also

`snd-patch`, `snd-note`.

## spawn

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
spawn   ( c-addr u -- task )
```

#### Description

Starts a program, given as a string of its name and arguments as `run` takes them, and does not wait for
it: the task's number is left, and both go on at once. The program has a note group of its own, and its
task number is also put in the environment variable `apid`, as rc does for a command started with `&`.

`wait` waits for it to end.

#### See also

`wait`, `run`, `note`, `status`.

#### Example

```session
/> s" sleep 1" spawn wait status .
0
```

## spi, spi-read, spi-mode

The devices; in the `spi` library, a file of Forth in `/lib/forth` (`lib spi`).

#### Stack

```text
spi        ( dev c-addr u -- )
spi-read   ( dev c-addr u -- )
spi-mode   ( dev mode -- )
```

#### Description

The SPI devices, 0 to 15 (section 17.4): 0 to 7 are the board's headers, and 8 to 15 the slots' cards.

`spi` is one transaction: the device is selected, the *u* bytes at *c-addr* are sent, and the bytes the
device sent back meanwhile are put in their place, in the same memory. `spi-read` clocks *u* bytes in,
sending `$FF` for each. `spi-mode` sets the clock's mode for the device, 0 or 3.

A device with nothing connected gives `$FF` for every byte. One that is an SD card the system is using
can't be opened: `busy`.

#### See also

`i2c-read`, `gpio`.

#### Example

With nothing connected to device 3:

```session
/> lib spi
/> create tx  1 c, 2 c, 3 c,
/> 3 tx 3 spi
/> tx c@ . tx 1+ c@ . tx 2 + c@ .
255 255 255
```

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

```text
status   ( -- n )
```

#### Description

Leaves the exit code of the last command that was run and waited for: a command line typed at the prompt, a
`%` line, or a task that `wait` waited for. 0 means it succeeded.

#### See also

`%`, `wait`, `sh`, `exits`.

#### Example

```session
/> ls /nothere
ls: /nothere: not found
/> status .
1
/> echo fine
fine
/> status .
0
```

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

```text
sys   ( addr a x y -- a x y p )
```

#### Description

Calls machine code. The 65C02's A, X and Y registers are loaded from the low bytes of *a*, *x* and *y*, and
the code at *addr* is called as a subroutine. When it returns, with `RTS`, the three registers as it left
them are on the stack, and above them the processor's flags, *p*: the carry is bit 0.

The code can be bytes laid down with `c,`, a routine loaded from a file, or an entry of the system's.
HyForth's own data stack is kept in the zero page from `$22` up, indexed by X: code that uses zero page must
keep clear of it. (The system's calls are easier to make with the `sys-` words.)

#### See also

`sys-`, `c,`, `disasm`; Chapter 26.

#### Example

The three bytes are `LDA #$2A` and `RTS`:

```session
/> lib hydra
/> create code  $A9 c, $2A c, $60 c,
/> code 0 0 0 sys drop 2drop .
42
```

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

## task-args, task-cwd, task-env, task-ns, task-regs, task-mem, task-ram

The devices; in the `proc` library, a file of Forth in `/lib/forth` (`lib proc`).

#### Stack

```text
task-args   ( task -- c-addr u )
task-cwd    ( task -- c-addr u )
task-env    ( task -- c-addr u )
task-ns     ( task -- c-addr u )
task-regs   ( task -- c-addr u )
task-mem    ( task addr c-addr u -- )
task-ram    ( task bank offset c-addr u -- )
```

#### Description

A look inside any task, through its files in `/proc` (section 17.8). The first five leave text, in a buffer
of the library's, 512 characters at most: the task's arguments; its current directory; its environment, a
line for each variable; its namespace, as the `bind` and `mount` lines that made it; and the processor's
registers as they were when it last stopped running.

`task-mem` reads *u* bytes of the task's memory, from *addr* as the task sees its 64K, into memory at
*c-addr*; `task-ram` reads from one of its RAM banks. The memory and registers of the kernel's task and of
the drivers can't be read: `not allowed`.

#### See also

`sys-getpid`, `note`; the commands `ps` and `db`.

#### Example

```session
/> lib proc
/> 1 task-args type
-l
/> 1 task-cwd type
/
/> 9 task-args type
task-args: not found
```

## tbit

Bits; in the `bits` library (`lib bits`).

#### Stack

```text
tbit   ( n b -- n flag )
```

#### Description

Tests one bit of a cell: the flag is true if bit *b* of *n* is 1. Bit 0 is the lowest and bit 15 the
highest. *n* stays on the stack, under the flag, so several bits can be tested in turn.

Only the low four bits of *b* count: bit 16 is bit 0.

#### See also

`sbit`, `and`, `nbit?`.

#### Example

```session
/> lib bits
/> 5 0 tbit . drop
-1
/> 5 1 tbit . drop
0
```

## text

The screen; in the `video` library (`lib video`).

#### Stack

```text
text   ( x y c-addr u -- )
```

#### Description

Draws a string on the bitmap, in the pen's colour and the console's 8 by 8 font, with the top left of its
first character at *x*, *y*. At most 40 characters are drawn.

Text drawn this way is part of the picture: unlike what `type` prints, it does not scroll.

#### See also

`type`, `at-xy`, `pen`.

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

```text
time&date   ( -- +n1 +n2 +n3 +n4 +n5 +n6 )
```

#### Description

Leaves the date and time now, as six numbers: the seconds (0 to 59), the minutes, the hours (0 to 23), the
day of the month, the month (1 to 12) and, on top, the year in full.

On a Hydra with no clock chip, whose clock has not been set, the time counts from the start of 1 January
2000 when it is switched on (section 17.5).

#### See also

`ms`; the command `date`.

#### Example

```text
/> lib facility
/> time&date . . . . . .
2026 10 13 9 41 0
```

## to-fixed

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
to-fixed   ( u -- ) ( N: x -- y )
```

#### Description

Makes the top number a decimal with at most *u* places after the point. Digits beyond that are cut off, not
rounded: two thirds to two places is 0.66. This is where a fraction is turned into something to print.

#### See also

`to-rational`, `nround`, `n.`.

#### Example

```session
/> lib numbers
/> 1 s>n 3 s>n n/ 4 to-fixed n.
0.3333
/> 2/3 2 to-fixed n.
0.66
```

## to-rational, rational.n, rational.d

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

```text
to-rational   ( N: x -- y )
rational.n    ( N: x -- n )
rational.d    ( N: x -- n )
```

#### Description

`to-rational` makes a decimal the exact fraction it equals, in its lowest terms. `rational.n` leaves the top
of that fraction, the numerator, and `rational.d` the bottom, the denominator; an integer's denominator is
1.

#### See also

`to-fixed`, `n/`, `ngcd`.

#### Example

```session
/> lib numbers
/> 0.25 to-rational n.
1/4
/> 3/4 rational.n n. 3/4 rational.d n.
3 4
```

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

```text
truncate   ( N: x -- n )
nfloor     ( N: x -- n )
nround     ( N: x -- n )
```

#### Description

Make a real number whole. `truncate` drops what follows the point, which rounds toward zero. `nfloor` rounds
down, toward minus infinity. `nround` rounds to the nearest whole number, and a number exactly halfway
between two goes to the even one: 0.5 to 0, 1.5 and 2.5 to 2, 3.5 to 4.

#### See also

`to-fixed`, `n>s`, `n/mod`.

#### Example

```session
/> lib numbers
/> 2.7 truncate n. -2.7 truncate n.
2 -2
/> -2.7 nfloor n.
-3
/> 2.5 nround n. 3.5 nround n.
2 4
```

## tune

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
tune   ( c-addr u ch tempo -- )
```

#### Description

Plays a tune on a channel, and returns when it is over. The string is a list of notes and their lengths,
with spaces between: a note's name as `note-of` takes it, or `-` for a rest, and then its length in beats,
1 to 255. *tempo* is the number of beats in a minute.

Each note is started on its beat and ended just before the next, in time with the system's tick.

#### See also

`note-of`, `snd-mml`, `snd-patch`.

#### Example

```session
/> lib sound
/> 0 5 snd-patch
/> s" C4 1 E4 1 - 1 G4 2" 0 240 tune
```

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

Blocks can be written in any order. Writing one beyond the end of the block file makes the file that long;
the blocks before it that were never written take no room on the disk, and read as zero bytes.

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

## vpeek, vpoke, vram@, vram!

The screen; in the `video` library (`lib video`).

#### Stack

```text
vpeek   ( addr bank -- c )
vpoke   ( addr bank c -- )
vram@   ( addr bank c-addr u -- )
vram!   ( addr bank c-addr u -- )
```

#### Description

The VERA's own memory, the video RAM: 128K, so an address is 17 bits, given as a cell and a *bank* of 0 or
1. `vpeek` leaves the byte there and `vpoke` stores one. `vram@` reads *u* bytes from it into memory at
*c-addr*, and `vram!` writes *u* bytes to it.

This is the chip's raw memory: the bitmap, the tiles and the sprites' pictures are all in it (Chapter 16
gives the layout).

#### See also

`palette!`, `sprite!`, `bitmap`.

## wait

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

```text
wait   ( task -- )
```

#### Description

Waits until a task that this HyForth started has ended: one from `spawn`, or from a command line that ended
in `&`. Its exit code is then in `status`.

#### See also

`spawn`, `status`.

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

## window, windows, new-window, new-group, show-window, window-label, window-status, window-ctl

The devices; in the `cons` library, a file of Forth in `/lib/forth` (`lib cons`).

#### Stack

```text
window          ( -- n )
windows         ( -- c-addr u )
show-window     ( n -- )
new-window      ( c-addr u -- )
new-group       ( c-addr u -- )
window-label    ( c-addr u -- )
window-status   ( c-addr u -- )
window-ctl      ( c-addr u -- )
```

#### Description

The console's windows (Chapter 10). `window` leaves the number of the window HyForth is in. `windows` leaves
the text that lists them all, a line each, with a `*` on the one that is shown. `show-window` shows window
*n*.

`new-window` makes a window, shows it, and runs the command line given in it; an empty string gives it a
shell. `new-group` does the same with the window in a note group of its own.

`window-label` sets this window's title (an empty string puts back its program's name), `window-status` the
text of its status line, and `window-ctl` writes any line to its control file.

#### See also

`send`, `form`, `k-resize`; the command `new-window`.

#### Example

```session
/> lib cons
/> window .
0
/> windows type
0 0 80 24 *
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

```text
wordlist   ( -- wid )
```

#### Description

Makes a new, empty word list, and leaves its identifier. A **word list** is a set of names of its own: the
same name can be in two word lists and mean two things. The system's words are all in one, the one
`forth-wordlist` gives.

A new word list is not searched, and takes no definitions, until a program says so: `set-current` makes it
the one that new words go into, and `set-order` (or `also` and a `set-order`) puts it in the search order.
The identifier is the only handle on it, so it is kept in a constant.

For the usual case, a named word list that holds a library's private words, `library` does all of this in
one step.

#### See also

`set-current`, `set-order`, `search-wordlist`, `library`, `order`.

#### Example

```session
/> lib search
/> wordlist constant mine
/> mine set-current
/> : secret  42 ;
/> definitions
/> s" secret" mine search-wordlist . execute .
-1 42
/> s" secret" forth-wordlist search-wordlist .
0
```

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
two/>
```

## >z, zcount

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

```text
>z       ( c-addr u -- z-addr )
zcount   ( z-addr -- c-addr u )
```

#### Description

The system's calls take a name as a **zero-terminated string**: its characters, and then a byte of 0. Forth's
strings are an address and a length. These two convert between them.

`>z` copies a string, puts a zero after it, and leaves the copy's address, ready for a `sys-` word. The copy
is in one of two buffers, used in turn, so two names can be held at once, for a call that takes two; a
third `>z` overwrites the first. A string can be 127 characters at most.

`zcount` goes the other way: given the address of a zero-terminated string, it leaves that address and the
number of characters before the zero.

#### See also

`sys-`, `count`, `s"`.

#### Example

```session
/> lib hydra
/> s" /rom/lib/shell" >z 0 sys-open throw value fd
/> fd sys-close .
0
/> s" hello" >z zcount type
hello
```

