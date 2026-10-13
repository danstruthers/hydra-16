# Lab 24. The 65C02, on Paper

> **THIS LAB:** Manual: Chapter 26. About an hour. You need a pencil, the manual, and Appendix D. No Hydra: in this
> lab, *you* are the processor.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. ✎ Fill in the table of the 65C02's registers (section 26.3).

| Register | How many bits | What it's for |
|---|---|---|
| A | | |
| X | | |
| Y | | |
| S | | |
| PC | | |
| P | | |

2. Hex arithmetic, as the processor does it: in one byte, with a carry. ✎ Work these out.

| The sum | The byte left in A | The carry (0 or 1) |
|---|---|---|
| $10 + $20 | | |
| $0F + $01 | | |
| $80 + $80 | | |
| $FF + $01 | | |
| $F0 + $20 | | |

3. Run this program in your head. After each instruction, write what A and X hold, and the Z flag (1 if the
   instruction's result was zero).

| The instruction | A | X | Z | What it did |
|---|---|---|---|---|
| `lda #$03` | | | | |
| `tax` | | | | |
| `clc` | | | | |
| `adc #$04` | | | | |
| `dex` | | | | |
| `dex` | | | | |
| `dex` | | | | |

4. This one has a loop. Run it until it reaches `rts`. ✎ How many times does it go round? What's in A at the end?

```text
        lda #$00
        ldx #$04
loop:   clc
        adc #$05
        dex
        bne loop
        rts
```

<div class="write l2"></div>

5. ✎ What does that program work out? Change it so that it works out 7 × 6.

<div class="write"></div>

6. Each instruction has an **addressing mode**: its way of saying which byte it means (section 26.6). ✎ Name the
   mode of each, and say where the byte comes from.

| The instruction | Its addressing mode | The byte it loads is |
|---|---|---|
| `lda #$41` | | |
| `lda $22` | | |
| `lda $1234` | | |
| `lda $1234,x` | | |
| `lda ($22),y` | | |

7. Be the assembler. Using Appendix D, turn these three instructions into bytes, in hex.

| The instruction | Its bytes |
|---|---|
| `lda #$41` | |
| `sta $22` | |
| `rts` | |

### Questions

1. ✎ What's the difference between machine code and assembly language?

<div class="write l2"></div>

2. ✎ What does the program counter do? What changes it, besides just going on to the next instruction?

<div class="write l2"></div>

3. ✎ `lda #$41` and `lda $41` differ by one character. What's the difference in what they do?

<div class="write l2"></div>

4. ✎ A branch like `bne` can only reach instructions close by. How far? What do you use to go further?

<div class="write l2"></div>

5. ✎ What's the zero page, and why do programs like to keep their variables there?

<div class="write l2"></div>

6. ✎ The 65C02 can't multiply. How did the program in step 4 manage?

<div class="write"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | name the registers, and say what each is for |
| ☐ | add two bytes in hex, with the carry |
| ☐ | follow a short program by hand, flags and all |
| ☐ | tell the addressing modes apart |
| ☐ | look up an instruction's opcode in Appendix D |

> **CHALLENGE:** ★ Write, on paper, a program that leaves in A the sum 1 + 2 + 3 + ... + 10. Use X as the counter.
> Run it by hand to check it. What's the answer in hex? What would go wrong if you added up to 30 instead, and
> how could the program find out?
