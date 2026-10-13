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
