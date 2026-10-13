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

Core word set; in the ROM core, always there. Immediate.

#### Stack

( "name" -- *xt* )

#### Description

A word's execution token.

## (

Core word set; in the ROM core, always there.

#### Stack

( "text)" -- )

#### Description

A comment, to `)`.

## */, */mod

Core word set; in the ROM core, always there.

#### Stack

( *n1 n2 n3* -- *n4* ), ( -- *rem quot* )

#### Description

*n1* × *n2* ÷ *n3*, with a 32-bit product in between.

## +, -, *

Core word set; in the ROM core, always there.

#### Stack

( *n1 n2* -- *n3* )

#### Description

Add, subtract, multiply.

## +!

Core word set; in the ROM core, always there.

#### Stack

( *n addr* -- )

#### Description

Add to a cell.

## ,, c,

Core word set; in the ROM core, always there.

#### Stack

( *x* -- ), ( *char* -- )

#### Description

Put a cell, a byte, there.

## ., u.

Core word set; in the ROM core, always there.

#### Stack

( *n* -- ), ( *u* -- )

#### Description

Print a number, and a space.

## .", "

Core word set; in the ROM core, always there.

#### Stack

( -- )

#### Description

Print the text.

## .(

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate.

#### Stack

( "text)" -- )

#### Description

Print the text now.

## /, mod, /mod

Core word set; in the ROM core, always there.

#### Stack

( *n1 n2* -- *n3* ), ( -- *rem* ), ( -- *rem quot* )

#### Description

Divide (rounding toward zero), the remainder, both.

## :

Core word set; in the ROM core, always there.

#### Stack

( "name" -- )

#### Description

Begin a definition.

## ;

Core word set; in the ROM core, always there. Immediate.

#### Stack

( -- )

#### Description

End it.

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

( ... -- *flag* )

#### Description

Not equal; more (unsigned); not zero; above zero.

## =, <, >, u<

Core word set; in the ROM core, always there.

#### Stack

( *x1 x2* -- *flag* )

#### Description

Equal; less, more (signed); less (unsigned)

## ?

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( *addr* -- )

#### Description

Print the cell at *addr*.

## @, !

Core word set; in the ROM core, always there.

#### Stack

( *addr* -- *x* ), ( *x addr* -- )

#### Description

Fetch, store a cell.

## [, ]

Core word set; in the ROM core, always there.

#### Stack

( -- )

#### Description

Stop compiling; start again.

## \

Core word set; in the ROM core, always there.

#### Stack

( "text" -- )

#### Description

A comment, to the end of the line.

## {:

Memory-Allocation, Locals and Block; in the `locals` library (`lib locals`).

#### Stack

` *vals* `--` *comment* `:}` (I C)

#### Description

Local variables, from the stack (`locals`)

## 0=, 0<

Core word set; in the ROM core, always there.

#### Stack

( *x* -- *flag* )

#### Description

Zero; below zero.

## 1+, 1-, 2*, 2/

Core word set; in the ROM core, always there.

#### Stack

( *n1* -- *n2* )

#### Description

Add 1, take 1, double, halve.

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

( *addr* -- *x1 x2* ), ( *x1 x2 addr* -- )

#### Description

Fetch, store two cells.

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

( *c-addr n1* -- *n2* )

#### Description

Read a line into a buffer.

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

( -- ), ( *addr* -- *addr* )

#### Description

(Nothing on the 65C02: any address is aligned)

## allocate

Memory-Allocation, Locals and Block; in the `memory` library (`lib memory`).

#### Stack

( *u* -- *addr ior* )

#### Description

A block of *u* bytes, from the heap (`memory`)

## allot

Core word set; in the ROM core, always there.

#### Stack

( *n* -- )

#### Description

Reserve *n* bytes there.

## and, or, xor

Core word set; in the ROM core, always there.

#### Stack

( *x1 x2* -- *x3* )

#### Description

Bit by bit.

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

( -- *addr* )

#### Description

The variable holding the base.

## beep

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Ring the bell.

## begin, until

Core word set; in the ROM core, always there.

#### Stack

( *flag* -- )

#### Description

Loop until true.

## begin-structure, end-structure

Facility; in the `facility` library (`lib facility`).

#### Stack

( "name" -- *addr* 0 ), ( *addr n* -- )

#### Description

A structure's start and end.

## bin

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *fam1* -- *fam2* )

#### Description

Bytes, as they are.

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

( -- *char* )

#### Description

A space's code (32)

## black, red, green, yellow, blue, magenta, cyan, white

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *c* )

#### Description

The colours 0 to 7.

## blank

String; in the `string` library (`lib string`).

#### Stack

( *c-addr u* -- )

#### Description

Fill with spaces.

## blk, scr

Memory-Allocation, Locals and Block; in the `block` library (`lib block`).

#### Stack

( -- *addr* )

#### Description

The block being run; the block listed.

## block, buffer

Memory-Allocation, Locals and Block; in the `block` library (`lib block`).

#### Stack

( *u* -- *addr* )

#### Description

Block *u*'s buffer, read in; a buffer for it (`block`)

## >body

Core word set; in the ROM core, always there.

#### Stack

( *xt* -- *addr* )

#### Description

A `create`d word's data.

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

( *u* "name" -- )

#### Description

A named buffer of *u* bytes.

## bye

Core word set; in the ROM core, always there.

#### Stack

( -- )

#### Description

Leave HyForth.

## c"

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( "text"" -- *c-addr* )

#### Description

A counted string.

## c@, c!

Core word set; in the ROM core, always there.

#### Stack

( *addr* -- *char* ), ( *char addr* -- )

#### Description

Fetch, store a byte.

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

( *i*x xt* -- *j*x* 0 | *i*x n* )

#### Description

Run *xt*; 0 if it worked, or the error it threw.

## cd

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( "dir" -- )

#### Description

Change directory.

## cells, cell+, chars, char+

Core word set; in the ROM core, always there.

#### Stack

( *n1* -- *n2* )

#### Description

Sizes: a cell is 2 bytes, a character 1.

## char, [char]

Core word set; in the ROM core, always there. Immediate.

#### Stack

( "name" -- *char* )

#### Description

The first character of the next word.

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

( *fileid* -- *ior* )

#### Description

Close.

## cmove, cmove>

String; in the `string` library (`lib string`).

#### Stack

( *c-addr1 c-addr2 u* -- )

#### Description

Copy bytes, from the start up; from the end down.

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

( *c-addr1 u1 c-addr2 u2* -- *n* )

#### Description

-1, 0 or 1, as they sort.

## compile,

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *xt* -- )

#### Description

Compile a word.

## [compile]

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate.

#### Stack

( "name" -- )

#### Description

Compile an immediate word.

## complex

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

N: ( *re im* -- *z* )

#### Description

A complex number.

## count

Core word set; in the ROM core, always there.

#### Stack

( *c-addr1* -- *c-addr2 u* )

#### Description

A counted string (its length first) to a string.

## cr, space, spaces

Core word set; in the ROM core, always there.

#### Stack

( -- ), ( -- ), ( *n* -- )

#### Description

A new line; a space; *n* spaces.

## create

Core word set; in the ROM core, always there.

#### Stack

( "name" -- )

#### Description

A word that gives the address of what follows.

## create-file

Files; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *c-addr u fam* -- *fileid ior* )

#### Description

Make a new file (or empty an old one), and open it.

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

( -- )

#### Description

Base 10.

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

( "name" -- *flag* )

#### Description

Is a word there?

## definitions

Search-Order; in the `search` library (`lib search`).

#### Stack

( -- )

#### Description

New words go in the first word list of the order.

## delete-file, rename-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *c-addr u* -- *ior* ), ( *c-addr1 u1 c-addr2 u2* -- *ior* )

#### Description

Remove; rename.

## depth

Core word set; in the ROM core, always there.

#### Stack

( -- *u* )

#### Description

How many cells are on the stack.

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

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *limit start* -- )

#### Description

`do`, but skipped if they're equal.

## do, loop, +loop

Core word set; in the ROM core, always there.

#### Stack

( *limit start* -- ), ( *n* -- )

#### Description

Counted loops.

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

( *x* -- )

#### Description

Drops the top.

## dump

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( *addr u* -- )

#### Description

Show memory, 8 bytes a line, in hex and as text.

## ?dup

Core word set; in the ROM core, always there.

#### Stack

( *x* -- 0 | *x x* )

#### Description

Copies the top if it isn't 0.

## dup

Core word set; in the ROM core, always there.

#### Stack

( *x* -- *x x* )

#### Description

Copies the top.

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

( *char* -- )

#### Description

Print a character.

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

( *c-addr u* -- false | *x true* )

#### Description

Ask about the system.

## erase

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *addr u* -- )

#### Description

Fill with zeros.

## evaluate

Core word set; in the ROM core, always there.

#### Stack

( *c-addr u* -- )

#### Description

Run a string as Forth.

## execute

Core word set; in the ROM core, always there.

#### Stack

( *xt* -- )

#### Description

Run a word.

## exit

Core word set; in the ROM core, always there. Compile only.

#### Stack

( -- )

#### Description

Return from the definition.

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

( *fileid* -- *ud ior* ), ( *ud fileid* -- *ior* )

#### Description

The place in the file.

## file-size, resize-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *fileid* -- *ud ior* ), ( *ud fileid* -- *ior* )

#### Description

Its length.

## file-status

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *c-addr u* -- *x ior* )

#### Description

Is it there?

## fill

Core word set; in the ROM core, always there.

#### Stack

( *c-addr u char* -- )

#### Description

Fill with a byte.

## find

Core word set; in the ROM core, always there.

#### Stack

( *c-addr* -- *c-addr* 0 | *xt* 1 | *xt* -1 )

#### Description

Look a word up.

## flush-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *fileid* -- *ior* )

#### Description

Write what's waiting.

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

Memory-Allocation, Locals and Block; in the `memory` library (`lib memory`).

#### Stack

( *addr* -- *ior* )

#### Description

Give one back.

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

( -- *addr* )

#### Description

The dictionary's next free byte.

## hex

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( -- )

#### Description

Base 16.

## holds

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *c-addr u* -- )

#### Description

Pictured numbers: put a string in.

## i, j

Core word set; in the ROM core, always there. Compile only.

#### Stack

( -- *n* )

#### Description

The loop's count; the outer loop's.

## if, else, then

Core word set; in the ROM core, always there.

#### Stack

( *flag* -- )

#### Description

Decisions.

## [if], [else], [then]

Programming-Tools; in the `tools` library, which is loaded as HyForth starts. Immediate.

#### Stack

( *flag* -- )

#### Description

Choose which lines to compile.

## immediate

Core word set; in the ROM core, always there.

#### Stack

( -- )

#### Description

Make the last definition immediate.

## include-file

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *fileid* -- )

#### Description

Run an open file as Forth.

## included, include

Core word set; in the ROM core, always there.

#### Stack

( *c-addr u* -- ), ( "file" -- )

#### Description

Run a file of Forth.

## int?, fixed?, rational?, complex?

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *flag* ) N: ( *x* -- )

#### Description

Its kind.

## invert

Core word set; in the ROM core, always there.

#### Stack

( *x1* -- *x2* )

#### Description

Every bit flipped.

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

( -- *char* )

#### Description

Wait for a key.

## key?

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- *flag* )

#### Description

Is a key waiting?

## leave, unloop

Core word set; in the ROM core, always there. Compile only.

#### Stack

( -- )

#### Description

Leave the loop; drop its count (before `exit`)

## -lib

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( "name" -- )

#### Description

Hide a library's words.

## lib

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( "name" -- )

#### Description

Load a library (*name*`.fl`, or *name*`.fs`), or show its words again.

## library

Search-Order; in the `search` library (`lib search`).

#### Stack

( "name" -- *wid* )

#### Description

Start a library of your own: a new word list, first in the order.

## libs

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( -- )

#### Description

List the libraries loaded.

## line, box, bar

The screen; in the `video` library (`lib video`).

#### Stack

( *x0 y0 x1 y1* -- )

#### Description

A line, a rectangle, a filled rectangle.

## list

Memory-Allocation, Locals and Block; in the `block` library (`lib block`).

#### Stack

( *u* -- )

#### Description

Show a block.

## literal

Core word set; in the ROM core, always there.

#### Stack

( *x* -- )

#### Description

Compile a number.

## load, thru

Memory-Allocation, Locals and Block; in the `block` library (`lib block`).

#### Stack

( *u* -- ), ( *u1 u2* -- )

#### Description

Run blocks.

## lshift, rshift

Core word set; in the ROM core, always there.

#### Stack

( *x1 u* -- *x2* )

#### Description

Shift the bits *u* places left, right.

## marker

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( "name" -- )

#### Description

*name* forgets everything defined after it.

## min, max

Core word set; in the ROM core, always there.

#### Stack

( *n1 n2* -- *n3* )

#### Description

The smaller; the larger.

## =mkdir

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr u mode* -- *ior* )

#### Description

Make a directory.

## move

Core word set; in the ROM core, always there.

#### Stack

( *addr1 addr2 u* -- )

#### Description

Copy *u* bytes.

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

( *i*x n* -- ) R: ( -- *i*x n* )

#### Description

Several cells to and from the return stack.

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

( *nt* -- ... )

#### Description

A word's name, and how it runs.

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

( *n1* -- *n2* )

#### Description

Change the sign; the size.

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

( *x1 x2* -- *x2* ), ( *x1 x2* -- *x2 x1 x2* )

#### Description

Drop the second; copy the top under the second.

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

( -- *xt* )

#### Description

A definition without a name.

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

Memory-Allocation, Locals and Block; in the `block` library (`lib block`).

#### Stack

( *c-addr u* -- )

#### Description

Use a file of blocks.

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

( *c-addr u fam* -- *fileid ior* )

#### Description

Open; make (or empty) and open.

## output-of

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *xt* -- *c-addr u* )

#### Description

What a word prints.

## over

Core word set; in the ROM core, always there.

#### Stack

( *x1 x2* -- *x1 x2 x1* )

#### Description

Copies the second.

## pad

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( -- *c-addr* )

#### Description

A scratch buffer.

## page

Facility; in the `facility` library (`lib facility`).

#### Stack

( -- )

#### Description

Clear the screen.

## parse, parse-name

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *char* "text" -- *c-addr u* ), ( "name" -- *c-addr u* )

#### Description

Take text from the input.

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

( *xu ... x0 u* -- *xu ... x0 xu* )

#### Description

Copy the *u*th cell to the top.

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

Core word set; in the ROM core, always there.

#### Stack

( "name" -- )

#### Description

Compile a word's compiling behaviour.

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

( *n w* -- ), ( *u w* -- )

#### Description

Print a number right-aligned in *w* characters.

## >r, r>, r@

Core word set; in the ROM core, always there. Compile only.

#### Stack

( *x* -- ) R: ( -- *x* ), ( -- *x* ) R: ( *x* -- ), ( -- *x* )

#### Description

To, from, a copy of, the return stack.

## r/o, w/o, r/w

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( -- *fam* )

#### Description

Read only, write only, both.

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

( *c-addr u1 fileid* -- *u2 ior* )

#### Description

Read bytes.

## read-line

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *c-addr u1 fileid* -- *u2 flag ior* )

#### Description

Read a line (*flag* false at the end)

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

## rename-file

Files; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *c-addr1 u1 c-addr2 u2* -- *ior* )

#### Description

Rename one.

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

Memory-Allocation, Locals and Block; in the `memory` library (`lib memory`).

#### Stack

( *addr1 u* -- *addr2 ior* )

#### Description

Change one's size.

## roll

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *xu ... x0 u* -- *xu-1 ... x0 xu* )

#### Description

Move the *u*th cell to the top.

## rot

Core word set; in the ROM core, always there.

#### Stack

( *x1 x2 x3* -- *x2 x3 x1* )

#### Description

Brings the third to the top.

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

( -- )

#### Description

Show the stack.

## s", "

Core word set; in the ROM core, always there. Immediate.

#### Stack

( -- *c-addr u* )

#### Description

A string.

## s>d

Core word set; in the ROM core, always there.

#### Stack

( *n* -- *d* )

#### Description

A cell to a double.

## s>n, d>n

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( *n* -- ) N: ( -- *x* ), ( *d* -- ) N: ( -- *x* )

#### Description

From Forth's stack.

## s\"

Core Extension; in the `coreext` library, which is loaded as HyForth starts. Immediate.

#### Stack

( "text"" -- *c-addr u* )

#### Description

A string with escapes: `\n`, `\t`, `\"` ...

## sbit, cbit

Bits, random numbers and the disassembler; in the `bits` library (`lib bits`).

#### Stack

( *n b* -- *n'* )

#### Description

Set, clear bit *b*.

## search

String; in the `string` library (`lib string`).

#### Stack

( *c-addr1 u1 c-addr2 u2* -- *c-addr3 u3 flag* )

#### Description

Find the second string in the first.

## search-wordlist

Search-Order; in the `search` library (`lib search`).

#### Stack

( *c-addr u wid* -- 0 | *xt* 1 | *xt* -1 )

#### Description

Look a word up in a word list.

## see

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( "name" -- )

#### Description

Show a word's definition.

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

String; in the `string` library (`lib string`).

#### Stack

( *c-addr u* -- )

#### Description

Compile a string.

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

( -- *addr* )

#### Description

True while compiling.

## status

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( -- *n* )

#### Description

The last command's status.

## /string

String; in the `string` library (`lib string`).

#### Stack

( *c-addr u n* -- *c-addr+n u-n* )

#### Description

Skip *n* characters.

## swap

Core word set; in the ROM core, always there.

#### Stack

( *x1 x2* -- *x2 x1* )

#### Description

Swaps the top two.

## synonym

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( "new" "old" -- )

#### Description

Another name for a word.

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
2
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

( *k*x n* -- *k*x* | *i*x n* )

#### Description

Throw error *n* (nothing if it's 0)

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

( *c-addr u1* -- *c-addr u2* )

#### Description

Without the spaces at the end.

## traverse-wordlist

Programming-Tools; in the `tools` library, which is loaded as HyForth starts.

#### Stack

( *i*x xt wid* -- *j*x* )

#### Description

Run *xt* for each word of a word list.

## true, false

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( -- *flag* )

#### Description

-1, 0.

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

( *c-addr u* -- )

#### Description

Print a string.

## um*, m*

Core word set; in the ROM core, always there.

#### Stack

( *u1 u2* -- *ud* ), ( *n1 n2* -- *d* )

#### Description

A double-cell product.

## um/mod, fm/mod, sm/rem

Core word set; in the ROM core, always there.

#### Stack

( *ud u1* -- *u2 u3* ), ( *d n1* -- *n2 n3* )

#### Description

Divide a double by a cell: unsigned, floored, symmetric.

## unused

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( -- *u* )

#### Description

The dictionary's free bytes.

## update, save-buffers, flush, empty-buffers

Memory-Allocation, Locals and Block; in the `block` library (`lib block`).

#### Stack

( -- )

#### Description

Mark changed; write; write and free; free.

## value, to

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *x* "name" -- ), ( *x* "name" -- )

#### Description

A value; change it.

## variable, constant

Core word set; in the ROM core, always there.

#### Stack

( "name" -- ), ( *x* "name" -- )

#### Description

A variable; a constant.

## wait

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *task* -- )

#### Description

Wait for a background task.

## within

Core Extension; in the `coreext` library, which is loaded as HyForth starts.

#### Stack

( *n lo hi* -- *flag* )

#### Description

*lo* ≤ *n* < *hi*.

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

( -- )

#### Description

List the words (the newest first), with their addresses and kinds.

## write-file, write-line

File Access; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *c-addr u fileid* -- *ior* )

#### Description

Write bytes; a line.

## write-line

Files; in the `file` library, which is loaded as HyForth starts.

#### Stack

( *c-addr u fileid* -- *ior* )

#### Description

Write a line, and a new line.

## >z, zcount

The Hydra; in the `hydra` library (`lib hydra`).

#### Stack

( *c-addr u* -- *z-addr* ), ( *z-addr* -- *c-addr u* )

#### Description

To and from a zero-ended string.

