; ****************************************************************************
; lists.inc - hylang's list built-ins (phase 3; the rest with phase 4), in its second bank: each as danlang's
; (Builtins.cs, LibraryBuiltins.cs), its errors' messages too.  Each gets its values at argp (cn of them) and gives
; back ex (C = 1 if there's no room).  A list made from one keeps the scope the first was written in (a scoped
; Q-expression's), as danlang's do; one made new has none.  cf and cr are free for them (roots): a list being made is
; cf (its first cons) and cr (its last).

.segment "CODE2"

; ---- Helpers

; ex = the argument 0 (1, 2 ...)
l_arg0:
            ldy         #0
            bra         l_arg
l_arg1:
            ldy         #2
            bra         l_arg
l_arg2:
            ldy         #4
l_arg:
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            rts

; An error made of the zero-terminated text at .A, .X (in this bank): ex.  OUT: C = 1 if there's no room
l_error:
            sta         hq
            stx         hq + 1
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            jsr         error_make
            bcs         :+
            MOVW        ex, hv
:
            rts

; An error's message begun (capture), and ended: ex = the error.  OUT: C = 1 if there's no room
l_begin:
            jsr         cap_begin
            sta         l_start
            stx         l_start + 1
            rts
l_end:
            lda         l_start
            ldx         l_start + 1
            jsr         cap_end
            bcs         @rts
            lda         hv
            ldx         hv + 1
            jsr         kind_of
            cmp         #PK_ERROR
            beq         @done
            jsr         error_wrap
            bcs         @rts
@done:
            MOVW        ex, hv
            clc
@rts:
            rts

; A list begun (cf, cr: none yet)
lb_begin:
            stz         cf
            stz         cf + 1
            stz         cr
            stz         cr + 1
            rts

; et put at the list's end.  OUT: C = 1 if there's no room
lb_add:
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         make2
            bcs         @rts
            lda         cf
            ora         cf + 1
            bne         @link
            MOVW        cf, hv
            MOVW        cr, hv
            clc
@rts:
            rts
@link:
            lda         cr
            ldx         cr + 1
            jsr         deref
            ldy         #2
            lda         hv
            sta         (hp),y
            iny
            lda         hv + 1
            sta         (hp),y
            MOVW        cr, hv
            clc
            rts

; The list made (cf), ex; scoped with the scope .A, .X if that isn't NIL.  OUT: C = 1 if there's no room
lb_done:
            sta         l_scope
            stx         l_scope + 1
            MOVW        ex, cf
; ex, a list's conses, scoped with l_scope (if it isn't NIL; and the list isn't empty).  OUT: C = 1 if there's no room
l_scoped:
            lda         l_scope
            ora         l_scope + 1
            beq         @plain
            lda         ex
            ora         ex + 1
            beq         @plain
            MOVW        et, ex
            MOVW        eu, l_scope
            lda         #PK_SCOPED
            jsr         make2
            bcs         @rts
            MOVW        ex, hv
@plain:
            clc
@rts:
            rts

; ex, a Q-expression: its conses (.A, .X and l_items) and its scope (l_scope: NIL if it has none).  OUT: C = 1 if
; it isn't one
l_list:
            stz         l_scope
            stz         l_scope + 1
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_SCOPED
            bne         :+
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         cell_get
            sta         l_scope
            stx         l_scope + 1
:
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcs         @rts
            sta         l_items
            stx         l_items + 1
@rts:
            rts

; IN: .A, .X = a cons.  OUT: .A, .X = its first item (ld_first) or the rest (ld_rest).  Keeps .Y
l_car:
            phy
            ldy         #0
            jsr         cell_get
            ply
            rts
l_cdr:
            phy
            ldy         #1
            jsr         cell_get
            ply
            rts

; ex, a number that's whole (a fixnum, as yet): l_n its value.  OUT: C = 1 and ex = the error "'fn': what must be a
; whole number, not x" if it isn't (fn: l_fn, what: .A, .X)
l_whole:
            sta         l_what
            stx         l_what + 1
            lda         ex
            lsr
            bcc         @not
            lda         ex
            ldx         ex + 1
            jsr         unfix
            sta         l_n
            stx         l_n + 1
            clc
            rts
@not:
            jsr         l_begin
            lda         #$27
            jsr         out_byte
            lda         l_fn
            ldx         l_fn + 1
            jsr         out_text
            LDAX        s_lwhole
            jsr         out_text
            lda         l_what
            ldx         l_what + 1
            jsr         out_text
            LDAX        s_lmustwhole
            jsr         out_text
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         print_val
            jsr         l_end
            sec
            rts

; ---- The built-ins

; (list x...): a list of the values (written here: this scope)
bl_list:
            jsr         lb_begin
            stz         l_i
            stz         l_i + 1
@value:
            lda         l_i
            cmp         cn
            lda         l_i + 1
            sbc         cn + 1
            bcs         @done
            lda         l_i
            asl
            sta         hq
            lda         l_i + 1
            rol
            sta         hq + 1
            clc
            lda         hq
            adc         argp
            sta         hq
            lda         hq + 1
            adc         argp + 1
            sta         hq + 1
            lda         (hq)
            sta         et
            ldy         #1
            lda         (hq),y
            sta         et + 1
            jsr         lb_add
            bcs         @rts
            inc         l_i
            bne         @value
            inc         l_i + 1
            bra         @value
@done:
            lda         ee
            ldx         ee + 1
            jmp         lb_done
@rts:
            rts

; ex, the argument 0, a Q-expression that isn't empty (head's, tail's ...: l_fn's).  OUT: C = 1 and ex = the error
; ("'head' expects a QExpr", "'head' passed an empty list") if it isn't
l_nonempty:
            jsr         l_arg0
            jsr         l_list
            bcc         :+
            jsr         l_begin
            jsr         l_quoted_fn
            LDAX        s_lexpects
            jsr         out_text
            bra         @end
:
            lda         l_items
            ora         l_items + 1
            bne         @ok
            jsr         l_begin
            jsr         l_quoted_fn
            LDAX        s_lempty
            jsr         out_text
@end:
            jsr         l_end
            sec
            rts
@ok:
            clc
            rts

; l_fn, in quotes, out
l_quoted_fn:
            lda         #$27
            jsr         out_byte
            lda         l_fn
            ldx         l_fn + 1
            jsr         out_text
            lda         #$27
            jmp         out_byte

; (head l): a list of its first item
bl_head:
            LDAX        n_head
            jsr         l_set_fn
            jsr         l_nonempty
            bcc         :+
            clc
            rts
:
            jsr         lb_begin
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            jsr         lb_add
            bcs         :+
            lda         l_scope
            ldx         l_scope + 1
            jmp         lb_done
:
            rts

; (tail l): all but its first item
bl_tail:
            LDAX        n_tail
            jsr         l_set_fn
            jsr         l_nonempty
            bcc         :+
            clc
            rts
:
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         ex
            stx         ex + 1
            jmp         l_scoped

; (init l): all but its last item
bl_init:
            LDAX        n_init
            jsr         l_set_fn
            jsr         l_nonempty
            bcc         :+
            clc
            rts
:
            jsr         lb_begin
@item:
            lda         l_items                             ; (Each but the last: copied)
            ldx         l_items + 1
            jsr         l_cdr
            cmp         #0
            bne         :+
            cpx         #0
            beq         @done
:
            pha
            phx
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            plx
            pla
            sta         l_items
            stx         l_items + 1
            jsr         lb_add
            bcc         @item
            rts
@done:
            lda         l_scope
            ldx         l_scope + 1
            jmp         lb_done

; (end l): a list of its last item
bl_end:
            LDAX        n_end
            jsr         l_set_fn
            jsr         l_nonempty
            bcc         :+
            clc
            rts
:
@item:
            lda         l_items                             ; (The last cons)
            ldx         l_items + 1
            jsr         l_cdr
            cmp         #0
            bne         :+
            cpx         #0
            beq         @last
:
            sta         l_items
            stx         l_items + 1
            bra         @item
@last:
            jsr         lb_begin
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            jsr         lb_add
            bcs         :+
            lda         l_scope
            ldx         l_scope + 1
            jmp         lb_done
:
            rts

; l_fn = .A, .X (a name, for errors)
l_set_fn:
            sta         l_fn
            stx         l_fn + 1
            rts

; (join l...): the lists' items in one list (the first scope among them, its)
bl_join:
            jsr         lb_begin
            stz         l_i
            stz         l_i + 1
            stz         l_scope2
            stz         l_scope2 + 1
@list:
            lda         l_i                                 ; (Each list: all but the last copied, the last shared)
            cmp         cn
            lda         l_i + 1
            sbc         cn + 1
            bcc         :+
            jmp         @done
:
            lda         l_i
            asl
            tay
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            jsr         l_list
            bcc         :+
            LDAX        s_ljoin
            jmp         l_error
:
            lda         l_scope2                            ; (Its scope, if none's been found)
            ora         l_scope2 + 1
            bne         :+
            MOVW        l_scope2, l_scope
:
            inc         l_i
            bne         :+
            inc         l_i + 1
:
            lda         l_i                                 ; (The last: shared)
            cmp         cn
            bne         @copy
            lda         l_i + 1
            cmp         cn + 1
            bne         @copy
            lda         cf
            ora         cf + 1
            bne         @link
            MOVW        cf, l_items
            bra         @done
@link:
            lda         cr
            ldx         cr + 1
            jsr         deref
            ldy         #2
            lda         l_items
            sta         (hp),y
            iny
            lda         l_items + 1
            sta         (hp),y
            bra         @done
@copy:
            lda         l_items
            ora         l_items + 1
            bne         @lb22
            jmp         @list
@lb22:
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            jsr         lb_add
            bcc         @copy
            rts
@done:
            lda         l_scope2
            ldx         l_scope2 + 1
            jmp         lb_done

; (len x): a list's items, a string's bytes
bl_len:
            jsr         l_arg0
            jsr         l_list
            bcs         @string
            ldx         #0                                  ; (Counted: to 16383)
            stx         l_n
            stx         l_n + 1
@item:
            lda         l_items
            ora         l_items + 1
            beq         @count
            inc         l_n
            bne         :+
            inc         l_n + 1
:
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @item
@count:
            lda         l_n
            ldx         l_n + 1
            jmp         l_fix
@string:
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         :+
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            jmp         l_fix
:
            LDAX        s_llen
            jmp         l_error

; ex = the number .A, .X (0 to 16383), a fixnum; C = 0
l_fix:
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            rts

; (item-at l i): item i (from 0), as it's written
bl_item_at:
            LDAX        n_item_at
            jsr         l_set_fn
            jsr         l_arg0
            jsr         l_list
            bcc         :+
            LDAX        s_litemat
            jmp         l_error
:
            jsr         l_arg1
            LDAX        s_lindex
            jsr         l_whole
            bcc         :+
            clc
            rts
:
            lda         l_n + 1
            bmi         @outside
@item:
            lda         l_items
            ora         l_items + 1
            beq         @outside
            lda         l_n
            ora         l_n + 1
            beq         @found
            lda         l_n
            bne         :+
            dec         l_n + 1
:
            dec         l_n
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @item
@found:
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         ex
            stx         ex + 1
            clc
            rts
@outside:
            jsr         l_begin                             ; ("'item-at': the list has no item 3")
            LDAX        s_lnoitem
            jsr         out_text
            jsr         l_arg1
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         print_val
            jsr         l_end
            rts

; (reverse x): a list's items, or a string's characters, the other way round
bl_reverse:
            jsr         l_arg0
            jsr         l_list
            bcs         @string
            stz         eu                                  ; (Each consed onto the reversed list: cf)
            stz         eu + 1
            stz         cf
            stz         cf + 1
@item:
            lda         l_items
            ora         l_items + 1
            beq         @done
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            MOVW        eu, cf
            lda         #PK_QCONS
            jsr         make2
            bcs         @rts
            MOVW        cf, hv
            bra         @item
@done:
            MOVW        ex, cf
            jmp         l_scoped
@string:
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_lreverse
            jmp         l_error
:
            jsr         cap_begin                           ; (Its bytes out, the last first: a string)
            sta         l_start
            stx         l_start + 1
            jsr         l_string
            lda         hn
            sta         l_n
            lda         hn + 1
            sta         l_n + 1
@byte:
            lda         l_n
            ora         l_n + 1
            beq         @made
            lda         l_n
            bne         :+
            dec         l_n + 1
:
            dec         l_n
            jsr         l_string
            lda         hp
            clc
            adc         l_n
            sta         hp
            lda         hp + 1
            adc         l_n + 1
            sta         hp + 1
            lda         (hp)
            jsr         out_byte
            bra         @byte
@made:
            lda         l_start
            ldx         l_start + 1
            jsr         cap_end
            bcs         @rts
            MOVW        ex, hv
@rts:
            rts

; ex, a string: hp = its bytes, hn = its length (its bank in the window)
l_string:
            lda         ex
            ldx         ex + 1
            jsr         deref
            lda         (hp)
            sta         l_len
            ldy         #1
            lda         (hp),y
            sta         l_len + 1
            iny
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            MOVW        hn, l_len
            rts

; (range to), (range from to [step]): the numbers from from (0) up to, not including, to, by step (1); down, with
; a negative step
bl_range:
            stz         l_from                              ; (from 0, step 1)
            stz         l_from + 1
            lda         #1
            sta         l_step
            stz         l_step + 1
            jsr         l_arg0
            jsr         l_num
            bcs         @notnum
            lda         cn
            cmp         #1
            bne         :+
            MOVW        l_to, l_n
            bra         @made
:
            MOVW        l_from, l_n
            jsr         l_arg1
            jsr         l_num
            bcs         @notnum
            MOVW        l_to, l_n
            lda         cn
            cmp         #3
            bne         @made
            jsr         l_arg2
            jsr         l_num
            bcs         @notnum
            MOVW        l_step, l_n
            lda         l_step
            ora         l_step + 1
            bne         @made
            LDAX        s_lstep0
            jmp         l_error
@notnum:
            LDAX        s_lrange
            jmp         l_error
@made:
            jsr         lb_begin
@next:
            lda         l_step + 1                          ; (Up: while from < to; down: while from > to)
            bmi         @down
            lda         l_from
            cmp         l_to
            lda         l_from + 1
            sbc         l_to + 1
            bvc         :+
            eor         #$80
:
            bpl         @done
            bra         @add
@down:
            lda         l_to
            cmp         l_from
            lda         l_to + 1
            sbc         l_from + 1
            bvc         :+
            eor         #$80
:
            bpl         @done
@add:
            lda         l_from
            ldx         l_from + 1
            jsr         fix_make
            bcs         @past
            MOVW        et, hv
            jsr         lb_add
            bcs         @rts
            clc
            lda         l_from
            adc         l_step
            sta         l_from
            lda         l_from + 1
            adc         l_step + 1
            sta         l_from + 1
            bvc         @next
@done:
            lda         #0
            tax
            jmp         lb_done
@past:
            LDAX        s_lnumpast
            jmp         l_error
@rts:
            rts

; ex, a number (a fixnum, as yet): l_n its value.  OUT: C = 1 if it isn't one
l_num:
            lda         ex
            lsr
            bcc         @no
            lda         ex
            ldx         ex + 1
            jsr         unfix
            sta         l_n
            stx         l_n + 1
            clc
            rts
@no:
            sec
            rts

; (cons x l): a list of x, then l's items
bl_cons:
            jsr         l_arg1
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcc         :+
            LDAX        s_lcons
            jmp         l_error
:
            sta         eu
            stx         eu + 1
            jsr         l_arg0
            MOVW        et, ex
            lda         #PK_QCONS
            jsr         make2
            bcs         :+
            MOVW        ex, hv
:
            rts

; (fst l), (snd l), (thd l), (nth n l), (last l): the item, as it's written; past the list's end, an error
bl_fst:
            LDAX        n_fst
            ldy         #0
            bra         l_nth_y
bl_snd:
            LDAX        n_snd
            ldy         #1
            bra         l_nth_y
bl_thd:
            LDAX        n_thd
            ldy         #2
l_nth_y:
            jsr         l_set_fn
            sty         l_n
            stz         l_n + 1
            jsr         l_arg0
            bra         l_nth
bl_nth:
            LDAX        n_nth
            jsr         l_set_fn
            jsr         l_arg0
            LDAX        s_lindex
            jsr         l_whole
            bcc         :+
            clc
            rts
:
            jsr         l_arg1
; ex, a list: its item l_n (l_fn's)
l_nth:
            jsr         l_list
            bcc         :+
            jsr         l_begin                             ; ("'fst' expects a list")
            jsr         l_quoted_fn
            LDAX        s_lalist
            jsr         out_text
            jmp         l_end
:
            lda         l_n + 1
            bmi         @none
@item:
            lda         l_items
            ora         l_items + 1
            beq         @none
            lda         l_n
            ora         l_n + 1
            beq         @found
            lda         l_n
            bne         :+
            dec         l_n + 1
:
            dec         l_n
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @item
@found:
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         ex
            stx         ex + 1
            clc
            rts
@none:
            LDAX        s_lnosuch
            jmp         l_error
bl_last:
            LDAX        n_last
            jsr         l_set_fn
            jsr         l_arg0
            jsr         l_list
            bcc         :+
            jmp         l_nth
:
            lda         l_items                             ; (Its last: its length less one)
            ldx         l_items + 1
            jsr         count_list
            sec
            sbc         #1
            sta         l_n
            lda         #0
            sbc         #0
            sta         l_n + 1
            jmp         l_nth

; (take n l), (drop n l): the first n items, or all but them (n a count; past the end, all of them)
bl_take:
            LDAX        n_take
            ldy         #0
            bra         l_take_drop
bl_drop:
            LDAX        n_drop
            ldy         #1
l_take_drop:
            sty         l_drop
            jsr         l_set_fn
            jsr         l_arg0
            LDAX        s_lcount
            jsr         l_whole
            bcc         :+
            clc
            rts
:
            lda         l_n + 1
            bpl         :+
            jsr         l_begin                             ; ("'take': a count can't be negative")
            jsr         l_quoted_fn
            LDAX        s_lnegative
            jsr         out_text
            jmp         l_end
:
            jsr         l_arg1
            jsr         l_list
            bcc         :+
            jsr         l_begin
            jsr         l_quoted_fn
            LDAX        s_lexpects2
            jsr         out_text
            jmp         l_end
:
            jsr         lb_begin
@item:
            lda         l_items
            ora         l_items + 1
            beq         @done
            lda         l_n
            ora         l_n + 1
            beq         @rest
            lda         l_n
            bne         :+
            dec         l_n + 1
:
            dec         l_n
            lda         l_drop                              ; (drop: past it)
            bne         @skip
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            jsr         lb_add
            bcc         @skip
            rts
@skip:
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @item
@rest:
            lda         l_drop                              ; (drop: the rest, shared)
            beq         @done
            MOVW        ex, l_items
            clc
            rts
@done:
            lda         l_drop
            beq         :+
            stz         ex
            stz         ex + 1
            clc
            rts
:
            lda         #0
            tax
            jmp         lb_done

; ex, a string: hv = its characters, a list (each's).  OUT: C = 1 if there's no room
str_chars:
            jsr         lb_begin
            stz         l_i
            stz         l_i + 1
@byte:
            jsr         l_string
            lda         l_i
            cmp         hn
            lda         l_i + 1
            sbc         hn + 1
            bcs         @done
            lda         hp                                  ; (Its byte l_i: a character)
            clc
            adc         l_i
            sta         hp
            lda         hp + 1
            adc         l_i + 1
            sta         hp + 1
            lda         (hp)
            stz         et + 1
            asl
            sta         et
            rol         et + 1
            lda         et + 1
            ora         #>CHAR0
            sta         et + 1
            jsr         lb_add
            bcs         @rts
            inc         l_i
            bne         @byte
            inc         l_i + 1
            bra         @byte
@done:
            MOVW        hv, cf
            clc
@rts:
            rts

.bss
l_start:    .res        2                                   ; (A capture's start)
l_scope:    .res        2                                   ; (A list's scope; join's, the first found)
l_scope2:   .res        2
l_items:    .res        2                                   ; (A list's conses left)
l_fn:       .res        2                                   ; (The built-in's name, for errors)
l_what:     .res        2
l_n:        .res        2                                   ; (A number, an index, a count)
l_i:        .res        2
l_len:      .res        2
l_from:     .res        2                                   ; (range's)
l_to:       .res        2
l_step:     .res        2
l_drop:     .res        1                                   ; (take's 0, drop's 1)

.segment "RODATA2"
n_head:     .byte       "head", 0
n_tail:     .byte       "tail", 0
n_init:     .byte       "init", 0
n_end:      .byte       "end", 0
n_item_at:  .byte       "item-at", 0
n_fst:      .byte       "fst", 0
n_snd:      .byte       "snd", 0
n_thd:      .byte       "thd", 0
n_nth:      .byte       "nth", 0
n_last:     .byte       "last", 0
n_take:     .byte       "take", 0
n_drop:     .byte       "drop", 0
s_lexpects: .byte       " expects a QExpr", 0
s_lempty:   .byte       " passed an empty list", 0
s_ljoin:    .byte       "Invalid parameter passed to 'join'.  Expected QExpr.", 0
s_llen:     .byte       "'len' requires parameter of type string, list or hash", 0
s_litemat:  .byte       "First 'item-at' parameter must be a QExpr", 0
s_lindex:   .byte       "an index", 0
s_lcount:   .byte       "a count", 0
s_lwhole:   .byte       "': ", 0
s_lmustwhole: .byte     " must be a whole number, not ", 0
s_lnoitem:  .byte       "'item-at': the list has no item ", 0
s_lreverse: .byte       "'reverse' expects a QExpr or a String", 0
s_lrange:   .byte       "'range' expects Numbers", 0
s_lstep0:   .byte       "'range' step cannot be 0", 0
s_lnumpast: .byte       "Not yet: a number past a fixnum, or a fraction (phase 5)", 0
s_lcons:    .byte       "'cons' expects a list second", 0
s_lalist:   .byte       " expects a list", 0
s_lnosuch:  .byte       "'nth': the list has no such item", 0
s_lnegative: .byte      ": a count can't be negative", 0
s_lexpects2: .byte      " expects a list", 0
