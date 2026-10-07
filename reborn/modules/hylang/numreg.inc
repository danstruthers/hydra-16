; ****************************************************************************
; numreg.inc - hylang's integers of any size (phase 5), in its third bank: registers to work them in, and their
; arithmetic.  An integer is a fixnum, or past one a bignum (PK_BIGNUM: its length and sign, its bytes in a blob,
; least first), to 255 bytes (about 614 digits); one past that is the error "Too big".
;   A register (R0-R7) is a page of the reader's scratch (free while a number is worked: the reader reads a number's
; word from its text), its bytes least first, with a length (0: zero) and a sign; so a sum or a product of bignums
; makes nothing on the heap till its value is made (r_store).  An operation names its registers in nra, nrb (its
; operands) and nrc (its result), and points na, nb, nc at their pages (nd: its own).
;   An entry to the number code (a built-in, the reader's, the printer's ...) sets an abort point as it starts
; (n_enter): the 65C02's stack, the root stack and the captures as they were; n_big goes back to it with the error
; "Too big", and n_nomem with no room, from however deep.

NREGS           = 8             ; The registers: R0-R7, the first pages of scratch (from its first whole page)
NR_DIG          = 8             ; (The pages after them: a number's digits as it's written, 7 pages ...
NR_DIGS         = 7
NR_TEXT         = 8             ;   and, as it's read, its text: the first of them)
NLEVELS         = 6             ; Abort points set at once, at most

.bss
r_len:      .res        NREGS                               ; Each register's length (bytes; 0: zero) ...
r_neg:      .res        NREGS                               ;   and sign ($FF: negative)
nra:        .res        1                                   ; (An operation's registers: its operands, its result,
nrb:        .res        1                                   ;   and divmod's quotient and remainder)
nrc:        .res        1
nrq:        .res        1
nrr:        .res        1
n_lvl:      .res        1                                   ; Abort points set: each one's 65C02 stack and root stack
n_spa:      .res        NLEVELS
n_rsa:      .res        NLEVELS
n_cpa:      .res        NLEVELS                             ;   and its captures (open, their length: a capture
n_cla:      .res        NLEVELS                             ;   begun after it is given up)
n_cha:      .res        NLEVELS
n_abt:      .res        1                                   ; <> 0: the last entry went back to its abort point
n_t:        .res        2                                   ; (Scratch: a number, lengths, a carry, a byte's
n_m:        .res        1                                   ;   multiplier and its product's high byte, signs, counts)
n_la:       .res        1
n_lb:       .res        1
n_lc:       .res        1
n_cy:       .res        1
n_m8:       .res        1
n_x8:       .res        1
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

.segment "CODE3"

; ---- Abort points

; An abort point set: the entry that calls it (first thing: jsr n_enter; then its work; then jmp n_leave) goes back
; to its caller from n_big or n_nomem, the root stack as it was.  Keeps .Y
n_enter:
            stz         n_abt
            tsx                                             ; (The entry's stack: past this call's return)
            inx
            inx
            txa
            ldx         n_lvl
            sta         n_spa,x
            lda         rsp
            sta         n_rsa,x
            lda         cap_on
            sta         n_cpa,x
            lda         cap_len
            sta         n_cla,x
            lda         cap_len + 1
            sta         n_cha,x
            inc         n_lvl
            rts

; The entry's end: its abort point dropped.  Keeps .A, .X, .Y, C
n_leave:
            php
            dec         n_lvl
            plp
            rts

; No room: back to the abort point, C = 1 (the number code calls it as anything it makes fails)
n_nomem:
            jsr         n_back
            sec
            rts

; Too big: back to the abort point, ex = the error "Too big: an integer past 255 bytes" (C = 1 if there's no room
; for it)
n_big:
            jsr         n_back
            LDAX        s_toobig
            jmp         n_error

; Back to the abort point: the 65C02's stack (n_back's caller's return left on it) and the root stack as it was set
n_back:
            pla                                             ; (Its return, kept)
            sta         n_t
            pla
            sta         n_t + 1
            lda         #1
            sta         n_abt
            dec         n_lvl
            ldx         n_lvl
            lda         n_rsa,x
            sta         rsp
            lda         n_cpa,x
            sta         cap_on
            lda         n_cla,x
            sta         cap_len
            lda         n_cha,x
            sta         cap_len + 1
            lda         n_spa,x
            tax
            txs
            lda         n_t + 1
            pha
            lda         n_t
            pha
            rts

; ---- Registers

; na, nb, nc = the pages of nra, nrb, nrc (r_abc); na, nb (r_ab); nc (r_c)
r_abc:
            lda         nrc
            jsr         r_page
            sta         nc + 1
            stz         nc
r_ab:
            lda         nrb
            jsr         r_page
            sta         nb + 1
            stz         nb
            lda         nra
            jsr         r_page
            sta         na + 1
            stz         na
            rts
r_c:
            lda         nrc
            jsr         r_page
            sta         nc + 1
            stz         nc
            rts

; .A = register .A's page (or the digits' page .A)
r_page:
            clc
            adc         #>(scratch + 255)
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

; nrc = the integer .A, .X (a fixnum or a bignum)
r_load:
            sta         n_t
            stx         n_t + 1
            lsr
            bcc         @big
            lda         n_t
            ldx         n_t + 1
            jsr         unfix
            jmp         r_set16
@big:
            jsr         r_c
            lda         n_t
            ldx         n_t + 1
            jsr         deref
            ldx         nrc
            lda         (hp)                                ; (Its length, its sign)
            sta         r_len,x
            ldy         #1
            lda         (hp),y
            sta         r_neg,x
            iny
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldx         nrc
            ldy         #0
@byte:
            tya
            cmp         r_len,x
            bcs         @done
            lda         (hp),y
            sta         (nc),y
            iny
            bne         @byte
@done:
            rts

; hv = the integer in nrc: a fixnum if it's in one's range, else a bignum.  No room: n_nomem
r_store:
            jsr         r_c
            jsr         r_normc
            ldx         nrc
            lda         r_len,x
            cmp         #3
            bcs         @big
            stz         n_t                                 ; (Its magnitude, two bytes at most)
            stz         n_t + 1
            tay
            beq         @fix
            lda         (nc)
            sta         n_t
            cpy         #2
            bcc         @range
            ldy         #1
            lda         (nc),y
            sta         n_t + 1
@range:
            lda         n_t + 1                             ; (A fixnum's: to 16383, or 16384 negative)
            cmp         #$40
            bcc         @fix
            bne         @big
            lda         n_t
            bne         @big
            lda         r_neg,x
            beq         @big
@fix:
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
            jsr         fix_make
            clc
            rts
@big:
            lda         #PK_BIGNUM                          ; (A cell, then its blob: its length, its bytes)
            jsr         cell_alloc
            bcs         @nomem
            ldx         nrc
            lda         r_len,x
            sta         hn
            stz         hn + 1
            jsr         blob_alloc
            bcs         @nomem
            lda         hv
            ldx         hv + 1
            jsr         deref
            ldx         nrc
            lda         r_len,x
            sta         (hp)
            ldy         #1
            lda         r_neg,x
            sta         (hp),y
            iny
            lda         hb
            sta         (hp),y
            iny
            lda         hb + 1
            sta         (hp),y
            lda         hb
            ldx         hb + 1
            jsr         blob_data
            stz         hq
            lda         nrc
            jsr         r_page
            sta         hq + 1
            ldx         nrc
            lda         r_len,x
            sta         hn
            stz         hn + 1
            jsr         copy_in
            clc
            rts
@nomem:
            jmp         n_nomem

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
            jsr         r_page
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

; .A * n_m8: .A = the product's low byte, n_phi its high.  Keeps .Y
mul8:
            sta         n_x8
            lda         #0
            ldx         #8
            lsr         n_x8
@bit:
            bcc         :+
            clc
            adc         n_m8
:
            ror
            ror         n_x8
            dex
            bne         @bit
            sta         n_phi
            lda         n_x8
            rts

; nrc = nra * nrb (nrc another register).  Too big: n_big
r_mul:
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
            jsr         r_abc
            ldy         #0                                  ; (0, then each of nra's bytes times nrb added in)
            tya
@clear:
            sta         (nc),y
            iny
            cpy         n_lc
            bne         @clear
            stz         n_i
@row:
            ldy         n_i
            lda         (na),y
            beq         @next
            sta         n_m8
            lda         nc                                  ; (nd: the product's byte n_i on)
            clc
            adc         n_i
            sta         nd
            lda         nc + 1
            adc         #0
            sta         nd + 1
            stz         n_cy
            ldy         #0
@col:
            lda         (nb),y
            jsr         mul8
            clc
            adc         n_cy
            bcc         :+
            inc         n_phi
:
            clc
            adc         (nd),y
            sta         (nd),y
            lda         n_phi
            adc         #0
            sta         n_cy
            iny
            cpy         n_lb
            bne         @col
            lda         n_cy
            sta         (nd),y
@next:
            inc         n_i
            lda         n_i
            cmp         n_la
            bne         @row
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
            stz         n_cy                                ; (The remainder so far)
@byte:
            dey
            cpy         #$FF
            beq         @done
            lda         (nc),y
            sta         n_q8
            lda         n_cy
            ldx         #8
@bit:
            asl         n_q8
            rol
            bcs         @sub
            cmp         n_d8
            bcc         @no
@sub:
            sbc         n_d8
            inc         n_q8
@no:
            dex
            bne         @bit
            sta         n_cy
            lda         n_q8
            sta         (nc),y
            bra         @byte
@done:
            jsr         r_normc
            lda         n_cy
            rts

; nrq = nra / nrb (toward 0), nrr = nra - nrq * nrb (nra's sign): nrq and nrr registers of their own, nrb not 0.
; A bit at a time: the remainder shifted up through nra's bits, nrb taken from it where it can be
r_divmod:
            ldx         nra
            lda         r_neg,x
            sta         n_sa
            ldx         nrb
            eor         r_neg,x
            sta         n_sb                                ; (The quotient's sign)
            ldx         nra
            lda         r_len,x
            sta         n_la
            ldx         nrb
            lda         r_len,x
            sta         n_lb
            lda         nrr                                 ; (nd: the remainder, n_lb bytes and n_rx, its bit past
            jsr         r_page                              ;   them; nc: the quotient)
            sta         nd + 1
            stz         nd
            lda         nrq
            sta         nrc
            jsr         r_abc
            ldy         #0
            tya
@clear:
            sta         (nd),y
            iny
            cpy         n_lb
            bne         @clear
            stz         n_rx
            ldy         n_la
            sty         n_i
@byte:
            ldy         n_i
            beq         @made
            dey
            sty         n_i
            lda         (na),y
            sta         n_ab
            lda         #8
            sta         n_bits
@bit:
            asl         n_ab                                ; (Its next bit into the remainder)
            ldy         #0
            ldx         n_lb
@shift:
            lda         (nd),y
            rol
            sta         (nd),y
            iny
            dex
            bne         @shift
            rol         n_rx
            lda         n_rx                                ; (Not less than nrb: nrb taken, the quotient's bit 1)
            bne         @take
            ldy         n_lb
@cmp:
            dey
            cpy         #$FF
            beq         @take
            lda         (nd),y
            cmp         (nb),y
            beq         @cmp
            bcc         @zero
@take:
            ldy         #0
            ldx         n_lb
            sec
@sub:
            lda         (nd),y
            sbc         (nb),y
            sta         (nd),y
            iny
            dex
            bne         @sub
            stz         n_rx
            sec
            bra         @qbit
@zero:
            clc
@qbit:
            rol         n_qb
            dec         n_bits
            bne         @bit
            ldy         n_i
            lda         n_qb
            sta         (nc),y
            bra         @byte
@made:
            ldx         nrq                                 ; (Their lengths and signs)
            lda         n_la
            sta         r_len,x
            lda         n_sb
            sta         r_neg,x
            jsr         r_normc
            lda         nrr
            sta         nrc
            jsr         r_c
            ldx         nrr
            lda         n_lb
            sta         r_len,x
            lda         n_sa
            sta         r_neg,x
            jsr         r_normc
            lda         nrq
            sta         nrc
            jsr         r_c
            jmp         r_normc

; nrc = 10 ^ .A, .X (places: 0 to 16383).  Too big: n_big
r_pow10:
            sta         n_cnt
            stx         n_cnt + 1
            lda         #1
            ldx         #0
            jsr         r_set16
@ten:
            lda         n_cnt
            ora         n_cnt + 1
            beq         @done
            lda         n_cnt
            bne         :+
            dec         n_cnt + 1
:
            dec         n_cnt
            lda         #10
            jsr         r_mulsmall
            bra         @ten
@done:
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

.segment "RODATA3"
s_toobig:   .byte       "Too big: an integer past 255 bytes", 0

.segment "CODE3"
