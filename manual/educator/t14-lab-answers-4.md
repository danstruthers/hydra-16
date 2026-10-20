# Topic 14. Labs 24 to 26

## Lab 24. The 65C02, on Paper

**1.**

| Register | Bits | What it's for |
|---|---|---|
| A | 8 | The accumulator: arithmetic and logic are done here |
| X | 8 | An index register: counting, and pointing into tables |
| Y | 8 | Another index register |
| S | 8 | The stack pointer: where the next byte pushed on the stack goes |
| PC | 16 | The program counter: the address of the next instruction |
| P | 8 | The processor status: the flags |

**2.**

| The sum | A | Carry |
|---|---|---|
| $10 + $20 | $30 | 0 |
| $0F + $01 | $10 | 0 |
| $80 + $80 | $00 | 1 |
| $FF + $01 | $00 | 1 |
| $F0 + $20 | $10 | 1 |

A sum that passes $FF keeps only its low two hex digits in A, and the "one to carry" goes into the carry flag.

**3.**

| The instruction | A | X | Z | What it did |
|---|---|---|---|---|
| `lda #$03` | $03 | (unknown) | 0 | Put the number 3 in A |
| `tax` | $03 | $03 | 0 | Copied A into X |
| `clc` | $03 | $03 | 0 | Cleared the carry, ready to add |
| `adc #$04` | $07 | $03 | 0 | Added 4 (and the carry, 0) to A |
| `dex` | $07 | $02 | 0 | Took 1 from X |
| `dex` | $07 | $01 | 0 | And again |
| `dex` | $07 | $00 | 1 | And again: the result is zero, so Z is set |

(Strictly, `clc` leaves Z as it was. A student who writes "unchanged" for flags that an instruction doesn't touch
has understood more than one who writes 0.)

**4.** Four times: X counts 4, 3, 2, 1, and `bne` falls through when `dex` makes it 0. A is $14 at the end, which
is 20.

**5.** 4 × 5, by adding 5 four times. For 7 × 6: `ldx #$06` and `adc #$07` (or the other way round).

The Hydra will check both, and the challenge's sum too. HyBASIC can hold the routines, and show A afterwards:

```session
> 10 T = 0
> 20 ASM
> 30 times:  lda #$00
> 40         ldx #$04
> 50 loop:   clc
> 60         adc #$05
> 70         dex
> 80         bne loop
> 90         rts
> 100 sum:   lda #$00
> 110        ldx #$0A
> 120 next:  stx t + 1
> 130        clc
> 140        adc t + 1
> 150        dex
> 160        bne next
> 170        rts
> 180 END ASM
> 190 CALL ASM times : RREG A, X, Y, P : PRINT A; HEX$(A)
> 200 CALL ASM sum : RREG A, X, Y, P : PRINT A; HEX$(A)
> RUN
 20 14
 55 37
```

**6.**

| The instruction | Its mode | The byte it loads is |
|---|---|---|
| `lda #$41` | Immediate | The number $41 itself |
| `lda $22` | Zero page | The byte at address $0022 |
| `lda $1234` | Absolute | The byte at address $1234 |
| `lda $1234,x` | Absolute, X | The byte at $1234 plus whatever is in X |
| `lda ($22),y` | Indirect, Y | The byte at the address held in $22 and $23, plus Y |

**7.** `A9 41`; `85 22`; `60`.

**Questions.**

1. Machine code is the bytes the processor actually reads and obeys. Assembly language is the same
   instructions written as names that people can read; an assembler turns each line into its bytes.
2. It holds the address of the next instruction, and normally just moves on past each one. Jumps (`jmp`),
   branches, subroutine calls and returns (`jsr`, `rts`), and interrupts change it.
3. `lda #$41` puts the number $41 in A. `lda $41` puts in A whatever byte is stored *at address* $41.
4. Up to 128 bytes back or 127 forward. Further than that takes a `jmp`.
5. The first 256 bytes of memory, $0000 to $00FF. Instructions that use it are a byte shorter and a cycle
   quicker; and pointers (addresses used through `($22),y`) can *only* be kept there.
6. By adding, over and over, in a loop. Anything a bigger processor does in one instruction, a 65C02 does with
   a few simple ones.

**Challenge.** The second routine in the transcript above: A starts at 0; X counts down from 10; each time
round, X is put into memory and added to A. The answer is 55, or $37. Adding up to 30 would give 465, which
doesn't fit in a byte: A would hold only the low part, and the carry flag would be set by the addition that
overflowed. So the program can find out by testing the carry after each `adc`, with `bcs`.

## Lab 25. Your First Machine Code

**1.** Nothing. From `as`, silence means success: it made the program `stars`.

**2.** For example:

| Find | The line |
|---|---|
| A comment | `; stars - a triangle of stars` |
| A constant being made | `ROWS        = 5` |
| A variable being made | `row:        .res 1` |
| A label | `main:` (or `line:`, `@row:`, `@star:`) |
| A system call | `jsr PUTC` |
| The instruction that ends the program | The `rts` at the end of `main` |

**3.** One line each time. With a name, the number is in one place, with a word that says what it means; if the 5
were written wherever it's needed, changing it would mean finding every one (and `ROWS + 1` would be a
mysterious 6).

**4.**

| The mistake | What `as` said |
|---|---|
| `lad` for `lda` | `as: stars.s:15: not an instruction, directive or macro: lad` |
| `lda 1` for `lda #1` | Nothing: it's a valid instruction |
| `jsr lime` | `as: stars.s:18: undefined: lime` |
| No `.include "hydra.inc"` | `undefined: PUTC`, twice, and two more for names the header needs |

Each message has the file's name and the line's number. The second mistake is the important one. `lda 1` loads
the byte *at address 1*, which is the ROM bank register, and so the triangle starts with a row as long as
whatever number happens to be there, and carries on from that. The assembler can only check that a program is
*written* correctly, never that it does what was meant.

**5.**

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

`main` is at $0830, and `line` at $0842. A program is loaded at $0800, but its first 48 bytes ($30) are its
header, so the code begins at $0830.

**6.** The program is 83 bytes, and its source about ten times that. The source has what people need and the
processor doesn't: the comments, the names, the layout.

<!-- session
cd /ram
!file samples/stars.s
as stars.s
-->

```session
/ram> ls -l stars stars.s
--rw-rw-rw- fr       83 2000-01-01 00:01 stars
--rw-rw-rw- fr      873 2000-01-01 00:00 stars.s
```

**7.** One way: start `row` at `ROWS`, count down, and stop at 0.

```text
main:       lda #ROWS           ; The longest row first
            sta row
@row:       lda row
            jsr line
            dec row             ; The next row is one shorter, till none is left
            bne @row
            rts
```

**Questions.**

1. It turns a text file of assembly language (the source) into a file of machine code that the Hydra can run
   (the program).
2. A system call may change A, X and Y. `line` calls `PUTC` for every star, so a count kept in X wouldn't
   survive. Memory does.
3. `.zeropage`, before their lines. `row` is at $22 and `count` at $23: the first two bytes of the program's own
   part of the zero page.
4. A request to HydraOS, made by `jsr` to a fixed address that has a name. `stars` uses `PUTC`, which prints the
   character whose code is in A.
5. `lda 1` reads memory address 1, and whatever is there, when a programmer nearly always means the number 1.
   It's the commonest bug in 6502 programming, and the assembler can't warn about it.

**Challenge.** One way to print 0 to 9:

```text
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "count", main

.zeropage
digit:      .res 1

.code
main:       stz digit           ; From 0
@next:      lda digit
            clc
            adc #'0'            ; A digit's code is $30 plus the digit
            jsr PUTC
            inc digit
            lda digit
            cmp #10
            bne @next
            lda #LF
            jsr PUTC
            rts
```

## Lab 26. Taking Programs Apart

**1.** Without the labels file, `dis` makes up names for the places that are jumped to (`L0834`) and shows
variables as bare addresses (`$22`). With it, the program's own names come back: `row`, `count`, `line`. What
never comes back is everything that wasn't in the program to begin with: the comments, the constant `ROWS` (it's
just a 6 now), the layout.

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
-->

```session indented
/ram> dis stars | head -12
; stars, as dis read it: as assembles this to its bytes again
.include "hydra.inc"

.include "hyx2.inc"

        HYX2_PROGRAM "stars", main
.code
main:
        lda #$01
        sta $22
L0834:
        lda $22
/ram> dis -l stars.lbl stars | head -12
; stars, as dis read it: as assembles this to its bytes again
.include "hydra.inc"
row             = $22
count           = $23

.include "hyx2.inc"

        HYX2_PROGRAM "stars", main
.code
main:
        lda #$01
        sta row
```

**2.** `cmp` says nothing, which means the two files are the same, byte for byte: so `echo` prints `the same`. It
proves that the disassembly is complete and exact: assembling it gives back the very same program.

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
-->

```session
/ram> dis stars >stars2.s
/ram> as stars2.s stars2
/ram> cmp stars stars2 && echo the same
the same
```

**3.** Any of the calls it makes: a `jsr` to a name from `hydra.inc`, such as `jsr WRITE`.

**4.** `db` shows the task it started the program in, the registers, and the next instruction. The program
counter is $0830, the program's first instruction. A holds whatever was left in it: `stars` hasn't run a single
instruction yet.

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
-->

```session
/ram> db ./stars
task 3
PC=0830 A=30 X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0830  A9 01     lda #$01
db> l /lib/as/hydra.inc
112 symbols
db> l stars.lbl
4 symbols
db> d main 9
0830  A9 01     lda #$01                 ; main
0832  85 22     sta row                  ; main+2
0834  A5 22     lda row                  ; main+4
0836  20 42 08  jsr line                 ; main+6
0839  E6 22     inc row                  ; main+9
083B  A5 22     lda row                  ; main+B
083D  C9 06     cmp #$06                 ; main+D
083F  D0 F3     bne main+4               ; main+F
0841  60        rts                      ; main+11
```

**5.**

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
-->

```session
db> s
PC=0832 A=01 X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0832  85 22     sta row                  ; main+2
db> s
PC=0834 A=01 X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0834  A5 22     lda row                  ; main+4
db> s
PC=0836 A=01 X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0836  20 42 08  jsr line                 ; main+6
db> s
PC=0842 A=01 X=FF Y=00 S=FB P=nv--dizc W=0 U=0 RAM=00 ROM=00
0842  85 23     sta count                ; line
db> s
PC=0844 A=01 X=FF Y=00 S=FB P=nv--dizc W=0 U=0 RAM=00 ROM=00
0844  A9 2A     lda #$2A                 ; line+2
```

| I typed | PC | A | The next instruction | What just happened |
|---|---|---|---|---|
| `s` | 0832 | 01 | `sta row` | `lda #$01`: A became 1 |
| `s` | 0834 | 01 | `lda row` | `sta row`: the 1 went into `row` |
| `s` | 0836 | 01 | `jsr line` | `lda row`: A loaded from `row` (still 1) |
| `s` | 0842 | 01 | `sta count` | `jsr line`: the program counter jumped into `line`, and S went down by 2 |
| `s` | 0844 | 01 | `lda #$2A` | `sta count`: the 1 went into `count` |

**6.** After the first two steps, `row` is 1, as it should be. `count` holds whatever was in that byte of memory
before: the program hasn't set it yet. Memory isn't zero until something makes it so.

**7.** Each `c` runs the program on to the breakpoint, at the instruction that's about to print a new line. So
between stops it prints a row of stars (one more each time) and the new line from the time before. `r` shows
the registers at the stop: PC is $084D each time, and A is $2A, the star it has just printed.

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
-->

```session
db> b line+B
db> c
*breakpoint 1
PC=084D A=2A X=FF Y=00 S=FB P=nv--diZc W=0 U=0 RAM=00 ROM=00
084D  A9 0A     lda #$0A                 ; line+B
db> r
PC=084D A=2A X=FF Y=00 S=FB P=nv--diZc W=0 U=0 RAM=00 ROM=00
084D  A9 0A     lda #$0A                 ; line+B
db> c

**breakpoint 1
PC=084D A=2A X=FF Y=00 S=FB P=nv--diZc W=0 U=0 RAM=00 ROM=00
084D  A9 0A     lda #$0A                 ; line+B
```

**8.** The rest of the triangle is printed in `#`: the program in memory now has $23 where it had $2A. Neither
file has changed. `stars.s` still says `'*'`, and `stars` on the disk still has $2A in it: run it again, and the
stars are back. `db` changed only the copy in the task's memory.

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
-->

```session
db> d line 3
0842  85 23     sta count                ; line
0844  A9 2A     lda #$2A                 ; line+2
0846  20 50 F9  jsr PUTC                 ; line+4
db> w 0845 23
db> c
#
##
###
####
#####
task 3 ended: code 0
```

**Questions.**

1. For programs whose source you don't have; for checking what the assembler really produced; and for learning
   how someone else's program works.
2. Nothing in a program's bytes says which are instructions and which are data: a string looks like any other
   bytes. `dis` follows the program's flow from its start, and what it can't reach as code, or what isn't a
   valid instruction, it shows as data (`.byte`).
3. `s` steps *into* the subroutine: the next stop is its first instruction. `n` runs the whole subroutine and
   stops at the instruction after the `jsr`.
4. A place where the program is to stop. `db` writes a `BRK` instruction over the instruction there (remembering
   what was underneath). When the processor reaches it, HydraOS stops the task and tells `db`.
5. Make the same change in the source, and assemble it again. The fix in memory is gone as soon as the program
   ends.

**Challenge.** The manual's section 29.8 is the answer. The bug: `LDR r0, msg` loads a register pair and, on the
way, changes A, which held the error code that the next call needed. The fix is to save A on the stack round the
`LDR`: `pha` before, `pla` after. A student who finds that A changes across a line that "shouldn't" change it
has found it.
