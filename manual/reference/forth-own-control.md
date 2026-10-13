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
