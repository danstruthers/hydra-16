; ****************************************************************************
; numbers.inc - hylang's numbers (step 7.3), in the module's third bank: danlang's tower on bignum.inc's integers.
; A number is an integer (a fixnum, or OT_BIG), a fixed decimal (OT_FIX: its digits, an integer, and its places,
; kept with no zeros at their end), a rational (OT_RAT: a numerator and a denominator, in their lowest terms, the
; denominator more than 1), or a complex number (OT_COMPLEX: a real part and an imaginary part, not 0).
;   num_op is the arithmetic, danlang's (Num's operators): a result is complex if either number is; else, for a
; quotient, rational (or an integer, if it's whole); else rational if either is; else fixed if either is; else an
; integer.  num_cmp compares by value, whatever the kinds (complex numbers by real part, then imaginary part).
; num_print shows one; num_parse reads one (decimal here; bases.inc has the rest).
;   The values worked on are held on the heap's roots (root_push) while more are made: nr_peek reads one by its
; place from the top.

; num_op's operations
NO_ADD          = 0
NO_SUB          = 1
NO_MUL          = 2
NO_DIV          = 3

; Kinds (num_kind)
NK_INT          = 0
NK_FIX          = 1
NK_RAT          = 2
NK_COMPLEX      = 3

DG_SIZE         = 320           ; An integer's digits, at most (NB_MAX bytes: some 304)

.bss
nx:         .res        2                                   ; num_op's and num_cmp's first operand (a value: pushed
                                                            ; before anything's made)
n_nop:      .res        1                                   ; num_op's operation
n_kx:       .res        1                                   ; The two kinds
n_ky:       .res        1
n_scale:    .res        1                                   ; A fixed decimal's places
n_obj:      .res        2                                   ; An object read (its fields pushed: nothing's made)
n_plen:     .res        1                                   ; num_parse's text: its length, and its / (a rational's)
n_slash:    .res        1
p_end:      .res        1                                   ; The end of a number's text (parse_real's, parse_base's)
p_sign:     .res        1                                   ;   and $80 if a - is before it
n_places:   .res        1                                   ; to-fixed's places, at most
n_sgn:      .res        1                                   ; A sign kept ($FF negative), or which (rational.n/d)
rnd_seed:   .res        4                                   ; random's generator (xorshift32: 0, not seeded yet) ...
rnd_t:      .res        4                                   ;   and its shifted copy
dg_n:       .res        2                                   ; Digits made: how many ...
dg_neg:     .res        1                                   ;   $80: the number's negative ...
dg:         .res        DG_SIZE                             ;   and they, the last first
.segment "NUMCODE"

; ---- The roots, a stack

; h_v pushed.  Keeps .X, .Y
nr_push:
            phx
            jsr         root_push
            plx
            rts

; h_v = the root .X from the top (0: the top).  Keeps .Y
nr_peek:
            txa
            eor         #$FF
            sec
            adc         root_n
            sbc         #1
            tax
            lda         roots_lo,x
            sta         h_v
            lda         roots_hi,x
            sta         h_v + 1
            rts

; Root .X from the top = h_v
nr_poke:
            txa
            eor         #$FF
            sec
            adc         root_n
            sbc         #1
            tax
            lda         h_v
            sta         roots_lo,x
            lda         h_v + 1
            sta         roots_hi,x
            rts

; .A roots dropped.  Keeps C, .X, .Y
nr_drop:
            php
            eor         #$FF
            sec
            adc         root_n
            sta         root_n
            plp
            rts

; h_v = root .X (op .A, IO_*) root .Y (each from the top), integers.  OUT: C = 1, an error
iop2:
            pha
            phy
            jsr         nr_peek
            MOVR        ia, h_v
            plx
            jsr         nr_peek
            pla
            jmp         int_op

; ---- Kinds and parts

; h_v's kind: .A = NK_*.  OUT: C = 1, it isn't a number
num_kind:
            lda         h_v
            lsr
            bcs         @int
            ldx         h_v + 1
            lda         page_kind,x
            cmp         #PK_C8
            bcc         @no
            jsr         deref
            lda         (h_p)
            sec
            sbc         #OT_BIG
            cmp         #4
            bcs         @no
            tax
            lda         nk_of,x
            clc
            rts
@int:
            lda         #NK_INT
            clc
            rts
@no:
            sec
            rts

; h_v = its field at .Y (2 or 4: an object's two values)
num_field:
            jsr         deref
            lda         (h_p),y
            tax
            iny
            lda         (h_p),y
            stx         h_v
            sta         h_v + 1
            rts

; h_v's two fields pushed: its first, then its second
num_fields:
            MOVR        n_obj, h_v
            ldy         #2
            jsr         num_field
            jsr         nr_push
            MOVR        h_v, n_obj
            ldy         #4
            jsr         num_field
            jmp         nr_push

; The fixed decimal (or integer) h_v's digits and places pushed
fix_parts:
            jsr         num_kind
            cmp         #NK_FIX
            beq         num_fields
            jsr         nr_push                             ; (An integer: itself, and 0 places)
            LDR         h_v, 1
            jmp         nr_push

; The rational (a fixed decimal, an integer) h_v's numerator and denominator pushed
rat_parts:
            jsr         num_kind
            cmp         #NK_RAT
            beq         num_fields
            cmp         #NK_FIX
            beq         @fix
            jsr         nr_push                             ; (An integer: itself, over 1)
            LDR         h_v, 3
            jmp         nr_push
@fix:
            MOVR        n_obj, h_v                          ; (A fixed decimal: its digits over 10^its places)
            ldy         #2
            jsr         num_field
            jsr         nr_push
            MOVR        h_v, n_obj
            ldy         #4
            jsr         num_field
            lda         h_v
            lsr
            jsr         pow10
            jmp         nr_push

; The complex number (a real one) h_v's real and imaginary parts pushed
cpx_parts:
            jsr         num_kind
            cmp         #NK_COMPLEX
            jeq         num_fields
            jsr         nr_push                             ; (A real one: itself, and 0)
            LDR         h_v, 1
            jmp         nr_push

; h_v = 10 to the .A (0-255).  OUT: C = 1, too big (h_v an error)
pow10:
            pha
            jsr         nq_init
            lda         #1                                  ; (C = 1)
            sta         nc
            ldy         #2
            sta         nc,y
            stz         nc + 1
            lda         #10
            sta         n_s
            stz         n_t
            plx
:
            cpx         #0
            beq         :+
            phx
            jsr         mag_muls
            plx
            lda         nc
            cmp         #NB_MAX + 1
            bcs         :+
            dex
            bra         :-
:
            jmp         nb_store

; C = 1 if h_v is 0 (0, or a fixed decimal's 0)
num_zero:
            lda         h_v + 1
            bne         :+
            lda         h_v
            cmp         #1
            bne         :+
            sec
            rts
:
            jsr         num_kind
            cmp         #NK_FIX
            bne         @no
            ldy         #2
            jsr         num_field
            lda         h_v + 1
            bne         @no
            lda         h_v
            cmp         #1
            bne         @no
            sec
            rts
@no:
            clc
            rts

; ---- Making them

; h_v = the fixed decimal of the digits h_v and n_scale places, those places' zeros at the end left off.  OUT: C = 1,
; an error
make_fix:
            jsr         nq_init
            NQ          nq_c, nc
            jsr         nb_load
            lda         nc + 1                              ; (Its sign, kept: a quotient of 0 loses it)
            sta         n_sgn
            lda         #10
            sta         n_s
@zero:
            lda         n_scale                             ; (A place's 0 off, while there is one; 0 has none)
            beq         @made
            lda         nc
            bne         :+
            stz         n_scale
            bra         @made
:
            jsr         mag_divs
            beq         :+
            sta         n_t                                 ; (Not 0: as it was)
            jsr         mag_muls
            bra         @made
:
            dec         n_scale
            bra         @zero
@made:
            lda         n_sgn
            sta         nc + 1
            jsr         nb_store
            bcs         @done
            MOVR        h_a, h_v
            lda         n_scale
            asl
            ora         #1
            sta         h_d
            stz         h_d + 1
            lda         #OT_FIX
            ldx         #0
            jsr         make_cell2
            bcc         @done
            MOVR        h_v, err_room
@done:
            rts

; The rational of the numerator and denominator on the roots (the numerator under), in its lowest terms (an
; integer, if its denominator's 1): h_v; the two dropped.  OUT: C = 1, an error (division by zero)
rat_norm:
            ldx         #0                                  ; (Its denominator 0: division by zero)
            jsr         nr_peek
            jsr         num_zero
            bcc         :+
            LDR         r0, nm_div_zero
            jsr         io_error
            lda         #2
            jmp         nr_drop
:
            lda         #IO_GCD                             ; (g, their gcd)
            ldx         #1
            ldy         #0
            jsr         iop2
            bcs         @fail2
            jsr         nr_push                             ; [n d g]
            lda         #IO_QUO
            ldx         #2
            ldy         #0
            jsr         iop2
            bcs         @fail3
            jsr         nr_push                             ; [n d g n/g]
            lda         #IO_QUO
            ldx         #2
            ldy         #1
            jsr         iop2
            bcs         @fail4
            jsr         nr_push                             ; [n d g n/g d/g]
            jsr         int_sign                            ; (A negative denominator: both negated)
            cmp         #$FF
            bne         @signed
            jsr         neg_top
            bcs         @fail5
            ldx         #1
            jsr         nr_peek
            jsr         neg_int
            bcs         @fail5
            ldx         #1
            jsr         nr_poke
@signed:
            ldx         #0                                  ; (Over 1: an integer)
            jsr         nr_peek
            lda         h_v + 1
            bne         @rat
            lda         h_v
            cmp         #3
            bne         @rat
            ldx         #1
            jsr         nr_peek
            lda         #5
            clc
            jmp         nr_drop
@rat:
            jsr         make_rat
            lda         #5
            jmp         nr_drop
@fail5:
            lda         #5
            jmp         nr_drop
@fail4:
            lda         #4
            jmp         nr_drop
@fail3:
            lda         #3
            jmp         nr_drop
@fail2:
            lda         #2
            jmp         nr_drop

; h_v = the rational of the top two roots (the numerator under), as they are.  OUT: C = 1, no room
make_rat:
            ldx         #1
            jsr         nr_peek
            MOVR        h_a, h_v
            ldx         #0
            jsr         nr_peek
            MOVR        h_d, h_v
            lda         #OT_RAT
            ldx         #0
            jsr         make_cell2
            bcc         :+
            MOVR        h_v, err_room
:
            rts

; The top root negated (an integer).  OUT: C = 1, an error (h_v)
neg_top:
            ldx         #0
            jsr         nr_peek
            jsr         neg_int
            bcs         :+
            jsr         root_set
            clc
:
            rts

; h_v = -h_v (an integer).  OUT: C = 1, an error
neg_int:
            LDR         ia, 1
            lda         #IO_SUB
            jmp         int_op

; h_v = the complex number of the top two roots (the real part under); the real part alone, if the other's 0
cpx_norm:
            ldx         #0
            jsr         nr_peek
            jsr         num_zero
            bcc         :+
            ldx         #1
            jsr         nr_peek
            clc
            rts
:
            ldx         #1
            jsr         nr_peek
            MOVR        h_a, h_v
            ldx         #0
            jsr         nr_peek
            MOVR        h_d, h_v
            lda         #OT_COMPLEX
            ldx         #0
            jsr         make_cell2
            bcc         :+
            MOVR        h_v, err_room
:
            rts

; ---- The arithmetic

; h_v = nx (op .A: NO_*) h_v, numbers.  OUT: C = 1, h_v an error (not numbers; too big; division by zero)
num_op:
            sta         n_nop
            jsr         num_kind
            bcs         @not
            sta         n_ky
            jsr         nr_push                             ; [y]
            MOVR        h_v, nx
            jsr         num_kind
            bcs         @not1
            sta         n_kx
            jsr         nr_push                             ; [y x]
            lda         n_kx                                ; (The kind: the higher)
            cmp         n_ky
            bcs         :+
            lda         n_ky
:
            cmp         #NK_COMPLEX
            bne         :+
            jsr         cpx_op
            bra         @done
:
            ldx         n_nop
            cpx         #NO_DIV
            beq         @rat
            cmp         #NK_RAT
            beq         @rat
            cmp         #NK_FIX
            beq         @fix
            ldx         n_nop                               ; ---- Integers
            lda         io_of_no,x
            ldx         #0
            ldy         #1
            jsr         iop2
            bra         @done
@rat:
            jsr         rat_op
            bra         @done
@fix:
            jsr         fix_op
@done:
            lda         #2
            jmp         nr_drop
@not1:
            lda         #1
            jsr         nr_drop
@not:
            LDR         r0, nm_numbers
            jmp         io_error

; The rationals x and y (roots 0 and 1): h_v = x (n_nop) y
rat_op:
            ldx         #0
            jsr         nr_peek
            jsr         rat_parts                           ; [y x nx dx]
            ldx         #3
            jsr         nr_peek
            jsr         rat_parts                           ; [y x nx dx ny dy]
            lda         n_nop
            cmp         #NO_MUL
            beq         @mul
            cmp         #NO_DIV
            beq         @div
            lda         #IO_MUL                             ; ---- (nx dy +- ny dx) / (dx dy)
            ldx         #3
            ldy         #0
            jsr         iop2
            jcs         @fail4
            jsr         nr_push                             ; [.. nx dx ny dy a]
            lda         #IO_MUL
            ldx         #2
            ldy         #3
            jsr         iop2
            bcs         @fail5
            jsr         nr_push                             ; [.. nx dx ny dy a b]
            ldx         n_nop
            lda         io_of_no,x
            ldx         #1
            ldy         #0
            jsr         iop2
            bcs         @fail6
            jsr         nr_push                             ; [.. nx dx ny dy a b n]
            lda         #IO_MUL
            ldx         #5
            ldy         #3
            jsr         iop2
            bcs         @fail7
            jsr         nr_push                             ; [.. nx dx ny dy a b n d]
            jsr         rat_norm
            lda         #6
            jmp         nr_drop
@mul:
            lda         #IO_MUL                             ; ---- (nx ny) / (dx dy)
            ldx         #3
            ldy         #1
            jsr         iop2
            bcs         @fail4
            jsr         nr_push
            lda         #IO_MUL
            ldx         #3
            ldy         #1
            jsr         iop2
            bcs         @fail5
            jsr         nr_push
            jsr         rat_norm
            lda         #4
            jmp         nr_drop
@div:
            lda         #IO_MUL                             ; ---- (nx dy) / (dx ny)
            ldx         #3
            ldy         #0
            jsr         iop2
            bcs         @fail4
            jsr         nr_push
            lda         #IO_MUL
            ldx         #3
            ldy         #2
            jsr         iop2
            bcs         @fail5
            jsr         nr_push
            jsr         rat_norm
            lda         #4
            jmp         nr_drop
@fail7:
            lda         #7
            jmp         nr_drop
@fail6:
            lda         #6
            jmp         nr_drop
@fail5:
            lda         #5
            jmp         nr_drop
@fail4:
            lda         #4
            jmp         nr_drop

; The fixed decimals (or integers) x and y (roots 0 and 1): h_v = x (n_nop: not NO_DIV) y
fix_op:
            ldx         #0
            jsr         nr_peek
            jsr         fix_parts                           ; [y x mx sx]
            ldx         #3
            jsr         nr_peek
            jsr         fix_parts                           ; [y x mx sx my sy]
            lda         n_nop
            cmp         #NO_MUL
            bne         @add
            ldx         #2                                  ; ---- (mx my), sx + sy places
            jsr         nr_peek
            lda         h_v
            lsr
            sta         n_scale
            ldx         #0
            jsr         nr_peek
            lda         h_v
            lsr
            clc
            adc         n_scale
            sta         n_scale
            lda         #IO_MUL
            ldx         #3
            ldy         #1
            jsr         iop2
            bcs         @fail
            jsr         make_fix
@fail:
            lda         #4
            jmp         nr_drop
@add:
            ldx         #2                                  ; ---- The more places: each to them, then +-
            jsr         nr_peek
            lda         h_v
            lsr
            sta         n_scale
            ldx         #0
            jsr         nr_peek
            lda         h_v
            lsr
            cmp         n_scale
            bcc         :+
            sta         n_scale
:
            ldx         #3                                  ; (mx, its places made n_scale)
            ldy         #2
            jsr         fix_widen
            bcs         @fail
            jsr         nr_push                             ; [.. mx sx my sy a]
            ldx         #2
            ldy         #1
            jsr         fix_widen
            bcs         @fail5
            jsr         nr_push                             ; [.. a b]
            ldx         n_nop
            lda         io_of_no,x
            ldx         #1
            ldy         #0
            jsr         iop2
            bcs         @fail6
            jsr         make_fix
            lda         #6
            jmp         nr_drop
@fail5:
            lda         #5
            jmp         nr_drop
@fail6:
            lda         #6
            jmp         nr_drop

; h_v = the digits at root .X, their places (a fixnum) at root .Y made n_scale: times 10^(n_scale - its places)
fix_widen:
            phx
            tya
            tax
            jsr         nr_peek
            lda         h_v
            lsr
            eor         #$FF
            sec
            adc         n_scale
            jsr         pow10
            plx
            bcs         @done
            jsr         nr_push
            inx
            lda         #IO_MUL
            ldy         #0
            jsr         iop2
            php
            pha
            lda         #1
            jsr         nr_drop
            pla
            plp
@done:
            rts

; The complex numbers (or real ones) x and y (roots 0 and 1): h_v = x (n_nop) y
cpx_op:
            ldx         #0
            jsr         nr_peek
            jsr         cpx_parts                           ; [y x rx ix]
            ldx         #3
            jsr         nr_peek
            jsr         cpx_parts                           ; [y x rx ix ry iy]
            lda         n_nop
            cmp         #NO_MUL
            beq         @mul
            cmp         #NO_DIV
            jeq         @div
            ldx         #3                                  ; ---- (rx +- ry) + (ix +- iy)i
            ldy         #1
            jsr         cpx_part_op
            jcs         @fail4
            jsr         nr_push                             ; [.. rx ix ry iy re]
            ldx         #3
            ldy         #1
            jsr         cpx_part_op
            bcs         @fail5
            jsr         nr_push                             ; [.. re im]
            jsr         cpx_norm
            lda         #6
            jmp         nr_drop
@mul:
            lda         #NO_MUL                             ; ---- (rx ry - ix iy) + (rx iy + ix ry)i
            ldx         #3
            ldy         #1
            jsr         cpx_part_with
            bcs         @fail4
            jsr         nr_push                             ; [.. rx ix ry iy rxry]
            lda         #NO_MUL
            ldx         #3
            ldy         #1
            jsr         cpx_part_with
            bcs         @fail5
            jsr         nr_push                             ; [.. rxry ixiy]
            lda         #NO_SUB
            ldx         #1
            ldy         #0
            jsr         cpx_part_with
            bcs         @fail6
            jsr         nr_push                             ; [.. rxry ixiy re]
            lda         #NO_MUL
            ldx         #6
            ldy         #3
            jsr         cpx_part_with
            bcs         @fail7
            jsr         nr_push                             ; [.. re rxiy]
            lda         #NO_MUL
            ldx         #6
            ldy         #5
            jsr         cpx_part_with
            bcs         @fail8
            jsr         nr_push                             ; [.. re rxiy ixry]
            lda         #NO_ADD
            ldx         #1
            ldy         #0
            jsr         cpx_part_with
            bcs         @fail9
            jsr         nr_push                             ; [.. re rxiy ixry im]
            ldx         #3                                  ; (re, im on top)
            jsr         nr_peek
            jsr         nr_push
            ldx         #1
            jsr         nr_peek
            jsr         nr_push
            jsr         cpx_norm
            lda         #12
            jmp         nr_drop
@fail4:
            lda         #4
            jmp         nr_drop
@fail5:
            lda         #5
            jmp         nr_drop
@fail6:
            lda         #6
            jmp         nr_drop
@fail7:
            lda         #7
            jmp         nr_drop
@fail8:
            lda         #8
            jmp         nr_drop
@fail9:
            lda         #9
            jmp         nr_drop
@div:
            lda         #NO_MUL                             ; ---- d = ry ry + iy iy
            ldx         #1
            ldy         #1
            jsr         cpx_part_with
            bcs         @fail4
            jsr         nr_push                             ; [.. rx ix ry iy ryry]
            lda         #NO_MUL
            ldx         #1
            ldy         #1
            jsr         cpx_part_with
            bcs         @fail5
            jsr         nr_push                             ; [.. ryry iyiy]
            lda         #NO_ADD
            ldx         #1
            ldy         #0
            jsr         cpx_part_with
            bcs         @fail6
            jsr         nr_push                             ; [.. rx ix ry iy ryry iyiy d]
            lda         #NO_MUL                             ; (re = (rx ry + ix iy) / d)
            ldx         #6
            ldy         #4
            jsr         cpx_part_with
            bcs         @fail7
            jsr         nr_push                             ; [.. d rxry]
            lda         #NO_MUL
            ldx         #6
            ldy         #4
            jsr         cpx_part_with
            bcs         @fail8
            jsr         nr_push                             ; [.. d rxry ixiy]
            lda         #NO_ADD
            ldx         #1
            ldy         #0
            jsr         cpx_part_with
            bcs         @fail9
            jsr         nr_push                             ; [.. d rxry ixiy sum]
            lda         #NO_DIV
            ldx         #0
            ldy         #3
            jsr         cpx_part_with
            bcs         @fail10
            jsr         nr_push                             ; [.. d rxry ixiy sum re]
            lda         #NO_MUL                             ; (im = (ix ry - rx iy) / d)
            ldx         #9
            ldy         #8
            jsr         cpx_part_with
            bcs         @fail11
            jsr         nr_push                             ; [.. re ixry]
            lda         #NO_MUL
            ldx         #11
            ldy         #8
            jsr         cpx_part_with
            bcs         @fail12
            jsr         nr_push                             ; [.. re ixry rxiy]
            lda         #NO_SUB
            ldx         #1
            ldy         #0
            jsr         cpx_part_with
            bcs         @fail13
            jsr         nr_push                             ; [.. re ixry rxiy diff]
            lda         #NO_DIV
            ldx         #0
            ldy         #7
            jsr         cpx_part_with
            bcs         @fail14
            jsr         nr_push                             ; [.. re ixry rxiy diff im]
            ldx         #4
            jsr         nr_peek
            jsr         nr_push
            ldx         #1
            jsr         nr_peek
            jsr         nr_push
            jsr         cpx_norm
            lda         #17
            jmp         nr_drop
@fail10:
            lda         #10
            jmp         nr_drop
@fail11:
            lda         #11
            jmp         nr_drop
@fail12:
            lda         #12
            jmp         nr_drop
@fail13:
            lda         #13
            jmp         nr_drop
@fail14:
            lda         #14
            jmp         nr_drop

; h_v = root .X (n_nop) root .Y, real numbers
cpx_part_op:
            lda         n_nop
            ; (falls into cpx_part_with)

; h_v = root .X (op .A) root .Y, real numbers.  OUT: C = 1, an error
cpx_part_with:
            pha
            phy
            jsr         nr_peek
            MOVR        nx, h_v
            plx
            jsr         nr_peek
            pla
            ldx         n_nop                               ; (n_nop kept: num_op changes it)
            phx
            jsr         num_op
            plx
            stx         n_nop
            rts

; h_v = -h_v, a number.  OUT: C = 1, an error
num_neg:
            MOVR        nx, h_v                             ; (0 - it: for a fixed decimal, a fixed decimal)
            jsr         num_kind
            bcs         @not
            cmp         #NK_COMPLEX
            beq         @sub
            cmp         #NK_INT
            beq         @int
@sub:
            MOVR        h_v, nx
            LDR         nx, 1
            lda         #NO_SUB
            jmp         num_op
@int:
            MOVR        h_v, nx
            jmp         neg_int
@not:
            LDR         r0, nm_numbers
            jmp         io_error

; nx against h_v, numbers, by value: .A = $FF, 0 or 1.  OUT: C = 1, they aren't both numbers (complex numbers: by
; real part, then imaginary part)
num_cmp:
            jsr         num_kind
            jcs         @no
            sta         n_ky
            jsr         nr_push                             ; [y]
            MOVR        h_v, nx
            jsr         num_kind
            jcs         @no1
            sta         n_kx
            jsr         nr_push                             ; [y x]
            lda         n_kx
            ora         n_ky
            bne         :+
            MOVR        ia, nx                              ; (Integers)
            ldx         #1
            jsr         nr_peek
            jsr         int_cmp
            jmp         @done
:
            lda         n_kx
            cmp         #NK_COMPLEX
            beq         @cpx
            lda         n_ky
            cmp         #NK_COMPLEX
            beq         @cpx
            ldx         #0                                  ; (Real: nx dy against ny dx)
            jsr         nr_peek
            jsr         rat_parts                           ; [y x nx dx]
            ldx         #3
            jsr         nr_peek
            jsr         rat_parts                           ; [y x nx dx ny dy]
            lda         #IO_MUL
            ldx         #3
            ldy         #0
            jsr         iop2
            jcs         @fail4
            jsr         nr_push                             ; [.. a]
            lda         #IO_MUL
            ldx         #2
            ldy         #3
            jsr         iop2
            jcs         @fail5
            MOVR        ia, h_v                             ; (b; a against it)
            ldx         #0
            jsr         nr_peek
            ldx         ia
            ldy         ia + 1
            MOVR        ia, h_v
            stx         h_v
            sty         h_v + 1
            jsr         int_cmp
            pha
            lda         #5
            jsr         nr_drop
            pla
            bra         @done
@cpx:
            ldx         #0                                  ; (Complex: the real parts, then the imaginary)
            jsr         nr_peek
            jsr         cpx_parts                           ; [y x rx ix]
            ldx         #3
            jsr         nr_peek
            jsr         cpx_parts                           ; [y x rx ix ry iy]
            ldx         #3
            jsr         nr_peek
            MOVR        nx, h_v
            ldx         #1
            jsr         nr_peek
            jsr         num_cmp
            cmp         #0
            bne         :+
            ldx         #2
            jsr         nr_peek
            MOVR        nx, h_v
            ldx         #0
            jsr         nr_peek
            jsr         num_cmp
:
            pha
            lda         #4
            jsr         nr_drop
            pla
@done:
            pha
            lda         #2
            jsr         nr_drop
            pla
            clc
            rts
@fail5:
            lda         #1
            jsr         nr_drop
@fail4:
            lda         #4
            jsr         nr_drop
            bra         @done
@no1:
            lda         #1
            jsr         nr_drop
@no:
            sec
            rts

; ---- Showing them

; The integer h_v's digits (decimal), the last first: dg, dg_n of them; dg_neg $80 if it's negative
int_digits:
            jsr         nq_init
            NQ          nq_c, nc
            jsr         nb_load
            lda         nc + 1
            sta         dg_neg
            NQ          nq_d, dg                            ; (nq_d: the next digit's place)
            stz         dg_n
            stz         dg_n + 1
            lda         #10
            sta         n_s
@digit:
            jsr         mag_divs
            ora         #'0'
            sta         (nq_d)
            inc         nq_d
            bne         :+
            inc         nq_d + 1
:
            inc         dg_n
            bne         :+
            inc         dg_n + 1
:
            lda         nc
            bne         @digit
            rts

; dg's digits written, the first (the last made) first; with a . before the last n_scale of them (and 0s before
; them, if there aren't that many: 0.05)
dg_write:
            lda         dg_n + 1                            ; (Fewer digits than places: 0. and 0s first)
            bne         @digits
            lda         n_scale
            beq         @digits
            cmp         dg_n
            bcc         @digits
            lda         #'0'
            jsr         out_char
            lda         #'.'
            jsr         out_char
            sec
            lda         n_scale
            sbc         dg_n
            tax
:
            beq         @digits0
            lda         #'0'
            jsr         out_char
            dex
            bra         :-
@digits0:
            stz         n_scale                             ; (No . to come)
@digits:
            lda         dg_n                                ; (From the end: nq_d past it)
            ora         dg_n + 1
            beq         @done
            lda         nq_d
            bne         :+
            dec         nq_d + 1
:
            dec         nq_d
            lda         dg_n
            bne         :+
            dec         dg_n + 1
:
            dec         dg_n
            lda         (nq_d)
            jsr         out_char
            lda         n_scale                             ; (n_scale digits left: the . first)
            beq         @digits
            lda         dg_n + 1
            bne         @digits
            lda         dg_n
            cmp         n_scale
            bne         @digits
            ora         #0
            beq         @digits
            lda         #'.'
            jsr         out_char
            bra         @digits
@done:
            rts

; h_v (a number) written as the REPL shows it: 42, 1.5, 1/2, 1+2i
num_print:
            jsr         num_kind
            bcs         @done
            tax
            lda         np_lo,x
            sta         ev_j
            lda         np_hi,x
            sta         ev_j + 1
            jmp         (ev_j)
@done:
            rts

np_int:
            stz         n_scale
np_digits:
            jsr         int_digits
            bit         dg_neg
            bpl         :+
            lda         #'-'
            jsr         out_char
:
            jmp         dg_write

np_fix:
            MOVR        n_obj, h_v
            ldy         #4
            jsr         num_field
            lda         h_v
            lsr
            pha
            MOVR        h_v, n_obj
            ldy         #2
            jsr         num_field
            pla
            sta         n_scale
            bra         np_digits

np_rat:
            MOVR        n_obj, h_v
            ldy         #2
            jsr         num_field
            jsr         np_int
            lda         #'/'
            jsr         out_char
            MOVR        h_v, n_obj
            ldy         #4
            jsr         num_field
            bra         np_int

np_cpx:
            jsr         num_fields                          ; (Its parts: pushed, as the printing goes on)
            ldx         #1
            jsr         nr_peek
            jsr         num_zero
            bcs         :+
            ldx         #1                                  ; (Its real part, unless it's 0)
            jsr         nr_peek
            jsr         num_print
            ldx         #0                                  ; (+, unless the imaginary part has a -)
            jsr         nr_peek
            jsr         num_sign
            cmp         #$FF
            beq         :+
            lda         #'+'
            jsr         out_char
:
            ldx         #0
            jsr         nr_peek
            jsr         num_print
            lda         #'i'
            jsr         out_char
            lda         #2
            jmp         nr_drop

; h_v's sign, a number's (a complex number's: its real part's, else its imaginary part's): .A = $FF, 0 or 1.
; OUT: C = 1, it isn't a number
num_sign_of:
            jsr         num_kind
            bcs         @no
            cmp         #NK_COMPLEX
            bne         @real
            MOVR        n_obj, h_v
            ldy         #2
            jsr         num_field
            jsr         num_sign
            cmp         #0
            bne         @done
            MOVR        h_v, n_obj
            ldy         #4
            jsr         num_field
@real:
            jsr         num_sign
@done:
            clc
            rts
@no:
            sec
            rts

; h_v's sign (a real number): .A = $FF, 0 or 1
num_sign:
            jsr         num_kind
            cmp         #NK_INT
            beq         @int
            ldy         #2                                  ; (A fixed decimal's digits, a rational's numerator)
            jsr         num_field
@int:
            jmp         int_sign

; ---- Reading them

; The number written in the .Y bytes at r0 (in the task's RAM): decimal digits, _ among them, a sign before them, a
; . and more after them (a fixed decimal); a / and another (a rational); a base (#x ...: bases.inc).  OUT: C = 0,
; h_v = it; C = 1: .X = 0, it isn't a number; .X = RD_ERR, an error (h_v: too big, division by zero)
num_parse:
            sty         n_plen
            ldy         #0                                  ; (A /: a rational, two integers)
:
            cpy         n_plen
            beq         @whole
            lda         (r0),y
            cmp         #'/'
            beq         @rat
            iny
            bra         :-
@rat:
            sty         n_slash
            jsr         parse_int                           ; (Its numerator: the bytes before the /)
            bcs         @fail
            jsr         nr_push
            clc                                             ; (Its denominator: those after it)
            lda         n_slash
            adc         #1
            adc         r0
            sta         r0
            bcc         :+
            inc         r0 + 1
:
            lda         n_plen
            clc
            sbc         n_slash
            tay
            jsr         parse_int
            bcs         @fail1
            jsr         nr_push
            jsr         rat_norm
            bcc         @done
            ldx         #RD_ERR
            rts
@fail1:
            pha
            lda         #1
            jsr         nr_drop
            pla
@fail:
            rts
@whole:
            ldy         n_plen
            jsr         parse_real
@done:
            rts

; The integer in the first .Y bytes at r0 (in either base: no . in it).  OUT: as num_parse's
parse_int:
            jsr         parse_real
            bcs         @done
            jsr         num_kind
            cmp         #NK_INT
            beq         @ok
            ldx         #0                                  ; (Not an integer: 1/2.5 isn't a number)
            sec
            rts
@ok:
            clc
@done:
            rts

; The number, not a rational, in the .Y bytes at r0: a decimal one (an integer, a fixed decimal), or one in a base
parse_real:
            sty         p_end
            stz         p_sign                              ; (p_sign: $80, a - before it)
            ldy         #0
            cpy         p_end
            jeq         @not
            lda         (r0),y
            cmp         #'+'
            beq         @sign
            cmp         #'-'
            bne         @base
            lda         #$80
            sta         p_sign
@sign:
            iny
            cpy         p_end
            jeq         @not
@base:
            lda         (r0),y
            cmp         #'#'
            bne         @decimal
            jmp         parse_base                          ; (bases.inc)
@decimal:
            jsr         nq_init                             ; ---- Decimal: C = 0, then its digits
            NQ          nq_c, nc
            phy
            jsr         nb_zero
            ply
            stz         n_scale
            stz         n_cnt                               ; (Digits seen: some, before any . and after it)
            lda         #10
            sta         n_s
@digit:
            cpy         p_end
            beq         @end
            lda         (r0),y
            iny
            cmp         #'_'
            beq         @digit
            cmp         #'.'
            beq         @point
            sec
            sbc         #'0'
            cmp         #10
            bcs         @not
            sta         n_t
            phy
            jsr         mag_muls
            ply
            lda         nc
            cmp         #NB_MAX + 1
            bcs         @big
            inc         n_cnt
            lda         n_scale                             ; (Past the .: a place more)
            beq         @digit
            inc         n_scale
            bra         @digit
@point:
            lda         n_scale                             ; (One ., digits before it)
            bne         @not
            lda         n_cnt
            beq         @not
            stz         n_cnt
            inc         n_scale
            bra         @digit
@end:
            lda         n_cnt                               ; (Digits at the end: none, not a number)
            beq         @not
            lda         p_sign
            sta         nc + 1
            lda         n_scale
            beq         @int
            dec         n_scale                             ; (Its places: one less than counted)
            jsr         nb_store
            bcs         @err
            jsr         make_fix
            bcs         @err
            clc
            rts
@int:
            jsr         nb_store
            bcs         @err
            rts
@big:
            LDR         r0, nm_big
            jsr         io_error
@err:
            ldx         #RD_ERR
            sec
            rts
@not:
            ldx         #0
            sec
            rts

; h_v = h_v's integer part, toward 0 (a complex number's real part's).  OUT: C = 1, it isn't a number
num_trunc:
            jsr         num_kind
            bcs         @done
            cmp         #NK_INT
            beq         @int
            cmp         #NK_COMPLEX
            bne         :+
            ldy         #2
            jsr         num_field
            bra         num_trunc
:
            jsr         rat_parts                           ; (A rational's, a fixed decimal's: its numerator over
            lda         #IO_QUO                             ; its denominator)
            ldx         #1
            ldy         #0
            jsr         iop2
            lda         #2
            jmp         nr_drop
@int:
            clc
@done:
            rts

; ---- Built-ins (hylnum's: BF_NUM)

; (truncate n): n's integer part, toward 0 (a rational's, a fixed decimal's; a complex number's real part's)
bn_truncate:
            jsr         count_a
            cmp         #1
            bne         @not
            jsr         arg_next
            jsr         num_trunc
            bcs         @not
            MOVR        ev_v, h_v
            rts
@not:
            LDR         r0, nm_truncate
            jmp         ev_error_text

; (complex re im): the complex number re + im i (as it is: (complex 1 0) too)
bn_complex:
            jsr         count_a
            cmp         #2
            beq         :+
            LDR         r0, nm_complex_few
            jmp         ev_error_text
:
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         arg_next
            MOVR        ev_t, h_v
            jsr         num_kind                            ; (Each a number, not complex)
            bcs         @not
            cmp         #NK_COMPLEX
            beq         @part
            MOVR        h_v, ev_x
            jsr         num_kind
            bcs         @not
            cmp         #NK_COMPLEX
            beq         @part
            MOVR        h_a, ev_x
            MOVR        h_d, ev_t
            lda         #OT_COMPLEX
            ldx         #0
            jsr         make_cell2
            bcc         :+
            MOVR        h_v, err_room
:
            MOVR        ev_v, h_v
            rts
@not:
            LDR         r0, nm_complex_num
            jmp         ev_error_text
@part:
            LDR         r0, nm_complex_part
            jmp         ev_error_text

; (val s): the number written in the string s (an error, if it isn't one)
bn_val:
            jsr         count_a
            cmp         #1
            beq         :+
            LDR         r0, nm_val_one
            jmp         ev_error_text
:
            jsr         arg_next
            jsr         nb_text                             ; (Its text, in dg)
            bcs         @not_str
            LDR         r0, dg
            ldy         dg_n
            jsr         num_parse
            bcs         @not
            MOVR        ev_v, h_v
            rts
@not:
            cpx         #RD_ERR
            bne         :+
            MOVR        ev_v, h_v
            rts
:
            LDR         r0, nm_val1                         ; ('val': "abc" isn't a number)
            jsr         msg_start
            ldx         #0
:
            cpx         dg_n
            beq         :+
            lda         dg,x
            jsr         msg_char
            inx
            bra         :-
:
            LDR         r0, nm_val2
            jsr         msg_text
            jmp         msg_error
@not_str:
            LDR         r0, nm_val_str
            jmp         ev_error_text

; h_v's text (a string's, or a character's) copied to dg: dg_n bytes.  OUT: C = 1, it isn't a string
nb_text:
            jsr         type_of
            cmp         #TY_CHAR
            beq         @char
            cmp         #TY_STRING
            bne         @no
            jsr         deref
            ldy         #1
            lda         (h_p),y
            sta         dg_n
            ldx         #0
            ldy         #2
:
            cpx         dg_n
            beq         :+
            lda         (h_p),y
            sta         dg,x
            inx
            iny
            bra         :-
:
            clc
            rts
@char:
            lda         h_v + 1                             ; (Its code: (the value - $0100) / 2)
            lsr
            lsr
            lda         h_v
            ror
            sta         dg
            lda         #1
            sta         dg_n
            clc
            rts
@no:
            sec
            rts

; (to-fixed x [places]): x as a fixed decimal, with places at most (10, if none), truncated
bn_to_fixed:
            jsr         count_a
            cmp         #1
            jcc         @not
            jsr         arg_next
            jsr         num_kind
            jcs         @not
            jsr         nr_push                             ; [x]
            lda         #10
            sta         n_places                            ; (n_places: the places at most)
            jsr         arg_next
            bcs         :+
            lda         h_v                                 ; (Places: a fixnum's, 0-255)
            lsr
            bcc         :+
            lda         h_v + 1
            bne         :+
            lda         h_v
            lsr
            sta         n_places
:
            ldx         #0
            jsr         nr_peek
            jsr         rat_parts                           ; [x n d]
            ldx         #1                                  ; (The sign, then the size: n as |n|)
            jsr         nr_peek
            jsr         num_sign
            sta         n_sgn
            cmp         #$FF
            bne         :+
            jsr         neg_int
            jcs         @fail3
            ldx         #1
            jsr         nr_poke
:
            lda         #IO_QUO                             ; (w = n / d, r = n % d)
            ldx         #1
            ldy         #0
            jsr         iop2
            jcs         @fail3
            jsr         nr_push                             ; [x n d w]
            lda         #IO_REM
            ldx         #2
            ldy         #1
            jsr         iop2
            jcs         @fail4
            jsr         nr_push                             ; [x n d w r]
            stz         n_scale
@place:
            lda         n_scale                             ; (While r > 0, and places are left: r = 10 r;
            cmp         n_places                            ; w = 10 w + r / d; r = r % d)
            jcs         @made
            ldx         #0
            jsr         nr_peek
            jsr         num_zero
            jcs         @made
            inc         n_scale
            MOVR        ia, h_v
            LDR         h_v, 21
            lda         #IO_MUL
            jsr         int_op
            jcs         @fail5
            jsr         root_set                            ; [x n d w 10r]
            lda         #IO_QUO
            ldx         #0
            ldy         #2
            jsr         iop2
            bcs         @fail5
            jsr         nr_push                             ; [x n d w 10r q]
            lda         #IO_REM
            ldx         #1
            ldy         #3
            jsr         iop2
            bcs         @fail6
            ldx         #1
            jsr         nr_poke                             ; [x n d w r' q]
            ldx         #2
            jsr         nr_peek
            MOVR        ia, h_v
            LDR         h_v, 21
            lda         #IO_MUL
            jsr         int_op
            bcs         @fail6
            MOVR        ia, h_v
            ldx         #0
            jsr         nr_peek
            lda         #IO_ADD
            jsr         int_op
            bcs         @fail6
            ldx         #2
            jsr         nr_poke                             ; [x n d w' r' q]
            lda         #1
            jsr         nr_drop
            jmp         @place
@made:
            ldx         #1                                  ; (w, its sign n's)
            jsr         nr_peek
            lda         n_sgn
            cmp         #$FF
            bne         :+
            jsr         neg_int
            bcs         @fail5
:
            jsr         make_fix
            MOVR        ev_v, h_v
            lda         #5
            jmp         nr_drop
@fail6:
            lda         #1
            jsr         nr_drop
@fail5:
            lda         #1
            jsr         nr_drop
@fail4:
            lda         #1
            jsr         nr_drop
@fail3:
            MOVR        ev_v, h_v
            lda         #3
            jmp         nr_drop
@not:
            LDR         r0, nm_to_fixed
            jmp         ev_error_text

; (to-rational x): x as a rational (an integer's, over 1: a rational all the same)
bn_to_rational:
            jsr         count_a
            cmp         #1
            bne         @not
            jsr         arg_next
            jsr         num_kind
            bcs         @not
            cmp         #NK_RAT
            beq         @done
            cmp         #NK_COMPLEX
            bne         :+
            LDR         h_v, 1                              ; (danlang's: a complex number's, 0)
:
            jsr         rat_parts                           ; (n / d, in their lowest terms)
            jsr         rat_norm
            bcs         @done
            jsr         num_kind
            cmp         #NK_RAT
            beq         @done
            jsr         nr_push                             ; (Whole: over 1, as it is)
            LDR         h_v, 3
            jsr         nr_push
            jsr         make_rat
            pha
            lda         #2
            jsr         nr_drop
            pla
@done:
            MOVR        ev_v, h_v
            rts
@not:
            LDR         r0, nm_to_rational
            jmp         ev_error_text

; (rational.n x), (rational.d x): x's numerator, or denominator, as a rational in its lowest terms
bn_rational_n:
            lda         #0
            bra         :+
bn_rational_d:
            lda         #1
:
            sta         n_sgn
            jsr         count_a
            cmp         #1
            bcs         :+
            LDR         r0, nm_rat_args
            jmp         ev_error_text
:
            jsr         arg_next
            jsr         num_kind
            bcs         @not
            cmp         #NK_COMPLEX
            beq         @not
            jsr         rat_parts
            jsr         rat_norm
            bcs         @done
            jsr         num_kind
            cmp         #NK_RAT
            beq         @rat
            lda         n_sgn                               ; (An integer: itself, over 1)
            beq         @done
            LDR         h_v, 3
            bra         @done
@rat:
            ldy         #2
            lda         n_sgn
            beq         :+
            ldy         #4
:
            jsr         num_field
@done:
            MOVR        ev_v, h_v
            rts
@not:
            LDR         r0, nm_rat_num
            jmp         ev_error_text

; (random n): an integer from 0 to n - 1, at random (n an integer, more than 0)
bn_random:
            jsr         count_a
            cmp         #1
            bne         @not
            jsr         arg_next
            jsr         num_kind
            bcs         @not
            cmp         #NK_INT
            bne         @not
            jsr         int_sign
            cmp         #1
            bne         @not
            jsr         nq_init                             ; (B = n; A = random bytes, 2 more than n has)
            NQ          nq_c, nb
            jsr         nb_load
            lda         nb
            clc
            adc         #2
            sta         na
            stz         na + 1
            tax
            ldy         #2
:
            phx
            phy
            jsr         rnd_byte
            ply
            plx
            sta         na,y
            iny
            dex
            bne         :-
            NQ          nq_c, na
            jsr         nb_trim
            NQ          nq_c, nc                            ; (D = A % B)
            jsr         mag_div
            NQ          nq_c, nd
            jsr         nb_store
            MOVR        ev_v, h_v
            rts
@not:
            LDR         r0, nm_random
            jmp         ev_error_text

; .A = a random byte: xorshift32's (seeded, the first time, from the ticks and the clock)
rnd_byte:
            lda         rnd_seed
            ora         rnd_seed + 1
            ora         rnd_seed + 2
            ora         rnd_seed + 3
            bne         @step
            jsr         TICKS
            sta         rnd_seed
            stx         rnd_seed + 1
            jsr         TIME
            lda         r0
            sta         rnd_seed + 2
            lda         r0 + 1
            ora         #1
            sta         rnd_seed + 3
@step:
            ldx         #13                                 ; (x ^= x << 13; x ^= x >> 17; x ^= x << 5)
            jsr         rnd_shl
            ldx         #17
            jsr         rnd_shr
            ldx         #5
            jsr         rnd_shl
            lda         rnd_seed + 1
            eor         rnd_seed + 3
            rts

; rnd_seed ^= rnd_seed << .X
rnd_shl:
            jsr         rnd_copy
:
            asl         rnd_t
            rol         rnd_t + 1
            rol         rnd_t + 2
            rol         rnd_t + 3
            dex
            bne         :-
            bra         rnd_xor

; rnd_seed ^= rnd_seed >> .X
rnd_shr:
            jsr         rnd_copy
:
            lsr         rnd_t + 3
            ror         rnd_t + 2
            ror         rnd_t + 1
            ror         rnd_t
            dex
            bne         :-
rnd_xor:
            ldy         #3
:
            lda         rnd_t,y
            eor         rnd_seed,y
            sta         rnd_seed,y
            dey
            bpl         :-
            rts

rnd_copy:
            ldy         #3
:
            lda         rnd_seed,y
            sta         rnd_t,y
            dey
            bpl         :-
            rts

; (fib n): the nth Fibonacci number (0 1 1 2 3 5 ...)
bn_fib:
            jsr         count_a
            cmp         #1
            jne         @not
            jsr         arg_next
            lda         h_v                                 ; (A fixnum, not negative)
            lsr
            jcc         @not
            lda         h_v + 1
            jmi         @neg
            lsr
            sta         n_q
            lda         h_v
            ror
            sta         n_rem                               ; (n_rem/n_q: n, a count down)
            jsr         nq_init                             ; (A = 0, B = 1)
            stz         na
            stz         na + 1
            lda         #1
            sta         nb
            sta         nb + 2
            stz         nb + 1
@step:
            lda         n_rem
            ora         n_q
            beq         @done
            lda         nq_d                                ; (D = A + B; then A, B = B, D: the pointers)
            sta         nq_c
            lda         nq_d + 1
            sta         nq_c + 1
            jsr         mag_add
            lda         (nq_c)
            cmp         #NB_MAX + 1
            bcs         @big
            ldx         nq_a
            ldy         nq_a + 1
            lda         nq_b
            sta         nq_a
            lda         nq_b + 1
            sta         nq_a + 1
            lda         nq_d
            sta         nq_b
            lda         nq_d + 1
            sta         nq_b + 1
            stx         nq_d
            sty         nq_d + 1
            lda         n_rem
            bne         :+
            dec         n_q
:
            dec         n_rem
            bra         @step
@done:
            lda         nq_a
            sta         nq_c
            lda         nq_a + 1
            sta         nq_c + 1
            ldy         #1
            lda         #0
            sta         (nq_c),y
            jsr         nb_store
            MOVR        ev_v, h_v
            rts
@big:
            LDR         r0, nm_big
            jmp         ev_error_text
@neg:
            LDR         r0, nm_fib_neg
            jmp         ev_error_text
@not:
            LDR         r0, nm_fib
            jmp         ev_error_text

; num_to_base, for to-str (the second bank's): the number ev_x written in the base at bs_spec (bs_spec_n bytes,
; in the task's RAM).  OUT: C = 1, h_v an error
bn_to_base:
            MOVR        h_v, ev_x
            jmp         num_to_base

.segment "NUMDATA"
nk_of:      .byte       NK_INT, NK_RAT, NK_FIX, NK_COMPLEX  ; (OT_BIG, OT_RAT, OT_FIX, OT_COMPLEX)
io_of_no:   .byte       IO_ADD, IO_SUB, IO_MUL              ; (num_op's integers)
np_lo:      .lobytes    np_int, np_fix, np_rat, np_cpx
np_hi:      .hibytes    np_int, np_fix, np_rat, np_cpx
nm_numbers: .byte       "Expects numbers", 0
nm_truncate: .byte      "'truncate' expects a Number", 0
nm_complex_few: .byte   "Too few parameters passed to 'complex'", 0
nm_complex_num: .byte   "One or more parameters passed to 'complex' is not a Number", 0
nm_complex_part: .byte  "A complex number's parts must be an int, a rational, or a fixed", 0
nm_val_one: .byte       "'val' requires 1 parameter", 0
nm_val_str: .byte       "'val' expects a String", 0
nm_val1:    .byte       "'val': ", '"', 0
nm_val2:    .byte       '"', " isn't a number", 0
nm_to_fixed: .byte      "'to-fixed' expects a Number", 0
nm_to_rational: .byte   "'to-rational' expects a Number", 0
nm_rat_args: .byte      "One or more arguments required", 0
nm_rat_num: .byte       "Argument is not a number", 0
nm_random:  .byte       "'random' expects an integer more than 0", 0
nm_fib:     .byte       "'fib' expects one parameter of type Number", 0
nm_fib_neg: .byte       "'fib' parameter 'n' cannot be negative", 0
.segment "NUMCODE"
