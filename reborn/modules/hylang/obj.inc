; ****************************************************************************
; obj.inc - hylang's objects, on heap.inc's cells: the constants, each value's type (type_of), symbols and atoms
; (interned: one object a name), Q- and S-expressions (a header over a chain of pairs: a Q-expression's header keeps
; the scope it was written in, which eval runs it in), errors, functions and frames.
;   A symbol keeps its global value (def's), so a global look-up is one read; UNBOUND is a symbol's value till it has
; one.  The symbols and atoms are in 256 buckets (sym_lo/hi, by a hash of the name: the collector's roots), each a
; chain through its objects' second field.  A built-in is a constant, BUILTIN0 + 2n (pages 3 and 4, taken from the
; heap: no object at all).  The code is in the task's RAM (the module's DATA), where both of hylang's banks see it.

EXIT_VAL        = $0004         ; exit: the value that ends the REPL, and (exit n)'s head
UNBOUND         = $0006         ; A symbol's global value, till def gives it one (never a value a program sees)
GLOBAL          = $0008         ; The global environment (the scope at the top: the symbols' own values)
BUILTIN0        = $0300         ; The built-ins: BUILTIN0 + 2n (256 at most) ...
BUILTIN_END     = $0500         ;   and past them

; Types (type_of)
TY_NUM          = 1             ; A number: a fixnum, or an object (OT_BIG, OT_RAT, OT_FIX, OT_COMPLEX)
TY_LIST         = 2             ; NIL and the Q-expressions
TY_T            = 3
TY_EXIT         = 4
TY_FUNC         = 5             ; A built-in, or a function (fn, fexpr)
TY_CHAR         = 6
TY_STRING       = 7
TY_SYMBOL       = 8
TY_ATOM         = 9
TY_SEXPR        = 10
TY_ERROR        = 11
TY_FRAME        = 12            ; (The evaluator's: a scope)
TY_PAIR         = 13            ; (A list's: a bare pair)
TY_OTHER        = 14
TY_HASH         = 15

; A hash's flags (OT_HASH's second byte)
HF_PROXY        = $80           ; A method's &0 (hash-call's): the hash its first value, its private entries open

; A function's flags (OT_FUNC's second byte)
FN_FEXPR        = $01           ; Its arguments come unevaluated

; Where a symbol's or atom's fields are
SYM_NEXT        = 2             ; The next in its bucket
SYM_VALUE       = 4             ; Its global value (a symbol's)
SYM_NAME        = 6             ; Its name
SYM_MAX         = 122           ; The longest name (a cell of 128)
SYM_LOCAL       = $80           ; A symbol's length byte: it's been bound in a frame (else look-ups go to its value)

.zeropage
ob_p:       .res        2                                   ; A pointer (into an object, or a buffer)
ob_n:       .res        1                                   ; A length
ob_t:       .res        1                                   ; A type
ob_h:       .res        1                                   ; A hash

.bss
sym_lo:     .res        256                                 ; The symbols' and atoms' buckets: each the first
sym_hi:     .res        256                                 ; (0: none)
.segment "DATA"

; ---- Making it

; The objects' tables: no symbols yet; the collector told of them (gc_hook); the built-ins' pages taken from the heap
obj_init:
            lda         #PK_CONST
            sta         page_kind + >BUILTIN0
            sta         page_kind + >BUILTIN0 + 1
            ldx         #0
:
            stz         sym_lo,x
            stz         sym_hi,x
            inx
            bne         :-
            LDR         gc_hook, obj_roots
            rts

; The collector's hook: every bucket's first marked (the rest follow, as each is scanned)
obj_roots:
            ldx         #0
@bucket:
            lda         sym_lo,x
            sta         h_c
            lda         sym_hi,x
            sta         h_c + 1
            phx
            jsr         mark_root
            plx
            inx
            bne         @bucket
            rts

; ---- Types

; h_v's type: .A = TY_*
type_of:
            lda         h_v
            lsr
            bcs         @num
            ldx         h_v + 1
            lda         page_kind,x
            cmp         #PK_CHAR
            beq         @char
            cmp         #PK_PAIR
            beq         @pair
            bcs         @cell
            lda         h_v                                 ; ---- A constant
            ora         h_v + 1
            beq         @list
            lda         h_v + 1
            bne         @func                               ; (Pages 3 and 4: the built-ins)
            lda         h_v
            cmp         #<T_VAL
            beq         @t
            cmp         #<EXIT_VAL
            beq         @exit
            lda         #TY_OTHER
            rts
@num:
            lda         #TY_NUM
            rts
@char:
            lda         #TY_CHAR
            rts
@pair:
            lda         #TY_PAIR
            rts
@list:
            lda         #TY_LIST
            rts
@t:
            lda         #TY_T
            rts
@exit:
            lda         #TY_EXIT
            rts
@func:
            lda         #TY_FUNC
            rts
@cell:
            jsr         deref                               ; ---- An object: its type byte's type
            lda         (h_p)
            tax
            lda         ot_type,x
            rts

; ---- Symbols and atoms

; The symbol (.A = OT_SYMBOL) or atom (OT_ATOM) named by .Y bytes at r0 (1 to SYM_MAX, in lower case).  OUT: C = 0,
; h_v = it (made, if it wasn't); or C = 1, no room (or the name's too long)
intern:
            sta         ob_t
            sty         ob_n
            tya                                             ; (The hash: the length, then each byte rotated in)
            ldy         #0
@hash:
            cpy         ob_n
            beq         @hashed
            asl
            adc         #0
            clc
            adc         (r0),y
            iny
            bra         @hash
@hashed:
            sta         ob_h
            tax
            lda         sym_lo,x
            sta         h_v
            lda         sym_hi,x
            sta         h_v + 1
@look:
            lda         h_v + 1                             ; (The bucket's next)
            beq         @make
            jsr         deref
            lda         (h_p)
            cmp         ob_t
            bne         @next
            ldy         #1
            lda         (h_p),y
            and         #$7F                                ; (Not SYM_LOCAL)
            cmp         ob_n
            bne         @next
            clc                                             ; (Its name)
            lda         h_p
            adc         #SYM_NAME
            sta         ob_p
            lda         h_p + 1
            sta         ob_p + 1
            ldy         #0
@cmp:
            cpy         ob_n
            beq         @found
            lda         (r0),y
            cmp         (ob_p),y
            bne         @next
            iny
            bra         @cmp
@found:
            clc
            rts
@next:
            ldy         #SYM_NEXT
            lda         (h_p),y
            tax
            iny
            lda         (h_p),y
            stx         h_v
            sta         h_v + 1
            bra         @look
@make:
            lda         ob_n                                ; ---- Made: onto its bucket
            cmp         #SYM_MAX + 1
            bcs         @no
            adc         #SYM_NAME
            jsr         cell_kind
            jsr         alloc
            bcs         @no
            lda         ob_t
            sta         (h_p)
            ldy         #1
            lda         ob_n
            sta         (h_p),y
            ldx         ob_h
            iny
            lda         sym_lo,x
            sta         (h_p),y
            iny
            lda         sym_hi,x
            sta         (h_p),y
            iny
            ldx         #<UNBOUND                           ; (Its value: a symbol's UNBOUND, an atom's NIL)
            lda         ob_t
            cmp         #OT_SYMBOL
            beq         :+
            ldx         #0
:
            txa
            sta         (h_p),y
            iny
            lda         #0
            sta         (h_p),y
            clc                                             ; (Its name)
            lda         h_p
            adc         #SYM_NAME
            sta         ob_p
            lda         h_p + 1
            sta         ob_p + 1
            ldy         #0
@name:
            cpy         ob_n
            beq         @named
            lda         (r0),y
            sta         (ob_p),y
            iny
            bra         @name
@named:
            ldx         ob_h
            lda         h_v
            sta         sym_lo,x
            lda         h_v + 1
            sta         sym_hi,x
            clc
            rts
@no:
            sec
            rts

; h_v (a symbol) = its global value (UNBOUND: none)
sym_value:
            jsr         deref
            ldy         #SYM_VALUE + 1
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            rts

; h_v (a symbol) marked as bound in a frame (SYM_LOCAL)
sym_local:
            jsr         deref
            ldy         #1
            lda         (h_p),y
            ora         #SYM_LOCAL
            sta         (h_p),y
            rts

; The global value of h_v (a symbol) = h_a
sym_set:
            jsr         deref
            ldy         #SYM_VALUE
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
            rts

; ---- Lists

; A Q-expression (.A = OT_QEXPR) or S-expression (OT_SEXPR) of the list h_v; a Q-expression's scope NIL, and an empty
; one NIL itself.  OUT: C = 0, h_v = it; or C = 1, no room
make_list:
            sta         ob_t
            cmp         #OT_QEXPR
            bne         :+
            lda         h_v + 1                             ; (An empty Q-expression: NIL)
            ora         h_v
            beq         @nil
:
            MOVR        h_a, h_v                            ; (The list: a root meanwhile)
            LDR         h_d, NIL
            ldy         #PK_C8
            jsr         alloc
            bcs         @no
            lda         ob_t
            sta         (h_p)
            ldy         #2
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
            iny
            lda         #0
            sta         (h_p),y
            iny
            sta         (h_p),y
@nil:
            clc
@no:
            rts

; h_v, a hash: the hash itself (a proxy's, the hash it stands for), h_p its cell (its bank selected).  OUT: C = 1,
; it was a proxy (a method's &0, which has the private entries)
hash_real:
            jsr         deref
            ldy         #1
            lda         (h_p),y
            asl
            bcc         @done
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         deref
            sec
@done:
            rts

; h_v (a Q- or S-expression) = its list (a chain of pairs: NIL for NIL)
list_of:
            lda         h_v + 1
            ora         h_v
            beq         :+
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
:
            rts

; The cdr of h_v (a pair) = h_a.  (Only a list being made: a list a program has is never changed)
set_cdr:
            jsr         deref
            ldy         #2
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
            rts

; ---- Errors

; An error: its message the string h_a, its code h_d (an atom, or NIL).  OUT: C = 0, h_v = it; C = 1, no room
make_error:
            ldy         #PK_C8
            jsr         alloc
            bcs         @no
            lda         #OT_ERROR
            sta         (h_p)
            ldy         #2
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
            iny
            lda         h_d
            sta         (h_p),y
            iny
            lda         h_d + 1
            sta         (h_p),y
@no:
            rts

; An error whose message is the zero-terminated text at r0 (in the ROM, or the task's RAM), with no code.  OUT:
; C = 0, h_v = it; or C = 1, no room for it (h_v the error "No room", made as hylang starts)
error_text:
            ldy         #0
:
            lda         (r0),y
            beq         :+
            iny
            cpy         #126
            bne         :-
:
            tya
            jsr         make_string
            bcs         @no
            MOVR        h_a, h_v
            LDR         h_d, NIL
            jsr         make_error
            bcc         @done
@no:
            MOVR        h_v, err_room
            sec
@done:
            rts

.segment "DATA"
; Each object type's type (OT_*: TY_*)
ot_type:    .byte       TY_OTHER, TY_STRING, TY_OTHER, TY_LIST, TY_SEXPR, TY_SYMBOL, TY_ATOM, TY_FUNC, TY_ERROR
            .byte       TY_FRAME, TY_SYMBOL, TY_NUM, TY_NUM, TY_NUM, TY_NUM, TY_STRING, TY_HASH
.code
