; ****************************************************************************
; eqcmp.inc - hylang's equality and order (phase 3), in its second bank: danlang's (reference.md, section 2; LVal's
; Equals and CompareTo).  eq: numbers by value (as yet fixnums), characters, strings, atoms and symbols by their text,
; lists item by item (an S-expression isn't a Q-expression), T and exit as themselves, errors by their messages,
; functions by their formals and body, built-ins by which and the values given them.  cmp: by kind first (numbers,
; characters, strings, atoms, symbols, Q-expressions, S-expressions, T, functions, hashes, streams, errors), then by
; value; a list item by item, one that's the start of another first; two different functions as they print.
;   Lists nest as deep as they like: the pairs still to be compared are on the evaluation stack (q_n of them), not
; the 65C02's.  A scoped value is compared as its expression.

.segment "CODE2"

; ---- eq

; (eq x y), (== x y): T if they're equal
bq_eq:
            jsr         q_args
            jsr         q_equal
            jmp         q_bool
; (neq x y): NIL if they're equal
bq_neq:
            jsr         q_args
            jsr         q_equal
            php
            pla
            eor         #1                                  ; (C the other way)
            pha
            plp
            jmp         q_bool

; (elem? x l), (in? x l): T if an item of l is equal to x
bq_elem:
bq_in:
            ldy         #2                                  ; (l: a list)
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            jsr         q_conses
            bcc         :+
            LDAX        s_qelem
            jmp         l_error
:
            sta         q_items
            stx         q_items + 1
@item:
            lda         q_items
            ora         q_items + 1
            bne         :+
            clc
            jmp         q_bool
:
            lda         q_items
            ldx         q_items + 1
            jsr         l_car
            sta         q_b
            stx         q_b + 1
            lda         (argp)
            sta         q_a
            ldy         #1
            lda         (argp),y
            sta         q_a + 1
            jsr         q_equal
            bcs         q_bool
            lda         q_items
            ldx         q_items + 1
            jsr         l_cdr
            sta         q_items
            stx         q_items + 1
            bra         @item

; q_a, q_b = the arguments 0 and 1
q_args:
            lda         (argp)
            sta         q_a
            ldy         #1
            lda         (argp),y
            sta         q_a + 1
            iny
            lda         (argp),y
            sta         q_b
            iny
            lda         (argp),y
            sta         q_b + 1
            rts

; ex = T if C = 1, else NIL; C = 0
q_bool:
            stz         ex + 1
            lda         #0
            bcc         :+
            lda         #<T_VAL
:
            sta         ex
            clc
            rts

; OUT: C = 1 if q_a and q_b are equal (each pair still to compare on the stack)
q_equal:
            stz         q_n
            stz         q_n + 1
            jsr         q_push
@pair:
            lda         q_n
            ora         q_n + 1
            bne         :+
            sec                                             ; (None left: equal)
            rts
:
            jsr         q_pop
            jsr         q_same
            bcc         @differ
            bvc         @pair                               ; (V: a list's, its items pushed to compare)
            bra         @pair
@differ:
            jsr         q_drop
            clc
            rts

; The pair q_a, q_b pushed
q_push:
            lda         q_a
            ldx         q_a + 1
            jsr         push
            lda         q_b
            ldx         q_b + 1
            jsr         push
            inc         q_n
            bne         :+
            inc         q_n + 1
:
            rts

; q_a, q_b = a pair popped
q_pop:
            jsr         pop
            sta         q_b
            stx         q_b + 1
            jsr         pop
            sta         q_a
            stx         q_a + 1
            lda         q_n
            bne         :+
            dec         q_n + 1
:
            dec         q_n
            rts

; The pairs left dropped
q_drop:
            lda         q_n
            ora         q_n + 1
            beq         @done
            jsr         q_pop
            bra         q_drop
@done:
            rts

; q_a, q_b: a scoped one seen as its expression
q_seen:
            lda         q_a
            ldx         q_a + 1
            jsr         q_unscope
            sta         q_a
            stx         q_a + 1
            lda         q_b
            ldx         q_b + 1
            jsr         q_unscope
            sta         q_b
            stx         q_b + 1
            rts

; IN: .A, .X = a value.  OUT: .A, .X = it, or, scoped, its expression
q_unscope:
            pha
            jsr         kind_of
            cmp         #PK_SCOPED
            beq         :+
            pla
            rts
:
            pla
            ldy         #0
            jmp         cell_get

; q_a and q_b compared, as far as they are themselves (two lists: their firsts and their rests pushed).  OUT: C = 1
; if they may be equal (V = 1: pushed), C = 0 if they aren't
q_same:
            jsr         q_seen
            lda         q_a                                 ; (The same value: equal)
            cmp         q_b
            bne         :+
            lda         q_a + 1
            cmp         q_b + 1
            bne         :+
            sec
            clv
            rts
:
            lda         q_a                                 ; (Their kinds: q_ka, q_kb)
            ldx         q_a + 1
            jsr         q_kind
            sta         q_ka
            lda         q_b
            ldx         q_b + 1
            jsr         q_kind
            sta         q_kb
            cmp         q_ka
            bne         @no
            cmp         #QK_QLIST
            beq         @list
            cmp         #QK_SLIST
            beq         @list
            cmp         #QK_STRING
            bne         @lb205
            jmp         @string
@lb205:
            cmp         #QK_ERROR
            bne         @lb204
            jmp         @error
@lb204:
            cmp         #QK_FUNCTION
            bne         @lb26
            jmp         @function
@lb26:
            cmp         #QK_NUMBER
            bne         @no
            lda         q_a                                 ; (Two numbers: by value, but two fixnums
            and         q_b                                 ;   aren't the same value)
            lsr
            bcs         @no
            FARN        3, n_qcmp
            lda         q_r
            bne         @no
            sec
            clv
            rts
@no:
            clc                                             ; (Anything else: equal only if it's the same value)
            rts
@list:
            lda         q_a                                 ; (Two lists: both empty, or each a cons)
            ora         q_a + 1
            beq         @empty
            lda         q_b
            ora         q_b + 1
            beq         @no
            lda         q_a                                 ; (Either (): no conses)
            ldx         q_a + 1
            jsr         list_kind
            cmp         #1
            beq         @nocons
            lda         q_b
            ldx         q_b + 1
            jsr         list_kind
            cmp         #1
            beq         @no
            jsr         q_rests                             ; (Their rests, then their firsts)
            jsr         q_firsts
            jsr         q_push
            sec
            bit         q_vbit
            rts
@nocons:
            lda         q_b                                 ; (One () and the other: () only)
            ldx         q_b + 1
            jsr         list_kind
            cmp         #1
            bne         @no
            sec
            clv
            rts
@empty:
            lda         q_b
            ora         q_b + 1
            bne         @no
            sec
            clv
            rts
@string:
            jsr         q_strcmp
            bne         @no
            sec
            clv
            rts
@error:
            lda         q_a                                 ; (Their messages)
            ldx         q_a + 1
            ldy         #0
            jsr         cell_get
            sta         q_a
            stx         q_a + 1
            lda         q_b
            ldx         q_b + 1
            ldy         #0
            jsr         cell_get
            sta         q_b
            stx         q_b + 1
            bra         @string
@function:
            lda         q_a + 1                             ; (Built-ins: the same one, or none)
            cmp         #IMM_PAGES
            bcs         @lb25
            jmp         @no
@lb25:
            lda         q_b + 1
            cmp         #IMM_PAGES
            bcs         @lb24
            jmp         @no
@lb24:
            tax                                             ; (Both cells: the same kind)
            lda         pk,x
            sta         q_kb
            ldx         q_a + 1
            lda         pk,x
            cmp         q_kb
            beq         @lb23
            jmp         @no
@lb23:
                                                            ; (Partially applied: the built-in, the values;
                                                            ;   a function: its formals, its body: words 0, 1)
            jsr         q_rests                             ; (Their words 1 pushed, then their words 0)
            jsr         q_firsts
            jsr         q_push
            sec
            bit         q_vbit
            rts
q_vbit:     .byte       $40

; q_a, q_b = their words 0 (q_firsts); or their words 1 pushed, as a pair, q_a and q_b kept (q_rests)
q_firsts:
            lda         q_a
            ldx         q_a + 1
            ldy         #0
            jsr         cell_get
            pha
            phx
            lda         q_b
            ldx         q_b + 1
            ldy         #0
            jsr         cell_get
            sta         q_b
            stx         q_b + 1
            plx
            pla
            sta         q_a
            stx         q_a + 1
            rts
q_rests:
            lda         q_a                                 ; (q_a, q_b kept: q_firsts reads them after)
            ldx         q_a + 1
            ldy         #1
            jsr         cell_get
            sta         q_a2
            stx         q_a2 + 1
            lda         q_b
            ldx         q_b + 1
            ldy         #1
            jsr         cell_get
            sta         q_b2
            stx         q_b2 + 1
            lda         q_a                                 ; (Pushed as the pair q_a2, q_b2; q_a, q_b kept)
            pha
            lda         q_a + 1
            pha
            lda         q_b
            pha
            lda         q_b + 1
            pha
            MOVW        q_a, q_a2
            MOVW        q_b, q_b2
            jsr         q_push
            pla
            sta         q_b + 1
            pla
            sta         q_b
            pla
            sta         q_a + 1
            pla
            sta         q_a
            rts

; IN: .A, .X = a value.  OUT: .A = its kind, as eq and cmp see it (QK_*: in cmp's order)
QK_NUMBER       = 0
QK_CHAR         = 1
QK_STRING       = 2
QK_ATOM         = 3
QK_SYMBOL       = 4
QK_QLIST        = 5
QK_SLIST        = 6
QK_T            = 7
QK_FUNCTION     = 8
QK_HASH         = 9
QK_STREAM       = 10
QK_ERROR        = 11
QK_OTHER        = 12
q_kind:
            pha
            lsr
            pla
            bcs         @number
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcs         @function
            cpx         #>CHAR0
            bcs         @char
            cpx         #0
            bne         @other
            cmp         #<NIL
            beq         @qlist
            cmp         #<SNIL
            beq         @slist
            cmp         #<T_VAL
            beq         @t
@other:
            lda         #QK_OTHER
            rts
@number:
            lda         #QK_NUMBER
            rts
@function:
            lda         #QK_FUNCTION
            rts
@char:
            lda         #QK_CHAR
            rts
@qlist:
            lda         #QK_QLIST
            rts
@slist:
            lda         #QK_SLIST
            rts
@t:
            lda         #QK_T
            rts
@cell:
            lda         pk,x
            tax
            lda         q_kinds,x
            rts

; q_a, q_b, two strings (or, q_isname set, names): compared by their bytes.  OUT: Z = 1 if they're the same; else
; C = 1 if q_a's after.  (Each byte read in its own bank: they may be in two)
q_strcmp:
            jsr         @compare
            php
            stz         q_isname
            plp
            rts
@compare:
            lda         q_a
            ldx         q_a + 1
            jsr         q_strat
            MOVW        q_pa, hp
            MOVW        q_la, hn
            lda         $00
            sta         q_bka
            lda         q_b
            ldx         q_b + 1
            jsr         q_strat
            MOVW        q_pb, hp
            MOVW        q_lb, hn
            lda         $00
            sta         q_bkb
            ldy         #0
@byte:
            lda         q_la                                ; (One ended: the shorter first)
            ora         q_la + 1
            beq         @aend
            lda         q_lb
            ora         q_lb + 1
            beq         @after
            lda         q_bkb
            sta         $00
            lda         (q_pb),y
            sta         q_byte
            lda         q_bka
            sta         $00
            lda         (q_pa),y
            cmp         q_byte
            bne         @rts
            iny
            bne         :+
            inc         q_pa + 1
            inc         q_pb + 1
:
            lda         q_la
            bne         :+
            dec         q_la + 1
:
            dec         q_la
            lda         q_lb
            bne         :+
            dec         q_lb + 1
:
            dec         q_lb
            bra         @byte
@aend:
            lda         q_lb
            ora         q_lb + 1
            beq         @rts                                ; (Both: the same)
            lda         #0                                  ; (q_a shorter: before)
            cmp         #1
            rts
@after:
            lda         #1
            cmp         #0
@rts:
            rts

; IN: .A, .X = a string (or, q_isname set, a symbol or an atom: its name).  OUT: hp = its bytes, hn = its length
; (its bank in the window)
q_strat:
            jsr         deref
            lda         q_isname
            beq         @string
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            jmp         blob_data
@string:
            lda         (hp)
            sta         q_len
            ldy         #1
            lda         (hp),y
            sta         q_len + 1
            iny
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            MOVW        hn, q_len
            rts

; ---- cmp

; (cmp x y): -1, 0 or 1: x before y, the same, after
bq_cmp:
            jsr         q_args
            jsr         q_order
            lda         q_r
            ldx         #0
            cmp         #$80
            bcc         :+
            dex
:
            jsr         fix_ax
            sta         ex
            stx         ex + 1
            clc
            rts

; (< x y), (> x y), (<= x y), (>= x y)
bq_lt:
            jsr         q_args
q_lt:
            jsr         q_order
            lda         q_r
            cmp         #$FF
            jmp         q_eqbool
bq_gt:
            jsr         q_args
q_gt:
            jsr         q_order
            lda         q_r
            cmp         #1
            jmp         q_eqbool
bq_le:
            jsr         q_args
            jsr         q_order
            lda         q_r
            cmp         #1
            jmp         q_nebool
bq_ge:
            jsr         q_args
            jsr         q_order
            lda         q_r
            cmp         #$FF
; T if Z = 0 (q_nebool), or Z = 1 (q_eqbool)
q_nebool:
            beq         :+
            sec
            jmp         q_bool
:
            clc
            jmp         q_bool
q_eqbool:
            bne         :+
            sec
            jmp         q_bool
:
            clc
            jmp         q_bool

; (neg? x): (< x 0).  (pos? x): (< 0 x).  (zero? x): (eq 0 x).  (one? x): (eq 1 x)
bq_negp:
            jsr         q_args
            lda         #<FIX(0)
            sta         q_b
            stz         q_b + 1
            bra         q_lt
bq_posp:
            jsr         q_args
            MOVW        q_b, q_a
            lda         #<FIX(0)
            sta         q_a
            stz         q_a + 1
            bra         q_lt
bq_zerop:
            lda         #<FIX(0)
            bra         q_is_n
bq_onep:
            lda         #<FIX(1)
q_is_n:
            sta         q_a
            stz         q_a + 1
            lda         (argp)
            sta         q_b
            ldy         #1
            lda         (argp),y
            sta         q_b + 1
            jsr         q_equal
            jmp         q_bool

; (min x...), (max x...): the least (by <) or the most (by >) of the arguments
bq_min:
            lda         #$FF
            bra         q_least
bq_max:
            lda         #1
q_least:
            sta         q_by
            lda         (argp)                              ; (acc: the first)
            sta         ex
            ldy         #1
            lda         (argp),y
            sta         ex + 1
            ldy         #2
@next:
            tya
            lsr
            cmp         cn
            bcs         @done
            phy
            MOVW        q_a, ex
            lda         (argp),y
            sta         q_b
            iny
            lda         (argp),y
            sta         q_b + 1
            jsr         q_order                             ; (acc by it: kept if so, else it)
            ply
            lda         q_r
            cmp         q_by
            beq         :+
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            dey
:
            iny
            iny
            bra         @next
@done:
            clc
            rts

; q_r = -1, 0 or 1: q_a's order against q_b's (the pairs still to compare on the stack, each pair's own order
; deciding unless it's 0)
q_order:
            stz         q_n
            stz         q_n + 1
            jsr         q_push
@pair:
            lda         q_n
            ora         q_n + 1
            bne         :+
            stz         q_r                                 ; (None left: the same)
            rts
:
            jsr         q_pop
            jsr         q_compare
            lda         q_r
            beq         @pair
            jmp         q_drop

; q_a against q_b, as far as they are themselves: q_r = -1, 1, or 0 (equal so far: for lists, their firsts and rests
; pushed)
q_compare:
            jsr         q_seen
            stz         q_r
            lda         q_a                                 ; (The same value)
            cmp         q_b
            bne         :+
            lda         q_a + 1
            cmp         q_b + 1
            bne         :+
            rts
:
            lda         q_a
            ldx         q_a + 1
            jsr         q_kind
            sta         q_ka
            lda         q_b
            ldx         q_b + 1
            jsr         q_kind
            sta         q_kb
            lda         q_ka                                ; (By kind first)
            cmp         q_kb
            beq         @kind
            bcc         @less
            bra         @more
@kind:
            cmp         #QK_NUMBER
            beq         @numval
            cmp         #QK_CHAR
            beq         @number                             ; (A character's value: its code, in order too)
            cmp         #QK_STRING
            beq         @string
            cmp         #QK_ATOM
            beq         @name
            cmp         #QK_SYMBOL
            beq         @name
            cmp         #QK_QLIST
            beq         @list
            cmp         #QK_SLIST
            beq         @list
            cmp         #QK_ERROR
            beq         @error
            cmp         #QK_FUNCTION
            bne         @lb203
            jmp         @function
@lb203:
            rts                                             ; (T, and the rest: the same)
@less:
            lda         #$FF
            sta         q_r
            rts
@more:
            lda         #1
            sta         q_r
            rts
@numval:
            lda         q_a                                 ; (Two numbers: fixnums here, else numval.inc's)
            and         q_b
            lsr
            bcs         @number
            FARN        3, n_qcmp
            rts
@number:
            lda         q_a                                 ; (Signed, the values' order: a fixnum's or a character's)
            cmp         q_b
            lda         q_a + 1
            sbc         q_b + 1
            bvc         :+
            eor         #$80
:
            bmi         @less
            bra         @more
@string:
            jsr         q_strcmp
            beq         @same
            bcc         @less
            bra         @more
@same:
            rts
@name:
            jsr         q_names                             ; (Their names' blobs: as strings)
            bra         @string
@error:
            jsr         q_firsts
            bra         @string
@list:
            lda         q_a                                 ; (Empty: before; both: the same)
            ldx         q_a + 1
            jsr         q_empty
            bcc         :+
            lda         q_b
            ldx         q_b + 1
            jsr         q_empty
            bcs         @same
            bra         @less
:
            lda         q_b
            ldx         q_b + 1
            jsr         q_empty
            bcs         @more
            jsr         q_rests                             ; (Their rests, then their firsts)
            jsr         q_firsts
            jmp         q_push
@function:
            jsr         q_same_fn                           ; (Equal: the same)
            bcs         @same
            jsr         q_printed                           ; (Else as they print)
            bra         @string

; IN: .A, .X = a list.  OUT: C = 1 if it's empty (NIL, ())
q_empty:
            jsr         list_kind
            cmp         #1
            beq         @yes
            clc
            rts
@yes:
            sec
            rts

; q_a, q_b, atoms or symbols: compared by their names (q_strcmp's next)
q_names:
            lda         #1
            sta         q_isname
            rts

; q_a, q_b, functions: C = 1 if they're equal (eq's own pairs on the stack above cmp's, q_n kept)
q_same_fn:
            lda         q_n
            pha
            lda         q_n + 1
            pha
            lda         q_a
            pha
            lda         q_a + 1
            pha
            lda         q_b
            pha
            lda         q_b + 1
            pha
            jsr         q_equal
            pla
            sta         q_b + 1
            pla
            sta         q_b
            pla
            sta         q_a + 1
            pla
            sta         q_a
            pla
            sta         q_n + 1
            pla
            sta         q_n
            rts

; q_a, q_b, functions: made strings of their printed forms, to compare (the first on the stack while the second's
; made; they're reached from the arguments meanwhile)
q_printed:
            lda         q_a
            ldx         q_a + 1
            jsr         q_repr
            lda         hv
            ldx         hv + 1
            jsr         push
            lda         q_b
            ldx         q_b + 1
            jsr         q_repr
            MOVW        q_b, hv
            jsr         pop
            sta         q_a
            stx         q_a + 1
            rts

; IN: .A, .X = a value.  OUT: hv = its printed form (repr), a string
q_repr:
            pha
            phx
            jsr         cap_begin
            sta         q_start
            stx         q_start + 1
            plx
            pla
            ldy         #0
            jsr         print_val
            lda         q_start
            ldx         q_start + 1
            jmp         cap_end

.bss
q_a:        .res        2                                   ; (The pair being compared ...
q_b:        .res        2
q_a2:       .res        2                                   ;   their rests ...
q_b2:       .res        2
q_ka:       .res        1                                   ;   their kinds ...
q_kb:       .res        1
q_n:        .res        2                                   ;   the pairs still to compare ...
q_r:        .res        1                                   ;   the order found)
q_items:    .res        2
q_by:       .res        1                                   ; (min's -1, max's 1)
q_la:       .res        2                                   ; (q_strcmp's: lengths, banks, a byte)
q_lb:       .res        2
q_bka:      .res        1
q_bkb:      .res        1
q_byte:     .res        1
q_len:      .res        2
q_isname:   .res        1
q_start:    .res        2

.segment "RODATA2"
s_qelem:    .byte       "'elem?' expects a list", 0
; Each kind of cell as eq and cmp see it
q_kinds:    .byte       QK_OTHER, QK_OTHER, QK_OTHER, QK_SLIST, QK_QLIST, QK_QLIST, QK_STRING, QK_NUMBER
            .byte       QK_NUMBER, QK_NUMBER, QK_NUMBER, QK_ERROR, QK_FUNCTION, QK_SYMBOL, QK_ATOM, QK_FUNCTION
            .byte       QK_HASH, QK_STREAM, QK_OTHER, QK_OTHER, QK_OTHER, QK_OTHER
