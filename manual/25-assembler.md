# Chapter 25. The Assembler

An **assembler** turns assembly language into machine code. The Hydra's is called `as`. It reads a **source file**
(a text file of instructions, usually with a name ending in `.s`) and writes a **program**: a file that the Hydra can
run like any other command. Its language is the one of **ca65**, the assembler of the cc65 package that's used for
6502 programs on PCs, so programs (and books about ca65) work on both.

```text
as [-bl] file.s [out]
```

| Option | What it does |
|---|---|
| `-l` | Write a **labels file** too, *out*`.lbl`, for the debugger and the disassembler (section 25.17) |
| `-b` | Write only the bytes, without a program's header (section 25.17) |
| *out* | The program's name (without it: *file.s*'s name without its `.s`) |

## 25.1 Your first program

The SD card has some sample programs, with their sources, in `/sd/0/sample/as`. Copy `hi.s` into `/ram`, assemble it,
and run it:

```session
/> cd /ram
/ram> cp /sd/0/sample/as/hi.s .
/ram> as hi.s
/ram> ./hi Ann Bob
Hello, Ann!
Hello, Bob!
I'm task 4, in /ram.
/ram> ./hi
Hello, you!
I'm task 4, in /ram.
```

`as hi.s` said nothing, which means it worked: it made the program `hi`. Run with names, `hi` greets each one;
without, it greets you. Then it says which task it's running in, and its current directory (and its window, when it
has one). `./hi` runs the `hi` in this directory: just `hi` would look for it in `/bin`.

## 25.2 Inside hi.s

Here is `hi.s`. Don't worry if it looks strange: the rest of this chapter explains every part of it.

<!-- listing ../reborn/sdk/asm/samples/hi/hi.s -->

```text
; ****************************************************************************
; hi [name ...] - a sample RAM program (the assembly SDK's: sdk/asm/README.md).  It greets each name it's given (or
; you), then says where it runs: its task (GETPID), its current directory (GETCWD) and its window ($window, from its
; environment: ENV_GET).
;   % hi Ann Bob
;   Hello, Ann!
;   Hello, Bob!
;   I'm task 3, in /ram, in window 0.

.include "hydra.inc"                                        ; The calls and constants (made from spec/api.def)
.include "hyx2.inc"                                         ; The header: HYX2_PROGRAM
.include "macros.inc"                                       ; LDR, MOVR, PRINT, CALL, CHECK

            HYX2_PROGRAM "hi", main                         ; Its name, and where it starts

.zeropage
arg:        .res        2                                   ; An argument (its zero page: $22-$7F)

.bss
cwd:        .res        PATH_MAX + 1                        ; Its current directory
window:     .res        8                                   ;   and window

.code

; Its arguments at r0: strings one after another, each zero-terminated, an empty one after the last.  Returning
; ends it, with code 0 (EXITS)
main:
            MOVR        arg, r0
            lda         (arg)
            bne         @name
            LDR         arg, s_you                          ; None: you
@name:
            PRINT       "Hello, "                           ; "Hello, NAME!"
            MOVR        r0, arg
            CALL        PUTS
            PRINT       s_bang
:
            lda         (arg)                               ; The next: past this one's 0
            inc         arg
            bne         :+
            inc         arg + 1
:
            cmp         #0
            bne         :--
            lda         (arg)
            bne         @name
            PRINT       "I'm task "                         ; Its task: 0-15, in decimal
            CALL        GETPID
            cmp         #10
            bcc         :+
            pha
            lda         #'1'
            CALL        PUTC
            pla
            sec
            sbc         #10
:
            ora         #'0'
            CALL        PUTC
            PRINT       ", in "                             ; Its current directory
            LDR         r0, cwd
            CALL        GETCWD
            PRINT       cwd
            LDR         r0, s_window                        ; Its window, if its environment has one
            LDR         r1, window
            LDR         r2, 7
            stz         r3
            stz         r3 + 1
            lda         #$FF                                ; (This task's)
            CALL        ENV_GET
            CHECK       @end                                ; (Not there: C = 1, .A = E_NOENT)
            tax
            stz         window,X                            ; (Its value: zero-terminated)
            PRINT       ", in window "
            PRINT       window
@end:
            PRINT       s_end
            rts

.rodata
s_you:      .byte       "you", 0, 0
s_bang:     .byte       "!", LF, 0
s_window:   .byte       "window", 0
s_end:      .byte       ".", LF, 0
```

Look at it in pieces:

* A `;` starts a **comment**: everything after it on the line is for people, and `as` skips it.
* `.include "hydra.inc"` reads in another file of source: `hydra.inc` gives the names of HydraOS's system calls and
  constants, such as `PUTS` and `PATH_MAX`; `hyx2.inc` has `HYX2_PROGRAM`; and `macros.inc` has the **macros**
  `LDR`, `MOVR`, `PRINT`, `CALL` and `CHECK` (section 25.11). They're all in `/lib/as`.
* `HYX2_PROGRAM "hi", main` makes the program's **header**: its name is `hi`, and it starts at `main`.
* `.zeropage`, `.bss`, `.code` and `.rodata` say which **segment** the lines after them go into (section 25.8): the
  zero page variable `arg`, the BSS's buffers `cwd` and `window`, the code, and the read-only data (the strings).
* `main:` and `@name:` are **labels**: names for places in the program (section 25.5). A label alone on a line, `:`,
  has no name: `:+` means the next one, and `:--` the one two back.
* A program starts with its arguments in r0, and ends when `main` returns.
* `CALL PUTS` (the same as `jsr PUTS`) makes a **system call**: it asks HydraOS to do something, here to print the
  string at r0 (section 25.13).

## 25.3 A program of your own

Now write one. Start `edit stars.s` in `/ram`, and type this in:

<!-- listing samples/stars.s -->

```text
; stars - a triangle of stars
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "stars", main

ROWS        = 5                 ; How many rows

.zeropage
row:        .res 1              ; The row it's on: 1 to ROWS
count:      .res 1              ; The stars still to print on it

.code
main:       lda #1              ; The first row
            sta row
@row:       lda row             ; As many stars as the row's number
            jsr line
            inc row             ; The next row, till the last is done
            lda row
            cmp #ROWS + 1
            bne @row
            rts

; A line of stars: .A of them (1 or more)
line:       sta count
@star:      lda #'*'
            jsr PUTC
            dec count
            bne @star
            lda #LF             ; The line's end
            jsr PUTC
            rts
```

Save it (<kbd>CTRL</kbd>-<kbd>S</kbd>), leave `edit` (<kbd>CTRL</kbd>-<kbd>X</kbd>), and assemble and run it:

<!-- session
cd /ram
!file samples/stars.s
-->

```session
/ram> as stars.s
/ram> ./stars
*
**
***
****
*****
```

How it works:

* `ROWS = 5` makes a **constant**: from then on, `ROWS` means 5. Change it to 10, assemble again, and the triangle
  has ten rows.
* `row` and `count` are variables on the zero page: `.res 1` **reserves** a byte for each.
* `main` puts 1 in `row`, then goes round a loop: it calls the subroutine `line` with the row's number in A, adds 1
  to `row`, and goes round again (`bne @row`) until `row` is `ROWS + 1`.
* `line` prints A stars. It keeps the count in `count`, not in X, because a system call (here `PUTC`, which prints
  the character in A) may change A, X and Y. `dec count` takes 1 from it and sets Z when it reaches 0, so `bne @star`
  goes round again until it does.
* `LF` (from `macros.inc`) is the code of a new line, 10.

If you make a mistake, `as` tells you where (section 25.18). Fix it in `edit`, and assemble again.

## 25.4 What's on a line

Each line of a source can have, in this order, any of:

1. a **label**, ending with a colon: `main:`
2. a **statement**: an instruction (`lda #1`), a **directive** (`.byte 1, 2, 3`, starting with a dot), the use of a
   macro (`PRINT "Hello"`), or a constant (`ROWS = 5`)
3. a **comment**, from a `;` to the end of the line.

Spaces and tabs separate the parts; how many doesn't matter, so line your columns up as you like. Instruction names
and directives can be written in capitals or small letters (`LDA`, `lda`). Names are different: `Count` and `count`
are two different names. A line can be 255 characters long.

Instructions are written as in section 24.6: `lda #$41`, `sta $22,x`, `lda (r0),y`, `jmp ($1234,x)`, `asl a`. When
an address is under $100, `as` uses the shorter zero page form. To force the 3-byte form, write `a:` before it
(`lda a:$12`); to insist on the zero page form, `z:`.

## 25.5 Labels

A label names the place it's at. There are three kinds:

| Kind | Written | Its name is seen |
|---|---|---|
| A **label** | `main:` | Everywhere in the program |
| A **cheap local** label | `@loop:` | Only between the two normal labels around it |
| An **unnamed** label | `:` | Through `:+` (the next one), `:++` (the one after that), `:-` (the last one), `:--` ... |

Cheap locals let every routine have its own `@loop` and `@done`, without inventing a new name each time. Unnamed
labels are for little hops of a line or two:

```text
            lda count
            bne :+              ; (Not zero: past the inx)
            inx
:           rts
```

A name is letters, digits and `_` (and a cheap local's `@` in front), starting with a letter or `_`, and up to 32
characters long.

## 25.6 Numbers and expressions

Numbers can be written in four ways:

| Written | Means |
|---|---|
| `65` | Decimal |
| `$41` | Hexadecimal |
| `%01000001` | Binary |
| `'A'` | A character's code: 65 |

Wherever a number goes, an **expression** can go instead: numbers, names and operators. `as` works them out in 32
bits, as signed numbers. `*` alone means the address the statement is at.

| Operator | Means | Example |
|---|---|---|
| `+` `-` `*` `/` | Add, subtract, multiply, divide (whole numbers) | `ROWS + 1` |
| `.mod` | The remainder | `100 .mod 7` is 2 |
| `<` (before) | The low byte | `<$1234` is $34 |
| `>` (before) | The high byte | `>$1234` is $12 |
| `^` (before) | The bank byte (bits 16-23) | |
| `&` `\|` `^` | AND, OR, XOR of the bits | `$F0 & $3C` is $30 |
| `~` | NOT of the bits | |
| `<<` `>>` | Shift left, right | `1 << 4` is 16 |
| `=` `<>` `<` `>` `<=` `>=` | Compare: 1 if it's so, 0 if not | `2 < 3` is 1 |
| `&&` `\|\|` `!` | And, or, not (of true and false) | |
| `( )` | Brackets | `(ROWS + 1) * 2` |

The operators are worked out in this order (highest first): the ones before a number (`-`, `~`, `<`, `>`, `^`, `!`);
then `*`, `/`, `.mod`, `&`, `^`, `<<`, `>>`; then `+`, `-`, `|`; then the comparisons; then `&&` and `.xor`; and last
`||`. The words `.bitand`, `.bitor`, `.bitxor`, `.bitnot`, `.shl`, `.shr`, `.and`, `.or` and `.not` can be used
instead of the signs.

**Functions** work on what's in brackets after them:

| Function | Gives |
|---|---|
| `.lobyte(x)`, `.hibyte(x)`, `.bankbyte(x)` | Byte 0, 1 or 2 of *x* |
| `.loword(x)`, `.hiword(x)` | The low or high 16 bits of *x* |
| `.strlen("text")` | A string's length |
| `.defined(name)`, `.def(name)` | 1 if *name* has been defined, else 0 |
| `.blank(x)` | 1 if nothing's in the brackets (for macros) |
| `.match(a, b)` | 1 if *a* and *b* are the same kinds of thing (for macros) |

## 25.7 Constants

`name = expression` makes a **constant**: a name for a number. `name := expression` is the same. Constants make
programs easier to read and to change:

```text
ROWS        = 5                 ; How many rows
SCREEN_W    = 80
MIDDLE      = SCREEN_W / 2
```

A constant isn't stored in the program: `as` puts its value wherever its name is used. `hydra.inc` is mostly
constants: `PUTC = $F950` is the address of the `PUTC` system call.

## 25.8 Segments: where everything goes

A program has several parts, called **segments**. `as` keeps each one's lines together, wherever they are in the
source, and puts the segments in order when it writes the program:

| Segment | Directive | What it's for | Where it goes |
|---|---|---|---|
| `ZEROPAGE` | `.zeropage` | Variables on the zero page (`.res` only) | From $22, up to $7F |
| `HEADER` | (`HYX2_PROGRAM`) | The program's header | From $0800 (48 bytes) |
| `CODE` | `.code` | Instructions | After the header |
| `RODATA` | `.rodata` | Data that doesn't change: strings, tables | After the code |
| `DATA` | `.data` | Data that starts with a value but may change | After the read-only data |
| `BSS` | `.bss` | Variables that start as 0 (`.res` only) | After the data: not in the file |

A source starts in `CODE`. `.segment "NAME"` is the long way to switch, and `.pushseg` and `.popseg` switch to
another segment for a while and back. The BSS isn't kept in the program's file: HydraOS clears it to zeros as it
loads the program, so a program with a big buffer is still a small file. (`hi`'s file is 326 bytes; its BSS, 72
bytes more, isn't in it.)

`as` also defines the names a linker would, for programs that want to know where their parts went: `__DATA_LOAD__`,
`__DATA_RUN__` and `__DATA_SIZE__` (the data's place and size), `__BSS_RUN__` and `__BSS_SIZE__`, `__RAM_START__`
($0800), `__RAM_SIZE__` and `__RAM_LAST__` (the end of the program's memory), `__ZP_START__`, `__ZP_SIZE__` and
`__ZP_LAST__`. `HYX2_RAM` is defined too, which tells `hyx2.inc` that this is a program in a file.

A program can be 30K at most: from $0800 to $7FFF, its BSS included. It has 94 bytes of zero page ($22 to $7F).

## 25.9 Data

| Directive | What it puts in the program |
|---|---|
| `.byte` *x*, ... (or `.byt`) | Bytes: numbers from -128 to 255, and strings (`"text"`), a byte for each character |
| `.asciiz "text"`, ... | Strings, each with a 0 after it |
| `.word` *x*, ... (or `.addr`) | 16-bit numbers, two bytes each, low byte first |
| `.dword` *x*, ... | 32-bit numbers, four bytes each, lowest first |
| `.res` *count* [, *fill*] | *count* bytes, all *fill* (0 if it's not given) |
| `.incbin "file"` [, *start* [, *count*]] | A file's bytes (from *start*, *count* of them) |

```text
.rodata
greeting:   .byte "Hello!", LF, 0   ; A string, a new line, and the 0 that ends it
squares:    .byte 0, 1, 4, 9, 16, 25
jumps:      .word main, line        ; Two addresses
.bss
buffer:     .res 256                ; 256 bytes, cleared as it starts
```

A string in HydraOS ends with a 0 byte: that's how `PUTS` knows where it stops. A string can't have a `"` in it;
use its code, `34`, in a `.byte` instead.

## 25.10 Including files

`.include "file"` reads another source file in, as if its lines were in this one. `as` looks for the file as it's
named, then in the directory of the file that includes it, and then in `/lib/as`, which has the **SDK**'s files (the
software development kit's: the files for writing programs):

| File | What it has |
|---|---|
| `hydra.inc` | Every system call's address, every error code, and HydraOS's constants (Appendix K) |
| `hyx2.inc` | The header macros: `HYX2_PROGRAM` for a program, `HYX2_DRIVER` for a driver, `HYX2_LIBRARY` |
| `macros.inc` | `LDR`, `MOVR`, `PRINT`, `CALL`, `CHECK`; and `CR`, `LF`, `TAB` |
| `toollib.inc`, `toollib.s` | The tools' library: flags, files, output (section 25.16) |
| `numbers.inc`, `numlib.s` | The numbers and math libraries (section 25.16) |
| `asmlib.inc` | The assembler library itself, for programs that assemble or disassemble |
| `srvlib.inc`, `srvlib.s` | For writing a **server**: a driver with files of its own |
| `nslib.s` | Building a namespace from a namespace file, as init does |

A `.s` file of the SDK is included at the **end** of a program's source (its code is put after the program's own),
and its `.inc` at the top.

## 25.11 Macros

A **macro** is a piece of source with a name. Using the name puts the piece in, as if you'd typed it. Macros can
have **parameters**: names that are replaced by what's written after the macro's name when it's used.

<!-- listing samples/mac.s -->

```text
; mac - a macro of its own
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "mac", main

; SAY char, times: the character, times times
.macro SAY char, times
            ldx #times
:           lda #char
            phx                 ; (PUTC may change .X: kept on the stack)
            jsr PUTC
            plx
            dex
            bne :-
.endmacro

.code
main:       SAY 'H', 1
            SAY 'o', 5
            SAY '!', 3
            SAY LF, 1
            rts
```

<!-- session
cd /ram
!file samples/mac.s
-->

```session
/ram> as mac.s
/ram> ./mac
Hooooo!!!
```

`.macro NAME param, ...` starts one (up to 9 parameters), and `.endmacro` ends it. `.exitmacro` leaves it early (in
an `.if`, section 25.12). An unnamed label inside a macro is fine, as here; but a normal label would be defined twice
if the macro were used twice.

The SDK's macros, in `macros.inc`, make the commonest jobs shorter:

| Macro | What it does | The same as |
|---|---|---|
| `LDR r, value` | Put a 16-bit value (an address, often) in a call register | `lda #<value`, `sta r`, `lda #>value`, `sta r+1` |
| `MOVR to, from` | Copy one zero page word (a call register) to another | Two `lda`s and two `sta`s |
| `PRINT label` | Print the string at *label* | `LDR r0, label` and `jsr PUTS` |
| `PRINT "text"` | Print the text (kept in the code, with a jump over it) | |
| `CALL name` | A system call | `jsr name` |
| `CHECK label` | After a call: if it failed (C = 1), go to *label* | `bcc` past a `jmp label` |

Note that `LDR`, `MOVR` and `PRINT` change A (and `PRINT` changes Y and r0 too).

## 25.12 Conditions and checks

**Conditional assembly** assembles some lines only if something's so:

```text
.if ROWS > 9
            .warning "a big triangle"
.elseif ROWS = 0
            .error "no rows"
.else
            ...
.endif
```

| Directive | Assembles the lines after it if |
|---|---|
| `.if` *expr* | *expr* isn't 0 |
| `.ifdef` *name*, `.ifndef` *name* | *name* is defined; isn't |
| `.ifblank` ..., `.ifnblank` ... | Nothing follows it on the line; something does (for a macro's parameters) |
| `.elseif` *expr* | The ones before weren't, and *expr* isn't 0 |
| `.else` | The ones before weren't |
| `.endif` | (The end) |

`.if`s can be inside each other, 16 deep. Three directives check things:

| Directive | What it does |
|---|---|
| `.assert` *expr*, `error`, "message" | Says the message as an error if *expr* is 0 (or `warning`, as a warning) |
| `.error "message"` | Says the message as an error: `as` makes no program |
| `.warning "message"` | Says it as a warning: `as` goes on |

`hyx2.inc` uses `.assert` to check that a program's name is short enough. `as` also takes, and ignores, ca65's
directives that don't matter here (`.setcpu`, `.import`, `.export`, `.global`, `.macpack`, `.feature`, `.list`,
`.case`, `.smart` and others), so ca65's sources assemble.

## 25.13 System calls

A **system call** asks HydraOS to do something: print, open a file, start a program, wait. Each has an address in
HydraOS's **jump table**, from $F800 up, and a program calls it with `jsr`. The rules are the same for all of them:

* The inputs go in A, X, Y and the call registers r0 to r15 (each 2 bytes, low byte first, at $02 to $21).
* The answers come back in the same places. A 16-bit answer comes back in A (its low byte) and X (its high byte).
* **C = 0** means it worked. **C = 1** means it failed, and A holds the **error code** (Appendix G).
* A call may change A, X, Y, r0 to r15 and the flags. It never changes $22 to $7F: the program's zero page is safe.

Appendix K lists every call. The ones a first program needs:

| Call | Inputs | What it does |
|---|---|---|
| `PUTC` | A = a character | Write it to the standard output (the screen) |
| `PUTS` | r0 = a string | Write it (up to its 0) |
| `GETC` | | Read a byte from the standard input (a key): A = it |
| `OPEN` | r0 = a file's name; A = `O_READ`, `O_WRITE` or `O_RDWR` | Open a file: A = its **fd** (file descriptor) |
| `READ` | A = an fd; r0 = a buffer; r1 = how many bytes | Read: A/X = how many came (0 at the end) |
| `WRITE` | A = an fd; r0 = the bytes; r1 = how many | Write them |
| `CLOSE` | A = an fd | Close it |
| `ERRSTR` | A = an error code; r0 = a 32-byte buffer | The error's message, in the buffer |
| `SLEEP` | A/X = how many ticks (200 a second) | Wait |
| `EXITS` | A = the exit code; r0 = a message, or 0 | End the program |

Every program has three fds open as it starts: 0, the **standard input** (the keyboard, usually); 1, the **standard
output** (the screen); and 2, the **standard error** (for error messages, usually the screen too).

**Arguments.** A program starts at its `main` with r0 pointing at its arguments: each one a string ending in 0, one
after the other, with an empty string (a lone 0) after the last. `hi.s` shows how to walk along them.

**Ending.** When `main` returns (`rts`), the program ends with exit code 0, which means success. To end with
another code, and a message, put them in A and r0 and `jmp EXITS`. rc's `$status` shows the message (section 7.9).

## 25.14 Reading a file

`show.s` is a little `cat`: it prints the file it's given. It opens the file, then reads 256 bytes at a time and
writes them to fd 1 until a read gives 0 bytes, which means the end. If any call fails, it prints the error's
message and ends with code 1.

<!-- listing samples/show.s -->

```text
; show file - a file's text, on the screen
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "show", main

.zeropage
fd:         .res 1              ; The file, open

.bss
buf:        .res 256            ; A piece of it
msg:        .res 32             ; An error's text

.code
main:       lda (r0)            ; The file's name: r0's first argument
            bne @open
            PRINT "usage: show file"
            lda #LF
            jsr PUTC
            rts
@open:      lda #O_READ         ; Open it, to read
            jsr OPEN
            bcs @fail           ; (C = 1: it failed, .A the error)
            sta fd
@read:      LDR r0, buf         ; 256 bytes of it at most
            LDR r1, 256
            lda fd
            jsr READ
            bcs @fail
            sta r1              ; The count read: .A (low), .X (high)
            stx r1 + 1
            ora r1 + 1
            beq @done           ; (0: the file's end)
            LDR r0, buf         ; Those bytes, to fd 1: the screen
            lda #1
            jsr WRITE
            bcs @fail
            bra @read
@done:      lda fd
            jsr CLOSE
            rts
@fail:      pha                 ; The error's text (LDR changes .A: kept)
            LDR r0, msg
            pla
            jsr ERRSTR
            PRINT "show: "
            PRINT msg
            lda #LF
            jsr PUTC
            lda #1              ; The end, with code 1
            stz r0
            stz r0 + 1
            jmp EXITS
```

<!-- session
cd /ram
!file samples/show.s
-->

```session
/ram> as show.s
/ram> echo Roses are red >poem
/ram> echo Violets are blue >>poem
/ram> ./show poem
Roses are red
Violets are blue
/ram> ./show nothing
show: not found
/ram> ./show
usage: show file
```

Look at the start of `@fail`. `LDR` uses A, so the error code in A would be lost: `pha` keeps it on the stack, and
`pla` brings it back for `ERRSTR`. Mistakes like this are easy to make, and the debugger (Chapter 27) is how you find
them.

## 25.15 More memory: RAM banks

A program's 30K isn't much. But each task also has its own **RAM banks**, 8K each: 16 on each RAM module of the
memory card (48 on the book's Hydra). A program asks HydraOS for banks with `BANKS_ALLOC`, and writes a bank's number
to address $00 to put it at $8000-$9FFF. `BANKS_FREE` gives banks back (HydraOS takes them back anyway when the
program ends).

<!-- listing samples/bank.s -->

```text
; bank - a RAM bank of its own
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "bank", main

.zeropage
mine:       .res 1              ; The bank it's given

.code
main:       lda #1              ; One bank, please
            jsr BANKS_ALLOC
            bcs @none           ; (C = 1: none free)
            sta mine
            sta $00             ; It's at $8000-$9FFF now
            ldx #0              ; The message copied into it ...
:           lda message,x
            sta $8000,x
            beq :+              ;   (its 0 too)
            inx
            bra :-
:           LDR r0, $8000       ;   and printed from there
            jsr PUTS
            lda mine            ; The bank given back
            ldx #1
            jsr BANKS_FREE
            rts
@none:      PRINT "no bank free"
            rts

.rodata
message:    .byte "Hello from a RAM bank!", LF, 0
```

<!-- session
cd /ram
!file samples/bank.s
-->

```session
/ram> as bank.s
/ram> ./bank
Hello from a RAM bank!
```

Only one bank is at $8000 at a time; to use another, write its number to $00. The `BANKS` call says how many banks the
task has. For more memory in the program's own 32K, `BREAK` moves the program's **break** (the end of its memory,
after its BSS) further up.

## 25.16 The SDK's libraries

The SDK has libraries that save writing everything yourself. Each sample on the SD card shows one:

**The tools' library.** `toollib.s` does what every command needs: its options (`-x`), its files (or the standard
input, with none), its output (written 256 bytes at a time, which is much quicker than `PUTC`), and its error
messages, in the same form as the Hydra's own tools. The `upper` sample (`/sd/0/sample/as/upper.s`) is a whole
tool in a few lines: it prints its files in capitals.

| Routine | What it does |
|---|---|
| `tl_start` | Reads the options (the letters in `tl_flagset`) into `tl_flags`; `tl_arg` points at the first argument after them |
| `tl_next` | On to the next argument (A = its first byte: 0 if there's none) |
| `tl_eachin` | For each file named (or the standard input), calls the routine at `tl_ivec`, which reads it with `tl_getc` |
| `tl_getc` | The next byte of the file in A (C = 1 at its end) |
| `tl_putc`, `tl_puts`, `tl_nl`, `tl_space` | Output: a character, the string at r0, a new line, a space |
| `tl_dec` | Print the 4-byte number in `tl_num` in decimal |
| `tl_err`, `tl_warn` | Say an error about a name, or a message: "*tool*: ..." on fd 2, and set the exit code to 1 |
| `tl_end` | Write the output, and end, with the exit code |
| `tl_badusage` | Say "usage: ..." (`tl_usage`), and end |

A tool defines `tl_name`, `tl_flagset` and `tl_usage`, includes `toollib.inc` at its top and `toollib.s` at its end.

**The numbers libraries.** `numbers.inc` and `numlib.s` give assembly programs Chapter 17's numbers: integers of any
size, fractions, decimals and complex numbers, read and written in any base. The `nsum` sample adds up the numbers it's
given, and gives the square root too:

```session
/> /sd/0/sample/nsum 1/3 0.5 2
17/6
sqrt 1.68325082306
```

**Notes.** A **note** is a message to a task from HydraOS or another task: <kbd>CTRL</kbd>-<kbd>C</kbd> sends the
`interrupt` note to the programs in its window. Most notes end a program, but a program can catch them with a
**note handler**, set with the `NOTIFY` call. The `tick` sample prints a dot a second until you press
<kbd>CTRL</kbd>-<kbd>C</kbd>, and then, instead of just stopping, says how many seconds it ran.

## 25.17 as's options and the labels file

**The program's name.** `as stars.s` makes `stars`; `as stars.s /bin/stars` puts it straight in `/bin` (in fact in
`/ram/bin`, where new files in `/bin` go: section 8.7), so the shell finds it as plain `stars`, from any directory. A
source whose name doesn't end with `.s` needs the output's name given.

**-l: the labels file.** `as -l stars.s` also writes `stars.lbl`, listing the address of every label:

<!-- session
cd /ram
!file samples/stars.s
-->

```session
/ram> as -l stars.s
/ram> cat stars.lbl
al 000023 .count
al 000830 .main
al 000022 .row
al 000842 .line
```

Each line is `al`, the address in hex, and the label's name after a dot. (It's the form the ld65 linker writes with
its `-Ln` option, so the same tools read both.) The debugger and the disassembler read it to show names instead of
addresses. Cheap local labels aren't in it.

**-b: bytes alone.** `as -b` writes only the machine code, with no header, as a **raw** file. Its code starts at
the address given by `.org` (or $0800 if there's no `.org`), which must come before anything else that makes bytes.
A raw file isn't a program you can run by name: it's machine code for something else to load, such as a ROM, or a
routine that another program reads in.

<!-- listing samples/raw.s -->

```text
; A routine on its own, at $1000: no header (as -b)
.org $1000
double:     asl a               ; .A times 2 ...
            bcc :+
            inx                 ;   (its carry into .X)
:           rts
```

<!-- session
cd /ram
!file samples/raw.s
-->

```session
/ram> as -b raw.s
/ram> xd raw
0000000  0a 90 01 e8 60                                   ....`
```

The five bytes are `asl a` ($0A), `bcc` ($90) and its distance (1: past one byte), `inx` ($E8) and `rts` ($60).

## 25.18 When as finds a mistake

`as` says each mistake as *file*`:`*line*`:` *what*, and makes no program:

<!-- listing samples/bad.s -->

```text
; bad - a program with mistakes in it, to see what as says
.include "hydra.inc"
.include "hyx2.inc"

            HYX2_PROGRAM "bad", main

.code
main:       lda #300            ; A byte is 0 to 255
            ldx #10
@loop:      jsr PUTCH           ; (PUTC, spelt wrong)
            dex
            bne @loop
            rts
```

<!-- session
cd /ram
!file samples/bad.s
-->

```session
/ram> as bad.s
as: bad.s:10: undefined: PUTCH
```

`PUTCH` should be `PUTC`. Fix it and assemble again, and `as` finds the next mistake:

<!-- session
cd /ram
!file samples/bad4.s bad.s
-->

```session
/ram> as bad.s
as: bad.s:8: not a byte
```

Line 8 is `lda #300`: a byte only goes up to 255. `as` reads a source three times (section 25.19), and it stops
after the first reading that finds a mistake, so a mistake can hide another until it's fixed. The messages you'll
see most:

| Message | What's wrong |
|---|---|
| `undefined: NAME` | A name that isn't defined anywhere: often a spelling mistake, or a missing `.include` |
| `not an instruction, directive or macro: NAME` | A misspelt instruction, or a line that doesn't start where you think |
| `not a byte` | A number that should be a byte (from -128 to 255) isn't |
| `an addressing mode the instruction doesn't have` | Like `stz ($22),y`: the 65C02 has no such instruction (Appendix D) |
| `a bad operand` | What follows the instruction isn't an address, a number or a mode it knows |
| `a branch too far` | A branch to more than 128 bytes away: use `jmp` (or a branch over a `jmp`) |
| `defined twice` | Two labels with the same name |
| `not found: FILE` | An `.include` (or the source) that isn't there |
| `a string with no closing quote` | A `"` missing |
| `more on the line than it takes` | Something extra at the end of a line |
| `an .if with no .endif`, `a .macro with no .endmacro` | A block not closed |
| `data in BSS or the zero page` | Only `.res` can go in those segments |
| `too big: past $8000` | The program (with its BSS) is bigger than 30K |
| `the zero page's $5E bytes overrun` | More zero page variables than fit in $22 to $7F |
| `a segment's size changed between passes` | A size that depends on a name defined later (section 25.19) |
| `an assertion failed` | An `.assert` (`HYX2_PROGRAM`'s, say: a name longer than 11 characters) |

## 25.19 How as works

`as` reads the source three times, called **passes**:

1. The first pass finds how big each segment is. It can't know yet where every label will be.
2. The second finds every label's and constant's value.
3. The third makes the bytes.

Why three? Because a program can use a name before it's defined: `bne @star` where `@star` comes later, or
`jsr line` before `line`. The first pass can't know where `line` is, so it leaves room for it; the second fills in the
places; the third has everything.

There's one catch. An instruction with a zero page address is a byte shorter than one with an absolute address, so
`as` must decide which to use the same way in every pass. It uses the zero page form only for an address it already
knows (from earlier in the source) is under $100. So define zero page variables at the top of the program, before
the code that uses them, as the samples do; one defined later is reached with the longer form, which works but is
slower.

`as` keeps its symbols, its macros, the files it reads and the program it makes in RAM banks, so a big source fits.
It has some limits: lines of 255 characters, names of 32, files and macros 16 inside each other, `.if`s 16 deep,
9 parameters to a macro, and 30 errors (then it stops).

`as` is a small program; the assembler itself is the **asm library**, a module in the ROM. HyBASIC uses the same
library for its `ASM` blocks (section 25.20), and the disassembler and debugger use its other half to show
instructions, so all of them write instructions exactly the same way.

## 25.20 Assembly language in HyBASIC

HyBASIC can have assembly language inside a program. The lines between `ASM` and `END ASM` (each alone on its
line) are given to the assembler when the program runs, and `CALL ASM label` calls the code at one of their labels:

```session
> 10 REM Stars, from an ASM block
> 20 ASM
> 30 .include "hydra.inc"
> 40 stars:  lda #'*'          ; X stars, then a new line
> 50         phx
> 60         jsr PUTC
> 70         plx
> 80         dex
> 90         bne stars
> 100        lda #10
> 110        jsr PUTC
> 120        rts
> 130 twice:  lda n + 1         ; N's low byte, times 2
> 140         asl a
> 150         rts
> 160 END ASM
> 170 FOR N = 1 TO 5
> 180   CALL ASM stars, 0, N
> 190 NEXT
> 200 N = 21
> 210 CALL ASM twice
> 220 RREG A, X, Y, P
> 230 PRINT "twice"; N; "is"; A
> RUN
*
**
***
****
*****
twice 21 is 42
```

* `CALL ASM label, a, x, y` sets A, X and Y (all optional) before calling. Here X is the number of stars.
* `RREG a, x, y, p` copies the registers, as they were when the code returned, into four variables.
* An `ASM` block can use the program's **variables**: each number variable's name (in capitals or in small letters)
  is the address of its value (the main program's variables, not a `SUB`'s own), and each integer `CONST`'s name is
  its value. A whole number's value is 5 bytes: a 0, then the number, 4 bytes, low byte first. So
  `n + 1` is the low byte of `N`.
* `.include "hydra.inc"` gives the system calls' names, as in a program.
* All of a program's `ASM` blocks are assembled together into 8K of their own, a RAM bank, at $8000. A mistake in one
  is said as a HyBASIC error, at its line.

`SYS address [, a [, x [, y]]]` calls machine code anywhere in memory, as `CALL ASM` does.

## 25.21 Machine code from HyForth

HyForth's `sys` ( *addr a x y* -- *a x y p* ) calls machine code at *addr* with A, X and Y set, and gives back the
registers and the flags after it (`lib hydra` has it). The machine code can be anywhere: here it's made with `c,`
right in the dictionary, a byte at a time, and `disasm` (Chapter 26) shows it:

```session
/> lib hydra
/> hex
/> create hi  a9 c, 48 c, 20 c, 50 c, f9 c,
/> a9 c, 69 c, 20 c, 50 c, f9 c, 60 c,
/> decimal
/> hi 0 0 0 sys . . . .
Hi48 0 0 105
/> lib disasm
/> hi 5 disasm
 463B  A9 48     lda #$48
 463D  20 50 F9  jsr $F950
 4640  A9 69     lda #$69
 4642  20 50 F9  jsr $F950
 4645  60        rts
```

The bytes are `lda #'H'`, `jsr PUTC`, `lda #'i'`, `jsr PUTC` and `rts`. After it ran, `.` printed the flags (48), Y,
X and A (105, the code of `i`, which it was left holding).

A file made with `as -b` can be read into memory with HyForth's file words, and called the same way. Here is
`raw.s`'s routine from section 25.17, which doubles A (its carry going into X), given 200:

<!-- session
cd /ram
!file samples/raw.s
as -b raw.s
-->

```session
/ram> lib hydra
/ram> create code 16 allot
/ram> s" /ram/raw" r/o open-file throw value fd
/ram> code 16 fd read-file throw .
5
/ram> fd close-file throw
/ram> code 200 0 0 sys . . . .
49 0 1 144
```

Twice 200 is 400, which is $190: A is $90 (144), and X is 1. The flags, 49, have C set: the carry out of the top.
The routine only uses a relative branch, so it works wherever it's loaded; code with `jmp`s or `jsr`s to its own
labels has to be assembled (with `.org`) for the address it will run at.

hylang has no way to call machine code, but `(peek addr)` and `(poke addr byte)` reach memory, and `(sys name ...)`
makes any system call (Chapter 21).
