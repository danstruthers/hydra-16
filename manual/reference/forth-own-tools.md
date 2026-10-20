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
3A62: 48 79 64 72 61 00 01 FF  Hydra...
3A6A: 10                       .
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
