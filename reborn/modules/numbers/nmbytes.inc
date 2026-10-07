; ****************************************************************************
; nmbytes.inc - BYTES: bytes checked, a number in the stored format in its one form (as numfmt.js's decode and
; danlang's bytes-number take them), and copied to the result's place.  Its one form: an integer in its shortest tag
; (-64 to 63 the tag alone; 1 to 16 bytes, then 17 to 255, its top byte not 0); a fixed decimal's places in their own
; tag below 16, its digits not ending in 0 when it has places; a rational's denominator above 1 (in lowest terms: with
; GCD); a complex number's imaginary part not 0.

nm_bytes:
            jsr         nm_begin
            lda         r0
            ldx         r0 + 1
            ldy         #0
            jsr         nm_in                               ; (num_a, its tags and lengths checked)
            lda         v_len                               ; (The bytes all the number)
            cmp         r1
            bne         f_bad
            lda         v_len + 1
            cmp         r1 + 1
            bne         f_bad
            LDR         nq, num_a
            lda         (nq)
            cmp         #NT_COMPLEX
            bne         @real
            jsr         f_next
            jsr         f_real
            lda         (nq)                                ; (Its imaginary part not 0: $00, or $B0 $00, 0.0)
            beq         f_bad
            cmp         #NT_FIXED
            bne         @im
            ldy         #1
            lda         (nq),y
            beq         f_bad
@im:
@real:
            jsr         f_real
            ldy         #0
            jmp         nm_out

f_bad:
            lda         #NE_NOTNUM
            jmp         nm_fail

; nq moved on a byte
f_next:
            inc         nq
            bne         :+
            inc         nq + 1
:
            rts

; nq moved on .A bytes
f_skip:
            clc
            adc         nq
            sta         nq
            bcc         :+
            inc         nq + 1
:
            rts

; The integer at nq in its one form, nq moved past it
f_int:
            lda         (nq)
            cmp         #NT_POS
            bcc         f_next                              ; (-64 to 63: the tag)
            jsr         f_bytes
            lda         (nq)
            cmp         #NT_LONG_POS                        ; (17 bytes or more: past the tag and the length)
            bcc         @short
            lda         #2
            jsr         f_skip
            dex
            txa
            tay
            lda         (nq),y                              ; (Its top byte not 0)
            beq         f_bad2
            iny
            tya
            jmp         f_skip
@short:
            cpx         #1
            bne         @top
            ldy         #1                                  ; (One byte: 64 or more; -65 or less)
            lda         (nq),y
            cmp         #64
            bcc         f_bad2
            bne         @top
            lda         (nq)
            cmp         #NT_NEG
            bcs         f_bad2
@top:
            txa
            tay
            lda         (nq),y                              ; (Its top byte not 0)
            beq         f_bad2
            iny
            tya
            jmp         f_skip
f_bad2:
            jmp         f_bad

; .X = the magnitude's bytes of the integer at nq (its tag $80 on)
f_bytes:
            lda         (nq)
            cmp         #NT_LONG_POS
            bcs         :+
            and         #$0F
            tax
            inx
            rts
:
            ldy         #1
            lda         (nq),y
            tax
            rts

; The real number at nq in its one form, nq moved past it (its tags and lengths are right: nm_measure's)
f_real:
            lda         (nq)
            cmp         #NT_FIXED
            bcc         f_int                               ; (An integer)
            cmp         #NT_FIXED + 16
            bcs         @long
            jsr         f_next
            and         #$0F                                ; (0 places: its digits any integer)
            beq         f_int
            bra         f_digits
@long:
            cmp         #NT_FIXED_LONG
            bne         @rat
            ldy         #2                                  ; (16 places or more)
            lda         (nq),y
            bne         :+
            dey
            lda         (nq),y
            cmp         #16
            bcc         f_bad2
:
            lda         #3
            jsr         f_skip
            bra         f_digits
@rat:
            jsr         f_next                              ; (A rational: nm_measure let no other tag by)
            lda         (nq)                                ; (Its numerator not 0)
            beq         f_bad2
            jsr         f_int
            lda         (nq)                                ; (Its denominator above 1: 2-63, or more bytes, positive)
            cmp         #2
            bcc         f_bad2
            cmp         #$40
            bcc         @int
            cmp         #NT_POS
            bcc         f_bad2
            cmp         #NT_NEG
            bcc         @int
            cmp         #NT_LONG_POS
            bne         f_bad2
@int:
            jmp         f_int

; A fixed decimal's digits at nq (it has places), an integer in its one form not ending in 0, nq moved past it
f_digits:
            lda         (nq)
            cmp         #NT_POS
            bcs         @big
            and         #$7F                                ; (-64 to 63: its magnitude, not 0 or 10's ...)
            cmp         #$40
            bcc         :+
            eor         #$7F
            inc         a
:
            cmp         #0
            beq         @bad
:
            cmp         #10
            bcc         :+
            sbc         #10
            bra         :-
:
            cmp         #0
            beq         @bad
            jmp         f_next
@big:
            ldy         #1                                  ; (Its magnitude's first byte: after the tag, or the length)
            cmp         #NT_LONG_POS
            bcc         :+
            iny
:
            lda         (nq),y                              ; (Even, and 0 mod 5: ends in 0)
            and         #1
            bne         @int
            MOVR        nt, nq                              ; (Mod 5: mod 255's, its bytes summed, the carry around)
            tya
            clc
            adc         nt
            sta         nt
            bcc         :+
            inc         nt + 1
:
            jsr         f_bytes                             ; (.X: how many)
            lda         #0
            clc
:
            adc         (nt)
            inc         nt
            bne         :+
            inc         nt + 1
:
            dex
            bne         :--
            adc         #0                                  ; (The last carry around, and its own: $FF + 1)
            adc         #0
:
            cmp         #5
            bcc         :+
            sbc         #5
            bra         :-
:
            cmp         #0
            beq         @bad
@int:
            jmp         f_int
@bad:
            jmp         f_bad
