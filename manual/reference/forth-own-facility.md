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
