; ****************************************************************************
; library.inc - the functions of danlang's library that are built in (danlang's LibraryBuiltins.cs), in the module's
; third bank (BF_LIB): what its globals.dl had as danlang, each doing what that did, an item of a list as it's
; written.  A comparison is compare's or equal's (the second bank's: the order of <, the equality of eq); 1+, 1-,
; abs, sum and product are calls of +, - or * (the call becomes theirs: ev_again), on the built-ins as hylang starts
; (lib_init), whatever's defined later.  map, filter, foldl, foldr, any?, all?, find and count are the evaluator's
; (eval.inc: BF_CONT), as they call a function for each item.

.bss
lb_plus:    .res        2                                   ; The built-ins +, - and * ...
lb_minus:   .res        2
lb_times:   .res        2
lb_from:    .res        2                                   ; A call that becomes another's: its arguments' place
lb_n:       .res        2                                   ; A count
lb_k:       .res        1                                   ; A kind, or an order
.segment "CODE3"

; The built-ins +, - and *, kept.  OUT: C = 1, no room
lib_init:
            ldx         #0
@name:
            phx
            lda         ln_lo,x
            sta         r0
            lda         ln_hi,x
            sta         r0 + 1
            lda         #OT_SYMBOL
            jsr         intern_counted
            bcs         @no
            jsr         sym_value
            plx
            txa
            asl
            tay
            lda         h_v
            sta         lb_plus,y
            lda         h_v + 1
            sta         lb_plus + 1,y
            inx
            cpx         #3
            bne         @name
            clc
            rts
@no:
            plx
            rts

; ---- Helpers

; The call becomes ev_f's, of the arguments from lb_from to the stack's top: the evaluator applies it (ev_again)
lb_tail:
            MOVR        ar_at, lb_from
            jsr         ev_here
            sta         ar_end
            stx         ar_end + 1
            lda         #1
            sta         ev_again
            rts

; .A = ev_x against h_v, in <'s order: $FF less, 0 the same, 1 more (compare's, the second bank's)
lb_compare:
            lda         #8
            sta         ev_c
            FARN        2, compare
            rts

; h_v, a list argument: h_v = its chain.  OUT: C = 1, it isn't one (ev_v an error)
lb_list:
            jsr         type_of
            cmp         #TY_LIST
            bne         @not
            jsr         list_of
            clc
            rts
@not:
            LDR         r0, lm_list
            jmp         ev_error_text

; ev_v = the item lb_n of the chain h_v.  OUT: C = 1, there's none (ev_v an error)
lb_item:
            lda         h_v + 1
            beq         @none
            lda         lb_n
            ora         lb_n + 1
            beq         @here
            jsr         cdr
            lda         lb_n
            bne         :+
            dec         lb_n + 1
:
            dec         lb_n
            bra         lb_item
@here:
            jsr         car
            MOVR        ev_v, h_v
            clc
            rts
@none:
            LDR         r0, lm_no_item
            jmp         ev_error_text

; ---- Logic and comparison

; (not a): T if a is NIL, else NIL
lb_not:
            jsr         arg_next
            lda         h_v
            ora         h_v + 1
            bne         lb_nil
lb_t:
            sec
            jmp         bi_bool
lb_nil:
            clc
            jmp         bi_bool

; (== a b): eq's
lb_eqq:
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         arg_next
            FARN        2, equal
            jmp         bi_bool

; (>= a b), (<= a b): (not (< a b)), (not (> a b))
lb_ge:
            lda         #$FF
            bra         :+
lb_le:
            lda         #1
:
            sta         lb_k
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         arg_next
            jsr         lb_compare
            cmp         lb_k
            beq         lb_nil
            bra         lb_t

; (neg? a), (pos? a): (< a 0), (< 0 a)
lb_neg:
            jsr         arg_next
            MOVR        ev_x, h_v
            LDR         h_v, 1                              ; (0)
            bra         :+
lb_pos:
            jsr         arg_next
            LDR         ev_x, 1
:
            jsr         lb_compare
            cmp         #$FF
            beq         lb_t
            bra         lb_nil

; (zero? a), (one? a): (eq 0 a), (eq 1 a)
lb_zero:
            LDR         ev_x, 1
            bra         :+
lb_one:
            LDR         ev_x, 3
:
            jsr         arg_next
            FARN        2, equal
            jmp         bi_bool

; ---- Arithmetic

; (1+ n), (1- n): (+ n 1), (- n 1)
lb_1plus:
            MOVR        ev_f, lb_plus
            bra         :+
lb_1minus:
            MOVR        ev_f, lb_minus
:
            MOVR        lb_from, ar_at
            lda         #3                                  ; (1)
            ldx         #0
            jsr         ev_push
            jmp         lb_tail

; (abs a): a number's size: (- a) if it's less than 0
lb_abs:
            MOVR        lb_from, ar_at
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_NUM
            bne         @not
            LDR         h_v, 1
            jsr         lb_compare
            cmp         #$FF
            beq         @neg
            MOVR        ev_v, ev_x
            rts
@neg:
            MOVR        ev_f, lb_minus
            jmp         lb_tail
@not:
            LDR         r0, lm_abs
            jmp         ev_error_text

; (sum l), (product l): (+ 0 item ...), (* 1 item ...), as (foldl + 0 l) and (foldl * 1 l) are
lb_sum:
            MOVR        ev_f, lb_plus
            lda         #1                                  ; (0)
            bra         :+
lb_product:
            MOVR        ev_f, lb_times
            lda         #3                                  ; (1)
:
            sta         lb_k
            jsr         arg_next
            jsr         lb_list
            bcs         @done
            MOVR        ev_r, h_v
            jsr         ev_here                             ; (Their arguments, after its own)
            sta         lb_from
            stx         lb_from + 1
            lda         lb_k
            ldx         #0
            jsr         ev_push
@item:
            lda         ev_r + 1
            beq         @call
            jsr         ev_deep
            bcs         @deep
            MOVR        h_v, ev_r
            jsr         car
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            bra         @item
@call:
            jmp         lb_tail
@deep:
            LDR         r0, lm_long
            jmp         ev_error_text
@done:
            rts

; (min a b ...), (max a b ...): the least (or most), by < (or >): each in turn, kept unless the next is less (more)
lb_min:
            lda         #$FF
            bra         :+
lb_max:
            lda         #1
:
            sta         lb_k
            jsr         arg_next
            MOVR        ev_s, h_v
@next:
            jsr         arg_next
            bcs         @done
            MOVR        ev_t, h_v
            MOVR        ev_x, ev_s
            jsr         lb_compare
            cmp         lb_k
            beq         @next
            MOVR        ev_s, ev_t
            bra         @next
@done:
            MOVR        ev_v, ev_s
            rts

; ---- Lists: an item as it's written

; (cons a l): a list of a, then l's items
lb_cons:
            jsr         arg_next
            MOVR        ev_t, h_v
            jsr         arg_next
            jsr         lb_list
            bcs         @done
            MOVR        h_d, h_v
            MOVR        h_a, ev_t
            jsr         cons
            bcs         lb_room
            lda         #OT_QEXPR
            jsr         make_list
            bcs         lb_room
            MOVR        ev_v, h_v
@done:
            rts
lb_room:
            jmp         bi_no_room

; (fst l), (snd l), (thd l): its item 0, 1 or 2
lb_fst:
            lda         #0
            bra         :+
lb_snd:
            lda         #1
            bra         :+
lb_thd:
            lda         #2
:
            sta         lb_n
            stz         lb_n + 1
            jsr         arg_next
            jsr         lb_list
            bcs         :+
            jmp         lb_item
:
            rts

; (nth n l): l's item n (from 0); before its first, or past its last, an error.  n not a whole number: its
; whole part's (if n's in the list)
lb_nth:
            jsr         arg_next
            MOVR        ev_t, h_v
            jsr         arg_next
            jsr         lb_list
            jcs         @done
            MOVR        ev_s, h_v
            lda         ev_t                                ; (A fixnum: not negative)
            lsr
            bcc         @other
            lda         ev_t + 1
            bmi         @none
            lsr
            sta         lb_n + 1
            lda         ev_t
            ror
            sta         lb_n
@item:
            MOVR        h_v, ev_s
            jmp         lb_item
@other:
            MOVR        h_v, ev_t                           ; (Another number: not less than 0, its whole part a
            jsr         type_of                             ; fixnum)
            cmp         #TY_NUM
            bne         @not
            MOVR        ev_x, ev_t
            LDR         h_v, 1
            jsr         lb_compare
            cmp         #$FF
            beq         @none
            MOVR        h_v, ev_t
            FARX        lib_num, num_trunc
            lda         h_v
            lsr
            bcc         @none
            lsr         h_v + 1
            ror         h_v
            MOVR        lb_n, h_v
            bra         @item
@none:
            LDR         r0, lm_no_item
            jmp         ev_error_text
@not:
            LDR         r0, lm_nth
            jmp         ev_error_text
@done:
            rts

; (last l): l's last item; none, an error
lb_last:
            jsr         arg_next
            jsr         lb_list
            bcs         @done
            lda         h_v + 1
            beq         @none
:
            MOVR        ev_s, h_v                           ; (The last pair: its car)
            jsr         cdr
            lda         h_v + 1
            bne         :-
            MOVR        h_v, ev_s
            jsr         car
            MOVR        ev_v, h_v
@done:
            rts
@none:
            LDR         r0, lm_no_item
            jmp         ev_error_text

; (take n l), (drop n l): l's first n items, or the rest after them (n counted down to 0: a negative n, or one that
; isn't a whole number, is all of them)
lb_take:
            lda         #0
            bra         :+
lb_drop:
            lda         #1
:
            sta         lb_k
            jsr         arg_next
            MOVR        ev_t, h_v
            jsr         arg_next
            jsr         lb_list
            jcs         @done
            MOVR        ev_r, h_v
            lda         ev_r + 1
            jeq         @empty
            lda         ev_t                                ; (How many: a fixnum not negative, itself)
            lsr
            bcc         @other
            lda         ev_t + 1
            bmi         @all
            lsr
            sta         lb_n + 1
            lda         ev_t
            ror
            sta         lb_n
            bra         @go
@other:
            MOVR        h_v, ev_t                           ; (Another number: all; not a number, an error)
            jsr         type_of
            cmp         #TY_NUM
            jne         @not
@all:
            lda         #$FF
            sta         lb_n
            sta         lb_n + 1
@go:
            lda         lb_k
            bne         @drop
            LDR         ev_a, NIL                           ; ---- take: a copy of the first n
            LDR         ev_t, NIL
@take:
            lda         lb_n
            ora         lb_n + 1
            beq         @made
            lda         ev_r + 1
            beq         @made
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_v, h_v
            jsr         ev_append
            jcs         lb_room
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            jsr         @count
            bra         @take
@made:
            MOVR        h_v, ev_a
            bra         @list
@drop:
            lda         lb_n                                ; ---- drop: the rest after them
            ora         lb_n + 1
            beq         @rest
            lda         ev_r + 1
            beq         @rest
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            jsr         @count
            bra         @drop
@rest:
            MOVR        h_v, ev_r
@list:
            lda         #OT_QEXPR
            jsr         make_list
            jcs         lb_room
            MOVR        ev_v, h_v
            rts
@empty:
            LDR         ev_v, NIL
@done:
            rts
@not:
            LDR         r0, lm_nth
            jmp         ev_error_text
@count:
            lda         lb_n
            bne         :+
            dec         lb_n + 1
:
            dec         lb_n
            rts

; (elem? x l), (in? x l): T if an item of l is equal to x (eq's)
lb_elem:
            jsr         arg_next
            MOVR        ev_s, h_v
            jsr         arg_next
            jsr         lb_list
            bcs         @done
            MOVR        ev_r, h_v
@next:
            lda         ev_r + 1
            jeq         lb_nil
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_x, ev_s
            FARN        2, equal
            jcs         lb_t
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            bra         @next
@done:
            rts

.segment "RODATA3"
ln_lo:      .lobytes    ln_plus, ln_minus, ln_times
ln_hi:      .hibytes    ln_plus, ln_minus, ln_times
ln_plus:    .byte       1, "+"
ln_minus:   .byte       1, "-"
ln_times:   .byte       1, "*"
lm_list:    .byte       "Expects a list", 0
lm_nth:     .byte       "Expects a Number and a list", 0
lm_no_item: .byte       "'nth': the list has no such item", 0
lm_abs:     .byte       "Cannot perform 'abs' on non-number", 0
lm_long:    .byte       "Too many items for the stack", 0
.segment "CODE3"
