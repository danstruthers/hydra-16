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
