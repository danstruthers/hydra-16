; ****************************************************************************
; nmreg.inc - integers of any size, to 255 bytes (about 614 digits): registers to work them in, and their arithmetic
; (hylang's numreg.inc, its integers read from and written in the stored format).  A register (R0-R7, regs: a page
; each in the bank) holds an integer's bytes least first, with a length (0: zero) and a sign, so a sum or a product
; makes nothing in the arena till it's written (r_store).  An operation names its registers in nra, nrb (its operands)
; and nrc (its result), and points na, nb, nc at their pages (nd: its own).  An integer past 255 bytes is NE_BIG, from
; however deep (nm_fail).

NR_DIG          = >(DIGS - $8000)               ; (A number's digits as it's written: their pages, as registers')

.ifndef NM_COPY
.segment "NUMBANK"
r_len:      .res        NRALL                               ; Each register's length (bytes; 0: zero) ...
r_neg:      .res        NRALL                               ;   and sign ($FF: negative)
nra:        .res        1                                   ; (An operation's registers: its operands, its result,
nrb:        .res        1                                   ;   and divmod's quotient and remainder)
nrc:        .res        1
nrq:        .res        1
nrr:        .res        1
n_t:        .res        2                                   ; (Scratch: a number, lengths, a carry, a byte's
n_m:        .res        1                                   ;   multiplier and its product's high byte, signs, counts)
n_la:       .res        1
n_lb:       .res        1
n_lc:       .res        1
n_cy:       .res        1
n_m8:       .res        1
n_x8:       .res        1
n_sy8:      .res        1
n_phi:      .res        1
n_d8:       .res        1
n_q8:       .res        1
n_i:        .res        1
n_sa:       .res        1
n_sb:       .res        1
n_flip:     .res        1
n_rx:       .res        1
n_bits:     .res        1
n_ab:       .res        1
n_qb:       .res        1
n_z:        .res        1
n_cnt:      .res        2
n_k:        .res        2                                   ; (A shift's count)
d_s:        .res        1                                   ; (r_divmod's: the divisor's shift, the quotient's byte
d_j:        .res        1                                   ;   being made, its estimate, rhat, the divisor's top two
d_qh:       .res        1                                   ;   digits, a window's top bytes, the carry, R's byte)
d_rh:       .res        1
d_v1:       .res        1
d_v0:       .res        1
d_t:        .res        4
d_kl:       .res        1
d_kh:       .res        1
d_w:        .res        1
n_cut:      .res        1                                   ; (r_mulc's: the product's columns below this not made)
n_j0:       .res        1
n_sq:       .res        1                                   ; (r_mulc's: a square (1), a diagonal's product)
n_dl:       .res        1
n_dh:       .res        1
mm_zp:      .res        8                                   ; (r_mul's: np, nq, nt, nu kept)

.endif
            NM_CODE

; ---- Too big

; Too big (an integer past 255 bytes, or no room left in the arena): NE_BIG, back to the entry's caller
n_big:
            lda         #NE_BIG
            jmp         nm_fail

; ---- Registers

; na, nb, nc = the pages of nra, nrb, nrc (r_abc); na, nb (r_ab); nc (r_c)
r_abc:
            lda         nrc
            RPAGE
            sta         nc + 1
            stz         nc
r_ab:
            lda         nrb
            RPAGE
            sta         nb + 1
            stz         nb
            lda         nra
            RPAGE
            sta         na + 1
            stz         na
            rts
r_c:
            lda         nrc
            RPAGE
            sta         nc + 1
            stz         nc
            rts

; .A = register .A's page (or the digits' page .A)
r_page:
            clc
            adc         #>regs
            rts

; nrc's length less its high zero bytes (nc its page); 0, it's zero, and positive
r_normc:
            ldx         nrc
            ldy         r_len,x
@byte:
            tya
            beq         @zero
            dey
            lda         (nc),y
            beq         @byte
            iny
            tya
            sta         r_len,x
            rts
@zero:
            stz         r_len,x
            stz         r_neg,x
            rts

; nrc = the number .A, .X (signed: -32768 to 32767)
r_set16:
            sta         n_t
            stx         n_t + 1
            jsr         r_c
            ldx         nrc
            stz         r_neg,x
            lda         n_t + 1
            bpl         @mag
            dec         r_neg,x
            sec
            lda         #0
            sbc         n_t
            sta         n_t
            lda         #0
            sbc         n_t + 1
            sta         n_t + 1
@mag:
            lda         n_t
            sta         (nc)
            ldy         #1
            lda         n_t + 1
            sta         (nc),y
            lda         #2
            sta         r_len,x
            jmp         r_normc

; nrc = the integer at .A, .X (in the stored format: its tag, any of an integer's, then its bytes)
r_load:
            sta         nt
            stx         nt + 1
            jsr         r_c
            ldx         nrc
            lda         (nt)
            cmp         #NT_POS
            bcs         @big
            stz         r_neg,x                             ; (-64 to 63: the tag, 7 bits' two's complement)
            cmp         #$40
            bcc         :+
            dec         r_neg,x
            eor         #$7F
            inc         a
:
            sta         (nc)
            lda         #1
            sta         r_len,x
            jmp         r_normc
@big:
            ldy         #1                                  ; (Its bytes after the tag, or after the length too)
            cmp         #NT_LONG_POS
            bcs         @long
            pha
            and         #$0F
            inc         a
            sta         r_len,x
            pla
            and         #NT_NEG - NT_POS
            beq         @sign
            lda         #$FF
            bra         @sign
@long:
            and         #1
            beq         :+
            lda         #$FF
:
            pha
            lda         (nt),y
            sta         r_len,x
            iny
            pla
@sign:
            sta         r_neg,x
            tya
            clc
            adc         nt
            sta         nt
            bcc         :+
            inc         nt + 1
:
            ldy         #0
@byte:
            lda         (nt),y
            sta         (nc),y
            iny
            tya
            cmp         r_len,x
            bne         @byte
            jmp         r_normc

; hv (and .A, .X) = the integer in nrc, written in the arena in its one form.  No room: NE_BIG
r_store:
            jsr         r_c
            jsr         r_normc
            ldx         nrc
            lda         r_len,x
            ldx         #0
            jsr         a_room
; The integer in nrc written at nu (r_c and r_normc done, the room there), nu past it; hv (and .A, .X) the address
; a_room gave, a_top nu
r_write:
            ldx         nrc
            lda         r_len,x
            beq         @small
            cmp         #2
            bcs         @bytes
            lda         (nc)                                ; (A byte: 63 or less, -64 or more, the tag alone)
            ldy         r_neg,x
            bne         :+
            cmp         #64
            bcc         @small
            bra         @bytes
:
            cmp         #65
            bcc         @small
@bytes:
            lda         r_len,x
            cmp         #17
            bcs         @long
            dec         a
            ldy         r_neg,x
            beq         :+
            ora         #NT_NEG - NT_POS
:
            ora         #NT_POS
            jsr         a_put
            bra         @copy
@long:
            lda         r_neg,x
            and         #1
            ora         #NT_LONG_POS
            jsr         a_put
            lda         r_len,x
            jsr         a_put
@copy:
            ldy         #0
:
            lda         (nc),y
            sta         (nu),y
            iny
            tya
            cmp         r_len,x
            bne         :-
            jsr         a_skip
            bra         a_made
@small:
            lda         r_len,x
            beq         @zero
            lda         (nc)
            ldy         r_neg,x
            beq         :+
            eor         #$FF
            inc         a
:
            and         #$7F
            bra         :+
@zero:
            lda         #0
:
            jsr         a_put
; The arena's number written: a_top past it, hv (and .A, .X) its address
a_made:
            MOVR        a_top, nu
            lda         hv
            ldx         hv + 1
            rts

; ---- The arena

; Room in the arena for a number of .A, .X bytes and 3 more (an integer's tag and length, a fixed decimal's places,
; written before it): nu = hv = a_top, where it's written.  No room: NE_BIG.  Keeps nothing
a_room:
            clc
            adc         #3
            bcc         :+
            inx
:
            clc
            adc         a_top
            tay
            txa
            adc         a_top + 1
            bcs         @big
            cmp         #>ARENA_END
            bcc         @fits
            bne         @big
            cpy         #<ARENA_END
            beq         @fits
            bcs         @big
@fits:
            MOVR        nu, a_top
            MOVR        hv, a_top
            rts
@big:
            jmp         n_big

; .A to the arena (nu), nu moved on.  Keeps .X, .Y
a_put:
            sta         (nu)
            inc         nu
            bne         :+
            inc         nu + 1
:
            rts

; nu moved on .Y bytes (1-256: 0 is 256).  Keeps .X
a_skip:
            tya
            clc
            adc         nu
            sta         nu
            bcc         :+
            inc         nu + 1
:
            cpy         #0
            bne         :+
            inc         nu + 1
:
            rts

; nrc = nra (another register)
r_copy:
            jsr         r_abc
            ldx         nra
            lda         r_len,x
            ldy         r_neg,x
            ldx         nrc
            sta         r_len,x
            tya
            sta         r_neg,x
            ldy         #0
@byte:
            tya
            cmp         r_len,x
            bcs         @done
            lda         (na),y
            sta         (nc),y
            iny
            bne         @byte
@done:
            rts

; Register .X: Z = 1 if it's zero
r_zero_p:
            lda         r_len,x
            rts

; Register .X = 0
r_clear:
            stz         r_len,x
            stz         r_neg,x
            rts

; Register .X's sign the other way (zero stays positive)
r_negate:
            lda         r_len,x
            beq         @done
            lda         r_neg,x
            eor         #$FF
            sta         r_neg,x
@done:
            rts

; ---- Magnitudes

; n_m = the longer of nra's and nrb's lengths; each zero-extended to it (its bytes past its length made 0)
r_maxlen:
            ldx         nra
            lda         r_len,x
            ldx         nrb
            cmp         r_len,x
            bcs         :+
            lda         r_len,x
:
            sta         n_m
            ldx         nra
            jsr         r_zext
            ldx         nrb
; Register .X's bytes from its length to n_m made 0 (nd its page)
r_zext:
            txa
            RPAGE
            sta         nd + 1
            stz         nd
            ldy         r_len,x
            lda         #0
@byte:
            cpy         n_m
            bcs         @done
            sta         (nd),y
            iny
            bne         @byte
@done:
            rts

; .A = nra's magnitude against nrb's: $FF less, 0 the same, 1 more
r_ucmp:
            jsr         r_ab
            ldx         nra
            lda         r_len,x
            ldx         nrb
            cmp         r_len,x
            bne         @order
            tay
@byte:
            dey
            cpy         #$FF
            beq         @same
            lda         (na),y
            cmp         (nb),y
            beq         @byte
@order:
            lda         #1
            bcs         :+
            lda         #$FF
:
            rts
@same:
            lda         #0
            rts

; .A = nra against nrb, signed: $FF less, 0 the same (Z = 1), 1 more
r_cmp:
            ldx         nra
            lda         r_neg,x
            ldx         nrb
            cmp         r_neg,x
            beq         @same
            lda         r_neg,x                             ; (Signs that differ: the negative one less)
            beq         @less
            lda         #1
            rts
@less:
            lda         #$FF
            rts
@same:
            pha
            jsr         r_ucmp
            plx
            beq         @rts                                ; (Both negative: the other way)
            eor         #$FF
            inc
@rts:
            cmp         #0                                  ; (Z: the same)
            rts

; |nrc| = |nra| + |nrb| (nrc may be either).  Too big: n_big
r_uadd:
            jsr         r_maxlen
            jsr         r_abc
            ldy         #0
            ldx         n_m
            clc
            beq         @end
@byte:
            lda         (na),y
            adc         (nb),y
            sta         (nc),y
            iny
            dex
            bne         @byte
@end:
            bcc         @len
            cpy         #255
            bcc         :+
            jmp         n_big
:
            lda         #1
            sta         (nc),y
            iny
@len:
            tya
            ldx         nrc
            sta         r_len,x
            jmp         r_normc

; |nrc| = |nra| - |nrb|, |nra| not less (nrc may be either)
r_usub:
            jsr         r_maxlen
            jsr         r_abc
            ldy         #0
            ldx         n_m
            sec
            beq         @end
@byte:
            lda         (na),y
            sbc         (nb),y
            sta         (nc),y
            iny
            dex
            bne         @byte
@end:
            tya
            ldx         nrc
            sta         r_len,x
            jmp         r_normc

; nrc = nra + nrb (r_add), or nra - nrb (r_sub); nrc may be either
r_sub:
            lda         #$FF
            bra         r_addsub
r_add:
            lda         #0
r_addsub:
            sta         n_flip
            ldx         nrb
            lda         r_neg,x
            eor         n_flip
            sta         n_sb                                ; (nrb's sign, as it's added)
            ldx         nra
            lda         r_neg,x
            sta         n_sa
            eor         n_sb
            bne         @differ
            jsr         r_uadd
            lda         n_sa
            bra         @sign
@differ:
            jsr         r_ucmp                              ; (The less from the more, its sign the more's)
            cmp         #$FF
            beq         @more_b
            jsr         r_usub
            lda         n_sa
            bra         @sign
@more_b:
            jsr         r_swap
            jsr         r_usub
            jsr         r_swap
            lda         n_sb
@sign:
            ldx         nrc
            ldy         r_len,x
            bne         :+
            lda         #0
:
            sta         r_neg,x
            rts

; nra and nrb the other way round
r_swap:
            lda         nra
            ldx         nrb
            sta         nrb
            stx         nra
            rts

; .A * n_m8: .A = the product's low byte, n_phi its high.  Keeps .Y.  By quarter squares: a b = f(a + b) - f(|a - b|),
; f(n) = n^2 / 4 rounded down (sq_lo, sq_hi: f's 512, 0 to 510)
mul8:
            sta         n_x8
            sty         n_sy8
            sec                                             ; (|a - b|: .Y)
            sbc         n_m8
            bcs         :+
            eor         #$FF
            adc         #1
:
            tay
            lda         n_x8                                ; (a + b: .X, past 255 the tables' second half)
            clc
            adc         n_m8
            tax
            bcs         @high
            lda         sq_lo,x
            sec
            sbc         sq_lo,y
            sta         n_x8
            lda         sq_hi,x
            sbc         sq_hi,y
            sta         n_phi
            ldy         n_sy8
            lda         n_x8
            rts
@high:
            lda         sq_lo + 256,x
            sec
            sbc         sq_lo,y
            sta         n_x8
            lda         sq_hi + 256,x
            sbc         sq_hi,y
            sta         n_phi
            ldy         n_sy8
            lda         n_x8
            rts

            NM_RODATA
sq_lo:                                                      ; (f(n) = n^2 / 4 rounded down: its low bytes, its high)
.repeat 512, I
            .byte       <((I * I) / 4)
.endrep
sq_hi:
.repeat 512, I
            .byte       >((I * I) / 4)
.endrep
nsq_lo:                                                     ; (f(|k - 255|), k 0 to 510: r_mul's, from 255 - a)
.repeat 511, I
            .byte       <(((I - 255) * (I - 255)) / 4)
.endrep
nsq_hi:
.repeat 511, I
            .byte       >(((I - 255) * (I - 255)) / 4)
.endrep
            NM_CODE

; nrc = nra * nrb (nrc another register).  Too big: n_big.  r_mulc: the product's columns (bytes) below n_cut not
; made (a truncated product: m_mulw's, whose shift drops them)
r_mul:
            stz         n_cut
r_mulc:
            ldx         nra
            lda         r_neg,x
            ldx         nrb
            eor         r_neg,x
            sta         n_sa                                ; (Its sign)
            ldx         nra
            lda         r_len,x
            beq         @zero
            sta         n_la
            ldx         nrb
            lda         r_len,x
            beq         @zero
            sta         n_lb
            clc
            adc         n_la
            sta         n_lc                                ; (Its length at most: 256 is 0)
            bcc         @fits
            beq         @fits
            jmp         n_big
@zero:
            ldx         nrc
            jmp         r_clear
@fits:
            stz         n_sq                                ; (A square: its cross products once)
            lda         nra
            cmp         nrb
            bne         :+
            inc         n_sq
:
            jsr         r_abc
            ldy         #0                                  ; (0, then each of nra's bytes times nrb added in)
            tya
@clear:
            sta         (nc),y
            iny
            cpy         n_lc
            bne         @clear
            ldx         #7                                  ; (np, nq, nt, nu: the tables' pointers a while, kept)
:
            lda         np,x
            sta         mm_zp,x
            dex
            bpl         :-
            stz         n_i
@row:
            lda         n_cut                               ; (Its columns from cut - i: none below the cut; a
            sec                                             ;   square's from i + 1)
            sbc         n_i
            bcs         :+
            lda         #0
:
            ldx         n_sq
            beq         :+
            ldx         n_i
            inx
            stx         n_j0
            cmp         n_j0
            bcs         :+
            txa
:
            sta         n_j0
            cmp         n_lb
            bcc         :+
            jmp         @next
:
            ldy         n_i                                 ; (a: its pointers into the tables)
            lda         (na),y
            bne         :+
            jmp         @next
:
            tax
            clc
            adc         #<sq_lo
            sta         np
            lda         #>sq_lo
            adc         #0
            sta         np + 1
            txa
            clc
            adc         #<sq_hi
            sta         nq
            lda         #>sq_hi
            adc         #0
            sta         nq + 1
            txa
            eor         #$FF
            tax
            clc
            adc         #<nsq_lo
            sta         nt
            lda         #>nsq_lo
            adc         #0
            sta         nt + 1
            txa
            clc
            adc         #<nsq_hi
            sta         nu
            lda         #>nsq_hi
            adc         #0
            sta         nu + 1
            lda         n_i                                 ; (nd: the product's byte i + j0 on, moved on a byte a
            clc                                             ;   time: its page nc's)
            adc         n_j0
            sta         nd
            lda         nc + 1
            sta         nd + 1
            stz         n_cy
            ldx         n_j0
@col:
            txa                                             ; (b)
            tay
            lda         (nb),y
            tay
            lda         (np),y                              ; (a b: its low byte, then its high, .Y)
            sec
            sbc         (nt),y
            sta         n_phi
            lda         (nq),y
            sbc         (nu),y
            tay
            lda         n_phi                               ; (The carry added, then the product's byte)
            clc
            adc         n_cy
            bcc         :+
            iny
:
            clc
            adc         (nd)
            sta         (nd)
            tya
            adc         #0
            sta         n_cy
            inc         nd
            inx
            cpx         n_lb
            bne         @col
            lda         n_cy
            sta         (nd)
@next:
            inc         n_i
            lda         n_i
            cmp         n_la
            beq         :+
            jmp         @row
:
            ldx         #7
:
            lda         mm_zp,x
            sta         np,x
            dex
            bpl         :-
            lda         n_sq                                ; (A square: the cross products doubled, the diagonal's
            beq         @made                               ;   squares added)
            ldy         #0
            ldx         n_lc
            clc
:
            lda         (nc),y
            rol         a
            sta         (nc),y
            iny
            dex
            bne         :-
            stz         n_i
@diag:
            ldy         n_i
            lda         (na),y
            beq         @dnext
            asl         a                                   ; (a^2 = sq[2a])
            tax
            bcs         @dhi
            lda         sq_lo,x
            sta         n_dl
            lda         sq_hi,x
            bra         :+
@dhi:
            lda         sq_lo + 256,x
            sta         n_dl
            lda         sq_hi + 256,x
:
            sta         n_dh
            tya                                             ; (At 2i, its carry on up)
            asl         a
            tay
            lda         (nc),y
            clc
            adc         n_dl
            sta         (nc),y
            iny
            lda         (nc),y
            adc         n_dh
            sta         (nc),y
            bcc         @dnext
:
            iny
            lda         (nc),y
            adc         #0
            sta         (nc),y
            bcs         :-
@dnext:
            inc         n_i
            lda         n_i
            cmp         n_la
            bne         @diag
@made:
            ldx         nrc
            lda         n_sa
            sta         r_neg,x
            lda         n_lc
            bne         @len
            ldy         #255                                ; (256 bytes at most: its last must be 0)
            lda         (nc),y
            beq         :+
            jmp         n_big
:
            lda         #255
@len:
            sta         r_len,x
            jsr         r_normc
            ldx         nrc
            lda         r_len,x
            bne         :+
            stz         r_neg,x
:
            rts

; |nrc| = |nrc| * .A (1-255), in place.  Too big: n_big
r_mulsmall:
            sta         n_m8
            jsr         r_c
            ldx         nrc
            lda         r_len,x
            sta         n_lc
            stz         n_cy
            ldy         #0
@byte:
            cpy         n_lc
            beq         @end
            lda         (nc),y
            jsr         mul8
            clc
            adc         n_cy
            sta         (nc),y
            lda         n_phi
            adc         #0
            sta         n_cy
            iny
            bne         @byte
@end:
            lda         n_cy
            beq         @len
            cpy         #255
            bcc         :+
            jmp         n_big
:
            sta         (nc),y
            iny
@len:
            tya
            ldx         nrc
            sta         r_len,x
            jmp         r_normc

; |nrc| = |nrc| / .A (1-255), in place.  OUT: .A = the remainder
r_divsmall:
            sta         n_d8
            jsr         r_c
            ldx         nrc
            ldy         r_len,x
            lda         #0                                  ; (The remainder so far, in .A)
@byte:
            dey
            cpy         #$FF
            bne         :+
            jmp         @done
:
            tax                                             ; (A 0 with no remainder: its quotient's byte 0)
            bne         @div
            lda         (nc),y
            beq         @byte
            txa
@div:
            pha
            lda         (nc),y
            sta         n_q8
            pla
.repeat 8                                                   ; (Its 8 steps, unrolled)
            asl         n_q8
            rol         a
            bcs         :+
            cmp         n_d8
            bcc         :++
:
            sbc         n_d8
            inc         n_q8
:
.endrep
            pha
            lda         n_q8
            sta         (nc),y
            pla
            jmp         @byte
@done:
            sta         n_cy
            jsr         r_normc
            lda         n_cy
            rts

; nrq = nra / nrb (toward 0), nrr = nra - nrq * nrb (nra's sign): nrq and nrr registers of their own, nrb not 0.
; A byte of the quotient at a time (Knuth's algorithm D, NUMSPEED.md's step 4): the remainder (nrr, nra's bytes at
; first) and the divisor (nrb, as it is); each quotient byte estimated from the remainder's window's top three bytes
; and the divisor's top two, both as if shifted till the divisor's top bit is 1 (its digits then), at most one too
; many; the divisor times it taken from the window, and added back if it was.  A divisor of a byte: r_divsmall
r_divmod:
            ldx         nra
            lda         r_neg,x
            sta         n_sa                                ; (The remainder's sign)
            ldx         nrb
            eor         r_neg,x
            sta         n_sb                                ; (The quotient's)
            ldx         nra
            lda         r_len,x
            sta         n_la
            ldx         nrb
            lda         r_len,x
            sta         n_lb
            lda         nra                                 ; (na: the dividend; nb: the divisor; nc: the quotient;
            RPAGE      ;   nd: the remainder)
            sta         na + 1
            stz         na
            lda         nrb
            RPAGE
            sta         nb + 1
            stz         nb
            lda         nrq
            RPAGE
            sta         nc + 1
            stz         nc
            lda         nrr
            RPAGE
            sta         nd + 1
            stz         nd
            lda         n_lb
            cmp         #2
            bcs         @wide
            lda         nrq                                 ; (A divisor of a byte: the quotient nra / it, in place)
            sta         nrc
            jsr         r_copy
            lda         (nb)
            jsr         r_divsmall                          ; (.A: the remainder)
            ldx         nrr
            sta         (nd)
            lda         #1
            sta         r_len,x
            jmp         @signs
@wide:
            ldy         #0                                  ; (The remainder: the dividend's bytes, and a 0 above)
@copy:
            cpy         n_la
            beq         :+
            lda         (na),y
            sta         (nd),y
            iny
            bra         @copy
:
            lda         #0
            sta         (nd),y
            lda         n_la                                ; (Shorter than the divisor: a quotient of 0)
            cmp         n_lb
            bcs         :+
            ldx         nrq
            stz         r_len,x
            ldx         nrr
            lda         n_la
            sta         r_len,x
            bra         @signs
:
            ldy         n_lb                                ; (The divisor's top three bytes, then its shift: d_v
            dey                                             ;   its top two digits as shifted)
            lda         (nb),y
            sta         d_t + 2
            dey
            lda         (nb),y
            sta         d_t + 1
            stz         d_t
            dey
            bmi         :+
            lda         (nb),y
            sta         d_t
:
            stz         d_s
            lda         d_t + 2
@norm:
            bmi         :+
            inc         d_s
            asl         d_t
            rol         d_t + 1
            rol         d_t + 2
            lda         d_t + 2
            bra         @norm
:
            sta         d_v1
            lda         d_t + 1
            sta         d_v0
            lda         n_la                                ; (j: the quotient's byte, from its top)
            sec
            sbc         n_lb
            sta         d_j
            inc         a
            ldx         nrq
            sta         r_len,x
@digit:
            jsr         d_qhat                              ; (d_qh: its estimate)
            jsr         d_mulsub                            ; (The divisor times it taken off: C = 1 if too many)
            bcc         :+
            jsr         d_addback
            dec         d_qh
:
            ldy         d_j
            lda         d_qh
            sta         (nc),y
            lda         d_j
            beq         :+
            dec         d_j
            jmp         @digit
:
            ldx         nrr                                 ; (The remainder: the divisor's length)
            lda         n_lb
            sta         r_len,x
@signs:
            ldx         nrq                                 ; (Their signs, their lengths without top 0s)
            lda         n_sb
            sta         r_neg,x
            ldx         nrr
            lda         n_sa
            sta         r_neg,x
            lda         nrr
            sta         nrc
            jsr         r_c
            jsr         r_normc
            lda         nrq
            sta         nrc
            jsr         r_c
            jmp         r_normc

; d_qh = the estimate of the quotient's byte d_j: the window R(d_j .. d_j + lb)'s top three bytes shifted as the
; divisor was (d_s), over its top two digits (d_v1, d_v0); Knuth's: (u2 u1) / v1 at most 255, then less while its
; times v0 is past (rhat u0)
d_qhat:
            lda         d_j                                 ; (The window's top four bytes: R(j + lb) ... R(j + lb - 3))
            clc
            adc         n_lb
            tay
            lda         (nd),y
            sta         d_t + 3
            dey
            lda         (nd),y
            sta         d_t + 2
            dey
            lda         (nd),y
            sta         d_t + 1
            stz         d_t
            dey
            cpy         #$FF
            beq         :+
            lda         (nd),y
            sta         d_t
:
            ldx         d_s                                 ; (Shifted)
            beq         @shifted
:
            asl         d_t
            rol         d_t + 1
            rol         d_t + 2
            rol         d_t + 3
            dex
            bne         :-
@shifted:
            lda         d_t + 3                             ; (u2 = v1: 255, rhat u1 + v1)
            cmp         d_v1
            bcc         @divide
            lda         #255
            sta         d_qh
            lda         d_t + 2
            clc
            adc         d_v1
            sta         d_rh
            bcc         @check
            rts
@divide:
            lda         d_t + 2                             ; ((u2 u1) / v1: d_qh, its remainder rhat .A)
            sta         d_qh
            lda         d_t + 3
            ldx         #8
:
            asl         d_qh
            rol         a
            bcs         @take
            cmp         d_v1
            bcc         @keep
@take:
            sbc         d_v1
            inc         d_qh
@keep:
            dex
            bne         :-
            sta         d_rh
@check:
            lda         d_qh                                ; (qhat v0 past (rhat u1'): one less, rhat + v1)
            sta         n_m8
            lda         d_v0
            jsr         mul8
            ldx         n_phi
            cpx         d_rh
            bcc         @done
            bne         @less
            cmp         d_t + 1
            bcc         @done
            beq         @done
@less:
            dec         d_qh
            lda         d_rh
            clc
            adc         d_v1
            sta         d_rh
            bcc         @check                              ; (rhat past a byte: no more)
@done:
            rts

; R(j .. j + lb) -= d_qh * the divisor: C = 1 if it went below 0 (one too many)
d_mulsub:
            lda         d_qh
            sta         n_m8
            stz         d_kl                                ; (The carry: a product's high byte and a borrow, 9 bits)
            stz         d_kh
            ldy         #0
            lda         d_j
            sta         d_w                                 ; (R's byte: j + i)
@byte:
            lda         (nb),y
            jsr         mul8                                ; (.A low, n_phi high; .Y kept)
            clc
            adc         d_kl
            sta         d_t
            lda         n_phi
            adc         d_kh
            sta         d_kl
            lda         #0
            adc         #0
            sta         d_kh
            phy
            ldy         d_w
            lda         (nd),y
            sec
            sbc         d_t
            sta         (nd),y
            bcs         :+
            inc         d_kl                                ; (A borrow: one more off the next)
            bne         :+
            inc         d_kh
:
            inc         d_w
            ply
            iny
            cpy         n_lb
            bne         @byte
            ldy         d_w                                 ; (The window's top byte, less the carry)
            lda         (nd),y
            sec
            sbc         d_kl
            sta         (nd),y
            lda         #0
            sbc         d_kh
            bcs         :+
            sec                                             ; (Below 0)
            rts
:
            clc
            rts

; R(j .. j + lb) += the divisor (it went below 0: one too many taken)
d_addback:
            ldy         #0
            lda         d_j
            sta         d_w
            ldx         n_lb
            clc
@byte:
            lda         (nb),y
            phy
            ldy         d_w
            adc         (nd),y
            sta         (nd),y
            ply
            inc         d_w
            iny
            dex
            bne         @byte
            ldy         d_w                                 ; (Its carry to the top byte)
            lda         (nd),y
            adc         #0
            sta         (nd),y
            rts

; nrc = 10 ^ .A, .X.  Too big: n_big
r_pow10:
            cpx         #0                                  ; (To 10^24: the table's; past it, 10^24 times 10^(k - 24))
            bne         @big
            cmp         #P10_TAB
            bcs         @big
            jmp         r_p10set
@big:
            sec
            sbc         #P10_TAB - 1
            pha
            txa
            sbc         #0
            pha
            lda         #P10_TAB - 1
            jsr         r_p10set
            plx
            pla
; |nrc| = |nrc| * 10 ^ .A, .X, in place (by 100s, then a 10).  Too big: n_big (at once for 10 ^ 615 or more:
; past 2040 bits)
r_scale10:
            sta         n_cnt
            stx         n_cnt + 1
            ldx         nrc
            lda         r_len,x
            beq         @done                               ; (0 stays 0)
            lda         n_cnt
            cmp         #<615
            lda         n_cnt + 1
            sbc         #>615
            bcc         @hundred
            jmp         n_big
@hundred:
            lda         n_cnt + 1
            bne         :+
            lda         n_cnt
            cmp         #2
            bcc         @ten
:
            lda         n_cnt                               ; (Two places)
            sec
            sbc         #2
            sta         n_cnt
            bcs         :+
            dec         n_cnt + 1
:
            lda         #100
            jsr         r_mulsmall
            bra         @hundred
@ten:
            lda         n_cnt
            beq         @done
            lda         #10
            jmp         r_mulsmall
@done:
            rts

; nrc = 10 ^ .A (below P10_TAB), from the table (pow10.inc)
r_p10set:
            tay
            lda         p10_len,y
            sta         n_t
            lda         p10_off,y
            tax
            jsr         r_c
            ldy         #0
:
            lda         p10_dat,x
            sta         (nc),y
            inx
            iny
            cpy         n_t
            bne         :-
            ldx         nrc
            lda         n_t
            sta         r_len,x
            stz         r_neg,x
            rts

; .A, .X = the trailing zero bits of nrc's magnitude (not 0)
r_tz:
            jsr         r_c
            stz         n_cnt
            stz         n_cnt + 1
            ldy         #0
@byte:
            lda         (nc),y
            bne         @bits
            lda         n_cnt
            clc
            adc         #8
            sta         n_cnt
            bcc         :+
            inc         n_cnt + 1
:
            iny
            bra         @byte
@bits:
            lsr
            bcs         @done
            inc         n_cnt
            bne         @bits
            inc         n_cnt + 1
            bra         @bits
@done:
            lda         n_cnt
            ldx         n_cnt + 1
            rts

; |nrc| = |nrc| / 2 ^ .A, .X, in place
r_shrax:
            sta         n_k
            stx         n_k + 1
            jmp         r_shr

; |nrc| shifted left n_k bits.  Too big (past 2040 bits): n_big
r_shl:
            jsr         r_c
            ldx         nrc
            lda         r_len,x
            bne         :+
            rts
:
            tay                                             ; (Its bits: 8 a byte below its top, and its top's)
            dey
            lda         (nc),y
            ldy         #0
:
            iny
            lsr
            bne         :-
            sty         n_t                                 ; (8 * (len - 1) + top's + n_k, past 2040: too big)
            lda         r_len,x
            dec         a
            stz         n_t + 1
            asl
            rol         n_t + 1
            asl
            rol         n_t + 1
            asl
            rol         n_t + 1
            clc
            adc         n_t
            sta         n_t
            bcc         :+
            inc         n_t + 1
:
            lda         n_t
            clc
            adc         n_k
            sta         n_t
            lda         n_t + 1
            adc         n_k + 1
            bcs         @big
            cmp         #>2041
            bcc         @fits
            bne         @big
            lda         n_t
            cmp         #<2041
            bcc         @fits
@big:
            jmp         n_big
@fits:
            lda         n_k                                 ; (Whole bytes: n_k / 8)
            sta         n_t
            lda         n_k + 1
            lsr
            ror         n_t
            lsr
            ror         n_t
            lsr
            ror         n_t
            lda         r_len,x
            clc
            adc         n_t
            sta         n_lc                                ; (Its bytes up by n_t: 255 at most, as it fits)
            ldy         r_len,x
@up:
            dey
            cpy         #$FF
            beq         @zeros
            lda         (nc),y
            pha
            tya
            clc
            adc         n_t
            tay
            pla
            sta         (nc),y
            tya
            sec
            sbc         n_t
            tay
            bra         @up
@zeros:
            ldy         n_t
            beq         :+
            lda         #0
@zero:
            dey
            sta         (nc),y
            bne         @zero
:
            lda         n_lc                                ; (A byte more, 0, for the bits, if there's room: there
            cmp         #255                                ;   is but at 255, where the bits fit as they are)
            beq         :+
            tay
            lda         #0
            sta         (nc),y
            inc         n_lc
:
            lda         n_lc
            sta         r_len,x
            lda         n_k
            and         #7
            beq         @norm
            sta         n_bits
@bit:
            ldy         #0
            ldx         nrc
            lda         r_len,x
            tax
            clc
@shift:
            lda         (nc),y
            rol
            sta         (nc),y
            iny
            dex
            bne         @shift
            dec         n_bits
            bne         @bit
@norm:
            jmp         r_normc

; |nrc| shifted right n_k bits
r_shr:
            jsr         r_c
            ldx         nrc
            lda         n_k + 1                             ; (Whole bytes: n_k / 8; all of them, 0)
            cmp         #8
            bcs         @zero
            lda         n_k
            sta         n_t
            lda         n_k + 1
            lsr
            ror         n_t
            lsr
            ror         n_t
            lsr
            ror         n_t
            lda         n_t
            cmp         r_len,x
            bcs         @zero
            ldy         #0                                  ; (Its bytes down by n_t)
@down:
            tya
            clc
            adc         n_t
            cmp         r_len,x
            bcs         @len
            phy
            tay
            lda         (nc),y
            ply
            sta         (nc),y
            iny
            bra         @down
@len:
            tya
            sta         r_len,x
            lda         n_k
            and         #7
            beq         @norm
            sta         n_bits
@bit:
            ldx         nrc
            ldy         r_len,x
            clc
@shift:
            dey
            lda         (nc),y
            ror
            sta         (nc),y
            tya
            bne         @shift
            dec         n_bits
            bne         @bit
@norm:
            jmp         r_normc
@zero:
            jmp         r_clear

; nrc in two's complement in n_m bytes (its bytes to n_m 0, then if it's negative each inverted and 1 added)
r_tc:
            ldx         nrc
            jsr         r_zext
            jsr         r_c
            ldx         nrc
            lda         r_neg,x
            bne         r_inv1
            rts
; Its n_m bytes inverted and 1 added
r_inv1:
            ldy         #0
            ldx         n_m
            sec
@byte:
            lda         (nc),y
            eor         #$FF
            adc         #0
            sta         (nc),y
            iny
            dex
            bne         @byte
@rts:
            rts

; nrc from two's complement in n_m bytes: negative if its top bit is set
r_untc:
            jsr         r_c
            ldx         nrc
            lda         n_m
            sta         r_len,x
            stz         r_neg,x
            tay
            dey
            lda         (nc),y
            bpl         @norm
            jsr         r_inv1
            ldx         nrc
            lda         #$FF
            sta         r_neg,x
@norm:
            jmp         r_normc

; .A, .X = nrc's value, 16 bits signed (past them -32768 or 32767)
r_word:
            jsr         r_c
            ldx         nrc
            stz         n_t
            stz         n_t + 1
            lda         r_len,x
            beq         @sign
            cmp         #3
            bcs         @past
            lda         (nc)
            sta         n_t
            lda         r_len,x
            cmp         #2
            bcc         @sign
            ldy         #1
            lda         (nc),y
            sta         n_t + 1
            bpl         @sign
            lda         r_neg,x                             ; (32768 and more: -32768 only, negative)
            beq         @past
            lda         n_t + 1
            cmp         #$80
            bne         @past
            lda         n_t
            bne         @past
            lda         #0
            ldx         #$80
            rts
@sign:
            lda         r_neg,x
            beq         :+
            sec
            lda         #0
            sbc         n_t
            sta         n_t
            lda         #0
            sbc         n_t + 1
            sta         n_t + 1
:
            lda         n_t
            ldx         n_t + 1
            rts
@past:
            lda         r_neg,x
            bne         :+
            lda         #$FF
            ldx         #$7F
            rts
:
            lda         #0
            ldx         #$80
            rts

; Is nrc one?  OUT: Z = 1 if it is
r_one_p:
            ldx         nrc
            lda         r_len,x
            cmp         #1
            bne         @rts
            lda         r_neg,x
            bne         @rts
            jsr         r_c
            lda         (nc)
            cmp         #1
@rts:
            rts

.include "pow10.inc"
