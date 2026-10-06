; ****************************************************************************
; disasm.s - HyForth's disassembler, the old HyForth's (/lib/forth/disasm.fl: lib disasm): disasm, every 65C02
; opcode (the Rockwell bit ones too: the W65C02 has them), with the old monitor's tables; and (see-code), which see
; (tools.fl) runs for an assembly word while this library is loaded: its instructions, in place of its bytes.

.include "forthlib.inc"

.bss
da_bytes:   .res        3                                   ; An instruction's bytes (from p1) ...
da_mode:    .res        1                                   ;   its mode (AM_*) ...
da_len:     .res        1                                   ;   its length ...
da_tgt:     .res        2                                   ;   where it branches or jumps to (0: nowhere) ...
da_far:     .res        2                                   ; SEE of a code word: the furthest branch forward ...
da_lim:     .res        2                                   ;   and the next header (its code's end at the most)
.code

            HEADER      "(see-code)", 0
seecode:                                                    ; ( nt -- ): the code word nt (an assembly word's
            lda         dlo,x                               ;   header), as see shows it
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            jmp         see_code

s_immed:    .byte       " immediate", 0

            HEADER      "disasm", 0
disasm:                                                     ; ( addr u -- ): u instructions from addr, a line each:
            lda         dlo,x                               ;   its address, its bytes, it (a jsr's or jmp's to a
            sta         tmp3                                ;   word, with the word's name)
            lda         dhi,x
            sta         tmp3 + 1
            lda         dlo + 1,x
            sta         p1
            lda         dhi + 1,x
            sta         p1 + 1
            inx
            inx
@line:
            lda         tmp3
            ora         tmp3 + 1
            beq         @done
            bit         intr                                ; (Ctrl-C)
            bvc         :+
            jmp         intr_throw
:
            jsr         da_line
            lda         tmp3
            bne         :+
            dec         tmp3 + 1
:
            dec         tmp3
            bra         @line
@done:
            rts

; SEE of the code word w (an assembly word: hdr_asm's): "code name", its instructions to its end (an rts, rti, jmp,
; bra or stp past every branch forward in it, or the next header), "end-code"
see_code:
            LDR         w2, s_code
            jsr         w2_out
            jsr         hdr_out
            jsr         hdr_xt
            lda         cnt
            pha
            jsr         cr
            lda         w2
            sta         p1
            sta         da_far
            lda         w2 + 1
            sta         p1 + 1
            sta         da_far + 1
            jsr         da_limit
@line:
            bit         intr                                ; (Ctrl-C)
            bvc         :+
            pla
            jmp         intr_throw
:
            lda         p1                                  ; (At the next header: the end)
            cmp         da_lim
            lda         p1 + 1
            sbc         da_lim + 1
            bcs         @end
            jsr         da_line
            lda         da_tgt + 1                          ; A branch forward in it: its end is after there
            beq         @ends
            lda         da_far
            cmp         da_tgt
            lda         da_far + 1
            sbc         da_tgt + 1
            bcs         @ends
            lda         da_tgt
            cmp         da_lim
            lda         da_tgt + 1
            sbc         da_lim + 1
            bcs         @ends
            lda         da_tgt
            sta         da_far
            lda         da_tgt + 1
            sta         da_far + 1
@ends:
            ldy         #da_ends_n - 1                      ; Its end: an instruction that goes no further, past
            lda         da_bytes                            ;   every branch forward
:
            cmp         da_ends,y
            beq         :+
            dey
            bpl         :-
            bra         @line
:
            lda         da_far
            cmp         p1
            lda         da_far + 1
            sbc         p1 + 1
            bcs         @line
@end:
            LDR         w2, s_endcode
            jsr         w2_out
            pla
            bpl         :+
            LDR         w, s_immed
            jsr         type_z
:
            jmp         cr

da_ends:    .byte       $60, $40, $4C, $6C, $7C, $80, $DB   ; rts rti jmp jmp ( ) jmp ( ,x) bra stp
da_ends_n   = * - da_ends
s_code:     .byte       "code ", 0
s_endcode:  .byte       "end-code", 0

; The zero-terminated string at w2 out (type_z's, w kept)
w2_out:
            ldy         #0
:
            lda         (w2),y
            beq         :+
            jsr         emit_a
            iny
            bra         :-
:
            rts

; da_lim: the first header after the code at p1 (any word list's), or HERE (code in RAM), or the module's end
da_limit:
            lda         here
            sta         da_lim
            lda         here + 1
            sta         da_lim + 1
            lda         p1 + 1
            cmp         #$A0
            bcc         :+
            stz         da_lim
            lda         #$E0
            sta         da_lim + 1
:
            lda         wl_last
            sta         w3
            lda         wl_last + 1
            sta         w3 + 1
@wl:
            lda         w3
            ora         w3 + 1
            beq         @done
            lda         (w3)
            sta         w2
            ldy         #1
            lda         (w3),y
            sta         w2 + 1
@hdr:
            lda         w2
            ora         w2 + 1
            beq         @next
            lda         p1                                  ; (After p1 ...
            cmp         w2
            lda         p1 + 1
            sbc         w2 + 1
            bcs         :+
            lda         w2                                  ;   and before da_lim: da_lim)
            cmp         da_lim
            lda         w2 + 1
            sbc         da_lim + 1
            bcs         :+
            lda         w2
            sta         da_lim
            lda         w2 + 1
            sta         da_lim + 1
:
            ldy         #1
            lda         (w2),y
            pha
            lda         (w2)
            sta         w2
            pla
            sta         w2 + 1
            bra         @hdr
@next:
            ldy         #3                                  ; (The word list before it)
            lda         (w3),y
            pha
            dey
            lda         (w3),y
            sta         w3
            pla
            sta         w3 + 1
            bra         @wl
@done:
            rts

; The instruction at p1 out, a line: its address, its bytes, its mnemonic and operand (a jsr's or jmp's to a word
; with "  \ name"); p1 past it.  OUT: da_bytes, da_mode, da_len its; da_tgt the address a branch or jmp goes to (else
; 0).  Keeps tmp3
da_line:
            jsr         space
            lda         p1 + 1
            jsr         hex2
            lda         p1
            jsr         hex2
            jsr         space
            jsr         da_decode
            ldy         #0                                  ; Its bytes, in 3 bytes' room
:
            jsr         space
            lda         da_bytes,y
            jsr         hex2
            iny
            cpy         da_len
            bne         :-
:
            cpy         #3
            beq         :+
            jsr         space
            jsr         space
            jsr         space
            iny
            bra         :-
:
            jsr         space
            jsr         space
            ldy         da_bytes                            ; Its mnemonic
            lda         mn_offsets,y
            tay
            lda         mn_str,y
            jsr         emit_a
            lda         mn_str + 1,y
            jsr         emit_a
            lda         mn_str + 2,y
            jsr         emit_a
            lda         da_bytes                            ; (rmb, smb, bbr, bbs: their bit)
            and         #7
            cmp         #7
            bne         :+
            lda         da_bytes
            lsr
            lsr
            lsr
            lsr
            and         #7
            ora         #'0'
            jsr         emit_a
:
            stz         da_tgt
            stz         da_tgt + 1
            lda         da_len
            cmp         #1
            bne         :+
            jmp         @end
:
            jsr         space
            ldy         da_mode                             ; Its operand: a prefix ...
            lda         da_pre,y
            beq         :+
            jsr         emit_a
:
            lda         da_mode
            cmp         #AM_REL
            beq         @rel
            cmp         #AM_ZPREL
            bne         @not_rel
            lda         da_bytes + 1                        ; (bbr, bbs: the zero page's byte, then where)
            jsr         byte_out
            lda         #','
            jsr         emit_a
            jsr         space
            lda         da_bytes + 2
            bra         @to
@rel:
            lda         da_bytes + 1
@to:
            ldy         #0                                  ;   (where a branch goes: after it, + its offset)
            ora         #0
            bpl         :+
            dey
:
            clc
            adc         p1
            sta         da_tgt
            tya
            adc         p1 + 1
            sta         da_tgt + 1
            clc
            lda         da_tgt
            adc         da_len
            sta         da_tgt
            bcc         :+
            inc         da_tgt + 1
:
            lda         #'$'
            jsr         emit_a
            lda         da_tgt + 1
            jsr         hex2
            lda         da_tgt
            jsr         hex2
            bra         @suffix
@not_rel:
            lda         da_len                              ;   the zero page's, or an immediate, or an address ...
            cmp         #3
            beq         :+
            lda         da_bytes + 1
            jsr         byte_out
            bra         @suffix
:
            lda         #'$'
            jsr         emit_a
            lda         da_bytes + 2
            jsr         hex2
            lda         da_bytes + 1
            jsr         hex2
@suffix:
            ldy         da_mode                             ;   then a suffix
            lda         da_suf,y
            tay
:
            lda         da_sufs,y
            beq         :+
            jsr         emit_a
            iny
            bra         :-
:
            lda         da_bytes                            ; A jsr or jmp: to a word?
            cmp         #$20
            beq         :+
            cmp         #$4C
            bne         @end
            lda         da_bytes + 1                        ;   (a jmp: where it goes)
            sta         da_tgt
            lda         da_bytes + 2
            sta         da_tgt + 1
:
            lda         w
            pha
            lda         w + 1
            pha
            lda         da_bytes + 1
            ldy         da_bytes + 2
            PUSHAY
            jsr         hdr_of_xt
            inx
            bcs         :+
            LDR         w2, s_comment
            jsr         w2_out
            jsr         hdr_out
:
            pla
            sta         w + 1
            pla
            sta         w
@end:
            clc                                             ; p1 past it
            lda         p1
            adc         da_len
            sta         p1
            bcc         :+
            inc         p1 + 1
:
            jmp         cr

s_comment:  .byte       "  \ ", 0

; "$xx": .A
byte_out:
            pha
            lda         #'$'
            jsr         emit_a
            pla
            jmp         hex2

; The instruction at p1: da_bytes (its opcode, then the next two), da_mode (AM_*: the old monitor's tables), da_len
da_decode:
            ldy         #2
:
            lda         (p1),y
            sta         da_bytes,y
            dey
            bpl         :-
            lda         da_bytes
            lsr
            bcc         :+                                  ; (An odd opcode: modes 64-71, by its bits 4-2)
            and         #$0F
            ora         #$80
:
            lsr                                             ; (C: the mode in the byte's high 4 bits)
            tay
            lda         mn_modes,y
            bcc         :+
            lsr
            lsr
            lsr
            lsr
:
            and         #$0F
            sta         da_mode
            ldy         #1                                  ; Its length: 1 (AM_ACC, AM_IMP), 2 (the odd modes), 3
            and         #7
            beq         :+
            iny
            and         #1
            bne         :+
            iny
:
            sty         da_len
            rts

AM_ACC      = 0                                             ; The modes: (none)
AM_REL      = 1                                             ;   $aaaa (a branch's: where it goes)
AM_ZPREL    = 2                                             ;   $zz, $aaaa (bbr, bbs)
AM_ZP       = 3                                             ;   $zz
AM_ABSX     = 4                                             ;   $aaaa,x
AM_ZPX      = 5                                             ;   $zz,x
AM_ABSY     = 6                                             ;   $aaaa,y
AM_ZPY      = 7                                             ;   $zz,y
AM_IMP      = 8                                             ;   (none)
AM_IMM      = 9                                             ;   #$ii
AM_IND      = $A                                            ;   ($aaaa)
AM_ZPIND    = $B                                            ;   ($zz)
AM_ABSIX    = $C                                            ;   ($aaaa,x)
AM_ZPIX     = $D                                            ;   ($zz,x)
AM_ABS      = $E                                            ;   $aaaa
AM_ZPIY     = $F                                            ;   ($zz),y

; Each mode's prefix, and its suffix (an offset in da_sufs)
da_pre:     .byte       0, 0, 0, 0, 0, 0, 0, 0, 0, '#', '(', '(', '(', '(', 0, '('
da_suf:     .byte       0, 0, 0, 0, 1, 1, 4, 4, 0, 0, 7, 7, 9, 9, 0, 13
da_sufs:    .byte       0, ",x", 0, ",y", 0, ")", 0, ",x)", 0, "),y", 0

; The old monitor's tables (os_rom/monitor/disasm.s): each opcode's mnemonic, an offset in mn_str (its three letters
; overlap the next's); and the modes, two to a byte (the low 4 bits: the opcode's bit 1 clear), the even opcodes' by
; their bits 7-2, the odd ones' by their bits 4-2 (64 on)
MN_adc      = $2B
MN_and      = $24
MN_asl      = $10
MN_bbr      = $90
MN_bbs      = $66
MN_bcc      = $71
MN_bcs      = $38
MN_beq      = $84
MN_bit      = $57
MN_bmi      = $52
MN_bne      = $6B
MN_bpl      = $45
MN_bra      = $0E
MN_brk      = $91
MN_bvc      = $61
MN_bvs      = $09
MN_clc      = $2D
MN_cld      = $2F
MN_cli      = $89
MN_clv      = $78
MN_cmp      = $19
MN_cpx      = $73
MN_cpy      = $63
MN_stp      = $7B
MN_dec      = $76
MN_dex      = $04
MN_dey      = $26
MN_eor      = $6D
MN_inc      = $17
MN_inx      = $54
MN_iny      = $8B
MN_jmp      = $94
MN_jsr      = $4D
MN_lda      = $22
MN_ldx      = $30
MN_ldy      = $12
MN_lsr      = $1E
MN_nop      = $5C
MN_ora      = $6E
MN_pha      = $7F
MN_php      = $7D
MN_phx      = $5E
MN_phy      = $98
MN_pla      = $29
MN_plp      = $96
MN_plx      = $46
MN_ply      = $1B
MN_rmb      = $82
MN_rol      = $20
MN_ror      = $33
MN_rti      = $4F
MN_rts      = $35
MN_sbc      = $37
MN_sec      = $87
MN_sed      = $02
MN_sei      = $0B
MN_smb      = $8E
MN_sta      = $49
MN_stx      = $41
MN_sty      = $3A
MN_stz      = $68
MN_tax      = $4A
MN_tay      = $59
MN_trb      = $07
MN_tsb      = $36
MN_tsx      = $3E
MN_txa      = $42
MN_txs      = $00
MN_tya      = $3B
MN_wai      = $15

mn_offsets:
            .byte       MN_brk, MN_ora, MN_nop, MN_nop, MN_tsb, MN_ora, MN_asl, MN_rmb, MN_php, MN_ora, MN_asl, MN_nop, MN_tsb, MN_ora, MN_asl, MN_bbr
            .byte       MN_bpl, MN_ora, MN_ora, MN_nop, MN_trb, MN_ora, MN_asl, MN_rmb, MN_clc, MN_ora, MN_inc, MN_nop, MN_trb, MN_ora, MN_asl, MN_bbr
            .byte       MN_jsr, MN_and, MN_nop, MN_nop, MN_bit, MN_and, MN_rol, MN_rmb, MN_plp, MN_and, MN_rol, MN_nop, MN_bit, MN_and, MN_rol, MN_bbr
            .byte       MN_bmi, MN_and, MN_and, MN_nop, MN_bit, MN_and, MN_rol, MN_rmb, MN_sec, MN_and, MN_dec, MN_nop, MN_bit, MN_and, MN_rol, MN_bbr
            .byte       MN_rti, MN_eor, MN_nop, MN_nop, MN_nop, MN_eor, MN_lsr, MN_rmb, MN_pha, MN_eor, MN_lsr, MN_nop, MN_jmp, MN_eor, MN_lsr, MN_bbr
            .byte       MN_bvc, MN_eor, MN_eor, MN_nop, MN_nop, MN_eor, MN_lsr, MN_rmb, MN_cli, MN_eor, MN_phy, MN_nop, MN_nop, MN_eor, MN_lsr, MN_bbr
            .byte       MN_rts, MN_adc, MN_nop, MN_nop, MN_stz, MN_adc, MN_ror, MN_rmb, MN_pla, MN_adc, MN_ror, MN_nop, MN_jmp, MN_adc, MN_ror, MN_bbr
            .byte       MN_bvs, MN_adc, MN_adc, MN_nop, MN_stz, MN_adc, MN_ror, MN_rmb, MN_sei, MN_adc, MN_ply, MN_nop, MN_jmp, MN_adc, MN_ror, MN_bbr
            .byte       MN_bra, MN_sta, MN_nop, MN_nop, MN_sty, MN_sta, MN_stx, MN_smb, MN_dey, MN_bit, MN_txa, MN_nop, MN_sty, MN_sta, MN_stx, MN_bbs
            .byte       MN_bcc, MN_sta, MN_sta, MN_nop, MN_sty, MN_sta, MN_stx, MN_smb, MN_tya, MN_sta, MN_txs, MN_nop, MN_stz, MN_sta, MN_stz, MN_bbs
            .byte       MN_ldy, MN_lda, MN_ldx, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_smb, MN_tay, MN_lda, MN_tax, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_bbs
            .byte       MN_bcs, MN_lda, MN_lda, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_smb, MN_clv, MN_lda, MN_tsx, MN_nop, MN_ldy, MN_lda, MN_ldx, MN_bbs
            .byte       MN_cpy, MN_cmp, MN_nop, MN_nop, MN_cpy, MN_cmp, MN_dec, MN_smb, MN_iny, MN_cmp, MN_dex, MN_wai, MN_cpy, MN_cmp, MN_dec, MN_bbs
            .byte       MN_bne, MN_cmp, MN_cmp, MN_nop, MN_nop, MN_cmp, MN_dec, MN_smb, MN_cld, MN_cmp, MN_phx, MN_stp, MN_nop, MN_cmp, MN_dec, MN_bbs
            .byte       MN_cpx, MN_sbc, MN_nop, MN_nop, MN_cpx, MN_sbc, MN_inc, MN_smb, MN_inx, MN_sbc, MN_nop, MN_nop, MN_cpx, MN_sbc, MN_inc, MN_bbs
            .byte       MN_beq, MN_sbc, MN_sbc, MN_nop, MN_nop, MN_sbc, MN_inc, MN_smb, MN_sed, MN_sbc, MN_plx, MN_nop, MN_nop, MN_sbc, MN_inc, MN_bbs

mn_modes:
            .byte       $88, $33, $08, $EE                  ; %0000yyz0
            .byte       $B1, $53, $08, $4E                  ; %0001yyz0
            .byte       $8E, $33, $08, $EE                  ; %0010yyz0
            .byte       $B1, $55, $08, $44                  ; %0011yyz0
            .byte       $88, $38, $08, $EE                  ; %0100yyz0
            .byte       $B1, $58, $88, $48                  ; %0101yyz0
            .byte       $88, $33, $08, $EA                  ; %0110yyz0
            .byte       $B1, $55, $88, $4C                  ; %0111yyz0
            .byte       $81, $33, $88, $EE                  ; %1000yyz0
            .byte       $B1, $75, $88, $4E                  ; %1001yyz0
            .byte       $99, $33, $88, $EE                  ; %1010yyz0
            .byte       $B1, $75, $88, $64                  ; %1011yyz0
            .byte       $89, $33, $88, $EE                  ; %1100yyz0
            .byte       $B1, $58, $88, $48                  ; %1101yyz0
            .byte       $89, $33, $88, $EE                  ; %1110yyz0
            .byte       $B1, $58, $88, $48                  ; %1111yyz0
            .byte       $8D, $33, $89, $2E                  ; %xxx0yyz1
            .byte       $8F, $35, $86, $24                  ; %xxx1yyz1

mn_str:     .byte       "txsedextrbvseibrasldywaincmplylsroldandeypladclcldxrortsbcstyatsxstxabplxstaxjsrtibminxbitaynophxbvcpybbstzbneorabccpxdeclvstphpharmbeqseclinysmbbrkjmplphy"
