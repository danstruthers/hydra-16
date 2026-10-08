; ****************************************************************************
; nmrand.inc - RANDOM (hylang's generator: a 16-bit xorshift, its state in the bank, st_rand) and FIB (hylang's, by
; doubling).

.segment "NUMBANK"
n_fn:       .res        2                                   ; (FIB's n, shifted out a bit at a time)
n_fb:       .res        1                                   ; (Its bits left)

.code

; RANDOM: r0 = n, an integer: 1 or more, an integer from 0 below it at r2; 0, a fixed decimal from 0 up to 1, of the
; precision's places (st_digits), at r2.  NE_INT: n isn't an integer, or it's below 0
nm_random:
            jsr         nm_begin
            jsr         nm_in1
            ldy         #0
            jsr         n_intof
            ldx         #0
            lda         r_neg,x
            beq         :+
            lda         #NE_INT
            jmp         nm_fail
:
            lda         r_len,x
            bne         @below
            lda         st_digits                           ; (0: below 10 ^ the precision, that many places)
            ldx         #0
            jsr         r_pow10
            jsr         rn_below
            lda         #1
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            lda         st_digits
            sta         n_pl
            stz         n_pl + 1
            jsr         n_fix0
            jmp         nm_out
@below:
            jsr         rn_below
            lda         #1
            sta         nrc
            jsr         r_store
            jmp         nm_out

; R1 = an integer from 0 below R0 (1 or more): as many of the generator's bytes as R0 has, the top one masked to R0's
; top byte's bits, drawn again while it's R0 or more
rn_below:
            lda         #1
            sta         nrc
            jsr         r_c
            ldx         #0
            lda         r_len,x
            sta         n_la
            ldx         #1
            sta         r_len,x
            stz         r_neg,x
            ldy         #0
@byte:
            jsr         rn_next
            sta         (nc),y
            iny
            cpy         n_la
            bne         @byte
            dey
            lda         #0
            jsr         r_page
            sta         na + 1
            stz         na
            lda         (na),y                              ; (R0's top byte: the bits it has)
            ldx         #$FF
:
            asl
            bcs         :+
            pha
            txa
            lsr
            tax
            pla
            bra         :-
:
            txa
            and         (nc),y
            sta         (nc),y
            jsr         r_normc
            lda         #1                                  ; (R0 or more: again)
            sta         nra
            lda         #0
            sta         nrb
            jsr         r_ucmp
            cmp         #$FF
            bne         rn_below
            rts

; The generator's next byte (st_rand: a 16-bit xorshift, 7 9 8).  OUT: .A.  Keeps .X, .Y
rn_next:
            lda         st_rand + 1
            lsr
            lda         st_rand
            ror
            eor         st_rand + 1
            sta         st_rand + 1
            ror
            eor         st_rand
            sta         st_rand
            eor         st_rand + 1
            sta         st_rand + 1
            rts

; FIB: r0 = n, an integer 0 or more; the nth Fibonacci number at r2: by doubling, from n's top bit down (n past 16
; bits: 32767, too big).  NE_INT: n isn't an integer, or it's below 0
nm_fib:
            jsr         nm_begin
            jsr         nm_in1
            ldy         #0
            jsr         n_intof
            ldx         #0
            lda         r_neg,x
            beq         :+
            lda         #NE_INT
            jmp         nm_fail
:
            jsr         r_word
            sta         n_fn
            stx         n_fn + 1
            lda         #0                                  ; (a = 0: R0; b = 1: R1)
            sta         nrc
            tax
            jsr         r_set16
            lda         #1
            sta         nrc
            ldx         #0
            jsr         r_set16
            lda         #16
            sta         n_fb
@bit:
            lda         #1                                  ; (t = a(2b - a): R3; b = aa + bb; a = t)
            sta         nra
            sta         nrb
            lda         #2
            sta         nrc
            jsr         r_add
            lda         #2
            sta         nra
            sta         nrc
            lda         #0
            sta         nrb
            jsr         r_sub
            lda         #0
            ldx         #2
            ldy         #3
            jsr         n_mulr
            lda         #0
            ldx         #0
            ldy         #4
            jsr         n_mulr
            lda         #1
            ldx         #1
            ldy         #5
            jsr         n_mulr
            lda         #4
            sta         nra
            lda         #5
            sta         nrb
            lda         #1
            sta         nrc
            jsr         r_add
            lda         #3
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            asl         n_fn                                ; (n's bit: a, b = b, a + b)
            rol         n_fn + 1
            bcc         @next
            lda         #0
            sta         nra
            lda         #1
            sta         nrb
            lda         #2
            sta         nrc
            jsr         r_add
            lda         #1
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            lda         #2
            sta         nra
            lda         #1
            sta         nrc
            jsr         r_copy
@next:
            dec         n_fb
            beq         :+
            jmp         @bit
:
            lda         #0
            sta         nrc
            jsr         r_store
            jmp         nm_out
