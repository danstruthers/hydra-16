# Card 7. The 65C02

Chapter 26 teaches it, and Appendix D has every instruction's opcodes and timings.

### The registers

| Register | Bits | What it's for |
|---|---|---|
| A | 8 | The accumulator: arithmetic and logic happen here |
| X, Y | 8 | Index registers: counting, and pointing into tables |
| S | 8 | The stack pointer: the stack is at $0100 to $01FF |
| PC | 16 | The program counter: the next instruction's address |
| P | 8 | The flags: N V - B D I Z C |

### The flags

| Flag | Is 1 when |
|---|---|
| N | The last result was negative (its top bit was 1) |
| Z | The last result was zero |
| C | An addition carried out of the byte; or a subtraction *didn't* borrow; or a shift pushed a 1 out |
| V | A signed sum went out of range |
| I | Interrupts are held off |
| D | `ADC` and `SBC` work in decimal |

### The instructions, by job

| Job | Instructions |
|---|---|
| Load and store | `LDA` `LDX` `LDY` `STA` `STX` `STY` `STZ` (store zero) |
| Copy between registers | `TAX` `TAY` `TXA` `TYA` `TSX` `TXS` |
| The stack | `PHA` `PLA` `PHX` `PLX` `PHY` `PLY` `PHP` `PLP` |
| Add and subtract | `ADC` `SBC` (both use the carry: `CLC` before adding, `SEC` before subtracting) |
| Count | `INC` `DEC` `INX` `DEX` `INY` `DEY` |
| Logic | `AND` `ORA` `EOR` `BIT` `TRB` `TSB` |
| Shift | `ASL` `LSR` `ROL` `ROR` |
| Compare | `CMP` `CPX` `CPY` (they set Z, C and N, and change nothing else) |
| Branch | `BEQ` `BNE` (on Z); `BCS` `BCC` (on C); `BMI` `BPL` (on N); `BVS` `BVC` (on V); `BRA` (always) |
| Jump and call | `JMP` `JSR` `RTS` |
| Flags | `CLC` `SEC` `CLD` `SED` `CLI` `SEI` `CLV` |
| Interrupts and the rest | `BRK` `RTI` `NOP` `WAI` `STP` |
| Single bits | `RMB0`-`7` `SMB0`-`7` `BBR0`-`7` `BBS0`-`7` |

### After a compare

| After `cmp #n` | Branch with |
|---|---|
| A equals *n* | `beq` |
| A differs from *n* | `bne` |
| A is *n* or more (unsigned) | `bcs` |
| A is less than *n* (unsigned) | `bcc` |

### The addressing modes (section 26.6)

| Written | The byte it means |
|---|---|
| `lda #$41` | The number $41 itself |
| `lda $22` | The byte at $0022, on the zero page |
| `lda $1234` | The byte at $1234 |
| `lda $1234,x` | The byte at $1234 + X |
| `lda ($22)` | The byte at the address kept in $22 and $23 |
| `lda ($22),y` | The byte at that address + Y |
| `asl a` | A itself |
| `bne loop` | (A branch: up to 128 bytes back, or 127 on) |

### A program's skeleton, for `as` (Chapter 27)

```text
.include "hydra.inc"            ; the system calls' names
.include "hyx2.inc"             ; HYX2_PROGRAM
.include "macros.inc"           ; LF, PRINT, LDR ...

            HYX2_PROGRAM "name", main

.zeropage
count:      .res 1              ; a variable

.code
main:       lda #'A'
            jsr PUTC            ; a system call: print the character in A
            rts                 ; the end
```

`as name.s` assembles it; `./name` runs it; `as -l` writes a labels file too; `dis name` takes it apart; `db ./name`
runs it an instruction at a time (`s` step, `n` next, `c` continue, `b` breakpoint, `r` registers, `m` memory,
`q` quit).
