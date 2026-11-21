; ****************************************************************************
; xsjumpa.s - (dq0harn) xsjump.c's matrix applied, in assembly (speed8, 2026-11-21: its C some 30,000 cycles a foe
; felled, the referee's, a line waiting on it): the generator's state (xorshift.s's xs_seed) through one of xs_cols'
; matrices, the XOR of the columns of its set bits, bit 0 of byte 0 the first column.  Overlay 10's, beside xs_jump.
;
;   void __fastcall__ xs_apply (const unsigned char* col);   col: 32 columns of 4 bytes, low byte first
; ****************************************************************************

            .setcpu     "65C02"
            .export     _xs_apply
            .import     _xs_seed
            .importzp   ptr1, tmp1, tmp2, tmp3, tmp4, sreg

            .segment    "OV10CODE"

_xs_apply:
            sta         ptr1
            stx         ptr1 + 1
            stz         tmp1                    ; tmp1-tmp4: the new state, low byte first
            stz         tmp2
            stz         tmp3
            stz         tmp4
            ldx         #0                      ; (The state's byte)
            ldy         #0                      ; (The column's first byte)
@byte:      lda         _xs_seed,x
            sta         sreg                    ; (Its bits, shifted out low first)
            lda         #8
            sta         sreg + 1
@bit:       lsr         sreg
            bcc         @skip
            lda         (ptr1),y
            eor         tmp1
            sta         tmp1
            iny
            lda         (ptr1),y
            eor         tmp2
            sta         tmp2
            iny
            lda         (ptr1),y
            eor         tmp3
            sta         tmp3
            iny
            lda         (ptr1),y
            eor         tmp4
            sta         tmp4
            iny
            dec         sreg + 1
            bne         @bit
            bra         @next
@skip:      iny
            iny
            iny
            iny
            dec         sreg + 1
            bne         @bit
@next:      inx
            cpx         #4
            bne         @byte
            lda         tmp1
            sta         _xs_seed
            lda         tmp2
            sta         _xs_seed + 1
            lda         tmp3
            sta         _xs_seed + 2
            lda         tmp4
            sta         _xs_seed + 3
            rts
