; ****************************************************************************
; hashes.inc - hylang's hashes (step 7.4a), in the module's fourth bank: danlang's LHash.  A hash (OT_HASH) is its
; entries, a chain of lists, each a key (an atom, a string or a number), its value and its tags, and the hash's own
; tags, a chain of atoms; both are changed in place (put, remove, the tags), and nothing else of the heap ever is.
; Four tags are reserved: __locked (no entry added or taken out), __read-only (none changed: hash-make-const's),
; __private (an entry only a method has) and __not_nil (an entry never NIL).  hash-call calls a method (a function in
; the hash) with &0 bound to a proxy of the hash (OT_HASH, HF_PROXY: the hash its first value), through which its
; private entries are had; the evaluator applies it (ev_again), so a method's call is like any other's, a tail call
; too.

HS_DEPTH        = 16            ; Hashes in hashes copied (hash-clone, from#), so deep at most
TG_LOCKED       = 0             ; The reserved tags (at_tags + TG_*)
TG_RO           = 2
TG_PRIV         = 4
TG_NOTNIL       = 6

.bss
at_tags:    .res        8                                   ; :__locked, :__read-only, :__private, :__not_nil
sym_amp0:   .res        2                                   ; &0
hs_h:       .res        2                                   ; The hash (not a proxy) ...
hs_mem:     .res        1                                   ;   $80: through a proxy (a method's: its private
                                                            ;   entries open)
hs_k:       .res        2                                   ; A key ...
hs_e:       .res        2                                   ;   its entry (hs_find's) ...
hs_c:       .res        2                                   ;   the pair of the entries' chain that has it ...
hs_p:       .res        2                                   ;   and the pair before (NIL: none)
hs_t:       .res        2                                   ; A tag
hs_w:       .res        2                                   ; A chain being walked (eq_entries')
hs_x:       .res        2                                   ;   and its entry
hs_n:       .res        1                                   ; A count, or a reserved tag (TG_*)
hs_ty:      .res        1                                   ; A key's type (hs_key_eq's)
hs_depth:   .res        1                                   ; hs_clone's, hs_to_list's: hashes nested
.segment "CODE4"

; The reserved tags and &0.  OUT: C = 1, no room
hash_init:
            ldx         #0
@tag:
            phx
            lda         hs_tag_lo,x
            sta         r0
            lda         hs_tag_hi,x
            sta         r0 + 1
            lda         #OT_ATOM
            jsr         intern_counted
            plx
            bcs         @no
            txa
            asl
            tay
            lda         h_v
            sta         at_tags,y
            lda         h_v + 1
            sta         at_tags + 1,y
            inx
            cpx         #4
            bne         @tag
            LDR         r0, hn_amp0
            lda         #OT_SYMBOL
            jsr         intern_counted
            bcs         @no
            MOVR        sym_amp0, h_v
            jsr         sym_local
            stz         hs_depth
            clc
@no:
            rts

; ---- Keys, entries and tags

; h_v, a hash argument: hs_h the hash (a proxy's, the hash it stands for: and hs_mem $80), ev_s too.  OUT: C = 1, it
; isn't one (ev_v an error)
hs_arg:
            stz         hs_mem
            jsr         type_of
            cmp         #TY_HASH
            bne         @not
            jsr         hash_real
            ror         hs_mem
            MOVR        hs_h, h_v
            MOVR        ev_s, h_v
            clc
            rts
@not:
            LDR         r0, hm_not_hash
            jmp         ev_error_text

; C = 1 if ev_x is a key: an atom (not a reserved tag), a string or a number
hs_is_key:
            MOVR        h_v, ev_x
            jsr         type_of
            cmp         #TY_NUM
            beq         @yes
            cmp         #TY_STRING
            beq         @yes
            cmp         #TY_ATOM
            bne         @no
            ldx         #TG_NOTNIL
:
            lda         at_tags,x
            cmp         ev_x
            bne         :+
            lda         at_tags + 1,x
            cmp         ev_x + 1
            beq         @no
:
            dex
            dex
            bpl         :--
@yes:
            sec
            rts
@no:
            clc
            rts

; ev_x, a key argument: hs_k it.  OUT: C = 1, it isn't one (ev_v an error)
hs_key:
            jsr         hs_is_key
            bcc         @not
            MOVR        hs_k, ev_x
            clc
            rts
@not:
            LDR         r0, hm_key
            jmp         ev_error_text

; C = 1 if the key h_v is hs_k: the same atom or number, or strings of the same text
hs_key_eq:
            lda         h_v
            cmp         hs_k
            bne         @diff
            lda         h_v + 1
            cmp         hs_k + 1
            beq         @yes
@diff:
            MOVR        hs_t, h_v
            jsr         type_of
            sta         hs_ty
            MOVR        h_v, hs_k
            jsr         type_of
            cmp         hs_ty
            bne         @no
            cmp         #TY_STRING
            beq         @string
            cmp         #TY_NUM
            bne         @no
            MOVR        nx, hs_t                            ; (Numbers: by value)
            MOVR        h_v, hs_k
            FARN        3, num_cmp
            bra         @same
@string:
            MOVR        fl_b, hs_k                          ; (Strings: by text)
            MOVR        h_v, hs_t
            jsr         str_cmp2
@same:
            cmp         #0
            bne         @no
@yes:
            sec
            rts
@no:
            clc
            rts

; hs_k's entry in the hash hs_h.  OUT: C = 1, found: hs_e the entry, hs_c its pair (the entries chain's), hs_p the
; pair before (NIL: none); C = 0, not: hs_c the chain's last pair (NIL: none)
hs_find:
            LDR         hs_p, NIL
            LDR         hs_c, NIL
            MOVR        h_v, hs_h
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
@next:
            sta         h_v
            stx         h_v + 1
            ora         h_v + 1
            beq         @none
            MOVR        hs_p, hs_c
            MOVR        hs_c, h_v
            jsr         car
            MOVR        hs_e, h_v
            jsr         car
            jsr         hs_key_eq
            bcs         @found
            MOVR        h_v, hs_c
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            bra         @next
@none:
            clc
@found:
            rts

; hs_e's value: h_v (its pair: hs_value_pair)
hs_value:
            jsr         hs_value_pair
            jmp         car
hs_value_pair:
            MOVR        h_v, hs_e
            jmp         cdr

; The entries of the hash h_v: h_v
hs_entries:
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            rts

; C = 1 if the hash hs_h has the tag at_tags + .X (hs_htag), or the entry hs_e has (hs_etag); hs_t the tag
hs_htag:
            jsr         hs_tag_x
            MOVR        h_v, hs_h
            jsr         deref
            ldy         #5
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            bra         hs_has
hs_etag:
            jsr         hs_tag_x
            jsr         hs_value_pair
            jsr         cdr
            ; (falls into hs_has)

; C = 1 if the atom hs_t is in the chain h_v
hs_has:
            lda         h_v + 1
            beq         @no
            jsr         deref
            lda         (h_p)
            cmp         hs_t
            bne         :+
            ldy         #1
            lda         (h_p),y
            cmp         hs_t + 1
            beq         @yes
:
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            bra         hs_has
@yes:
            sec
            rts
@no:
            clc
            rts

; hs_t = the reserved tag at_tags + .X
hs_tag_x:
            lda         at_tags,x
            sta         hs_t
            lda         at_tags + 1,x
            sta         hs_t + 1
            rts

; h_v (a chain, not empty) = its last pair
hs_last:
            jsr         deref
            ldy         #3
            lda         (h_p),y
            beq         @done
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            bra         hs_last
@done:
            rts

; The tag hs_t added to the hash hs_h (hs_hadd), or to its entry hs_e (hs_eadd), at the end, if it hasn't it.  OUT:
; C = 1, no room; C = 0, .A = 1 (added) or 0 (it had it)
hs_eadd:
            jsr         hs_value_pair
            jsr         cdr
            jsr         hs_has
            bcs         hs_had
            jsr         hs_tag_pair
            bcs         hs_added
            MOVR        h_v, hs_e
            jsr         hs_last
            jsr         set_cdr
            bra         hs_one
hs_hadd:
            MOVR        h_v, hs_h
            jsr         deref
            ldy         #5
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         hs_has
            bcs         hs_had
            jsr         hs_tag_pair
            bcs         hs_added
            MOVR        h_v, hs_h
            jsr         deref
            ldy         #5
            lda         (h_p),y
            beq         @first
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         hs_last
            jsr         set_cdr
            bra         hs_one
@first:
            dey
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
hs_one:
            lda         #1
            clc
hs_added:
            rts
hs_had:
            lda         #0
            clc
            rts

; A pair of hs_t and NIL: h_a.  OUT: C = 1, no room
hs_tag_pair:
            MOVR        h_a, hs_t
            LDR         h_d, NIL
            jsr         cons
            bcs         :+
            MOVR        h_a, h_v
:
            rts

; hs_c's entry taken out of the hash hs_h (hs_p the pair before it)
hs_unlink:
            MOVR        h_v, hs_c
            jsr         cdr
            MOVR        h_a, h_v
            lda         hs_p + 1
            beq         @first
            MOVR        h_v, hs_p
            jmp         set_cdr
@first:
            MOVR        h_v, hs_h
            jsr         deref
            ldy         #2
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
            rts

; ---- Get, put, remove

; hs_k's value in the hash hs_h (danlang's Get): ev_v (NIL: none), or an error.  OUT: C = 1, an error
hs_get:
            jsr         hs_find
            bcc         @none
            bit         hs_mem
            bmi         :+
            ldx         #TG_PRIV
            jsr         hs_etag
            bcc         :+
            LDR         r0, hm_get_private
            jmp         ev_error_text
:
            jsr         hs_value
            MOVR        ev_v, h_v
            clc
            rts
@none:
            LDR         ev_v, NIL
            clc
            rts

; hs_k's entry (ev_x, the key) in the hash hs_h (ev_s) set to ev_f (danlang's Put; hs_mem: a member's): ev_v = its
; value before (NIL: none), or an error.  Putting NIL takes the entry out (or, the hash or the entry locked, makes
; its value NIL).  OUT: C = 1, an error
hs_put:
            ldx         #TG_RO
            jsr         hs_htag
            bcc         :+
            LDR         r0, hm_put_ro
            jmp         ev_error_text
:
            jsr         hs_find
            jcs         @found
            ldx         #TG_LOCKED
            jsr         hs_htag
            bcc         :+
            LDR         r0, hm_put_locked
            jmp         ev_error_text
:
            LDR         ev_v, NIL
            lda         ev_f                                ; (A new entry of NIL: none)
            ora         ev_f + 1
            beq         @done
            MOVR        h_a, ev_f                           ; (A new entry: {key value}, after the last)
            LDR         h_d, NIL
            jsr         cons
            bcs         @room
            MOVR        h_a, ev_x
            MOVR        h_d, h_v
            jsr         cons
            bcs         @room
            MOVR        h_a, h_v
            LDR         h_d, NIL
            jsr         cons
            bcs         @room
            MOVR        h_a, h_v
            lda         hs_c + 1
            beq         @first
            MOVR        h_v, hs_c
            jsr         set_cdr
            bra         @done
@first:
            MOVR        h_v, hs_h
            jsr         deref
            ldy         #2
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
@done:
            clc
            rts
@room:
            MOVR        ev_v, err_room
            sec
            rts
@found:
            bit         hs_mem
            bmi         :+
            ldx         #TG_PRIV
            jsr         hs_etag
            bcc         :+
            LDR         r0, hm_put_private
            jmp         ev_error_text
:
            ldx         #TG_RO
            jsr         hs_etag
            bcc         :+
            LDR         r0, hm_put_entry_ro
            jmp         ev_error_text
:
            jsr         hs_value
            MOVR        ev_v, h_v
            lda         ev_f
            ora         ev_f + 1
            bne         @set
            ldx         #TG_NOTNIL                          ; (NIL: not for a __not_nil entry)
            jsr         hs_etag
            bcc         :+
            LDR         r0, hm_not_nil
            jmp         ev_error_text
:
            ldx         #TG_LOCKED
            jsr         hs_htag
            bcs         @set
            ldx         #TG_LOCKED
            jsr         hs_etag
            bcs         @set
            jsr         hs_unlink
            clc
            rts
@set:
            jsr         hs_value_pair
            MOVR        h_a, ev_f
            jsr         set_car
            clc
            rts

; As hs_put, then the atoms of the chain ev_t added to the entry's tags (if it's not read-only, nor private but to
; a member).  OUT: C = 1, an error (ev_v)
hs_put_tags:
            MOVR        hs_k, ev_x
            MOVR        hs_h, ev_s
            jsr         hs_put
            bcs         @done
            lda         ev_t + 1
            beq         @ok
            MOVR        hs_k, ev_x
            MOVR        hs_h, ev_s
            jsr         hs_find
            bcc         @ok
            ldx         #TG_RO
            jsr         hs_etag
            bcs         @ok
            bit         hs_mem
            bmi         @tag
            ldx         #TG_PRIV
            jsr         hs_etag
            bcs         @ok
@tag:
            lda         ev_t + 1
            beq         @ok
            MOVR        h_v, ev_t
            jsr         car
            MOVR        hs_t, h_v
            jsr         type_of
            cmp         #TY_ATOM
            bne         :+
            jsr         hs_eadd
            bcs         @room
:
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            bra         @tag
@ok:
            clc
@done:
            rts
@room:
            MOVR        ev_v, err_room
            sec
            rts

; The entry h_v ({key value}: the value evaluated in ev_e; {key value tag ...}: not, as danlang has it; {tag}: the
; hash's tag, if it's no key's) put in the hash ev_s (hs_mem: a member's): ev_v = the value it had (NIL: none), or
; an error.  Keeps the arguments left (ar_at), ev_w, ev_r, ev_s, ev_e
hs_put_entry:
            jsr         type_of
            cmp         #TY_LIST
            jne         @nil
            jsr         list_of
            MOVR        ev_t, h_v
            lda         ev_t + 1
            jeq         @nil
            jsr         cdr
            lda         h_v + 1
            bne         @pair
            MOVR        h_v, ev_t                           ; ---- {tag}
            jsr         car
            MOVR        hs_t, h_v
            MOVR        hs_k, h_v
            jsr         type_of
            cmp         #TY_ATOM
            bne         @nil
            MOVR        hs_h, ev_s
            jsr         hs_find
            bcs         @nil
            MOVR        hs_t, hs_k
            jsr         hs_hadd
            jcs         @room
@nil:
            LDR         ev_v, NIL
            clc
            rts
@pair:
            MOVR        h_v, ev_t                           ; ---- {key value tag ...}
            jsr         car
            MOVR        ev_x, h_v
            jsr         hs_key
            jcs         @done
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            jsr         car
            MOVR        ev_f, h_v
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            lda         ev_t + 1
            jne         @put
            lda         hs_mem                              ; (Its value evaluated: everything kept meanwhile)
            pha
            PUSHV       ar_at
            PUSHV       ar_end
            PUSHV       ev_w
            PUSHV       ev_r
            PUSHV       ev_s
            PUSHV       ev_x
            PUSHV       ev_e
            MOVR        ev_x, ev_f
            jsr         eval_nested
            POPV        ev_e
            POPV        ev_x
            POPV        ev_s
            POPV        ev_r
            POPV        ev_w
            POPV        ar_end
            POPV        ar_at
            pla
            sta         hs_mem
            MOVR        ev_f, ev_v
            LDR         ev_t, NIL
@put:
            jmp         hs_put_tags
@room:
            MOVR        ev_v, err_room
            sec
@done:
            rts

; ---- Making them

; A new hash, empty: h_v.  OUT: C = 1, no room
hs_new:
            LDR         h_a, NIL
            LDR         h_d, NIL
            lda         #OT_HASH
            ldx         #0
            jmp         make_cell2

; (hash-create), (hash-create entries): a hash of the entries (a list: {key value tag ...} each, its value evaluated
; in this scope; an atom, the hash's tag); (to# entries): the same
bh_create:
            LDR         r0, hm_create_args
            lda         #0
            ldx         #1
            jsr         s_args
            bcs         bh_create_done
            jsr         arg_next
            bcc         bh_entries
            jsr         hs_new
            jcs         bi_no_room
            MOVR        ev_v, h_v
bh_create_done:
            rts
bh_to_hash:
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            bcs         bh_create_done
            jsr         arg_next
bh_entries:
            jsr         type_of
            cmp         #TY_LIST
            beq         :+
            LDR         r0, hm_entries
            jmp         ev_error_text
:
            jsr         list_of
            MOVR        ev_r, h_v
            jsr         hs_new
            jcs         bi_no_room
            MOVR        ev_s, h_v
            lda         #$80
            sta         hs_mem
@entry:
            lda         ev_r + 1
            jeq         @made
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_x, h_v
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            MOVR        h_v, ev_x
            jsr         type_of
            cmp         #TY_ATOM
            jeq         @tag
            cmp         #TY_LIST
            bne         @entry
            MOVR        h_v, ev_x                           ; (An entry: two items at least, a key first)
            jsr         list_of
            MOVR        ev_t, h_v
            jsr         cdr
            lda         h_v + 1
            beq         @entry
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            jsr         hs_is_key
            bcc         @entry
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            PUSHV       ev_r                                ; (Its value, evaluated)
            PUSHV       ev_s
            PUSHV       ev_t
            PUSHV       ev_x
            PUSHV       ev_e
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            jsr         eval_nested
            POPV        ev_e
            POPV        ev_x
            POPV        ev_t
            POPV        ev_s
            POPV        ev_r
            MOVR        ev_f, ev_v
            MOVR        h_v, ev_t                           ; (Its tags)
            jsr         cdr
            MOVR        ev_t, h_v
            lda         #$80
            sta         hs_mem
            jsr         hs_put_tags
            jcc         @entry
            lda         ev_v                                ; (No room: an error; another, as danlang has it, the
                                                            ; entry's not put)
            cmp         err_room
            jne         @entry
            lda         ev_v + 1
            cmp         err_room + 1
            jne         @entry
            rts
@tag:
            MOVR        hs_t, ev_x
            MOVR        hs_h, ev_s
            jsr         hs_hadd
            jcc         @entry
            jmp         bi_no_room
@made:
            MOVR        ev_v, ev_s
            rts

; (hash-clone h), (hash-clone h overrides): a copy of the hash (its entries, a hash among their values copied too;
; not its tags), the overrides put in it (as written: {key value tag ...}, a list of them, or a tag)
bh_clone:
            LDR         r0, hm_clone_args
            lda         #1
            ldx         #255
            jsr         s_args
            jcs         @done
            jsr         arg_next
            jsr         hs_arg
            jcs         @done
            jsr         args_list                           ; (The overrides)
            jcs         @room
            MOVR        ev_f, h_v
            MOVR        h_v, ev_s
            jsr         hs_clone
            jcs         @done
            MOVR        ev_s, ev_v
            lda         #$80
            sta         hs_mem
@unwrap:
            MOVR        h_v, ev_f                           ; ---- One: what's in it
            jsr         count_h
            cmp         #1
            jne         @several
            MOVR        h_v, ev_f
            jsr         car
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_LIST
            bne         @one
            MOVR        h_v, ev_x
            jsr         list_of
            MOVR        ev_f, h_v
            bra         @unwrap
@one:
            cmp         #TY_ATOM                            ; (A tag)
            jne         @made
            MOVR        hs_t, ev_x
            MOVR        hs_h, ev_s
            jsr         hs_hadd
            jcs         @room
            jmp         @made
@several:
            lda         ev_f + 1
            jeq         @made
            MOVR        h_v, ev_f                           ; ---- {key value tag ...}: one entry
            jsr         car
            MOVR        ev_x, h_v
            jsr         hs_is_key
            bcc         @each
            MOVR        ev_t, ev_f
            jsr         bh_raw
            jmp         @made
@each:
            lda         ev_f + 1                            ; ---- Else each: an entry, or a tag
            jeq         @made
            MOVR        h_v, ev_f
            jsr         car
            MOVR        ev_x, h_v
            MOVR        h_v, ev_f
            jsr         cdr
            MOVR        ev_f, h_v
            MOVR        h_v, ev_x
            jsr         type_of
            cmp         #TY_ATOM
            beq         @tag
            cmp         #TY_LIST
            bne         @each
            MOVR        h_v, ev_x
            jsr         list_of
            MOVR        ev_t, h_v
            jsr         cdr
            lda         h_v + 1
            bne         @entry
            MOVR        h_v, ev_t                           ; ({tag})
            jsr         car
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_ATOM
            bne         @each
@tag:
            MOVR        hs_t, ev_x
            MOVR        hs_h, ev_s
            jsr         hs_hadd
            bcs         @room
            jmp         @each
@entry:
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            jsr         hs_is_key
            jcc         @each
            PUSHV       ev_f
            jsr         bh_raw
            POPV        ev_f
            jmp         @each
@made:
            MOVR        ev_v, ev_s
@done:
            rts
@room:
            jmp         bi_no_room

; The entry ev_t ({key value tag ...}, the key ev_x) put in ev_s as written (a member's)
bh_raw:
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            jsr         car
            MOVR        ev_f, h_v
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            lda         #$80
            sta         hs_mem
            jmp         hs_put_tags

; ev_v = a copy of the hash h_v (not a proxy): its entries, each chain copied, a hash among the values copied too;
; not its tags.  OUT: C = 1, no room, or nested too deeply (ev_v an error)
hs_clone:
            lda         hs_depth
            cmp         #HS_DEPTH
            bcc         :+
            LDR         r0, hm_deep
            jmp         ev_error_text
:
            inc         hs_depth
            jsr         hs_entries
            MOVR        ev_s, h_v                           ; (Its entries, to go)
            LDR         ev_w, NIL                           ; (The copies: ev_w, its last pair ev_r)
            LDR         ev_r, NIL
@entry:
            lda         ev_s + 1
            beq         @hash
            MOVR        h_v, ev_s
            jsr         car
            MOVR        ev_x, h_v
            MOVR        h_v, ev_s
            jsr         cdr
            MOVR        ev_s, h_v
            LDR         ev_a, NIL
            LDR         ev_t, NIL
@item:
            lda         ev_x + 1
            beq         @copied
            MOVR        h_v, ev_x
            jsr         car
            MOVR        ev_v, h_v
            jsr         ev_append
            jcs         @room
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
            bra         @item
@copied:
            MOVR        ev_v, ev_a
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            jcs         @room
            jmp         @entry
@hash:
            MOVR        h_a, ev_w
            LDR         h_d, NIL
            lda         #OT_HASH
            ldx         #0
            jsr         make_cell2
            jcs         @room
            MOVR        ev_s, h_v
            MOVR        ev_x, ev_w                          ; ---- A hash among the values: copied
@value:
            lda         ev_x + 1
            beq         @done
            MOVR        h_v, ev_x
            jsr         car
            jsr         cdr
            MOVR        ev_t, h_v
            jsr         car
            jsr         type_of
            cmp         #TY_HASH
            bne         @next
            PUSHV       ev_s
            PUSHV       ev_x
            PUSHV       ev_t
            jsr         hash_real
            jsr         hs_clone
            php
            POPV        ev_t
            POPV        ev_x
            POPV        ev_s
            plp
            bcs         @fail
            MOVR        h_v, ev_t
            MOVR        h_a, ev_v
            jsr         set_car
@next:
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
            bra         @value
@done:
            MOVR        ev_v, ev_s
            dec         hs_depth
            clc
            rts
@room:
            MOVR        ev_v, err_room
@fail:
            dec         hs_depth
            sec
            rts

; (from# h): the hash as a list: {key value tag ...} for each entry (a hash its value: as a list too), then its tags
bh_from_hash:
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            MOVR        h_v, ev_s
            jmp         hs_to_list
@done:
            rts

; ev_v = the hash h_v (not a proxy) as a list, as from# has it.  OUT: C = 1, no room, or nested too deeply (ev_v an
; error)
hs_to_list:
            lda         hs_depth
            cmp         #HS_DEPTH
            bcc         :+
            LDR         r0, hm_deep
            jmp         ev_error_text
:
            inc         hs_depth
            MOVR        ev_f, h_v
            jsr         hs_entries
            MOVR        ev_s, h_v
            LDR         ev_w, NIL                           ; (The list: ev_w, its last pair ev_r)
            LDR         ev_r, NIL
@entry:
            lda         ev_s + 1
            jeq         @tags
            MOVR        h_v, ev_s
            jsr         car
            MOVR        ev_x, h_v
            MOVR        h_v, ev_s
            jsr         cdr
            MOVR        ev_s, h_v
            LDR         ev_a, NIL                           ; (The entry's: ev_a, its last pair ev_t)
            LDR         ev_t, NIL
            MOVR        h_v, ev_x                           ; (Its key)
            jsr         car
            MOVR        ev_v, h_v
            jsr         ev_append
            jcs         @room
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
            jsr         car                                 ; (Its value: a hash, as a list)
            MOVR        ev_v, h_v
            jsr         type_of
            cmp         #TY_HASH
            bne         @add
            PUSHV       ev_f
            PUSHV       ev_s
            PUSHV       ev_w
            PUSHV       ev_r
            PUSHV       ev_a
            PUSHV       ev_t
            PUSHV       ev_x
            MOVR        h_v, ev_v
            jsr         hash_real
            jsr         hs_to_list
            php
            POPV        ev_x
            POPV        ev_t
            POPV        ev_a
            POPV        ev_r
            POPV        ev_w
            POPV        ev_s
            POPV        ev_f
            plp
            jcs         @fail
@add:
            jsr         ev_append
            jcs         @room
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
@tag:
            lda         ev_x + 1                            ; (Its tags)
            beq         @made
            MOVR        h_v, ev_x
            jsr         car
            MOVR        ev_v, h_v
            jsr         ev_append
            jcs         @room
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
            bra         @tag
@made:
            MOVR        h_v, ev_a
            lda         #OT_QEXPR
            jsr         make_list
            bcs         @room
            MOVR        ev_v, h_v
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            bcs         @room
            jmp         @entry
@tags:
            MOVR        h_v, ev_f                           ; (The hash's tags)
            jsr         deref
            ldy         #5
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_x
            stx         ev_x + 1
@htag:
            lda         ev_x + 1
            beq         @list
            MOVR        h_v, ev_x
            jsr         car
            MOVR        ev_v, h_v
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            bcs         @room
            MOVR        h_v, ev_x
            jsr         cdr
            MOVR        ev_x, h_v
            bra         @htag
@list:
            MOVR        h_v, ev_w
            lda         #OT_QEXPR
            jsr         make_list
            bcs         @room
            MOVR        ev_v, h_v
            dec         hs_depth
            clc
            rts
@room:
            MOVR        ev_v, err_room
@fail:
            dec         hs_depth
            sec
            rts

; ---- The built-ins

; (hash-get h key): key's value in the hash (NIL: none; and if h is NIL)
bh_get:
            LDR         r0, hm_two
            lda         #2
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            lda         h_v
            ora         h_v + 1
            beq         @nil
            jsr         hs_arg
            bcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         hs_key
            bcs         @done
            jmp         hs_get
@nil:
            LDR         ev_v, NIL
@done:
            rts

; (hash-put h entry ...): each entry ({key value}, the value evaluated; {key value tag ...}; {tag}) put in the
; hash: the value it had (NIL: none), or an error; for more than one entry, a list of them
bh_put:
            LDR         r0, hm_put_args
            lda         #2
            ldx         #255
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            jsr         count_a
            cmp         #1
            bne         @many
            jsr         arg_next
            jmp         hs_put_entry
@many:
            LDR         ev_w, NIL                           ; (Their values: ev_w, its last pair ev_r)
            LDR         ev_r, NIL
@next:
            jsr         arg_next
            bcs         @list
            lda         hs_mem
            pha
            jsr         hs_put_entry
            pla
            sta         hs_mem
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            bcc         @next
            jmp         bi_no_room
@list:
            MOVR        h_v, ev_w
            jmp         s_qexpr
@done:
            rts

; (hash-remove h key): the entry taken out of the hash: its value (NIL: none)
bh_remove:
            LDR         r0, hm_two
            lda         #2
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         hs_key
            bcs         @done
            ldx         #TG_RO
            jsr         hs_htag
            bcc         :+
            LDR         r0, hm_remove_ro
            jmp         ev_error_text
:
            jsr         hs_find
            bcs         :+
            LDR         ev_v, NIL
            rts
:
            bit         hs_mem
            bmi         :+
            ldx         #TG_PRIV
            jsr         hs_etag
            bcc         :+
            LDR         r0, hm_remove_private
            jmp         ev_error_text
:
            ldx         #TG_LOCKED
            jsr         hs_htag
            bcs         @locked
            ldx         #TG_RO
            jsr         hs_etag
            bcs         @locked
            jsr         hs_value
            MOVR        ev_v, h_v
            jmp         hs_unlink
@locked:
            LDR         r0, hm_remove_locked
            jmp         ev_error_text
@done:
            rts

; (hash-key? h key): T if the hash has the key
bh_key_p:
            LDR         r0, hm_two
            lda         #2
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         hs_key
            bcs         @done
            jsr         hs_find
            jmp         bi_bool
@done:
            rts

; (hash-keys h), (hash-values h): the hash's keys, or its values, a list
bh_keys:
            lda         #0
            bra         bh_list
bh_values:
            lda         #1
bh_list:
            sta         hs_n
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            MOVR        h_v, ev_s
            jsr         hs_entries
            MOVR        ev_s, h_v
            LDR         ev_w, NIL
            LDR         ev_r, NIL
@entry:
            lda         ev_s + 1
            beq         @list
            MOVR        h_v, ev_s
            jsr         car
            lda         hs_n
            beq         :+
            jsr         cdr
:
            jsr         car
            MOVR        ev_v, h_v
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            jcs         bi_no_room
            MOVR        h_v, ev_s
            jsr         cdr
            MOVR        ev_s, h_v
            bra         @entry
@list:
            MOVR        h_v, ev_w
            jmp         s_qexpr
@done:
            rts

; (hash-call h key arg ...): the method key (a function in the hash) applied to the arguments, &0 the hash (a proxy
; of it, through which its private entries are had)
bh_call:
            LDR         r0, hm_call_args
            lda         #2
            ldx         #255
            jsr         s_args
            jcs         @done
            jsr         arg_next
            jsr         hs_arg
            jcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         hs_key
            jcs         @done
            jsr         hs_get
            jcs         @done
            MOVR        h_v, ev_v
            jsr         type_of
            cmp         #TY_FUNC
            beq         :+
            LDR         r0, hm_call_fn
            jmp         ev_error_text
:
            MOVR        ev_f, ev_v
            lda         ev_f + 1
            cmp         #>BUILTIN_END
            jcc         @apply
            MOVR        h_a, ev_s                           ; (&0: a proxy of the hash)
            LDR         h_d, NIL
            lda         #OT_HASH
            ldx         #HF_PROXY
            jsr         make_cell2
            jcs         @room
            MOVR        ev_t, h_v
            MOVR        h_v, ev_f                           ; (The function: a copy, &0 bound first)
            jsr         deref
            ldy         #8
            lda         (h_p),y
            sta         h_d
            iny
            lda         (h_p),y
            sta         h_d + 1
            MOVR        h_a, ev_t
            jsr         cons
            bcs         @room
            MOVR        h_a, sym_amp0
            MOVR        h_d, h_v
            jsr         cons
            bcs         @room
            MOVR        ev_t, h_v
            ldy         #PK_C16
            jsr         alloc
            bcs         @room
            MOVR        ev_w, h_v
            MOVR        h_v, ev_f                           ; (Its bytes, through msg: the two may be in different
            jsr         deref                               ; banks)
            ldy         #9
:
            lda         (h_p),y
            sta         msg,y
            dey
            bpl         :-
            lda         ev_t
            sta         msg + 8
            lda         ev_t + 1
            sta         msg + 9
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #9
:
            lda         msg,y
            sta         (h_p),y
            dey
            bpl         :-
            MOVR        ev_f, ev_w
@apply:
            lda         #1                                  ; (The evaluator applies it, to the arguments left)
            sta         ev_again
@done:
            rts
@room:
            jmp         bi_no_room

; (hash-add-tag h tag ...): each tag added: an atom, the hash's; {key tag}, the entry's.  T if it's added (NIL if
; it was there); for more than one, a list of them
bh_add_tag:
            LDR         r0, hm_tag_args
            lda         #2
            ldx         #255
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            jsr         count_a
            cmp         #1
            bne         @many
            jsr         arg_next
            jmp         hs_add_tag
@many:
            LDR         ev_w, NIL
            LDR         ev_r, NIL
@next:
            jsr         arg_next
            bcs         @list
            jsr         hs_add_tag
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            bcc         @next
            jmp         bi_no_room
@list:
            MOVR        h_v, ev_w
            jmp         s_qexpr
@done:
            rts

; The tag h_v added to the hash ev_s (danlang's AddTag): an atom, the hash's; {key tag}, the entry's (an error if
; it has none); {tag}, nothing.  ev_v = T (added) or NIL, or an error
hs_add_tag:
            MOVR        hs_h, ev_s
            MOVR        hs_t, h_v
            jsr         type_of
            cmp         #TY_ATOM
            beq         @hash
            cmp         #TY_LIST
            bne         @wrong
            MOVR        h_v, hs_t
            jsr         list_of
            MOVR        ev_x, h_v
            jsr         count_h
            cmp         #1
            beq         @nil
            cmp         #2
            bne         @wrong
            MOVR        h_v, ev_x                           ; ({key tag})
            jsr         car
            MOVR        hs_k, h_v
            jsr         hs_find
            bcc         @none
            MOVR        h_v, ev_x
            jsr         cdr
            jsr         car
            MOVR        hs_t, h_v
            jsr         type_of
            cmp         #TY_ATOM
            bne         @nil
            jsr         hs_eadd
            bra         @added
@hash:
            jsr         hs_hadd
@added:
            bcs         @room
            lsr
            jmp         bi_bool
@nil:
            LDR         ev_v, NIL
            rts
@none:
            LDR         r0, hm_tag_none
            jmp         ev_error_text
@wrong:
            LDR         r0, hm_tag_wrong
            jmp         ev_error_text
@room:
            jmp         bi_no_room

; (hash-lock h key ...), (hash-make-const ...), (hash-make-private ...), (hash-make-not-nil ...): the tag __locked
; (__read-only, __private, __not_nil) added to the hash (no keys), or to each key's entry
bh_lock:
            ldx         #TG_LOCKED
            bra         bh_apply
bh_const:
            ldx         #TG_RO
            bra         bh_apply
bh_private:
            ldx         #TG_PRIV
            bra         bh_apply
bh_not_nil:
            ldx         #TG_NOTNIL
bh_apply:
            stx         hs_n
            LDR         r0, hm_lock_args
            lda         #1
            ldx         #255
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            ldx         hs_n
            jsr         hs_tag_x
            jsr         count_a
            cmp         #1
            beq         @key
            bcs         @many
            jsr         hs_hadd                             ; (No keys: the hash's)
            bcs         @room
            lsr
            jmp         bi_bool
@key:
            jsr         arg_next
            jmp         hs_key_tag
@many:
            LDR         ev_w, NIL
            LDR         ev_r, NIL
@next:
            jsr         arg_next
            bcs         @list
            jsr         hs_key_tag
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            bcc         @next
@room:
            jmp         bi_no_room
@list:
            MOVR        h_v, ev_w
            jmp         s_qexpr
@done:
            rts

; The reserved tag hs_n (at_tags + hs_n) added to the entry of the key h_v in the hash ev_s: ev_v = T (added) or NIL,
; or an error
hs_key_tag:
            MOVR        ev_x, h_v
            jsr         hs_key
            bcs         @done
            MOVR        hs_h, ev_s
            jsr         hs_find
            bcc         @none
            ldx         hs_n
            jsr         hs_tag_x
            jsr         hs_eadd
            bcs         @room
            lsr
            jmp         bi_bool
@none:
            LDR         r0, hm_tag_none
            jmp         ev_error_text
@room:
            jmp         bi_no_room
@done:
            rts

; (hash-tag? h tag): T if the hash has the tag (an atom, or {tag}), or the entry ({key tag}) has
bh_tag_p:
            LDR         r0, hm_two
            lda         #2
            tax
            jsr         s_args
            jcs         @done
            jsr         arg_next
            jsr         hs_arg
            jcs         @done
            jsr         arg_next
            MOVR        hs_t, h_v
            jsr         type_of
            cmp         #TY_ATOM
            beq         @hash
            cmp         #TY_LIST
            jne         @wrong
            MOVR        h_v, hs_t
            jsr         list_of
            MOVR        ev_x, h_v
            jsr         count_h
            cmp         #1
            beq         @one
            cmp         #2
            bne         @wrong
            MOVR        h_v, ev_x                           ; ({key tag})
            jsr         car
            MOVR        hs_k, h_v
            jsr         hs_find
            bcc         @none
            MOVR        h_v, ev_x
            jsr         cdr
            jsr         car
            MOVR        hs_t, h_v
            jsr         hs_value_pair
            jsr         cdr
            jsr         hs_has
            jmp         bi_bool
@one:
            MOVR        h_v, ev_x                           ; ({tag})
            jsr         car
            MOVR        hs_t, h_v
@hash:
            jsr         hs_htag_t
            jmp         bi_bool
@none:
            LDR         r0, hm_tag_none
            jmp         ev_error_text
@wrong:
            LDR         r0, hm_tag_wrong
            jmp         ev_error_text
@done:
            rts

; C = 1 if the hash hs_h has the tag hs_t
hs_htag_t:
            MOVR        h_v, hs_h
            jsr         deref
            ldy         #5
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jmp         hs_has

; (hash-locked? h), (hash-locked? h key), and hash-private?'s, hash-const?'s: T if the hash (or the key's entry)
; has the tag __locked (__private, __read-only)
bh_locked_p:
            ldx         #TG_LOCKED
            bra         bh_has
bh_private_p:
            ldx         #TG_PRIV
            bra         bh_has
bh_const_p:
            ldx         #TG_RO
bh_has:
            stx         hs_n
            LDR         r0, hm_has_args
            lda         #1
            ldx         #2
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         hs_arg
            bcs         @done
            jsr         arg_next
            bcc         @key
            ldx         hs_n
            jsr         hs_htag
            jmp         bi_bool
@key:
            MOVR        ev_x, h_v
            jsr         hs_key
            bcs         @done
            jsr         hs_find
            bcc         @none
            ldx         hs_n
            jsr         hs_etag
            jmp         bi_bool
@none:
            LDR         r0, hm_tag_none
            jmp         ev_error_text
@done:
            rts

; ---- Equality (equal's, the second bank's)

; Whether the hashes eq_a and eq_b (not proxies) have the same keys: if they have, the pairs of their values pushed,
; for equal to compare.  OUT: C = 1, they have
eq_entries:
            MOVR        h_v, eq_a
            jsr         hs_entries
            jsr         count_h
            sta         hs_n
            MOVR        h_v, eq_b
            jsr         hs_entries
            jsr         count_h
            cmp         hs_n
            jne         @no
            MOVR        hs_h, eq_b
            MOVR        h_v, eq_a
            jsr         hs_entries
            MOVR        hs_w, h_v
@entry:
            lda         hs_w + 1
            beq         @yes
            MOVR        h_v, hs_w
            jsr         car
            MOVR        hs_x, h_v
            jsr         car
            MOVR        hs_k, h_v
            jsr         hs_find
            bcc         @no
            MOVR        h_v, hs_x                           ; (Its value, and the other's)
            jsr         cdr
            jsr         car
            PUSHV       h_v
            jsr         hs_value
            PUSHV       h_v
            inc         eq_n
            bne         :+
            inc         eq_n + 1
:
            MOVR        h_v, hs_w
            jsr         cdr
            MOVR        hs_w, h_v
            bra         @entry
@yes:
            sec
            rts
@no:
            clc
            rts

.segment "RODATA4"
hs_tag_lo:  .lobytes    hn_locked, hn_ro, hn_priv, hn_notnil
hs_tag_hi:  .hibytes    hn_locked, hn_ro, hn_priv, hn_notnil
hn_locked:  .byte       8, "__locked"
hn_ro:      .byte       11, "__read-only"
hn_priv:    .byte       9, "__private"
hn_notnil:  .byte       9, "__not_nil"
hn_amp0:    .byte       2, "&0"
hm_not_hash: .byte      "First parameter must be a hash", 0
hm_key:     .byte       "Unsupported key type (a key is an atom, a string or a number)", 0
hm_two:     .byte       "Expects 2 parameters", 0
hm_entries: .byte       "Hash entries must be a QExpr", 0
hm_create_args: .byte   "'hash-create' expects maybe a list of entries", 0
hm_clone_args: .byte    "'hash-clone' expects a hash and maybe overrides", 0
hm_put_args: .byte      "'hash-put' requires two or more parameters", 0
hm_call_args: .byte     "'hash-call' requires a hash and a key", 0
hm_call_fn: .byte       "Second parameter to 'hash-call' must be a key to a member function", 0
hm_tag_args: .byte      "'hash-add-tag' requires two or more parameters", 0
hm_lock_args: .byte     "Expects a hash and maybe keys", 0
hm_has_args: .byte      "Expects a hash and maybe a key", 0
hm_tag_none: .byte      "Hash entry not found", 0
hm_tag_wrong: .byte     "Invalid tag value", 0
hm_get_private: .byte   "hash-get error: cannot access private hash entry", 0
hm_put_ro:  .byte       "hash-put error: cannot modify read-only hash", 0
hm_put_locked: .byte    "hash-put error: cannot add new entries to a locked hash", 0
hm_put_private: .byte   "hash-put error: cannot access private hash entry", 0
hm_put_entry_ro: .byte  "hash-put error: cannot modify read-only hash entry", 0
hm_not_nil: .byte       "Cannot set a non-nillable field to NIL", 0
hm_remove_ro: .byte     "hash-remove error: cannot modify read-only hash", 0
hm_remove_private: .byte "hash-remove error: cannot access private hash entry", 0
hm_remove_locked: .byte "hash-remove error: cannot remove an entry from a locked hash", 0
hm_deep:    .byte       "Hashes nested too deeply", 0
.segment "CODE4"
