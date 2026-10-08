; ****************************************************************************
; num.s - num.h's functions: each its arguments into the registers its library entry takes (spec/numbers.def), then
; the call (numcall.s: __num_xcall); a result's length, or -1 (num_error set).  cc65's calls: the last argument in
; .A/.X (a long's high word in sreg; a char in .A), the others on the C stack, the first deepest, which the function
; pops.  And num_size, which reads a number's tags itself.

            .export     _num_set_base, _num_get_base, _num_parse, _num_display, __num_format
            .export     _num_add, _num_sub, _num_mul, _num_div, _num_neg, _num_abs, _num_pow, _num_idiv, _num_gcd
            .export     _num_cmp, _num_kind, _num_sign
            .export     _num_truncate, _num_floor, _num_round, _num_to_fixed, _num_to_rational, _num_numerator
            .export     _num_denominator, _num_complex, _num_part, _num_from_int, _num_from_uint, _num_to_int
            .export     _num_check, _num_bits, _num_random, _num_seed, _num_fib
            .export     _num_digits, _num_sqrt, _num_exp, _num_log, _num_sin, _num_cos, _num_tan, _num_atan, _num_pi
            .export     _num_size
            .import     popax, _num_error
            .import     __num_ready, __num_xcall, __num_entry, __num_mmod

            .include    "zeropage.inc"
            .include    "hydra.inc"
            .include    "numbers.inc"

; The entries: each one's offset in its library's table, the math library's with bit 7
.define     NUM(e)      <(e - NUM_INIT)
.define     MATH(e)     <(e - MATH_DIGITS + $80)

            .bss
word:       .res        2                                   ; An argument kept (a room, a pointer, places)
yval:       .res        1                                   ; .Y's for the call (or which of two functions)

            .code

; ---- Arithmetic and conversions: (dst, room, a, b) and (dst, room, a)

_num_add:   ldy         #NUM(NUM_ADD)
            bra         binary
_num_sub:   ldy         #NUM(NUM_SUB)
            bra         binary
_num_mul:   ldy         #NUM(NUM_MUL)
            bra         binary
_num_div:   ldy         #NUM(NUM_DIV)
            bra         binary
_num_gcd:   ldy         #NUM(NUM_GCD)
            bra         binary
_num_complex: ldy       #NUM(NUM_COMPLEX)
binary:                                                     ; (.Y the entry) r1 = b; r0 = a; r3 = room; r2 = dst
            sty         __num_entry
            sta         r1
            stx         r1 + 1
            jsr         popax
            jsr         popdst0
call00:                                                     ; The call: .A, .X, .Y 0
            lda         #0
            tax
            tay
go:
            jsr         __num_xcall
            bcc         :+
fail:
            lda         #$FF
            tax
:
            rts

_num_neg:   ldy         #NUM(NUM_NEG)
            bra         unary
_num_abs:   ldy         #NUM(NUM_ABS)
            bra         unary
_num_truncate: ldy      #NUM(NUM_TRUNCATE)
            bra         unary
_num_floor: ldy         #NUM(NUM_FLOOR)
            bra         unary
_num_round: ldy         #NUM(NUM_ROUND)
            bra         unary
_num_to_rational: ldy   #NUM(NUM_TO_RATIONAL)
            bra         unary
_num_numerator: ldy     #NUM(NUM_NUMERATOR)
            bra         unary
_num_denominator: ldy   #NUM(NUM_DENOMINATOR)
            bra         unary
_num_random: ldy        #NUM(NUM_RANDOM)
            bra         unary
_num_fib:   ldy         #NUM(NUM_FIB)
            bra         unary
_num_sqrt:  ldy         #MATH(MATH_SQRT)
            bra         unary
_num_exp:   ldy         #MATH(MATH_EXP)
            bra         unary
_num_log:   ldy         #MATH(MATH_LOG)
            bra         unary
_num_atan:  ldy         #MATH(MATH_ATAN)
unary:                                                      ; (.Y the entry) r0 = a; r3 = room; r2 = dst
            sty         __num_entry
            jsr         popdst0
            bra         call00

call0:                                                      ; The call (.Y its entry): .A, .X, .Y 0
            sty         __num_entry
            bra         call00

; r3 = the C stack's next word (a room), r2 the one after (a place).  Keeps .Y
popdst:
            phy
            jsr         popax
            sta         r3
            stx         r3 + 1
            jsr         popax
            sta         r2
            stx         r2 + 1
            ply
            rts

; r0 = .A/.X, then popdst.  Keeps .Y
popdst0:
            sta         r0
            stx         r0 + 1
            bra         popdst

_num_sin:   ldy         #0                                  ; (TRIG: .Y = 0, 1, 2)
            bra         trig
_num_cos:   ldy         #1
            bra         trig
_num_tan:   ldy         #2
trig:
            sty         yval
            jsr         popdst0
            ldy         #MATH(MATH_TRIG)
withy:                                                      ; The call, .Y = yval
            sty         __num_entry
            lda         #0
            tax
            ldy         yval
            bra         go

_num_pi:                                                    ; (dst, room)
            sta         r3
            stx         r3 + 1
            jsr         popax
            sta         r2
            stx         r2 + 1
            ldy         #MATH(MATH_PI)
            bra         call0

; int num_pow (dst, room, a, b): the math library's RPOW, else (none) the numbers library's POW
_num_pow:
            sta         r1
            stx         r1 + 1
            jsr         popax
            jsr         popdst0
            jsr         __num_ready
            bcc         @lb201
            jmp         fail
@lb201:
            ldy         #NUM(NUM_POW)
            lda         __num_mmod
            beq         :+
            ldy         #MATH(MATH_RPOW)
:
            bra         call0

; int num_idiv (q, qroom, r, rroom, a, b)
_num_idiv:
            sta         r1
            stx         r1 + 1
            jsr         popax
            sta         r0
            stx         r0 + 1
            jsr         popax
            sta         r6
            stx         r6 + 1
            jsr         popax
            sta         r5
            stx         r5 + 1
            jsr         popdst
            ldy         #NUM(NUM_IDIV)
            jmp         call0

; int num_to_fixed (dst, room, a, places)
_num_to_fixed:
            sta         word
            stx         word + 1
            jsr         popax
            jsr         popdst0
            lda         #NUM(NUM_TO_FIXED)
            sta         __num_entry
            lda         word
            ldx         word + 1
            ldy         #0
            jmp         go

; int num_part (dst, room, a, im)
_num_part:
            sta         yval
            jsr         popax
            jsr         popdst0
            ldy         #NUM(NUM_PART)
            jmp         withy

; int num_from_int (dst, room, long v), num_from_uint (dst, room, unsigned long v)
_num_from_int:
            ldy         #1                                  ; (.Y = 1: signed)
            bra         :+
_num_from_uint:
            ldy         #0
:
            sty         yval
            sta         r0
            stx         r0 + 1
            lda         sreg
            sta         r1
            lda         sreg + 1
            sta         r1 + 1
            jsr         popdst
            ldy         #NUM(NUM_FROM_INT)
            jmp         withy

; int num_check (dst, room, bytes, count)
_num_check:
            ldy         #NUM(NUM_BYTES)
            jmp         binary

; void num_seed (unsigned seed)
_num_seed:
            sta         r0
            stx         r0 + 1
            ldy         #NUM(NUM_SEED)
            jmp         call0

; int num_bits (dst, room, a, b, op): NBIT_TEST's answer .A (0 or 1)
_num_bits:
            sta         yval
            jsr         popax
            sta         r1
            stx         r1 + 1
            jsr         popax
            jsr         popdst0
            lda         #NUM(NUM_BITS)
            sta         __num_entry
            lda         #0
            tax
            ldy         yval
            jsr         __num_xcall
            bcs         fail1
            ldy         yval
            cpy         #NBIT_TEST
            bne         :+
            ldx         #0
:
            rts
fail1:
            jmp         fail

; int num_digits (unsigned char digits): the precision before
_num_digits:
            ldy         #MATH(MATH_DIGITS)
            sty         __num_entry
            ldy         #0
            jsr         __num_xcall
            bcs         fail1
            ldx         #0
            rts

; int num_set_base (const char* base): what it is (0-3)
_num_set_base:
            sta         r0
            stx         r0 + 1
            ldy         #NUM(NUM_SET_BASE)
            sty         __num_entry
            lda         #0
            tax
            tay
            jsr         __num_xcall
            bcs         fail1
            ldx         #0
            rts

; int num_cmp (a, b): -1, 0, 1; 2 if it fails
_num_cmp:
            sta         r1
            stx         r1 + 1
            jsr         popax
            sta         r0
            stx         r0 + 1
            ldy         #NUM(NUM_CMP)
            sty         __num_entry
            lda         #0
            tax
            tay
            jsr         __num_xcall
            bcs         two
            ldx         #0
            cmp         #$FF
            bne         :+
            dex
:
            rts
two:
            lda         #2
            ldx         #0
            rts

; int num_kind (a): its kind, or -1; int num_sign (a): -1, 0, 1, or 2
_num_kind:
            ldy         #0
            bra         :+
_num_sign:
            ldy         #1
:
            sty         yval
            sta         r0
            stx         r0 + 1
            ldy         #NUM(NUM_KIND)
            sty         __num_entry
            lda         #0
            tax
            tay
            jsr         __num_xcall
            bcs         @fail
            ldy         yval
            bne         @sign
            ldx         #0
            rts
@sign:
            txa
            ldx         #0
            cmp         #$FF
            bne         :+
            dex
:
            rts
@fail:
            lda         yval
            beq         fail1
            bra         two

; int num_to_int (a, long* v): 0, 1 or 2 (whether it fits), or -1
_num_to_int:
            sta         word
            stx         word + 1
            jsr         popax
            sta         r0
            stx         r0 + 1
            ldy         #NUM(NUM_TO_INT)
            sty         __num_entry
            lda         #0
            tax
            tay
            jsr         __num_xcall
            bcc         @lb200
            jmp         fail1
@lb200:
            pha
            lda         word
            sta         ptr1
            lda         word + 1
            sta         ptr1 + 1
            ldy         #3                                  ; (r4/r5: its low 32 bits, r4 low)
:
            lda         r4,y
            sta         (ptr1),y
            dey
            bpl         :-
            pla
            ldx         #0
            rts

; ---- Text: (dst, room) with a 0 after it

; int num_get_base (char* dst, unsigned room)
_num_get_base:
            sta         word
            stx         word + 1
            jsr         popax
            sta         r2
            stx         r2 + 1
            ldy         #NUM(NUM_GET_BASE)
            bra         text

; int num_display (char* dst, unsigned room, const num_t* a, const char* base)
_num_display:
            sta         r4
            stx         r4 + 1
            ldy         #NUM(NUM_DISPLAY)
            bra         text4

; int _num_format (char* dst, unsigned room, const char* fmt, const unsigned char* args) (numfmt.c's)
__num_format:
            sta         r4
            stx         r4 + 1
            ldy         #NUM(NUM_FORMAT)
text4:                                                      ; r0, then the room (word) and dst (r2)
            phy
            jsr         popax
            sta         r0
            stx         r0 + 1
            jsr         popax
            sta         word
            stx         word + 1
            jsr         popax
            sta         r2
            stx         r2 + 1
            ply
text:                                                       ; The call (.Y its entry): its text at r2, its room
            sty         __num_entry                         ;   word less 1 (a 0 after it)
            lda         word
            ora         word + 1
            bne         :+
            lda         #NE_ROOM
            sta         _num_error
            jmp         fail
:
            lda         word
            sec
            sbc         #1
            sta         r3
            lda         word + 1
            sbc         #0
            sta         r3 + 1
            lda         #0
            tax
            tay
            jsr         __num_xcall
            bcs         @fail
            pha
            phx
            clc
            adc         r2
            sta         ptr1
            txa
            adc         r2 + 1
            sta         ptr1 + 1
            lda         #0
            sta         (ptr1)
            plx
            pla
            rts
@fail:
            lda         #0                                  ; (Its text empty)
            sta         (r2)
            jmp         fail

; int num_parse (num_t* dst, unsigned room, const char* text, const char* base, char** end)
_num_parse:
            sta         word
            stx         word + 1
            jsr         popax
            sta         r4
            stx         r4 + 1
            jsr         popax
            jsr         popdst0
            lda         r0                                  ; r1 = its length
            sta         ptr1
            lda         r0 + 1
            sta         ptr1 + 1
            ldy         #0
            ldx         #0
@len:
            lda         (ptr1),y
            beq         :+
            iny
            bne         @len
            inc         ptr1 + 1
            inx
            bra         @len
:
            sty         r1
            stx         r1 + 1
            lda         #NUM(NUM_PARSE)
            sta         __num_entry
            ldy         #NPARSE_WHOLE                       ; (No end: the whole text)
            lda         word
            ora         word + 1
            beq         :+
            ldy         #0
:
            lda         #0
            tax
            jsr         __num_xcall
            bcs         @fail
            pha
            phx
            lda         word
            ora         word + 1
            beq         :+
            lda         word
            sta         ptr1
            lda         word + 1
            sta         ptr1 + 1
            lda         r0                                  ; *end = text + what it took (r5)
            clc
            adc         r5
            sta         (ptr1)
            lda         r0 + 1
            adc         r5 + 1
            ldy         #1
            sta         (ptr1),y
:
            plx
            pla
            rts
@fail:
            jmp         fail

; ---- unsigned num_size (const num_t* a): the bytes a number takes, by its tags (spec/numbers.def's NT_)

_num_size:
            sta         ptr1
            stx         ptr1 + 1
            sta         ptr2
            stx         ptr2 + 1
            jsr         skip
            lda         ptr1
            sec
            sbc         ptr2
            pha
            lda         ptr1 + 1
            sbc         ptr2 + 1
            tax
            pla
            rts

; ptr1 past the number there
skip:
            lda         (ptr1)
            cmp         #NT_POS
            bcc         @one                                ; (-64 to 63: its tag)
            cmp         #NT_LONG_POS
            bcc         @short                              ; (1-16 bytes)
            cmp         #NT_LONG_NEG + 1
            bcc         @long
            cmp         #NT_FIXED
            bcc         @one
            cmp         #NT_FIXED_LONG
            bcc         @fixed
            beq         @fixlong
            cmp         #NT_RATIONAL
            beq         @two
            cmp         #NT_COMPLEX
            beq         @two
@one:
            lda         #1                                  ; (Not a number's: its tag)
            bra         add
@short:
            and         #$0F
            clc
            adc         #2
            bra         add
@long:
            ldy         #1
            lda         (ptr1),y
            clc
            adc         #2
            bcc         add
            inc         ptr1 + 1
            bra         add
@fixed:
            lda         #1
            jsr         add
            bra         skip
@fixlong:
            lda         #3
            jsr         add
            bra         skip
@two:
            lda         #1
            jsr         add
            jsr         skip
            bra         skip

; ptr1 += .A
add:
            clc
            adc         ptr1
            sta         ptr1
            bcc         :+
            inc         ptr1 + 1
:
            rts
