; ****************************************************************************
; nmarith.inc - the arithmetic's entries: ADD, SUB, MUL, DIV (the tower, nmval.inc), NEG, ABS, CMP, KIND; ARITH, any
; of ADD ... CMP with its operands read where they are, in RAM banks of the caller's (NUMSPEED.md's As built, "A
; cheaper call"); and the quick way an operand comes in, for these (nm_inq).

.segment "NUMBANK"
v_ob:       .res        2                                   ; The banks x and y are read from, at $8000-$9FFF
            .code

; ADD, SUB, MUL, DIV: r0 = x, r1 = y; the result at r2 (room r3).  NE_DIV0: y is 0
nm_add:
            ldy         #OP_ADD
            bra         nm_arith
nm_sub:
            ldy         #OP_SUB
            bra         nm_arith
nm_mul:
            ldy         #OP_MUL
            bra         nm_arith
nm_div:
            ldy         #OP_DIV
nm_arith:
            jsr         nm_begin
            sty         n_opc
            lda         v_bank                              ; (Both read from the caller's own bank)
            sta         v_ob
            sta         v_ob + 1
nm_arith2:
            jsr         nm_inq2
            lda         n_opc                               ; (Two small numbers: their own way, nmsmall.inc)
            jsr         s_arith
            bcs         :+
            lda         hv                                  ; (Its length: the arena's, the last made)
            sta         nq
            ldx         hv + 1
            stx         nq + 1
            sec
            lda         a_top
            sbc         hv
            sta         v_len
            lda         a_top + 1
            sbc         hv + 1
            sta         v_len + 1
            jmp         nm_outl
:
            lda         n_opc
            cmp         #OP_DIV
            bne         @op
            lda         n_y
            ldx         n_y + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @op
            lda         n_y
            ldx         n_y + 1
            jsr         n_zero_p
            bne         @op
            lda         #NE_DIV0
            jmp         nm_fail
@op:
            lda         n_opc
            jsr         n_op
            jmp         nm_out

; NEG: r0 = x; -x at r2
nm_neg:
            jsr         nm_begin
            jsr         nm_in1
            jsr         n_neg
            jmp         nm_out

; ABS: r0 = x, a real number; |x| at r2.  NE_REAL: it's complex
nm_abs:
            jsr         nm_begin
            jsr         nm_in1
            jsr         n_kind
            cmp         #NK_COMPLEX
            bne         :+
            lda         #NE_REAL
            jmp         nm_fail
:
            jsr         n_rsign
            cmp         #$FF
            bne         :+
            jsr         n_rneg
            jmp         nm_out
:
            lda         n_x
            ldx         n_x + 1
            jmp         nm_out

; CMP: r0 = x, r1 = y.  OUT: .A = $FF, 0 or 1 (x less, the same, more)
nm_cmp:
            jsr         nm_begin
            lda         v_bank                              ; (Both read from the caller's own bank)
            sta         v_ob
            sta         v_ob + 1
nm_cmp2:
            jsr         nm_inq2
            jsr         s_cmp                               ; (Two small numbers: their own way)
            bcc         :+
            jsr         n_cmp
:
            ldx         #0
            clc
            jmp         nm_end

; KIND: r0 = x.  OUT: .A = its kind (NK_), .X = its sign ($FF, 0, 1: a complex number's real part's)
nm_kind:
            jsr         nm_begin
            jsr         nm_in1
            jsr         n_kind
            pha
            lda         n_x
            ldx         n_x + 1
            jsr         n_parts
            sta         n_x
            stx         n_x + 1
            jsr         n_rsign
            tax
            pla
            clc
            jmp         nm_end

; ARITH: .Y = the operation (0 ADD, 1 SUB, 2 MUL, 3 DIV, 4 CMP), r0 = x, r1 = y, each read in the RAM bank r4 (x's)
; or r4 + 1 (y's) gives it if it's at $8000-$9FFF (0: the caller's, as the others); the result at r2 (room r3), in the
; caller's own, as ADD's ... (CMP: .A its order).  An operand in a bank not the caller's must be within its window
; ($9FFF its last byte at most) and 255 bytes long at most: else NE_NOTNUM
nm_arith_far:
            jsr         nm_begin
            cpy         #OP_CMP + 1
            bcc         :+
            lda         #NE_NOTNUM
            jmp         nm_fail
:
            sty         n_opc
            ldx         #1
:
            lda         r4,x
            bne         :+
            lda         v_bank
:
            sta         v_ob,x
            dex
            bpl         :--
            cpy         #OP_CMP
            bne         :+
            jmp         nm_cmp2
:
            jmp         nm_arith2

; n_x = the caller's r0 (its copy), n_y = its r1: nm_inq's
nm_inq2:
            lda         r0
            ldx         r0 + 1
            ldy         v_ob
            jsr         nm_inq
            sta         n_x
            stx         n_x + 1
            lda         r1
            ldx         r1 + 1
            ldy         v_ob + 1
            jsr         nm_inq
            sta         n_y
            stx         n_y + 1
            rts

; The caller's number at .A, .X (in bank .Y, if it's at $8000-$9FFF) copied to the arena, as nm_in: below $7E00, or
; in that bank, measured by its tags (q_num) in one pass and copied in another (from the bank 8 bytes at a time,
; through na-nd: two bank switches for each 8); else nm_in's way.  OUT: .A, .X = its copy (a value); v_len its length
nm_inq:
            sta         np
            stx         np + 1
            cpx         #$7E
            bcs         @high
            jsr         q_num
            bcs         @slow
            sty         v_len                               ; (In the RAM: copied straight, from its end)
            stz         v_len + 1
            sty         nt
            tya
            ldx         #0
            jsr         a_room
            ldy         nt
:
            dey
            lda         (np),y
            sta         (nu),y
            tya
            bne         :-
            lda         nt
            clc
            adc         nu
            sta         nu
            bcc         :+
            inc         nu + 1
:
            jmp         a_made
@slow:
            lda         np
            ldx         np + 1
            jmp         nm_in
@bslow:
            lda         nq                                  ; (The caller's own bank: nm_in's way; another's: not one)
            cmp         v_bank
            beq         @slow
            lda         #NE_NOTNUM
            jmp         nm_fail
@high:
            cpx         #$80
            bcc         @slow
            cpx         #$A0
            bcs         @slow
            sty         nq                                  ; (In a bank: nq its bank)
            sty         RAM_BANK
            jsr         q_num
            lda         r13
            sta         RAM_BANK
            bcs         @bslow
            tya                                             ; (Its end in the window)
            clc
            adc         np
            tax
            lda         np + 1
            adc         #0
            cmp         #$A0
            bcc         :+
            bne         @bslow
            cpx         #0
            bne         @bslow
:
            sty         v_len
            stz         v_len + 1
            sty         nt + 1
            tya
            ldx         #0
            jsr         a_room
            ldy         nt + 1                              ; (32 bytes or fewer: through the stack, as nm_out's,
            cpy         #33                                 ;   its bank selected once)
            bcs         @long
            lda         nq
            sta         RAM_BANK
:
            dey
            lda         (np),y
            pha
            tya
            bne         :-
            lda         r13
            sta         RAM_BANK
            ldx         nt + 1
:
            pla
            sta         (nu),y
            iny
            dex
            bne         :-
            bra         @end
@long:
            stz         nt                                  ; (nt: the bytes copied; nt + 1 all of them)
@chunk:
            lda         nq
            sta         RAM_BANK
            ldy         nt
            ldx         #0
:
            lda         (np),y
            sta         na,x
            iny
            inx
            cpy         nt + 1
            beq         :+
            cpx         #8
            bne         :-
:
            lda         r13
            sta         RAM_BANK
            ldy         nt
            ldx         #0
:
            lda         na,x
            sta         (nu),y
            iny
            inx
            cpy         nt + 1
            beq         :+
            cpx         #8
            bne         :-
:
            sty         nt
            cpy         nt + 1
            bne         @chunk
@end:
            tya                                             ; (nu past it)
            clc
            adc         nu
            sta         nu
            bcc         :+
            inc         nu + 1
:
            jmp         a_made

; .Y = the length of the number at np (in the bank selected), its tags and lengths as nm_measure checks them: C = 0;
; C = 1 if they aren't a number's, or it's past 255 bytes (nm_measure's way then).  Modifies .A, nt
q_num:
            lda         (np)                                ; (An integer of 16 bytes or fewer: its tag alone)
            cmp         #NT_LONG_POS
            bcc         @int
            cmp         #NT_FIXED                           ; (A fixed decimal of 0-15 places, its digits so: their
            bcc         @other                              ;   tag, after its own)
            cmp         #NT_FIXED + 16
            bcs         @other
            ldy         #1
            lda         (np),y
            cmp         #NT_LONG_POS
            bcs         @other
            iny
            cmp         #NT_POS
            bcc         @rts
            and         #$0F                                ; (C = 1: + 3)
            adc         #2
            tay
            clc
            rts
@int:
            ldy         #1
            cmp         #NT_POS
            bcc         @rts                                ; (-64 to 63: C = 0)
            and         #$0F                                ; (1-16 bytes and the tag: C = 1, + 2)
            adc         #1
            tay
            clc
@rts:
            rts
@other:
            ldy         #0
            cmp         #NT_COMPLEX
            bne         q_real
            iny
            jsr         q_real
            bcc         q_real
            rts
; A real number's, at .Y: .Y past it
q_real:
            lda         (np),y
            cmp         #NT_FIXED
            bcc         q_int
            cmp         #NT_FIXED + 16
            bcc         @fix
            cmp         #NT_FIXED_LONG
            beq         @long
            cmp         #NT_RATIONAL
            bne         q_no
            iny
            beq         q_no
            jsr         q_int
            bcc         q_int
            rts
@long:
            tya                                             ; (Its tag and its places, then its digits)
            clc
            adc         #3
            tay
            bcs         q_no
            bra         q_int
@fix:
            iny
            beq         q_no
; An integer's, at .Y: .Y past it
q_int:
            lda         (np),y
            bpl         @one                                ; (-64 to 63: the tag alone)
            cmp         #NT_LONG_POS
            bcs         @long
            and         #$0F                                ; (1-16 bytes, and the tag)
            sec
            adc         #1
@add:
            sta         nt
            tya
            clc
            adc         nt
            tay
            rts                                             ; (C = 1: past 255 bytes)
@long:
            cmp         #NT_LONG_NEG + 1
            bcs         q_no
            iny                                             ; (Its length, 17 to 255; and it, and the length's byte)
            beq         q_no
            lda         (np),y
            cmp         #17
            bcc         q_no
            sta         nt
            tya
            sec
            adc         nt
            tay
            rts
@one:
            iny
            beq         q_no
            clc
            rts
q_no:
            sec
            rts
