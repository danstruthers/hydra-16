# Chapter 27. The Disassembler

A **disassembler** does an assembler's job backwards: it reads machine code and writes it out as assembly language.
It's how you look inside a program you don't have the source of, check what the assembler really made, or find out
how the Hydra's own tools work. The Hydra has three: the `dis` command, HyForth's `disasm`, and the debugger's `d`
command. All three use the same part of the asm library, so they all write instructions exactly as `as` reads them.

```text
dis [-cnw] [-l labels] [-o addr] file
```

| Option | What it does |
|---|---|
| `-l` *labels* | Use the names in a labels file (`as -l`'s, section 26.17) |
| `-n` | No names for the system calls and r0 to r15: just their addresses |
| `-w` | Wide: the instructions in the SDK's columns, as its sources are written |
| `-c` | Every byte as code: none taken for data |
| `-o` *addr* | The file is raw bytes (`as -b`'s), to be loaded at *addr* (in hex) |

## 27.1 Taking a program apart

Here is Chapter 26's `stars`, taken apart:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
-->

```session indented
/ram> dis stars
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
        jsr L0842
        inc $22
        lda $22
        cmp #$06
        bne L0834
        rts
L0842:
        sta $23
L0844:
        lda #$2A
        jsr PUTC
        dec $23
        bne L0844
        lda #$0A
        jsr PUTC
        rts
```

Compare it with `stars.s` (section 26.3). The instructions are all there, but much has gone, because it was never in
the program: the comments, the constant `ROWS` (just `#$06` is left, `ROWS + 1`), the names of the variables (just
their addresses, $22 and $23) and of the labels. `dis` makes up a name for each place an instruction jumps or
branches to: `L` and its address, so `L0842` is the routine at $0842, `line`. It knows the system calls' names
(`PUTC`), and `main`, where the program starts. Numbers are in hex, and characters as their codes (`#$2A` is `'*'`).

## 27.2 Names from a labels file

With the labels file `as -l` wrote, `dis` has the names back:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
-->

```session indented
/ram> dis -l stars.lbl stars
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
L0834:
        lda row
        jsr line
        inc row
        lda row
        cmp #$06
        bne L0834
        rts
line:
        sta count
L0844:
        lda #$2A
        jsr PUTC
        dec count
        bne L0844
        lda #$0A
        jsr PUTC
        rts
```

Names for places outside the program, like the zero page's, become constants at the top (`row = $22`). The two
cheap local labels, `@row` and `@star`, weren't in the labels file, so they're still `L0834` and `L0844`.

## 27.3 And back again

The first line says it: `as` assembles what `dis` writes back into the same bytes. So you can take a program apart,
change it, and put it back together, even without its source:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
-->

```session indented
/ram> dis stars >stars2.s
/ram> as stars2.s stars2
/ram> cmp stars stars2 && echo the same
the same
```

`cmp` compares two files, and says nothing if they're the same. Edit `stars2.s` (change `#$2A` to `#$2B`, say),
assemble it again, and `stars2` prints a triangle of plus signs.

## 27.4 Code and data

A program file has data in it as well as code: strings, tables, numbers. A byte doesn't say which it is, so `dis` has
to work it out. It starts where the program starts, at `main`, and follows the code: each instruction leads to the
next one, and a branch, jump or `jsr` leads to another place too. It stops following at an instruction that doesn't
go on to the next: `rts`, `rti`, `jmp`, `bra`, `brk` or `stp`. Every byte it reached is code; the rest is data, which
it writes as `.byte` lines, with any text in quotes.

Look at `hi` from Chapter 26. Its `PRINT "Hello, "` keeps the text in the code, with a jump over it, and `dis` sees
that:

<!-- session
cd /ram
-->

```session indented
/ram> cp /sd/0/sample/hi .
/ram> dis hi | head -32
; hi, as dis read it: as assembles this to its bytes again
.include "hydra.inc"

.include "hyx2.inc"

        HYX2_PROGRAM "hi", main
.code
main:
        lda r0
        sta $22
        lda r0+1
        sta $23
        lda ($22)
        bne L0844
        lda #$34
        sta $22
        lda #$09
        sta $23
L0844:
        lda #$52
        sta r0
        lda #$08
        sta r0+1
        jsr PUTS
        jmp L085A
        .byte "Hello, ", $00
L085A:
        lda $22
        sta r0
        lda $23
        sta r0+1
        jsr PUTS
```

`lda #$52`, `sta r0`, `lda #$08` and `sta r0+1` are how `PRINT` puts $0852, the address of the text just after the
`jmp`, in r0, for `PUTS`. The end of the program has its strings, and its BSS:

<!-- session
cd /ram
cp /sd/0/sample/hi .
-->

```session indented
/ram> dis hi | tail -12
        lda #$43
        sta r0
        lda #$09
        sta r0+1
        jsr PUTS
        rts
        .byte $79, $6F, $75, $00, $00, $21, $0A, $00, "window", $00, $2E
        .byte $0A, $00
.bss
        .res 64
B0986:
        .res 8
```

`dis` writes text in quotes only when there are at least four printable characters in a row, so `$79, $6F, $75`
(`you`) stays as numbers. The BSS isn't in the file, so `dis` writes its size, with a `B` label where the code uses
it.

Following the code can miss some: code that's only reached through a table of addresses, or a `jmp (addr)`, looks
like data. `-c` takes every byte as code, from the start to the end, which shows it (and shows data as nonsense
instructions, so use it with care).

## 27.5 The other options

`-w` writes the SDK's columns, which are easier to read:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
-->

```session indented
/ram> dis -w -l stars.lbl stars | tail -10
line:
            sta         count
L0844:
            lda         #$2A
            jsr         PUTC
            dec         count
            bne         L0844
            lda         #$0A
            jsr         PUTC
            rts
```

`-n` leaves out the names of the system calls and the call registers, so `jsr PUTC` is `jsr $F950`, and `lda r0` is
`lda $02`.

`-o` reads a raw file, as `as -b` makes them, and says where it's loaded, since a raw file doesn't:

<!-- session
cd /ram
!file samples/raw.s
as -b raw.s
-->

```session indented
/ram> dis -o 1000 raw
; raw, as dis read it: as assembles this to its bytes again
.include "hydra.inc"

.org $1000
.code
        asl a
        bcc L1004
        inx
L1004:
        rts
```

## 27.6 Inside the Hydra's tools

The Hydra's own tools are programs too, on the ROM disk in `/rom/bin`, and `dis` can take any of them apart. Here is
the start of `echo`:

<!-- session
-->

```session indented
/> dis /rom/bin/echo | head -40
; /rom/bin/echo, as dis read it: as assembles this to its bytes again
.include "hydra.inc"

.include "hyx2.inc"

        HYX2_PROGRAM "echo", main
.code
main:
        lda r0
        sta $22
        lda r0+1
        sta $23
        lda #$01
        sta B0A40
        sta B0A41
        ldx #$00
        lda ($22)
        cmp #$2D
        bne L085B
        ldy #$01
        lda ($22),y
        cmp #$6E
        bne L085B
        iny
        lda ($22),y
        bne L085B
        stz B0A40
        jsr L088D
L085B:
        lda ($22)
        beq L087D
        lda B0A41
        bne L0869
        lda #$20
        jsr L08A0
L0869:
        stz B0A41
        ldy #$00
L086E:
        lda ($22),y
```

You can read it like a puzzle. It keeps its arguments' address (r0) on the zero page at $22. Then it checks for `-n`:
the first argument's first character is $2D (`-`), its second $6E (`n`), and then it ends. If so, it clears the
variable at $0A40, which is whether to print a new line at the end, and moves on to the next argument (`L088D`).
Then, at `L085B`, it prints each argument (`L08A0` puts a character into its output), with a space ($20) before
each but the first.

## 27.7 Disassembling in HyForth

HyForth's `lib disasm` has `disasm` ( *addr u* -- ), which disassembles *u* instructions from *addr*, in any
memory the task can see. And while it's loaded, `see` shows a **code word** (a word written in machine code, as
HyForth's own words are) as its instructions:

<!-- session
-->

```session indented
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
/> see 2dup
code 2dup
 A318  20 CD A2  jsr $A2CD  \ over
 A31B  4C CD A2  jmp $A2CD  \ over
end-code
```

This shows how HyForth keeps its stack: on the zero page, the low bytes of its cells from $22 and their high bytes
from $42, with X pointing at the top. `dup` takes 1 from X, to make room, and copies the top cell's two bytes into
the new top. `2dup` is just `over over`: a `jsr` to `over`, then a `jmp` to it (which is quicker than another
`jsr` and an `rts`). A `jsr` or `jmp` to a word has its name after it.

`disasm` reaches anywhere. Here is the start of HydraOS's jump table, at $F800, and `PUTC` and `PUTS`, at $F950:

<!-- session
lib disasm
-->

```session indented
/> hex f800 4 disasm decimal
 F800  4C 77 EB  jmp $EB77
 F803  4C B0 EB  jmp $EBB0
 F806  4C 88 EE  jmp $EE88
 F809  4C 2B EB  jmp $EB2B
/> hex f950 2 disasm decimal
 F950  4C 5C E2  jmp $E25C
 F953  4C 01 E3  jmp $E301
```

Each system call's address holds a `jmp` to the code that does it, in the kernel's ROM. Programs call the jump
table, not the code, so a new HydraOS can move its code about and every program still works.

## 27.8 In the debugger

The debugger's `d` command disassembles a running program's memory, with names from its labels file and
`hydra.inc`. Chapter 28 has it.
