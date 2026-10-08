; ****************************************************************************
; nmpow.inc - POW, whole powers exactly (and n_intof, an integer in a register): nmconv.inc's once, a file of its own
; so that the math library has it too (its RPOW's whole powers: n_powxy) without the conversions.

.ifndef NM_COPY
.segment "NUMBANK"
n_e:        .res        2                                   ; (POW: its exponent's magnitude, its length, sign, bit)
n_el:       .res        1
n_es:       .res        1
n_eb:       .res        2
n_e0:       .res        1                                   ; (An exponent of -64 to 63: its magnitude, here)
n_r:        .res        2                                   ; (POW's r and b; where they're kept, and its work)
n_b:        .res        2
n_slot:     .res        2
n_work:     .res        2

.endif
            NM_CODE

; ---- Integers

; Register .Y = the number .A, .X, an integer (or one equal to one: a fixed decimal whose digits end in as many 0s
; as it has places).  NE_INT: it isn't one
n_intof:
            sty         nrc
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIXED
            bne         @not
            lda         n_v
            ldx         n_v + 1
            jsr         n_first
            jsr         r_load
@place:
            lda         n_pl                                ; (Each place: a 0 its digits end in, dropped)
            ora         n_pl + 1
            beq         @rts
            lda         #10
            jsr         r_divsmall
            bne         @not
            lda         n_pl
            bne         :+
            dec         n_pl + 1
:
            dec         n_pl
            bra         @place
@int:
            lda         n_v
            ldx         n_v + 1
            jmp         r_load
@not:
            lda         #NE_INT
            jmp         nm_fail
@rts:
            rts

; POW: r0 = x, r1 = n, an integer; x^n at r2, exactly (a negative n: 1 / x^-n), as numref.js's: r = 1, b = x; for each
; of n's bits from its lowest, r = r * b if it's 1, and b = b * b if there are more.  r and b are kept in two slots
; in the arena after the operands, each product made after them and copied in.  NE_DIV0: 0 to a power below 0
nm_pow:
            jsr         nm_begin
            jsr         nm_in2
; n_x to the power n_y (an integer, or a number equal to one), the call's result (math's RPOW's whole powers)
n_powxy:
            lda         n_y                                 ; (n, in its one form: its magnitude's bytes)
            ldx         n_y + 1
            ldy         #0
            jsr         n_intof
            lda         #0
            sta         nrc
            jsr         r_store
            jsr         n_mag
            LDAX        n_one                               ; (r = 1, b = x)
            sta         n_r
            stx         n_r + 1
            MOVR        n_b, n_x
            MOVR        n_slot, a_top                       ; (The slots: r's, then b's; then the work)
            lda         a_top
            clc
            adc         #<(2 * NUM_MAX)
            sta         n_work
            lda         a_top + 1
            adc         #>(2 * NUM_MAX)
            sta         n_work + 1
            cmp         #>ARENA_END
            bcc         :+
            jmp         n_big
:
            stz         n_eb
            stz         n_eb + 1
@bit:
            jsr         n_ebit                              ; (C: n's bit n_eb; Z: none past it)
            php
            bcc         @square
            MOVR        n_x, n_r                            ; (r = r * b)
            MOVR        n_y, n_b
            MOVR        a_top, n_work
            lda         #OP_MUL
            jsr         n_op
            lda         n_slot                              ; (Kept in r's slot)
            ldx         n_slot + 1
            sta         n_r
            stx         n_r + 1
            jsr         n_keep
@square:
            plp
            beq         @made
            MOVR        n_x, n_b                            ; (b = b * b)
            MOVR        n_y, n_b
            MOVR        a_top, n_work
            lda         #OP_MUL
            jsr         n_op
            lda         n_slot                              ; (Kept in b's slot)
            clc
            adc         #<NUM_MAX
            sta         n_b
            lda         n_slot + 1
            adc         #>NUM_MAX
            sta         n_b + 1
            tax
            lda         n_b
            jsr         n_keep
            inc         n_eb
            bne         :+
            inc         n_eb + 1
:
            jmp         @bit
@made:
            lda         n_es                                ; (n below 0: 1 / r)
            bne         :+
            lda         n_r
            ldx         n_r + 1
            jmp         nm_out
:
            MOVR        a_top, n_work
            lda         n_r
            ldx         n_r + 1
            jsr         n_zero_p
            bne         :+
            lda         #NE_DIV0
            jmp         nm_fail
:
            MOVR        n_y, n_r
            LDAX        n_one
            sta         n_x
            stx         n_x + 1
            lda         #OP_DIV
            jsr         n_op
            jmp         nm_out

; The number hv copied to .A, .X (a slot, below it in the arena)
n_keep:
            sta         nu
            stx         nu + 1
            MOVR        nq, hv
            lda         hv
            ldx         hv + 1
            jsr         n_size
            sta         n_t
            stx         n_t + 1
            jmp         a_copy

; The integer value .A, .X (in its one form): n_e = its magnitude's bytes' address, n_el their count, n_es its sign
; ($FF negative)
n_mag:
            sta         nt
            stx         nt + 1
            stz         n_es
            lda         (nt)
            cmp         #NT_POS
            bcs         @big
            cmp         #$40                                ; (-64 to 63: its magnitude in n_e0)
            bcc         :+
            dec         n_es
            eor         #$7F
            inc         a
:
            sta         n_e0
            LDAX        n_e0
            sta         n_e
            stx         n_e + 1
            lda         #1
            sta         n_el
            rts
@big:
            ldy         #1
            cmp         #NT_LONG_POS
            bcs         @long
            pha
            and         #$0F
            inc         a
            sta         n_el
            pla
            and         #NT_NEG - NT_POS
            beq         @at
            dec         n_es
            bra         @at
@long:
            and         #1
            beq         :+
            dec         n_es
:
            lda         (nt),y
            sta         n_el
            iny
@at:
            tya
            clc
            adc         nt
            sta         n_e
            lda         nt + 1
            adc         #0
            sta         n_e + 1
            rts

; n_e's bit n_eb.  OUT: C = it; Z = 1 if no bit past it is 1 (it n_e's last; its top byte isn't 0)
n_ebit:
            MOVR        nt, n_e
            lda         n_eb                                ; (Its byte, n_eb / 8, and the bit in it)
            and         #7
            tax
            lda         n_eb + 1
            sta         n_t
            lda         n_eb
            lsr         n_t
            ror
            lsr         n_t
            ror
            lsr         n_t
            ror
            tay
            lda         n_t
            bne         @none
            cpy         n_el
            bcs         @none
            lda         (nt),y                              ; (Bits past it: in its byte, or bytes past it)
            and         bit_above,x
            sta         n_t + 1
            iny
            cpy         n_el
            bcs         :+
            lda         #1
            sta         n_t + 1
:
            dey
            lda         (nt),y                              ; (C: it)
            and         bit_mask,x
            cmp         #1
            lda         n_t + 1
            rts
@none:
            clc
            lda         #0
            rts

            NM_RODATA
n_one:      .byte       $01                                 ; (The number 1)
bit_mask:   .byte       $01, $02, $04, $08, $10, $20, $40, $80
bit_above:  .byte       $FE, $FC, $F8, $F0, $E0, $C0, $80, $00
            NM_CODE
