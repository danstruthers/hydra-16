; ****************************************************************************
; numbi.inc - hylang's number built-ins (phase 5), in its third bank, as danlang's: + - * / (fixnums first, the rest
; by numval.inc's tower), abs, rational.n, rational.d, to-rational, to-fixed, truncate, complex, val, fib, random,
; the bits and bytes; and what the other banks call: a number's sign, whole value and order, range, to-str in a base.
; Each built-in gets its values at argp (cn of them) and gives back ex (C = 1 if there's no room); those that work
; numbers past a fixnum set an abort point (n_enter) as they start, and end through n_leave.

.segment "CODE3"

; ---- + - * /

; (+ x...): the sum (0 for none); or, the first a string or a character, the values as print shows them, joined
bi_add:
            lda         #'+'
            sta         nm_op
            stz         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            lda         cn
            ora         cn + 1
            bne         :+
            jmp         nm_done
:
            jsr         n_arg0                              ; (The first a string or a character: joined)
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @join
            lda         ex + 1
            cmp         #>CHAR0
            bcc         @sum
            cmp         #>BUILTIN0
            bcs         @sum
            lda         ex
            lsr
            bcs         @sum
@join:
            FARN        2, bo_join
            rts
@sum:
            stz         nm_i                                ; (Fixnums: their sum, if it's one)
            stz         nm_i + 1
@next:
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvc         @lb220
            jmp         nm_general
@lb220:
            clc
            lda         nm_acc
            adc         nm_n
            sta         nm_acc
            lda         nm_acc + 1
            adc         nm_n + 1
            sta         nm_acc + 1
            bvc         @next
            jmp         nm_general

; (- x...): x less the rest; (- x) is -x; (-) is 0
bi_sub:
            lda         #'-'
            sta         nm_op
            stz         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            stz         nm_i
            stz         nm_i + 1
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvc         @lb219
            jmp         nm_general
@lb219:
            MOVW        nm_acc, nm_n
            lda         cn                                  ; ((- x): -x)
            cmp         #1
            bne         @next
            lda         cn + 1
            bne         @next
            jsr         nm_negate
            jmp         nm_done
@next:
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvs         nm_general
            sec
            lda         nm_acc
            sbc         nm_n
            sta         nm_acc
            lda         nm_acc + 1
            sbc         nm_n + 1
            sta         nm_acc + 1
            bvc         @next
            bra         nm_general

; (* x...): the product (1 for none)
bi_mul:
            lda         #'*'
            sta         nm_op
            lda         #1
            sta         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            stz         nm_i
            stz         nm_i + 1
@next:
            jsr         nm_arg
            bcc         :+
            jmp         nm_done
:
            bvs         nm_general
            jsr         nm_times
            lda         nm_over
            beq         @next
            bra         nm_general

; (/ x...): x divided by the rest, exactly; (/ x) is 1/x; (/) is 1
bi_div:
            lda         #'/'
            sta         nm_op
            lda         #1
            sta         nm_acc
            stz         nm_acc + 1
            stz         nm_over
            lda         cn
            ora         cn + 1
            bne         nm_general
            jmp         nm_done

; The operation nm_op over the arguments, any numbers (past a fixnum, or not all fixnums): the first, then it with
; each after it in turn (n_op); (- x) its negation, (/ x) its reciprocal
nm_general:
            jsr         n_enter
            stz         nm_i                                ; (Each a number)
            stz         nm_i + 1
@check:
            lda         nm_i
            cmp         cn
            lda         nm_i + 1
            sbc         cn + 1
            bcs         @numbers
            jsr         nm_argv
            jsr         n_kind
            cmp         #NK_NOT
            bne         @check
            lda         nm_op
            cmp         #'+'
            bne         :+
            LDAX        s_addnum
            jsr         n_error
            jmp         n_leave
:
            jsr         nm_notnum
            jmp         n_leave
@numbers:
            ldx         nm_op                               ; (Its operation)
            ldy         #OP_ADD
            cpx         #'+'
            beq         @op
            iny
            cpx         #'-'
            beq         @op
            iny
            cpx         #'*'
            beq         @op
            iny
@op:
            sty         n_opc2
            stz         nm_i
            stz         nm_i + 1
            jsr         nm_argv                             ; (ex: the first)
            sta         ex
            stx         ex + 1
            lda         cn                                  ; (One: -x, or 1/x)
            cmp         #1
            bne         @next
            lda         cn + 1
            bne         @next
            lda         n_opc2
            cmp         #OP_SUB
            bne         :+
            MOVW        n_x, ex
            jsr         n_neg
            MOVW        ex, hv
            bra         @done
:
            cmp         #OP_DIV
            bne         @done
            MOVW        n_y, ex
            lda         #<FIX(1)
            sta         ex
            stz         ex + 1
            bra         @divide
@next:
            lda         nm_i
            cmp         cn
            lda         nm_i + 1
            sbc         cn + 1
            bcs         @done
            jsr         nm_argv
            sta         n_y
            stx         n_y + 1
@divide:
            MOVW        n_x, ex
            lda         n_opc2
            cmp         #OP_DIV
            bne         @do
            lda         n_y
            ldx         n_y + 1
            jsr         n_zero_p
            bne         @do
            LDAX        s_divzero
            jsr         n_error
            jmp         n_leave
@do:
            lda         n_opc2
            jsr         n_op
            MOVW        ex, hv
            lda         cn                                  ; (Done, if it was (/ x))
            cmp         #1
            bne         @next
            lda         cn + 1
            bne         @next
@done:
            clc
            jmp         n_leave

; .A, .X = the argument nm_i, nm_i one more
nm_argv:
            lda         nm_i
            asl
            tay
            inc         nm_i
            bne         :+
            inc         nm_i + 1
:
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            rts

; The next argument (nm_i): nm_n its number.  OUT: C = 1 if there are no more; V = 1 if it isn't a fixnum
nm_arg:
            lda         nm_i
            cmp         cn
            lda         nm_i + 1
            sbc         cn + 1
            bcs         @rts
            jsr         nm_argv
            sta         nm_n
            stx         nm_n + 1
            lsr
            bcc         @notfix
            lda         nm_n
            ldx         nm_n + 1
            jsr         unfix
            sta         nm_n
            stx         nm_n + 1
            clc
            clv
@rts:
            rts
@notfix:
            clc
            bit         nm_v
            rts
nm_v:       .byte       $40

; nm_acc = -nm_acc (nm_over if it's past 16 bits)
nm_negate:
            sec
            lda         #0
            sbc         nm_acc
            sta         nm_acc
            lda         #0
            sbc         nm_acc + 1
            sta         nm_acc + 1
            bvc         :+
            inc         nm_over
:
            rts

; nm_acc = nm_acc * nm_n (nm_over if it's past 15 bits)
nm_times:
            stz         nm_sign
            lda         nm_acc + 1                          ; (The magnitudes, the sign apart)
            bpl         :+
            jsr         nm_negate
            inc         nm_sign
:
            lda         nm_n + 1
            bpl         :+
            sec
            lda         #0
            sbc         nm_n
            sta         nm_n
            lda         #0
            sbc         nm_n + 1
            sta         nm_n + 1
            lda         nm_sign
            eor         #1
            sta         nm_sign
:
            stz         nm_p                                ; (nm_p: the product, 32 bits)
            stz         nm_p + 1
            stz         nm_p + 2
            stz         nm_p + 3
            ldx         #16
@bit:
            asl         nm_p
            rol         nm_p + 1
            rol         nm_p + 2
            rol         nm_p + 3
            asl         nm_n
            rol         nm_n + 1
            bcc         @no
            clc
            lda         nm_p
            adc         nm_acc
            sta         nm_p
            lda         nm_p + 1
            adc         nm_acc + 1
            sta         nm_p + 1
            bcc         @no
            inc         nm_p + 2
            bne         @no
            inc         nm_p + 3
@no:
            dex
            bne         @bit
            lda         nm_p + 2                            ; (Past 15 bits: too big)
            ora         nm_p + 3
            bne         @over
            lda         nm_p + 1
            bmi         @over
            MOVW        nm_acc, nm_p
            lda         nm_sign
            beq         :+
            jsr         nm_negate
:
            rts
@over:
            inc         nm_over
            rts

; The result: nm_acc, a fixnum (past one: the general way)
nm_done:
            lda         nm_over
            beq         :+
            jmp         nm_general
:
            lda         nm_acc
            ldx         nm_acc + 1
            jsr         fix_make
            bcc         :+
            jmp         nm_general
:
            MOVW        ex, hv
            clc
            rts

; ex = "All parameters to operator '-' must be numbers."
nm_notnum:
            jsr         nu_begin
            LDAX        s_opnum
            jsr         out_text
            lda         nm_op
            jsr         out_byte
            LDAX        s_mustnum
            jsr         out_text
            jmp         nu_end

; (1+ x): (+ x 1).  (1- x): (- x 1)
bi_inc:
            lda         #<bi_add
            ldx         #>bi_add
            bra         nm_plus1
bi_dec:
            lda         #<bi_sub
            ldx         #>bi_sub
nm_plus1:
            sta         ev_jv
            stx         ev_jv + 1
            ldy         #1                                  ; (x and 1: the arguments, here)
:
            lda         (argp),y
            sta         nm_args,y
            dey
            bpl         :-
            lda         #<FIX(1)
            sta         nm_args + 2
            stz         nm_args + 3
            LDAX        nm_args
            sta         argp
            stx         argp + 1
            lda         #2
            sta         cn
            jsr         n_jump
            php                                             ; (Its one argument on the stack, still)
            lda         #1
            sta         cn
            plp
            rts

; (abs x): x's magnitude: x, or -x if it's below 0
bi_abs:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         #NK_NOT
            bne         :+
            LDAX        s_absnum
            jmp         n_error
:
            jsr         n_enter
            MOVW        n_x, ex
            lda         #<FIX(0)
            sta         n_y
            stz         n_y + 1
            jsr         n_cmp
            cmp         #$FF
            bne         @done
            jsr         n_neg
            MOVW        ex, hv
@done:
            clc
            jmp         n_leave

; ---- Conversions

; ex, the argument 0, a real number?  OUT: C = 1 and ex = the error "'fn' expects a real number" (fn: .A, .X) if not
n_real_arg:
            sta         nm_m
            stx         nm_m + 1
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         #NK_CPX
            bcc         @rts
            jsr         nu_begin
            lda         #$27
            jsr         out_byte
            lda         nm_m
            ldx         nm_m + 1
            jsr         out_text
            LDAX        s_expreal
            jsr         out_text
            jsr         nu_end
            sec
@rts:
            rts

; (to-rational x): a real number as a rational (an integer when it's whole)
bi_to_rational:
            LDAX        n_torat
            jsr         n_real_arg
            bcc         :+
            clc
            rts
:
            jsr         n_enter
            jsr         nb_rat
            MOVW        ex, hv
            clc
            jmp         n_leave

; hv = ex, a real number, as a rational in lowest terms (an integer if it's whole)
nb_rat:
            lda         ex
            ldx         ex + 1
            ldy         #4
            jsr         n_load
            jmp         n_rat

; (rational.n x), (rational.d x): a real number's numerator, or denominator, as a rational in lowest terms
bi_rational_n:
            LDAX        n_ratn
            ldy         #0
            bra         nb_ratpart
bi_rational_d:
            LDAX        n_ratd
            ldy         #1
nb_ratpart:
            sty         n_c
            jsr         n_real_arg
            bcc         :+
            clc
            rts
:
            jsr         n_enter
            jsr         nb_rat
            lda         hv
            ldx         hv + 1
            jsr         n_kind
            cmp         #NK_RAT
            beq         @part
            lda         n_c                                 ; (An integer: itself, over 1)
            beq         @int
            lda         #<FIX(1)
            sta         ex
            stz         ex + 1
            bra         @done
@int:
            MOVW        ex, hv
            bra         @done
@part:
            lda         hv
            ldx         hv + 1
            ldy         n_c
            jsr         cell_get
            sta         ex
            stx         ex + 1
@done:
            clc
            jmp         n_leave

; (truncate x): a real number's integer part (toward 0)
bi_truncate:
            LDAX        n_trunc
            jsr         n_real_arg
            bcc         :+
            clc
            rts
:
            jsr         n_enter
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            beq         @done                               ; (An integer: itself)
            jsr         nb_trunc
            MOVW        ex, hv
@done:
            clc
            jmp         n_leave

; hv = ex, a real number, truncated (toward 0): R0
nb_trunc:
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         n_load
            lda         #0
            sta         nra
            lda         #1
            sta         nrb
            lda         #2
            sta         nrq
            lda         #3
            sta         nrr
            jsr         r_divmod
            lda         #2
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            jmp         r_store

; (to-fixed x [places]): a real number as a fixed decimal of places (10, 0 to 10000) places, cut short
bi_to_fixed:
            LDAX        n_tofix
            jsr         n_real_arg
            bcc         :+
            clc
            rts
:
            lda         #10
            sta         nm_p
            stz         nm_p + 1
            lda         cn
            cmp         #2
            bcc         @places
            LDAX        n_tofix                             ; (Its places: a whole number, 0 to 10000)
            sta         nm_m
            stx         nm_m + 1
            LDAX        n_whatpl
            ldy         #2
            jsr         nb_whole
            bcc         :+
            clc
            rts
:
            sta         nm_p
            stx         nm_p + 1
            cpx         #>10001
            bcc         :+
            bne         @outside
            cmp         #<10001
            bcs         @outside
:
            txa
            bpl         @places
@outside:
            jsr         nu_begin                            ; ("'to-fixed': 10001 places is outside 0 to 10000")
            LDAX        s_fixout1
            jsr         out_text
            jsr         n_arg1
            lda         ex
            ldx         ex + 1
            ldy         #0
            FARN        2, print_val
            LDAX        s_fixout2
            jsr         out_text
            jmp         nu_end
@places:
            jsr         n_enter
            jsr         n_arg0
            lda         ex                                  ; (|n|/d: R0/R1; its sign kept)
            ldx         ex + 1
            ldy         #0
            jsr         n_load
            ldx         #0
            lda         r_neg,x
            sta         n_sign
            stz         r_neg,x
            lda         #0                                  ; (Its whole part: R2; the rest, R3)
            sta         nra
            lda         #1
            sta         nrb
            lda         #2
            sta         nrq
            lda         #3
            sta         nrr
            jsr         r_divmod
            stz         n_fdig
            stz         n_fdig + 1
@place:
            ldx         #3                                  ; (While there's more, and places: a digit more)
            lda         r_len,x
            beq         @made
            lda         n_fdig
            cmp         nm_p
            lda         n_fdig + 1
            sbc         nm_p + 1
            bcs         @made
            inc         n_fdig
            bne         :+
            inc         n_fdig + 1
:
            lda         #3
            sta         nrc
            lda         #10
            jsr         r_mulsmall
            lda         #2
            sta         nrc
            lda         #10
            jsr         r_mulsmall
            stz         n_d
@less:
            lda         #3                                  ; (The digit: R1 taken from R3 while it can be)
            sta         nra
            lda         #1
            sta         nrb
            jsr         r_ucmp
            cmp         #$FF
            beq         @digit
            lda         #3
            sta         nra
            sta         nrc
            lda         #1
            sta         nrb
            jsr         r_sub
            inc         n_d
            bra         @less
@digit:
            lda         n_d
            ldx         #0
            ldy         #4
            sty         nrc
            jsr         r_set16
            lda         #2
            sta         nra
            sta         nrc
            lda         #4
            sta         nrb
            jsr         r_add
            bra         @place
@made:
            lda         #2                                  ; (Its digits, R0, with its sign)
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            ldx         #0
            lda         n_sign
            beq         :+
            jsr         r_negate
:
            MOVW        n_pl, n_fdig
            jsr         n_fix0
            MOVW        ex, hv
            clc
            jmp         n_leave

; (complex re im): a complex number of two real numbers (a real one if im is 0)
bi_complex:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            sta         n_k1
            jsr         n_arg1
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            sta         n_k2
            cmp         #NK_NOT
            beq         @notnum
            lda         n_k1
            cmp         #NK_NOT
            beq         @notnum
            cmp         #NK_CPX
            bne         :+
            LDAX        s_cpxre
            jmp         n_error
:
            lda         n_k2
            cmp         #NK_CPX
            bne         :+
            LDAX        s_cpxim
            jmp         n_error
:
            lda         ex                                  ; (im 0: re)
            ldx         ex + 1
            jsr         n_zero_p
            bne         :+
            jsr         n_arg0
            clc
            rts
:
            MOVW        eu, ex
            jsr         n_arg0
            MOVW        et, ex
            lda         #PK_COMPLEX
            jsr         make2
            bcs         @rts
            MOVW        ex, hv
@rts:
            rts
@notnum:
            LDAX        s_cpxnum
            jmp         n_error

; (val s): the number a string (or a character) is written as, spaces around it allowed
bi_val:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_text_of
            bcc         :+
            LDAX        s_valstr
            jmp         n_error
:
            lda         hn + 1                              ; (Longer than 255: not a number)
            bne         @not
            jsr         num_parse
            bcs         @rts
            lda         n_abt
            bne         @rts
            lda         n_res
            bne         :+
            MOVW        ex, hv
            clc
            rts
:
            cmp         #NP_DIVZ
            bne         @not
            LDAX        s_divzero
            jmp         n_error
@not:
            jsr         nu_begin                            ; ("'val': "abc" isn't a number")
            LDAX        s_valnot
            jsr         out_text
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            ldy         #1
            FARN        2, print_val
            LDAX        s_valnot2
            jsr         out_text
            jmp         nu_end
@rts:
            rts

; IN: .A, .X = a value.  OUT: C = 0, hq its bytes (in the window: a string's), hn their count, if it's a string or a
; character (its byte, at n_c); C = 1 if it isn't
n_text_of:
            sta         n_v
            stx         n_v + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @string
            lda         n_v + 1
            and         #$FE
            cmp         #>CHAR0
            bne         @no
            lda         n_v + 1
            lsr
            lda         n_v
            ror
            sta         n_c
            LDAX        n_c
            sta         hq
            stx         hq + 1
            lda         #1
            sta         hn
            stz         hn + 1
            clc
            rts
@string:
            lda         n_v
            ldx         n_v + 1
            jsr         deref
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            sta         n_t
            iny
            lda         (hp),y
            tax
            lda         n_t
            jsr         blob_data
            pla
            sta         hn + 1
            pla
            sta         hn
            MOVW        hq, hp
            clc
            rts
@no:
            sec
            rts

; (fib n): the nth Fibonacci number (n 0 or more): by doubling, from n's top bit down
bi_fib:
            LDAX        n_fib
            sta         nm_m
            stx         nm_m + 1
            LDAX        n_fibn
            ldy         #0
            jsr         nb_whole
            bcc         :+
            clc
            rts
:
            sta         nm_n
            stx         nm_n + 1
            txa
            bpl         :+
            LDAX        s_fibneg
            jmp         n_error
:
            jsr         n_enter
            lda         #0                                  ; (a = 0: R0; b = 1: R1)
            sta         nrc
            tax
            jsr         r_set16
            lda         #1
            sta         nrc
            ldx         #0
            jsr         r_set16
            lda         #16
            sta         n_bits
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
            asl         nm_n                                ; (n's bit: a, b = b, a + b)
            rol         nm_n + 1
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
            dec         n_bits
            beq         @lb218
            jmp         @bit
@lb218:
            lda         #0
            sta         nrc
            jsr         r_store
            MOVW        ex, hv
            clc
            jmp         n_leave

; (random n): an integer from 0 up to, not including, n (an integer, 1 or more): a 16-bit xorshift's bytes, seeded by
; the tick count as it's first wanted; drawn again while they're n or more
bi_random:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            beq         :+
            cmp         #NK_FIX                             ; (A fixed decimal of no places: its digits)
            beq         @lb217
            jmp         @notint
@lb217:
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         cell_get
            cmp         #<FIX(0)
            beq         @lb216
            jmp         @notint
@lb216:
            cpx         #0
            beq         @lb215
            jmp         @notint
@lb215:
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            sta         ex
            stx         ex + 1
:
            jsr         n_enter
            lda         #0                                  ; (n: R0)
            sta         nrc
            lda         ex
            ldx         ex + 1
            jsr         r_load
            ldx         #0
            lda         r_neg,x
            bne         @low
            lda         r_len,x
            bne         :+
@low:
            LDAX        s_randpos
            jsr         n_error
            jmp         n_leave
:
            lda         rn_x                                ; (Seeded: the tick count, not 0)
            ora         rn_x + 1
            bne         @draw
            jsr         TICKS
            eor         #$5A
            sta         rn_x
            txa
            eor         #$A5
            sta         rn_x + 1
            ora         rn_x
            bne         @draw
            inc         rn_x
@draw:
            lda         #1                                  ; (R1: as many bytes as n, its top one masked as n's)
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
            lda         (na),y                              ; (n's top byte: the bits it has)
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
            lda         #1                                  ; (n or more: again)
            sta         nra
            lda         #0
            sta         nrb
            jsr         r_ucmp
            cmp         #$FF
            bne         @draw
            jsr         r_store
            MOVW        ex, hv
            clc
            jmp         n_leave
@notint:
            LDAX        s_randint
            jmp         n_error

; The generator's next byte (rn_x: a 16-bit xorshift, 7 9 8).  OUT: .A.  Keeps .Y
rn_next:
            lda         rn_x + 1
            lsr
            lda         rn_x
            ror
            eor         rn_x + 1
            sta         rn_x + 1
            ror
            eor         rn_x
            sta         rn_x
            eor         rn_x + 1
            sta         rn_x + 1
            rts

; The argument .Y / 2 (0, 1 ...), a whole number for 'fn' (nm_m) as what (.A, .X: "an index", "a count" ...).  OUT: C =
; 0 and .A, .X = it (past 16 bits: -32768 or 32767); C = 1 and ex = the error "'fn': what must be a whole number, not
; x" if it isn't one
nb_whole:
            sta         nm_n
            stx         nm_n + 1
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            lda         ex
            ldx         ex + 1
            jsr         n_whole
            bcc         @rts
            jsr         nu_begin
            lda         #$27
            jsr         out_byte
            lda         nm_m
            ldx         nm_m + 1
            jsr         out_text
            LDAX        s_whole1
            jsr         out_text
            lda         nm_n
            ldx         nm_n + 1
            jsr         out_text
            LDAX        s_whole2
            jsr         out_text
            lda         ex
            ldx         ex + 1
            ldy         #0
            FARN        2, print_val
            jsr         nu_end
            sec
@rts:
            rts

; ---- What the other banks call

; IN: .A, .X = a value.  OUT: C = 0 and .A, .X = it if it's a whole number (an integer, or a fixed decimal of no
; places), past 16 bits -32768 or 32767; C = 1 if it isn't one
n_whole:
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIX
            bne         @no
            lda         n_v
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            cmp         #<FIX(0)
            bne         @no
            cpx         #0
            bne         @no
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            sta         n_v
            stx         n_v + 1
@int:
            lda         n_v
            lsr
            bcc         @big
            lda         n_v
            ldx         n_v + 1
            jsr         unfix
            clc
            rts
@big:
            lda         n_v                                 ; (A bignum: past 16 bits)
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            txa
            bmi         :+
            lda         #$FF
            ldx         #$7F
            clc
            rts
:
            lda         #0
            ldx         #$80
            clc
            rts
@no:
            sec
            rts

; q_a against q_b, two numbers (eqcmp.inc's): q_r = $FF less, 0 the same, 1 more (too big to work out: 1)
n_qcmp:
            jsr         n_enter
            MOVW        n_x, q_a
            MOVW        n_y, q_b
            lda         #1
            sta         q_r
            jsr         n_cmp
            sta         q_r
            clc
            jmp         n_leave

; IN: .A, .X = a value.  OUT: C = 0 and .A = its sign: 0 below 0, 1 0, 2 above (<=>'s); C = 1 if it isn't a number
n_sign3:
            sta         n_x
            stx         n_x + 1
            jsr         n_kind
            cmp         #NK_NOT
            bne         :+
            sec
            rts
:
            jsr         n_enter
            lda         #<FIX(0)
            sta         n_y
            stz         n_y + 1
            jsr         n_cmp
            clc
            adc         #1
            clc
            jmp         n_leave

; (range to), (range from to [step]) of numbers (lists.inc's bl_range, those past fixnums): the numbers from from (0)
; up to, not including, to, by step (1); down, with a negative step
n_range:
            jsr         n_enter
            stz         nm_i
            stz         nm_i + 1
@check:
            lda         nm_i
            cmp         cn
            bcs         @numbers
            jsr         nm_argv
            jsr         n_kind
            cmp         #NK_NOT
            bne         @check
            LDAX        s_rangenum
            jsr         n_error
            jmp         n_leave
@numbers:
            lda         #<FIX(0)                            ; (from: the root stack's; step, eu; to, n_xi)
            ldx         #0
            ldy         cn
            dey
            beq         :+
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
:
            jsr         n_rpush
            lda         #<FIX(1)
            sta         n_yi
            stz         n_yi + 1
            lda         cn
            cmp         #3
            bcc         :+
            jsr         n_arg2
            MOVW        n_yi, ex
:
            ldy         #0
            lda         cn
            cmp         #1
            beq         :+
            ldy         #2
:
            lda         (argp),y
            sta         n_xi
            iny
            lda         (argp),y
            sta         n_xi + 1
            MOVW        n_x, n_yi                           ; (Its direction: step's sign; 0, an error)
            lda         #<FIX(0)
            sta         n_y
            stz         n_y + 1
            jsr         n_cmp
            eor         #$FE                                ; (n_c2: what x against to is, on the way)
            sta         n_c2
            cmp         #$FE
            bne         :+
            LDAX        s_rangestep
            jsr         n_error
            jmp         n_leave
:
            stz         cf                                  ; (The list: cf its first cons, cr its last)
            stz         cf + 1
            stz         cr
            stz         cr + 1
@next:
            ldx         rsp                                 ; (x before to, going up; after it, going down)
            dex
            lda         rs_lo,x
            sta         n_x
            lda         rs_hi,x
            sta         n_x + 1
            MOVW        n_y, n_xi
            jsr         n_cmp
            cmp         n_c2
            beq         @lb214
            jmp         @done
@lb214:
            ldx         rsp                                 ; (x put in the list)
            dex
            lda         rs_lo,x
            sta         et
            lda         rs_hi,x
            sta         et + 1
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         n_make2
            lda         cf
            ora         cf + 1
            bne         :+
            MOVW        cf, hv
            bra         @last
:
            lda         hv
            ldx         hv + 1
            pha
            phx
            MOVW        n_t, hv
            MOVW        hv, cr
            plx
            pla
            ldy         #1
            jsr         cell_set
            MOVW        hv, n_t
@last:
            MOVW        cr, hv
            ldx         rsp                                 ; (x = x + step)
            dex
            lda         rs_lo,x
            sta         n_x
            lda         rs_hi,x
            sta         n_x + 1
            MOVW        n_y, n_yi
            lda         #OP_ADD
            jsr         n_op
            ldx         rsp
            dex
            lda         hv
            sta         rs_lo,x
            lda         hv + 1
            sta         rs_hi,x
            jmp         @next
@done:
            MOVW        ex, cf
            jsr         root_pop
            clc
            jmp         n_leave

; (to-str x base), valout.inc's: the number x in the base (a string)
n_to_str2:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         #NK_NOT
            bne         :+
            LDAX        s_otostr3
            jmp         n_error
:
            jsr         n_arg1
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_tostrbase
            jmp         n_error
:
            jsr         n_enter
            lda         ex
            ldx         ex + 1
            jsr         n_text_of
            lda         hn + 1
            beq         :+
            stz         hn                                  ; (Past 255 bytes: as an empty one, wrong)
:
            jsr         n_arg0                              ; (n_v: the number, n_text_of's used it)
            MOVW        n_v, ex
            jsr         cap_begin
            sta         nu_start
            stx         nu_start + 1
            jsr         n_tobase
            bcs         @error
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_end
            bcs         :+
            MOVW        ex, hv
:
            jmp         n_leave
@error:
            sta         nm_m
            stx         nm_m + 1
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_drop
            jsr         nu_begin
            lda         nm_m
            ldx         nm_m + 1
            jsr         out_text
            lda         nm_m
            cmp         #<s_cpxbase
            bne         :+
            lda         nm_m + 1
            cmp         #>s_cpxbase
            beq         @made
:
            ldx         #0
:
            cpx         n_tl
            beq         :+
            lda         n_text,x
            jsr         out_byte
            inx
            bra         :-
:
            lda         n_lowrad
            beq         @made
            LDAX        s_lowbase
            jsr         out_text
@made:
            jsr         nu_end
            jmp         n_leave

; ---- For the system library (the fifth bank's): integers to and from four bytes (lengths, offsets, times), ticks

; hv = the integer of the four bytes at n_w32 (least first; unsigned).  OUT: C = 1 if there's no room
n_u32:
            jsr         n_enter
            stz         nrc
            jsr         r_c
            ldx         nrc
            stz         r_neg,x
            ldy         #3
:
            lda         n_w32,y
            sta         (nc),y
            dey
            bpl         :-
            lda         #4
            sta         r_len,x
            jsr         r_store
            jmp         n_leave

; n_w32 = the integer .A, .X (0 to 2^32 - 1), or the number equal to one (2.0).  OUT: C = 1 if it isn't one
n_to_u32:
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIX
            bne         @no
            lda         n_v                                 ; (A fixed decimal of no places: its digits)
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            cmp         #<FIX(0)
            bne         @no
            cpx         #0
            bne         @no
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            sta         n_v
            stx         n_v + 1
@int:
            stz         nrc
            lda         n_v
            ldx         n_v + 1
            jsr         r_load
            ldx         #0
            lda         r_neg,x
            bne         @no
            lda         r_len,x
            cmp         #5
            bcs         @no
            sta         n_t
            stz         nrc
            jsr         r_c
            ldy         #0
:
            lda         #0
            cpy         n_t
            bcs         :+
            lda         (nc),y
:
            sta         n_w32,y
            iny
            cpy         #4
            bcc         :--
            clc
            rts
@no:
            sec
            rts

; n_w32 = the ticks in .A, .X seconds, a real number 0 or more (TICK_HZ a second, the fraction's floor: sleep's;
; 2^32 - 1 at most).  OUT: C = 1 if it isn't one
n_ticks:
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            cmp         #NK_CPX
            bcs         @no
            jsr         n_enter
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         n_load                              ; (R0 / R1)
            ldx         #0
            lda         r_neg,x
            bne         @neg
            stz         nrc
            lda         #TICK_HZ
            jsr         r_mulsmall
            stz         nra
            lda         #1
            sta         nrb
            lda         #2
            sta         nrq
            lda         #3
            sta         nrr
            jsr         r_divmod
            ldx         #2                                  ; (R2: past 4 bytes, the most)
            lda         r_len,x
            cmp         #5
            bcc         :+
            lda         #$FF
            sta         n_w32
            sta         n_w32 + 1
            sta         n_w32 + 2
            sta         n_w32 + 3
            clc
            jmp         n_leave
:
            sta         n_t
            lda         #2
            sta         nrc
            jsr         r_c
            ldy         #0
:
            lda         #0
            cpy         n_t
            bcs         :+
            lda         (nc),y
:
            sta         n_w32,y
            iny
            cpy         #4
            bcc         :--
            clc
            jmp         n_leave
@neg:
            sec
            jmp         n_leave
@no:
            sec
            rts

.bss
n_opc2:     .res        1                                   ; (nm_general's operation)
n_w32:      .res        4                                   ; (n_u32's and n_to_u32's four bytes, least first)

.segment "RODATA3"
n_torat:    .byte       "to-rational", 0
n_ratn:     .byte       "rational.n", 0
n_ratd:     .byte       "rational.d", 0
n_trunc:    .byte       "truncate", 0
n_tofix:    .byte       "to-fixed", 0
n_whatpl:   .byte       "a number of places", 0
n_fib:      .byte       "fib", 0
n_fibn:     .byte       "n", 0
s_expreal:  .byte       "' expects a real number", 0
s_fixout1:  .byte       "'to-fixed': ", 0
s_fixout2:  .byte       " places is outside 0 to 10000", 0
s_whole1:   .byte       "': ", 0
s_whole2:   .byte       " must be a whole number, not ", 0
s_cpxnum:   .byte       "One or more parameters passed to 'complex' is not a Number", 0
s_cpxre:    .byte       "Real part of complex must be an int, a rational, or a fixed", 0
s_cpxim:    .byte       "Imaginary part of complex must be an int, a rational, or a fixed", 0
s_valstr:   .byte       "'val' expects a String", 0
s_valnot:   .byte       "'val': ", $22, 0
s_valnot2:  .byte       $22, " isn't a number", 0
s_fibneg:   .byte       "'fib' parameter 'n' cannot be negative", 0
s_rangenum: .byte       "'range' expects Numbers", 0
s_rangestep: .byte      "'range' step cannot be 0", 0
s_otostr3:  .byte       "Only a Number has a base for 'to-str'", 0
s_tostrbase: .byte      "Second 'to-str' parameter must be a String", 0

.segment "CODE3"
