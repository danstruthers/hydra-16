; ****************************************************************************
; numbi.inc - hylang's number built-ins, in its third bank, as danlang's: + - * / (fixnums first, the rest by the
; numbers library: numlib.inc), abs, rational.n, rational.d, to-rational, to-fixed, truncate, complex, val, fib,
; random, base, number-bytes, bytes-number; and what the other banks call: a number's sign, whole value and order,
; range, to-str in a base, format's {base}, the four bytes of the system's calls.  Each built-in gets its values at
; argp (cn of them) and gives back ex (C = 1 if there's no room).

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

; The operation nm_op over the arguments, any numbers (past a fixnum, or not all fixnums), by the library: the first's
; bytes at NL_A, then it with each after it in turn (each at NL_B, the result at NL_A); (- x) its negation, (/ x) its
; reciprocal
nm_general:
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
            jmp         n_error
:
            jmp         nm_notnum
@numbers:
            ldx         #0                                  ; (Its entry)
            lda         nm_op
:
            cmp         nm_ops,x
            beq         :+
            inx
            bra         :-
:
            lda         nm_entry_lo,x
            sta         n_entry
            lda         nm_entry_hi,x
            sta         n_entry + 1
            stz         nm_i
            stz         nm_i + 1
            lda         cn                                  ; (One: -x, or 1/x)
            cmp         #1
            bne         @first
            lda         cn + 1
            bne         @first
            lda         nm_op
            cmp         #'-'
            bne         :+
            jsr         nm_argv
            jsr         nl_one
            NLCALL      NUM_NEG
            jmp         nl_result
:
            cmp         #'/'
            bne         @first
            LDR         nd, NL_A                            ; (1, then it)
            lda         #<FIX(1)
            ldx         #0
            jsr         nl_put
            bra         @next
@first:
            LDR         nd, NL_A
            jsr         nm_argv
            jsr         nl_put
@next:
            lda         nm_i
            cmp         cn
            lda         nm_i + 1
            sbc         cn + 1
            bcs         @done
            LDR         nd, NL_B
            jsr         nm_argv
            jsr         nl_put
            LDR         r0, NL_A
            LDR         r1, NL_B
            LDR         r2, NL_A
            LDR         r3, NL_ROOM
            MOVR        r15, n_entry
            jsr         nl_call
            bcc         :+
            jmp         nl_error
:
            sta         nl_len
            stx         nl_len + 1
            bra         @next
@done:
            LDR         nd, NL_A
            lda         nl_len
            ldx         nl_len + 1
            jmp         nl_ex

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

; (abs x): x's magnitude: x, or -x if it's below 0 (danlang's: a complex number by its order, too)
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
            lda         ex                                  ; (Below 0?)
            ldx         ex + 1
            lsr
            bcc         @lib
            lda         ex + 1
            bpl         @self
            jsr         nm_neg1
            jmp         nl_result
@lib:
            MOVW        q_a, ex
            lda         #<FIX(0)
            sta         q_b
            stz         q_b + 1
            jsr         n_qcmp
            lda         q_r
            cmp         #$FF
            bne         @self
            jsr         nm_neg1
            jmp         nl_result
@self:
            clc
            rts

; ex's negation, by the library: C, .A, .X its answer at NL_R
nm_neg1:
            lda         ex
            ldx         ex + 1
            jsr         nl_one
            NLCALL      NUM_NEG
            rts

; ---- Conversions

; ex, the argument 0, a real number?  OUT: C = 1 and ex = the error "'fn' expects a real number" (fn: .A, .X) if not
n_real_arg:
            sta         nm_m
            stx         nm_m + 1
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
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

; ex, a real number (n_real_arg's error .A, .X), and the library's entry r15 on it: the result, ex
nb_real1:
            jsr         n_real_arg
            bcc         :+
            clc
            rts
:
            lda         ex
            ldx         ex + 1
            jsr         nl_one
            jsr         nl_call
            jmp         nl_result

; (to-rational x): a real number as a rational (an integer when it's whole)
bi_to_rational:
            LDR         r15, NUM_TO_RATIONAL
            LDAX        n_torat
            jmp         nb_real1

; (rational.n x), (rational.d x): a real number's numerator, or denominator, in lowest terms
bi_rational_n:
            LDR         r15, NUM_NUMERATOR
            LDAX        n_ratn
            jmp         nb_real1
bi_rational_d:
            LDR         r15, NUM_DENOMINATOR
            LDAX        n_ratd
            jmp         nb_real1

; (truncate x): a real number's integer part (toward 0)
bi_truncate:
            LDR         r15, NUM_TRUNCATE
            LDAX        n_trunc
            jmp         nb_real1

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
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         nl_one
            LDR         r15, NUM_TO_FIXED
            lda         nm_p
            ldx         nm_p + 1
            jsr         nl_call
            jmp         nl_result

; (complex re im): a complex number of two real numbers (a real one if im is 0)
bi_complex:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            sta         nm_sign
            jsr         n_arg1
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         #NK_NOT
            beq         @notnum
            tax
            lda         nm_sign
            cmp         #NK_NOT
            beq         @notnum
            cmp         #NK_COMPLEX
            bne         :+
            LDAX        s_cpxre
            jmp         n_error
:
            cpx         #NK_COMPLEX
            bne         :+
            LDAX        s_cpxim
            jmp         n_error
:
            jsr         n_arg1                              ; (re at NL_A, im at NL_B)
            MOVW        nl_w, ex
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         nl_two
            NLCALL      NUM_COMPLEX
            jmp         nl_result_a
@notnum:
            LDAX        s_cpxnum
            jmp         n_error

; (val s): the number a string (or a character) is written as, in the base, spaces around it allowed
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
            MOVR        r0, hq                              ; (Its bytes: in the window, its bank there for the
            MOVR        r1, hn                              ;   library to read)
            LDR         r4, 0
            LDR         r2, NL_R
            LDR         r3, NL_ROOM
            LDR         r15, NUM_PARSE
            ldy         #2                                  ; (The whole text: not a program's)
            jsr         nl_call
            bcc         @num
            cmp         #NE_NOTNUM
            beq         @not
            jmp         nl_error
@num:
            jmp         nl_result
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
            pha
            iny
            lda         (hp),y
            tax
            pla
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

; n_text_of's, and a buffer's bytes too (bytes's: hq its bytes, hn their count).  OUT: C = 1 if it's none of those
n_bytes_of:
            sta         n_v
            stx         n_v + 1
            jsr         kind_of
            cmp         #PK_BUFFER
            beq         :+
            lda         n_v
            ldx         n_v + 1
            jmp         n_text_of
:
            lda         n_v                                 ; (Its length, its blob: a string's)
            ldx         n_v + 1
            jsr         deref
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            pla
            sta         hn + 1
            pla
            sta         hn
            MOVW        hq, hp
            clc
            rts

; (fib n): the nth Fibonacci number (n 0 or more)
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
            txa
            bpl         :+
            LDAX        s_fibneg
            jmp         n_error
:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         nl_one
            NLCALL      NUM_FIB
            jmp         nl_result

; (random n): an integer from 0 up to, not including, n (an integer, 1 or more; or a fixed decimal of no places): the
; library's generator (hylang's: a 16-bit xorshift)
bi_random:
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIXED                           ; (A fixed decimal of no places)
            bne         @notint
            lda         ex
            ldx         ex + 1
            jsr         nl_tag
            cmp         #NT_FIXED
            bne         @notint
@int:
            lda         ex                                  ; (Above 0)
            ldx         ex + 1
            jsr         n_sign3
            cmp         #2
            beq         :+
            LDAX        s_randpos
            jmp         n_error
:
            lda         ex
            ldx         ex + 1
            jsr         nl_one
            NLCALL      NUM_RANDOM
            jmp         nl_result
@notint:
            LDAX        s_randint
            jmp         n_error

; IN: .A, .X = a PK_NUMBER cell.  OUT: .A = its number's tag (its first byte)
nl_tag:
            sta         nl_v
            stx         nl_v + 1
            jsr         nl_blob
            lda         (hp)
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

; ---- The base, and the stored format

; (base): the base, its string.  (base b): the base from now on b (a string: "x", "#16r", "c" ...), the one before
; given back
bi_base:
            LDR         r2, NL_S                            ; (The base before)
            LDR         r3, NL_BASE_MAX
            NLCALL      NUM_GET_BASE
            sta         nl_i
            lda         cn
            beq         @old
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_basestr
            jmp         n_error
:
            ldx         nl_i                                ; (The base before: after the new one's room)
:
            lda         NL_S,x
            sta         NL_T,x
            dex
            bpl         :-
            lda         ex
            ldx         ex + 1
            jsr         nl_spec
            LDR         r0, NL_S
            NLCALL      NUM_SET_BASE
            bcc         :+
            jsr         nu_begin                            ; ("'base': Invalid base specifier #q?")
            LDAX        s_basebad
            jsr         out_text
            LDAX        NL_S
            jsr         out_text
            jmp         nu_end
:
            jsr         nl_flags_set
            LDAX        NL_T
            bra         @string
@old:
            LDAX        NL_S
@string:
            sta         hq
            stx         hq + 1
            lda         nl_i
            sta         hn
            stz         hn + 1
            jsr         string_make
            bcs         :+
            MOVW        ex, hv
:
            rts

; (number-bytes n): n in the stored format, its bytes (a list)
bi_number_bytes:
            jsr         n_arg0
            LDR         nd, NL_A
            lda         ex
            ldx         ex + 1
            jsr         nl_put
            bcc         :+
            LDAX        s_nbnum
            jmp         n_error
:
            sta         nl_i                                ; (Its bytes from the last: each consed on)
            stx         nl_i + 1
            stz         cf
            stz         cf + 1
@byte:
            lda         nl_i
            ora         nl_i + 1
            beq         @made
            lda         nl_i
            bne         :+
            dec         nl_i + 1
:
            dec         nl_i
            clc
            lda         #<NL_A
            adc         nl_i
            sta         nd
            lda         #>NL_A
            adc         nl_i + 1
            sta         nd + 1
            lda         (nd)
            ldx         #0
            jsr         fix_ax
            sta         et
            stx         et + 1
            MOVW        eu, cf
            lda         #PK_QCONS
            jsr         make2
            bcs         @rts
            MOVW        cf, hv
            bra         @byte
@made:
            MOVW        ex, cf
            clc
@rts:
            rts

; (bytes-number b): the number b's bytes (a list of bytes, 0-255, or a buffer) are, in the stored format, in its
; one form
bi_bytes_number:
            jsr         n_arg0
            stz         nl_i
            stz         nl_i + 1
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_BUFFER
            bne         @list
            lda         ex                                  ; (A buffer: its bytes, NUM_MAX at most)
            ldx         ex + 1
            jsr         n_bytes_of
            lda         hn + 1
            cmp         #>NUM_MAX
            bcc         :+
            beq         @lb206
            jmp         @bad
@lb206:
            lda         hn
            cmp         #<NUM_MAX
            bcc         @lb205
            jmp         @bad
@lb205:
:
            MOVW        nl_i, hn
            LDR         nd, NL_A
            ldy         #0
            ldx         hn + 1
            beq         @last
@page:
            lda         (hq),y
            sta         (nd),y
            iny
            bne         @page
            inc         hq + 1
            inc         nd + 1
            dex
            bne         @page
@last:
            cpy         hn
            bne         @lb204
            jmp         @check
@lb204:
            lda         (hq),y
            sta         (nd),y
            iny
            bra         @last
@list:
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcc         :+
            LDAX        s_bnlist
            jmp         n_error
:
            sta         n_v                                 ; (n_v: its conses left)
            stx         n_v + 1
            LDR         nd, NL_A
@item:
            lda         n_v
            ora         n_v + 1
            beq         @check
            lda         nl_i + 1                            ; (NUM_MAX at most)
            cmp         #>NUM_MAX
            bcc         :+
            lda         nl_i
            cmp         #<NUM_MAX
            bcc         @lb203
            jmp         @bad
@lb203:
:
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            sta         n_x
            stx         n_x + 1
            lsr                                             ; (A byte: a fixnum 0-255)
            bcc         @nonbyte
            lda         n_x
            ldx         n_x + 1
            jsr         unfix
            cpx         #0
            bne         @nonbyte
            ldy         nl_i
            sta         (nd),y
            inc         nl_i
            bne         :+
            inc         nl_i + 1
            inc         nd + 1
:
            lda         n_v
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            sta         n_v
            stx         n_v + 1
            bra         @item
@check:
            LDR         r0, NL_A                            ; (Its one form: BYTES's)
            MOVR        r1, nl_i
            LDR         r2, NL_R
            LDR         r3, NL_ROOM
            NLCALL      NUM_BYTES
            bcs         @bad
            jmp         nl_result
@bad:
            LDAX        s_bnbad
            jmp         n_error
@nonbyte:
            LDAX        s_bnbyte
            jmp         n_error

; ---- The math functions: the math library's (the module math; NUMBERS.md, step 4): exact when the answer is, else a
; fixed decimal of (digits) significant digits, correctly rounded, as danlang's are

MF_SQRT         = 0             ; (Each function's number: its entry, its .Y, its name)
MF_EXP          = 1
MF_LOG          = 2
MF_ATAN         = 3
MF_SIN          = 4
MF_COS          = 5
MF_TAN          = 6
MF_PI           = 7
MF_POW          = 8
MF_DIGITS       = 9

; (sqrt x), (exp x), (log x), (atan x), (sin x), (cos x), (tan x): x a real number
bi_sqrt:
            lda         #MF_SQRT
            bra         nm_math1
bi_exp:
            lda         #MF_EXP
            bra         nm_math1
bi_log:
            lda         #MF_LOG
            bra         nm_math1
bi_atan:
            lda         #MF_ATAN
            bra         nm_math1
bi_sin:
            lda         #MF_SIN
            bra         nm_math1
bi_cos:
            lda         #MF_COS
            bra         nm_math1
bi_tan:
            lda         #MF_TAN
nm_math1:
            sta         n_mf
            jsr         n_enter
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         #NK_NOT
            bne         :+
            LDAX        s_mexpects                          ; ("'sqrt' expects a number")
            jmp         nm_mtext
:
            lda         ex
            ldx         ex + 1
            jsr         nl_one
            LDR         r3, NL_ROOM
; The math library's entry for n_mf called (r0 ... set): its result the value (at NL_R)
nm_mcall:
            lda         nl_math                             ; (The library there?)
            bne         :+
            LDAX        s_nomath
            jmp         nm_mtext
:
            ldx         n_mf
            lda         mf_lo,x
            sta         r15
            lda         mf_hi,x
            sta         r15 + 1
            ldy         mf_y,x
            jsr         nl_mcall
            bcs         nm_merr
            jsr         nl_result
            jmp         n_leave
; Its error (.A, NE_*): danlang's message
nm_merr:
            cmp         #NE_REAL
            bne         :+
            LDAX        s_mreal                             ; ("'sqrt' expects a real number")
            jmp         nm_mtext
:
            cmp         #NE_DOMAIN
            bne         @other
            ldx         n_mf
            cpx         #MF_LOG
            bne         :+
            LDAX        s_mlog0
            jsr         n_error
            jmp         n_fail
:
            LDAX        s_mrange                            ; ("'exp': out of range")
            jmp         nm_mtext
@other:
            cmp         #NE_NOTNUM
            bne         :+
            LDAX        s_mexpects
            jmp         nm_mtext
:
            jsr         nl_error
            jmp         n_fail

; An error: "'", the function's name, then the text at .A, .X
nm_mtext:
            sta         n_mt
            stx         n_mt + 1
            jsr         nu_begin
            lda         #$27
            jsr         out_byte
            ldx         n_mf
            lda         mf_name_hi,x
            tay
            lda         mf_name_lo,x
            tax
            tya
            pha
            txa
            plx
            jsr         out_text
            lda         n_mt
            ldx         n_mt + 1
            jsr         out_text
            jsr         nu_end
            jmp         n_fail

; (pi)
bi_pi:
            lda         #MF_PI
            sta         n_mf
            jsr         n_enter
            LDR         r2, NL_R
            LDR         r3, NL_ROOM
            jmp         nm_mcall

; (pow x y): x to the power y.  A whole y's power exact (any x); else x real and 0 or more: exact when its root is
; rational, else a fixed decimal of (digits) significant digits
bi_pow:
            lda         #MF_POW
            sta         n_mf
            jsr         n_enter
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_kind
            cmp         #NK_NOT
            beq         @nums
            ldy         #2
            jsr         nm_argv2
            lda         nl_w
            ldx         nl_w + 1
            jsr         n_kind
            cmp         #NK_NOT
            bne         :+
@nums:
            LDAX        s_mnums                             ; ("'pow' expects numbers")
            jmp         nm_mtext
:
            cmp         #NK_COMPLEX
            bne         :+
            LDAX        s_mpower                            ; ("'pow' expects a real power")
            jmp         nm_mtext
:
            lda         nl_math
            bne         :+
            LDAX        s_nomath
            jmp         nm_mtext
:
            lda         ex
            ldx         ex + 1
            jsr         nl_two                              ; (x at NL_A, y at NL_B; the result at NL_A)
            LDR         r15, MATH_RPOW
            jsr         nl_mcall
            bcs         @err
            clc
            jsr         nl_result_a
            jmp         n_leave
@err:
            cmp         #NE_REAL                            ; (x complex, y not whole)
            bne         :+
            LDAX        s_mpowreal
            jmp         nm_mtext
:
            cmp         #NE_DOMAIN                          ; (x below 0, y not whole; or y log x too far)
            bne         @other
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
            jsr         n_sign3
            cmp         #0
            bne         :+
            LDAX        s_mpowneg
            jmp         nm_mtext
:
            LDAX        s_mrange
            jmp         nm_mtext
@other:
            jsr         nl_error
            jmp         n_fail

; nl_w = argument .Y / 2's value (argp's)
nm_argv2:
            lda         (argp),y
            sta         nl_w
            iny
            lda         (argp),y
            sta         nl_w + 1
            rts

; (digits): the precision of the math functions' results when they aren't exact (12 at the start); (digits n): it set
; to n (a whole number from 1 to 100), the one before given back
bi_digits:
            lda         #MF_DIGITS
            sta         n_mf
            jsr         n_enter
            lda         nl_math
            bne         :+
            LDAX        s_nomath
            jmp         nm_mtext
:
            lda         #0
            ldx         cn
            beq         @call
            jsr         n_arg0                              ; (A whole number from 1 to 100: 12.0 too)
            lda         ex
            ldx         ex + 1
            jsr         n_whole
            bcs         @bad
            cpx         #0
            bne         @bad
            cmp         #0
            beq         @bad
            cmp         #101
            bcs         @bad
@call:
            pha
            lda         nl_bank
            sta         r13
            lda         nl_math
            sta         r14
            LDR         r15, MATH_DIGITS
            pla
            jsr         XCALL
            ldx         #0
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            jmp         n_leave
@bad:
            LDAX        s_mdigits
            jsr         n_error
            jmp         n_fail

; ---- What the other banks call

; IN: .A, .X = a value.  OUT: C = 0 and .A, .X = it if it's a whole number (an integer, or a fixed decimal of no
; places), past 16 bits -32768 or 32767; C = 1 if it isn't one
n_whole:
            sta         n_v
            stx         n_v + 1
            lsr
            bcc         @cell
            lda         n_v
            ldx         n_v + 1
            jsr         unfix
            clc
            rts
@cell:
            lda         n_v
            ldx         n_v + 1
            jsr         kind_of
            cmp         #PK_NUMBER
            bne         @no
            lda         n_v
            ldx         n_v + 1
            jsr         nl_tag
            ldy         #0
            cmp         #NT_FIXED                           ; (A fixed decimal of no places: its digits, after its
            bne         @int                                ;   tag)
            iny
            lda         (hp),y
@int:
            cmp         #NT_FIXED                           ; (Not an integer)
            bcs         @no
            cmp         #NT_POS                             ; (-64 to 63: the tag, 7 bits)
            bcs         @big
            ldx         #0
            cmp         #$40
            bcc         :+
            ora         #$80
            ldx         #$FF
:
            clc
            rts
@big:
            cmp         #NT_LONG_POS                        ; (Its sign: a long one's bit 0, else bit 4)
            bcc         :+
            and         #1
            sta         n_t
            bra         @past
:
            and         #NT_NEG - NT_POS
            sta         n_t
            lda         (hp),y                              ; (A byte or two: its value, if it's in 16 bits)
            and         #1
            pha
            iny
            lda         (hp),y
            sta         n_x
            stz         n_x + 1
            pla
            beq         :+
            iny
            lda         (hp),y
            bmi         @past
            sta         n_x + 1
:
            lda         n_t
            beq         :+
            sec
            lda         #0
            sbc         n_x
            sta         n_x
            lda         #0
            sbc         n_x + 1
            sta         n_x + 1
:
            lda         n_x
            ldx         n_x + 1
            clc
            rts
@past:
            lda         n_t
            bne         :+
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
            lda         #1
            sta         q_r
            MOVW        nl_w, q_b
            lda         q_a
            ldx         q_a + 1
            jsr         nl_two
            bcs         @rts
            NLCALL      NUM_CMP
            bcs         @rts
            sta         q_r
@rts:
            clc
            rts

; IN: .A, .X = a value.  OUT: C = 0 and .A = its sign: 0 below 0, 1 0, 2 above (<=>'s); C = 1 if it isn't a number
n_sign3:
            sta         n_x
            stx         n_x + 1
            lsr
            bcc         @cell
            ldx         n_x + 1                             ; (A fixnum: by its value)
            bmi         @neg
            lda         n_x
            cmp         #<FIX(0)
            bne         @pos
            txa
            bne         @pos
            lda         #1
            clc
            rts
@neg:
            lda         #0
            clc
            rts
@pos:
            lda         #2
            clc
            rts
@cell:
            lda         n_x
            ldx         n_x + 1
            jsr         nl_one
            bcs         @rts
            NLCALL      NUM_KIND
            txa
            clc
            adc         #1
            clc
@rts:
            rts

; (range to), (range from to [step]) of numbers (lists.inc's bl_range, those past fixnums): the numbers from from (0)
; up to, not including, to, by step (1); down, with a negative step.  x at NL_A, to at NL_B, step at NL_R
n_range:
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
            jmp         n_error
@numbers:
            LDR         nd, NL_A                            ; (x: from, or 0)
            lda         #<FIX(0)
            ldx         #0
            ldy         cn
            dey
            beq         :+
            jsr         n_arg0
            lda         ex
            ldx         ex + 1
:
            jsr         nl_put
            sta         n_xl
            stx         n_xl + 1
            LDR         nd, NL_R                            ; (step: 1, or the third)
            lda         #<FIX(1)
            ldx         #0
            ldy         cn
            cpy         #3
            bcc         :+
            jsr         n_arg2
            lda         ex
            ldx         ex + 1
:
            jsr         nl_put
            ldy         #0                                  ; (to)
            lda         cn
            cmp         #1
            beq         :+
            ldy         #2
:
            LDR         nd, NL_B
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            jsr         nl_put
            LDR         r0, NL_R                            ; (Its direction: step's sign; 0, an error)
            NLCALL      NUM_KIND
            txa
            bne         :+
            LDAX        s_rangestep
            jmp         n_error
:
            eor         #$FE                                ; (What x against to is, on the way: -step's sign)
            sta         n_c
            stz         cf                                  ; (The list: cf its first cons, cr its last)
            stz         cf + 1
            stz         cr
            stz         cr + 1
@next:
            LDR         r0, NL_A                            ; (x before to, going up; after it, going down)
            LDR         r1, NL_B
            NLCALL      NUM_CMP
            bcc         :+
            jmp         nl_error
:
            cmp         n_c
            beq         @lb202
            jmp         @done
@lb202:
            LDR         nd, NL_A                            ; (x's value, a cons at the list's end)
            lda         n_xl
            ldx         n_xl + 1
            jsr         nl_value
            bcc         @lb201
            jmp         @rts
@lb201:
            MOVW        et, hv
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         make2
            bcc         @lb200
            jmp         @rts
@lb200:
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
            MOVW        n_v, hv
            MOVW        hv, cr
            plx
            pla
            ldy         #1
            jsr         cell_set
            MOVW        hv, n_v
@last:
            MOVW        cr, hv
            LDR         r0, NL_A                            ; (x = x + step)
            LDR         r1, NL_R
            LDR         r2, NL_A
            LDR         r3, NL_ROOM
            NLCALL      NUM_ADD
            bcc         :+
            jmp         nl_error
:
            sta         n_xl
            stx         n_xl + 1
            jmp         @next
@done:
            MOVW        ex, cf
            clc
@rts:
            rts

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
            lda         ex
            ldx         ex + 1
            jsr         nl_spec
            jsr         n_arg0
            jsr         cap_begin
            sta         nu_start
            stx         nu_start + 1
            lda         #1
            sta         nl_spc
            lda         ex
            ldx         ex + 1
            jsr         nl_display
            bcs         @error
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_end
            bcs         :+
            MOVW        ex, hv
:
            rts
@error:
            MOVW        nl_v, ex                            ; (The error: past the capture given up)
            lda         nu_start
            ldx         nu_start + 1
            jsr         cap_drop
            MOVW        ex, nl_v
            clc
            rts

; The value .A, .X out in the base whose string is at NL_S (format's {base}, valout.inc's).  OUT: C = 1 and ex the
; error if it isn't a number ("'format's {x} is a base, for a number") or the string names no base
n_fmt_base:
            sta         n_x
            stx         n_x + 1
            jsr         n_kind
            cmp         #NK_NOT
            beq         @notnum
            lda         #1
            sta         nl_spc
            lda         n_x
            ldx         n_x + 1
            jmp         nl_display
@notnum:
            jsr         nu_begin
            LDAX        s_fmtnum1
            jsr         out_text
            LDAX        NL_S
            jsr         out_text
            LDAX        s_fmtnum2
            jsr         out_text
            jsr         nu_end
            sec
            rts

; IN: .A, .X = a whole number (n_whole's).  OUT: hv = the integer it equals (a fixed decimal of no places: its digits)
; C = 1 if there's no room (hashes.inc's: a key)
n_intval:
            sta         n_v
            stx         n_v + 1
            sta         hv
            stx         hv + 1
            lsr
            bcs         @rts
            lda         n_v                                 ; (An integer: itself)
            ldx         n_v + 1
            jsr         nl_tag
            cmp         #NT_FIXED
            bcc         @rts
            lda         n_v
            ldx         n_v + 1
            jsr         nl_one
            NLCALL      NUM_TRUNCATE
            bcs         @rts
            pha
            LDR         nd, NL_R
            pla
            jmp         nl_value
@rts:
            clc
            rts

; ---- For the system library (the fifth bank's): integers to and from four bytes (lengths, offsets, times), ticks.
; These are worked as a system call's registers are set (hysys.inc's sy_put) and read: the call registers r0-r15 kept
; across the library's calls (nl_rsave, nl_rrest)

; hv = the integer of the four bytes at n_w32 (least first; unsigned).  OUT: C = 1 if there's no room
n_u32:
            lda         n_w32 + 3                           ; (0-16383: a fixnum, here)
            ora         n_w32 + 2
            bne         @lib
            lda         n_w32 + 1
            cmp         #$40
            bcs         @lib
            tax
            lda         n_w32
            jsr         fix_ax
            sta         hv
            stx         hv + 1
            clc
            rts
@lib:
            jsr         nl_rsave
            jsr         @made
            jmp         nl_rrest
@made:
            MOVR        r0, n_w32
            MOVR        r1, n_w32 + 2
            LDR         r2, NL_R
            LDR         r3, NL_ROOM
            LDR         r15, NUM_FROM_INT
            ldy         #0
            jsr         nl_call
            pha
            LDR         nd, NL_R
            pla
            jmp         nl_value

; n_w32 = the integer .A, .X (0 to 2^32 - 1), or the number equal to one (2.0).  OUT: C = 1 if it isn't one.
; n_to_i32: -2^31 to 2^32 - 1, a negative one's two's complement (an offset: SEEK's)
n_to_i32:
            sec
            bra         :+
n_to_u32:
            clc
:
            ror         n_sgn
            sta         n_v
            stx         n_v + 1
            lsr
            bcc         @cell
            lda         n_v                                 ; (A fixnum: here)
            jsr         unfix
            sta         n_w32
            stx         n_w32 + 1
            ldy         #0
            txa
            bpl         :+
            bit         n_sgn                               ; (Below 0: n_to_i32's)
            bpl         @no
            dey
:
            sty         n_w32 + 2
            sty         n_w32 + 3
            clc
            rts
@cell:
            lda         n_v
            ldx         n_v + 1
            jsr         n_kind
            beq         :+
            cmp         #NK_FIXED
            bne         @no
:
            jsr         nl_rsave
            lda         n_v
            ldx         n_v + 1
            jsr         nl_one
            bcs         @nolib
            NLCALL      NUM_TO_INT
            bcs         @nolib
            tay
            ldx         #3
:
            lda         r4,x
            sta         n_w32,x
            dex
            bpl         :-
            jsr         nl_rrest
            cpy         #2                                  ; (Past 32 bits either way)
            beq         @no
            cpy         #1                                  ; (2^31 to 2^32 - 1: either's)
            beq         @ok
            lda         n_w32 + 3                           ; (Below 0: n_to_i32's)
            bpl         @ok
            bit         n_sgn
            bpl         @no
@ok:
            clc
            rts
@nolib:
            jsr         nl_rrest
@no:
            sec
            rts

; n_w32 = the ticks in .A, .X seconds, a real number 0 or more (TICK_HZ a second, the fraction's floor: sleep's;
; 2^32 - 1 at most).  OUT: C = 1 if it isn't one
n_ticks:
            jsr         nl_rsave
            jsr         @ticks
            jmp         nl_rrest
@ticks:
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            bcs         @no
            lda         n_v                                 ; (Below 0: not one)
            ldx         n_v + 1
            jsr         n_sign3
            cmp         #0
            beq         @no
            lda         #<FIX(TICK_HZ)                      ; (x * TICK_HZ, its floor)
            sta         nl_w
            lda         #>FIX(TICK_HZ)
            sta         nl_w + 1
            lda         n_v
            ldx         n_v + 1
            jsr         nl_two
            NLCALL      NUM_MUL
            bcs         @most
            LDR         r0, NL_A
            LDR         r2, NL_R
            NLCALL      NUM_FLOOR
            LDR         r0, NL_R
            NLCALL      NUM_TO_INT
            cmp         #2
            beq         @most
            ldx         #3
:
            lda         r4,x
            sta         n_w32,x
            dex
            bpl         :-
            clc
            rts
@most:
            lda         #$FF                                ; (Past 32 bits: the most)
            sta         n_w32
            sta         n_w32 + 1
            sta         n_w32 + 2
            sta         n_w32 + 3
            clc
            rts
@no:
            sec
            rts

.bss
n_mf:       .res        1                                   ; (A math function's number: MF_*)
n_mt:       .res        2                                   ; (Its error's text)
n_entry:    .res        2                                   ; (nm_general's entry)
n_w32:      .res        4                                   ; (n_u32's and n_to_u32's four bytes, least first)
n_sgn:      .res        1                                   ; (n_to_i32's: bit 7, a negative one taken)
n_v:        .res        2                                   ; (A value)
n_x:        .res        2
n_t:        .res        2                                   ; (A count, a part)
n_c:        .res        1                                   ; (A byte; range's direction)
n_xl:       .res        2                                   ; (range's x: its bytes' count)

.segment "RODATA3"
mf_lo:      .lobytes    MATH_SQRT, MATH_EXP, MATH_LOG, MATH_ATAN, MATH_TRIG, MATH_TRIG, MATH_TRIG, MATH_PI, MATH_RPOW, MATH_DIGITS
mf_hi:      .hibytes    MATH_SQRT, MATH_EXP, MATH_LOG, MATH_ATAN, MATH_TRIG, MATH_TRIG, MATH_TRIG, MATH_PI, MATH_RPOW, MATH_DIGITS
mf_y:       .byte       0, 0, 0, 0, 0, 1, 2, 0, 0, 0
mf_name_lo: .lobytes    s_fsqrt, s_fexp, s_flog, s_fatan, s_fsin, s_fcos, s_ftan, s_fpi, s_fpow, s_fdigits
mf_name_hi: .hibytes    s_fsqrt, s_fexp, s_flog, s_fatan, s_fsin, s_fcos, s_ftan, s_fpi, s_fpow, s_fdigits
s_fsqrt:    .byte       "sqrt", 0
s_fexp:     .byte       "exp", 0
s_flog:     .byte       "log", 0
s_fatan:    .byte       "atan", 0
s_fsin:     .byte       "sin", 0
s_fcos:     .byte       "cos", 0
s_ftan:     .byte       "tan", 0
s_fpi:      .byte       "pi", 0
s_fpow:     .byte       "pow", 0
s_fdigits:  .byte       "digits", 0
s_mexpects: .byte       "' expects a number", 0
s_mreal:    .byte       "' expects a real number", 0
s_mrange:   .byte       "': out of range", 0
s_mlog0:    .byte       "'log': the logarithm of 0", 0
s_mnums:    .byte       "' expects numbers", 0
s_mpower:   .byte       "' expects a real power", 0
s_mpowreal: .byte       "' expects a real number when its power isn't whole", 0
s_mpowneg:  .byte       "': a negative number to a power that isn't whole", 0
s_mdigits:  .byte       "'digits' takes a whole number from 1 to 100", 0
s_nomath:   .byte       "': the math library (the module math) isn't there", 0
nm_ops:     .byte       "+-*/"
nm_entry_lo: .lobytes   NUM_ADD, NUM_SUB, NUM_MUL, NUM_DIV
nm_entry_hi: .hibytes   NUM_ADD, NUM_SUB, NUM_MUL, NUM_DIV
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
s_badbase:  .byte       "Invalid base specifier ", 0
s_basestr:  .byte       "'base' expects a string: a base (", $22, "x", $22, ", ", $22, "#16r", $22, ", ", $22, "c", $22, " ...)", 0
s_basebad:  .byte       "'base': Invalid base specifier ", 0
s_nbnum:    .byte       "'number-bytes' expects a number", 0
s_bnlist:   .byte       "'bytes-number' expects a list of bytes or a buffer", 0
s_bnbyte:   .byte       "'bytes-number' expects bytes, 0-255", 0
s_bnbad:    .byte       "'bytes-number': not a number in the stored format", 0
s_fmtbase:  .byte       "'format': Invalid base specifier ", 0
s_fmtnum1:  .byte       "'format's {", 0
s_fmtnum2:  .byte       "} is a base, for a number", 0

.segment "CODE3"
