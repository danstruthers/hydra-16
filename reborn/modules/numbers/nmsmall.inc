; ****************************************************************************
; nmsmall.inc - small numbers' own way (docs/design/plans/NUMSPEED.md, step 3): ADD, SUB, MUL, DIV and CMP of two
; numbers each an integer of 4 bytes or fewer or a fixed decimal of 0-15 places whose digits are, worked here in 64-bit
; words, not in the registers.  The results are the tower's (nmval.inc): a sum, difference or product of two integers
; an integer, of a fixed decimal and either a fixed decimal (a sum's places the more of the two's, a product's both's,
; the 0s at its digits' end dropped); a quotient an integer if it's whole, else a rational in lowest terms.  Anything
; past 64 bits on the way (32 for a quotient's two parts), or any other kind: C = 1, the tower's way.

.ifndef NM_COPY
.segment "NUMBANK"
s_ma:       .res        8                                   ; x and y: their magnitudes (64 bits, least first) ...
s_mb:       .res        8
s_na:       .res        1                                   ;   their signs ($FF: below 0) ...
s_nb:       .res        1
s_pa:       .res        1                                   ;   and their places
s_pb:       .res        1
s_fx:       .res        1                                   ; Either a fixed decimal (1)
s_mr:       .res        8                                   ; The result: its magnitude, its sign, its places
s_nr:       .res        1
s_pr:       .res        1
s_t:        .res        8                                   ; (Scratch: a magnitude, a quotient)
s_rm:       .res        4                                   ; (A division's remainder)
s_i:        .res        1                                   ; (Indices, lengths, counts)
s_j:        .res        1
s_la:       .res        1
s_lb:       .res        1
s_k:        .res        1
f_n:        .res        4                                   ; (Fractions: x's numerator and y's (16 bits each) ...
f_d:        .res        4                                   ;   and their denominators ...
f_p:        .res        4                                   ;   x's numerator times y's denominator, y's times x's
f_q:        .res        4                                   ;   (32 bits))
f_u:        .res        2                                   ; (s_m16's factors, its product)
f_v:        .res        2
f_w:        .res        4
f_sgn:      .res        1                                   ; (f_int16's sign)
.endif
            NM_CODE

; ---- The two operands

; n_x and n_y (values) as x and y: C = 0; C = 1 if either isn't small
s_both:
            stz         s_fx
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            jsr         s_load
            bcc         :+
            rts
:
            lda         n_y
            ldx         n_y + 1
            ldy         #1
; The number at .A, .X as x (.Y = 0) or y (1): C = 0; C = 1 if it isn't small
s_load:
            sta         nt
            stx         nt + 1
            tya
            tax                                             ; (.X: its scalars' index)
            asl
            asl
            asl
            tay                                             ; (.Y: its magnitude's offset)
            stz         s_na,x
            stz         s_pa,x
            lda         #0
            sta         s_ma,y
            sta         s_ma + 1,y
            sta         s_ma + 2,y
            sta         s_ma + 3,y
            sta         s_ma + 4,y
            sta         s_ma + 5,y
            sta         s_ma + 6,y
            sta         s_ma + 7,y
            lda         (nt)
            cmp         #NT_FIXED
            bcc         @int
            cmp         #NT_FIXED + 16
            bcs         @no
            and         #$0F                                ; (A fixed decimal of 0-15 places: its digits after)
            sta         s_pa,x
            lda         #1
            sta         s_fx
            inc         nt
            bne         @int
            inc         nt + 1
@int:
            lda         (nt)
            cmp         #NT_POS
            bcs         @tagged
            cmp         #$40                                ; (-64 to 63: the tag itself)
            bcc         :+
            eor         #$7F
            inc         a
            dec         s_na,x
:
            sta         s_ma,y
            clc
            rts
@tagged:
            cmp         #NT_POS + 4                         ; (1-4 bytes)
            bcc         @bytes
            cmp         #NT_NEG
            bcc         @no
            cmp         #NT_NEG + 4
            bcs         @no
            dec         s_na,x
@bytes:
            and         #$03
            clc
            adc         #2
            sta         s_k                                 ; (Its bytes and 1: the copy's end)
            tya
            tax
            ldy         #1
:
            lda         (nt),y
            sta         s_ma,x
            inx
            iny
            cpy         s_k
            bne         :-
            clc
@rts:
            rts
@no:
            sec
            rts

; ---- Magnitudes

; arr = arr * 10 (8 bytes): C = 0; C = 1 if it might pass 64 bits (its top byte $19 or more)
.macro S_X10    arr
            lda         arr + 7
            cmp         #$19
            bcs         :+++                                ; (Its end: no label to end the callers' locals)
            asl         arr                                 ; (* 2: s_t)
            rol         arr + 1
            rol         arr + 2
            rol         arr + 3
            rol         arr + 4
            rol         arr + 5
            rol         arr + 6
            rol         arr + 7
            ldx         #7
:
            lda         arr,x
            sta         s_t,x
            dex
            bpl         :-
            asl         arr                                 ; (* 8, then the * 2 added)
            rol         arr + 1
            rol         arr + 2
            rol         arr + 3
            rol         arr + 4
            rol         arr + 5
            rol         arr + 6
            rol         arr + 7
            asl         arr
            rol         arr + 1
            rol         arr + 2
            rol         arr + 3
            rol         arr + 4
            rol         arr + 5
            rol         arr + 6
            rol         arr + 7
            ldx         #0
            ldy         #8
            clc
:
            lda         arr,x
            adc         s_t,x
            sta         arr,x
            inx
            dey
            bne         :-
            clc
:
.endmacro

; x's magnitude times 10, .A times (C = 1: past 64 bits)
s_scale_a:
            sta         s_k
@ten:
            lda         s_k
            beq         @done
            S_X10       s_ma
            bcs         @rts
            dec         s_k
            bra         @ten
@done:
            clc
@rts:
            rts

; y's
s_scale_b:
            sta         s_k
@ten:
            lda         s_k
            beq         @done
            S_X10       s_mb
            bcs         @rts
            dec         s_k
            bra         @ten
@done:
            clc
@rts:
            rts

; x and y at the same places (the more of the two's: s_pr): C = 1 if a magnitude would pass 64 bits
s_align:
            lda         s_pa
            sec
            sbc         s_pb
            beq         @same
            bcc         @a                                  ; (x has fewer: x scaled)
            ldx         s_pa                                ; (y has fewer: y scaled)
            stx         s_pr
            jmp         s_scale_b
@a:
            eor         #$FF
            inc         a
            ldx         s_pb
            stx         s_pr
            jmp         s_scale_a
@same:
            lda         s_pa
            sta         s_pr
            clc
            rts

; .A = x's magnitude against y's: $FF less, 0 the same, 1 more
s_ucmp:
            ldx         #7
:
            lda         s_ma,x
            cmp         s_mb,x
            bne         @order
            dex
            bpl         :-
            lda         #0
            rts
@order:
            lda         #1
            bcs         :+
            lda         #$FF
:
            rts

; ---- The operations

; hv = n_x .A n_y (OP_ADD, OP_SUB, OP_MUL, OP_DIV) if both are small: C = 0; else C = 1
s_arith:
            sta         s_j
            jsr         s_both
            bcc         :+
            jmp         s_frac                              ; (Not two of those: fractions, perhaps)
:
            lda         s_j
            cmp         #OP_MUL
            bne         :+
            jmp         s_mul
:
            bcc         :+
            jmp         s_div
:
            cmp         #OP_SUB                             ; (A difference: y's sign the other way)
            bne         :+
            lda         s_nb
            eor         #$FF
            sta         s_nb
:
            jsr         s_align
            bcs         @rts
            lda         s_na
            cmp         s_nb
            bne         @differ
            sta         s_nr                                ; (The same signs: the magnitudes added)
            ldx         #0
            ldy         #8
            clc
:
            lda         s_ma,x
            adc         s_mb,x
            sta         s_mr,x
            inx
            dey
            bne         :-
            bcs         @rts                                ; (Past 64 bits: the tower's)
            jmp         s_made
@differ:
            jsr         s_ucmp                              ; (Signs that differ: the less from the more, its sign)
            cmp         #$FF
            beq         @yx
            lda         s_na
            sta         s_nr
            ldx         #0
            ldy         #8
            sec
:
            lda         s_ma,x
            sbc         s_mb,x
            sta         s_mr,x
            inx
            dey
            bne         :-
            jmp         s_made
@yx:
            lda         s_nb
            sta         s_nr
            ldx         #0
            ldy         #8
            sec
:
            lda         s_mb,x
            sbc         s_ma,x
            sta         s_mr,x
            inx
            dey
            bne         :-
            jmp         s_made
@rts:
            rts

; A product: x's magnitude times y's (32 by 32 bits), the places added
s_mul:
            lda         s_pa
            clc
            adc         s_pb
            cmp         #16                                 ; (16 places or more: the tower's)
            bcc         :+
            rts
:
            sta         s_pr
            lda         s_na
            eor         s_nb
            sta         s_nr
            ldx         #7
:
            stz         s_mr,x
            dex
            bpl         :-
            ldx         #4                                  ; (y's bytes: those but its top 0s)
:
            dex
            beq         :+
            lda         s_mb,x
            beq         :-
:
            inx
            stx         s_lb
            ldx         #4                                  ; (x's)
:
            dex
            beq         :+
            lda         s_ma,x
            beq         :-
:
            inx
            stx         s_la
            stz         s_i
@row:
            ldx         s_i
            lda         s_ma,x
            beq         @next
            sta         n_m8
            stz         n_cy
            ldy         #0
@col:
            lda         s_mb,y
            jsr         mul8                                ; (.A its product's low byte, n_phi its high; .Y kept)
            clc
            adc         n_cy
            bcc         :+
            inc         n_phi
:
            sta         s_k
            tya
            clc
            adc         s_i
            tax
            lda         s_k
            clc
            adc         s_mr,x
            sta         s_mr,x
            lda         n_phi
            adc         #0
            sta         n_cy
            iny
            cpy         s_lb
            bne         @col
            lda         n_cy
            sta         s_mr + 1,x
@next:
            inc         s_i
            lda         s_i
            cmp         s_la
            bne         @row
            jmp         s_made

; A quotient: (x's magnitude * 10^y's places) / (y's * 10^x's), each 32 bits at most, in lowest terms (their greatest
; common divisor, the binary way), its sign the two's; y 0: the tower's (its error)
s_div:
            lda         s_mb
            ora         s_mb + 1
            ora         s_mb + 2
            ora         s_mb + 3
            bne         :+
            sec
            rts
:
            lda         s_pb
            jsr         s_scale_a
            bcs         @rts
            lda         s_pa
            jsr         s_scale_b
            bcs         @rts
            lda         s_ma + 4                            ; (Past 32 bits: the tower's)
            ora         s_ma + 5
            ora         s_ma + 6
            ora         s_ma + 7
            ora         s_mb + 4
            ora         s_mb + 5
            ora         s_mb + 6
            ora         s_mb + 7
            beq         :+
            sec
@rts:
            rts
:
            lda         s_na
            eor         s_nb
            sta         s_nr
; hv = s_ma / s_mb (32 bits each, s_mb not 0), its sign s_nr, in lowest terms: an integer if it's whole, else a
; rational: C = 0
s_ratio:
            stz         s_fx                                ; (A quotient's no fixed decimal)
            stz         s_pr
            jsr         s_gcd                               ; (Their divisor: s_t; 1: as they are)
            lda         s_t + 1
            ora         s_t + 2
            ora         s_t + 3
            bne         @divide
            lda         s_t
            cmp         #1
            beq         @lowest
@divide:
            ldx         #s_ma - s_ma                        ; (Each divided by it)
            jsr         s_div32
            ldx         #s_mb - s_ma
            jsr         s_div32
@lowest:
            ldx         #3                                  ; (The numerator: the result's magnitude)
:
            lda         s_ma,x
            sta         s_mr,x
            stz         s_mr + 4,x
            dex
            bpl         :-
            lda         s_mb + 1                            ; (Whole: an integer)
            ora         s_mb + 2
            ora         s_mb + 3
            bne         @rational
            lda         s_mb
            cmp         #1
            bne         @rational
            jmp         s_made
@rational:
            lda         s_mr                                ; (0 over anything is 0: the integer)
            ora         s_mr + 1
            ora         s_mr + 2
            ora         s_mr + 3
            bne         :+
            jmp         s_made
:
            lda         #12                                 ; (Its tag, the numerator, the denominator)
            ldx         #0
            jsr         a_room
            lda         #NT_RATIONAL
            jsr         a_put
            jsr         s_wint
            ldx         #3
:
            lda         s_mb,x
            sta         s_mr,x
            dex
            bpl         :-
            stz         s_nr
            jsr         s_wint
            jsr         a_made
            clc
            rts

; s_t = the greatest common divisor of x's and y's magnitudes (32 bits; y not 0): the binary way
s_gcd:
            ldx         #3                                  ; (u: s_t = x, v: s_mr = y)
:
            lda         s_ma,x
            sta         s_t,x
            lda         s_mb,x
            sta         s_mr,x
            dex
            bpl         :-
            lda         s_t
            ora         s_t + 1
            ora         s_t + 2
            ora         s_t + 3
            bne         :++
            ldx         #3                                  ; (u 0: v)
:
            lda         s_mr,x
            sta         s_t,x
            dex
            bpl         :-
            rts
:
            stz         s_k                                 ; (The twos both have: out, counted)
@twos:
            lda         s_t
            ora         s_mr
            lsr
            bcs         @odd
            jsr         s_shr_t
            jsr         s_shr_r
            inc         s_k
            bra         @twos
@odd:
            lda         s_t                                 ; (u odd)
            lsr
            bcs         @loop
            jsr         s_shr_t
            bra         @odd
@loop:
            lda         s_mr                                ; (v odd)
            lsr
            bcs         :+
            jsr         s_shr_r
            bra         @loop
:
            ldx         #3                                  ; (u the less: swapped if it's more)
:
            lda         s_t,x
            cmp         s_mr,x
            bne         :+
            dex
            bpl         :-
            bra         @sub                                ; (The same)
:
            bcc         @sub
            ldx         #3
:
            lda         s_t,x
            ldy         s_mr,x
            sta         s_mr,x
            tya
            sta         s_t,x
            dex
            bpl         :-
@sub:
            ldx         #0                                  ; (v = v - u)
            ldy         #4
            sec
:
            lda         s_mr,x
            sbc         s_t,x
            sta         s_mr,x
            inx
            dey
            bne         :-
            lda         s_mr
            ora         s_mr + 1
            ora         s_mr + 2
            ora         s_mr + 3
            bne         @loop
@shl:
            lda         s_k                                 ; (u times 2^k)
            beq         @done
            asl         s_t
            rol         s_t + 1
            rol         s_t + 2
            rol         s_t + 3
            dec         s_k
            bra         @shl
@done:
            rts

s_shr_t:
            lsr         s_t + 3
            ror         s_t + 2
            ror         s_t + 1
            ror         s_t
            rts
s_shr_r:
            lsr         s_mr + 3
            ror         s_mr + 2
            ror         s_mr + 1
            ror         s_mr
            rts

; The 32-bit magnitude at s_ma + .X (x's or y's) divided by s_t (exactly: a divisor of it), in place
s_div32:
            stx         s_j
            stz         s_rm
            stz         s_rm + 1
            stz         s_rm + 2
            stz         s_rm + 3
            lda         #32
            sta         s_i
@top:
            ldx         s_j                                 ; (Its top 0 bytes: a byte's 8 steps at once)
            lda         s_ma + 3,x
            bne         @bit
            lda         s_ma + 2,x
            sta         s_ma + 3,x
            lda         s_ma + 1,x
            sta         s_ma + 2,x
            lda         s_ma,x
            sta         s_ma + 1,x
            stz         s_ma,x
            lda         s_i
            sec
            sbc         #8
            sta         s_i
            bne         @top
            rts
@bit:
            ldx         s_j
            asl         s_ma,x
            rol         s_ma + 1,x
            rol         s_ma + 2,x
            rol         s_ma + 3,x
            rol         s_rm
            rol         s_rm + 1
            rol         s_rm + 2
            rol         s_rm + 3
            sec
            lda         s_rm
            sbc         s_t
            pha
            lda         s_rm + 1
            sbc         s_t + 1
            pha
            lda         s_rm + 2
            sbc         s_t + 2
            tay
            lda         s_rm + 3
            sbc         s_t + 3
            bcc         @no
            sta         s_rm + 3
            sty         s_rm + 2
            pla
            sta         s_rm + 1
            pla
            sta         s_rm
            inc         s_ma,x
            bra         @next
@no:
            pla
            pla
@next:
            dec         s_i
            bne         @bit
            rts

; .A = n_x against n_y if both are small: C = 0; else C = 1
s_cmp:
            jsr         s_both
            bcc         :+
            lda         #OP_CMP                             ; (Not two of those: fractions, perhaps)
            sta         s_j
            jmp         s_frac
:
            jsr         s_align
            bcs         @rts
            lda         s_na                                ; (Signs that differ: the negative one less, but 0's)
            cmp         s_nb
            beq         @same
            jsr         s_zeros
            beq         @equal
            lda         s_na
            bne         @less
            lda         #1
            clc
            rts
@same:
            jsr         s_ucmp                              ; (Both negative: the other way)
            ldx         s_na
            beq         :+
            eor         #$FF
            inc         a
:
            clc
@rts:
            rts
@less:
            lda         #$FF
            clc
            rts
@equal:
            lda         #0
            clc
            rts

; Z = 1 if x's and y's magnitudes are both 0
s_zeros:
            ldx         #7
            lda         #0
:
            ora         s_ma,x
            ora         s_mb,x
            dex
            bpl         :-
            cmp         #0
            rts

; ---- Small fractions: one a rational, each an integer, a fixed decimal of 0-4 places or a rational whose parts are
; 16 bits at most (a numerator and a denominator each)

OP_CMP          = 4                                         ; (s_frac's comparison)

; n_x .s_j n_y (OP_ADD, OP_SUB, OP_MUL, OP_DIV: hv; OP_CMP: .A, $FF, 0, 1), as fractions: C = 0; C = 1 if either isn't
; one of those, or neither is a rational, or a part passes 32 bits
f_no:
            sec
            rts
s_frac:
            stz         s_k                                 ; (A rational among them: s_k's bit 7)
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            jsr         f_load
            bcs         f_no
            lda         n_y
            ldx         n_y + 1
            ldy         #1
            jsr         f_load
            bcs         f_no
            lda         s_k
            bpl         f_no                                ; (No rational: not a fraction's to do)
            lda         s_j
            cmp         #OP_MUL
            bne         :+
            jmp         @mul
:
            cmp         #OP_DIV
            bne         :+
            jmp         @div
:
            jsr         f_cross                             ; (ADD, SUB, CMP: x's n times y's d, y's n times x's d)
            lda         s_j
            cmp         #OP_CMP
            bne         :+
            jmp         f_cmp
:
            cmp         #OP_SUB                             ; (A difference: y's sign the other way)
            bne         :+
            lda         s_nb
            eor         #$FF
            sta         s_nb
:
            lda         s_na                                ; (Its numerator: the two added, or the less from the
            cmp         s_nb                                ;   more, its sign)
            bne         @differ
            sta         s_nr
            clc
            ldx         #0
            ldy         #4
:
            lda         f_p,x
            adc         f_q,x
            sta         s_ma,x
            inx
            dey
            bne         :-
            bcc         :+
            jmp         @no                                 ; (Past 32 bits)
:
            jmp         @den
@differ:
            ldx         #3
:
            lda         f_p,x
            cmp         f_q,x
            bne         :+
            dex
            bpl         :-
:
            bcc         @qp
            lda         s_na
            sta         s_nr
            sec
            ldx         #0
            ldy         #4
:
            lda         f_p,x
            sbc         f_q,x
            sta         s_ma,x
            inx
            dey
            bne         :-
            bra         @den
@qp:
            lda         s_nb
            sta         s_nr
            sec
            ldx         #0
            ldy         #4
:
            lda         f_q,x
            sbc         f_p,x
            sta         s_ma,x
            inx
            dey
            bne         :-
@den:
            ldx         #0                                  ; (Its denominator: the two's)
            ldy         #2
            jsr         f_mulnd
            bra         @ratio
@mul:
            ldx         #0                                  ; (A product: the numerators', the denominators')
            ldy         #2
            jsr         f_mulnn
            ldx         #0
            ldy         #2
            jsr         f_mulnd
            bra         @sign
@div:
            lda         f_n + 2                             ; (A quotient: y 0, the tower's: its error)
            ora         f_n + 3
            beq         @no
            jsr         f_cross                             ; (x's n times y's d over y's n times x's d)
            ldx         #3
:
            lda         f_p,x
            sta         s_ma,x
            lda         f_q,x
            sta         s_mb,x
            dex
            bpl         :-
@sign:
            lda         s_na
            eor         s_nb
            sta         s_nr
@ratio:
            ldx         #3                                  ; (Their top halves 0: 32 bits)
:
            stz         s_ma + 4,x
            stz         s_mb + 4,x
            dex
            bpl         :-
            jmp         s_ratio
@no:
            sec
            rts

; f_p = x's numerator times y's denominator, f_q = y's numerator times x's denominator
f_cross:
            lda         f_n
            sta         f_u
            lda         f_n + 1
            sta         f_u + 1
            lda         f_d + 2
            sta         f_v
            lda         f_d + 3
            sta         f_v + 1
            jsr         s_m16
            ldx         #3
:
            lda         f_w,x
            sta         f_p,x
            dex
            bpl         :-
            lda         f_n + 2
            sta         f_u
            lda         f_n + 3
            sta         f_u + 1
            lda         f_d
            sta         f_v
            lda         f_d + 1
            sta         f_v + 1
            jsr         s_m16
            ldx         #3
:
            lda         f_w,x
            sta         f_q,x
            dex
            bpl         :-
            rts

; s_ma = the numerators' product (.X, .Y: 0 and 2), s_mb = the denominators' (f_mulnd)
f_mulnn:
            lda         f_n,x
            sta         f_u
            lda         f_n + 1,x
            sta         f_u + 1
            lda         f_n,y
            sta         f_v
            lda         f_n + 1,y
            sta         f_v + 1
            jsr         s_m16
            ldx         #3
:
            lda         f_w,x
            sta         s_ma,x
            dex
            bpl         :-
            rts
f_mulnd:
            lda         f_d,x
            sta         f_u
            lda         f_d + 1,x
            sta         f_u + 1
            lda         f_d,y
            sta         f_v
            lda         f_d + 1,y
            sta         f_v + 1
            jsr         s_m16
            ldx         #3
:
            lda         f_w,x
            sta         s_mb,x
            dex
            bpl         :-
            rts

; .A = x against y, f_p and f_q their cross products: $FF less, 0 the same, 1 more: C = 0
f_cmp:
            lda         s_na                                ; (Signs that differ: the negative one less, but 0's)
            cmp         s_nb
            beq         @same
            lda         f_p
            ora         f_p + 1
            ora         f_p + 2
            ora         f_p + 3
            ora         f_q
            ora         f_q + 1
            ora         f_q + 2
            ora         f_q + 3
            beq         @equal
            lda         s_na
            bne         @less
            lda         #1
            clc
            rts
@same:
            ldx         #3                                  ; (Their magnitudes; both negative, the other way)
:
            lda         f_p,x
            cmp         f_q,x
            bne         :+
            dex
            bpl         :-
            bra         @equal
:
            lda         #1
            bcs         :+
            lda         #$FF
:
            ldx         s_na
            beq         :+
            eor         #$FF
            inc         a
:
            clc
            rts
@less:
            lda         #$FF
            clc
            rts
@equal:
            lda         #0
            clc
            rts

; f_w = f_u * f_v (16 by 16 bits)
s_m16:
            stz         f_w + 2
            stz         f_w + 3
            lda         f_v
            sta         n_m8
            lda         f_u                                 ; (u0 v0)
            jsr         mul8
            sta         f_w
            lda         n_phi
            sta         f_w + 1
            lda         f_u + 1                             ; (u1 v0)
            jsr         mul8
            clc
            adc         f_w + 1
            sta         f_w + 1
            lda         n_phi
            adc         #0
            sta         f_w + 2
            lda         f_v + 1
            sta         n_m8
            lda         f_u                                 ; (u0 v1)
            jsr         mul8
            clc
            adc         f_w + 1
            sta         f_w + 1
            lda         n_phi
            adc         f_w + 2
            sta         f_w + 2
            lda         #0
            adc         #0
            sta         f_w + 3
            lda         f_u + 1                             ; (u1 v1)
            jsr         mul8
            clc
            adc         f_w + 2
            sta         f_w + 2
            lda         n_phi
            adc         f_w + 3
            sta         f_w + 3
            rts

; The number at .A, .X as fraction .Y (0: x, 1: y): f_n, f_d (16 bits each), its sign s_na or s_nb: C = 0; C = 1 if
; it isn't one of those.  A rational sets s_k's bit 7
f_load:
            sta         nt
            stx         nt + 1
            tya
            tax
            stz         s_na,x
            asl
            tax                                             ; (.X: its parts' offset, 0 or 2)
            lda         #1                                  ; (Its denominator 1 at first)
            sta         f_d,x
            stz         f_d + 1,x
            lda         (nt)
            cmp         #NT_RATIONAL
            beq         @rat
            cmp         #NT_FIXED
            bcc         @int
            cmp         #NT_FIXED + 5
            bcs         @no
            and         #$0F                                ; (0-4 places: 10^places its denominator)
            tay
            lda         f_p10lo,y
            sta         f_d,x
            lda         f_p10hi,y
            sta         f_d + 1,x
            jsr         s_next
@int:
            jsr         f_int16                             ; (Its numerator)
            bcs         @no
            sta         f_n,x
            tya
            sta         f_n + 1,x
            lda         f_sgn
            beq         :+
            txa
            lsr
            tay
            lda         #$FF
            sta         s_na,y
:
            clc
            rts
@rat:
            jsr         s_next
            jsr         f_int16                             ; (Its numerator, then its denominator)
            bcs         @no
            sta         f_n,x
            tya
            sta         f_n + 1,x
            lda         f_sgn
            beq         :+
            phx
            txa
            lsr
            tax
            lda         #$FF
            sta         s_na,x
            plx
:
            jsr         f_int16
            bcs         @no
            sta         f_d,x
            tya
            sta         f_d + 1,x
            ora         f_d,x                               ; (0: not a number's one form, the tower's error)
            beq         @no
            lda         #$80
            sta         s_k
            clc
            rts
@no:
            sec
            rts

; nt on a byte
s_next:
            inc         nt
            bne         :+
            inc         nt + 1
:
            rts

; .A, .Y = the magnitude of the integer at nt (16 bits at most), f_sgn its sign ($FF below 0), nt past it: C = 0;
; C = 1 if it's longer.  Keeps .X
f_int16:
            stz         f_sgn
            lda         (nt)
            cmp         #NT_POS
            bcs         @tagged
            ldy         #0
            cmp         #$40
            bcc         :+
            eor         #$7F
            inc         a
            dec         f_sgn
:
            pha
            jsr         s_next
            pla
            clc
            rts
@tagged:
            cmp         #NT_POS + 2
            bcc         @bytes
            cmp         #NT_NEG
            bcc         @no
            cmp         #NT_NEG + 2
            bcs         @no
            dec         f_sgn
@bytes:
            and         #1                                  ; (1 or 2 bytes)
            beq         @one
            ldy         #2
            lda         (nt),y
            pha
            dey
            lda         (nt),y
            ply
            pha                                             ; (Past the tag and two bytes)
            jsr         s_next
            jsr         s_next
            jsr         s_next
            pla
            clc
            rts
@one:
            ldy         #1
            lda         (nt),y
            pha
            jsr         s_next
            jsr         s_next
            pla
            ldy         #0
            clc
            rts
@no:
            sec
            rts

; ---- The result

; hv = the result (s_mr, s_nr, s_pr; a fixed decimal if s_fx), the 0s at a fixed decimal's digits' end dropped: C = 0
s_made:
            lda         s_fx
            beq         @int
@zeros:
            lda         s_pr                                ; (Its 0s dropped while it has places)
            beq         @fixed
            jsr         s_div10p
            bne         @fixed
            jsr         s_div10
            dec         s_pr
            bra         @zeros
@fixed:
            lda         #12
            ldx         #0
            jsr         a_room
            lda         s_pr
            ora         #NT_FIXED
            jsr         a_put
            bra         @digits
@int:
            lda         #12
            ldx         #0
            jsr         a_room
@digits:
            jsr         s_wint
            jsr         a_made
            clc
            rts

; Z = 1 if the result's magnitude is divisible by 10: even, its bytes' sum divisible by 5 (256 is 1 more than 5's
; multiple)
s_div10p:
            lda         s_mr
            and         #1
            bne         @rts
            ldx         #7
            stz         s_k
            stz         s_j
            clc
:
            lda         s_mr,x
            adc         s_k
            sta         s_k
            bcc         :+
            inc         s_j
            clc
:
            dex
            bpl         :--
            lda         s_k                                 ; (The sum's two bytes added: 256 is 1)
            adc         s_j
            adc         #0
            pha                                             ; (Its nibbles added: 16 is 1)
            lsr
            lsr
            lsr
            lsr
            sta         s_k
            pla
            and         #$0F
            clc
            adc         s_k
:
            cmp         #5
            bcc         :+
            sbc         #5
            bra         :-
:
            cmp         #0
@rts:
            rts

; The result's magnitude / 10 (exactly), in place
s_div10:
            lda         #0
            ldx         #7
@byte:
            ldy         #8
@bit:
            asl         s_mr,x
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         s_mr,x
:
            dey
            bne         @bit
            dex
            bpl         @byte
            rts

; The integer s_mr (its sign s_nr) to the arena (nu), in its shortest form
s_wint:
            ldx         #8                                  ; (Its bytes: those but its top 0s)
:
            dex
            bmi         @zero
            lda         s_mr,x
            beq         :-
            inx
            cpx         #1
            bne         @bytes
            lda         s_mr                                ; (One byte: -64 to 63 the tag alone)
            ldy         s_nr
            bne         @neg
            cmp         #64
            bcs         @bytes
            jmp         a_put
@neg:
            cmp         #65
            bcs         @bytes
            eor         #$FF
            inc         a
            and         #$7F
            jmp         a_put
@zero:
            lda         #0
            jmp         a_put
@bytes:
            stx         s_k
            txa                                             ; (Its tag: n - 1, + $10 below 0)
            dec         a
            ldy         s_nr
            beq         :+
            ora         #NT_NEG - NT_POS
:
            ora         #NT_POS
            jsr         a_put
            ldx         #0
:
            lda         s_mr,x
            jsr         a_put
            inx
            cpx         s_k
            bne         :-
            rts

            NM_RODATA
f_p10lo:    .byte       <1, <10, <100, <1000, <10000        ; (10^0-10^4)
f_p10hi:    .byte       >1, >10, >100, >1000, >10000
            NM_CODE
