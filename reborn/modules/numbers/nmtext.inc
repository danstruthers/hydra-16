; ****************************************************************************
; nmtext.inc - numbers as text, as danlang's NumberParser has them (numref.js's display and parse): bases (a base
; string's: SET_BASE's, or a call's own in r4), numbers written (DISPLAY, FORMAT) and read (PARSE).  hylang's
; numtext.inc's printer, and its reader's digits and named bases, with what danlang's feature/numbers added: the
; base for a number without a # of its own, a radix point in any base, complex numbers written and read, and whether a
; base's prefix is written (its string's #).
;   A base: its size (2-80); its digits (in upper case if no two differ only in case, which then doesn't matter as
; they're read); balanced (0 its middle digit), least digit first, negative.  Two of them: f_, the base a call has
; (from its string, f_spec), and w_, the one a number's being read or written in (f_'s, a number's own, #x..., or
; decimal's for a complex number in a base whose digits have +, - or i).

NAMED           = 19
TEXT_MAX        = 255                                       ; (PARSE: a text's characters read, at most)

.segment "NUMBANK"
f_radix:    .res        1                                   ; f_: the base a call has.  Its size; negative ($FF);
f_neg:      .res        1                                   ;   balanced ($FF); least digit first ($80); no two digits
f_bal:      .res        1                                   ;   the same but for case (1); its 0's place among its
f_le:       .res        1                                   ;   digits; decimal (1: 0-9, none of those three); its
f_ci:       .res        1                                   ;   prefix written (1: its string's #); its digits
f_zero:     .res        1
f_dec:      .res        1
f_pre:      .res        1
f_dset:     .res        80
w_radix:    .res        1                                   ; w_: the base a number's read or written in, the same
w_neg:      .res        1
w_bal:      .res        1
w_le:       .res        1
w_ci:       .res        1
w_zero:     .res        1
w_dec:      .res        1
w_pre:      .res        1
w_dset:     .res        80
B_SIZE          = w_radix - f_radix
f_spec:     .res        BASE_MAX                            ; f_'s string (its prefix: # and the string past its #)
f_body:     .res        1                                   ; (Where its string's # ends)
t_dir:      .res        1                                   ; (A base's modifiers: < $FF, > 1; = $FF; - $FF, + 0,
t_eq:       .res        1                                   ;   neither $80)
t_sgn:      .res        1
t_len:      .res        1                                   ; (PARSE: its text's length, the start being tried; its
t_try:      .res        1                                   ;   number's text, its _s out: its length; from, to; a
t_n:        .res        1                                   ;   program's text)
t_from:     .res        1
t_to:       .res        1
t_src:      .res        1
t_whole:    .res        1                                   ; (PARSE: the whole text, or its longest start)
t_mark:     .res        2                                   ; (The arena's top as a try began)
t_re:       .res        2                                   ; (A complex number's real part; a denominator's place)
t_p:        .res        1                                   ; (A place in the text; where a complex number's + or -
t_sp:       .res        1                                   ;   is, p_text's)
t_sign:     .res        1                                   ; ($FF: a - read)
t_dot:      .res        1                                   ; (A point read, the digits after it: t_np)
t_slash:    .res        1
t_c:        .res        1
t_d:        .res        1
t_np:       .res        2                                   ; (Digits made; to the point; a number's places in a base)
t_left:     .res        2
t_k:        .res        2
t_i:        .res        2
t_top:      .res        1
t_chk:      .res        1                                   ; (1: a text checked, its number not made)
t_rk:       .res        1                                   ; (A plain base's chunk: its size's power, its digits,
t_kk:       .res        1                                   ;   its remainder, a count, a quotient)
t_rem:      .res        1
t_cnt8:     .res        1
t_q8:       .res        1
t_s:        .res        1                                   ; (A digit: n's sign; the sizes added to it)
t_t:        .res        1
o_len:      .res        2                                   ; (The text written: its length)
g_ptrs:                                                     ; (FORMAT's places in the caller's memory: its string's,
f_ptr:      .res        2                                   ;   its arguments' table's, an argument's)
f_args:     .res        2
f_at:       .res        2
G_STRING        = f_ptr - g_ptrs
G_ARGS          = f_args - g_ptrs
G_AT            = f_at - g_ptrs

; PARSE's texts: its first TEXT_MAX characters, and its number's, the _s out, on the digits' pages (PARSE writes no
; digits)
t_raw           = DIGS
t_num           = DIGS + 256

.code

; ---- Bases

; f_ = the base the state has (st_base)
b_state:
            ldx         #0
:
            lda         st_base,x
            sta         f_spec,x
            beq         b_parse
            inx
            bra         :-

; f_ = the base the caller's string at np names (zero-terminated: BASE_MAX bytes at most).  NE_BASE: it names none
b_caller:
            ldx         #0
:
            jsr         c_get
            sta         f_spec,x
            cmp         #0
            beq         b_parse
            inx
            cpx         #BASE_MAX
            bcc         :-
            jmp         b_bad

; f_ = the base f_spec names (danlang's NumberFormat.Of): #?[<>]?=?[+-]? then a radix ([0-9]{1,2}r, 2-80), digits of
; its own ([...]: 2-80, each once, none a control character, a space or one of < > [ ] ( ) . / _ \ ' "), or a named
; base's letter (not with =: it's balanced or not already).  NE_BASE: it names none
b_parse:
            ldx         #0
            stz         w_pre
            lda         f_spec
            cmp         #'#'
            bne         :+
            inc         w_pre
            inx
:
            stx         f_body
            jsr         b_mods                              ; (< >, =, + -)
            lda         f_spec,x
            cmp         #'['
            beq         @own
            jsr         b_digit
            bcs         @letter
            sta         w_radix                             ; (A radix: one or two digits, then r)
            inx
            jsr         b_digit
            bcs         :+
            sta         t_d
            lda         w_radix
            asl
            asl
            adc         w_radix
            asl
            adc         t_d
            sta         w_radix
            inx
:
            lda         f_spec,x
            ora         #$20
            cmp         #'r'
            bne         b_bad
            inx
            lda         w_radix
            cmp         #2
            bcc         b_bad
            cmp         #81
            bcs         b_bad
            ldy         #0
:
            lda         n_std,y
            sta         w_dset,y
            iny
            cpy         w_radix
            bne         :-
            bra         @said
@own:
            inx                                             ; (Digits of its own: to the ])
            ldy         #0
@sdig:
            lda         f_spec,x
            beq         b_bad
            cmp         #']'
            beq         :+
            cpy         #80
            bcs         b_bad
            sta         w_dset,y
            iny
            inx
            bra         @sdig
:
            inx
            sty         w_radix
            cpy         #2
            bcc         b_bad
            phx
            jsr         b_unique
            plx
            bcs         b_bad
@said:
            jsr         b_said                              ; (=, <, - as said)
            bra         @end
@letter:
            lda         f_spec,x                            ; (A named base's letter: not with =)
            jsr         b_named
            bcs         b_bad
            inx
            lda         t_eq
            bne         b_bad
@end:
            lda         f_spec,x                            ; (Nothing after it)
            bne         b_bad
            jsr         b_finish
            ldy         #B_SIZE - 1                         ; (f_ = it)
:
            lda         w_radix,y
            sta         f_radix,y
            dey
            bpl         :-
            rts
b_bad:
            lda         #NE_BASE
            jmp         nm_fail

; t_dir, t_eq, t_sgn: the modifiers at f_spec,x (.X past them)
b_mods:
            stz         t_dir
            lda         f_spec,x
            cmp         #'<'
            bne         :+
            dec         t_dir
            inx
            bra         @eq
:
            cmp         #'>'
            bne         @eq
            inc         t_dir
            inx
@eq:
            stz         t_eq
            lda         f_spec,x
            cmp         #'='
            bne         :+
            dec         t_eq
            inx
:
            lda         #$80
            sta         t_sgn
            lda         f_spec,x
            cmp         #'-'
            bne         :+
            lda         #$FF
            sta         t_sgn
            inx
            rts
:
            cmp         #'+'
            bne         :+
            stz         t_sgn
            inx
:
            rts

; f_spec,x a decimal digit?  OUT: C = 0 and .A = its value
b_digit:
            lda         f_spec,x
            sec
            sbc         #'0'
            cmp         #10
            rts

; w_ (a radix's or digits of its own) balanced if t_eq, least first if t_dir is <, negative if t_sgn is -
b_said:
            lda         t_eq
            sta         w_bal
            lda         t_dir
            and         #$80
            sta         w_le
            lda         t_sgn
            cmp         #$FF
            beq         :+
            lda         #0
:
            sta         w_neg
            rts

; w_ = the named base whose letter is .A (any case): least first if t_dir says, negative if t_sgn says, else as it
; is.  OUT: C = 1 if no base has that letter.  Keeps .X
b_named:
            ora         #$20
            ldy         #NAMED - 1
:
            cmp         nb_letter,y
            beq         :+
            dey
            bpl         :-
            sec
            rts
:
            phx
            lda         nb_size,y
            sta         w_radix
            lda         nb_digs_lo,y
            sta         nq
            lda         nb_digs_hi,y
            sta         nq + 1
            lda         nb_flags,y
            sta         t_c
            ldy         #0
:
            lda         (nq),y
            sta         w_dset,y
            iny
            cpy         w_radix
            bne         :-
            lda         t_c                                 ; (Balanced: as it is)
            and         #NB_BAL
            beq         :+
            lda         #$FF
:
            sta         w_bal
            lda         t_dir                               ; (Least first: < or > said, else as it is)
            bne         :+
            lda         t_c
            and         #NB_LE
            beq         :+
            lda         #$FF
:
            and         #$80
            sta         w_le
            lda         t_sgn                               ; (Negative: - or + said, else as it is)
            cmp         #$80
            bne         :+
            lda         t_c
            and         #NB_NEG
            beq         :+
            lda         #$FF
:
            sta         w_neg
            plx
            clc
            rts

; w_'s digits (w_radix of them) each once, none a control character, a space or invalid (danlang's IsUniqueSet).
; OUT: C = 1 if not
b_unique:
            ldx         #0
@one:
            lda         w_dset,x
            cmp         #'!'
            bcc         @no
            ldy         #NINVALID - 1
:
            cmp         n_invalid,y
            beq         @no
            dey
            bpl         :-
            txa
            tay
@other:
            iny
            cpy         w_radix
            bcs         @next
            lda         w_dset,x
            cmp         w_dset,y
            bne         @other
@no:
            sec
            rts
@next:
            inx
            cpx         w_radix
            bne         @one
            clc
            rts

; w_ finished: w_ci (no two digits the same in upper case: then in upper case), its 0's place, whether it's decimal
b_finish:
            lda         #1
            sta         w_ci
            ldx         #0
@one:
            lda         w_dset,x
            jsr         np_upper
            sta         t_c
            txa
            tay
@other:
            iny
            cpy         w_radix
            bcs         @next
            lda         w_dset,y
            jsr         np_upper
            cmp         t_c
            bne         @other
            stz         w_ci
            bra         @zero
@next:
            inx
            cpx         w_radix
            bne         @one
            ldx         #0                                  ; (Case not significant: the digits in upper case)
:
            lda         w_dset,x
            jsr         np_upper
            sta         w_dset,x
            inx
            cpx         w_radix
            bne         :-
@zero:
            stz         w_zero
            lda         w_bal
            beq         :+
            lda         w_radix
            lsr
            sta         w_zero
:
            stz         w_dec                               ; (Decimal: 0-9, none of the three)
            lda         w_radix
            cmp         #10
            bne         @rts
            lda         w_bal
            ora         w_le
            ora         w_neg
            bne         @rts
            ldx         #9
:
            lda         w_dset,x
            cmp         n_std,x
            bne         @rts
            dex
            bpl         :-
            inc         w_dec
@rts:
            rts

; w_ = f_
b_cur:
            ldy         #B_SIZE - 1
:
            lda         f_radix,y
            sta         w_radix,y
            dey
            bpl         :-
            rts

; w_ = decimal, its prefix written (#d)
b_decimal:
            lda         #10
            sta         w_radix
            stz         w_neg
            stz         w_bal
            stz         w_le
            stz         w_zero
            lda         #1
            sta         w_ci
            sta         w_dec
            sta         w_pre
            ldx         #9
:
            lda         n_std,x
            sta         w_dset,x
            dex
            bpl         :-
            rts

; Is .A one of w_'s digits?  OUT: C = 0 and .A = its value (less its 0's place: signed) if it is.  Keeps .X
b_value:
            ldy         w_ci
            beq         :+
            jsr         np_upper
:
            ldy         #0
:
            cpy         w_radix
            bcs         @no
            cmp         w_dset,y
            beq         @yes
            iny
            bra         :-
@yes:
            tya
            sec
            sbc         w_zero
            clc
            rts
@no:
            sec
            rts

; .A in upper case (ASCII's letters).  Keeps .X, .Y
np_upper:
            cmp         #'a'
            bcc         @rts
            cmp         #'z' + 1
            bcs         @rts
            and         #$DF
@rts:
            rts

; f_ = the base a call has: r4's string, or the state's (r4 = 0)
b_call:
            lda         r4
            ora         r4 + 1
            bne         :+
            jmp         b_state
:
            MOVR        np, r4
            jmp         b_caller

; ---- SET_BASE

; SET_BASE: r0 = a base string (its bytes and a 0), the base from now on.  OUT: .A = what it is: bit 0 decimal (0-9,
; not balanced, least digit first or negative), bit 1 its prefix written (its string's #).  NE_BASE: it names none
nm_set_base:
            jsr         nm_begin
            MOVR        np, r0
            jsr         b_caller
            ldx         #0
:
            lda         f_spec,x
            sta         st_base,x
            beq         :+
            inx
            bra         :-
:
            lda         f_pre
            asl
            ora         f_dec
            ldx         #0
            clc
            jmp         nm_end

; ---- Written

; DISPLAY: r0 = a number; r4 = a base string (0: the base).  The number as text at r2 (room r3), .A, .X its length.
; NE_ROOM: no room (as much as there's room for written); NE_BASE: r4 names no base
nm_display:
            jsr         nm_begin
            jsr         b_call
            jsr         nm_in1
            jsr         o_start
            lda         n_x
            ldx         n_x + 1
            jsr         t_number
            lda         o_len
            ldx         o_len + 1
            clc
            jmp         nm_end

; The text begun: at r2 (np), its room r3
o_start:
            MOVR        np, r2
            stz         o_len
            stz         o_len + 1
            rts

; .A to the text (np), if there's room (r3).  NE_ROOM: no room.  Keeps .X, .Y
o_put:
            pha
            lda         o_len
            cmp         r3
            lda         o_len + 1
            sbc         r3 + 1
            bcs         @room
            pla
            jsr         c_put
            inc         o_len
            bne         :+
            inc         o_len + 1
:
            rts
@room:
            lda         #NE_ROOM
            jmp         nm_fail

; The number .A, .X written in f_ (its prefix if f_pre: danlang's Display): decimal without its prefix as danlang
; prints; a complex number its parts (in a base with +, - or i among its digits, in decimal with #d); else t_real
t_number:
            sta         n_v
            stx         n_v + 1
            lda         f_pre
            bne         :+
            lda         f_dec
            beq         :+
            lda         n_v
            ldx         n_v + 1
            jmp         np_print
:
            jsr         b_cur
            lda         n_v
            ldx         n_v + 1
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @complex
            lda         n_v
            ldx         n_v + 1
            jmp         t_real
@complex:
            lda         #'+'
            jsr         b_value
            bcc         @decimal
            lda         #'-'
            jsr         b_value
            bcc         @decimal
            lda         #'i'
            jsr         b_value
            bcs         :+
@decimal:
            jsr         b_decimal
:
            lda         n_v
            ldx         n_v + 1
            jsr         n_parts
            sta         n_x
            stx         n_x + 1
            MOVR        n_xi, n_v
            lda         n_x                                 ; (Its real part 0: its imaginary alone)
            ldx         n_x + 1
            jsr         n_zero_p
            beq         @im
            lda         n_x
            ldx         n_x + 1
            jsr         t_real
            MOVR        n_x, n_xi                           ; (+ unless the imaginary part's written with a -)
            jsr         n_rsign
            cmp         #$FF
            bne         @plus
            lda         w_dec
            bne         @im
            lda         w_neg
            ora         w_bal
            beq         @im
@plus:
            lda         #'+'
            jsr         o_put
@im:
            lda         n_xi
            ldx         n_xi + 1
            jsr         t_real
            lda         #'i'
            jmp         o_put

; The prefix, if w_pre: # and f_'s string past its # (#d: w_ decimal's, not f_'s)
t_prefix:
            lda         w_pre
            beq         @rts
            lda         #'#'
            jsr         o_put
            lda         f_dec
            bne         :+
            lda         w_dec
            beq         :+
            lda         #'d'
            jmp         o_put
:
            ldx         f_body
:
            lda         f_spec,x
            beq         @rts
            jsr         o_put
            inx
            bra         :-
@rts:
            rts

; The real number .A, .X written in w_ (danlang's DisplayReal).  In decimal as danlang prints, the prefix after its -
; (a rational: each part so).  In another base, as a fraction n/d in lowest terms: n's digits if d is 1; with a radix
; point if d divides a power of the base (the least, k: n * base^k / d's digits, k of them after the point); else
; n's digits, /, d's (each with the prefix)
t_real:
            sta         n_v
            stx         n_v + 1
            lda         w_dec
            beq         :+
            lda         n_v
            ldx         n_v + 1
            jmp         np_real
:
            lda         n_v                                 ; (n/d: R4/R5, in lowest terms)
            ldx         n_v + 1
            jsr         n_kind
            bne         @lb210
            jmp         @int
@lb210:
            cmp         #NK_RATIONAL
            beq         @rat
            lda         n_v                                 ; (A fixed decimal: as a rational)
            ldx         n_v + 1
            ldy         #4
            jsr         n_load
            jsr         n_rat
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            beq         @int
@rat:
            lda         n_v
            ldx         n_v + 1
            ldy         #4
            jsr         n_load
            jsr         t_places                            ; (d dividing a power of the base: k, base^k / d R6)
            bcs         @fraction
            lda         #4                                  ; (n * base^k / d: R4 * R6, its sign the power's too)
            ldx         #6
            ldy         #0
            jsr         n_mulr
            lda         w_neg
            beq         :+
            lda         t_k
            lsr
            bcc         :+
            ldx         #0
            jsr         r_negate
:
            MOVR        t_left, t_k
            jmp         t_digits
@fraction:
            lda         #4                                  ; (n's digits, /, d's)
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            stz         t_left
            stz         t_left + 1
            jsr         t_digits
            lda         #'/'
            jsr         o_put
            lda         #5
            sta         nra
            lda         #0
            sta         nrc
            jsr         r_copy
            stz         t_left
            stz         t_left + 1
            jmp         t_digits
@int:
            lda         #0
            sta         nrc
            lda         n_v
            ldx         n_v + 1
            jsr         r_load
            stz         t_left
            stz         t_left + 1
            jmp         t_digits

; R5 (positive) dividing a power of w_'s size?  OUT: C = 0, t_k = the least such power, R6 = |base|^k / R5; C = 1 if
; none does.  R7 the denominator left, from R5: each step g = gcd(R7, |base|), R7 / g, R6 * (|base| / g), till R7 is
; 1.  Uses R1, R7
t_places:
            stz         t_k
            stz         t_k + 1
            lda         #6
            sta         nrc
            lda         #1
            ldx         #0
            jsr         r_set16
            lda         #5
            sta         nra
            lda         #7
            sta         nrc
            jsr         r_copy
@step:
            lda         #7
            sta         nrc
            jsr         r_one_p
            beq         @done
            lda         #7                                  ; (R7 mod the base, then their gcd: a byte's Euclid)
            sta         nra
            lda         #1
            sta         nrc
            jsr         r_copy
            lda         w_radix
            jsr         r_divsmall
            tax
            lda         w_radix
@euclid:
            cpx         #0
            beq         @g
            sta         t_c
            stx         t_d
            lda         t_c
:
            cmp         t_d
            bcc         :+
            sbc         t_d
            bra         :-
:
            tax
            lda         t_d
            bra         @euclid
@g:
            cmp         #1
            beq         @none
            sta         t_d                                 ; (R7 / g; R6 * base / g)
            lda         #7
            sta         nrc
            lda         t_d
            jsr         r_divsmall
            lda         w_radix
            ldx         #0
:
            cmp         t_d
            bcc         :+
            sbc         t_d
            inx
            bra         :-
:
            txa
            ldy         #6
            sty         nrc
            jsr         r_mulsmall
            inc         t_k
            bne         @step
            inc         t_k + 1
            bra         @step
@done:
            clc
            rts
@none:
            sec
            rts

; R0, an integer, written in w_ (danlang's digitsOf): - if it's below 0 (in a base that's positive and not
; balanced), the prefix, its digits; t_left of them after a radix point (0s first to one more than that).  Its digits:
; least first, d = n mod the size, in the base's range; n = (n - d) / base, till it's 0 (hylang's)
t_digits:
            ldx         #0
            lda         r_len,x
            beq         :+
            lda         r_neg,x
            beq         :+
            lda         w_neg
            ora         w_bal
            bne         :+
            jsr         r_negate
            lda         #'-'
            jsr         o_put
:
            jsr         t_prefix
            stz         t_np
            stz         t_np + 1
            lda         w_radix                             ; (A digit's value below this: size less its 0's place)
            sec
            sbc         w_zero
            sta         t_top
            lda         w_neg                               ; (A plain base: a chunk of digits a division)
            ora         w_bal
            bne         @digit
            jsr         t_chunks
            jmp         @pad
@digit:
            ldx         #0                                  ; (|n| = q * size + rem: R0 = q, its sign n's, s)
            lda         r_neg,x
            sta         t_s
            stx         nrc
            lda         w_radix
            jsr         r_divsmall
            sta         t_d
            ldx         #0
            lda         r_len,x
            beq         :+
            lda         t_s
            sta         r_neg,x
:
            stz         t_t                                 ; (d = s * rem, into the base's range, t sizes added:
            lda         t_s                                 ;   -its 0's place to size - 1 less it; balanced of an
            beq         :+                                  ;   even size, one less above 0)
            sec
            lda         #0
            sbc         t_d
            sta         t_d
:
            lda         t_d
            bmi         @low
            cmp         t_top
            bcc         @ranged
            sec
            sbc         w_radix
            dec         t_t
            bra         @set
@low:
            clc
            adc         w_zero
            bpl         @ranged
            lda         t_d
            clc
            adc         w_radix
            inc         t_t
@set:
            sta         t_d
@ranged:
            lda         t_d                                 ; (Its character, kept)
            clc
            adc         w_zero
            tay
            lda         w_dset,y
            jsr         t_keep
            lda         t_t                                 ; (n = (n - d) / base: s * q - t, less if base is)
            beq         @next
            bmi         :+                                  ; (-t: R2)
            lda         #$FF
            tax
            bra         :++
:
            lda         #1
            ldx         #0
:
            ldy         #2
            sty         nrc
            jsr         r_set16
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jsr         r_add
@next:
            lda         w_neg
            beq         :+
            ldx         #0
            jsr         r_negate
:
            ldx         #0
            lda         r_len,x
            beq         @pad
            jmp         @digit
@pad:
            lda         t_np + 1                            ; (0s to one more than the places)
            cmp         t_left + 1
            bcc         :+
            bne         @made
            lda         t_left
            cmp         t_np
            bcc         @made
:
            ldy         w_zero
            lda         w_dset,y
            jsr         t_keep
            bra         @pad
@made:
            lda         w_le
            bne         @le
            lda         t_np                                ; (Most first: from the last back, a point before the
            sta         nd                                  ;   last t_left)
            lda         t_np + 1
            clc
            adc         #>DIGS
            sta         nd + 1
@be:
            lda         t_np
            ora         t_np + 1
            beq         @rts
            lda         t_np
            cmp         t_left
            bne         :+
            lda         t_np + 1
            cmp         t_left + 1
            bne         :+
            lda         #'.'
            jsr         o_put
:
            lda         nd
            bne         :+
            dec         nd + 1
:
            dec         nd
            lda         (nd)
            jsr         o_put
            lda         t_np
            bne         :+
            dec         t_np + 1
:
            dec         t_np
            bra         @be
@le:
            LDR         nd, DIGS                            ; (Least first: as they were made, a point after the
            stz         t_i                                 ;   first t_left)
            stz         t_i + 1
@lnext:
            lda         t_i
            cmp         t_np
            bne         :+
            lda         t_i + 1
            cmp         t_np + 1
            beq         @rts
:
            lda         t_left
            ora         t_left + 1
            beq         :+
            lda         t_i
            cmp         t_left
            bne         :+
            lda         t_i + 1
            cmp         t_left + 1
            bne         :+
            lda         #'.'
            jsr         o_put
:
            lda         (nd)
            jsr         o_put
            inc         nd
            bne         :+
            inc         nd + 1
:
            inc         t_i
            bne         @lnext
            inc         t_i + 1
            bra         @lnext
@rts:
            rts

; R0 (0 or more) in a plain base (not balanced or negative): its digits kept, least first, k at a time (t_rk, the
; size to the k, the most in a byte: R0 divided by it, its remainder split by byte divisions); the last chunk's 0s
; before it not kept (one digit, 0, for 0)
t_chunks:
            lda         w_radix                             ; (t_rk, t_kk: the size's greatest power in a byte)
            sta         t_rk
            lda         #1
            sta         t_kk
:
            lda         w_radix
            sta         n_m8
            lda         t_rk
            jsr         mul8
            ldx         n_phi
            bne         @chunk
            sta         t_rk
            inc         t_kk
            bra         :-
@chunk:
            lda         #0
            sta         nrc
            lda         t_rk
            jsr         r_divsmall
            sta         t_rem
            ldx         #0
            lda         r_len,x
            beq         @last
            lda         t_kk                                ; (Not the last: k digits)
            sta         t_cnt8
:
            jsr         t_digit8
            dec         t_cnt8
            bne         :-
            bra         @chunk
@last:
            jsr         t_digit8                            ; (The last: till what's left is 0)
            lda         t_rem
            bne         @last
            rts

; t_rem's last digit in the plain base w_ kept, t_rem the rest (a byte's division)
t_digit8:
            lda         t_rem
            sta         t_q8
            lda         #0
            ldx         #8
:
            asl         t_q8
            rol         a
            cmp         w_radix
            bcc         :+
            sbc         w_radix
            inc         t_q8
:
            dex
            bne         :--
            tay
            lda         t_q8
            sta         t_rem
            lda         w_dset,y
            jmp         t_keep

; .A, a digit's character, kept on the digits' pages (t_np of them).  Past them: NE_BIG
t_keep:
            pha
            lda         t_np + 1
            cmp         #NR_DIGS
            bcc         :+
            jmp         n_big
:
            clc
            adc         #>DIGS
            sta         nd + 1
            lda         t_np
            sta         nd
            pla
            sta         (nd)
            inc         t_np
            bne         :+
            inc         t_np + 1
:
            rts

; ---- Written in decimal, as danlang prints: 42, -1.25, 2/3, 1+2i (hylang's num_print)

; The number .A, .X, no prefix
np_print:
            sta         n_v
            stx         n_v + 1
            stz         w_pre
            jsr         n_kind
            cmp         #NK_COMPLEX
            beq         @complex
            lda         n_v
            ldx         n_v + 1
            jmp         np_real
@complex:
            lda         n_v
            ldx         n_v + 1
            jsr         n_parts
            sta         n_x
            stx         n_x + 1
            MOVR        n_xi, n_v
            lda         n_x                                 ; (Its real part 0: its imaginary alone)
            ldx         n_x + 1
            jsr         n_zero_p
            beq         @im
            lda         n_x
            ldx         n_x + 1
            jsr         np_real
            MOVR        n_x, n_xi                           ; (+ before an imaginary part above 0)
            jsr         n_rsign
            cmp         #1
            bne         @im
            lda         #'+'
            jsr         o_put
@im:
            lda         n_xi
            ldx         n_xi + 1
            jsr         np_real
            lda         #'i'
            jmp         o_put

; The real number .A, .X (w_pre: the prefix after a -; a rational's, each part's)
np_real:
            sta         n_v
            stx         n_v + 1
            stz         n_pl
            stz         n_pl + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIXED
            beq         @fix
            lda         n_v                                 ; (A rational: n/d)
            ldx         n_v + 1
            jsr         n_first
            ldy         nt
            sty         t_re
            ldy         nt + 1
            sty         t_re + 1
            jsr         np_int
            lda         #'/'
            jsr         o_put
            stz         n_pl
            stz         n_pl + 1
            lda         t_re
            ldx         t_re + 1
            bra         np_int
@fix:
            lda         n_v                                 ; (A fixed decimal: its digits, a point before its places)
            ldx         n_v + 1
            jsr         n_first
            bra         np_int
@int:
            lda         n_v
            ldx         n_v + 1
; The integer .A, .X in decimal, a point before its last n_pl digits ("0." and 0s first if there are no more)
np_int:
            ldy         #0
            sty         nrc
            jsr         r_load
            ldx         #0
            lda         r_neg,x
            beq         :+
            lda         #'-'
            jsr         o_put
:
            jsr         t_prefix
            jsr         np_pairs
            lda         n_pl
            ora         n_pl + 1
            beq         @all
            lda         t_k                                 ; (Digits before the point: t_left)
            sec
            sbc         n_pl
            sta         t_left
            lda         t_k + 1
            sbc         n_pl + 1
            sta         t_left + 1
            bmi         @zeros
            ora         t_left
            bne         @digits
@zeros:
            lda         #'0'
            jsr         o_put
            lda         #'.'
            jsr         o_put
@zero:
            lda         t_left                              ; (A 0 for each t_left is below 0)
            ora         t_left + 1
            beq         @all
            lda         #'0'
            jsr         o_put
            inc         t_left
            bne         @zero
            inc         t_left + 1
            bra         @zero
@all:
            lda         #$FF                                ; (No point to come)
            sta         t_left
            sta         t_left + 1
@digits:
            jsr         np_prev                             ; (The top pair: its tens' 0 left out)
            jsr         np_tens
            cmp         #'0'
            beq         :+
            jsr         np_digit
:
            lda         t_c
            jsr         np_digit
@pair:
            lda         t_np
            bne         :+
            dec         t_np + 1
:
            dec         t_np
            lda         t_np
            ora         t_np + 1
            beq         @rts
            jsr         np_prev
            jsr         np_tens
            jsr         np_digit
            lda         t_c
            jsr         np_digit
            bra         @pair
@rts:
            rts

; .A = the pair before nd (nd moved back to it)
np_prev:
            lda         nd
            bne         :+
            dec         nd + 1
:
            dec         nd
            lda         (nd)
            rts

; IN: .A = a pair (0-99).  OUT: .A = its tens' digit, t_c its ones' (characters)
np_tens:
            ldx         #'0' - 1
            sec
:
            inx
            sbc         #10
            bcs         :-
            adc         #10 + '0'
            sta         t_c
            txa
            rts

; The digit .A out, and a point after it if it's the last before the point
np_digit:
            jsr         o_put
            lda         t_left
            bne         :+
            dec         t_left + 1
:
            dec         t_left
            lda         t_left
            ora         t_left + 1
            bne         @rts
            lda         #'.'
            jmp         o_put
@rts:
            rts

; R0's magnitude in decimal: its digits in pairs (each 0-99, least first) on the digits' pages, t_np of them, nd past
; the last; t_k digits (the top pair's tens' 0 not counted).  Uses R1
np_pairs:
            lda         #0
            sta         nra
            lda         #1
            sta         nrc
            jsr         r_copy
            ldx         #1
            stz         r_neg,x
            LDR         nd, DIGS
            stz         t_np
            stz         t_np + 1
@pair:
            lda         #100
            jsr         r_divsmall
            sta         (nd)
            inc         nd
            bne         :+
            inc         nd + 1
:
            inc         t_np
            bne         :+
            inc         t_np + 1
:
            ldx         #1
            lda         r_len,x
            bne         @pair
            lda         t_np                                ; (Two digits a pair, less the top one's tens' 0)
            asl
            sta         t_k
            lda         t_np + 1
            rol
            sta         t_k + 1
            jsr         np_prev
            inc         nd
            bne         :+
            inc         nd + 1
:
            cmp         #10
            bcs         @rts
            lda         t_k
            bne         :+
            dec         t_k + 1
:
            dec         t_k
@rts:
            rts

; ---- Read

; PARSE: r0 = text, r1 = its length; r4 = a base string (0: the base); .Y bit 0 = 1 if the text is a program's (a bare
; number there starts with a digit 0-9), bit 1 = 1 if the whole text must be the number.  The number the text starts
; with (its longest start that's one, among its first TEXT_MAX characters: spaces before it, and after it, passed
; over; or, bit 1, the whole text, spaces at its ends passed over) at r2; r5 = the characters it took.  NE_NOTNUM: it
; starts with none (it isn't one); NE_DIV0: the longest that might be is a rational of 0 below (1/0); NE_BASE
nm_parse:
            jsr         nm_begin
            tya
            and         #1
            sta         t_src
            tya
            and         #2
            sta         t_whole
            jsr         b_call
            lda         t_whole                             ; (The whole text: TEXT_MAX characters at most)
            beq         :+
            lda         r1 + 1
            bne         @not
:
            MOVR        np, r0                              ; (Its first TEXT_MAX characters: t_raw)
            lda         r1 + 1
            bne         :+
            lda         r1
            cmp         #TEXT_MAX
            bcc         :++
:
            lda         #TEXT_MAX
:
            sta         t_len
            ldx         #0
:
            cpx         t_len
            beq         :+
            jsr         c_get
            sta         t_raw,x
            inx
            bra         :-
:
            MOVR        t_mark, a_top
            lda         t_len
            sta         t_try
            lda         #1                                  ; (Its longest start that's a number: each checked, as
            sta         t_chk                               ;   numbers are written, not made; then that one made)
@try:
            lda         t_try
            beq         @not
            jsr         p_text
            bcc         @make
            lda         t_whole                             ; (The whole text: one try)
            bne         @not
            dec         t_try
            bra         @try
@not:
            lda         #NE_NOTNUM
            jmp         nm_fail
@make:
            stz         t_chk
            MOVR        a_top, t_mark
            jsr         p_text
            bcs         @not
@num:
            lda         t_try
            sta         r5
            stz         r5 + 1
            lda         hv
            ldx         hv + 1
            jmp         nm_out

; t_raw's first t_try characters a number (numref.js's parse)?  Its spaces at its ends out, then its _s (t_num);
; then a real number, or a complex one: an imaginary part, then i; or a real part, + or -, an imaginary part, i.
; OUT: C = 0 and hv the number (t_chk: none made); C = 1 if they aren't one
p_text:
            ldx         #0                                  ; (The spaces at its ends out)
:
            cpx         t_try
            bne         @lb209
            jmp         @none
@lb209:
            lda         t_raw,x
            jsr         np_space_p
            bne         :+
            inx
            bra         :-
:
            ldy         t_try
:
            lda         t_raw - 1,y
            jsr         np_space_p
            bne         :+
            dey
            bra         :-
:
            sty         t_to
            ldy         #0                                  ; (Its _s out)
@copy:
            cpx         t_to
            beq         :+
            lda         t_raw,x
            inx
            cmp         #'_'
            beq         @copy
            sta         t_num,y
            iny
            bra         @copy
:
            sty         t_n
            tya
            bne         @lb208
            jmp         @none
@lb208:
            lda         #0                                  ; (A real number)
            ldx         t_n
            ldy         t_src
            jsr         p_real
            bcs         :+
            rts
:
            ldx         t_n                                 ; (Else i last, an imaginary part before it)
            lda         t_num - 1,x
            cmp         #'i'
            beq         @lb207
            jmp         @none
@lb207:
            dex
            bne         @lb206
            jmp         @none
@lb206:
            stx         t_n
            MOVR        a_top, t_mark
            lda         #0
            ldx         t_n
            ldy         t_src
            jsr         p_real
            bcs         @sum
            lda         t_chk
            bne         @yes
            lda         hv
            ldx         hv + 1
            sta         n_y
            stx         n_y + 1
            LDAX        n_zero
            sta         n_x
            stx         n_x + 1
            jsr         n_cpx_make
@yes:
            clc
            rts
@sum:
            ldx         t_n                                 ; (A real part, + or -, an imaginary part: each + or -
            dex                                             ;   from the last back)
            stx         t_sp
@part:
            ldx         t_sp
            beq         @none
            lda         t_num,x
            cmp         #'+'
            beq         :+
            cmp         #'-'
            bne         @next
:
            MOVR        a_top, t_mark
            lda         #0
            ldx         t_sp
            ldy         t_src
            jsr         p_real
            bcs         @next
            sta         t_re
            stx         t_re + 1
            lda         t_sp
            ldx         t_n
            ldy         #0
            jsr         p_real
            bcs         @next
            lda         t_chk
            bne         @yes
            lda         hv
            ldx         hv + 1
            sta         n_y
            stx         n_y + 1
            MOVR        n_x, t_re
            jsr         n_cpx_make
            clc
            rts
@next:
            dec         t_sp
            bra         @part
@none:
            sec
            rts

; Is .A a space (as .NET's Trim has it, in Latin-1)?  OUT: Z = 1 if it is.  Keeps .X, .Y
np_space_p:
            cmp         #' '
            beq         @rts
            cmp         #$85
            beq         @rts
            cmp         #$A0
            beq         @rts
            cmp         #TAB
            bcc         @no
            cmp         #CR + 1
            bcs         @no
            lda         #0
            rts
@no:
            lda         #1
@rts:
            rts

; t_num from .A to .X, a real number (numref.js's parseReal); .Y = 1 if a bare one must start with 0-9.  A rational
; (n/d: one /, no point, two integers; 0 below: NE_DIV0), or one number.  OUT: C = 0 and .A, .X (hv) = it, in the
; arena; C = 1 if it isn't one
p_real:
            sta         t_from
            stx         t_to
            sty         t_c
            stz         t_dot
            ldy         #0                                  ; (Its /s, and whether there's a point)
            ldx         t_from
@scan:
            cpx         t_to
            beq         @scanned
            lda         t_num,x
            cmp         #'.'
            bne         :+
            sta         t_dot
:
            cmp         #'/'
            bne         :+
            iny
            stx         t_slash
:
            inx
            bra         @scan
@scanned:
            tya
            bne         :+
            lda         t_from
            ldx         t_to
            ldy         t_c
            jmp         p_one
:
            cmp         #1                                  ; (One /, no point, a part each side)
            beq         @lb213
            jmp         @not
@lb213:
            lda         t_dot
            beq         @lb212
            jmp         @not
@lb212:
            lda         t_slash
            cmp         t_from
            bne         @lb211
            jmp         @not
@lb211:
            inc         a
            cmp         t_to
            beq         @not
            lda         t_to
            pha
            lda         t_slash
            pha
            lda         t_from                              ; (Its numerator, an integer: R4)
            ldx         t_slash
            ldy         t_c
            jsr         p_one
            bcs         @pull
            lda         t_chk
            bne         @den
            lda         hv
            ldx         hv + 1
            jsr         n_kind
            bne         @pull
            lda         #4
            sta         nrc
            lda         hv
            ldx         hv + 1
            jsr         r_load
@den:
            pla                                             ; (Its denominator, an integer: R5)
            inc         a
            plx
            ldy         #0
            jsr         p_one
            bcs         @not
            lda         t_chk
            beq         :+
            clc
            rts
:
            lda         hv
            ldx         hv + 1
            jsr         n_kind
            bne         @not
            lda         #5
            sta         nrc
            lda         hv
            ldx         hv + 1
            jsr         r_load
            ldx         #5                                  ; (0 below: NE_DIV0; its sign on top)
            lda         r_len,x
            bne         :+
            lda         #NE_DIV0
            jmp         nm_fail
:
            lda         r_neg,x
            beq         :+
            jsr         r_negate
            ldx         #4
            jsr         r_negate
:
            jsr         n_rat
            clc
            rts
@pull:
            pla
            pla
@not:
            sec
            rts

; t_num from .A to .X, one number (no /): a sign (+ or -, unless the number's bare, no # after its +s and -s, and the
; base has it as a digit), then a number in a base of its own (#...), or a bare one in f_ (.Y = 1: it starts 0-9).
; OUT: C = 0 and .A, .X (hv) = it; C = 1 if it isn't one
p_one:
            sta         t_from
            stx         t_to
            sty         t_c
            jsr         b_cur
            stz         t_sign
            ldx         t_from
            cpx         t_to
            bne         @lb205
            jmp         @not
@lb205:
            lda         t_num,x                             ; (A sign?)
            cmp         #'+'
            beq         @sign
            cmp         #'-'
            bne         @body
@sign:
            ldy         t_from                              ; (# after its +s and -s: it's a sign)
@skip:
            lda         t_num,y
            cmp         #'+'
            beq         :+
            cmp         #'-'
            bne         @after
:
            iny
            cpy         t_to
            bne         @skip
@after:
            cpy         t_to
            beq         @bare
            lda         t_num,y
            cmp         #'#'
            beq         @signed
@bare:
            lda         t_num,x                             ; (Bare: a sign unless the base has it as a digit)
            jsr         b_value
            bcc         @body
@signed:
            lda         t_num,x
            cmp         #'-'
            bne         :+
            dec         t_sign
:
            inx
            cpx         t_to
            beq         @not
@body:
            stx         t_from
            stz         t_np
            stz         t_np + 1
            lda         t_num,x
            cmp         #'#'
            bne         @plain
            jsr         p_based
            bcs         @not
            bra         @value
@plain:
            lda         t_c                                 ; (A program's: 0-9 first)
            beq         :+
            lda         t_num,x
            cmp         #'0'
            bcc         @not
            cmp         #'9' + 1
            bcs         @not
:
            lda         w_dec
            beq         @digits
            jsr         p_decimal
            bcs         @not
            bra         @value
@digits:
            jsr         p_ends                              ; (Another base: no point first or last)
            bcs         @not
            jsr         p_digits
            bcs         @not
@value:
            lda         t_chk                               ; (Checked only: nothing made)
            bne         @yes
            lda         t_sign                              ; (Its sign)
            beq         :+
            ldx         #0
            jsr         r_negate
:
            jsr         p_make
@yes:
            clc
            rts
@not:
            sec
            rts

; t_num from t_from to t_to: something, and no point first or last?  OUT: C = 1 if not
p_ends:
            ldx         t_from
            cpx         t_to
            beq         @no
            lda         t_num,x
            cmp         #'.'
            beq         @no
            ldx         t_to
            lda         t_num - 1,x
            cmp         #'.'
            beq         @no
            clc
            rts
@no:
            sec
            rts

; t_num from t_from to t_to in decimal, [0-9]+(\.[0-9]+)?: R0 = its digits; t_dot <> 0, a point (t_np: the digits
; after it).  OUT: C = 1 if it isn't
p_decimal:
            lda         #0
            sta         nrc
            tax
            jsr         r_set16
            stz         t_dot
            ldx         t_from
            lda         t_num,x
            cmp         #'.'
            beq         @not
@char:
            lda         t_num,x
            cmp         #'.'
            bne         :+
            lda         t_dot
            bne         @not
            dec         t_dot
            inx
            cpx         t_to
            beq         @not
            bra         @char
:
            sec
            sbc         #'0'
            cmp         #10
            bcs         @not
            sta         t_d
            lda         t_chk
            bne         @counted
            phx
            lda         #0
            sta         nrc
            lda         #10
            jsr         r_mulsmall
            lda         #2
            sta         nrc
            lda         t_d
            ldx         #0
            jsr         r_set16
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jsr         r_add
            plx
@counted:
            lda         t_dot
            beq         :+
            inc         t_np
            bne         :+
            inc         t_np + 1
:
            inx
            cpx         t_to
            bne         @char
            clc
            rts
@not:
            sec
            rts

; t_num from t_from to t_to, digits of w_ with a point at most (danlang's AddString, all of it): R0 = their value;
; t_dot <> 0, a point: R1 the base to the power of the digits after it (as they're read: least first, from the last
; back), t_np how many.  OUT: C = 1 if they aren't
p_digits:
            lda         #0                                  ; (R0 = 0, R1 = 1)
            sta         nrc
            tax
            jsr         r_set16
            lda         #1
            sta         nrc
            ldx         #0
            jsr         r_set16
            stz         t_dot
            ldx         t_from
            lda         w_le
            beq         @take
            ldx         t_to
            dex
@take:
            stx         t_p
            lda         t_num,x
            cmp         #'.'
            bne         @digit
            lda         t_dot
            bne         @no
            dec         t_dot
            bra         @next
@digit:
            jsr         b_value
            bcs         @no
            sta         t_d                                 ; (R0 = R0 * base + it)
            lda         t_chk
            bne         @next
            lda         #0
            sta         nrc
            jsr         p_times
            lda         t_d
            ldx         #0
            cmp         #$80
            bcc         :+
            dex
:
            ldy         #2
            sty         nrc
            jsr         r_set16
            lda         #0
            sta         nra
            sta         nrc
            lda         #2
            sta         nrb
            jsr         r_add
            lda         t_dot                               ; (After the point: R1 times the base too)
            beq         @next
            lda         #1
            sta         nrc
            jsr         p_times
            inc         t_np
            bne         @next
            inc         t_np + 1
@next:
            ldx         t_p
            lda         w_le
            bne         @down
            inx
            cpx         t_to
            bcc         @take
            clc
            rts
@down:
            cpx         t_from
            beq         @done
            dex
            bra         @take
@done:
            clc
            rts
@no:
            sec
            rts

; nrc = nrc * the base (negative if w_neg)
p_times:
            lda         w_radix
            jsr         r_mulsmall
            lda         w_neg
            beq         @rts
            ldx         nrc
            jsr         r_negate
@rts:
            rts

; hv (.A, .X) = the number read: R0 (no point); with a point, in base 10 a fixed decimal of t_np places (its digits
; R0), in another base a rational, R0 / R1
p_make:
            lda         t_dot
            beq         @int
            lda         w_radix
            cmp         #10
            bne         @rat
            lda         w_neg
            bne         @rat
            MOVR        n_pl, t_np
            jmp         n_fix0
@rat:
            lda         #0
            sta         nra
            lda         #4
            sta         nrc
            jsr         r_copy
            lda         #1
            sta         nra
            lda         #5
            sta         nrc
            jsr         r_copy
            ldx         #5
            lda         r_neg,x
            beq         :+
            jsr         r_negate
            ldx         #4
            jsr         r_negate
:
            jmp         n_rat
@int:
            lda         #0
            sta         nrc
            jmp         r_store

; t_num from t_from (its #) to t_to, a number in a base of its own (numref.js's parseBased), its digits w_'s (R0, R1:
; p_digits).  #[<>]?=?[+-]? and a radix ([1-7]?[0-9]|80, then r) or digits of its own ([...]: 2-80 of 0-9, a-z, A-Z
; and `~!@#$%^&*-=+|;:,?, each once), then its digits and a point at most (none first or last); a radix below 2, or
; digits not each once, isn't a number.  Else #[<>]?[+-]? and a named base's letter, then its digits so.  OUT: C = 1
; if it isn't one
p_based:
            ldx         t_from                              ; (Past #: < or >, =, + or -)
            inx
            jsr         p_dir
            stx         t_p
            stz         t_eq
            lda         t_num,x
            cmp         #'='
            bne         :+
            dec         t_eq
            inx
:
            jsr         p_sgn
            jsr         p_radix
            bcc         @value
            lda         t_k                                 ; (A radix below 2, or digits not each once: not one)
            bne         @no
            ldx         t_p                                 ; (A named base: [+-]?, its letter)
            jsr         p_sgn
            cpx         t_to
            bcs         @no
            lda         t_num,x
            jsr         b_named
            bcs         @no
            inx
            phx
            jsr         b_finish
            plx
@value:
            stx         t_from
            jsr         p_ends
            bcs         @no
            jmp         p_digits
@no:
            sec
            rts

; t_dir = < ($FF), > (1) or neither (0) at t_num,x; .X past it
p_dir:
            stz         t_dir
            lda         t_num,x
            cmp         #'<'
            bne         :+
            dec         t_dir
            inx
            rts
:
            cmp         #'>'
            bne         :+
            inc         t_dir
            inx
:
            rts

; t_sgn = - ($FF), + (0) or neither ($80) at t_num,x; .X past it
p_sgn:
            lda         #$80
            sta         t_sgn
            lda         t_num,x
            cmp         #'-'
            bne         :+
            lda         #$FF
            sta         t_sgn
            inx
            rts
:
            cmp         #'+'
            bne         :+
            stz         t_sgn
            inx
:
            rts

; A base of its own at t_num,x: a radix ([1-7]?[0-9]|80, then r or R) of n_std's digits, or digits of its own (2-80
; legal ones, each once).  OUT: C = 0 and .X past it, w_ that base (= t_eq, < t_dir, - t_sgn), finished; C = 1 if
; there's none there (t_k <> 0: a radix below 2, or digits not each once: no number at all)
p_radix:
            stz         t_k
            lda         t_num,x
            cmp         #'['
            beq         @own
            jsr         p_dec
            bcc         @lb204
            jmp         @no
@lb204:
            sta         w_radix
            inx
            jsr         p_dec                               ; (Two digits: 1-7 and any, or 80)
            bcs         @r
            ldy         w_radix
            bne         @lb203
            jmp         @no
@lb203:
            cpy         #8
            bcc         @two
            beq         @lb202
            jmp         @no
@lb202:
            cmp         #0
            beq         @lb201
            jmp         @no
@lb201:
@two:
            sta         t_d
            lda         w_radix
            asl
            asl
            adc         w_radix
            asl
            adc         t_d
            sta         w_radix
            inx
@r:
            lda         t_num,x
            ora         #$20
            cmp         #'r'
            bne         @no
            inx
            lda         w_radix
            cmp         #2
            bcs         :+
            inc         t_k
            bra         @no
:
            ldy         #0
:
            lda         n_std,y
            sta         w_dset,y
            iny
            cpy         w_radix
            bne         :-
            bra         @made
@own:
            inx                                             ; (Its own digits: to the ])
            ldy         #0
@sdig:
            cpx         t_to
            bcs         @no
            lda         t_num,x
            cmp         #']'
            beq         @send
            phy
            jsr         np_legal
            ply
            bcs         @no
            cpy         #80
            bcs         @no
            sta         w_dset,y
            iny
            inx
            bra         @sdig
@send:
            inx
            sty         w_radix
            cpy         #2
            bcc         @no
            phx                                             ; (Each once)
            ldx         #0
@one:
            txa
            tay
@other:
            iny
            cpy         w_radix
            bcs         @onext
            lda         w_dset,x
            cmp         w_dset,y
            bne         @other
            plx
            inc         t_k
            bra         @no
@onext:
            inx
            cpx         w_radix
            bne         @one
            plx
@made:
            jsr         b_said
            phx
            jsr         b_finish
            plx
            clc
            rts
@no:
            sec
            rts

; t_num,x a decimal digit?  OUT: C = 0 and .A = its value
p_dec:
            lda         t_num,x
            sec
            sbc         #'0'
            cmp         #10
            rts

; Is .A a legal digit of a base of its own (0-9, a-z, A-Z, `~!@#$%^&*-=+|;:,?)?  OUT: C = 0 if it is.  Keeps .A, .X
np_legal:
            ldy         #NLEGAL - 1
:
            cmp         n_legal,y
            beq         @yes
            dey
            bpl         :-
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @yes
            pha
            ora         #$20
            cmp         #'a'
            bcc         @no1
            cmp         #'z' + 1
            pla
            bcc         @yes
@no:
            sec
            rts
@no1:
            pla
            sec
            rts
@yes:
            clc
            rts

; ---- FORMAT

; FORMAT: r0 = a format string (its bytes and a 0); r4 = its arguments: 3 bytes each, a kind (0 a number, 1 a string
; with a 0 after it, $FF the table's end) and its address.  Its text at r2 (room r3), .A, .X its length (danlang's
; format): the string's bytes, each {} its argument (a number in the base, a string as it is), {spec} a number in that
; base, {{ and }} a brace (a { with no } after it, or a } alone: itself).  NE_FORMAT: too few arguments or too many, or
; a string for a {spec}; NE_BASE: a spec names no base; NE_ROOM
nm_format:
            jsr         nm_begin
            MOVR        f_ptr, r0
            MOVR        f_args, r4
            jsr         o_start
            MOVR        t_mark, a_top
@char:
            ldy         #G_STRING                           ; (The string's next byte)
            jsr         g_get
            bne         @lb200
            jmp         @end
@lb200:
            cmp         #'{'
            beq         @open
            cmp         #'}'
            bne         @byte
            jsr         f_peek                              ; (}}: a brace)
            cmp         #'}'
            bne         :+
            ldy         #G_STRING
            jsr         g_get
:
            lda         #'}'
@byte:
            jsr         o_put
            bra         @char
@open:
            jsr         f_peek                              ; ({{: a brace)
            cmp         #'{'
            bne         :+
            ldy         #G_STRING
            jsr         g_get
            lda         #'{'
            bra         @byte
:
            jsr         f_close                             ; (A } after it: a placeholder; else itself)
            bcc         :+
            lda         #'{'
            bra         @byte
:
            jsr         f_arg                               ; (Its argument: its kind; f_at, its address)
            cmp         #$FF
            beq         @wrong
            cmp         #1
            beq         @string
            lda         t_n                                 ; (A number: in the base, or the spec's)
            bne         :+
            jsr         b_state
            bra         @num
:
            jsr         b_parse
@num:
            MOVR        a_top, t_mark
            lda         np                                  ; (Its copy: np the text's place still)
            pha
            lda         np + 1
            pha
            lda         f_at
            ldx         f_at + 1
            jsr         nm_in
            sta         n_x
            stx         n_x + 1
            pla
            sta         np + 1
            pla
            sta         np
            lda         n_x
            ldx         n_x + 1
            jsr         t_number
            jmp         @char
@string:
            lda         t_n                                 ; (A string, as it is: a {spec}'s, NE_FORMAT)
            bne         @wrong
:
            ldy         #G_AT
            jsr         g_get
            beq         :+
            jsr         o_put
            bra         :-
:
            jmp         @char
@end:
            jsr         f_arg                               ; (No argument left over)
            cmp         #$FF
            bne         @wrong
            lda         o_len
            ldx         o_len + 1
            clc
            jmp         nm_end
@wrong:
            lda         #NE_FORMAT
            jmp         nm_fail

; .A = the caller's byte at the pointer at g_ptrs + .Y (moved on; np, the text's place, kept); Z = 1 if it's 0.
; Keeps .Y
g_get:
            lda         np
            pha
            lda         np + 1
            pha
            lda         g_ptrs,y
            sta         np
            lda         g_ptrs + 1,y
            sta         np + 1
            jsr         c_get
            tax
            lda         np
            sta         g_ptrs,y
            lda         np + 1
            sta         g_ptrs + 1,y
            pla
            sta         np + 1
            pla
            sta         np
            txa
            rts

; .A = the format string's next byte, f_ptr where it was
f_peek:
            lda         f_ptr
            pha
            lda         f_ptr + 1
            pha
            ldy         #G_STRING
            jsr         g_get
            tax
            pla
            sta         f_ptr + 1
            pla
            sta         f_ptr
            txa
            rts

; A } ahead of f_ptr (before the string's 0)?  OUT: C = 0, the spec between in f_spec (t_n its length; past BASE_MAX
; - 1, one that names no base), f_ptr past the }; C = 1, f_ptr as it was, if there's none
f_close:
            lda         f_ptr
            pha
            lda         f_ptr + 1
            pha
            stz         t_c                                 ; (Its length so far)
@byte:
            ldy         #G_STRING
            jsr         g_get
            beq         @none
            cmp         #'}'
            beq         @found
            ldx         t_c
            cpx         #BASE_MAX - 1
            bcs         :+
            sta         f_spec,x
            inc         t_c
            bra         @byte
:
            inc         t_c                                 ; (Past BASE_MAX - 1: counted so, no more)
            bne         @byte
            dec         t_c
            bra         @byte
@found:
            ldx         t_c
            stx         t_n
            cpx         #BASE_MAX - 1
            bcc         :+
            ldx         #0                                  ; (Too long: ] alone, which names none)
            lda         #']'
            sta         f_spec
            inx
:
            stz         f_spec,x
            pla
            pla
            clc
            rts
@none:
            pla
            sta         f_ptr + 1
            pla
            sta         f_ptr
            sec
            rts

; .A = the next argument's kind (the table's next entry: f_args moved on, f_at its address; $FF, the end, stays)
f_arg:
            ldy         #G_ARGS
            jsr         g_get
            cmp         #$FF
            beq         @end
            pha
            jsr         g_get
            sta         f_at
            jsr         g_get
            sta         f_at + 1
            pla
            rts
@end:
            lda         f_args                              ; (The end: f_args back to it)
            bne         :+
            dec         f_args + 1
:
            dec         f_args
            lda         #$FF
            rts

.rodata
; The named bases: each one's letter, size, digits and flags (balanced, least first, negative)
NB_BAL          = $01
NB_LE           = $02
NB_NEG          = $04
nb_letter:  .byte       "cegijmbtqvfsondxzky"
nb_size:    .byte       3, 5, 7, 3, 5, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 36, 27, 53
nb_flags:   .byte       NB_BAL | NB_LE, NB_BAL | NB_LE, NB_BAL | NB_LE, NB_BAL | NB_LE, NB_BAL | NB_LE
            .byte       NB_BAL | NB_LE | NB_NEG, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NB_BAL, NB_BAL
nb_digs_lo: .lobytes    nd_c, nd_e, nd_g, nd_i, nd_j, nd_m, n_std, n_std, n_std, n_std, n_std, n_std, n_std
            .lobytes    n_std, n_std, n_std, n_std, nd_k, nd_y
nb_digs_hi: .hibytes    nd_c, nd_e, nd_g, nd_i, nd_j, nd_m, n_std, n_std, n_std, n_std, n_std, n_std, n_std
            .hibytes    n_std, n_std, n_std, n_std, nd_k, nd_y
nd_c:       .byte       "-0+"
nd_e:       .byte       "=-0+#"
nd_g:       .byte       "~=-0+#*"
nd_i:       .byte       "UON"
nd_j:       .byte       "WUONM"
nd_m:       .byte       "DanielStphrus"
nd_k:       .byte       "ZYXWVUTSRQPON0ABCDEFGHIJKLM"
nd_y:       .byte       "zyxwvutsrqponmlkjihgfedcba0ABCDEFGHIJKLMNOPQRSTUVWXYZ"
; A radix's digits: the first so many of these
n_std:      .byte       "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-=+`~!@#$%^&*,;:|?"
; The digits a base of its own may have in a number's text, past 0-9, a-z and A-Z
n_legal:    .byte       "`~!@#$%^&*-=+|;:,?"
NLEGAL          = * - n_legal
; What's never a digit (danlang's IsUniqueSet), past the control characters and the space
n_invalid:  .byte       "<>[]()./_\'", $22
NINVALID        = * - n_invalid
.code
