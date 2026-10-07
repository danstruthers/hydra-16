; ****************************************************************************
; lists.inc - hylang's list built-ins (phases 3 and 4: sort's merges, for forms.inc's bi_sort, too), in its second
; bank: each as danlang's (Builtins.cs, LibraryBuiltins.cs), its errors' messages too.  Each gets its values at argp
; (cn of them) and gives back ex (C = 1 if there's no room).  A list made from one keeps the scope the first was
; written in (a scoped Q-expression's), as danlang's do; one made new has none.  cf and cr are free for them (roots):
; a list being made is cf (its first cons) and cr (its last).

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

; ex, a number that's whole (an integer, or a number equal to one: 2.0): l_n its value (past 16 bits, -32768 or
; 32767).  OUT: C = 1 and ex = the error "'fn': what must be a whole number, not x" if it isn't (fn: l_fn, what: .A, .X)
l_whole:
            sta         l_what
            stx         l_what + 1
            lda         ex                                  ; (A fixnum here; else numbi.inc's n_whole)
            lsr
            bcc         @far
            lda         ex
            ldx         ex + 1
            jsr         unfix
            bra         @whole
@far:
            lda         ex
            ldx         ex + 1
            FARN        3, n_whole
            bcs         @not
@whole:
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
            beq         :+
            cmp         #PK_BUFFER                          ; (A buffer: its bytes, as a string's)
            bne         :++
:
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            jmp         l_fix
:
            cmp         #PK_HASH                            ; (A hash: its count)
            bne         :+
            lda         ex
            ldx         ex + 1
            FARN        4, h_count_of
            sta         ex
            stx         ex + 1
            clc
            rts
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
            jmp         l_noitem                            ; ("'item-at': the list has no item 3")

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
            FARN        3, n_range                          ; (Past fixnums, or not numbers: numbi.inc's)
            rts
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
            FARN        3, n_range
            rts
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

; (subset l i [n]): n items from i (to the end without n; i may be the length: NIL), a list made new
bl_subset:
            LDAX        n_subset
            jsr         l_set_fn
            jsr         l_arg0
            jsr         l_list
            bcc         :+
            LDAX        s_lsubset
            jmp         l_error
:
            stz         l_len                               ; (Its items counted)
            stz         l_len + 1
            lda         l_items
            ldx         l_items + 1
@count:
            sta         l_from
            stx         l_from + 1
            ora         l_from + 1
            beq         @counted
            inc         l_len
            bne         :+
            inc         l_len + 1
:
            lda         l_from
            ldx         l_from + 1
            jsr         l_cdr
            bra         @count
@counted:
            jsr         l_arg1
            LDAX        s_lindex
            jsr         l_whole
            bcc         :+
            clc
            rts
:
            lda         l_n + 1                             ; (From 0 to its length)
            bmi         @outside
            lda         l_len
            cmp         l_n
            lda         l_len + 1
            sbc         l_n + 1
            bcs         :+
@outside:
            jmp         l_noitem
:
            MOVW        l_from, l_n
            lda         l_len                               ; (The items from there)
            sec
            sbc         l_n
            sta         l_len
            lda         l_len + 1
            sbc         l_n + 1
            sta         l_len + 1
            lda         cn
            cmp         #3
            bcc         @take
            jsr         l_arg2
            LDAX        s_lcount
            jsr         l_whole
            bcc         :+
            clc
            rts
:
            lda         l_n + 1
            bpl         :+
            jsr         l_begin                             ; ("'subset': a count can't be negative")
            jsr         l_quoted_fn
            LDAX        s_lnegative
            jsr         out_text
            jmp         l_end
:
            lda         l_n                                 ; (n of them, if it's fewer)
            cmp         l_len
            lda         l_n + 1
            sbc         l_len + 1
            bcs         @take
            MOVW        l_len, l_n
@take:
            jsr         lb_begin
@skip:
            lda         l_from
            ora         l_from + 1
            beq         @item
            lda         l_from
            bne         :+
            dec         l_from + 1
:
            dec         l_from
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @skip
@item:
            lda         l_len
            ora         l_len + 1
            beq         @done
            lda         l_len
            bne         :+
            dec         l_len + 1
:
            dec         l_len
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            jsr         lb_add
            bcs         @rts
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @item
@done:
            lda         #0
            tax
            jmp         lb_done
@rts:
            rts

; The error "'item-at': the list has no item 9" (l_fn's; the argument 1 as the index)
l_noitem:
            jsr         l_begin
            jsr         l_quoted_fn
            LDAX        s_lnoitem
            jsr         out_text
            jsr         l_arg1
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         print_val
            jmp         l_end

; (index-of s sub), (last-index-of s sub): where sub (a string or a character) is first, or last, in the string s, or
; -1; for a list, where an item equal to sub is
bl_index_of:
            LDAX        n_index_of
            ldy         #0
            bra         l_index
bl_last_index_of:
            LDAX        n_last_index_of
            ldy         #1
l_index:
            sty         l_drop                              ; (0: the first; 1: the last)
            jsr         l_set_fn
            lda         #<FIX(-1)                           ; (None yet: -1)
            sta         l_found
            lda         #>FIX(-1)
            sta         l_found + 1
            stz         l_i
            stz         l_i + 1
            jsr         l_arg0
            jsr         l_list
            bcs         @string
@item:
            lda         l_items                             ; (A list: an item equal to sub)
            ora         l_items + 1
            beq         @done
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         q_a
            stx         q_a + 1
            ldy         #2
            lda         (argp),y
            sta         q_b
            iny
            lda         (argp),y
            sta         q_b + 1
            jsr         q_equal
            bcc         :+
            jsr         l_at_i
            lda         l_drop
            beq         @done
:
            inc         l_i
            bne         :+
            inc         l_i + 1
:
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @item
@done:
            MOVW        ex, l_found
            clc
            rts
@string:
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         @bad
            jsr         l_arg1                              ; (sub: a string's bytes, or a character's)
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @sub
            lda         ex + 1
            and         #$FE
            cmp         #>CHAR0
            bne         @bad
            lda         ex + 1
            lsr
            lda         ex
            ror
            sta         l_ch
            lda         #<l_ch
            sta         l_pb
            lda         #>l_ch
            sta         l_pb + 1
            lda         #1
            sta         l_lb
            stz         l_lb + 1
            bra         @search
@bad:
            jsr         l_begin                             ; ("'index-of' expects a String and a String")
            jsr         l_quoted_fn
            LDAX        s_lstrstr
            jsr         out_text
            jmp         l_end
@sub:
            jsr         l_string
            MOVW        l_pb, hp
            MOVW        l_lb, hn
            lda         $00
            sta         l_kb
@search:
            jsr         l_arg0                              ; (s: its bytes)
            jsr         l_string
            MOVW        l_pa, hp
            lda         $00
            sta         l_ka
            lda         hn                                  ; (Where sub may be: 0 to s's length less sub's)
            sec
            sbc         l_lb
            sta         l_n
            lda         hn + 1
            sbc         l_lb + 1
            sta         l_n + 1
            bcs         @lb107
            jmp         @done
@lb107:
            lda         l_drop
            beq         @at
            MOVW        l_i, l_n
@at:
            jsr         l_match
            bcc         :+
            jsr         l_at_i
            jmp         @done
:
            lda         l_drop
            bne         @down
            lda         l_i
            cmp         l_n
            bne         :+
            lda         l_i + 1
            cmp         l_n + 1
            bne         @lb106
            jmp         @done
@lb106:
:
            inc         l_i
            bne         @at
            inc         l_i + 1
            bra         @at
@down:
            lda         l_i
            ora         l_i + 1
            bne         @lb105
            jmp         @done
@lb105:
            lda         l_i
            bne         :+
            dec         l_i + 1
:
            dec         l_i
            bra         @at

; l_found = l_i, a fixnum
l_at_i:
            lda         l_i
            ldx         l_i + 1
            jsr         fix_ax
            sta         l_found
            stx         l_found + 1
            rts

; Is sub (l_pb, l_lb bytes, in bank l_kb) at s's byte l_i (l_pa, in bank l_ka)?  OUT: C = 1 if it is
l_match:
            lda         l_pa
            clc
            adc         l_i
            sta         q_pa
            lda         l_pa + 1
            adc         l_i + 1
            sta         q_pa + 1
            MOVW        q_pb, l_pb
            MOVW        l_m, l_lb
            ldy         #0
@byte:
            lda         l_m
            ora         l_m + 1
            beq         @yes
            lda         l_kb
            sta         $00
            lda         (q_pb),y
            sta         l_ch2
            lda         l_ka
            sta         $00
            lda         (q_pa),y
            cmp         l_ch2
            bne         @no
            iny
            bne         :+
            inc         q_pa + 1
            inc         q_pb + 1
:
            lda         l_m
            bne         :+
            dec         l_m + 1
:
            dec         l_m
            bra         @byte
@yes:
            sec
            rts
@no:
            clc
            rts

; ---- sort (forms.inc's bi_sort runs it): a merge sort.  Each item is made a run of one (a cons of its own); the runs
; are merged a pair at a time (a's and b's), into out, a pass at a time (the merged runs, next, the runs of the next
; pass), till one is left: stable, as an item of b goes before a's only if it's less.  The conses are its own, so
; they're linked again, not made: the frame (bi_sort's, its code on top) is its state
SS_LESS         = 0             ; less, or NIL (by cmp)
SS_RUNS         = 2             ; The runs left in this pass (a list of them)
SS_NEXT         = 4             ; The next pass's runs (its first cons, and its last: the merge's run goes there)
SS_NLAST        = 6
SS_A            = 8             ; The two runs being merged: their items left
SS_B            = 10
SS_OUT          = 12            ; The merged run: its first cons, and its last
SS_OLAST        = 14
SS_SIZE         = 16

; (sort l [less]) begun: each item a run of one, the runs a list.  OUT: C = 1 if there's no room; else the arguments
; dropped, and .A = 0 (Z = 1): ex the value (an error, a list of one item or none); or .A = 1: the frame pushed
; (bi_sort pushes its code)
sort_begin:
            LDAX        n_sort
            jsr         l_set_fn
            jsr         l_arg0
            jsr         l_list
            bcc         :+
            jsr         l_begin                             ; ("'sort' expects a QExpr")
            jsr         l_quoted_fn
            LDAX        s_lexpects
            jsr         out_text
            jsr         l_end
            bcc         @lb104
            jmp         @rts
@lb104:
            jmp         @value
:
            stz         l_less
            stz         l_less + 1
            lda         cn
            cmp         #2
            bcc         @runs
            jsr         l_arg1
            MOVW        l_less, ex
            jsr         l_isfn
            bcc         @runs
            LDAX        s_lsortfn
            jsr         l_error
            bcs         @rts
            bra         @value
@runs:
            jsr         lb_begin
@item:
            lda         l_items
            ora         l_items + 1
            beq         @made
            lda         l_items
            ldx         l_items + 1
            jsr         l_car
            sta         et
            stx         et + 1
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         make2                               ; (Its run)
            bcs         @rts
            MOVW        et, hv
            jsr         lb_add
            bcs         @rts
            lda         l_items
            ldx         l_items + 1
            jsr         l_cdr
            sta         l_items
            stx         l_items + 1
            bra         @item
@made:
            stz         ex
            stz         ex + 1
            lda         cf
            ora         cf + 1
            beq         @value                              ; (No items: NIL)
            lda         cf
            ldx         cf + 1
            jsr         l_cdr
            stx         l_m
            ora         l_m
            bne         @frame
            lda         cf                                  ; (One: its run)
            ldx         cf + 1
            jsr         l_car
            sta         ex
            stx         ex + 1
@value:
            stz         ev_inargs
            jsr         drop_args
            lda         #0
            clc
@rts:
            rts
@frame:
            stz         ev_inargs
            jsr         drop_args
            PUSHV       l_less
            PUSHV       cf
            lda         #0
            tax
            ldy         #SS_SIZE / 2 - 2
:
            jsr         push
            dey
            bne         :-
            lda         #1
            clc
            rts

; ex a function (a built-in, a function, one partially applied)?  OUT: C = 0 if it is
l_isfn:
            lda         ex
            lsr
            bcs         @no
            ldx         ex + 1
            cpx         #>BUILTIN0
            bcc         @no
            cpx         #IMM_PAGES
            bcc         @yes
            lda         pk,x
            cmp         #PK_FUNC
            beq         @yes
            cmp         #PK_PARTIAL
            beq         @yes
@no:
            sec
            rts
@yes:
            clc
            rts

; The sort's next step: .A = what the last comparison said (0: a's first item goes first; 1: b's; $FF: none yet).
; Merged till a comparison is wanted that less makes (one by cmp is made here), or it's sorted.  OUT: .A = 0 (Z = 1):
; ex the sorted list, the frame dropped (its code too); .A = 1: less wanted (cf) of q_a (b's first item) and q_b (a's)
sort_step:
            cmp         #$FF
            beq         @step
            ldy         #SS_A
            cmp         #0
            beq         :+
            ldy         #SS_B
:
            jsr         ss_at
            jsr         ss_get                              ; (Its first cons taken from its run ...
            sta         ss_c
            stx         ss_c + 1
            jsr         l_cdr
            jsr         ss_put
            ldy         #SS_OUT                             ;   and put at out's end)
            jsr         ss_get
            bne         :+
            lda         ss_c
            ldx         ss_c + 1
            jsr         ss_put
            bra         @olast
:
            lda         ss_c
            ldx         ss_c + 1
            ldy         #SS_OLAST
            jsr         ss_link
@olast:
            lda         ss_c
            ldx         ss_c + 1
            ldy         #SS_OLAST
            jsr         ss_put
@step:
            jsr         ss_at
            ldy         #SS_A                               ; (Both runs have items: their firsts compared)
            jsr         ss_get
            beq         @end
            jsr         l_car
            sta         q_b
            stx         q_b + 1
            ldy         #SS_B
            jsr         ss_get
            beq         @end
            jsr         l_car
            sta         q_a
            stx         q_a + 1
            ldy         #SS_LESS
            jsr         ss_get
            beq         @cmp
            sta         cf
            stx         cf + 1
            lda         #1
            rts
@cmp:
            jsr         q_order                             ; (By cmp: b's first goes first if it's less)
            lda         q_r
            cmp         #$FF
            beq         :+
            lda         #0
            jmp         sort_step
:
            lda         #1
            jmp         sort_step
@end:
            ldy         #SS_OUT                             ; (A merge's end: the rest of a or b after out, the
            jsr         ss_get                              ;   merged run the first of next's last cons)
            beq         @pair
            ldy         #SS_A
            jsr         ss_get
            bne         :+
            ldy         #SS_B
            jsr         ss_get
:
            ldy         #SS_OLAST
            jsr         ss_link
            ldy         #SS_NLAST
            jsr         ss_get
            sta         hv
            stx         hv + 1
            ldy         #SS_OUT
            jsr         ss_get
            ldy         #0
            jsr         cell_set
            lda         #0
            tax
            ldy         #SS_A
            jsr         ss_put
            ldy         #SS_B
            jsr         ss_put
            ldy         #SS_OUT
            jsr         ss_put
            ldy         #SS_OLAST
            jsr         ss_put
@pair:
            ldy         #SS_RUNS                            ; (The next two runs: a and b; their first cons put in
            jsr         ss_get                              ;   next, for the merged run)
            beq         @pass
            sta         ss_c
            stx         ss_c + 1
            jsr         l_cdr
            sta         ss_d
            stx         ss_d + 1
            ora         ss_d + 1
            beq         @one
            lda         ss_c
            ldx         ss_c + 1
            jsr         l_car
            ldy         #SS_A
            jsr         ss_put
            lda         ss_d
            ldx         ss_d + 1
            jsr         l_car
            ldy         #SS_B
            jsr         ss_put
            lda         ss_d
            ldx         ss_d + 1
            jsr         l_cdr
            ldy         #SS_RUNS
            jsr         ss_put
            jsr         ss_next
            jmp         @step
@one:
            ldy         #SS_RUNS                            ; (One left: on to next as it is)
            lda         #0
            tax
            jsr         ss_put
            jsr         ss_next
@pass:
            ldy         #SS_NEXT                            ; (The pass's end: one run, sorted; or another pass)
            jsr         ss_get
            sta         ss_c
            stx         ss_c + 1
            jsr         l_cdr
            stx         ss_d
            ora         ss_d
            bne         @again
            lda         ss_c
            ldx         ss_c + 1
            jsr         l_car
            sta         ex
            stx         ex + 1
            lda         #SS_SIZE / 2 + 1
            jsr         drop
            lda         #0
            rts
@again:
            lda         ss_c
            ldx         ss_c + 1
            ldy         #SS_RUNS
            jsr         ss_put
            lda         #0
            tax
            ldy         #SS_NEXT
            jsr         ss_put
            ldy         #SS_NLAST
            jsr         ss_put
            jmp         @pair

; hq = the sort's frame (its code on top)
ss_at:
            sec
            lda         sp
            sbc         #SS_SIZE + 2
            sta         hq
            lda         sp + 1
            sbc         #0
            sta         hq + 1
            rts

; The frame's word .Y: .A, .X; Z = 1 if it's NIL.  Keeps .Y
ss_get:
            iny
            lda         (hq),y
            tax
            dey
            lda         (hq),y
            bne         :+
            cpx         #0
:
            rts

; .A, .X put in the frame's word .Y.  Keeps .Y
ss_put:
            sta         (hq),y
            iny
            txa
            sta         (hq),y
            dey
            rts

; .A, .X put as the rest of the cons in the frame's word .Y
ss_link:
            pha
            phx
            jsr         ss_get
            sta         hv
            stx         hv + 1
            plx
            pla
            ldy         #1
            jmp         cell_set

; ss_c, a run's cons (its rest made NIL), put at next's end
ss_next:
            MOVW        hv, ss_c
            lda         #0
            tax
            ldy         #1
            jsr         cell_set
            ldy         #SS_NEXT
            jsr         ss_get
            bne         :+
            lda         ss_c
            ldx         ss_c + 1
            jsr         ss_put
            bra         @last
:
            lda         ss_c
            ldx         ss_c + 1
            ldy         #SS_NLAST
            jsr         ss_link
@last:
            lda         ss_c
            ldx         ss_c + 1
            ldy         #SS_NLAST
            jmp         ss_put

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
l_drop:     .res        1                                   ; (take's 0, drop's 1; index-of's 0, last-index-of's 1)
l_found:    .res        2                                   ; (index-of's: where it's found; sub's bytes, their
l_pb:       .res        2                                   ;   length and bank, or its character; s's bytes and
l_lb:       .res        2                                   ;   bank; bytes left to compare, the byte)
l_kb:       .res        1
l_ch:       .res        1
l_pa:       .res        2
l_ka:       .res        1
l_m:        .res        2
l_ch2:      .res        1
l_less:     .res        2                                   ; (sort's: less; conses)
ss_c:       .res        2
ss_d:       .res        2

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
n_subset:   .byte       "subset", 0
n_index_of: .byte       "index-of", 0
n_last_index_of: .byte  "last-index-of", 0
n_sort:     .byte       "sort", 0
s_lexpects: .byte       " expects a QExpr", 0
s_lempty:   .byte       " passed an empty list", 0
s_ljoin:    .byte       "Invalid parameter passed to 'join'.  Expected QExpr.", 0
s_llen:     .byte       "'len' requires parameter of type string, list, hash or buffer", 0
s_litemat:  .byte       "First 'item-at' parameter must be a QExpr", 0
s_lindex:   .byte       "an index", 0
s_lcount:   .byte       "a count", 0
s_lwhole:   .byte       "': ", 0
s_lmustwhole: .byte     " must be a whole number, not ", 0
s_lnoitem:  .byte       ": the list has no item ", 0
s_lreverse: .byte       "'reverse' expects a QExpr or a String", 0
s_lstep0:   .byte       "'range' step cannot be 0", 0
s_lcons:    .byte       "'cons' expects a list second", 0
s_lalist:   .byte       " expects a list", 0
s_lnosuch:  .byte       "'nth': the list has no such item", 0
s_lnegative: .byte      ": a count can't be negative", 0
s_lexpects2: .byte      " expects a list", 0
s_lsubset:  .byte       "First 'subset' parameter must be a QExpr", 0
s_lstrstr:  .byte       " expects a String and a String", 0
s_lsortfn:  .byte       "Second 'sort' parameter must be a function", 0
