; ****************************************************************************
; numtext.inc - hylang's numbers as text (phase 5), in its third bank, as danlang's (Numbers.cs's ToString,
; NumberParser.cs): a number written (num_print), read (num_parse: each form the reference has, in every base) and
; written in a base (n_tobase: to-str's).  A base: its size; its digits (its own, n_text's at n_setp; a named base's,
; n_digs; or the first of n_std's); balanced (0 its middle digit), least digit first, negative, its digits' case
; not significant.

.bss
n_text:     .res        256                                 ; A number's text, as it's read (its _s left out); a base
n_tl:       .res        1                                   ; (Its length; the part being read: from, to)
n_tp:       .res        1
n_te:       .res        1
n_res:      .res        1                                   ; (num_parse's answer: NP_*)
n_sign:     .res        1                                   ; (A sign: $FF -)
n_dot:      .res        1                                   ; (A point read, the digits after it)
n_fdig:     .res        2
n_slash:    .res        1                                   ; (A rational's /)
n_radix:    .res        1                                   ; (The base: its size; negative ($80: not said yet);
n_bneg:     .res        1                                   ;   balanced; least first ($80; as read, $FF <, 1 >);
n_bbal:     .res        1                                   ;   case not significant; its 0's place among its digits;
n_ble:      .res        1                                   ;   its own digits' place in n_text ($FE: n_digs', a
n_bcase:    .res        1                                   ;   named base's; $FF: n_std's); a radix below 2 read)
n_zero:     .res        1
n_setp:     .res        1
n_digs:     .res        2
n_lowrad:   .res        1
n_send:     .res        1
n_np:       .res        2                                   ; (Digits written: pairs, digits; to the point)
n_nd:       .res        2
n_left:     .res        2
n_c:        .res        1                                   ; (Bytes, a digit)
n_c2:       .res        1
n_d:        .res        1

NP_NUM          = 0             ; num_parse's answers: a number (hv) ...
NP_NOT          = 1             ;   not a number (a symbol, as the reader has it) ...
NP_DIVZ         = 2             ;   a rational below 0 (1/0)

.segment "CODE3"

; ---- Written

; The number .A, .X out (print's and repr's alike): 42, -1.25, 2/3, 1+2i, -2i
num_print:
            sta         n_v
            stx         n_v + 1
            jsr         n_kind
            cmp         #NK_CPX
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
            MOVW        n_xi, n_v
            lda         n_x                                 ; (Its real part 0: its imaginary alone)
            ldx         n_x + 1
            jsr         n_zero_p
            beq         @im
            lda         n_x
            ldx         n_x + 1
            jsr         np_real
            MOVW        n_x, n_xi                           ; (+ before an imaginary part above 0)
            jsr         n_rsign
            cmp         #1
            bne         @im
            lda         #'+'
            jsr         out_byte
@im:
            lda         n_xi
            ldx         n_xi + 1
            jsr         np_real
            lda         #'i'
            jmp         out_byte

; The real number .A, .X out
np_real:
            sta         n_v
            stx         n_v + 1
            stz         n_pl
            stz         n_pl + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_FIX
            beq         @fix
            lda         n_v                                 ; (A rational: n/d)
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            phx
            pha
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            jsr         np_int
            lda         #'/'
            jsr         out_byte
            pla
            plx
            bra         np_int
@fix:
            lda         n_v                                 ; (A fixed decimal: its digits, a point before its places)
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            jsr         unfix
            sta         n_pl
            stx         n_pl + 1
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            bra         np_int
@int:
            lda         n_v
            ldx         n_v + 1
; The integer .A, .X out, in decimal, a point before its last n_pl digits ("0." and 0s first if there are no more)
np_int:
            ldy         #0
            sty         nrc
            jsr         r_load
            ldx         #0
            lda         r_neg,x
            beq         :+
            lda         #'-'
            jsr         out_byte
:
            jsr         np_pairs
            lda         n_pl
            ora         n_pl + 1
            beq         @all
            lda         n_nd                                ; (Digits before the point: n_left)
            sec
            sbc         n_pl
            sta         n_left
            lda         n_nd + 1
            sbc         n_pl + 1
            sta         n_left + 1
            bmi         @zeros
            ora         n_left
            bne         @digits
@zeros:
            lda         #'0'
            jsr         out_byte
            lda         #'.'
            jsr         out_byte
@zero:
            lda         n_left                              ; (A 0 for each n_left is below 0)
            ora         n_left + 1
            beq         @all
            lda         #'0'
            jsr         out_byte
            inc         n_left
            bne         @zero
            inc         n_left + 1
            bra         @zero
@all:
            lda         #$FF                                ; (No point to come)
            sta         n_left
            sta         n_left + 1
@digits:
            jsr         np_prev                             ; (The top pair: its tens' 0 left out)
            jsr         np_tens
            cmp         #'0'
            beq         :+
            jsr         np_digit
:
            lda         n_c
            jsr         np_digit
@pair:
            lda         n_np
            bne         :+
            dec         n_np + 1
:
            dec         n_np
            lda         n_np
            ora         n_np + 1
            beq         @rts
            jsr         np_prev
            jsr         np_tens
            jsr         np_digit
            lda         n_c
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

; IN: .A = a pair (0-99).  OUT: .A = its tens' digit, n_c its ones' (characters)
np_tens:
            ldx         #'0' - 1
            sec
:
            inx
            sbc         #10
            bcs         :-
            adc         #10 + '0'
            sta         n_c
            txa
            rts

; The digit .A out, and a point after it if it's the last before the point
np_digit:
            jsr         out_byte
            lda         n_left
            bne         :+
            dec         n_left + 1
:
            dec         n_left
            lda         n_left
            ora         n_left + 1
            bne         @rts
            lda         #'.'
            jmp         out_byte
@rts:
            rts

; R0's magnitude in decimal: its digits in pairs (each 0-99, least first) on the digits' pages, n_np of them, nd past
; the last; n_nd digits (the top pair's tens' 0 not counted).  Uses R1
np_pairs:
            lda         #0
            sta         nra
            lda         #1
            sta         nrc
            jsr         r_copy
            ldx         #1
            stz         r_neg,x
            lda         #NR_DIG
            jsr         r_page
            sta         nd + 1
            stz         nd
            stz         n_np
            stz         n_np + 1
@pair:
            lda         #100
            jsr         r_divsmall
            sta         (nd)
            inc         nd
            bne         :+
            inc         nd + 1
:
            inc         n_np
            bne         :+
            inc         n_np + 1
:
            ldx         #1
            lda         r_len,x
            bne         @pair
            lda         n_np                                ; (Two digits a pair, less the top one's tens' 0)
            asl
            sta         n_nd
            lda         n_np + 1
            rol
            sta         n_nd + 1
            jsr         np_prev
            inc         nd
            bne         :+
            inc         nd + 1
:
            cmp         #10
            bcs         @rts
            lda         n_nd
            bne         :+
            dec         n_nd + 1
:
            dec         n_nd
@rts:
            rts

; ---- Read

; The text at hq (hn bytes, 255 at most) read as a number, as danlang reads one: an integer, a fixed decimal or a
; rational in decimal, or any in a base (#xFF, #16rFF, #[01]101, #<x..., #=16r..., -#x10 ...); its _s left out,
; and the spaces at its ends.  OUT: n_res = NP_NUM and hv = the number; NP_NOT, it isn't one; NP_DIVZ, a rational of
; 0 below (1/0).  Too big: n_abt <> 0, ex its error (C = 1 if there's no room for it)
num_parse:
            jsr         n_enter
            ldy         #0                                  ; (Its text, the _s left out)
            ldx         #0
@copy:
            cpy         hn
            beq         @copied
            lda         (hq),y
            iny
            cmp         #'_'
            beq         @copy
            sta         n_text,x
            inx
            bra         @copy
@copied:
            stx         n_tl
            ldx         #0                                  ; (The spaces at its ends left out)
@lead:
            cpx         n_tl
            beq         :+
            lda         n_text,x
            jsr         np_space_p
            bne         :+
            inx
            bra         @lead
:
            stx         n_tp
            ldy         n_tl
@trail:
            cpy         n_tp
            beq         :+
            lda         n_text - 1,y
            jsr         np_space_p
            bne         :+
            dey
            bra         @trail
:
            sty         n_te
            lda         #NP_NOT
            sta         n_res
            cpx         n_te
            bcc         @lb213
            jmp         @done
@lb213:
            ldy         #0                                  ; (Its /s counted, and whether it has a point)
            stz         n_dot
@scan:
            lda         n_text,x
            cmp         #'.'
            bne         :+
            sta         n_dot
:
            cmp         #'/'
            bne         :+
            iny
            stx         n_slash
:
            inx
            cpx         n_te
            bcc         @scan
            tya
            beq         @one
            cmp         #1
            bne         @done
            lda         n_dot
            bne         @done
            lda         n_te                                ; (A rational: two integers; the numerator R4, the
            pha                                             ;   denominator R5)
            lda         n_slash
            sta         n_te
            jsr         np_one
            pla
            bcs         @done
            sta         n_te
            lda         #0
            sta         nra
            lda         #4
            sta         nrc
            jsr         r_copy
            ldx         n_slash
            inx
            stx         n_tp
            jsr         np_one
            bcs         @done
            lda         #0
            sta         nra
            lda         #5
            sta         nrc
            jsr         r_copy
            lda         #NP_DIVZ
            sta         n_res
            ldx         #5
            lda         r_len,x
            beq         @done
            lda         r_neg,x
            beq         :+
            jsr         r_negate
            ldx         #4
            jsr         r_negate
:
            jsr         n_rat
            bra         @num
@one:
            jsr         np_one
            bcs         @done
            jsr         np_make
@num:
            stz         n_res
@done:
            clc
            jmp         n_leave

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

; n_text from n_tp to n_te, one number (no /), read: R0 its value; n_dot <> 0, there was a point, n_fdig the digits
; after it, R1 the base to that power.  OUT: C = 1 if it isn't one.  Uses R2
np_one:
            stz         n_dot
            stz         n_fdig
            stz         n_fdig + 1
            stz         n_sign
            stz         n_lowrad
            ldx         n_tp
            lda         n_text,x                            ; (A sign)
            cmp         #'+'
            beq         @sign
            cmp         #'-'
            bne         @base
            dec         n_sign
@sign:
            inx
@base:
            cpx         n_te
            bcs         @not
            lda         n_text,x
            cmp         #'#'
            beq         @hash
            lda         #10                                 ; (Decimal: [0-9]+(\.[0-9]+)?)
            sta         n_radix
            stz         n_bneg
            stz         n_bbal
            stz         n_ble
            stz         n_zero
            lda         #1
            sta         n_bcase
            lda         #$FF
            sta         n_setp
            bra         @value
@hash:
            inx                                             ; (#, [<>]?)
            jsr         np_dir
            stx         n_c2                                ; (A base of its own: =?[+-]? then NNr or [...])
            stz         n_bbal
            lda         #$80
            sta         n_bneg
            lda         n_text,x
            cmp         #'='
            bne         :+
            dec         n_bbal
            inx
:
            jsr         np_mod
            jsr         np_radix
            bcc         @value
            ldx         n_c2                                ; (Else a named base: [+-]? and its letter)
            stz         n_bbal
            lda         #$80
            sta         n_bneg
            jsr         np_mod
            jsr         np_letter
            bcs         @not
@value:
            stx         n_tp
            jsr         np_digits
            bcs         @rts
            lda         n_sign
            beq         :+
            ldx         #0
            jsr         r_negate
:
            clc
@rts:
            rts
@not:
            sec
            rts

; [<>]? at .X: n_ble $FF (<) or 1 (>), else 0; .X past it
np_dir:
            stz         n_ble
            lda         n_text,x
            cmp         #'<'
            bne         :+
            lda         #$FF
            bra         @set
:
            cmp         #'>'
            bne         @rts
            lda         #1
@set:
            sta         n_ble
            inx
@rts:
            rts

; [+-]? at .X: n_bneg $FF (-) or 0 (+), else as it was; .X past it
np_mod:
            lda         n_text,x
            cmp         #'-'
            bne         :+
            lda         #$FF
            bra         @set
:
            cmp         #'+'
            bne         @rts
            lda         #0
@set:
            sta         n_bneg
            inx
@rts:
            rts

; A named base's letter at .X (any case): the base set (np_named), .X past it.  OUT: C = 1 if it isn't one
np_letter:
            cpx         n_te
            bcs         @rts
            lda         n_text,x
            ora         #$20
            ldy         #NAMED - 1
:
            cmp         nb_letter,y
            beq         :+
            dey
            bpl         :-
            sec
@rts:
            rts
:
            inx
; The named base .Y (nb_letter's): its size, digits; balanced, least first and negative (unless said); case
np_named:
            lda         nb_size,y
            sta         n_radix
            lda         nb_digs_lo,y
            sta         n_digs
            lda         nb_digs_hi,y
            sta         n_digs + 1
            lda         #$FE
            sta         n_setp
            lda         nb_flags,y
            sta         n_c
            and         #NB_CASE
            sta         n_bcase
            lda         n_c
            and         #NB_BAL
            beq         :+
            lda         #$FF
:
            sta         n_bbal
            lda         n_ble                               ; (Least first: < or > said, else the base's)
            bne         :+
            lda         n_c
            and         #NB_LE
            beq         :+
            lda         #$FF
:
            and         #$80
            sta         n_ble
            lda         n_bneg                              ; (Negative: - or + said, else the base's)
            cmp         #$80
            bne         :+
            lda         n_c
            and         #NB_NEG
            beq         :+
            lda         #$FF
:
            sta         n_bneg
            jsr         np_zero
            clc
            rts

; A base of its own at .X: a radix (2-80: [1-7]?[0-9] or 80, then r or R) of n_std's digits; or digits of its own,
; [...] (2-80, each legal and once).  OUT: C = 0 and .X past it, the base set; C = 1 if there's none (n_lowrad <> 0:
; a radix below 2)
np_radix:
            lda         n_text,x
            cmp         #'['
            beq         @own
            jsr         np_dec
            bcc         @lb212
            jmp         @no
@lb212:
            sta         n_radix
            inx
            jsr         np_dec
            bcs         @r
            ldy         n_radix                             ; (Two digits: 1-7 and any, or 80)
            bne         @lb211
            jmp         @no
@lb211:
            cpy         #8
            bcc         @two
            beq         @lb210
            jmp         @no
@lb210:
            cmp         #0
            beq         @lb209
            jmp         @no
@lb209:
@two:
            sta         n_d
            lda         n_radix
            asl
            asl
            adc         n_radix
            asl
            adc         n_d
            sta         n_radix
            inx
@r:
            lda         n_text,x
            ora         #$20
            cmp         #'r'
            bne         @no
            inx
            lda         n_radix
            cmp         #2
            bcs         :+
            sta         n_lowrad
            inc         n_lowrad
            bra         @no
:
            lda         #$FF                                ; (n_std's first: their case matters past 36)
            sta         n_setp
            stz         n_bcase
            lda         n_radix
            cmp         #37
            bcs         @made
            inc         n_bcase
            bra         @made
@own:
            inx                                             ; (Its own digits: to the ])
            stx         n_setp
@sdig:
            cpx         n_te
            bcs         @no
            lda         n_text,x
            cmp         #']'
            beq         @send
            jsr         np_legal
            bcs         @no
            inx
            bra         @sdig
@send:
            stx         n_send
            inx
            lda         n_send
            sec
            sbc         n_setp
            sta         n_radix
            cmp         #2
            bcc         @no
            cmp         #81
            bcs         @no
            phx
            jsr         np_unique
            plx
            bcs         @no
@made:
            lda         n_bneg                              ; (Not said: positive)
            cmp         #$80
            bne         :+
            stz         n_bneg
:
            lda         n_ble                               ; (Least first only if <)
            and         #$80
            sta         n_ble
            jsr         np_zero
            clc
            rts
@no:
            sec
            rts

; n_zero: the 0's place among the base's digits (its middle one, balanced; else the first)
np_zero:
            stz         n_zero
            lda         n_bbal
            beq         @rts
            lda         n_radix
            lsr
            sta         n_zero
@rts:
            rts

; n_text's byte .X a decimal digit?  OUT: C = 0 and .A = its value if it is
np_dec:
            lda         n_text,x
            sec
            sbc         #'0'
            cmp         #10
            rts

; Is .A a legal digit of a base of its own (0-9, a-z, A-Z, `~!@#$%^&*-=+|;:,?)?  OUT: C = 0 if it is.  Keeps .X
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
            ora         #$20
            cmp         #'a'
            bcc         @no
            cmp         #'z' + 1
            bcc         @yes
@no:
            sec
            rts
@yes:
            clc
            rts

; The base's own digits (n_setp to n_send): each once (C = 1 if not); n_bcase <> 0 if no two differ only in case
np_unique:
            lda         #1
            sta         n_bcase
            ldx         n_setp
@one:
            cpx         n_send
            bcs         @yes
            txa
            tay
@other:
            iny
            cpy         n_send
            bcs         @next
            lda         n_text,x
            cmp         n_text,y
            beq         @no
            jsr         np_upper
            sta         n_c
            lda         n_text,y
            jsr         np_upper
            cmp         n_c
            bne         @other
            stz         n_bcase
            bra         @other
@next:
            inx
            bra         @one
@yes:
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

; The digits from n_tp to n_te, [d]+(\.[d]+)?, in the base set (least first: in n_te to n_tp's order): R0 = their
; value; n_dot <> 0, a point, n_fdig the digits after it as they're taken, R1 the base to that power.  OUT: C = 1 if
; they aren't.  Uses R2
np_digits:
            ldx         n_tp                                ; (A digit first and last)
            cpx         n_te
            bcc         @lb208
            jmp         @no
@lb208:
            lda         n_text,x
            cmp         #'.'
            bne         @lb207
            jmp         @no
@lb207:
            ldx         n_te
            lda         n_text - 1,x
            cmp         #'.'
            bne         @lb206
            jmp         @no
@lb206:
            lda         #0                                  ; (R0 = 0, R1 = 1)
            sta         nrc
            tax
            jsr         r_set16
            lda         #1
            sta         nrc
            ldx         #0
            jsr         r_set16
            ldx         n_tp
            lda         n_ble
            beq         @take
            ldx         n_te
            dex
@take:
            stx         n_c
            lda         n_text,x
            cmp         #'.'
            bne         @digit
            lda         n_dot
            bne         @no
            dec         n_dot
            bra         @next
@digit:
            jsr         np_value
            bcs         @no
            sta         n_d                                 ; (R0 = R0 * base + it)
            lda         #0
            sta         nrc
            jsr         np_times
            lda         n_d
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
            lda         n_dot                               ; (After the point: R1 times the base too)
            beq         @next
            lda         #1
            sta         nrc
            jsr         np_times
            inc         n_fdig
            bne         @next
            inc         n_fdig + 1
@next:
            ldx         n_c
            lda         n_ble
            bne         @down
            inx
            cpx         n_te
            bcc         @take
            clc
            rts
@down:
            cpx         n_tp
            beq         @done
            dex
            bra         @take
@done:
            clc
            rts
@no:
            sec
            rts

; nrc = nrc * the base (negative if n_bneg)
np_times:
            lda         n_radix
            jsr         r_mulsmall
            lda         n_bneg
            beq         @rts
            ldx         nrc
            jsr         r_negate
@rts:
            rts

; IN: .A = a byte.  OUT: C = 0 and .A = its value as a digit of the base (less its 0's place: signed); C = 1 if it
; isn't one
np_value:
            ldx         n_bcase
            beq         :+
            jsr         np_upper
:
            sta         n_c2
            ldy         #0
@digit:
            cpy         n_radix
            bcs         @no
            jsr         np_dig_y
            ldx         n_bcase
            beq         :+
            jsr         np_upper
:
            cmp         n_c2
            beq         @yes
            iny
            bra         @digit
@yes:
            tya
            sec
            sbc         n_zero
            clc
            rts
@no:
            sec
            rts

; .A = the base's digit .Y.  Keeps .Y
np_dig_y:
            lda         n_setp
            cmp         #$FE
            beq         @named
            bcs         @std
            phy
            tya
            clc
            adc         n_setp
            tay
            lda         n_text,y
            ply
            rts
@named:
            lda         n_digs
            sta         nd
            lda         n_digs + 1
            sta         nd + 1
            lda         (nd),y
            rts
@std:
            lda         n_std,y
            rts

; hv = the number np_one read: an integer (no point); a fixed decimal (a point, in base 10); else a rational
np_make:
            lda         n_dot
            beq         @int
            lda         n_radix
            cmp         #10
            bne         @rat
            lda         n_bneg
            bne         @rat
            MOVW        n_pl, n_fdig                        ; (Base 10: the digits, their places)
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

; ---- In a base

; The number n_v out in the base at hq (hn bytes: "x", "#x", "#16r", with < > + -), as to-str has it: "#xFF"; a
; rational's parts each so ("#x1/#x3"); a fixed decimal as a rational.  OUT: C = 0; or C = 1 and .A, .X = the error's
; message (s_cpxbase; or s_badbase, s_lowbase: the base, n_text's n_tl bytes, after it)
n_tobase:
            lda         n_v
            ldx         n_v + 1
            jsr         n_kind
            cmp         #NK_CPX
            bne         :+
            LDAX        s_cpxbase
            sec
            rts
:
            ldy         #0                                  ; (The base, # first if it hasn't one)
            ldx         #0
            lda         (hq)
            cmp         #'#'
            beq         @copy
            lda         #'#'
            sta         n_text
            inx
@copy:
            cpy         hn
            beq         :+
            lda         (hq),y
            sta         n_text,x
            iny
            inx
            bne         @copy
:
            stx         n_te
            stx         n_tl
            stz         n_lowrad
            stz         n_bbal
            lda         #$80
            sta         n_bneg
            ldx         #1                                  ; (#[<>]?[+-]?, then a radix or a letter: all of it)
            jsr         np_dir
            jsr         np_mod
            stx         n_c2
            jsr         np_radix
            bcc         @radix
            ldx         n_c2
            jsr         np_letter
            bcc         @end
@bad:
            LDAX        s_badbase
            ldy         n_lowrad
            beq         :+
            LDAX        s_lowbase
:
            sec
            rts
@radix:
            lda         n_setp                              ; (Not digits of its own)
            cmp         #$FF
            bne         @bad
@end:
            cpx         n_te
            bne         @bad
            lda         n_te                                ; ("#d": no prefix)
            cmp         #2
            bne         :+
            lda         n_text + 1
            cmp         #'d'
            bne         :+
            stz         n_te
:
            lda         n_v                                 ; (An integer, a rational, a fixed decimal)
            ldx         n_v + 1
            jsr         n_kind
            beq         @int
            cmp         #NK_RAT
            beq         @ratio
            lda         n_v                                 ; (A fixed decimal: as a rational, or as an integer)
            ldx         n_v + 1
            ldy         #4
            jsr         n_load
            jsr         n_rat
            MOVW        n_v, hv
            lda         n_v
            ldx         n_v + 1
            jsr         n_kind
            beq         @int
@ratio:
            lda         n_v
            ldx         n_v + 1
            ldy         #1
            jsr         cell_get
            phx
            pha
            lda         n_v
            ldx         n_v + 1
            ldy         #0
            jsr         cell_get
            jsr         nb_int
            lda         #'/'
            jsr         out_byte
            pla
            plx
            jsr         nb_int
            clc
            rts
@int:
            lda         n_v
            ldx         n_v + 1
            jsr         nb_int
            clc
            rts

; The integer .A, .X out in the base set: - first (if it's negative, in a base that's positive and not balanced), the
; prefix (n_text's first n_te bytes), its digits (most first, or least first).  Each digit d is n mod the base's
; size, in the base's range; then n is (n - d) / base, till it's 0.  Too many digits for the digits' pages: n_big
nb_int:
            ldy         #0
            sty         nrc
            jsr         r_load
            ldx         #0
            lda         r_neg,x
            beq         @prefix
            lda         n_bneg
            ora         n_bbal
            bne         @prefix
            jsr         r_negate
            lda         #'-'
            jsr         out_byte
@prefix:
            ldx         #0
:
            cpx         n_te
            beq         :+
            lda         n_text,x
            jsr         out_byte
            inx
            bra         :-
:
            lda         #NR_DIG
            jsr         r_page
            sta         nd + 1
            stz         nd
@digit:
            lda         #0                                  ; (|n| mod the size: d, its sign n's)
            sta         nra
            lda         #1
            sta         nrc
            jsr         r_copy
            lda         n_radix
            jsr         r_divsmall
            sta         n_d
            ldx         #0
            lda         r_neg,x
            beq         :+
            sec
            lda         #0
            sbc         n_d
            sta         n_d
:
            ldx         n_bbal                              ; (Into the base's range: 0 to size - 1; balanced, -zero
            beq         @nonneg                             ;   to zero)
            lda         n_d
            bmi         @low
            cmp         n_zero
            beq         @ranged
            bcc         @ranged
            sec
            sbc         n_radix
            bra         @set
@low:
            clc
            adc         n_zero
            bpl         @ranged
            lda         n_d
            clc
            adc         n_radix
            bra         @set
@nonneg:
            lda         n_d
            bpl         @ranged
            clc
            adc         n_radix
@set:
            sta         n_d
@ranged:
            lda         n_d                                 ; (Its character, kept)
            clc
            adc         n_zero
            tay
            jsr         np_dig_y
            ldx         n_d                                 ; (nd: np_dig_y may have used it for a named base's)
            sta         n_c
            lda         #NR_DIG
            jsr         r_page
            clc
            adc         n_np + 1
            sta         nd + 1
            lda         n_np
            sta         nd
            lda         n_c
            sta         (nd)
            inc         n_np
            bne         :+
            inc         n_np + 1
            lda         n_np + 1
            cmp         #NR_DIGS
            bcc         :+
            jmp         n_big
:
            lda         n_d                                 ; (n = (n - d) / base)
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
            jsr         r_sub
            lda         n_radix
            jsr         r_divsmall
            lda         n_bneg
            beq         :+
            ldx         #0
            jsr         r_negate
:
            ldx         #0
            lda         r_len,x
            beq         :+
            jmp         @digit
:
            lda         #NR_DIG                             ; (nd: the digits' start; n_np, how many)
            jsr         r_page
            sta         nd + 1
            stz         nd
            lda         n_ble
            bne         @le
            lda         nd + 1                              ; (Most first: from the last back)
            clc
            adc         n_np + 1
            sta         nd + 1
            lda         n_np
            sta         nd
@be:
            jsr         np_prev
            jsr         out_byte
            lda         n_np
            bne         :+
            dec         n_np + 1
:
            dec         n_np
            lda         n_np
            ora         n_np + 1
            bne         @be
            rts
@le:
            lda         (nd)                                ; (Least first: as they were made)
            jsr         out_byte
            inc         nd
            bne         :+
            inc         nd + 1
:
            lda         n_np
            bne         :+
            dec         n_np + 1
:
            dec         n_np
            lda         n_np
            ora         n_np + 1
            bne         @le
            rts

.segment "RODATA3"
; The named bases: each one's letter, size, digits and flags (balanced, least first, negative, case not significant)
NB_BAL          = $01
NB_LE           = $02
NB_NEG          = $04
NB_CASE         = $08
NAMED           = 19
nb_letter:  .byte       "cegijmbtqvfsondxzky"
nb_size:    .byte       3, 5, 7, 3, 5, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 36, 27, 53
nb_flags:   .byte       NB_BAL | NB_LE | NB_CASE, NB_BAL | NB_LE | NB_CASE, NB_BAL | NB_LE | NB_CASE
            .byte       NB_BAL | NB_LE | NB_CASE, NB_BAL | NB_LE | NB_CASE, NB_BAL | NB_LE | NB_NEG
            .byte       NB_CASE, NB_CASE, NB_CASE, NB_CASE, NB_CASE, NB_CASE, NB_CASE, NB_CASE, NB_CASE, NB_CASE
            .byte       NB_CASE, NB_BAL | NB_CASE, NB_BAL
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
; The digits a base of its own may have, past 0-9, a-z and A-Z
n_legal:    .byte       "`~!@#$%^&*-=+|;:,?"
NLEGAL          = * - n_legal
s_cpxbase:  .byte       "A complex number has no form in another base", 0
s_badbase:  .byte       "Invalid base specifier ", 0
s_lowbase:  .byte       ": a radix is 2 to 80", 0

.segment "CODE3"
