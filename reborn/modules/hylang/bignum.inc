; ****************************************************************************
; bignum.inc - hylang's integers of any size (step 7.3), in the module's third bank: what's past a fixnum is an
; object, OT_BIG (its length in bytes, its top bit the sign; its bytes, the low first: 126 at most, some 300 decimal
; digits).  The arithmetic is done in four buffers in the task's RAM (na, nb, nc, nd: a length, a sign, the bytes),
; reached through the pointers nq_a, nq_b, nq_c and nq_d: nb_load makes one of a value, nb_store a value of one (a
; fixnum, if it fits).  int_op is the arithmetic of two integer values (ia and h_v): a sum, a product, a quotient
; and a remainder (truncated, as danlang's BigInteger has them), a gcd; int_cmp their order.

NB_MAX          = 126           ; An integer's bytes, at most (an object's: a cell of 128)
NB_SIZE         = NB_MAX + 4    ; A buffer: its length, its sign ($80: negative), its bytes, and room for a carry

; int_op's operations
IO_ADD          = 0
IO_SUB          = 1
IO_MUL          = 2
IO_QUO          = 3             ; The quotient, truncated (toward 0)
IO_REM          = 4             ; The remainder (its sign the dividend's)
IO_GCD          = 5             ; The greatest common divisor (not negative)

.zeropage
nq_a:       .res        2                                   ; The buffers an operation takes: two operands ...
nq_b:       .res        2
nq_c:       .res        2                                   ;   its result (or what's loaded, stored) ...
nq_d:       .res        2                                   ;   and a remainder (or a pointer mag_mul moves)

.bss
na:         .res        NB_SIZE                             ; The buffers
nb:         .res        NB_SIZE
nc:         .res        NB_SIZE
nd:         .res        NB_SIZE
ia:         .res        2                                   ; int_op's first operand (a value, loaded before anything
n_op:       .res        1                                   ; is made, so it needn't be a root); the operation
n_len:      .res        1                                   ; A length
n_end:      .res        1                                   ; An end (an offset in a buffer)
n_cnt:      .res        1                                   ; A count
n_s:        .res        1                                   ; A small number (a multiplier, a divisor) ...
n_t:        .res        1                                   ;   and one added
n_pl:       .res        1                                   ; A product (mul8's) ...
n_ph:       .res        1
n_carry:    .res        1                                   ;   a carry ...
n_ai:       .res        1                                   ;   a multiplier's byte
n_q:        .res        1                                   ; A quotient's byte, and the remainder (mag_divs)
n_rem:      .res        1
n_sa:       .res        1                                   ; Signs kept
n_sb:       .res        1
n_bit:      .res        1                                   ; mag_div's bit, and its byte's offset
n_byte:     .res        1
.segment "CODE3"

.macro NQ ptr, buf
            LDR         ptr, buf
.endmacro

; The four pointers at their own buffers
nq_init:
            NQ          nq_a, na
            NQ          nq_b, nb
            NQ          nq_c, nc
            NQ          nq_d, nd
            rts

; nq_a and nq_b swapped
nq_swap:
            ldx         nq_a
            ldy         nq_b
            stx         nq_b
            sty         nq_a
            ldx         nq_a + 1
            ldy         nq_b + 1
            stx         nq_b + 1
            sty         nq_a + 1
            rts

; ---- Buffers

; The buffer at nq_c with its top zero bytes left off (0: its sign +).  OUT: .A = its length
nb_trim:
            lda         (nq_c)
            tay
@top:
            cpy         #0
            beq         @zero
            iny                                             ; (Its top byte: at 2 + its length - 1)
            lda         (nq_c),y
            dey
            cmp         #0
            bne         @done
            dey
            bra         @top
@zero:
            ldy         #1
            lda         #0
            sta         (nq_c),y
            tay
@done:
            tya
            sta         (nq_c)
            rts

; The buffers at nq_a and nq_b zero-extended to .X bytes (their lengths kept)
nb_zext:
            txa
            clc
            adc         #2
            sta         n_end
            lda         (nq_a)
            clc
            adc         #2
            tay
            lda         #0
@a:
            cpy         n_end
            bcs         :+
            sta         (nq_a),y
            iny
            bra         @a
:
            lda         (nq_b)
            clc
            adc         #2
            tay
            lda         #0
@b:
            cpy         n_end
            bcs         :+
            sta         (nq_b),y
            iny
            bra         @b
:
            rts

; The buffer at nq_c = 0
nb_zero:
            lda         #0
            sta         (nq_c)
            ldy         #1
            sta         (nq_c),y
            rts

; The buffer at nq_c = the buffer at nq_a
nb_copy:
            lda         (nq_a)
            clc
            adc         #2
            tay
:
            dey
            lda         (nq_a),y
            sta         (nq_c),y
            tya
            bne         :-
            rts

; ---- Magnitudes (the bytes, not the signs)

; |A| against |B| (each trimmed): .A = $FF, 0 or 1 (less, the same, more)
mag_cmp:
            lda         (nq_a)
            cmp         (nq_b)
            bne         @len
            tay                                             ; (From the top byte down)
            iny
@byte:
            cpy         #2
            bcc         @same
            lda         (nq_a),y
            cmp         (nq_b),y
            bne         @len
            dey
            bra         @byte
@len:
            bcc         @less
            lda         #1
            rts
@less:
            lda         #$FF
            rts
@same:
            lda         #0
            rts

; |C| = |A| + |B| (C may be A)
mag_add:
            lda         (nq_a)
            cmp         (nq_b)
            bcs         :+
            lda         (nq_b)
:
            sta         n_len
            tax
            jsr         nb_zext
            ldx         n_len
            beq         @done
            ldy         #2
            clc
@byte:
            lda         (nq_a),y
            adc         (nq_b),y
            sta         (nq_c),y
            iny
            dex
            bne         @byte
            lda         #0                                  ; (The carry: a byte more)
            adc         #0
            sta         (nq_c),y
            inc         n_len
@done:
            lda         n_len
            sta         (nq_c)
            jmp         nb_trim

; |C| = |A| - |B|, |A| not less (C may be A)
mag_sub:
            lda         (nq_a)
            sta         n_len
            tax
            jsr         nb_zext
            ldx         n_len
            beq         @done
            ldy         #2
            sec
@byte:
            lda         (nq_a),y
            sbc         (nq_b),y
            sta         (nq_c),y
            iny
            dex
            bne         @byte
@done:
            lda         n_len
            sta         (nq_c)
            jmp         nb_trim

; n_ph:n_pl = .A * n_s.  Keeps .X, .Y
mul8:
            sta         n_pl                                ; (Its bits shifted out as the product's low byte comes in)
            lda         #0
            phx
            ldx         #8
            lsr         n_pl
@bit:
            bcc         :+
            clc
            adc         n_s
:
            ror
            ror         n_pl
            dex
            bne         @bit
            sta         n_ph
            plx
            rts

; |C| = |C| * n_s + n_t (n_s, n_t a byte each)
mag_muls:
            lda         n_t
            sta         n_carry
            lda         (nq_c)
            tax
            ldy         #2
@byte:
            cpx         #0
            beq         @end
            lda         (nq_c),y
            jsr         mul8
            clc
            lda         n_pl
            adc         n_carry
            sta         (nq_c),y
            lda         n_ph
            adc         #0
            sta         n_carry
            iny
            dex
            bra         @byte
@end:
            lda         n_carry                             ; (The carry: a byte more)
            beq         @done
            sta         (nq_c),y
            lda         (nq_c)
            inc         a
            sta         (nq_c)
@done:
            rts

; |C| = |C| / n_s (a byte, not 0): .A = the remainder
mag_divs:
            stz         n_rem
            lda         (nq_c)
            tay
            iny                                             ; (From the top byte down)
@byte:
            cpy         #2
            bcc         @done
            lda         (nq_c),y
            sta         n_q
            ldx         #8
@bit:
            asl         n_q
            rol         n_rem
            bcs         @sub                                ; (Past a byte: more than the divisor)
            lda         n_rem
            cmp         n_s
            bcc         @next
@sub:
            lda         n_rem
            sbc         n_s
            sta         n_rem
            inc         n_q
@next:
            dex
            bne         @bit
            lda         n_q
            sta         (nq_c),y
            dey
            bra         @byte
@done:
            jsr         nb_trim
            lda         n_rem
            rts

; |C| = |A| * |B| (C neither of them).  OUT: C = 1, too many bytes
mag_mul:
            lda         (nq_a)
            beq         @zero
            lda         (nq_b)
            beq         @zero
            clc
            adc         (nq_a)
            cmp         #NB_MAX + 2
            bcc         :+
            rts                                             ; (C = 1)
:
            sta         n_len
            tax                                             ; (C's bytes, 0)
            ldy         #2
            lda         #0
:
            sta         (nq_c),y
            iny
            dex
            bne         :-
            ldx         #0                                  ; (Each of A's bytes, times B, added in at its place:
@i:                                                         ; nq_d at C's byte .X)
            txa
            cmp         (nq_a)
            beq         @done
            clc
            adc         nq_c
            sta         nq_d
            lda         nq_c + 1
            adc         #0
            sta         nq_d + 1
            txa
            clc
            adc         #2
            tay
            lda         (nq_a),y
            sta         n_s
            stz         n_carry
            lda         (nq_b)
            sta         n_cnt
            ldy         #2
@j:
            lda         (nq_b),y
            jsr         mul8
            clc
            lda         n_pl
            adc         (nq_d),y
            sta         n_pl
            lda         n_ph
            adc         #0
            sta         n_ph
            clc
            lda         n_pl
            adc         n_carry
            sta         (nq_d),y
            lda         n_ph
            adc         #0
            sta         n_carry
            iny
            dec         n_cnt
            bne         @j
            lda         n_carry
            sta         (nq_d),y
            inx
            bra         @i
@done:
            lda         n_len
            sta         (nq_c)
            jsr         nb_trim
            clc
            rts
@zero:
            jsr         nb_zero
            clc
            rts

; |C| = |A| / |B|, |D| = |A| % |B| (B not 0; C, D neither A nor B): a bit at a time, a divisor of a byte a byte
; at a time (mag_divs)
mag_div:
            lda         (nq_b)
            cmp         #1
            bne         @long
            ldy         #2                                  ; ---- By a byte: C = A, then divided
            lda         (nq_b),y
            sta         n_s
            jsr         nb_copy
            jsr         mag_divs
            ldy         #1                                  ; (D = the remainder, + till nb_divmod's sign)
            pha
            lda         #0
            sta         (nq_d),y
            pla
            beq         :+
            iny
            sta         (nq_d),y
            lda         #1
:
            sta         (nq_d)
            rts
@long:
            lda         #0                                  ; ---- A bit at a time: D = 0; C as long as A, 0
            sta         (nq_d)
            lda         (nq_a)
            sta         (nq_c)
            tax
            ldy         #2
            lda         #0
@clear:
            cpx         #0
            beq         :+
            sta         (nq_c),y
            iny
            dex
            bra         @clear
:
            lda         (nq_a)                              ; (From A's top byte down)
            clc
            adc         #1
            sta         n_byte
@nextbyte:
            lda         n_byte
            cmp         #2
            bcc         @done
            lda         #$80
            sta         n_bit
@nextbit:
            ldy         n_byte                              ; (D = 2D + the bit)
            lda         (nq_a),y
            and         n_bit
            cmp         #1                                  ; (C = the bit: kept through the loop, which counts
            php                                             ; with .X and tests it with txa)
            lda         (nq_d)
            tax
            ldy         #2
            plp
            txa
            beq         @shifted
@shift:
            lda         (nq_d),y
            rol
            sta         (nq_d),y
            iny
            dex
            bne         @shift
@shifted:
            lda         #0
            rol
            beq         :+
            sta         (nq_d),y                            ; (A byte more)
            lda         (nq_d)
            inc         a
            sta         (nq_d)
:
            jsr         div_cmp                             ; (D not less than B: D -= B, the quotient's bit 1)
            bcc         @bitdone
            jsr         div_sub
            ldy         n_byte
            lda         (nq_c),y
            ora         n_bit
            sta         (nq_c),y
@bitdone:
            lsr         n_bit
            bcc         @nextbit
            dec         n_byte
            bra         @nextbyte
@done:
            jsr         nb_trim
            rts

; C = 1 if |D| >= |B| (each trimmed)
div_cmp:
            lda         (nq_d)
            cmp         (nq_b)
            bne         @done
            tay
            iny
@byte:
            cpy         #2
            bcc         @same
            lda         (nq_d),y
            cmp         (nq_b),y
            bne         @done
            dey
            bra         @byte
@same:
            sec
@done:
            rts

; |D| -= |B| (|D| not less), trimmed
div_sub:
            lda         (nq_b)                              ; (B zero-extended to D's length)
            clc
            adc         #2
            tay
            lda         (nq_d)
            clc
            adc         #2
            sta         n_end
            lda         #0
:
            cpy         n_end
            bcs         :+
            sta         (nq_b),y
            iny
            bra         :-
:
            lda         (nq_d)
            tax
            ldy         #2
            sec
@byte:
            lda         (nq_d),y
            sbc         (nq_b),y
            sta         (nq_d),y
            iny
            dex
            bne         @byte
            lda         (nq_d)                              ; (Trimmed)
            tay
@top:
            cpy         #0
            beq         @set
            iny
            lda         (nq_d),y
            dey
            cmp         #0
            bne         @set
            dey
            bra         @top
@set:
            tya
            sta         (nq_d)
            rts

; ---- Signed

; C = A + B (C may be A; B's sign may change: nb_sub)
nb_add:
            ldy         #1
            lda         (nq_a),y
            sta         n_sa
            eor         (nq_b),y
            bmi         @differ
            jsr         mag_add                             ; (The same signs: A's)
            bra         @sign
@differ:
            lda         (nq_b),y
            sta         n_sb
            jsr         mag_cmp
            cmp         #$FF
            beq         @less
            jsr         mag_sub                             ; (|A| - |B|, A's sign)
            bra         @sign
@less:
            jsr         nq_swap                             ; (|B| - |A|, B's sign)
            jsr         mag_sub
            jsr         nq_swap
            lda         n_sb
            sta         n_sa
@sign:
            lda         (nq_c)
            beq         :+
            lda         n_sa
            ldy         #1
            sta         (nq_c),y
:
            rts

; C = A - B (B's sign changes)
nb_sub:
            ldy         #1
            lda         (nq_b),y
            eor         #$80
            sta         (nq_b),y
            jmp         nb_add

; C = A * B (C neither).  OUT: C = 1, too big
nb_mul:
            ldy         #1
            lda         (nq_a),y
            eor         (nq_b),y
            sta         n_sa
            jsr         mag_mul
            bcs         @done
            lda         (nq_c)
            beq         :+
            lda         n_sa
            ldy         #1
            sta         (nq_c),y
:
            clc
@done:
            rts

; C = A / B, truncated; D = A % B, its sign A's (B not 0; C, D neither)
nb_divmod:
            ldy         #1
            lda         (nq_a),y
            sta         n_sa
            eor         (nq_b),y
            sta         n_sb
            jsr         mag_div
            lda         (nq_c)
            beq         :+
            lda         n_sb
            ldy         #1
            sta         (nq_c),y
:
            lda         (nq_d)
            beq         :+
            lda         n_sa
            ldy         #1
            sta         (nq_d),y
:
            rts

; A against B, signed: .A = $FF, 0 or 1
nb_cmp:
            ldy         #1
            lda         (nq_a),y
            eor         (nq_b),y
            bpl         @same
            lda         (nq_a),y                            ; (Signs differ: the negative's less)
            bmi         @less
            lda         #1
            rts
@less:
            lda         #$FF
            rts
@same:
            lda         (nq_a),y
            pha
            jsr         mag_cmp
            ply                                             ; (Both negative: the other way round)
            bpl         @done
            eor         #$FF
            inc         a
@done:
            rts

; ---- Values

; h_v (an integer: a fixnum, or OT_BIG) into the buffer at nq_c
nb_load:
            lda         h_v
            lsr
            bcc         @big
            lda         h_v + 1                             ; (A fixnum: its value, h_v >> 1)
            cmp         #$80
            ror
            tax
            lda         h_v
            ror
            ldy         #1
            cpx         #$80
            bcc         @pos
            eor         #$FF                                ; (Negative: its size, and the sign)
            clc
            adc         #1
            pha
            txa
            eor         #$FF
            adc         #0
            tax
            lda         #$80
            sta         (nq_c),y
            pla
            bra         @bytes
@pos:
            pha
            lda         #0
            sta         (nq_c),y
            pla
@bytes:
            iny
            sta         (nq_c),y
            iny
            txa
            sta         (nq_c),y
            lda         #2
            sta         (nq_c)
            jmp         nb_trim
@big:
            jsr         deref                               ; (OT_BIG: its sign, its length, its bytes)
            ldy         #1
            lda         (h_p),y
            pha
            and         #$80
            sta         (nq_c),y
            pla
            and         #$7F
            sta         (nq_c)
            clc
            adc         #2
            tay
:
            dey
            cpy         #2
            bcc         :+
            lda         (h_p),y
            sta         (nq_c),y
            bra         :-
:
            rts

; The buffer at nq_c as a value: h_v, a fixnum if it fits, else OT_BIG.  OUT: C = 1, h_v an error (too big, or no
; room)
nb_store:
            jsr         nb_trim
            beq         @zero
            cmp         #3
            bcs         @big
            ldy         #2                                  ; (Two bytes at most: a fixnum, -16384 to 16383?)
            lda         (nq_c),y
            sta         n_pl
            stz         n_ph
            lda         (nq_c)
            cmp         #2
            bne         :+
            iny
            lda         (nq_c),y
            sta         n_ph
:
            ldy         #1
            lda         (nq_c),y
            bmi         @neg
            lda         n_ph
            cmp         #$40
            bcs         @big
            bra         @fix
@neg:
            lda         n_ph                                ; (-16384: $4000)
            cmp         #$40
            bcc         :+
            bne         @big
            lda         n_pl
            bne         @big
:
            sec
            lda         #0
            sbc         n_pl
            sta         n_pl
            lda         #0
            sbc         n_ph
            sta         n_ph
@fix:
            lda         n_pl                                ; (2n + 1)
            asl
            ora         #1
            sta         h_v
            lda         n_ph
            rol
            sta         h_v + 1
            clc
            rts
@zero:
            LDR         h_v, 1
            clc
            rts
@big:
            lda         (nq_c)
            cmp         #NB_MAX + 1
            bcs         @too_big
            clc
            adc         #2
            jsr         cell_kind
            jsr         alloc
            bcs         @room
            lda         #OT_BIG
            sta         (h_p)
            ldy         #1
            lda         (nq_c),y
            ora         (nq_c)
            sta         (h_p),y
            lda         (nq_c)
            clc
            adc         #2
            tay
:
            dey
            cpy         #2
            bcc         :+
            lda         (nq_c),y
            sta         (h_p),y
            bra         :-
:
            clc
            rts
@too_big:
            LDR         r0, nm_big
            jsr         error_text
            bcs         @room
            sec
            rts
@room:
            MOVR        h_v, err_room
            sec
            rts

; h_v = ia (op .A) h_v, integers (IO_ADD ...).  OUT: C = 1, h_v an error (too big, division by zero, no room)
int_op:
            sta         n_op
            jsr         nq_init
            NQ          nq_c, nb                            ; (h_v into B, ia into A)
            jsr         nb_load
            MOVR        h_v, ia
            NQ          nq_c, na
            jsr         nb_load
            NQ          nq_c, nc
            ldx         n_op
            lda         io_lo,x
            sta         ev_j
            lda         io_hi,x
            sta         ev_j + 1
            jmp         (ev_j)
io_add:
            jsr         nb_add
            jmp         nb_store
io_sub:
            jsr         nb_sub
            jmp         nb_store
io_mul:
            jsr         nb_mul
            bcc         :+
            LDR         r0, nm_big
            jmp         io_error
:
            jmp         nb_store
io_quo:
            jsr         io_div
            bcs         :+
            jmp         nb_store
:
            rts
io_rem:
            jsr         io_div
            bcs         :+
            NQ          nq_c, nd
            jmp         nb_store
:
            rts
io_gcd:
            ldy         #1                                  ; (Not negative)
            lda         #0
            sta         (nq_a),y
            sta         (nq_b),y
@step:
            lda         (nq_b)                              ; (B 0: A)
            beq         @done
            jsr         mag_div                             ; (A, B = B, A % B: the pointers turned round)
            lda         nq_a
            ldx         nq_a + 1
            pha
            phx
            lda         nq_b
            sta         nq_a
            lda         nq_b + 1
            sta         nq_a + 1
            lda         nq_d
            sta         nq_b
            lda         nq_d + 1
            sta         nq_b + 1
            plx
            pla
            sta         nq_d
            stx         nq_d + 1
            bra         @step
@done:
            lda         nq_a
            sta         nq_c
            lda         nq_a + 1
            sta         nq_c + 1
            ldy         #1                                  ; (+: the buffer may have been D, its sign unset)
            lda         #0
            sta         (nq_c),y
            jmp         nb_store

; C = A / B, D = A % B, B not 0.  OUT: C = 1, h_v the error Division by zero.
io_div:
            lda         (nq_b)
            bne         :+
            LDR         r0, nm_div_zero
            jmp         io_error
:
            jsr         nb_divmod
            clc
            rts

; h_v = the error whose message is the text at r0.  OUT: C = 1
io_error:
            jsr         error_text
            bcc         :+
            MOVR        h_v, err_room
:
            sec
            rts

; ia against h_v (integers), signed: .A = $FF, 0 or 1
int_cmp:
            lda         ia                                  ; (Two fixnums: their values' order)
            and         h_v
            lsr
            bcc         @big
            sec
            lda         ia
            sbc         h_v
            sta         n_pl
            lda         ia + 1
            sbc         h_v + 1
            bvc         :+
            eor         #$80
:
            bmi         @less
            ora         n_pl
            beq         :+
            lda         #1
:
            rts
@less:
            lda         #$FF
            rts
@big:
            jsr         nq_init
            NQ          nq_c, nb
            jsr         nb_load
            MOVR        h_v, ia
            NQ          nq_c, na
            jsr         nb_load
            jmp         nb_cmp

; Its sign: .A = $FF, 0 or 1 (h_v an integer)
int_sign:
            lda         h_v
            lsr
            bcc         @big
            lda         h_v + 1
            bmi         @neg
            ora         h_v
            cmp         #1
            beq         @zero
            lda         #1
            rts
@big:
            jsr         deref                               ; (OT_BIG: never 0)
            ldy         #1
            lda         (h_p),y
            bmi         @neg
            lda         #1
            rts
@neg:
            lda         #$FF
            rts
@zero:
            lda         #0
            rts

; ---- Its texts

.segment "RODATA3"
io_lo:      .lobytes    io_add, io_sub, io_mul, io_quo, io_rem, io_gcd
io_hi:      .hibytes    io_add, io_sub, io_mul, io_quo, io_rem, io_gcd
nm_big:     .byte       "A number too big (a few hundred digits at most)", 0
nm_div_zero: .byte      "Division by zero.", 0
.segment "CODE3"
