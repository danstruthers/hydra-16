; ****************************************************************************
; hashes.inc - hylang's hashes (phase 6), in its fourth bank, as danlang's (LHash.cs): keys (atoms, strings,
; integers: 3.0 is 3), values, and tags (atoms) on the hash and on its entries; the reserved tags :__locked (no entry
; added or taken out), :__read-only (nothing changed), :__private (an entry only the hash's methods reach) and
; :__not_nil (an entry that can't be NIL).
;   A hash (PK_HASH) is its items: its entries, each a list {key value tag...} never changed in place (a changed one
; is a new list in its place), in the order they were put; then its own tags.  So from# and the printer have it as
; it is.  Its second word: a proxy's hash (a proxy is &0, the hash as its methods see it, every entry reached: flag
; H_PROXY); its third, its count of entries.  A key is found by a walk along the items: a hash is small (danlang's
; objects).
;   Each entry from another bank sets an abort point (h_enter): no room for what's made goes back to it (h_nomem), the
; root stack as it was, C = 1.  An operation's value is ex: what it gives, or an error.

H_ITEMS         = 0             ; (A hash's words: its items, a proxy's hash, its count, its flags)
H_REAL          = 1
H_COUNT         = 2
H_FLAGS         = 3
H_PROXY         = $01
H_DEEP          = 8             ; (Hashes in hashes cloned, or listed, this deep; deeper, as they are)
TG_LOCKED       = 0             ; (The reserved tags: their places in h_tags)
TG_RO           = 2
TG_PRIV         = 4
TG_NOTNIL       = 6
HM_CREATE       = 0             ; (hash-create's, to#'s and hash-put's frame: its mode ...
HM_TO           = 1
HM_PUT          = 2
HS_NEXT         = 0             ;   its state: the next item; an entry evaluated; its value evaluated ...
HS_ENTRY        = 1
HS_VALUE        = 2
HS_SEVERAL      = 4             ;   hash-put's of several entries: a list of what each gave
HF_EE           = 0             ;   and its words, from its first: [ee hash items entry state first last])
HF_HASH         = 2
HF_ITEMS        = 4
HF_ENTRY        = 6
HF_STATE        = 8
HF_FIRST        = 10
HF_LAST         = 12
HF_SIZE         = 14

.bss
h_h:        .res        2                                   ; The hash (a proxy's: its own) ...
h_mem:      .res        1                                   ;   <> 0: reached by a member (through a proxy)
h_called:   .res        2                                   ; The hash called (a method's &0, if it's a proxy)
h_k:        .res        2                                   ; A key
h_prev:     .res        2                                   ; (h_find's: the items' cons before the entry's, the
h_cell:     .res        2                                   ;   entry's, the entry)
h_ent:      .res        2
h_v:        .res        2                                   ; (A value; a tag; lists walked; a cons made)
h_tg:       .res        2
h_t:        .res        2
h_e:        .res        2
h_w:        .res        2
h_nc:       .res        2
h_depth:    .res        1                                   ; (Hashes in hashes, cloned or listed: how deep)
h_n:        .res        1                                   ; (A count: a built-in's own)
h_start:    .res        2                                   ; (An error's capture: where it began)
h_mode:     .res        1                                   ; (hash-create's, to#'s, hash-put's: HM_*; the frame's
h_st:       .res        1                                   ;   state; the first step)
h_first:    .res        1
h_sp:       .res        1                                   ; The abort point: the 65C02's stack, the root stack,
h_rsp:      .res        1                                   ;   the captures open and their length
h_cap:      .res        1
h_clen:     .res        2

PSTATE_SEG
h_tags:     .res        8                                   ; The reserved tags' atoms (0: not made yet)
h_amp0:     .res        2                                   ; The symbol &0

.segment "CODE4"

; ---- Abort point, helpers

; An entry's abort point (first thing in it: jsr h_enter); then the reserved tags and &0 made, if they aren't yet
h_enter:
            tsx
            inx
            inx
            stx         h_sp
            lda         rsp
            sta         h_rsp
            lda         cap_on
            sta         h_cap
            lda         cap_len
            sta         h_clen
            lda         cap_len + 1
            sta         h_clen + 1
            lda         h_tags + 1
            ora         h_tags
            bne         @done
            ldx         #0
@tag:
            phx
            lda         tg_lo,x
            sta         hq
            lda         tg_hi,x
            sta         hq + 1
            lda         tg_len,x
            sta         hn
            stz         hn + 1
            lda         #PK_ATOM
            jsr         intern
            plx
            bcs         h_nomem
            txa
            asl
            tay
            lda         hv
            sta         h_tags,y
            lda         hv + 1
            sta         h_tags + 1,y
            inx
            cpx         #4
            bcc         @tag
            LDHQ        s_amp0, 2                           ; (&0: a name bound in a frame, so looked for there)
            lda         #PK_SYMBOL
            jsr         intern
            bcs         h_nomem
            MOVW        h_amp0, hv
            lda         hv
            ldx         hv + 1
            jsr         deref
            ldy         #6
            lda         (hp),y
            ora         #SF_LOCAL
            sta         (hp),y
@done:
            rts

; No room: back from the entry, C = 1, the root stack and the captures as they were
h_nomem:
            ldx         h_sp
            txs
            lda         h_rsp
            sta         rsp
            lda         h_cap
            sta         cap_on
            lda         h_clen
            sta         cap_len
            lda         h_clen + 1
            sta         cap_len + 1
            sec
            rts

; make2 (h_make2), root_push (h_rpush), cell_alloc (h_alloc): no room, back from the entry
h_make2:
            jsr         make2
            bcs         h_nomem
            rts
h_rpush:
            jsr         root_push
            bcs         h_nomem
            rts
h_alloc:
            jsr         cell_alloc
            bcs         h_nomem
            rts

; An error's message begun (h_end_msg: ended, ex the error, C = 0)
h_begin_msg:
            jsr         cap_begin
            sta         h_start
            stx         h_start + 1
            rts
h_end_msg:
            lda         h_start
            ldx         h_start + 1
            jsr         cap_end
            bcs         h_nomem
            lda         hv
            ldx         hv + 1
            jsr         kind_of
            cmp         #PK_ERROR
            beq         :+
            jsr         error_wrap
            bcs         h_nomem
:
            MOVW        ex, hv
            clc
            rts

; ex = the error of the zero-terminated message at .A, .X (in this bank); C = 0
h_error:
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
            bcs         h_nomem
            MOVW        ex, hv
            clc
            rts

; ex = the error of the message .A, .X and h_v's repr after it ("A tag must be an atom, not 2"); C = 0
h_error_v:
            pha
            phx
            jsr         h_begin_msg
            plx
            pla
            jsr         out_text
            lda         h_v
            ldx         h_v + 1
            jsr         h_repr
            jmp         h_end_msg

; The value .A, .X out as the REPL has it
h_repr:
            ldy         #0
            FARN        2, print_val
            rts

; ex = T if C = 0 (h_bool_nc) or C = 1 (h_bool), else NIL; C = 0
h_bool_nc:
            lda         #0
            bcs         h_set_a
            lda         #<T_VAL
            bra         h_set_a
h_bool:
            lda         #0
            bcc         h_set_a
            lda         #<T_VAL
h_set_a:
            sta         ex
            stz         ex + 1
            clc
            rts

; OUT: C = 1 if ex is an error (eval.inc's is_err, in the first bank)
h_is_err:
            lda         ex
            lsr
            bcs         @no
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_ERROR
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

; IN: .A, .X = a value.  OUT: C = 0 if it's a hash (a proxy too).  Keeps .A, .X
h_is:
            sta         h_nc
            lsr
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_HASH
            beq         @yes
@no:
            lda         h_nc
            sec
            rts
@yes:
            lda         h_nc
            clc
            rts

; IN: .A, .X = a hash.  OUT: h_h = it, or a proxy's hash (h_mem $FF: reached by a member)
h_open:
            sta         h_h
            stx         h_h + 1
            stz         h_mem
            ldy         #H_FLAGS
            jsr         cell_get
            and         #H_PROXY
            beq         @rts
            dec         h_mem
            lda         h_h
            ldx         h_h + 1
            ldy         #H_REAL
            jsr         cell_get
            sta         h_h
            stx         h_h + 1
@rts:
            rts

; .A, .X = h_h's word .Y
h_word:
            lda         h_h
            ldx         h_h + 1
            jmp         cell_get

; h_h's word .Y = .A, .X
h_setw:
            pha
            MOVW        hv, h_h
            pla
            jmp         cell_set

; h_h's count, one more (h_count1) or one less (h_count0): its fixnum's value 2 more or less
h_count1:
            lda         #2
            bra         h_count
h_count0:
            lda         #$FE
h_count:
            pha
            ldy         #H_COUNT
            jsr         h_word
            sta         h_nc
            stx         h_nc + 1
            pla
            pha
            clc
            adc         h_nc
            sta         h_nc
            pla
            bmi         @less
            bcc         @set
            inc         h_nc + 1
            bra         @set
@less:
            bcs         @set                                ; (Less: a borrow, the high byte one less)
            dec         h_nc + 1
@set:
            lda         h_nc
            ldx         h_nc + 1
            ldy         #H_COUNT
            jmp         h_setw

; .A, .X put at the end of the list made (cf its first cons, cr its last)
h_append:
            sta         et
            stx         et + 1
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         h_make2
            lda         cf
            ora         cf + 1
            bne         :+
            MOVW        cf, hv
            MOVW        cr, hv
            rts
:
            MOVW        h_nc, hv
            MOVW        hv, cr
            lda         h_nc
            ldx         h_nc + 1
            ldy         #1
            jsr         cell_set
            MOVW        cr, h_nc
            rts

; hv = the list {et eu}
h_pair:
            lda         et
            pha
            lda         et + 1
            pha
            MOVW        et, eu
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         h_make2
            pla
            sta         et + 1
            pla
            sta         et
            MOVW        eu, hv
            lda         #PK_QCONS
            jmp         h_make2

; The list .A, .X: .A = its items counted (to 255), h_w its conses.  OUT: C = 1 if it isn't a list
h_items:
            jsr         q_conses
            bcs         @rts
            sta         h_w
            stx         h_w + 1
            jsr         count_list
            clc
@rts:
            rts

; .A, .X = the list h_w's item .Y (0, 1)
h_nth:
            phy
            lda         h_w
            ldx         h_w + 1
@next:
            ply
            beq         @here
            dey
            phy
            ldy         #1
            jsr         cell_get
            bra         @next
@here:
            ldy         #0
            jmp         cell_get

; .A, .X = the list h_w's rest past its item .Y (1: its second's rest)
h_rest:
            lda         h_w
            ldx         h_w + 1
@next:
            phy
            ldy         #1
            jsr         cell_get
            ply
            dey
            bpl         @next
            rts

; ---- Keys

; The value .A, .X, a key: an atom (not a reserved tag's name), a string or an integer (or a number equal to one: as
; that integer).  OUT: C = 0 and h_k = it; C = 1 and ex = the error
h_key:
            jsr         h_key0
            bcs         @rts
            jsr         h_reserved
            bcs         @reserved
            clc
@rts:
            rts
@reserved:
            phx
            jsr         h_begin_msg                         ; ("Cannot get/set value for reserved hash key __locked")
            LDAX        s_hreserved
            jsr         out_text
            pla
            lsr
            tax
            lda         tg_hi,x
            pha
            lda         tg_lo,x
            plx
            jsr         out_text
            jsr         h_end_msg
            sec
            rts

; As h_key, but a reserved tag's name too (none is a key in a hash: h_find finds none)
h_key0:
            sta         h_k
            stx         h_k + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         @ok
            cmp         #PK_STRING
            beq         @ok
            lda         h_k
            ldx         h_k + 1
            FARN        3, n_whole                          ; (A number: an integer, or one equal to it)
            bcs         @num
            lda         h_k
            ldx         h_k + 1
            jsr         kind_of
            cmp         #PK_FIXED
            bne         @ok
            lda         h_k                                 ; (2.0: its digits, 2)
            ldx         h_k + 1
            ldy         #0
            jsr         cell_get
            sta         h_k
            stx         h_k + 1
@ok:
            clc
            rts
@num:
            lda         h_k
            ldx         h_k + 1
            FARN        3, n_kind
            cmp         #NK_NOT
            beq         @type
            MOVW        h_v, h_k                            ; ("A hash key must be an atom, a string or an integer,
            LDAX        s_hkeynum                           ;   not 1.5")
            jsr         h_error_v
            sec
            rts
@type:
            jsr         h_begin_msg                         ; ("Unsupported key type Q-Expression")
            LDAX        s_hkeytype
            jsr         out_text
            MOVW        ex, h_k
            FARN        3, out_type
            jsr         h_end_msg
            sec
            rts

; Is h_k a reserved tag's atom?  OUT: C = 1 and .X = its place in h_tags if it is
h_reserved:
            ldx         #6
@tag:
            lda         h_k
            cmp         h_tags,x
            bne         :+
            lda         h_k + 1
            cmp         h_tags + 1,x
            beq         @yes
:
            dex
            dex
            bpl         @tag
            clc
            rts
@yes:
            sec
            rts

; Is the key .A, .X the key h_k (the same value; two strings of the same bytes; two bignums of the same number)?
; OUT: Z = 1 if it is
h_keyeq:
            cmp         h_k
            bne         @other
            cpx         h_k + 1
            beq         @rts
@other:
            sta         q_a
            stx         q_a + 1
            lsr
            bcs         @no
            lda         h_k
            lsr
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            ldy         h_k + 1
            cpy         #IMM_PAGES
            bcc         @no
            MOVW        q_b, h_k
            lda         pk,x
            cmp         pk,y
            bne         @no
            cmp         #PK_STRING
            beq         @string
            cmp         #PK_BIGNUM
            bne         @no
            FARN        3, n_qcmp                           ; (Two bignums: their order 0)
            lda         q_r
            rts
@string:
            FARN        2, q_streq
            rts
@no:
            lda         #1
@rts:
            rts

; Is .A, .X of a key's kind (an atom, a string, a number)?  OUT: C = 0 if it is
h_keykind:
            sta         h_nc
            stx         h_nc + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         @yes
            cmp         #PK_STRING
            beq         @yes
            lda         h_nc
            ldx         h_nc + 1
            FARN        3, n_kind
            cmp         #NK_NOT
            beq         @no
@yes:
            clc
            rts
@no:
            sec
            rts

; ---- Entries

; The entry of key h_k in h_h.  OUT: C = 0: h_cell its items' cons, h_prev the one before (NIL: none), h_ent the
; entry; C = 1 if there's none
h_find:
            stz         h_prev
            stz         h_prev + 1
            ldy         #H_ITEMS
            jsr         h_word
@item:
            sta         h_cell
            stx         h_cell + 1
            ora         h_cell + 1
            beq         @none
            lda         h_cell
            ldx         h_cell + 1
            ldy         #0
            jsr         cell_get
            sta         h_ent
            stx         h_ent + 1
            jsr         list_kind                           ; (An entry: a list; past the entries, its tags)
            cmp         #PK_QCONS
            bne         @none
            lda         h_ent
            ldx         h_ent + 1
            ldy         #0
            jsr         cell_get
            jsr         h_keyeq
            beq         @found
            MOVW        h_prev, h_cell
            lda         h_cell
            ldx         h_cell + 1
            ldy         #1
            jsr         cell_get
            bra         @item
@found:
            clc
            rts
@none:
            sec
            rts

; .A, .X = h_ent's key (h_entkey), value (h_entval), tags (h_enttags: its items after its value)
h_entkey:
            lda         h_ent
            ldx         h_ent + 1
            ldy         #0
            jmp         cell_get
h_entval:
            lda         h_ent
            ldx         h_ent + 1
            ldy         #1
            jsr         cell_get
            ldy         #0
            jmp         cell_get
h_enttags:
            lda         h_ent
            ldx         h_ent + 1
            ldy         #1
            jsr         cell_get
            ldy         #1
            jmp         cell_get

; Is h_ent an entry whose value is a hash?  OUT: C = 0 if it is
h_entval_h:
            lda         h_ent
            ldx         h_ent + 1
            jsr         list_kind
            cmp         #PK_QCONS
            bne         @no
            jsr         h_entval
            jmp         h_is
@no:
            sec
            rts

; Has h_h the reserved tag .X (TG_*)?  OUT: C = 1 if it has
h_htag:
            phx
            ldy         #H_ITEMS
            jsr         h_word
            bra         h_tag0
; Has h_ent the reserved tag .X (TG_*)?  OUT: C = 1 if it has
h_etag:
            phx
            jsr         h_enttags
h_tag0:
            sta         h_t
            stx         h_t + 1
            plx
            lda         h_tags,x
            sta         h_tg
            lda         h_tags + 1,x
            sta         h_tg + 1
; Is the atom h_tg among the items of the list h_t?  OUT: C = 1 if it is
h_tagin:
            lda         h_t
            ora         h_t + 1
            beq         @no
            lda         h_t
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            cmp         h_tg
            bne         @next
            cpx         h_tg + 1
            beq         @yes
@next:
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            sta         h_t
            stx         h_t + 1
            bra         h_tagin
@no:
            clc
            rts
@yes:
            sec
            rts

; hv = a new entry {h_k h_v tags...}, the tags .A, .X (a list)
h_newent:
            sta         eu
            stx         eu + 1
            MOVW        et, h_v
            lda         #PK_QCONS
            jsr         h_make2
            MOVW        eu, hv
            MOVW        et, h_k
            lda         #PK_QCONS
            jmp         h_make2

; h_cell's entry made again: its key, the value h_v, the tags .A, .X (h_ent the new one)
h_reent:
            pha
            phx
            jsr         h_entkey
            sta         h_k
            stx         h_k + 1
            plx
            pla
            jsr         h_newent
            MOVW        h_ent, hv
            lda         hv
            ldx         hv + 1
            pha
            MOVW        hv, h_cell
            pla
            ldy         #0
            jmp         cell_set

; h_cell (its entry) taken out of h_h's items, the count one less
h_unlink:
            lda         h_cell
            ldx         h_cell + 1
            ldy         #1
            jsr         cell_get
            pha
            lda         h_prev
            ora         h_prev + 1
            bne         :+
            pla
            ldy         #H_ITEMS
            jsr         h_setw
            jmp         h_count0
:
            MOVW        hv, h_prev
            pla
            ldy         #1
            jsr         cell_set
            jmp         h_count0

; The entry hv put in h_h's items after its entries (before its tags), the count one more
h_insert:
            MOVW        et, hv
            stz         h_prev
            stz         h_prev + 1
            ldy         #H_ITEMS
            jsr         h_word
@item:
            sta         h_cell
            stx         h_cell + 1
            ora         h_cell + 1
            beq         @here
            lda         h_cell
            ldx         h_cell + 1
            ldy         #0
            jsr         cell_get
            jsr         list_kind
            cmp         #PK_QCONS
            bne         @here
            MOVW        h_prev, h_cell
            lda         h_cell
            ldx         h_cell + 1
            ldy         #1
            jsr         cell_get
            bra         @item
@here:
            MOVW        eu, h_cell
            lda         #PK_QCONS
            jsr         h_make2
            lda         h_prev
            ora         h_prev + 1
            bne         :+
            lda         hv
            ldx         hv + 1
            ldy         #H_ITEMS
            jsr         h_setw
            jmp         h_count1
:
            lda         hv
            ldx         hv + 1
            pha
            MOVW        hv, h_prev
            pla
            ldy         #1
            jsr         cell_set
            jmp         h_count1

; The tag h_tg on h_h (at its items' end).  OUT: .A = T if it's new, NIL if it was there
h_htag_add:
            ldy         #H_ITEMS
            jsr         h_word
            sta         h_t
            stx         h_t + 1
            jsr         h_tagin
            bcs         @there
            MOVW        et, h_tg                            ; (A cons of it, at the end)
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         h_make2
            ldy         #H_ITEMS
            jsr         h_word
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            bne         @last
            lda         hv
            ldx         hv + 1
            ldy         #H_ITEMS
            jsr         h_setw
            bra         @new
@last:
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            cmp         #0
            bne         :+
            cpx         #0
            beq         @end
:
            sta         h_t
            stx         h_t + 1
            bra         @last
@end:
            lda         hv
            ldx         hv + 1
            pha
            MOVW        hv, h_t
            pla
            ldy         #1
            jsr         cell_set
@new:
            lda         #<T_VAL
            rts
@there:
            lda         #0
            rts

; The tag h_tg on h_ent (its tags made again, it at their end; cf and cr kept).  OUT: .A = T if it's new, NIL if it
; was there
h_etag_add:
            jsr         h_enttags
            sta         h_t
            stx         h_t + 1
            jsr         h_tagin
            bcs         @there
            lda         cf
            ldx         cf + 1
            jsr         h_rpush
            lda         cr
            ldx         cr + 1
            jsr         h_rpush
            stz         cf                                  ; (Its tags, then it: cf)
            stz         cf + 1
            jsr         h_enttags
@copy:
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            beq         @add
            lda         h_t
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            jsr         h_append
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            bra         @copy
@add:
            lda         h_tg
            ldx         h_tg + 1
            jsr         h_append
            jsr         h_entval
            sta         h_v
            stx         h_v + 1
            lda         cf
            ldx         cf + 1
            jsr         h_reent
            jsr         root_pop
            sta         cr
            stx         cr + 1
            jsr         root_pop
            sta         cf
            stx         cf + 1
            lda         #<T_VAL
            rts
@there:
            lda         #0
            rts

; ---- Operations on h_h (h_mem: a member's)

; hv = a new hash, empty
h_new:
            lda         #PK_HASH
            jsr         h_alloc
            ldy         #4                                  ; (Its count: 0; its flags: none)
            lda         #<FIX(0)
            sta         (hp),y
            lda         #0
            iny
            sta         (hp),y
            iny
            sta         (hp),y
            iny
            sta         (hp),y
            rts

; Put the key .A, .X and the value h_v.  OUT: ex = the old value (NIL if none), or the error
h_put:
            pha
            phx
            ldx         #TG_RO                              ; (Read-only: nothing changed)
            jsr         h_htag
            plx
            pla
            bcc         :+
            LDAX        s_hput_ro
            jmp         h_error
:
            jsr         h_key
            bcc         :+
            rts
:
            stz         ex
            stz         ex + 1
            jsr         h_find
            bcc         @found
            ldx         #TG_LOCKED                          ; (New: not in a locked hash; NIL, nothing)
            jsr         h_htag
            bcc         :+
            LDAX        s_hput_lock
            jmp         h_error
:
            lda         h_v
            ora         h_v + 1
            beq         @ok
            lda         #0
            tax
            jsr         h_newent
            jsr         h_insert
@ok:
            clc
            rts
@found:
            ldx         #TG_PRIV                            ; (A private entry: a member's)
            jsr         h_etag
            bcc         :+
            lda         h_mem
            bne         :+
            LDAX        s_hput_priv
            jmp         h_error
:
            ldx         #TG_RO
            jsr         h_etag
            bcc         :+
            LDAX        s_hput_ero
            jmp         h_error
:
            jsr         h_entval                            ; (Its old value)
            sta         ex
            stx         ex + 1
            lda         h_v
            ora         h_v + 1
            bne         @set
            ldx         #TG_NOTNIL                          ; (NIL: not for a not-nil entry; kept in a locked hash
            jsr         h_etag                              ;   or entry, its value NIL; else the entry out)
            bcc         :+
            jsr         h_begin_msg                         ; ("Cannot set non-nillable field :k to NIL.")
            LDAX        s_hnotnil
            jsr         out_text
            lda         h_k
            ldx         h_k + 1
            jsr         h_repr
            LDAX        s_hnotnil2
            jsr         out_text
            jmp         h_end_msg
:
            ldx         #TG_LOCKED
            jsr         h_htag
            bcs         @set
            ldx         #TG_LOCKED
            jsr         h_etag
            bcs         @set
            jsr         h_unlink
            clc
            rts
@set:
            lda         ex                                  ; (Its old value kept)
            pha
            lda         ex + 1
            pha
            jsr         h_enttags
            jsr         h_reent
            pla
            sta         ex + 1
            pla
            sta         ex
            clc
            rts

; Put the entry h_e ({key value tag...}; {tag} alone: a tag on the hash, unless it's a key's), its tags on it (if it's
; there, not read-only, and a member's if private).  OUT: ex = the old value, NIL, or the error
h_putentry:
            lda         h_e
            ldx         h_e + 1
            jsr         h_items
            bcs         @bad
            cmp         #2
            bcs         @entry
            cmp         #1
            bne         @bad
            ldy         #0                                  ; ({tag})
            jsr         h_nth
            sta         h_tg
            stx         h_tg + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         @bad
            MOVW        h_k, h_tg
            jsr         h_reserved
            bcs         @tag
            jsr         h_find
            bcc         @nil
@tag:
            jsr         h_htag_add
@nil:
            stz         ex
            stz         ex + 1
            clc
            rts
@bad:
            MOVW        h_v, h_e                            ; ("A hash's entry is {key value tag...}, not 5")
            LDAX        s_hentry
            jmp         h_error_v
@entry:
            ldy         #1                                  ; (Its tags: atoms)
            jsr         h_rest
@tags:
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            beq         @put
            lda         h_t
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            sta         h_v
            stx         h_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         :+
            LDAX        s_htagatom
            jmp         h_error_v
:
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            bra         @tags
@put:
            ldy         #1                                  ; (Its value, its key: put)
            jsr         h_nth
            sta         h_v
            stx         h_v + 1
            ldy         #0
            jsr         h_nth
            jsr         h_put
            jsr         h_is_err
            bcs         @done
            lda         h_e                                 ; (Its entry: its tags on it)
            ldx         h_e + 1
            jsr         h_items
            ldy         #0
            jsr         h_nth
            jsr         h_key
            bcs         @done
            jsr         h_find
            bcs         @done
            ldx         #TG_RO
            jsr         h_etag
            bcs         @done
            ldx         #TG_PRIV
            jsr         h_etag
            bcc         :+
            lda         h_mem
            beq         @done
:
            lda         ex                                  ; (Its old value kept)
            pha
            lda         ex + 1
            pha
            lda         h_e
            ldx         h_e + 1
            jsr         h_items
            ldy         #1
            jsr         h_rest
@etag:
            sta         h_e
            stx         h_e + 1
            ora         h_e + 1
            beq         @tagged
            lda         h_e
            ldx         h_e + 1
            ldy         #0
            jsr         cell_get
            sta         h_tg
            stx         h_tg + 1
            jsr         h_etag_add
            lda         h_e
            ldx         h_e + 1
            ldy         #1
            jsr         cell_get
            bra         @etag
@tagged:
            pla
            sta         ex + 1
            pla
            sta         ex
@done:
            clc
            rts

; Take the key .A, .X out.  OUT: ex = its value, NIL (none), or the error
h_remove:
            pha
            phx
            ldx         #TG_RO
            jsr         h_htag
            plx
            pla
            bcc         :+
            LDAX        s_hrem_ro
            jmp         h_error
:
            jsr         h_key
            bcc         :+
            rts
:
            stz         ex
            stz         ex + 1
            jsr         h_find
            bcs         @done
            ldx         #TG_PRIV
            jsr         h_etag
            bcc         :+
            lda         h_mem
            bne         :+
            LDAX        s_hrem_priv
            jmp         h_error
:
            ldx         #TG_LOCKED                          ; (A locked hash, or a read-only entry: kept)
            jsr         h_htag
            bcs         @locked
            ldx         #TG_RO
            jsr         h_etag
            bcs         @locked
            jsr         h_entval
            sta         ex
            stx         ex + 1
            jsr         h_unlink
@done:
            clc
            rts
@locked:
            LDAX        s_hrem_lock
            jmp         h_error

; The value at the key .A, .X (a private entry: a member's); NIL if none.  OUT: ex = it, or the error
h_get:
            jsr         h_key
            bcc         :+
            rts
:
            stz         ex
            stz         ex + 1
            jsr         h_find
            bcs         @done
            ldx         #TG_PRIV
            jsr         h_etag
            bcc         :+
            lda         h_mem
            bne         :+
            LDAX        s_hget_priv
            jmp         h_error
:
            jsr         h_entval
            sta         ex
            stx         ex + 1
@done:
            clc
            rts

; A tag added (hash-add-tag's one): h_v an atom (on the hash), or {key tag} (on the entry).  OUT: ex = T if it's new,
; NIL if not, or the error
h_addtag:
            lda         h_v
            ldx         h_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         @list
            MOVW        h_tg, h_v
            jsr         h_htag_add
            bra         @set
@list:
            lda         h_v
            ldx         h_v + 1
            jsr         h_items
            bcs         @invalid
            cmp         #1
            bne         :+
            LDAX        s_htagatom                          ; ({x}: "A tag must be an atom, not {x}")
            jmp         h_error_v
:
            cmp         #2
            bne         @invalid
            ldy         #1                                  ; ({key tag}: the entry's)
            jsr         h_nth
            sta         h_tg
            stx         h_tg + 1
            ldy         #0
            jsr         h_nth
            jsr         h_key
            bcc         :+
            rts
:
            jsr         h_find
            bcc         :+
            LDAX        s_haddnone
            jmp         h_error
:
            lda         h_tg
            ldx         h_tg + 1
            jsr         kind_of
            cmp         #PK_ATOM
            beq         :+
            MOVW        h_v, h_tg
            LDAX        s_htagatom
            jmp         h_error_v
:
            jsr         h_etag_add
@set:
            jmp         h_set_a
@invalid:
            LDAX        s_hinvalid                          ; ("Failed to add tag: invalid parameters 5")
            jmp         h_error_v

; Has the hash, or an entry, a tag (hash-tag?'s): h_v an atom, {tag}, or {key tag}.  OUT: ex = T or NIL, or the
; error
h_hastag:
            lda         h_v
            ldx         h_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         @list
            MOVW        h_tg, h_v
@own:
            ldy         #H_ITEMS
            jsr         h_word
            sta         h_t
            stx         h_t + 1
            jsr         h_tagin
            jmp         h_bool
@list:
            lda         h_v
            ldx         h_v + 1
            jsr         h_items
            bcs         @invalid
            cmp         #0
            beq         @invalid
            cmp         #1
            bne         @entry
            ldy         #0                                  ; ({tag}: the hash's)
            jsr         h_nth
            sta         h_tg
            stx         h_tg + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         @invalid
            bra         @own
@entry:
            ldy         #1                                  ; ({key tag}: the entry's)
            jsr         h_nth
            sta         h_tg
            stx         h_tg + 1
            ldy         #0
            jsr         h_nth
            sta         h_v
            stx         h_v + 1
            jsr         h_key
            bcc         :+
            rts
:
            jsr         h_find
            bcc         :+
            jsr         h_begin_msg                         ; ("Key :x not found when looking up tag for hash entry")
            LDAX        s_hkeytag
            jsr         out_text
            lda         h_v
            ldx         h_v + 1
            jsr         h_repr
            LDAX        s_hkeytag2
            jsr         out_text
            jmp         h_end_msg
:
            lda         h_tg
            ldx         h_tg + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         @invalid
            jsr         h_enttags
            sta         h_t
            stx         h_t + 1
            jsr         h_tagin
            jmp         h_bool
@invalid:
            LDAX        s_hinvtag
            jmp         h_error

; ---- Lists of a hash

; cf = h_h's keys (h_keys), or values (h_values), a list
h_keys:
            lda         #0
            bra         h_list
h_values:
            lda         #1
h_list:
            sta         h_n
            stz         cf
            stz         cf + 1
            ldy         #H_ITEMS
            jsr         h_word
@item:
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            beq         @done
            lda         h_t
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            sta         h_ent
            stx         h_ent + 1
            jsr         list_kind
            cmp         #PK_QCONS
            bne         @done
            lda         h_n
            bne         :+
            jsr         h_entkey
            bra         :++
:
            jsr         h_entval
:
            jsr         h_append
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            bra         @item
@done:
            rts

; cf = the hash .A, .X as a list (from#'s): its entries {key value tag...} (a value that's a hash: such a list, to
; H_DEEP deep), then its tags.  (Each hash walked is reached from the arguments: only the lists made are kept)
h_tolist:
            jsr         h_open
            inc         h_depth
            stz         cf
            stz         cf + 1
            stz         cr
            stz         cr + 1
            ldy         #H_ITEMS
            jsr         h_word
@item:
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            bne         @lb316
            jmp         @done
@lb316:
            lda         h_t
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            sta         h_ent
            stx         h_ent + 1
            jsr         h_entval_h
            bcc         @lb315
            jmp         @plain
@lb315:
            lda         h_depth
            cmp         #H_DEEP
            bcc         @lb314
            jmp         @plain
@lb314:
            lda         h_t                                 ; (A hash in it: its list made first, the list so far
            pha                                             ;   kept)
            lda         h_t + 1
            pha
            lda         h_ent
            pha
            lda         h_ent + 1
            pha
            lda         cf
            ldx         cf + 1
            jsr         h_rpush
            lda         cr
            ldx         cr + 1
            jsr         h_rpush
            jsr         h_entval
            jsr         h_tolist
            lda         cf
            ldx         cf + 1
            jsr         h_rpush
            ldx         rsp                                 ; (The list so far: back)
            dex
            dex
            lda         rs_lo,x
            sta         cr
            lda         rs_hi,x
            sta         cr + 1
            dex
            lda         rs_lo,x
            sta         cf
            lda         rs_hi,x
            sta         cf + 1
            pla
            sta         h_ent + 1
            pla
            sta         h_ent
            pla
            sta         h_t + 1
            pla
            sta         h_t
            jsr         h_enttags                           ; (The entry: its key, the list, its tags)
            sta         eu
            stx         eu + 1
            ldx         rsp
            dex
            lda         rs_lo,x
            sta         et
            lda         rs_hi,x
            sta         et + 1
            lda         #PK_QCONS
            jsr         h_make2
            MOVW        eu, hv
            jsr         h_entkey
            sta         et
            stx         et + 1
            lda         #PK_QCONS
            jsr         h_make2
            lda         rsp                                 ; (The three kept: dropped)
            sec
            sbc         #3
            sta         rsp
            lda         hv
            ldx         hv + 1
            bra         @add
@plain:
            lda         h_ent                               ; (Else the item as it is: an entry, a tag)
            ldx         h_ent + 1
@add:
            jsr         h_append
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            jmp         @item
@done:
            dec         h_depth
            rts

; hv = a copy of the hash .A, .X: its entries (a value that's a hash, cloned with its tags, to H_DEEP deep), not its
; own tags (hash-clone's, after its overrides: h_tagsfrom)
h_clone:
            jsr         h_open
            lda         h_h
            ldx         h_h + 1
            jsr         h_rpush                             ; (The old hash, then the new one: kept)
            inc         h_depth
            jsr         h_new
            lda         hv
            ldx         hv + 1
            jsr         h_rpush
            jsr         h_old
            ldy         #H_ITEMS
            jsr         h_word
@item:
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            bne         @lb313
            jmp         @done
@lb313:
            lda         h_t
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            sta         h_ent
            stx         h_ent + 1
            jsr         list_kind
            cmp         #PK_QCONS
            beq         @lb322
            jmp         @done
@lb322:
            lda         h_t
            pha
            lda         h_t + 1
            pha
            jsr         h_entval_h                          ; (A hash in it: cloned)
            bcs         @share
            lda         h_depth
            cmp         #H_DEEP
            bcs         @share
            lda         h_ent
            pha
            lda         h_ent + 1
            pha
            jsr         h_entval
            jsr         h_clone
            MOVW        h_v, hv
            lda         h_v                                 ; (Kept while its tags are added and its entry made)
            ldx         h_v + 1
            jsr         h_rpush
            pla
            sta         h_ent + 1
            pla
            sta         h_ent
            jsr         h_entval                            ; (Its tags: h_h is the copy)
            jsr         h_tagsfrom
            jsr         h_entkey
            sta         h_k
            stx         h_k + 1
            jsr         h_enttags
            jsr         h_newent
            dec         rsp
            bra         @put
@share:
            MOVW        hv, h_ent
@put:
            jsr         h_new_h                             ; (In the new hash, after its entries)
            jsr         h_insert
            pla
            sta         h_t + 1
            pla
            sta         h_t
            jsr         h_old
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            jmp         @item
@done:
            jsr         h_new_h
            MOVW        hv, h_h
            lda         rsp
            sec
            sbc         #2
            sta         rsp
            dec         h_depth
            rts

; h_h = the old hash (h_old), or the new (h_new_h), h_clone's on the root stack
h_old:
            ldx         rsp
            dex
            dex
            bra         :+
h_new_h:
            ldx         rsp
            dex
:
            lda         rs_lo,x
            sta         h_h
            lda         rs_hi,x
            sta         h_h + 1
            rts

; The tags of the hash .A, .X (its own) added to h_h's
h_tagsfrom:
            sta         h_t
            stx         h_t + 1
            ldy         #H_FLAGS                            ; (A proxy's: its hash's)
            jsr         cell_get
            and         #H_PROXY
            beq         :+
            lda         h_t
            ldx         h_t + 1
            ldy         #H_REAL
            jsr         cell_get
            sta         h_t
            stx         h_t + 1
:
            lda         h_t
            ldx         h_t + 1
            ldy         #H_ITEMS
            jsr         cell_get
@item:
            sta         h_e
            stx         h_e + 1
            ora         h_e + 1
            beq         @done
            lda         h_e
            ldx         h_e + 1
            ldy         #0
            jsr         cell_get
            sta         h_tg
            stx         h_tg + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         :+
            jsr         h_htag_add
:
            lda         h_e
            ldx         h_e + 1
            ldy         #1
            jsr         cell_get
            bra         @item
@done:
            rts

; One of hash-clone's overrides, h_v, put in h_h (as a member): a tag; an entry {key value tag...} (its value as it
; is); or a list of them (a list of one: its item).  OUT: ex = the error, if one is
h_override:
            lda         h_v                                 ; (A list of one: its item)
            ldx         h_v + 1
            jsr         h_items
            bcs         @one
            cmp         #1
            bne         @one
            ldy         #0
            jsr         h_nth
            sta         h_v
            stx         h_v + 1
            bra         h_override
@one:
            lda         h_v
            ldx         h_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         :+
            jmp         h_addtag
:
            lda         h_v                                 ; (An entry: {key value ...}, its key of a key's kind)
            ldx         h_v + 1
            jsr         h_items
            bcs         @bad
            cmp         #0
            beq         @bad
            cmp         #2
            bcc         @list
            ldy         #0
            jsr         h_nth
            jsr         h_keykind
            bcs         @list
            MOVW        h_e, h_v
            jmp         h_putent1
@list:
            lda         h_v                                 ; (A list of them: each in turn)
            ldx         h_v + 1
            jsr         q_conses
@item:
            sta         h_cell
            stx         h_cell + 1
            ora         h_cell + 1
            beq         @done
            lda         h_cell
            pha
            lda         h_cell + 1
            pha
            lda         h_cell
            ldx         h_cell + 1
            ldy         #0
            jsr         cell_get
            sta         h_v
            stx         h_v + 1
            jsr         h_override1
            pla
            sta         h_cell + 1
            pla
            sta         h_cell
            jsr         h_is_err
            bcs         @rts
            lda         h_cell
            ldx         h_cell + 1
            ldy         #1
            jsr         cell_get
            bra         @item
@done:
            stz         ex
            stz         ex + 1
@rts:
            clc
            rts
@bad:
            LDAX        s_hentry
            jmp         h_error_v

; One of a list of overrides, h_v: an atom (a tag), {tag}, or an entry
h_override1:
            lda         h_v
            ldx         h_v + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         :+
            jmp         h_addtag
:
            lda         h_v
            ldx         h_v + 1
            jsr         h_items
            bcs         @bad
            cmp         #0
            beq         @bad
            cmp         #1
            bne         :+
            ldy         #0                                  ; ({tag})
            jsr         h_nth
            sta         h_v
            stx         h_v + 1
            jmp         h_addtag
:
            MOVW        h_e, h_v
            jmp         h_putent1
@bad:
            LDAX        s_hentry
            jmp         h_error_v

; The entry h_e put as it is (a member's), then each of its tags on it ({key tag}).  OUT: ex = the error if one is
h_putent1:
            lda         h_e
            ldx         h_e + 1
            jsr         h_items
            ldy         #1
            jsr         h_nth
            sta         h_v
            stx         h_v + 1
            ldy         #0
            jsr         h_nth
            jsr         h_put
            jsr         h_is_err
            bcs         @done
            lda         h_e
            ldx         h_e + 1
            jsr         h_items
            ldy         #1
            jsr         h_rest
@tag:
            sta         h_w
            stx         h_w + 1
            ora         h_w + 1
            beq         @done
            lda         h_w
            pha
            lda         h_w + 1
            pha
            lda         h_e                                 ; ({key tag})
            ldx         h_e + 1
            jsr         h_items
            ldy         #0
            jsr         h_nth
            sta         et
            stx         et + 1
            pla
            tax
            pla
            pha
            phx
            ldy         #0
            jsr         cell_get
            sta         eu
            stx         eu + 1
            jsr         h_pair
            MOVW        h_v, hv
            jsr         h_addtag
            plx
            pla
            sta         h_w
            stx         h_w + 1
            jsr         h_is_err
            bcs         @done
            lda         h_w
            ldx         h_w + 1
            ldy         #1
            jsr         cell_get
            bra         @tag
@done:
            clc
            rts

; ---- The built-ins (each: its hash the argument 0, else the error "First parameter to 'name' must be a hash")

; The argument 0 a hash: h_h (opened), and ex the argument 1.  OUT: C = 1 and ex = the error if it isn't one
h_arg:
            lda         (argp)
            pha
            ldy         #1
            lda         (argp),y
            tax
            pla
            jsr         h_is
            bcs         @not
            jsr         h_open
            ldy         #2
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            clc
            rts
@not:
            jsr         h_begin_msg
            LDAX        s_hfirst
            jsr         out_text
            ldx         bi_n
            FARN        2, bi_name
            LDAX        s_hmust
            jsr         out_text
            jsr         h_end_msg
            sec
            rts

; .A, .X = the argument 0
h_arg0:
            lda         (argp)
            pha
            ldy         #1
            lda         (argp),y
            tax
            pla
            rts

; (hash-get h key): the value at key (NIL if none; NIL for a NIL hash); a private entry, an error
bi_hash_get:
            jsr         h_enter
            lda         (argp)
            ldy         #1
            ora         (argp),y
            bne         :+
            stz         ex
            stz         ex + 1
            bra         h_done
:
            jsr         h_arg
            bcs         h_done
            lda         ex
            ldx         ex + 1
            jsr         h_get
; The built-in's value, ex
h_done:
            clc
            rts

; (hash-key? h key): T if it has the key
bi_hash_key_p:
            jsr         h_enter
            jsr         h_arg
            bcs         h_done
            lda         ex
            ldx         ex + 1
            jsr         h_key0
            bcs         h_done
            jsr         h_find
            jmp         h_bool_nc

; (hash-keys h), (hash-values h): its keys, its values, lists
bi_hash_keys:
            jsr         h_enter
            jsr         h_arg
            bcs         h_done
            jsr         h_keys
            bra         h_cf
bi_hash_values:
            jsr         h_enter
            jsr         h_arg
            bcs         h_done
            jsr         h_values
h_cf:
            MOVW        ex, cf
            bra         h_done

; (from# h): the hash as a list: its entries {key value tag...} (a value that's a hash: such a list), its tags
bi_from_hash:
            jsr         h_enter
            jsr         h_arg
            bcs         h_done
            stz         h_depth
            jsr         h_arg0
            jsr         h_tolist
            bra         h_cf

; (hash-remove h key): the entry taken out: its value, or NIL
bi_hash_remove:
            jsr         h_enter
            jsr         h_arg
            bcs         h_done
            lda         ex
            ldx         ex + 1
            jsr         h_remove
            bra         h_done

; (hash-tag? h tag): whether the hash has the tag (an atom, or {tag}), or its entry ({key tag})
bi_hash_tag_p:
            jsr         h_enter
            jsr         h_arg
            bcs         h_done
            MOVW        h_v, ex
            jsr         h_hastag
            bra         h_done

; (hash-locked? h [key]), (hash-private? ...), (hash-const? ...): whether the hash, or its entry, has the tag
bi_hash_locked_p:
            ldx         #TG_LOCKED
            bra         h_named_p
bi_hash_private_p:
            ldx         #TG_PRIV
            bra         h_named_p
bi_hash_const_p:
            ldx         #TG_RO
h_named_p:
            stx         h_n
            jsr         h_enter
            jsr         h_arg
            bcc         @lb312
            jmp         h_done
@lb312:
            jsr         h_tagarg
            lda         cn
            cmp         #2
            bcc         :+
            MOVW        et, ex                              ; ({key tag})
            MOVW        eu, h_v
            jsr         h_pair
            MOVW        h_v, hv
:
            jsr         h_hastag
            jmp         h_done

; h_v = the reserved tag h_n's atom
h_tagarg:
            ldx         h_n
            lda         h_tags,x
            sta         h_v
            lda         h_tags + 1,x
            sta         h_v + 1
            rts

; (hash-add-tag h tag...): a tag on the hash (an atom) or on an entry ({key tag}): T if it's new; several, a list
bi_hash_add_tag:
            jsr         h_enter
            jsr         h_arg
            bcc         @lb311
            jmp         h_done
@lb311:
            lda         #1
            sta         h_n
            stz         cf
            stz         cf + 1
@tag:
            lda         h_n
            cmp         cn
            bcc         @lb310
            jmp         h_cf
@lb310:
            asl
            tay
            lda         (argp),y
            sta         h_v
            iny
            lda         (argp),y
            sta         h_v + 1
            lda         h_n
            pha
            jsr         h_addtag
            pla
            sta         h_n
            lda         cn                                  ; (One: its value)
            cmp         #2
            bne         @lb309
            jmp         h_done
@lb309:
            lda         ex
            ldx         ex + 1
            jsr         h_append
            inc         h_n
            bra         @tag

; (hash-lock h [key...]), (hash-make-const ...), (hash-make-private ...), (hash-make-not-nil ...): the reserved tag on
; the hash, or on each key's entry ({key tag}), as hash-add-tag puts them
bi_hash_lock:
            ldx         #TG_LOCKED
            bra         h_named
bi_hash_make_const:
            ldx         #TG_RO
            bra         h_named
bi_hash_make_private:
            ldx         #TG_PRIV
            bra         h_named
bi_hash_make_not_nil:
            ldx         #TG_NOTNIL
h_named:
            stx         h_n
            jsr         h_enter
            jsr         h_arg
            bcs         @done
            jsr         h_tagarg
            lda         cn
            cmp         #2
            bcs         @keys
            jsr         h_addtag
@done:
            jmp         h_done
@keys:
            MOVW        h_tg, h_v
            lda         h_tg                                ; (The tag kept: h_addtag uses h_tg)
            pha
            lda         h_tg + 1
            pha
            lda         #1
            sta         h_n
            stz         cf
            stz         cf + 1
@key:
            lda         h_n
            cmp         cn
            bcs         @made
            asl
            tay
            lda         (argp),y
            sta         et
            iny
            lda         (argp),y
            sta         et + 1
            pla                                             ; ({key tag})
            sta         eu + 1
            pla
            sta         eu
            pha
            lda         eu + 1
            pha
            jsr         h_pair
            MOVW        h_v, hv
            lda         h_n
            pha
            jsr         h_addtag
            pla
            sta         h_n
            lda         cn                                  ; (One key: its value)
            cmp         #2
            beq         @one
            lda         ex
            ldx         ex + 1
            jsr         h_append
            inc         h_n
            bra         @key
@made:
            pla
            pla
            jmp         h_cf
@one:
            pla
            pla
            jmp         h_done

; (hash-clone h [override...]): a copy (a value that's a hash, cloned too), each override put (an atom, a tag; an
; entry {key value tag...}, its value as it is; a list of them), then the hash's own tags
bi_hash_clone:
            jsr         h_enter
            jsr         h_arg
            bcc         :+
            jmp         h_done
:
            stz         h_depth
            jsr         h_arg0
            jsr         h_clone
            lda         hv                                  ; (The copy, kept)
            ldx         hv + 1
            jsr         h_rpush
            lda         #1                                  ; (Each argument from 1: an override, put as a member)
            sta         h_n
@arg:
            lda         h_n
            cmp         cn
            bcs         @tags
            asl
            tay
            lda         (argp),y
            sta         h_v
            iny
            lda         (argp),y
            sta         h_v + 1
            jsr         @copy
            dec         h_mem
            lda         h_n
            pha
            jsr         h_override
            pla
            sta         h_n
            jsr         h_is_err
            bcs         @done
            inc         h_n
            bra         @arg
@tags:
            jsr         @copy
            jsr         h_arg0
            jsr         h_tagsfrom
            jsr         @copy
            MOVW        ex, h_h
@done:
            dec         rsp
            jmp         h_done
@copy:
            ldx         rsp                                 ; (h_h: the copy)
            dex
            lda         rs_lo,x
            pha
            lda         rs_hi,x
            tax
            pla
            jmp         h_open

; ---- What the other banks call

; hv = a hash of the entries {key value} in the list cf, put as a member puts them (the fifth bank's stat, date-parts,
; env).  OUT: C = 1 if there's no room
h_fromlist:
            jsr         h_enter
            jsr         h_new
            MOVW        h_h, hv
            lda         hv
            ldx         hv + 1
            jsr         h_rpush
            lda         #$FF
            sta         h_mem
            lda         cf
            ldx         cf + 1
@entry:
            sta         h_e
            stx         h_e + 1
            ora         h_e + 1
            beq         @done
            lda         h_e                                 ; (The list walked: on the 65C02's stack)
            pha
            lda         h_e + 1
            pha
            ldy         #0
            lda         h_e
            jsr         cell_get
            sta         h_e
            stx         h_e + 1
            jsr         h_putent1
            pla
            tax
            pla
            ldy         #1
            jsr         cell_get
            bra         @entry
@done:
            jsr         root_pop
            sta         hv
            stx         hv + 1
            clc
            rts

; .A, .X = the count of the hash .A, .X (a fixnum: len's)
h_count_of:
            jsr         h_open
            ldy         #H_COUNT
            jmp         h_word

; q_a and q_b, two hashes, equal (eqcmp.inc's q_same)?  The same count, each key of one the other's too, their
; values pushed to be compared (q_push's way).  OUT: C = 0 if they aren't; C = 1, V = 1 if pairs were pushed
h_qsame:
            lda         q_b
            ldx         q_b + 1
            jsr         h_open
            MOVW        h_e, h_h                            ; (q_b's hash: h_e)
            lda         q_a
            ldx         q_a + 1
            jsr         h_open
            lda         h_h
            cmp         h_e
            bne         :+
            lda         h_h + 1
            cmp         h_e + 1
            bne         @lb308
            jmp         @same
@lb308:
:
            ldy         #H_COUNT                            ; (Counts the same)
            jsr         h_word
            sta         h_w
            stx         h_w + 1
            lda         h_e
            ldx         h_e + 1
            ldy         #H_COUNT
            jsr         cell_get
            cmp         h_w
            beq         @lb307
            jmp         @no
@lb307:
            cpx         h_w + 1
            beq         @lb306
            jmp         @no
@lb306:
            stz         h_n
            ldy         #H_ITEMS
            jsr         h_word
@item:
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            bne         @lb305
            jmp         @done
@lb305:
            lda         h_t
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            sta         h_ent
            stx         h_ent + 1
            jsr         list_kind
            cmp         #PK_QCONS
            bne         @done
            jsr         h_entkey                            ; (Its key in the other)
            sta         h_k
            stx         h_k + 1
            jsr         h_entval
            sta         h_v
            stx         h_v + 1
            lda         h_h
            pha
            lda         h_h + 1
            pha
            MOVW        h_h, h_e
            lda         h_t
            pha
            lda         h_t + 1
            pha
            jsr         h_find
            pla
            sta         h_t + 1
            pla
            sta         h_t
            pla
            sta         h_h + 1
            pla
            sta         h_h
            bcs         @no
            lda         h_v                                 ; (The pair of values pushed)
            ldx         h_v + 1
            jsr         push
            jsr         h_entval
            jsr         push
            inc         q_n
            bne         :+
            inc         q_n + 1
:
            lda         #1
            sta         h_n
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            jmp         @item
@done:
            lda         h_n
            beq         @same
            sec
            bit         h_vbit
            rts
@same:
            sec
            clv
            rts
@no:
            clc
            rts
h_vbit:     .byte       $40

; hash-create's, to#'s and hash-put's (forms.inc's hash_begin): their entries' values evaluated, in the caller's
; scope, an item at a time.  h_begin: IN h_mode (HM_*), the arguments.  OUT: C = 1 if there's no room; the arguments
; dropped; .A = 0: ex the value; .A = 1: the frame to push, its hash et, its items cr, its state h_st
h_begin:
            jsr         h_enter
            lda         h_mode
            cmp         #HM_PUT
            beq         @put
            cmp         #HM_TO
            beq         @list
            lda         cn                                  ; (hash-create, none: an empty hash)
            bne         @list
            jsr         h_new
            MOVW        ex, hv
            bra         @value
@list:
            jsr         h_arg0                              ; (The entries: a list)
            jsr         q_conses
            bcc         :+
            LDAX        s_hentries
            ldy         h_mode
            beq         @error
            LDAX        s_htofirst
@error:
            jsr         h_error
@value:
            stz         ev_inargs
            jsr         drop_args
            lda         #0
            clc
            rts
:
            sta         cr                                  ; (The items: cr; the hash made: et)
            stx         cr + 1
            jsr         h_new
            MOVW        et, hv
            lda         #HS_NEXT
            bra         @frame
@put:
            jsr         h_arg0                              ; (hash-put: its hash, then its entries a list)
            jsr         h_is
            bcc         :+
            jsr         h_arg
            bra         @value
:
            stz         cf
            stz         cf + 1
            lda         #1
            sta         h_n
@arg:
            lda         h_n
            cmp         cn
            bcs         @made
            asl
            tay
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            jsr         h_append
            inc         h_n
            bra         @arg
@made:
            MOVW        cr, cf
            jsr         h_arg0
            sta         et
            stx         et + 1
            lda         cn
            cmp         #3
            lda         #HS_NEXT
            bcc         @frame
            lda         #HS_NEXT | HS_SEVERAL
@frame:
            sta         h_st                                ; (Its state: and its mode, << 3)
            lda         h_mode
            asl
            asl
            asl
            ora         h_st
            sta         h_st
            stz         ev_inargs
            jsr         drop_args
            lda         #1
            clc
            rts

; The next step (h_step): .A = $FF, the first; else 0, ex the value evaluated.  OUT: C = 1 if there's no room; .A =
; 0: done, ex the value (the frame and its code dropped); .A = 1: ex to be evaluated in ee (set)
h_step:
            sta         h_first
            jsr         h_enter
            ldy         #HF_STATE                           ; (Its state: h_st)
            jsr         hf_get
            jsr         unfix
            sta         h_st
            ldy         #HF_HASH                            ; (Its hash: h_h, a member's in hash-create's and to#'s)
            jsr         hf_get
            jsr         h_open
            lda         h_st
            and         #HM_PUT << 3
            bne         :+
            dec         h_mem
:
            lda         h_first
            cmp         #$FF
            bne         :+
            jmp         @next
:
            jsr         h_is_err                              ; (An error evaluated: the value, or one of several)
            bcc         @value
            lda         h_st
            and         #HS_SEVERAL
            bne         @lb304
            jmp         @done
@lb304:
            lda         ex                                  ; (Ctrl-C's: not one of several; it ends them)
            ldx         ex + 1
            ldy         #1
            jsr         cell_get
            cmp         atom_intr
            bne         @add
            cpx         atom_intr + 1
            bne         @lb303
            jmp         @done
@lb303:
@add:
            jsr         hf_add
            jmp         @next0
@result:
            lda         h_st
            and         #HS_SEVERAL
            bne         @lb302
            jmp         @done
@lb302:
            jsr         hf_add
            jmp         @next0
@value:
            lda         h_st
            and         #3
            cmp         #HS_ENTRY
            bne         @val
            MOVW        h_v, ex                             ; (An entry evaluated: its value next)
            lda         h_v
            ldx         h_v + 1
            ldy         #HF_ENTRY
            jsr         hf_put
            jsr         hf_entry
            bcs         @done
            jmp         @eval
@val:
            ldy         #HF_ENTRY                           ; (Its value: the entry made again with it, put)
            jsr         hf_get
            jsr         h_items
            ldy         #0
            jsr         h_nth
            sta         h_k
            stx         h_k + 1
            ldy         #1
            jsr         h_rest
            pha
            phx
            MOVW        h_v, ex
            plx
            pla
            jsr         h_newent
            MOVW        h_e, hv
            lda         hv                                  ; (Kept while it's put)
            ldx         hv + 1
            jsr         h_rpush
            lda         h_st
            and         #HM_PUT << 3
            bne         :+
            jsr         h_putent1
            dec         rsp
            jsr         h_is_err
            bcs         @done
            bra         @next0
:
            jsr         h_putentry
            dec         rsp
            jmp         @result
@done:
            lda         #HF_SIZE / 2 + 1                    ; (The frame and its code: dropped)
            jsr         drop
            lda         #0
            clc
            rts
@next0:
            lda         h_st
            and         #$FC                                ; (HS_NEXT)
            sta         h_st
@next:
            ldy         #HF_ITEMS                           ; (The next item, or the end)
            jsr         hf_get
            sta         h_t
            stx         h_t + 1
            ora         h_t + 1
            bne         @item
            lda         h_st
            and         #HS_SEVERAL
            beq         :+
            ldy         #HF_FIRST                           ; (hash-put's several: the list of what each gave)
            jsr         hf_get
            bra         @made
:
            ldy         #HF_HASH
            jsr         hf_get
@made:
            sta         ex
            stx         ex + 1
            bra         @done
@item:
            lda         h_t
            ldx         h_t + 1
            ldy         #1
            jsr         cell_get
            ldy         #HF_ITEMS
            jsr         hf_put
            lda         h_t                                 ; (The item: h_v, kept in the frame)
            ldx         h_t + 1
            ldy         #0
            jsr         cell_get
            sta         h_v
            stx         h_v + 1
            ldy         #HF_ENTRY
            jsr         hf_put
            lda         h_st
            and         #HM_PUT << 3
            bne         @putitem
            lda         h_v                                 ; (hash-create's, to#'s: a tag; an entry, as written;
            ldx         h_v + 1                             ;   an S-expression, evaluated to be one)
            jsr         kind_of
            cmp         #PK_ATOM
            bne         :+
            MOVW        h_tg, h_v
            jsr         h_htag_add
            bra         @next
:
            lda         h_v
            ldx         h_v + 1
            jsr         list_kind
            cmp         #PK_SCONS
            bne         :+
            lda         h_st
            ora         #HS_ENTRY
            sta         h_st
            MOVW        ex, h_v
            bra         @eval
:
            jsr         hf_entry
            bcc         @eval
            jmp         @done
@putitem:
            lda         h_v                                 ; (hash-put's: {key value tag...}, its value evaluated;
            ldx         h_v + 1                             ;   anything else put as it is)
            jsr         h_items
            bcs         @asis
            cmp         #2
            bcc         @asis
            jsr         hf_entry2
            bra         @eval
@asis:
            MOVW        h_e, h_v
            jsr         h_putentry
            jmp         @result
@eval:
            lda         h_st                                ; (Its state kept; ex evaluated in the caller's scope)
            ldx         #0
            jsr         fix_ax
            ldy         #HF_STATE
            jsr         hf_put
            ldy         #HF_EE
            jsr         hf_get
            sta         ee
            stx         ee + 1
            lda         #1
            clc
            rts

; The entry h_v (hash-create's): a list of two items or more, its first a key's kind (else ex = the error "A hash's
; entry is ...", C = 1); its value to be evaluated: ex, the state HS_VALUE (C = 0)
hf_entry:
            lda         h_v
            ldx         h_v + 1
            jsr         h_items
            bcs         hf_bad
            cmp         #2
            bcc         hf_bad
            ldy         #0
            jsr         h_nth
            jsr         h_keykind
            bcs         hf_bad
hf_entry2:
            lda         h_v
            ldx         h_v + 1
            jsr         h_items
            ldy         #1
            jsr         h_nth
            sta         ex
            stx         ex + 1
            lda         h_st
            and         #$FC
            ora         #HS_VALUE
            sta         h_st
            clc
            rts
hf_bad:
            LDAX        s_hentry
            jsr         h_error_v
            sec
            rts

; ex put at the end of the frame's list (hash-put's several)
hf_add:
            MOVW        et, ex
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         h_make2
            MOVW        h_nc, hv
            ldy         #HF_LAST
            jsr         hf_get
            sta         hv
            stx         hv + 1
            ora         hv + 1
            bne         :+
            lda         h_nc
            ldx         h_nc + 1
            ldy         #HF_FIRST
            jsr         hf_put
            bra         @last
:
            lda         h_nc                                ; (The last's rest: it)
            ldx         h_nc + 1
            ldy         #1
            jsr         cell_set
@last:
            lda         h_nc
            ldx         h_nc + 1
            ldy         #HF_LAST
            jmp         hf_put

; .A, .X = the frame's word at .Y (hf_get); set (hf_put).  The frame: below its code, on the stack's top
hf_get:
            jsr         hf_at
            lda         (nd),y
            pha
            iny
            lda         (nd),y
            tax
            pla
            rts
hf_put:
            pha
            jsr         hf_at
            pla
            sta         (nd),y
            iny
            txa
            sta         (nd),y
            rts
hf_at:
            pha
            sec
            lda         sp
            sbc         #HF_SIZE + 2
            sta         nd
            lda         sp + 1
            sbc         #0
            sta         nd + 1
            pla
            rts

; A hash called, (h key arg...) (eval.inc's ev_hcall): cf the hash, ex its key's value, cr the arguments after it
; (as written).  OUT: C = 1 if there's no room; ex = the value at key: a function there a method (made again with &0
; the hash, in a scope of its own); not one, with arguments after it, the error; or the error that getting it is
h_at:
            jsr         h_enter
            MOVW        h_called, cf
            MOVW        h_v, ex
            lda         cf
            ldx         cf + 1
            jsr         h_open
            lda         ex
            ldx         ex + 1
            jsr         h_get
            jsr         h_is_err
            bcs         @done
            lda         ex
            ldx         ex + 1
            jsr         h_fn_p
            bcc         @fn
            lda         cr                                  ; (Not a function: with arguments after it, an error)
            ora         cr + 1
            beq         @done
            jsr         h_begin_msg                         ; (":k isn't a method: its value isn't a function")
            lda         h_v
            ldx         h_v + 1
            jsr         h_repr
            LDAX        s_hnotmeth
            jsr         out_text
            jmp         h_end_msg
@fn:
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_FUNC
            bne         @done
            jsr         h_method
@done:
            clc
            rts

; (hash-call h key x...) (forms.inc's bi_hash_call): the method at key, to be applied to the values after it.  OUT:
; C = 1 if there's no room; .A = 0: ex the value (an error); .A = 1: cf the method (&0 the hash), the hash and the key
; dropped from the arguments (cn two less)
h_call:
            jsr         h_enter
            jsr         h_arg
            bcs         @value
            jsr         h_arg0
            sta         h_called
            stx         h_called + 1
            lda         ex
            ldx         ex + 1
            jsr         h_get
            jsr         h_is_err
            bcs         @value
            lda         ex
            ldx         ex + 1
            jsr         h_fn_p
            bcc         :+
            LDAX        s_hcallfn
            jsr         h_error
            bra         @value
:
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_FUNC
            bne         :+
            jsr         h_method
:
            MOVW        cf, ex
            jsr         h_shift2
            lda         #1
            clc
            rts
@value:
            lda         #0
            clc
            rts

; Is .A, .X a function (a built-in, a function, one partially applied)?  OUT: C = 0 if it is
h_fn_p:
            lsr
            bcs         @no
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcc         @no
            clc
            rts
@cell:
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

; ex = the function ex (a method) made again, its scope one of its own whose parent is its own: &0 there, the hash
; called (a proxy: h_called, if it was one; else a proxy of h_h made)
h_method:
            lda         h_mem
            beq         @make
            lda         h_called
            ldx         h_called + 1
            bra         @proxy
@make:
            lda         #PK_HASH
            jsr         h_alloc
            ldy         #2
            lda         h_h
            sta         (hp),y
            iny
            lda         h_h + 1
            sta         (hp),y
            iny
            lda         #<FIX(0)
            sta         (hp),y
            iny
            lda         #0
            sta         (hp),y
            iny
            lda         #H_PROXY
            sta         (hp),y
            iny
            lda         #0
            sta         (hp),y
            lda         hv
            ldx         hv + 1
@proxy:
            jsr         h_rpush
            lda         ex                                  ; (The function's scope: the frame's parent)
            ldx         ex + 1
            ldy         #2
            jsr         cell_get
            sta         h_w
            stx         h_w + 1
            lda         #PK_FRAME8                          ; (The frame: its parent, no more, &0 the proxy)
            jsr         h_alloc
            lda         h_w
            sta         (hp)
            ldy         #1
            lda         h_w + 1
            sta         (hp),y
            ldy         #4
            lda         h_amp0
            sta         (hp),y
            iny
            lda         h_amp0 + 1
            sta         (hp),y
            jsr         root_pop
            ldy         #6
            sta         (hp),y
            iny
            txa
            sta         (hp),y
            lda         hv                                  ; (The function made again, its scope the frame)
            ldx         hv + 1
            jsr         h_rpush
            lda         #PK_FUNC
            jsr         h_alloc
            ldy         #0
            jsr         @word
            ldy         #1
            jsr         @word
            ldy         #3
            jsr         @word
            jsr         root_pop
            ldy         #2
            jsr         cell_set
            MOVW        ex, hv
            rts
@word:
            lda         ex                                  ; (ex's word .Y, in hv's)
            ldx         ex + 1
            jsr         cell_get
            jmp         cell_set

; The first two of the cn values at argp dropped (hash-call's hash and key: the rest moved down), cn two less
h_shift2:
            MOVW        nd, argp
            sec
            lda         cn
            sbc         #2
            sta         cn
            sta         h_w
            lda         cn + 1
            sbc         #0
            sta         cn + 1
            sta         h_w + 1
@word:
            lda         h_w
            ora         h_w + 1
            beq         @drop
            ldy         #4
            lda         (nd),y
            sta         (nd)
            iny
            lda         (nd),y
            ldy         #1
            sta         (nd),y
            clc
            lda         nd
            adc         #2
            sta         nd
            bcc         :+
            inc         nd + 1
:
            lda         h_w
            bne         :+
            dec         h_w + 1
:
            dec         h_w
            bra         @word
@drop:
            lda         #2
            jmp         drop

.segment "RODATA4"
; The reserved tags' names (and &0)
tg_lo:      .lobytes    tg_locked, tg_ro, tg_priv, tg_notnil
tg_hi:      .hibytes    tg_locked, tg_ro, tg_priv, tg_notnil
tg_len:     .byte       8, 11, 9, 9
tg_locked:  .byte       "__locked", 0
tg_ro:      .byte       "__read-only", 0
tg_priv:    .byte       "__private", 0
tg_notnil:  .byte       "__not_nil", 0
s_amp0:     .byte       "&0"
s_hfirst:   .byte       "First parameter to '", 0
s_hmust:    .byte       "' must be a hash", 0
s_hentries: .byte       "A hash's entries are a list", 0
s_htofirst: .byte       "First parameter to 'to#' must be a QExpr", 0
s_hentry:   .byte       "A hash's entry is {key value tag...}, not ", 0
s_htagatom: .byte       "A tag must be an atom, not ", 0
s_hinvalid: .byte       "Failed to add tag: invalid parameters ", 0
s_haddnone: .byte       "hash-add-tag failed: entry not found", 0
s_hinvtag:  .byte       "Invalid tag value", 0
s_hkeytag:  .byte       "Key ", 0
s_hkeytag2: .byte       " not found when looking up tag for hash entry", 0
s_hkeynum:  .byte       "A hash key must be an atom, a string or an integer, not ", 0
s_hkeytype: .byte       "Unsupported key type ", 0
s_hreserved: .byte      "Cannot get/set value for reserved hash key ", 0
s_hget_priv: .byte      "hash-get error: cannot access private hash entry", 0
s_hput_ro:  .byte       "hash-put error: cannot modify read-only hash", 0
s_hput_priv: .byte      "hash-put error: cannot access private hash entry", 0
s_hput_ero: .byte       "hash-put error: cannot modify read-only hash entry", 0
s_hput_lock: .byte      "hash-put error: cannot add new entries to a locked hash", 0
s_hnotnil:  .byte       "Cannot set non-nillable field ", 0
s_hnotnil2: .byte       " to NIL.", 0
s_hrem_ro:  .byte       "hash-remove error: cannot modify read-only hash", 0
s_hrem_priv: .byte      "hash-remove error: cannot access private hash entry", 0
s_hrem_lock: .byte      "hash-remove error: cannot remove an entry from a locked hash", 0
s_hcallfn:  .byte       "Second parameter to 'hash-call' must be a key to a member function", 0
s_hnotmeth: .byte       " isn't a method: its value isn't a function", 0

.segment "CODE4"
