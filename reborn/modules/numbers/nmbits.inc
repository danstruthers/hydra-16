; ****************************************************************************
; nmbits.inc - BITS: the bits of integers of any size, in two's complement (hylang's numbits.inc, as danlang's
; bit-and, bit-or, bit-xor, bit-not, shl, shr and bit?).

.ifndef NM_COPY
.segment "NUMBANK"
n_bop:      .res        1                                   ; (The operation: .Y as BITS was called)
n_dir:      .res        1                                   ; (A shift's way: 0 left, $FF right)

.endif
            NM_CODE

; BITS: r0 = x, r1 = y, integers; .Y = the operation: 0 and, 1 or, 2 xor, 3 not (x alone: -x - 1), 4 x shifted left
; y bits, 5 right (toward minus infinity; a negative count either way the other way; a count past 16 bits -32768 or
; 32767), 6 bit y's test.  The result at r2 (the test: .A = 0 or 1, nothing at r2).  NE_INT: x or y isn't an
; integer; NE_BIG: too big (and, or and xor of an integer of 255 bytes too: they work a byte past the longer)
nm_bits:
            jsr         nm_begin
            sty         n_bop
            cpy         #3
            bne         @two
            jsr         nm_in1                              ; (Not: -x - 1)
            ldy         #0
            jsr         n_intof
            ldx         #0
            jsr         r_negate
            jsr         nb_less1
            bra         @store
@two:
            jsr         nm_in2
            lda         n_x
            ldx         n_x + 1
            ldy         #0
            jsr         n_intof
            lda         n_y
            ldx         n_y + 1
            ldy         #2
            jsr         n_intof
            lda         n_bop
            cmp         #4
            bcs         @shift
            ldx         #0                                  ; (Both in two's complement, a byte past the longer)
            lda         r_len,x
            ldx         #2
            cmp         r_len,x
            bcs         :+
            lda         r_len,x
:
            inc         a
            bne         :+
            jmp         n_big
:
            sta         n_m
            lda         #0
            sta         nrc
            jsr         r_tc
            lda         #2
            sta         nrc
            jsr         r_tc
            lda         #0
            sta         nra
            lda         #2
            sta         nrb
            jsr         r_ab
            ldy         #0
@byte:
            lda         (na),y
            ldx         n_bop
            beq         @and
            dex
            beq         @or
            eor         (nb),y
            bra         @set
@and:
            and         (nb),y
            bra         @set
@or:
            ora         (nb),y
@set:
            sta         (na),y
            iny
            cpy         n_m
            bne         @byte
            lda         #0
            sta         nrc
            jsr         r_untc
@store:
            lda         #0
            sta         nrc
            jsr         r_store
            jmp         nm_out
@shift:
            lda         #2                                  ; (The count: 16 bits)
            sta         nrc
            jsr         r_word
            sta         n_k
            stx         n_k + 1
            stz         n_dir
            lda         n_bop
            cmp         #4
            beq         :+
            dec         n_dir
            cmp         #6                                  ; (The test, of a bit below 0: 0, x * 2^k's)
            bne         :+
            lda         n_k + 1
            bpl         :+
            lda         #0
            bra         @bit
:
            jsr         nb_shift0
            lda         n_bop
            cmp         #6
            bne         @store
            ldx         #0                                  ; (The test: x >> y's bit 0, its magnitude's)
            lda         r_len,x
            beq         @bit
            lda         #0
            jsr         r_page
            sta         na + 1
            stz         na
            lda         (na)
            and         #1
@bit:
            ldx         #0
            clc
            jmp         nm_end

; R0 shifted by n_k: left (n_dir 0) or right (n_dir $FF, toward minus infinity); a negative count the other way
nb_shift0:
            lda         n_k + 1
            bpl         @way
            sec
            lda         #0
            sbc         n_k
            sta         n_k
            lda         #0
            sbc         n_k + 1
            sta         n_k + 1
            lda         n_dir
            eor         #$FF
            sta         n_dir
@way:
            lda         #0
            sta         nrc
            lda         n_dir
            bne         :+
            jmp         r_shl
:
            ldx         #0                                  ; (Right, and negative: -(((|n| - 1) >> k) + 1))
            lda         r_neg,x
            bne         :+
            jmp         r_shr
:
            jsr         r_negate
            jsr         nb_less1
            jsr         r_shr
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jsr         r_add
            ldx         #0
            jmp         r_negate

; R0 = R0 - 1 (R2: 1)
nb_less1:
            lda         #2
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jmp         r_sub
