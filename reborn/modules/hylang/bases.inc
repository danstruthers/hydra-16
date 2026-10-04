; ****************************************************************************
; bases.inc - hylang's numbers in other bases (step 7.3), in the module's third bank: danlang's (its NumberParser).
; A number in a base is #, then < (its digits the least first) or > (the most first), then = (balanced: its digits'
; values -n to n; a #NNr or #[...] base's only), then + or - (a positive or negative radix), then the base: a letter
; (bs_named: #x, #b, #z, #c ...), a radix NNr (2-80: bs_std's digits), or [digits] (its own; reading only); then the
; digits, a . among them for a fraction (a decimal fraction is a fixed decimal; another, a rational).  A - before
; the # makes it negative.  num_to_base writes an integer, or a rational (each part in the base), the same way.

BS_LE           = $01           ; A base's flags: its digits the least first ...
BS_BAL          = $02           ;   balanced (the middle digit 0) ...
BS_NEG          = $04           ;   its radix negative ...
BS_CASE         = $08           ;   its digits' case matters

bs_chars    = bi_src                                        ; The base: its digits (a pointer: builtins.inc's, free
bs_spec     = bi_q                                          ; here) ...; num_to_base's base, its text (a pointer)

.bss
bs_n:       .res        1                                   ;   how many (the radix's size) ...
bs_flags:   .res        1                                   ;   its flags (BS_*) ...
bs_zero:    .res        1                                   ;   the place of the digit 0 among them ...
bs_hi:      .res        1                                   ;   the highest digit's value ...
bs_cbuf:    .res        80                                  ;   a #[...] base's digits
bs_ovle:    .res        1                                   ; A < or >: $80 + BS_LE, or $80 (else 0)
bs_ovneg:   .res        1                                   ; A + or -: $80, or $80 + BS_NEG (else 0)
bs_eq:      .res        1                                   ; Not 0: an = (balanced)
bs_from:    .res        1                                   ; The digits: their first byte's offset ...
bs_k:       .res        1                                   ;   the digits after a . ...
bs_pt:      .res        1                                   ;   not 0: a . seen
bs_spec_n:  .res        1                                   ; num_to_base's base: its text's length
.segment "CODE3"

; ---- Reading

; The number in a base at r0's .Y (its #) to p_end, a - before it if p_sign is $80.  OUT: as num_parse's
parse_base:
            iny                                             ; ---- Past the #: < or >, =, + or -
            stz         bs_ovle
            stz         bs_ovneg
            stz         bs_eq
            jsr         pb_byte
            jcs         pb_not
            cmp         #'<'
            bne         :+
            lda         #$80 | BS_LE
            bra         @dir
:
            cmp         #'>'
            bne         @eq
            lda         #$80
@dir:
            sta         bs_ovle
            iny
@eq:
            jsr         pb_byte
            jcs         pb_not
            cmp         #'='
            bne         @mod
            inc         bs_eq
            iny
@mod:
            jsr         pb_byte
            jcs         pb_not
            cmp         #'+'
            bne         :+
            lda         #$80
            bra         @neg
:
            cmp         #'-'
            bne         @base
            lda         #$80 | BS_NEG
@neg:
            sta         bs_ovneg
            iny
@base:
            jsr         pb_byte                             ; ---- The base
            jcs         pb_not
            cmp         #'['
            jeq         @custom
            jsr         pb_radix
            bcc         @radix
            cpx         #0
            jne         pb_not                              ; (Digits, but not a radix)
            lda         bs_eq                               ; (A letter: no =)
            jne         pb_not
            lda         (r0),y
            jsr         bs_named_find
            jcs         pb_not
            iny
            bra         @value
@radix:
            jsr         bs_radix                            ; (.A, 2-80: the standard digits)
            bra         @value
@custom:
            iny                                             ; ([digits]: as many as there are, 2-80, till the ])
            ldx         #0
:
            jsr         pb_byte
            jcs         pb_not
            iny
            cmp         #']'
            beq         :+
            cpx         #80
            jcs         pb_not
            sta         bs_cbuf,x
            inx
            bra         :-
:
            cpx         #2
            jcc         pb_not
            stx         bs_n
            LDR         bs_chars, bs_cbuf
            stz         bs_flags
            jsr         bs_case                             ; (Its case: matters, unless they're all unique without)
@value:
            lda         bs_eq                               ; ---- The overrides
            beq         :+
            lda         bs_flags
            ora         #BS_BAL
            sta         bs_flags
:
            lda         bs_ovle
            beq         :+
            and         #BS_LE
            sta         n_t
            lda         bs_flags
            and         #$FF ^ BS_LE
            ora         n_t
            sta         bs_flags
:
            lda         bs_ovneg
            beq         :+
            and         #BS_NEG
            sta         n_t
            lda         bs_flags
            and         #$FF ^ BS_NEG
            ora         n_t
            sta         bs_flags
:
            jsr         bs_ready
            sty         bs_from                             ; ---- The digits: some, a . and some after it, or none
            stz         bs_pt
            stz         n_cnt                               ; (Digits seen since the start or the .)
:
            jsr         pb_byte
            bcs         @checked
            iny
            cmp         #'_'
            beq         :-
            cmp         #'.'
            bne         @digit
            lda         bs_pt
            jne         pb_not
            lda         n_cnt
            jeq         pb_not
            inc         bs_pt
            stz         n_cnt
            bra         :-
@digit:
            jsr         bs_digit
            jcs         pb_not
            inc         n_cnt
            bra         :-
@checked:
            lda         n_cnt
            jeq         pb_not
            jmp         pb_value

; The text's byte at .Y.  OUT: C = 1, it's at its end (p_end)
pb_byte:
            cpy         p_end
            bcs         :+
            lda         (r0),y
            clc
:
            rts

; A radix at .Y: one or two decimal digits, then r or R (.Y past it).  OUT: C = 0, .A = it (2-80); C = 1, .X = 0
; if it doesn't start with a digit (.Y as it was), else not one
pb_radix:
            ldx         #0
            sec
            sbc         #'0'
            cmp         #10
            bcc         :+
            sec
            rts
:
            ldx         #1
            sta         n_t
            iny
            jsr         pb_byte
            bcs         @no
            sec
            sbc         #'0'
            cmp         #10
            bcs         @r
            pha                                             ; (Two digits: 10 n + m, the first not 0 or 8 or 9 ...
            lda         n_t
            beq         @no2
            asl
            sta         n_s
            asl
            asl
            adc         n_s
            sta         n_t
            pla
            clc
            adc         n_t
            sta         n_t
            iny
            jsr         pb_byte
            bcs         @no
@r:
            lda         (r0),y
            ora         #$20
            cmp         #'r'
            bne         @no
            iny
            lda         n_t                                 ;   2 to 80)
            cmp         #2
            bcc         @no
            cmp         #81
            bcs         @no
            clc
            rts
@no2:
            pla
@no:
            ldx         #1
            sec
            rts

; The base of radix .A (2-80), the standard digits: their case matters past 36
bs_radix:
            sta         bs_n
            LDR         bs_chars, bs_std
            stz         bs_flags
            cmp         #37
            bcc         :+
            lda         #BS_CASE
            sta         bs_flags
:
            rts

; The named base .A (a letter: in either case).  OUT: C = 1, there's none
bs_named_find:
            ora         #$20
            ldx         #0
@look:
            cpx         #BS_NAMED * 5
            bcs         @none
            cmp         bs_named,x
            beq         @found
            inx
            inx
            inx
            inx
            inx
            bra         @look
@found:
            lda         bs_named + 1,x
            sta         bs_flags
            lda         bs_named + 2,x
            sta         bs_n
            lda         bs_named + 3,x
            sta         bs_chars
            lda         bs_named + 4,x
            sta         bs_chars + 1
            clc
            rts
@none:
            sec
            rts

; A #[...] base's case: BS_CASE if its digits aren't all different without it (else they're kept in upper case)
bs_case:
            ldx         #0
@i:
            cpx         bs_n
            bcs         @upper
            lda         bs_cbuf,x
            jsr         bs_up
            sta         n_t
            txa
            tay
@j:
            iny
            cpy         bs_n
            bcs         @next
            lda         bs_cbuf,y
            jsr         bs_up
            cmp         n_t
            bne         @j
            lda         bs_flags                            ; (Two the same: case matters)
            ora         #BS_CASE
            sta         bs_flags
            rts
@next:
            inx
            bra         @i
@upper:
            ldx         #0
:
            lda         bs_cbuf,x
            jsr         bs_up
            sta         bs_cbuf,x
            inx
            cpx         bs_n
            bne         :-
            rts

; .A in upper case (a-z only).  Keeps .X, .Y
bs_up:
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            rts

; The base's zero and its highest digit: from its count and flags.  Keeps .Y
bs_ready:
            lda         #0
            sta         bs_zero
            lda         bs_flags
            and         #BS_BAL
            beq         :+
            lda         bs_n
            lsr
            sta         bs_zero
:
            clc                                             ; (n - 1 - zero)
            lda         bs_n
            sbc         bs_zero
            sta         bs_hi
            rts

; The digit .A's value in the base: .A (signed: its place less bs_zero).  OUT: C = 1, it isn't one.  Keeps .Y
bs_digit:
            phy
            ldx         bs_flags                            ; (Case doesn't matter: upper case)
            pha
            txa
            and         #BS_CASE
            bne         :+
            pla
            jsr         bs_up
            pha
:
            pla
            sta         n_t
            ldy         #0
:
            cpy         bs_n
            bcs         @none
            lda         (bs_chars),y
            cmp         n_t
            beq         @found
            iny
            bra         :-
@found:
            tya
            sec
            sbc         bs_zero
            ply
            clc
            rts
@none:
            ply
            sec
            rts

; The digits checked (bs_from to p_end): their value, made (the least first, if BS_LE), into the number
pb_value:
            jsr         nq_init
            NQ          nq_c, nc
            jsr         nb_zero
            stz         bs_k
            stz         bs_pt
            ldy         bs_from                             ; (From the first byte to the last, or the other way)
            lda         bs_flags
            and         #BS_LE
            beq         @next
            ldy         p_end
@next:
            lda         bs_flags
            and         #BS_LE
            bne         @back
            cpy         p_end                               ; (Forward)
            beq         @done
            lda         (r0),y
            iny
            bra         @byte
@back:
            cpy         bs_from                             ; (Back)
            beq         @done
            dey
            lda         (r0),y
@byte:
            cmp         #'_'
            beq         @next
            cmp         #'.'
            bne         :+
            inc         bs_pt
            bra         @next
:
            phy
            jsr         bs_digit
            jsr         pb_add                              ; (C = C * the radix + the digit)
            ply
            lda         bs_pt                               ; (Past the .: a place more)
            beq         :+
            inc         bs_k
:
            lda         nc
            cmp         #NB_MAX + 1
            bcc         @next
            LDR         r0, nm_big
            jsr         io_error
            ldx         #RD_ERR
            sec
            rts
@done:
            jsr         nb_store                            ; ---- Its value: an integer, a fixed decimal, a
            bcs         @err                                ; rational; then its sign
            lda         bs_k
            beq         @sign
            lda         bs_n                                ; (Base 10, positive: a fixed decimal)
            cmp         #10
            bne         @rat
            lda         bs_flags
            and         #BS_NEG
            bne         @rat
            lda         bs_k
            sta         n_scale
            jsr         make_fix
            bcs         @err
            bra         @sign
@rat:
            jsr         nr_push                             ; (The digits over the radix to the bs_k)
            jsr         bs_radix_value
            MOVR        ia, h_v
            LDR         h_v, 3
:
            lda         #IO_MUL
            jsr         int_op
            bcs         @err1
            dec         bs_k
            beq         :+
            MOVR        ia, h_v
            jsr         bs_radix_value
            bra         :-
:
            jsr         nr_push
            jsr         rat_norm
            bcs         @err
@sign:
            lda         p_sign
            bpl         :+
            jsr         num_neg
            bcs         @err
:
            clc
            rts
@err1:
            lda         #1
            jsr         nr_drop
@err:
            ldx         #RD_ERR
            sec
            rts

; h_v = the radix, signed (a fixnum)
bs_radix_value:
            lda         bs_n
            ldx         #0
            asl
            ora         #1
            sta         h_v
            stx         h_v + 1
            lda         bs_flags
            and         #BS_NEG
            beq         :+
            sec                                             ; (-n: 2 - (2n + 1))
            lda         #2
            sbc         h_v
            sta         h_v
            lda         #0
            sbc         h_v + 1
            sta         h_v + 1
:
            rts

; C = C * the radix (signed) + .A (a digit's value, signed)
pb_add:
            pha
            lda         bs_n                                ; (Its size times the radix's size ...
            sta         n_s
            stz         n_t
            jsr         mag_muls
            lda         nc                                  ;   negative, for a negative radix)
            beq         :+
            lda         bs_flags
            and         #BS_NEG
            beq         :+
            lda         nc + 1
            eor         #$80
            sta         nc + 1
:
            pla                                             ; (Then the digit: B, a byte and a sign)
            stz         nb + 1
            cmp         #0
            bpl         :+
            eor         #$FF
            inc         a
            ldx         #$80
            stx         nb + 1
:
            sta         nb + 2
            lda         #1
            sta         nb
            NQ          nq_a, nc
            NQ          nq_b, nb
            NQ          nq_c, nc
            lda         nb + 2
            bne         :+
            stz         nb
:
            jmp         nb_add

; Not a number
pb_not:
            ldx         #0
            sec
            rts

; ---- Writing

; h_v (a number: not complex) written in the base whose text is bs_spec (bs_spec_n bytes: x, #x, #16r, #<-z ...; a
; rational's two parts each in it).  OUT: C = 1, h_v an error (the base isn't one; a complex number)
num_to_base:
            jsr         nr_push
            jsr         bs_spec_read
            bcs         @bad
            ldx         #0
            jsr         nr_peek
            jsr         num_kind
            cmp         #NK_COMPLEX
            beq         @complex
            cmp         #NK_INT
            beq         @int
            jsr         rat_parts                           ; (A rational, a fixed decimal: its numerator / its
            jsr         rat_norm                            ; denominator, in their lowest terms)
            bcs         @drop
            jsr         nr_push
            jsr         num_kind
            cmp         #NK_RAT
            bne         @int2
            ldy         #2
            jsr         num_field
            jsr         bs_write
            lda         #'/'
            jsr         out_char
            ldx         #0
            jsr         nr_peek
            ldy         #4
            jsr         num_field
            jsr         bs_write
            lda         #2
            clc
            jmp         nr_drop
@int2:
            jsr         bs_write
            lda         #2
            clc
            jmp         nr_drop
@int:
            jsr         bs_write
            clc
@drop:
            lda         #1
            jmp         nr_drop
@complex:
            LDR         r0, nm_complex_base
            jsr         io_error
            bra         @drop
@bad:
            LDR         r0, nm_bad_base                     ; (Invalid base specifier #q)
            jsr         msg_start
            lda         #'#'
            jsr         msg_char
            ldy         #0
            lda         (bs_spec),y
            cmp         #'#'
            bne         :+
            iny
:
            cpy         bs_spec_n
            bcs         :+
            lda         (bs_spec),y
            jsr         msg_char
            iny
            bra         :-
:
            jsr         msg_error
            MOVR        h_v, ev_v
            sec
            bra         @drop

; The base bs_spec read: # (or none), < or >, + or -, a letter or a radix (NNr), and nothing more.  OUT: C = 1, it
; isn't one
bs_spec_read:
            MOVR        r0, bs_spec
            lda         bs_spec_n
            sta         p_end
            stz         bs_ovle
            stz         bs_ovneg
            ldy         #0
            jsr         pb_byte
            jcs         @no
            cmp         #'#'
            bne         :+
            iny
:
            jsr         pb_byte
            jcs         @no
            cmp         #'<'
            bne         :+
            lda         #$80 | BS_LE
            sta         bs_ovle
            iny
            bra         @mod
:
            cmp         #'>'
            bne         @mod
            lda         #$80
            sta         bs_ovle
            iny
@mod:
            jsr         pb_byte
            bcs         @no
            cmp         #'+'
            bne         :+
            lda         #$80
            sta         bs_ovneg
            iny
            bra         @base
:
            cmp         #'-'
            bne         @base
            lda         #$80 | BS_NEG
            sta         bs_ovneg
            iny
@base:
            jsr         pb_byte
            bcs         @no
            jsr         pb_radix
            bcc         @radix
            cpx         #0
            bne         @no
            lda         (r0),y
            jsr         bs_named_find
            bcs         @no
            iny
            bra         @end
@radix:
            jsr         bs_radix
@end:
            cpy         p_end                               ; (Nothing more)
            bne         @no
            lda         bs_ovle                             ; (The overrides)
            beq         :+
            and         #BS_LE
            sta         n_t
            lda         bs_flags
            and         #$FF ^ BS_LE
            ora         n_t
            sta         bs_flags
:
            lda         bs_ovneg
            beq         :+
            and         #BS_NEG
            sta         n_t
            lda         bs_flags
            and         #$FF ^ BS_NEG
            ora         n_t
            sta         bs_flags
:
            jsr         bs_ready
            clc
            rts
@no:
            sec
            rts

; The integer h_v written in the base (bs_*): a - first (a positive radix's, not balanced, for a negative number),
; then the base's text (# and the rest: none for #d), then its digits
bs_write:
            jsr         nq_init
            NQ          nq_c, nc
            jsr         nb_load
            lda         nc + 1                              ; (A - first?)
            bpl         @prefix
            lda         bs_flags
            and         #BS_BAL | BS_NEG
            bne         @prefix
            lda         #'-'
            jsr         out_char
            stz         nc + 1
@prefix:
            ldy         #0                                  ; (The base's text: # added, unless it's #d)
            lda         (bs_spec),y
            cmp         #'#'
            bne         :+
            iny
:
            sty         n_t
            lda         bs_spec_n
            sec
            sbc         n_t
            cmp         #1
            bne         @hash
            lda         (bs_spec),y
            cmp         #'d'
            beq         @digits
@hash:
            lda         #'#'
            jsr         out_char
:
            cpy         bs_spec_n
            bcs         @digits
            lda         (bs_spec),y
            jsr         out_char
            iny
            bra         :-
@digits:
            NQ          nq_d, dg                            ; ---- Its digits, the least first, into dg
            stz         dg_n
            lda         bs_n
            sta         n_s
@digit:
            lda         nc + 1                              ; (Its sign, before the division)
            sta         n_sa
            jsr         mag_divs                            ; (|C| / the radix: .A the remainder)
            ldx         #0                                  ; (n_cnt: 1 if the quotient's 1 more)
            stx         n_cnt
            bit         n_sa
            bmi         @neg
            cmp         bs_hi                               ; (Not negative: the remainder, less the radix if
            beq         @have                               ; it's past the highest digit, the quotient 1 more)
            bcc         @have
            sec
            sbc         bs_n
            inc         n_cnt
            bra         @have
@neg:
            cmp         #0                                  ; (Negative: 0; or the radix less it, the quotient 1
            beq         @have                               ; more; or, past the highest digit, -it)
            sta         n_t
            lda         bs_n
            sec
            sbc         n_t
            inc         n_cnt
            cmp         bs_hi
            beq         @have
            bcc         @have
            lda         #0
            sec
            sbc         n_t
            stz         n_cnt
@have:
            clc                                             ; (Its digit)
            adc         bs_zero
            tay
            lda         (bs_chars),y
            sta         (nq_d)
            inc         nq_d
            bne         :+
            inc         nq_d + 1
:
            inc         dg_n
            lda         dg_n                                ; (255 digits at most: more is too long a string)
            cmp         #255
            beq         @stop
            lda         n_cnt                               ; (The quotient: 1 more?)
            beq         :+
            lda         #1
            sta         n_s
            sta         n_t
            jsr         mag_muls
            lda         bs_n
            sta         n_s
:
            lda         n_sa                                ; (Its sign: the number's, the other for a negative
            ldx         bs_flags                            ; radix)
            pha
            txa
            and         #BS_NEG
            beq         :+
            pla
            eor         #$80
            pha
:
            pla
            ldx         nc
            beq         :+
            sta         nc + 1
            jmp         @digit
:
@stop:
            stz         nc + 1
            lda         bs_flags                            ; ---- Written: the most first, or the least
            and         #BS_LE
            bne         @le
@be:
            lda         dg_n
            beq         @done
            lda         nq_d
            bne         :+
            dec         nq_d + 1
:
            dec         nq_d
            dec         dg_n
            lda         (nq_d)
            jsr         out_char
            bra         @be
@le:
            ldx         #0
:
            cpx         dg_n
            beq         @done
            lda         dg,x
            jsr         out_char
            inx
            bra         :-
@done:
            rts

.segment "RODATA3"
; The named bases (danlang's): each its letter, its flags, its digits' count and where they are
BS_NAMED        = 19
bs_named:
            .byte       'c', BS_LE | BS_BAL, 3
            .addr       bs_c
            .byte       'e', BS_LE | BS_BAL, 5
            .addr       bs_e
            .byte       'g', BS_LE | BS_BAL, 7
            .addr       bs_g
            .byte       'i', BS_LE | BS_BAL, 3
            .addr       bs_i
            .byte       'j', BS_LE | BS_BAL, 5
            .addr       bs_j
            .byte       'm', BS_LE | BS_BAL | BS_NEG | BS_CASE, 13
            .addr       bs_m
            .byte       'b', 0, 2
            .addr       bs_std
            .byte       't', 0, 3
            .addr       bs_std
            .byte       'q', 0, 4
            .addr       bs_std
            .byte       'v', 0, 5
            .addr       bs_std
            .byte       'f', 0, 6
            .addr       bs_std
            .byte       's', 0, 7
            .addr       bs_std
            .byte       'o', 0, 8
            .addr       bs_std
            .byte       'n', 0, 9
            .addr       bs_std
            .byte       'd', 0, 10
            .addr       bs_std
            .byte       'x', 0, 16
            .addr       bs_std
            .byte       'z', 0, 36
            .addr       bs_std
            .byte       'k', BS_BAL, 27
            .addr       bs_k27
            .byte       'y', BS_BAL | BS_CASE, 53
            .addr       bs_y
bs_c:       .byte       "-0+"
bs_e:       .byte       "=-0+#"
bs_g:       .byte       "~=-0+#*"
bs_i:       .byte       "UON"
bs_j:       .byte       "WUONM"
bs_m:       .byte       "DanielStphrus"
bs_k27:     .byte       "ZYXWVUTSRQPON0ABCDEFGHIJKLM"
bs_y:       .byte       "zyxwvutsrqponmlkjihgfedcba0ABCDEFGHIJKLMNOPQRSTUVWXYZ"
bs_std:     .byte       "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-=+`~!@#$%^&*,;:|?"
nm_complex_base: .byte  "A complex number has no form in another base", 0
nm_bad_base: .byte      "Invalid base specifier ", 0
.segment "CODE3"
