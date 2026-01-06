.zeropage
ZP_D_ICOUNT:
    .res        1
ZP_D_MODE:
    .res        1
ZP_XAM:
    .res 2      ; eXAMine address
ZP_D_ADDR:
    .res        2

.segment "BIOS"

; Offsets into MNEMONIC_STR
; ADC = $02
; AND = $21
; ASL = $14
; BBR = $26
; BBS = $2F
; BCC = $3D
; BCS = $36
; BEQ = $67
; BIT = $42
; BMI = $60
; BNE = $1C
; BPL = $46
; BRA = $00
; BRK = $7B
; BVC = $7E
; BVS = $4C
; CLC = $04
; CLD = $06
; CLI = $0A
; CLV = $33
; CMP = $0E
; CPX = $3A
; CPY = $3F
; STP = $87
; DEC = $08
; DEX = $50
; DEY = $23
; EOR = $1E
; INC = $0C
; INX = $6E
; INY = $62
; JMP = $71
; JSR = $6A
; LDA = $2C
; LDX = $81
; LDY = $84
; LSR = $16
; NOP = $76
; ORA = $1F
; PHA = $12
; PHP = $10
; PHX = $73
; PHY = $78
; PLA = $49
; PLP = $47
; PLX = $89
; PLY = $8C
; RMB = $5E
; ROL = $2A
; ROR = $28
; RTI = $6C
; RTS = $18
; SBC = $31
; SEC = $38
; SED = $4E
; SEI = $8F
; SMB = $1A
; STA = $53
; STX = $57
; STY = $5A
; STZ = $92
; TAX = $54
; TAY = $95
; TRB = $44
; TSB = $65
; TSX = $98
; TXA = $9B
; TXS = $58
; TYA = $5B
; WAI = $9E

OPCODES: .byte $00
    ;/* 0 */    ; brk,  ora,  nop,  nop,  tsb,  ora,  asl, rmb0,  php,  ora,  asl,  nop,  tsb,  ora,  asl, bbr0, /* 0 */
    ;/* 1 */    ; bpl,  ora,  ora,  nop,  trb,  ora,  asl, rmb1,  clc,  ora,  inc,  nop,  trb,  ora,  asl, bbr1, /* 1 */
    ;/* 2 */    ; jsr,  and,  nop,  nop,  bit,  and,  rol, rmb2,  plp,  and,  rol,  nop,  bit,  and,  rol, bbr2, /* 2 */
    ;/* 3 */    ; bmi,  and,  and,  nop,  bit,  and,  rol, rmb3,  sec,  and,  dec,  nop,  bit,  and,  rol, bbr3, /* 3 */
    ;/* 4 */    ; rti,  eor,  nop,  nop,  nop,  eor,  lsr, rmb4,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr, bbr4, /* 4 */
    ;/* 5 */    ; bvc,  eor,  eor,  nop,  nop,  eor,  lsr, rmb5,  cli,  eor,  phy,  nop,  nop,  eor,  lsr, bbr5, /* 5 */
    ;/* 6 */    ; rts,  adc,  nop,  nop,  stz,  adc,  ror, rmb6,  pla,  adc,  ror,  nop,  jmp,  adc,  ror, bbr6, /* 6 */
    ;/* 7 */    ; bvs,  adc,  adc,  nop,  stz,  adc,  ror, rmb7,  sei,  adc,  ply,  nop,  jmp,  adc,  ror, bbr7, /* 7 */
    ;/* 8 */    ; bra,  sta,  nop,  nop,  sty,  sta,  stx, smb0,  dey,  bit,  txa,  nop,  sty,  sta,  stx, bbs0, /* 8 */
    ;/* 9 */    ; bcc,  sta,  sta,  nop,  sty,  sta,  stx, smb1,  tya,  sta,  txs,  nop,  stz,  sta,  stz, bbs1, /* 9 */
    ;/* A */    ; ldy,  lda,  ldx,  nop,  ldy,  lda,  ldx, smb2,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx, bbs2, /* A */
    ;/* B */    ; bcs,  lda,  lda,  nop,  ldy,  lda,  ldx, smb3,  clv,  lda,  tsx,  nop,  ldy,  lda,  ldx, bbs3, /* B */
    ;/* C */    ; cpy,  cmp,  nop,  nop,  cpy,  cmp,  dec, smb4,  iny,  cmp,  dex,  wai,  cpy,  cmp,  dec, bbs4, /* C */
    ;/* D */    ; bne,  cmp,  cmp,  nop,  nop,  cmp,  dec, smb5,  cld,  cmp,  phx,  stp,  nop,  cmp,  dec, bbs5, /* D */
    ;/* E */    ; cpx,  sbc,  nop,  nop,  cpx,  sbc,  inc, smb6,  inx,  sbc,  nop,  nop,  cpx,  sbc,  inc, bbs6, /* E */
    ;/* F */    ; beq,  sbc,  sbc,  nop,  nop,  sbc,  inc, smb7,  sed,  sbc,  plx,  nop,  nop,  sbc,  inc, bbs7  /* F */
; 0..3 = even indexes, 4..7 = odd
;extra bytes:
;x000 = 0
;xxx1 = 1
;xyy0 = 2 (yy != 00)
;x1xx = Indexed
;   x10x = Indexed by X
;   x11x = Indexed by Y
;1xxx = Indirect (xxx != 000 and xxx != 1x0)

; ACC = 0                       %0000
; IMM = 1   #$ii                %0001
; ABS = 2   $aaaa               %0010
; ZP  = 3   $zz                 %0011
; ABSX = 4  $aaaa,X             %0100
; ZPX = 5   $zz,X               %0101
; ABSY = 6  $aaaa,Y             %0110
; ZPY = 7   $zz,Y               %0111
; IMP = 8                       %1000
; REL = 9   $rr[$aaaa]          %1001
; IND = A   ($aaaa)             %1010
; ZPIND = B ($zz)               %1011
; ZPREL = C n,$zz,$rr[$aaaa]    %1100   (where n = OPCODE>>4)
; ZPIX = D  ($zz,X)             %1101
; ZP2 = E   n,$zz               %1110   (where n = OPCODE>>4)
; ZPIY = F  ($zz),Y             %1111
ADDRESS_MODES: .byte $00

MNEMONIC_STR: .byte "BRADCLCLDECLINCMPHPHASLSRTSMBNEORANDEYBBROROLDABBSBCLVBCSECPXBCCPYBITRBPLPLABVSEDEXSTAXSTXSTYARMBMINYTSBEQJSRTINXJMPHXNOPHYBRKBVCLDXLDYSTPLXPLYSEISTZTAYTSXTXAWAI"

; ZP_XAM, ZP_XAM+1: Address to Disassemble
; .A.Y: Address at which to start disassembly
; C = 0, only disassemble one instruction
; C = 1, .X contains instruction count to disassemble, 0 means 256
DISASM_AY:
    sta         ZP_XAM
    sty         ZP_XAM + 1
    bcs         DISASM_X

DISASM:
    stz         ZP_D_ICOUNT
    inc         ZP_D_ICOUNT
    bra         DISASM1

; .X: Instruction count
DISASM_X:
    stx         ZP_D_ICOUNT

DISASM1:
    ldy         #0

NEXT_INST:
    lda         (ZP_XAM),Y
                                    ; do the disasm magic here

                                    ; decrement I count and go on to next inst if necessary
    iny
    bne         DISASMDECX          ; Y overflow?
    inc         ZP_XAM              ; increment LOB of addr
    bne         DISASMDECX          ; End of page?
    inc         ZP_XAM              ; increment HOB of addr

DISASMDECX:
    dec         ZP_D_ICOUNT
    bne         NEXT_INST
    rts

;IMM:
;    WRITE_CHAR ASCII_HASH
;    bra ZP

;ZPREL:
;    sec

;ZP2:
;    WRITE_HEX <operand> >> 4
;    WRITE_BYTE ASCII_COMMA

;ZP:
;    WRITE_CHAR ASCII_DOLLAR
;    WRITE_BYTE <<NextByte>>
;    ; IF not ZPREL, return
;    bcc REL
;    rts

;REL:
    ; output "," and fall through to REL, else done
;    WRITE_CHAR ASCII_COMMA
;    WRITE_CHAR ASCII_DOLLAR
;    WRITE_BYTE <<NextByte>>

;REL:
;    rts
