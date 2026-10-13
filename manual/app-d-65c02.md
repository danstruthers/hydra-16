# Appendix D. The 65C02's Instructions

This appendix lists every instruction of the W65C02S, as the assembler `as` writes it: what it does, the flags it
changes, and its opcode and cycles in each of its addressing modes. Chapter 26 explains them; WDC's data sheet for the
W65C02S has the chip's own tables.

## D.1 The addressing modes

The tables write each mode short, like this:

| Short | Mode | Written | Bytes |
|---|---|---|---|
| | Implied | `inx` | 1 |
| `a` | Accumulator | `asl a` | 1 |
| `#` | Immediate | `lda #$41` | 2 |
| `zp` | Zero page | `lda $22` | 2 |
| `zp,x` | Zero page, X | `lda $22,x` | 2 |
| `zp,y` | Zero page, Y | `ldx $22,y` | 2 |
| `abs` | Absolute | `lda $1234` | 3 |
| `abs,x` | Absolute, X | `lda $1234,x` | 3 |
| `abs,y` | Absolute, Y | `lda $1234,y` | 3 |
| `(zp)` | Zero page indirect | `lda ($22)` | 2 |
| `(zp),y` | Indirect, Y | `lda ($22),y` | 2 |
| `(zp,x)` | Indexed indirect | `lda ($22,x)` | 2 |
| `(abs)` | Absolute indirect | `jmp ($1234)` | 3 |
| `(abs,x)` | Absolute indexed indirect | `jmp ($1234,x)` | 3 |
| `rel` | Relative | `bne label` | 2 |
| `zp,rel` | Zero page, relative | `bbr3 $22,label` | 3 |

## D.2 The flags and the cycles

The **Flags** column lists the flags an instruction changes (section 26.3): **N** negative, **V** overflow, **B**
break, **D** decimal, **I** interrupt disable, **Z** zero, **C** carry. A `-` means it changes none.

The number in brackets after each opcode is how many **cycles** it takes (at 3.58 MHz, a cycle is about 0.28
millionths of a second). Some take more:

* `+` after the number: one more if the indexing (X or Y) carries the address into another page.
* A branch (`BCC` ... `BVS`, `BBR`, `BBS`): one more if it's taken, and one more again if it goes to another page.
  `BRA` is always taken: 3 cycles (4 to another page).
* `ADC` and `SBC`: one more in decimal mode.

## D.3 The instructions

In the table, *M* is the byte the instruction works on (its operand: section 26.6).

<!-- generated: tools/opcodes.js -->

| Instruction | What it does | Flags | Opcodes |
|---|---|---|---|
| `ADC` | Add with carry: A = A + M + C | N V Z C | (zp,x) $61 (6), zp $65 (3), # $69 (2), abs $6D (4), (zp),y $71 (5+), (zp) $72 (5), zp,x $75 (4), abs,y $79 (4+), abs,x $7D (4+) |
| `AND` | A = A AND M | N Z | (zp,x) $21 (6), zp $25 (3), # $29 (2), abs $2D (4), (zp),y $31 (5+), (zp) $32 (5), zp,x $35 (4), abs,y $39 (4+), abs,x $3D (4+) |
| `ASL` | Shift left: C gets bit 7, bit 0 gets 0 | N Z C | zp $06 (5), a $0A (2), abs $0E (6), zp,x $16 (6), abs,x $1E (6+) |
| `BBR`*n* | Branch if bit *n* of a zero page byte is 0 | - | BBR0 $0F (5), BBR1 $1F (5), BBR2 $2F (5), BBR3 $3F (5), BBR4 $4F (5), BBR5 $5F (5), BBR6 $6F (5), BBR7 $7F (5) |
| `BBS`*n* | Branch if bit *n* of a zero page byte is 1 | - | BBS0 $8F (5), BBS1 $9F (5), BBS2 $AF (5), BBS3 $BF (5), BBS4 $CF (5), BBS5 $DF (5), BBS6 $EF (5), BBS7 $FF (5) |
| `BCC` | Branch if C = 0 | - | rel $90 (2) |
| `BCS` | Branch if C = 1 | - | rel $B0 (2) |
| `BEQ` | Branch if Z = 1 (equal) | - | rel $F0 (2) |
| `BIT` | Test bits: Z = 1 if (A AND M) is 0; N and V get bits 7 and 6 of M (but not with #) | N V Z | zp $24 (3), abs $2C (4), zp,x $34 (4), abs,x $3C (4+), # $89 (2) |
| `BMI` | Branch if N = 1 (minus) | - | rel $30 (2) |
| `BNE` | Branch if Z = 0 (not equal) | - | rel $D0 (2) |
| `BPL` | Branch if N = 0 (plus) | - | rel $10 (2) |
| `BRA` | Branch always | - | rel $80 (2) |
| `BRK` | Break: a software interrupt | B D I | $00 (7) |
| `BVC` | Branch if V = 0 | - | rel $50 (2) |
| `BVS` | Branch if V = 1 | - | rel $70 (2) |
| `CLC` | C = 0 | C | $18 (2) |
| `CLD` | D = 0: binary arithmetic | D | $D8 (2) |
| `CLI` | I = 0: interrupts allowed | I | $58 (2) |
| `CLV` | V = 0 | V | $B8 (2) |
| `CMP` | Compare A with M (A - M, kept only in the flags) | N Z C | (zp,x) $C1 (6), zp $C5 (3), # $C9 (2), abs $CD (4), (zp),y $D1 (5+), (zp) $D2 (5), zp,x $D5 (4), abs,y $D9 (4+), abs,x $DD (4+) |
| `CPX` | Compare X with M | N Z C | # $E0 (2), zp $E4 (3), abs $EC (4) |
| `CPY` | Compare Y with M | N Z C | # $C0 (2), zp $C4 (3), abs $CC (4) |
| `DEC` | M = M - 1 (or A = A - 1) | N Z | a $3A (2), zp $C6 (5), abs $CE (6), zp,x $D6 (6), abs,x $DE (7) |
| `DEX` | X = X - 1 | N Z | $CA (2) |
| `DEY` | Y = Y - 1 | N Z | $88 (2) |
| `EOR` | A = A XOR M | N Z | (zp,x) $41 (6), zp $45 (3), # $49 (2), abs $4D (4), (zp),y $51 (5+), (zp) $52 (5), zp,x $55 (4), abs,y $59 (4+), abs,x $5D (4+) |
| `INC` | M = M + 1 (or A = A + 1) | N Z | a $1A (2), zp $E6 (5), abs $EE (6), zp,x $F6 (6), abs,x $FE (7) |
| `INX` | X = X + 1 | N Z | $E8 (2) |
| `INY` | Y = Y + 1 | N Z | $C8 (2) |
| `JMP` | Jump | - | abs $4C (3), (abs) $6C (6), (abs,x) $7C (6) |
| `JSR` | Jump to a subroutine (pushing the return address) | - | abs $20 (6) |
| `LDA` | A = M | N Z | (zp,x) $A1 (6), zp $A5 (3), # $A9 (2), abs $AD (4), (zp),y $B1 (5+), (zp) $B2 (5), zp,x $B5 (4), abs,y $B9 (4+), abs,x $BD (4+) |
| `LDX` | X = M | N Z | # $A2 (2), zp $A6 (3), abs $AE (4), zp,y $B6 (4), abs,y $BE (4+) |
| `LDY` | Y = M | N Z | # $A0 (2), zp $A4 (3), abs $AC (4), zp,x $B4 (4), abs,x $BC (4+) |
| `LSR` | Shift right: C gets bit 0, bit 7 gets 0 | N Z C | zp $46 (5), a $4A (2), abs $4E (6), zp,x $56 (6), abs,x $5E (6+) |
| `NOP` | Nothing | - | $EA (2) |
| `ORA` | A = A OR M | N Z | (zp,x) $01 (6), zp $05 (3), # $09 (2), abs $0D (4), (zp),y $11 (5+), (zp) $12 (5), zp,x $15 (4), abs,y $19 (4+), abs,x $1D (4+) |
| `PHA` | Push A | - | $48 (3) |
| `PHP` | Push P | - | $08 (3) |
| `PHX` | Push X | - | $DA (3) |
| `PHY` | Push Y | - | $5A (3) |
| `PLA` | Pull A | N Z | $68 (4) |
| `PLP` | Pull P | N V D I Z C | $28 (4) |
| `PLX` | Pull X | N Z | $FA (4) |
| `PLY` | Pull Y | N Z | $7A (4) |
| `RMB`*n* | Clear bit *n* of a zero page byte | - | RMB0 $07 (5), RMB1 $17 (5), RMB2 $27 (5), RMB3 $37 (5), RMB4 $47 (5), RMB5 $57 (5), RMB6 $67 (5), RMB7 $77 (5) |
| `ROL` | Rotate left: C gets bit 7, bit 0 gets C | N Z C | zp $26 (5), a $2A (2), abs $2E (6), zp,x $36 (6), abs,x $3E (6+) |
| `ROR` | Rotate right: C gets bit 0, bit 7 gets C | N Z C | zp $66 (5), a $6A (2), abs $6E (6), zp,x $76 (6), abs,x $7E (6+) |
| `RTI` | Return from an interrupt (pulling P, then the address) | N V D I Z C | $40 (6) |
| `RTS` | Return from a subroutine | - | $60 (6) |
| `SBC` | Subtract with borrow: A = A - M - (1 - C) | N V Z C | (zp,x) $E1 (6), zp $E5 (3), # $E9 (2), abs $ED (4), (zp),y $F1 (5+), (zp) $F2 (5), zp,x $F5 (4), abs,y $F9 (4+), abs,x $FD (4+) |
| `SEC` | C = 1 | C | $38 (2) |
| `SED` | D = 1: decimal (BCD) arithmetic | D | $F8 (2) |
| `SEI` | I = 1: interrupts held off | I | $78 (2) |
| `SMB`*n* | Set bit *n* of a zero page byte | - | SMB0 $87 (5), SMB1 $97 (5), SMB2 $A7 (5), SMB3 $B7 (5), SMB4 $C7 (5), SMB5 $D7 (5), SMB6 $E7 (5), SMB7 $F7 (5) |
| `STA` | M = A | - | (zp,x) $81 (6), zp $85 (3), abs $8D (4), (zp),y $91 (6), (zp) $92 (5), zp,x $95 (4), abs,y $99 (5), abs,x $9D (5) |
| `STP` | Stop the processor, until a reset | - | $DB (3) |
| `STX` | M = X | - | zp $86 (3), abs $8E (4), zp,y $96 (4) |
| `STY` | M = Y | - | zp $84 (3), abs $8C (4), zp,x $94 (4) |
| `STZ` | M = 0 | - | zp $64 (3), zp,x $74 (4), abs $9C (4), abs,x $9E (5) |
| `TAX` | X = A | N Z | $AA (2) |
| `TAY` | Y = A | N Z | $A8 (2) |
| `TRB` | Test and reset bits: Z as BIT's; then M = M AND NOT A | Z | zp $14 (5), abs $1C (6) |
| `TSB` | Test and set bits: Z as BIT's; then M = M OR A | Z | zp $04 (5), abs $0C (6) |
| `TSX` | X = S | N Z | $BA (2) |
| `TXA` | A = X | N Z | $8A (2) |
| `TXS` | S = X | - | $9A (2) |
| `TYA` | A = Y | N Z | $98 (2) |
| `WAI` | Wait for an interrupt | - | $CB (3) |

## D.4 The opcodes in order

Each square is an opcode: its row is the opcode's first hex digit, and its column the second. So `$A9`, row
`Ax` and column `x9`, is `LDA #`, taking 2 cycles. An empty square is an opcode the W65C02S doesn't use: it does
nothing, as a `NOP` does.

<div class="opmatrix">

| | x0 | x1 | x2 | x3 | x4 | x5 | x6 | x7 | x8 | x9 | xA | xB | xC | xD | xE | xF |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **0x** | BRK<br>&nbsp;<br>7 | ORA<br>(zp,x)<br>6 |  |  | TSB<br>zp<br>5 | ORA<br>zp<br>3 | ASL<br>zp<br>5 | RMB0<br>zp<br>5 | PHP<br>&nbsp;<br>3 | ORA<br>#<br>2 | ASL<br>a<br>2 |  | TSB<br>abs<br>6 | ORA<br>abs<br>4 | ASL<br>abs<br>6 | BBR0<br>zp,rel<br>5 |
| **1x** | BPL<br>rel<br>2 | ORA<br>(zp),y<br>5+ | ORA<br>(zp)<br>5 |  | TRB<br>zp<br>5 | ORA<br>zp,x<br>4 | ASL<br>zp,x<br>6 | RMB1<br>zp<br>5 | CLC<br>&nbsp;<br>2 | ORA<br>abs,y<br>4+ | INC<br>a<br>2 |  | TRB<br>abs<br>6 | ORA<br>abs,x<br>4+ | ASL<br>abs,x<br>6+ | BBR1<br>zp,rel<br>5 |
| **2x** | JSR<br>abs<br>6 | AND<br>(zp,x)<br>6 |  |  | BIT<br>zp<br>3 | AND<br>zp<br>3 | ROL<br>zp<br>5 | RMB2<br>zp<br>5 | PLP<br>&nbsp;<br>4 | AND<br>#<br>2 | ROL<br>a<br>2 |  | BIT<br>abs<br>4 | AND<br>abs<br>4 | ROL<br>abs<br>6 | BBR2<br>zp,rel<br>5 |
| **3x** | BMI<br>rel<br>2 | AND<br>(zp),y<br>5+ | AND<br>(zp)<br>5 |  | BIT<br>zp,x<br>4 | AND<br>zp,x<br>4 | ROL<br>zp,x<br>6 | RMB3<br>zp<br>5 | SEC<br>&nbsp;<br>2 | AND<br>abs,y<br>4+ | DEC<br>a<br>2 |  | BIT<br>abs,x<br>4+ | AND<br>abs,x<br>4+ | ROL<br>abs,x<br>6+ | BBR3<br>zp,rel<br>5 |
| **4x** | RTI<br>&nbsp;<br>6 | EOR<br>(zp,x)<br>6 |  |  |  | EOR<br>zp<br>3 | LSR<br>zp<br>5 | RMB4<br>zp<br>5 | PHA<br>&nbsp;<br>3 | EOR<br>#<br>2 | LSR<br>a<br>2 |  | JMP<br>abs<br>3 | EOR<br>abs<br>4 | LSR<br>abs<br>6 | BBR4<br>zp,rel<br>5 |
| **5x** | BVC<br>rel<br>2 | EOR<br>(zp),y<br>5+ | EOR<br>(zp)<br>5 |  |  | EOR<br>zp,x<br>4 | LSR<br>zp,x<br>6 | RMB5<br>zp<br>5 | CLI<br>&nbsp;<br>2 | EOR<br>abs,y<br>4+ | PHY<br>&nbsp;<br>3 |  |  | EOR<br>abs,x<br>4+ | LSR<br>abs,x<br>6+ | BBR5<br>zp,rel<br>5 |
| **6x** | RTS<br>&nbsp;<br>6 | ADC<br>(zp,x)<br>6 |  |  | STZ<br>zp<br>3 | ADC<br>zp<br>3 | ROR<br>zp<br>5 | RMB6<br>zp<br>5 | PLA<br>&nbsp;<br>4 | ADC<br>#<br>2 | ROR<br>a<br>2 |  | JMP<br>(abs)<br>6 | ADC<br>abs<br>4 | ROR<br>abs<br>6 | BBR6<br>zp,rel<br>5 |
| **7x** | BVS<br>rel<br>2 | ADC<br>(zp),y<br>5+ | ADC<br>(zp)<br>5 |  | STZ<br>zp,x<br>4 | ADC<br>zp,x<br>4 | ROR<br>zp,x<br>6 | RMB7<br>zp<br>5 | SEI<br>&nbsp;<br>2 | ADC<br>abs,y<br>4+ | PLY<br>&nbsp;<br>4 |  | JMP<br>(abs,x)<br>6 | ADC<br>abs,x<br>4+ | ROR<br>abs,x<br>6+ | BBR7<br>zp,rel<br>5 |
| **8x** | BRA<br>rel<br>2 | STA<br>(zp,x)<br>6 |  |  | STY<br>zp<br>3 | STA<br>zp<br>3 | STX<br>zp<br>3 | SMB0<br>zp<br>5 | DEY<br>&nbsp;<br>2 | BIT<br>#<br>2 | TXA<br>&nbsp;<br>2 |  | STY<br>abs<br>4 | STA<br>abs<br>4 | STX<br>abs<br>4 | BBS0<br>zp,rel<br>5 |
| **9x** | BCC<br>rel<br>2 | STA<br>(zp),y<br>6 | STA<br>(zp)<br>5 |  | STY<br>zp,x<br>4 | STA<br>zp,x<br>4 | STX<br>zp,y<br>4 | SMB1<br>zp<br>5 | TYA<br>&nbsp;<br>2 | STA<br>abs,y<br>5 | TXS<br>&nbsp;<br>2 |  | STZ<br>abs<br>4 | STA<br>abs,x<br>5 | STZ<br>abs,x<br>5 | BBS1<br>zp,rel<br>5 |
| **Ax** | LDY<br>#<br>2 | LDA<br>(zp,x)<br>6 | LDX<br>#<br>2 |  | LDY<br>zp<br>3 | LDA<br>zp<br>3 | LDX<br>zp<br>3 | SMB2<br>zp<br>5 | TAY<br>&nbsp;<br>2 | LDA<br>#<br>2 | TAX<br>&nbsp;<br>2 |  | LDY<br>abs<br>4 | LDA<br>abs<br>4 | LDX<br>abs<br>4 | BBS2<br>zp,rel<br>5 |
| **Bx** | BCS<br>rel<br>2 | LDA<br>(zp),y<br>5+ | LDA<br>(zp)<br>5 |  | LDY<br>zp,x<br>4 | LDA<br>zp,x<br>4 | LDX<br>zp,y<br>4 | SMB3<br>zp<br>5 | CLV<br>&nbsp;<br>2 | LDA<br>abs,y<br>4+ | TSX<br>&nbsp;<br>2 |  | LDY<br>abs,x<br>4+ | LDA<br>abs,x<br>4+ | LDX<br>abs,y<br>4+ | BBS3<br>zp,rel<br>5 |
| **Cx** | CPY<br>#<br>2 | CMP<br>(zp,x)<br>6 |  |  | CPY<br>zp<br>3 | CMP<br>zp<br>3 | DEC<br>zp<br>5 | SMB4<br>zp<br>5 | INY<br>&nbsp;<br>2 | CMP<br>#<br>2 | DEX<br>&nbsp;<br>2 | WAI<br>&nbsp;<br>3 | CPY<br>abs<br>4 | CMP<br>abs<br>4 | DEC<br>abs<br>6 | BBS4<br>zp,rel<br>5 |
| **Dx** | BNE<br>rel<br>2 | CMP<br>(zp),y<br>5+ | CMP<br>(zp)<br>5 |  |  | CMP<br>zp,x<br>4 | DEC<br>zp,x<br>6 | SMB5<br>zp<br>5 | CLD<br>&nbsp;<br>2 | CMP<br>abs,y<br>4+ | PHX<br>&nbsp;<br>3 | STP<br>&nbsp;<br>3 |  | CMP<br>abs,x<br>4+ | DEC<br>abs,x<br>7 | BBS5<br>zp,rel<br>5 |
| **Ex** | CPX<br>#<br>2 | SBC<br>(zp,x)<br>6 |  |  | CPX<br>zp<br>3 | SBC<br>zp<br>3 | INC<br>zp<br>5 | SMB6<br>zp<br>5 | INX<br>&nbsp;<br>2 | SBC<br>#<br>2 | NOP<br>&nbsp;<br>2 |  | CPX<br>abs<br>4 | SBC<br>abs<br>4 | INC<br>abs<br>6 | BBS6<br>zp,rel<br>5 |
| **Fx** | BEQ<br>rel<br>2 | SBC<br>(zp),y<br>5+ | SBC<br>(zp)<br>5 |  |  | SBC<br>zp,x<br>4 | INC<br>zp,x<br>6 | SMB7<br>zp<br>5 | SED<br>&nbsp;<br>2 | SBC<br>abs,y<br>4+ | PLX<br>&nbsp;<br>4 |  |  | SBC<br>abs,x<br>4+ | INC<br>abs,x<br>7 | BBS7<br>zp,rel<br>5 |

</div>

<!-- end generated: tools/opcodes.js -->
