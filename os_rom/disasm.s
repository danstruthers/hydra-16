.debuginfo
.zeropage

ZP_D_STATE:
    .byte       0
ZP_D_EXBYTES:
    .res        1
ZP_D_INST:
    .res        3
ZP_D_MODE:
    .res        1
ZP_XAM:
    .res        2      ; eXAMine address
ZP_D_ICOUNT:
    .res        1

.segment "DISASM"

; Offsets into MNEMONIC_STR
MN_adc := $2B
MN_and := $24
MN_asl := $10
MN_bbr := $90
MN_bbs := $66
MN_bcc := $71
MN_bcs := $38
MN_beq := $84
MN_bit := $57
MN_bmi := $52
MN_bne := $6B
MN_bpl := $45
MN_bra := $0E
MN_brk := $91
MN_bvc := $61
MN_bvs := $09
MN_clc := $2D
MN_cld := $2F
MN_cli := $89
MN_clv := $78
MN_cmp := $19
MN_cpx := $73
MN_cpy := $63
MN_stp := $7B
MN_dec := $76
MN_dex := $04
MN_dey := $26
MN_eor := $6D
MN_inc := $17
MN_inx := $54
MN_iny := $8B
MN_jmp := $94
MN_jsr := $4D
MN_lda := $22
MN_ldx := $30
MN_ldy := $12
MN_lsr := $1E
MN_nop := $5C
MN_ora := $6E
MN_pha := $7F
MN_php := $7D
MN_phx := $5E
MN_phy := $98
MN_pla := $29
MN_plp := $96
MN_plx := $46
MN_ply := $1B
MN_rmb := $82
MN_rol := $20
MN_ror := $33
MN_rti := $4F
MN_rts := $35
MN_sbc := $37
MN_sec := $87
MN_sed := $02
MN_sei := $0B
MN_smb := $8E
MN_sta := $49
MN_stx := $41
MN_sty := $3A
MN_stz := $68
MN_tax := $4A
MN_tay := $59
MN_trb := $07
MN_tsb := $36
MN_tsx := $3E
MN_txa := $42
MN_txs := $00
MN_tya := $3B
MN_wai := $15

AM_ACC   := 0     ;                     %0000
AM_REL   := 1     ; $rr => $aaaa        %0001
AM_ZPREL := 2     ; $zz,$rr => $aaaa    %0010
AM_ZP    := 3     ; $zz                 %0011
AM_ABSX  := 4     ; $aaaa,X             %0100
AM_ZPX   := 5     ; $zz,X               %0101
AM_ABSY  := 6     ; $aaaa,Y             %0110
AM_ZPY   := 7     ; $zz,Y               %0111
AM_IMP   := 8     ;                     %1000
AM_IMM   := 9     ; #$ii                %1001
AM_IND   := $A    ; ($aaaa)             %1010
AM_ZPIND := $B    ; ($zz)               %1011
AM_ABSIX := $C    ; ($aaaa,X)           %1110 
AM_ZPIX  := $D    ; ($zz,X)             %1101
AM_ABS   := $E    ; $aaaa               %1100
AM_ZPIY  := $F    ; ($zz),Y             %1111

.define M2(even, odd) (even + (odd*16))

MN_OFFSETS:
    .byte MN_brk, MN_ora, MN_nop, MN_nop, MN_tsb, MN_ora, MN_asl, MN_rmb, MN_php, MN_ora, MN_asl, MN_nop, MN_tsb, MN_ora, MN_asl, MN_bbr
    .byte MN_bpl, MN_ora, MN_ora, MN_nop, MN_trb, MN_ora, MN_asl, MN_rmb, MN_clc, MN_ora, MN_inc, MN_nop, MN_trb, MN_ora, MN_asl, MN_bbr
    .byte MN_jsr, MN_and, MN_nop, MN_nop, MN_bit, MN_and, MN_rol, MN_rmb, MN_plp, MN_and, MN_rol, MN_nop, MN_bit, MN_and, MN_rol, MN_bbr
    .byte MN_bmi, MN_and, MN_and, MN_nop, MN_bit, MN_and, MN_rol, MN_rmb, MN_sec, MN_and, MN_dec, MN_nop, MN_bit, MN_and, MN_rol, MN_bbr
    .byte MN_rti, MN_eor, MN_nop, MN_nop, MN_nop, MN_eor, MN_lsr, MN_rmb, MN_pha, MN_eor, MN_lsr, MN_nop, MN_jmp, MN_eor, MN_lsr, MN_bbr
    .byte MN_bvc, MN_eor, MN_eor, MN_nop, MN_nop, MN_eor, MN_lsr, MN_rmb, MN_cli, MN_eor, MN_phy, MN_nop, MN_nop, MN_eor, MN_lsr, MN_bbr
    .byte MN_rts, MN_adc, MN_nop, MN_nop, MN_stz, MN_adc, MN_ror, MN_rmb, MN_pla, MN_adc, MN_ror, MN_nop, MN_jmp, MN_adc, MN_ror, MN_bbr
    .byte MN_bvs, MN_adc, MN_adc, MN_nop, MN_stz, MN_adc, MN_ror, MN_rmb, MN_sei, MN_adc, MN_ply, MN_nop, MN_jmp, MN_adc, MN_ror, MN_bbr
    .byte MN_bra, MN_sta, MN_nop, MN_nop, MN_sty, MN_sta, MN_stx, MN_smb, MN_dey, MN_bit, MN_txa, MN_nop, MN_sty, MN_sta, MN_stx, MN_bbs
    .byte MN_bcc, MN_sta, MN_sta, MN_nop, MN_sty, MN_sta, MN_stx, MN_smb, MN_tya, MN_sta, MN_txs, MN_nop, MN_stz, MN_sta, MN_stz, MN_bbs
    .byte MN_ldy, MN_lda, MN_ldx, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_smb, MN_tay, MN_lda, MN_tax, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_bbs
    .byte MN_bcs, MN_lda, MN_lda, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_smb, MN_clv, MN_lda, MN_tsx, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_bbs
    .byte MN_cpy, MN_cmp, MN_nop, MN_nop, MN_cpy, MN_cmp, MN_dec, MN_smb, MN_iny, MN_cmp, MN_dex, MN_wai, MN_cpy, MN_cmp, MN_dec, MN_bbs
    .byte MN_bne, MN_cmp, MN_cmp, MN_nop, MN_nop, MN_cmp, MN_dec, MN_smb, MN_cld, MN_cmp, MN_phx, MN_stp, MN_nop, MN_cmp, MN_dec, MN_bbs
    .byte MN_cpx, MN_sbc, MN_nop, MN_nop, MN_cpx, MN_sbc, MN_inc, MN_smb, MN_inx, MN_sbc, MN_nop, MN_nop, MN_cpx, MN_sbc, MN_inc, MN_bbs
    .byte MN_beq, MN_sbc, MN_sbc, MN_nop, MN_nop, MN_sbc, MN_inc, MN_smb, MN_sed, MN_sbc, MN_plx, MN_nop, MN_nop, MN_sbc, MN_inc, MN_bbs

MN_AMODE_EVEN:
    .byte $88,$33,$08,$EE	; %0000xxx0
    .byte $B1,$53,$08,$4E	; %0001xxx0
    .byte $8E,$33,$08,$EE	; %0010xxx0
    .byte $B1,$55,$08,$44	; %0011xxx0
    .byte $88,$38,$08,$EE	; %0100xxx0
    .byte $B1,$58,$88,$48	; %0101xxx0
    .byte $88,$33,$08,$EA	; %0110xxx0
    .byte $B1,$55,$88,$4C	; %0111xxx0
    .byte $81,$33,$88,$EE	; %1000xxx0
    .byte $B1,$75,$88,$4E	; %1001xxx0
    .byte $99,$33,$88,$EE	; %1010xxx0
    .byte $B1,$75,$88,$64	; %1011xxx0
    .byte $89,$33,$88,$EE	; %1100xxx0
    .byte $B1,$58,$88,$48	; %1101xxx0
    .byte $89,$33,$88,$EE	; %1110xxx0
    .byte $B1,$58,$88,$48	; %1111xxx0
; 
    .byte $8D,$33,$89,$2E	; %xxx0xxx1
    .byte $8F,$35,$86,$24	; %xxx1xxx1

; 0..3 = even indexes, 4..7 = odd
;extra bytes:
;x000 = 0
;xxx1 = 1
;xyy0 = 2 (yy != 00)
;x1xx = Indexed
;   x10x = Indexed by X
;   x11x = Indexed by Y
;1xxx = Indirect (xxx != 000 and xxx != 1x0)

; x000: one byte
; xxx1: two byte
; xxx0 (!x000): three byte
; x10x == ,X
; x11x == ,Y

MNEMONIC_STR:
    .byte "TXSEDEXTRBVSEIBRASLDYWAINCMPLYLSROLDANDEYPLADCLCLDXRORTSBCSTYATSXSTXABPLXSTAXJSRTIBMINXBITAYNOPHXBVCPYBBSTZBNEORABCCPXDECLVSTPHPHARMBEQSECLINYSMBBRKJMPLPHY"

NamedHString HS_RelPrefix, " => $"

.segment "DISASM_CODE"

; ZP_XAM, ZP_XAM+1: Address to Disassemble
; .A.Y: Start address of instruction to disassemble
; C=0 means single instruction, C=1 means number of instructions/bytes to print in .X
; set ZP_D_STATE=1 to print disassembly, ZP_D_STATE=0 to print bytes
DISASM_AY:
    sty         ZP_XAM + 1
    sta         ZP_XAM
    bcc         DISASM
    clc
    stx         ZP_D_ICOUNT
    bne         DISASM_LOOP
    rts

DISASM:
    stz         ZP_D_ICOUNT
    inc         ZP_D_ICOUNT

DISASM_LOOP:
    jsr         _disasm_load_inst
    jsr         _disasm_print_inst_bytes
    lda         ZP_D_STATE
    bne         :+
    jmp         @end_of_print

:
    ldy         #0
    jsr         _disasm_print_padding
    PRINT_SPACE
    ldx         ZP_D_INST
    lda         MN_OFFSETS, x
    tax
    PRINT_CHAR  {MNEMONIC_STR, x}, {MNEMONIC_STR + 1, x}, {MNEMONIC_STR + 2, x}
    lda         ZP_D_EXBYTES
    bne         :+
    jmp         @end_of_print       ; one-byte inst prints only mnemonic

:
    lda         ZP_D_INST
    tax
    and         #7
    cmp         #7                  ; BBR, BBS, RMB, SMB
    bne         @ex_space
    txa
    SR_N        4
    and         #7
    PRINT_HEX
    bra         @skip_ex_space      ; skip one space

@ex_space:
    PRINT_SPACE

@skip_ex_space:
    PRINT_SPACE
    ldx         ZP_D_MODE
    cpx         #AM_IND             ; indirect
    bcc         @not_indirect
    cpx         #AM_ABS             ; but not absolute
    beq         @not_indirect
    PRINT_CHAR  #ASCII_LPAREN

@not_indirect:
    cpx         #AM_IMM
    bne         @not_immediate
    jsr         _disasm_immediate
    jmp         @end_of_print

@not_immediate:
    cpx         #AM_ZP              ; relative?
    bcs         @not_relative
    jsr         _disasm_zeropage
    txa
    lsr                             ; shift LOb to C
    bcs         @not_zprel          ; AM_REL = 1, AM_ZPREL = 2, so skip ZP part if LOb is 1
    PRINT_CHAR  #ASCII_COMMA
    PRINT_SPACE
    jsr         _disasm_zeropage

@not_zprel:
    _M_WRITE_HSTRING HS_RelPrefix
    clc
    ldy         ZP_D_EXBYTES
    stz         ZP_TEMP
    lda         ZP_D_INST, y
    bpl         @skip_ff_set
    dec         ZP_TEMP                             ; $FF => ZP_TEMP

@skip_ff_set:
    adc         ZP_XAM
    tax
    lda         ZP_XAM + 1
    adc         ZP_TEMP
    PRINT_BYTE
    txa
    PRINT_BYTE
    bra         @end_of_print

@not_relative:
    lda         ZP_D_EXBYTES
    cmp         #1
    beq         @two_byte_inst      ; inst is two bytes (vs three)?
    jsr         _disasm_absolute
    bra         @continue_multibyte

@two_byte_inst:
    jsr         _disasm_zeropage

@continue_multibyte:
    cpx         #AM_ABS             ; absolute?
    beq         @end_of_print
    cpx         #AM_ZPIY
    bne         @not_zpiy
   PRINT_CHAR   #ASCII_RPAREN

@not_zpiy:
    bbr2        ZP_D_MODE, @not_indexed ; indexed?
    txa
    lsr
    lsr                             ; shift LOb to C
    jsr         _disasm_commaXY

@not_indexed:
    cpx         #AM_IND
    bmi         @end_of_print
    cpx         #AM_ABS
    bcs         @end_of_print           ; AM_ABS or AM_ZPIY?  Skip trailing RPAREN
    PRINT_CHAR  #ASCII_RPAREN

@end_of_print:
    dec         ZP_D_ICOUNT
    beq         @done
    jmp         DISASM_LOOP

@done:
    rts

;---------------------------------------
; Helper procedures

_disasm_load_inst:
    ldy         #0
    ldx         ZP_D_STATE
    beq         @done               ; if not in DISASM mode, just print one byte
    lda         (ZP_XAM)
    lsr                             ; /2 and shift LOb to C
    bcc         @shift_mode         ; is even bytecode?
    and         #$0F
    ora         #$80

@shift_mode:
    lsr
    tax
    lda         MN_AMODE_EVEN, x
    bcc         @no_shift
    SR_N        4

@no_shift:
    and         #$0F
    sta         ZP_D_MODE
    and         #7                  ; test if bits 0..2 are zeros (one-byte inst)
    beq         @done
    iny
    and         #1
    bne         @done               ; odd modes are 2-byte inst
    iny

@done:
    sty         ZP_D_EXBYTES
    ldy         #0

:
    jsr         _disasm_inst_byte
    cpy         ZP_D_EXBYTES
    beq         :+
    iny
    bra         :-

:
    rts

;AM_ACC   := 0     ;                     %0000
;AM_REL   := 1     ; $rr => $aaaa        %0001
;AM_ZPREL := 2     ; $zz,$rr => $aaaa    %0010
;AM_ZP    := 3     ; $zz                 %0011
;AM_ABSX  := 4     ; $aaaa,X             %0100
;AM_ZPX   := 5     ; $zz,X               %0101
;AM_ABSY  := 6     ; $aaaa,Y             %0110
;AM_ZPY   := 7     ; $zz,Y               %0111
;AM_IMP   := 8     ;                     %1000
;AM_IMM   := 9     ; #$ii                %1001
;AM_IND   := $A    ; ($aaaa)             %1010
;AM_ZPIND := $B    ; ($zz)               %1011
;AM_ABSIX := $C    ; ($aaaa,X)           %1100 
;AM_ZPIX  := $D    ; ($zz,X)             %1101
;AM_ABS   := $E    ; $aaaa               %1110
;AM_ZPIY  := $F    ; ($zz),Y             %1111

_disasm_inst_byte:
    lda         (ZP_XAM)
    sta         ZP_D_INST, y
    inc         ZP_XAM
    bne         :+
    inc         ZP_XAM + 1

:
    rts

_disasm_print_inst_bytes:
    ldy         #0
    bra         :++

:
    iny

:
    PRINT_SPACE
    PRINT_BYTE  {ZP_D_INST, y}
    cpy         ZP_D_EXBYTES
    bcc         :--
    rts

_disasm_print_padding:
    lda         ZP_D_EXBYTES
    cmp         #1
    beq         :+
    eor         #2
    beq         :+++

:
    tax

:
    PRINT_SPACE
    PRINT_CHAR
    PRINT_CHAR
    dex
    bne         :-

:
    rts

; Carry: 1 = Y, Carry: 0 = X
_disasm_commaXY:
    PRINT_CHAR  #ASCII_COMMA
    lda         #ASCII_X
    bcc         :+
    inc
:
    PRINT_CHAR_JMP

_disasm_absolute:
    iny
    jsr         _disasm_zeropage
    dey
    bra         _disasm_zp_byte

_disasm_immediate:
    PRINT_CHAR  #ASCII_HASH
    ; fall through

_disasm_zeropage:
    iny
    PRINT_CHAR  #ASCII_DOLLAR

_disasm_zp_byte:
    PRINT_BYTE_JMP  {ZP_D_INST, y}
