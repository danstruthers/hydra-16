; ****************************************************************************
; fns4.inc - the numbers' built-in functions (the fourth bank: fn.inc's F_B4 group), on the libraries: each takes its
; arguments from the value stack and leaves its value in the first's place.

.bss
rnd_last:   .res        64                                  ; RND's last (its bytes: RND(0)'s), its length
rnd_len:    .res        1
f_base:     .res        8                                   ; (A base string made: HEX$'s "x")
f_bstr:     .res        BASE_ROOM                           ; (A base string: STR$'s, BASE$'s)
f_sp:       .res        1                                   ; (STR$'s space before a number not below 0)

.segment "CODE4"

fn_jt4:
            .word       f_abs, f_atn, f_bit, f_cint, f_complex, f_cos, f_cvn, f_den, f_digits, f_exp, f_fib, f_fix
            .word       f_fixed, f_gcd, f_imag, f_int, f_log, f_numer, f_pi, f_rational, f_real, f_rnd, f_sgn, f_shl
            .word       f_shr, f_sin, f_sqr, f_tan, f_val, f_str, f_hex, f_oct, f_mkn, f_bases
.assert     * - fn_jt4 = (F_B5 - F_B4) * 2, error, "fn_jt4 and fn.inc's F_B4 group differ"

; The top value made the operand (r0), the result's place NS_R (r2, r3)
one_op:
            jsr         top_vt
            ldy         #0
            jsr         operand
r2_res:
            LDW         r2, NS_R
            LDW         r3, NUM_MAX
            rts

; The top two: operands r0 and r1
two_ops:
            jsr         two_vals
            ldy         #0
            jsr         operand
            MOVW        vt, vu
            ldy         #1
            jsr         operand
            jsr         two_vals
            bra         r2_res

; The math library there?  Else Illegal function call
need_math:
            lda         math_mod
            bne         :+
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error
:
            rts

; An integer at the top (C = 0), its value unchanged by the function (INT, FIX, CINT of an integer)
top_int:
            jsr         top_vt
            lda         (vt)
            cmp         #1
            rts

f_abs:
            jsr         top_int
            bcs         @lib
            ldy         #4                                  ; (An integer: its sign; -2^31's isn't one)
            lda         (vt),y
            bpl         @done
            cmp         #$80
            bne         :+
            dey
            lda         (vt),y
            dey
            ora         (vt),y
            dey
            ora         (vt),y
            beq         @lib
:
            sec
            ldy         #1
            ldx         #4
:
            lda         #0
            sbc         (vt),y
            sta         (vt),y
            iny
            dex
            bne         :-
@done:
            rts
@lib:
            jsr         one_op
            NUMCALL     NUM_ABS
            jmp         n_result1

f_int:
            jsr         top_int
            bcc         :+
            jsr         one_op
            NUMCALL     NUM_FLOOR
            jmp         n_result1
:
            rts
f_fix:
            jsr         top_int
            bcc         :+
            jsr         one_op
            NUMCALL     NUM_TRUNCATE
            jmp         n_result1
:
            rts
f_cint:
            jsr         top_int
            bcc         :+
            jsr         one_op
            NUMCALL     NUM_ROUND
            jmp         n_result1
:
            rts

f_den:
            jsr         one_op
            NUMCALL     NUM_DENOMINATOR
            jmp         n_result1
f_numer:
            jsr         one_op
            NUMCALL     NUM_NUMERATOR
            jmp         n_result1
f_rational:
            jsr         one_op
            NUMCALL     NUM_TO_RATIONAL
            jmp         n_result1
f_fib:
            jsr         one_op
            NUMCALL     NUM_FIB
            jmp         n_result1
f_real:
            ldy         #0
            bra         :+
f_imag:
            ldy         #1
:
            phy
            jsr         one_op
            ply
            NUMCALL     NUM_PART
            jmp         n_result1

f_sqr:
            jsr         need_math
            jsr         one_op
            MATHCALL    MATH_SQRT
            jmp         n_result1
f_exp:
            jsr         need_math
            jsr         one_op
            MATHCALL    MATH_EXP
            jmp         n_result1
f_log:
            jsr         need_math
            jsr         one_op
            MATHCALL    MATH_LOG
            jmp         n_result1
f_atn:
            jsr         need_math
            jsr         one_op
            MATHCALL    MATH_ATAN
            jmp         n_result1
f_sin:
            ldy         #0
            bra         f_trig
f_cos:
            ldy         #1
            bra         f_trig
f_tan:
            ldy         #2
f_trig:
            phy
            jsr         need_math
            jsr         one_op
            ply
            MATHCALL    MATH_TRIG
            jmp         n_result1

; PI: pushed
f_pi:
            jsr         need_math
            jsr         push_zero                           ; (Its place)
            jsr         r2_res
            lda         nsb
            sta         RAM_BANK
            MATHCALL    MATH_PI
            jmp         n_result1

; A 0 pushed (a place for a result)
push_zero:
            ldy         #4
            lda         #0
:
            sta         (vsp),y
            dey
            bpl         :-
            jmp         vsp_up

f_complex:
            jsr         two_ops
            NUMCALL     NUM_COMPLEX
            jmp         n_result
f_gcd:
            jsr         two_ops
            jsr         round_ops
            NUMCALL     NUM_GCD
            jmp         n_result
f_shl:
            ldy         #NBIT_SHL
            bra         f_bits
f_shr:
            ldy         #NBIT_SHR
f_bits:
            phy
            jsr         two_ops
            jsr         round_ops
            ply
            NUMCALL     NUM_BITS
            jmp         n_result
; BIT(x, n): bit n of x, 0 or 1
f_bit:
            jsr         two_ops
            jsr         round_ops
            ldy         #NBIT_TEST
            NUMCALL     NUM_BITS
            bcc         :+
            jmp         n_fail
:
            pha
            jsr         code_sel
            jsr         two_vals
            pla
            jsr         int_a_vt                            ; (vt: the bit, an integer)
            MOVW        vsp, vu
            rts

; The integer .A (0-255) at vt
int_a_vt:
            pha
            lda         #VT_INT
            sta         (vt)
            ldy         #1
            pla
            sta         (vt),y
            lda         #0
            iny
            sta         (vt),y
            iny
            sta         (vt),y
            iny
            sta         (vt),y
            rts

; FIXED(x, n): x cut to n places, a fixed decimal
f_fixed:
            jsr         two_vals                            ; (n: an integer)
            MOVW        vt, vu
            jsr         pop_int16
            jsr         one_op
            lda         t0
            ldx         t0 + 1
            NUMCALL     NUM_TO_FIXED
            jmp         n_result1

; SGN(x): -1, 0, 1 (a complex number's real part's)
f_sgn:
            jsr         top_vt
            jsr         num_sign_vt
            pha
            jsr         top_vt
            pla
            pha
            jsr         int_a_vt
            pla
            cmp         #$FF                                ; (-1: its bytes all $FF)
            bne         :+
            ldy         #1
            lda         #$FF
            sta         (vt),y
            iny
            sta         (vt),y
            iny
            sta         (vt),y
            iny
            sta         (vt),y
:
            jmp         code_sel

; RND, RND(x): (x's count pushed: 0 or 1) a fixed decimal from 0 up to 1, of DIGITS digits; RND(0) the last again;
; RND(x) below 0 seeds the generator with x first
f_rnd:
            jsr         pop_int16                           ; (Its arguments' count)
            lda         t0
            beq         @next
            jsr         top_vt                              ; (x: its sign)
            jsr         num_sign_vt
            cmp         #0
            bne         @lb900
            jmp         @last
@lb900:
            cmp         #1
            beq         @drop
            jsr         top_vt                              ; (Below 0: a seed of its low 16 bits)
            jsr         pop_int_any
            MOVW        r0, t0
            NUMCALL     NUM_SEED
@drop:
            jsr         pop_vt
@next:
            jsr         digits_sync
            jsr         push_zero
            jsr         r2_res
            stz         nia                                 ; (n: 0, a fixed decimal from 0 up to 1)
            LDW         r0, nia
            lda         nsb
            sta         RAM_BANK
            NUMCALL     NUM_RANDOM
            bcc         :+
            jmp         n_fail
:
            sta         rnd_len                             ; (Kept: RND(0)'s)
            stx         nlen + 1
            sta         nlen
            ldy         #0
:
            lda         NS_R,y
            sta         rnd_last,y
            iny
            cpy         rnd_len
            bne         :-
            jsr         top_vt
            jsr         result_vt
            jmp         code_sel
@last:
            jsr         pop_vt                              ; (The last again: its bytes, to NS_R)
            jsr         push_zero
            lda         nsb
            sta         RAM_BANK
            ldy         #0
:
            lda         rnd_last,y
            sta         NS_R,y
            iny
            cpy         rnd_len
            bne         :-
            lda         rnd_len
            sta         nlen
            stz         nlen + 1
            LDW         r2, NS_R
            jsr         top_vt
            jsr         result_vt
            jmp         code_sel

; t0 = the top value's low 16 bits (an integer, or a number made one: rounded), taken off
pop_int_any:
            jsr         top_vt
            lda         (vt)
            beq         :+
            jsr         num_to_int
:
            jsr         pop_vt
            ldy         #1
            lda         (vt),y
            sta         t0
            iny
            lda         (vt),y
            sta         t0 + 1
            rts

; ---- The fourth bank's statements (fn.inc: ST_B4 on)
st_disp4:
            tax
            jmp         (st_jt4,x)
st_jt4:
            .word       st_randomize, st_base, st_digits
.assert     * - st_jt4 = (ST_B5 - ST_B4) * 2, error, "st_jt4 and fn.inc's ST_B4 group differ"

; RANDOMIZE n: the generator seeded with n's low 16 bits (0: from the clock)
st_randomize:
            jsr         pop_int_any
            MOVW        r0, t0
            NUMCALL     NUM_SEED
            jmp         code_sel

; BASE b$: the base numbers are shown and read in
st_base:
            jsr         top_vt
            jsr         base_of_top
            jsr         pop_vt
            LDW         r0, f_bstr
            NUMCALL     NUM_SET_BASE
            bcc         :+
            jmp         n_fail
:
            jmp         code_sel

; DIGITS n: the math functions' precision (1 to 100 significant digits)
st_digits:
            jsr         pop_int16
            lda         t0 + 1
            bne         @bad
            lda         t0
            beq         @bad
            cmp         #101
            bcs         @bad
            sta         digits_set
            lda         math_mod
            beq         :+
            lda         digits_set
            MATHCALL    MATH_DIGITS
            jmp         code_sel
:
            rts
@bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; TIMER's value: t0-t3 milliseconds pushed as seconds, a fixed decimal of 3 places
num_ms:
            lda         #VT_INT
            sta         (vsp)
            ldy         #4
:
            lda         t0 - 1,y
            sta         (vsp),y
            dey
            bne         :-
            jsr         vsp_up
            lda         #<1000
            ldx         #>1000
            jsr         push_ax4
            lda         #OP_DIV
            jsr         num_op
            lda         #3
            ldx         #0
            jsr         push_ax4
            jmp         f_fixed

; .A/.X (0-65535) pushed, an integer
push_ax4:
            pha
            jsr         push_zero
            jsr         top_vt
            pla
            ldy         #1
            sta         (vt),y
            iny
            txa
            sta         (vt),y
            rts

; INPUT's number: the text at r0 (r1 bytes, in the RAM) read whole, pushed.  OUT: C = 1 not a number (0 pushed)
num_field:
            jsr         push_zero
            jsr         r2_res
            stz         r4
            stz         r4 + 1
            lda         nsb
            sta         RAM_BANK
            ldy         #NPARSE_WHOLE
            NUMCALL     NUM_PARSE
            bcs         @bad
            sta         nlen
            stx         nlen + 1
            jsr         top_vt
            jsr         result_vt
            jsr         code_sel
            clc
            rts
@bad:
            jmp         code_sel

; The math library's precision made DIGITS's (digits_set), RND's digits too
digits_sync:
            rts

; DIGITS(): the precision
f_digits:
            jsr         push_zero
            lda         math_mod
            beq         :+
            lda         #0
            MATHCALL    MATH_DIGITS
            jsr         code_sel
            pha
            jsr         top_vt
            pla
            jmp         int_a_vt
:
            jsr         top_vt
            lda         digits_set
            jmp         int_a_vt

; VAL(s$): the longest start of s$ that's a number, read in the base (none: 0)
f_val:
            jsr         top_vt
            FARN        5, str_get_a                        ; (sla, sfa)
            lda         sla + 1                             ; (Its first 255 bytes, to NS_A)
            beq         :+
            lda         #255
            sta         sla
:
            lda         sla
            sta         cn
            stz         cn + 1
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            LDW         cb, NS_A
            lda         #LB_NSB
            sta         cbl
            lda         cn
            beq         @zero
            jsr         far_copy
            LDW         r0, NS_A
            lda         sla
            sta         r1
            stz         r1 + 1
            stz         r4
            stz         r4 + 1
            jsr         r2_res
            lda         nsb
            sta         RAM_BANK
            jsr         amp_base
            ldy         #0
            NUMCALL     NUM_PARSE
            bcs         @zero
            jmp         n_result1
@zero:
            jsr         code_sel
            jsr         top_vt
            lda         #0
            jmp         int_a_vt

; QuickBASIC's &H, &O, &B at the text's start (NS_A, r1 bytes, its bank selected; spaces before it) made #x, #o, #b
amp_base:
            ldy         #0
:
            cpy         r1
            bcs         @done
            lda         NS_A,y
            cmp         #' '
            bne         :+
            iny
            bra         :-
:
            cmp         #'&'
            bne         @done
            iny
            cpy         r1
            bcs         @done
            lda         NS_A,y
            and         #$DF
            ldx         #2
:
            cmp         amp_ch,x
            beq         :+
            dex
            bpl         :-
            rts
:
            lda         amp_to,x
            sta         NS_A,y
            lda         #'#'
            sta         NS_A - 1,y
@done:
            rts
amp_ch:     .byte       "HOB"
amp_to:     .byte       "xob"

; STR$(x [, base$]): x's text (in the base, a space before it if it's not below 0; in a base named, as it is)
f_str:
            jsr         pop_int16                           ; (Its count: 1 or 2)
            stz         f_sp
            lda         t0
            cmp         #2
            beq         @based
            inc         f_sp
            stz         nt_base
            stz         nt_base + 1
            bra         f_text
@based:
            jsr         top_vt                              ; (The base: its string's bytes, to f_bstr)
            jsr         base_of_top
            jsr         pop_vt
            LDW         nt_base, f_bstr
            ; (fall through)

; The top value's text (num_text, nt_base's base) made a string in its place; a space first if f_sp and it's not
; below 0
f_text:
            jsr         top_vt
            jsr         num_text
            sta         t0
            stx         t0 + 1
            lda         nsign                               ; (The space: when it's wanted, and it isn't below 0)
            bpl         :+
            stz         f_sp
:
            lda         t0                                  ; (Its room: the text, and the space)
            clc
            adc         f_sp
            ldx         t0 + 1
            bcc         :+
            inx
:
            FARN        5, heap_str                         ; (cb: at its bytes)
            lda         f_sp
            beq         :+
            jsr         cb_sel
            lda         #' '
            jsr         cb_wr
:
            LDW         ca, $8000
            lda         #LB_TXB
            sta         cal
            MOVW        cn, t0
            jsr         far_copy
            jsr         top_vt                              ; (The string in the number's place)
            jmp         put_hres_vt

; vt's value: VT_STR at hres
put_hres_vt:
            lda         #VT_STR
            sta         (vt)
            ldy         #1
            lda         hres
            sta         (vt),y
            iny
            lda         hres + 1
            sta         (vt),y
            iny
            lda         hres + 2
            sta         (vt),y
            jmp         code_sel

; The string at vt copied to f_bstr, zero-terminated (a base string: BASE_ROOM - 1 at most)
base_of_top:
            FARN        5, str_get_a
            lda         sla + 1
            bne         @long
            lda         sla
            cmp         #BASE_ROOM
            bcs         @long
            sta         cn
            stz         cn + 1
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            ldy         #0
            lda         cn
            beq         @end
            jsr         ca_sel
:
            jsr         ca_rd
            sta         f_bstr,y
            iny
            cpy         cn
            bne         :-
@end:
            lda         #0
            sta         f_bstr,y
            jmp         code_sel
@long:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; HEX$(x), OCT$(x): x's text in base x, o
f_hex:
            lda         #'x'
            bra         :+
f_oct:
            lda         #'o'
:
            sta         f_base
            stz         f_base + 1
            LDW         nt_base, f_base
            stz         f_sp
            jmp         f_text

; BASE$: the base's string
f_bases:
            jsr         push_zero
            LDW         r2, f_bstr
            LDW         r3, BASE_ROOM
            NUMCALL     NUM_GET_BASE
            jsr         code_sel
            sta         t0
            stz         t0 + 1
            FARN        5, heap_str
            jsr         cb_sel
            ldy         #0
:
            cpy         t0
            beq         :+
            lda         f_bstr,y
            jsr         cb_wr
            iny
            bra         :-
:
            jsr         top_vt
            jmp         put_hres_vt

; MKN$(x): its bytes in the stored format, as a string
f_mkn:
            jsr         one_op                              ; (r0: its bytes, in nia or NS_A)
            lda         r0 + 1                              ; (Their length: an integer's from int_sf, a number's
            bmi         @num                                ;   from its value's)
            LDW         np, nia
            jsr         top_vt
            ldy         #4
:
            lda         (vt),y
            sta         t0 - 1,y
            dey
            bne         :-
            jsr         int_sf
            sta         t0
            stz         t0 + 1
            bra         @have
@num:
            jsr         top_vt                              ; (Its length: its far address's two bytes)
            ldy         #1
            lda         (vt),y
            sta         ca
            iny
            lda         (vt),y
            sta         ca + 1
            iny
            lda         (vt),y
            sta         cal
            jsr         ca_sel
            jsr         ca_rd
            sta         t0
            jsr         ca_rd
            sta         t0 + 1
@have:
            MOVW        t4, r0                              ; (Where they are)
            lda         t0
            ldx         t0 + 1
            FARN        5, heap_str
            MOVW        ca, t4
            lda         #LB_NSB
            sta         cal
            lda         t4 + 1
            bmi         :+
            lda         #0                                  ; (In the RAM: copied from there)
            sta         cal
:
            lda         t4 + 1
            bmi         @far
            jsr         cb_sel
            ldy         #0
:
            cpy         t0
            beq         @done
            lda         (t4),y
            jsr         cb_wr
            iny
            bra         :-
@far:
            MOVW        cn, t0
            jsr         far_copy
@done:
            jsr         top_vt
            jmp         put_hres_vt

; CVN(s$): the number whose bytes s$ is (else Illegal function call)
f_cvn:
            jsr         top_vt
            FARN        5, str_get_a
            lda         sla + 1                             ; (NUM_MAX at most)
            cmp         #>NUM_MAX
            bcc         :+
            bne         @bad
            lda         sla
            cmp         #<NUM_MAX
            bcs         @bad
:
            MOVW        cn, sla
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            LDW         cb, NS_A
            lda         #LB_NSB
            sta         cbl
            jsr         far_copy
            LDW         r0, NS_A
            MOVW        r1, sla
            jsr         r2_res
            lda         nsb
            sta         RAM_BANK
            NUMCALL     NUM_BYTES
            bcs         @bad
            jmp         n_result1
@bad:
            jsr         code_sel
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error
