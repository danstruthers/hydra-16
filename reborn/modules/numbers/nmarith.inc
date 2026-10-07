; ****************************************************************************
; nmarith.inc - the arithmetic's entries: ADD, SUB, MUL, DIV (the tower, nmval.inc), NEG, ABS, CMP, KIND.

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
            jsr         nm_in2
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
            jsr         nm_in2
            jsr         n_cmp
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
