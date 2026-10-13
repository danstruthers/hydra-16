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

## n>str

Numbers; in the `numbers` library (`lib numbers`).

#### Stack

( -- *c-addr u* ) N: ( *x* -- )

#### Description

Takes the number on top of the number stack and gives its text, in the base that is set, as `n.` would print
it but without the space after it. The text is in a transient buffer: use it or copy it before the next number
is converted.

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

## piped

The shell; in the `shell` library, which is loaded when HyForth starts as a shell.

#### Stack

( *xt c-addr u* -- )

#### Description

Runs a command line, given as a string, with what the word *xt* prints as its standard input. The word is run
with its output going into a pipe, the command reads the pipe, and `piped` waits for the command to end.
`' words s" grep file" piped` shows the words with `file` in their names.

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
