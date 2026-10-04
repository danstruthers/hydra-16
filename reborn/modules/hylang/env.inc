; ****************************************************************************
; env.inc - hylang's scopes, names and evaluation stack, and errors' messages (step 7.2b): what the evaluator
; (eval.inc, in the module's first bank) and the built-ins (builtins.inc, in its second) share, so it's in the task's
; RAM (the module's DATA), as heap.inc's and obj.inc's code is, where both banks see it.
;   A scope is a frame (OT_FRAME: its parent, and its bindings, a list of a symbol and its value in turn) or GLOBAL,
; the top, where a symbol's value is its own (obj.inc); a symbol never bound in a frame (SYM_LOCAL) is looked up
; there at once.  The evaluation stack is EV_BANKS banks of its own, one after the other (ev_bank the top's): values,
; which the collector takes as roots (hylang_roots), as it does the registers ev_x to ev_s.
;   eval_nested and apply_nested run the evaluator from either bank: native code that wants an expression's value,
; or a function applied, calls them, and holds what it has on the stack meanwhile.
;   A call's arguments are on the stack: each value pushed as it's evaluated, then the function applied to those
; from its first (ev_a) to the top.  A place on the stack is a position, a fixnum (so the collector passes it by):
; the entry's address, the second bank's $2000 on ($A000-$BFFE), bit 0 set, so positions are in order, two apart.
; A built-in reads its arguments in place (arg_next, from ar_at to ar_end), and they're taken off the stack after
; it.

EV_BANKS        = 2             ; The evaluation stack's banks (16K: some 1300 calls nested, not in tail position)
EV_LIMIT        = $9F00         ; The last bank's top, past which an evaluation isn't started (room for frames on it)
.assert EV_BANKS = 2, error, "env.inc's positions are for a stack of two banks"
FR_CALL         = $01           ; A frame's second byte: a function call's (its &_ is NIL, if it has none bound)

.zeropage
ev_x:       .res        2                                   ; The expression at hand
ev_e:       .res        2                                   ; The scope it's in (a frame, or GLOBAL)
ev_v:       .res        2                                   ; The value at hand
ev_f:       .res        2                                   ; A function being applied ...
ev_a:       .res        2                                   ;   its arguments' place on the stack (a position) ...
ev_t:       .res        2                                   ;   the last of them (as they're made)
ev_w:       .res        2                                   ; Scratch values (roots, as the rest)
ev_r:       .res        2
ev_s:       .res        2
EV_REGS     = ev_s + 2 - ev_x
ev_sp:      .res        2                                   ; The stack's top (in ev_bank)
ev_gp:      .res        2                                   ; The collector's place in the stack
ev_j:       .res        2                                   ; A jump's address
ev_i:       .res        1                                   ; A count, or a kind
ev_c:       .res        1                                   ; A count
ev_nest:    .res        1                                   ; eval_nested's evaluations, nested
ev_intr:    .res        1                                   ; Not 0: Ctrl-C (the note handler's)

.bss
ev_bank:    .res        1                                   ; The stack's bank, at its top ...
ev_bank0:   .res        1                                   ;   its first ...
ev_bank9:   .res        1                                   ;   its last ...
ev_gb:      .res        1                                   ;   and the collector's
ev_fr:      .res        1                                   ; A frame's second byte (FR_CALL), as lookup walks
sym_rest:   .res        2                                   ; &_
sym_err:    .res        2                                   ; &err
sym_code:   .res        2                                   ; &code
atom_intr:  .res        2                                   ; :intr
ev_again:   .res        1                                   ; Not 0: a built-in's value is ev_f applied to the
                                                            ; arguments it has left (hash-call's: the evaluator
                                                            ; applies it)
ar_at:      .res        2                                   ; A built-in's next argument (a position) ...
ar_end:     .res        2                                   ;   and the place past its last
ev_base:    .res        2                                   ; A built-in's arguments' place, as it's called
err_room:   .res        2                                   ; The error "No room", made at the start
ev_name:    .res        8                                   ; &n's name, made
msg:        .res        128                                 ; An error's message, made ...
msg_n:      .res        1                                   ;   and its length
.code

.segment "DATA"

; ---- The collector's roots, the stack

; The symbol or atom (.A: OT_SYMBOL, OT_ATOM) named at r0: its length, then its name.  OUT: as intern's.  Changes r0
intern_counted:
            pha
            lda         (r0)
            tay
            inc         r0
            bne         :+
            inc         r0 + 1
:
            pla
            jmp         intern

; The collector's roots: the symbols (obj_roots), the evaluator's registers, its stack, and its error "No room"
hylang_roots:
            jsr         obj_roots
            ldx         #0
@reg:
            lda         ev_x,x
            sta         h_c
            lda         ev_x + 1,x
            sta         h_c + 1
            phx
            jsr         mark_root
            plx
            inx
            inx
            cpx         #EV_REGS
            bne         @reg
            MOVR        h_c, err_room
            jsr         mark_root
            LDR         ev_gp, $8000
            lda         ev_bank0
            sta         ev_gb
@stack:
            lda         ev_gb                               ; (Up to the top: a bank before its, all of it)
            cmp         ev_bank
            bcc         :+
            lda         ev_gp
            cmp         ev_sp
            lda         ev_gp + 1
            sbc         ev_sp + 1
            bcs         @done
:
            lda         ev_gb                               ; (Each time: a push changes the bank)
            sta         RAM_BANK
            lda         (ev_gp)
            sta         h_c
            ldy         #1
            lda         (ev_gp),y
            sta         h_c + 1
            jsr         mark_root
            clc
            lda         ev_gp
            adc         #2
            sta         ev_gp
            bcc         @stack
            inc         ev_gp + 1
            lda         ev_gp + 1                           ; (Past a bank's end: the next's start)
            cmp         #$A0
            bne         @stack
            lda         #$80
            sta         ev_gp + 1
            inc         ev_gb
            bra         @stack
@done:
            rts

; The note handler, in RAM (either bank may be at $A000 when a note comes): Ctrl-C (NOTE_INTERRUPT) noted in ev_intr,
; for the evaluator to make an error of; another note, the default
ev_notes:
            cmp         #NOTE_INTERRUPT
            bne         :+
            lda         #1
            sta         ev_intr
            clc
            rts
:
            sec
            rts

; .A/.X pushed.  (No check: ev_chain leaves room, with ev_deep, for the frames pushed between its checks)
ev_push:
            pha
            lda         ev_bank
            sta         RAM_BANK
            pla
            sta         (ev_sp)
            ldy         #1
            txa
            sta         (ev_sp),y
            clc
            lda         ev_sp
            adc         #2
            sta         ev_sp
            bcc         :+
            inc         ev_sp + 1
            lda         ev_sp + 1                           ; (Past a bank's end: the next's start)
            cmp         #$A0
            bne         :+
            lda         #$80
            sta         ev_sp + 1
            inc         ev_bank
:
            rts

; .A/.X = the value popped
ev_pop:
            lda         ev_sp                               ; (At a bank's start: the last's end)
            bne         :+
            lda         ev_sp + 1
            cmp         #$80
            bne         :+
            lda         #$A0
            sta         ev_sp + 1
            dec         ev_bank
:
            lda         ev_bank
            sta         RAM_BANK
            sec
            lda         ev_sp
            sbc         #2
            sta         ev_sp
            bcs         :+
            dec         ev_sp + 1
:
            ldy         #1
            lda         (ev_sp),y
            tax
            lda         (ev_sp)
            rts

; C = 1 if the stack's near its end: its last bank, past EV_LIMIT
ev_deep:
            lda         ev_bank
            cmp         ev_bank9
            bcc         :+
            lda         ev_sp + 1
            cmp         #>EV_LIMIT
:
            rts

.macro PUSHV var
            lda         var
            ldx         var + 1
            jsr         ev_push
.endmacro

.macro POPV var
            jsr         ev_pop
            sta         var
            stx         var + 1
.endmacro

.macro PUSHK k
            lda         #(k) * 2 + 1
            ldx         #0
            jsr         ev_push
.endmacro

; ---- Nested evaluations

; ev_x evaluated in ev_e, from either bank: the evaluator (the first bank's) run till it's done.  OUT: ev_v.  (What
; the caller holds must be on the stack or in the registers, or it may be collected; the registers are changed)
eval_nested:
            ldx         #0
            bra         nested

; ev_f applied to ev_a (its arguments' values), from either bank: as eval_nested
apply_nested:
            ldx         #2
nested:
            lda         $01                                 ; (The caller's bank, kept)
            pha
            lda         hyx2_bank1
            sta         $01
            jsr         @go
            pla
            sta         $01
            rts
@go:
            jmp         (nested_to,x)

nested_to:  .addr       ev_nested, ap_nested

; ---- Lists, and frames

; ev_v added to the list ev_a, whose last pair is ev_t (ev_append_x: the list at ev_x + .X, its last after it).
; OUT: C = 1, no room
ev_append:
            ldx         #ev_a - ev_x
ev_append_x:
            phx
            MOVR        h_a, ev_v
            LDR         h_d, NIL
            jsr         cons
            plx
            bcs         @no
            lda         ev_x + 1,x
            bne         @after
            lda         h_v                                 ; (The first)
            sta         ev_x,x
            sta         ev_x + 2,x
            lda         h_v + 1
            sta         ev_x + 1,x
            sta         ev_x + 3,x
            clc
            rts
@after:
            phx                                             ; (After the last)
            MOVR        h_a, h_v
            lda         ev_x + 2,x
            sta         h_v
            lda         ev_x + 3,x
            sta         h_v + 1
            jsr         set_cdr
            plx
            lda         h_a
            sta         ev_x + 2,x
            lda         h_a + 1
            sta         ev_x + 3,x
            clc
@no:
            rts

; C = 1 if ev_v is an error (h_v = ev_v)
ev_is_error:
            MOVR        h_v, ev_v
            lda         ev_v
            lsr
            bcs         @no
            ldx         ev_v + 1
            lda         page_kind,x
            cmp         #PK_C8
            bcc         @no
            jsr         deref
            lda         (h_p)
            cmp         #OT_ERROR
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

; h_v = ev_r's car
car_of_r:
            MOVR        h_v, ev_r
            jmp         car

; .A = how many items ev_r has (255 at most)
count_r:
            MOVR        h_v, ev_r

; .A = how many items the chain h_v has (255 at most)
count_h:
            ldx         #0
@next:
            lda         h_v + 1
            beq         @done
            phx
            jsr         cdr
            plx
            inx
            bne         @next
            dex
@done:
            txa
            rts

; ev_x = ev_v, onto the bindings ev_s (first).  OUT: C = 1, no room
prepend_binding:
            MOVR        h_a, ev_v
            MOVR        h_d, ev_s
            jsr         cons
            bcs         @no
            MOVR        h_a, ev_x
            MOVR        h_d, h_v
            jsr         cons
            bcs         @no
            MOVR        ev_s, h_v
@no:
            rts

; A frame: its parent ev_e, its bindings ev_s (make_frame_x: its second byte .X, FR_CALL for a call's).  OUT:
; C = 0, h_v = it; C = 1, no room
make_frame:
            ldx         #0
make_frame_x:
            MOVR        h_a, ev_e
            MOVR        h_d, ev_s
            lda         #OT_FRAME
            ; (falls into make_cell2)

; A cell of 8 bytes: its type .A, its second byte .X, h_a and h_d its two values.  OUT: C = 0, h_v = it; C = 1, no
; room
make_cell2:
            pha
            phx
            ldy         #PK_C8
            jsr         alloc
            plx
            pla
            bcs         @no
            sta         (h_p)
            ldy         #1
            txa
            sta         (h_p),y
            iny
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
            clc
@no:
            rts

; A copy of h_v (a Q- or S-expression) with the scope ev_e.  OUT: C = 0, ev_v = it; C = 1, no room
scoped_copy:
            jsr         deref
            lda         (h_p)
            pha
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_a
            stx         h_a + 1
            MOVR        h_d, ev_e
            pla
            ldx         #0
            jsr         make_cell2
            bcs         :+
            MOVR        ev_v, h_v
:
            rts

; ---- Names

; ev_x's value (a symbol's) in ev_e (lookup_w: in ev_w): ev_v; or an error, Unbound Symbol 'x'.  OUT: C = 0
lookup:
            MOVR        ev_w, ev_e
lookup_w:
            MOVR        h_v, ev_x                           ; (Never bound in a frame: its global value)
            jsr         deref
            ldy         #1
            lda         (h_p),y
            bmi         @frame
            ldy         #SYM_VALUE + 1
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jmp         @value
@frame:
            lda         ev_w + 1                            ; (GLOBAL: the symbol's own value)
            jeq         @global
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #5                                  ; (The frame's bindings ...
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            pha
            dey                                             ;   and its parent, the next)
            lda         (h_p),y
            sta         ev_w + 1
            dey
            lda         (h_p),y
            sta         ev_w
            dey                                             ; (Its second byte: FR_CALL?)
            lda         (h_p),y
            sta         ev_fr
            pla
            sta         h_v
            stx         h_v + 1
@walk:
            lda         h_v + 1
            bne         @pair
            lda         ev_fr                               ; (Not in the frame: a call's &_ is NIL)
            beq         @frame
            lda         ev_x
            cmp         sym_rest
            bne         @frame
            lda         ev_x + 1
            cmp         sym_rest + 1
            bne         @frame
            LDR         ev_v, NIL
            clc
            rts
@pair:
            jsr         deref
            lda         (h_p)
            cmp         ev_x
            bne         @skip
            ldy         #1
            lda         (h_p),y
            cmp         ev_x + 1
            bne         @skip
            ldy         #3                                  ; (Found: the next's car)
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         car
            MOVR        ev_v, h_v
            clc
            rts
@skip:
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         cdr
            bra         @walk
@global:
            MOVR        h_v, ev_x
            jsr         sym_value
@value:
            lda         h_v + 1
            bne         @found
            lda         h_v
            cmp         #<UNBOUND
            beq         @unbound
@found:
            MOVR        ev_v, h_v
            clc
            rts
@unbound:
            LDR         r0, es_unbound                      ; (Unbound Symbol 'x')
            jsr         msg_start
            MOVR        h_v, ev_x
            jsr         msg_name
            lda         #$27
            jsr         msg_char
            jsr         msg_error
            clc
            rts

; ev_x = ev_v in the scope ev_w: its binding there changed, or a new one (GLOBAL: its global value).  OUT: C = 1,
; no room
bind:
            lda         ev_w + 1
            bne         @frame
            MOVR        h_v, ev_x
            MOVR        h_a, ev_v
            jsr         sym_set
            clc
            rts
@frame:
            jsr         find_binding                        ; (Its own: changed)
            bcs         @new
            MOVR        h_a, ev_v
            jsr         set_car
            clc
            rts
@new:
            MOVR        h_v, ev_x                           ; (A new one, first)
            jsr         sym_local
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #4
            lda         (h_p),y
            sta         h_d
            iny
            lda         (h_p),y
            sta         h_d + 1
            MOVR        h_a, ev_v
            jsr         cons
            bcs         @no
            MOVR        h_d, h_v
            MOVR        h_a, ev_x
            jsr         cons
            bcs         @no
            MOVR        h_a, h_v
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #4
            lda         h_a
            sta         (h_p),y
            iny
            lda         h_a + 1
            sta         (h_p),y
            clc
@no:
            rts

; ev_x's binding in the frame ev_w (not its parents).  OUT: C = 0, h_v = its value's pair; C = 1, none
find_binding:
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #5
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
@walk:
            lda         h_v + 1
            beq         @none
            jsr         deref
            lda         (h_p)
            cmp         ev_x
            bne         @skip
            ldy         #1
            lda         (h_p),y
            cmp         ev_x + 1
            bne         @skip
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            clc
            rts
@skip:
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         cdr
            bra         @walk
@none:
            sec
            rts

; set!'s: ev_x's nearest binding, from ev_e out, = ev_v.  OUT: C = 0; C = 1, ev_v = an error (Unbound Symbol)
update:
            MOVR        ev_w, ev_e
@frame:
            lda         ev_w + 1
            beq         @global
            jsr         find_binding
            bcs         @parent
            MOVR        h_a, ev_v
            jsr         set_car
            clc
            rts
@parent:
            MOVR        h_v, ev_w
            jsr         deref
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_w
            stx         ev_w + 1
            bra         @frame
@global:
            MOVR        h_v, ev_x
            jsr         sym_value
            lda         h_v + 1
            bne         :+
            lda         h_v
            cmp         #<UNBOUND
            beq         @unbound
:
            MOVR        h_v, ev_x
            MOVR        h_a, ev_v
            jsr         sym_set
            clc
            rts
@unbound:
            jsr         lookup                              ; (Its error)
            sec
            rts

; The car of h_v (a pair) = h_a
set_car:
            jsr         deref
            lda         h_a
            sta         (h_p)
            ldy         #1
            lda         h_a + 1
            sta         (h_p),y
            rts

; ---- Positions on the stack, and a call's arguments

; .A/.X = the stack's top, a position
ev_here:
            lda         ev_sp + 1
            ldx         ev_bank
            cpx         ev_bank0
            beq         :+
            ora         #$20                                ; (The second bank's)
:
            tax
            lda         ev_sp
            ora         #1
            rts

; The stack's top = the position .A/.X (what's above it, let go)
ev_set:
            and         #$FE
            sta         ev_sp
            ldy         ev_bank0
            txa
            cmp         #$A0
            bcc         :+
            iny                                             ; (The second bank's)
            and         #$9F
:
            sta         ev_sp + 1
            sty         ev_bank
            rts

; h_p = the stack's entry at the position .A/.X (its bank selected)
pos_at:
            and         #$FE
            sta         h_p
            ldy         ev_bank0
            txa
            cmp         #$A0
            bcc         :+
            iny                                             ; (The second bank's)
            and         #$9F
:
            sta         h_p + 1
            sty         RAM_BANK
            rts

; h_v = the value at the position .A/.X (pos_get); or put there (pos_put)
pos_get:
            jsr         pos_at
            ldy         #1
            lda         (h_p),y
            tax
            lda         (h_p)
            sta         h_v
            stx         h_v + 1
            rts
pos_put:
            jsr         pos_at
            lda         h_v
            sta         (h_p)
            ldy         #1
            lda         h_v + 1
            sta         (h_p),y
            rts

; The chain h_v's items pushed, the arguments of a call: ev_a their place.  OUT: C = 1, the stack's full (none left
; pushed)
push_list:
            jsr         ev_here
            sta         ev_a
            stx         ev_a + 1
@next:
            lda         h_v + 1
            beq         @done
            jsr         ev_deep
            bcs         @full
            lda         h_v + 1
            pha
            lda         h_v
            pha
            jsr         car
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            bra         @next
@done:
            clc
            rts
@full:
            lda         ev_a
            ldx         ev_a + 1
            jsr         ev_set
            sec
            rts

; The next argument: h_v = it (on the stack till the built-in returns: it needn't be held elsewhere).  OUT: C = 1,
; there's none
arg_next:
            lda         ar_at
            cmp         ar_end
            bne         :+
            lda         ar_at + 1
            cmp         ar_end + 1
            bne         :+
            sec
            rts
:
            lda         ar_at
            ldx         ar_at + 1
            jsr         pos_get
            clc
            lda         ar_at
            adc         #2
            sta         ar_at
            bcc         :+
            inc         ar_at + 1
:
            clc
            rts

; Z = 1 if no argument's left
args_none:
            lda         ar_at
            cmp         ar_end
            bne         :+
            lda         ar_at + 1
            cmp         ar_end + 1
:
            rts

; .A = how many arguments are left (255 at most)
count_a:
            sec
            lda         ar_end
            sbc         ar_at
            tax
            lda         ar_end + 1
            sbc         ar_at + 1
            cmp         #2
            bcs         @many
            lsr
            txa
            ror
            rts
@many:
            lda         #255
            rts

; The arguments left, a chain: h_v (and none left).  OUT: C = 1, no room
args_list:
            LDR         h_v, NIL
            jsr         root_push
            bcs         @no
@next:
            jsr         args_none
            beq         @done
            sec                                             ; (From the last back)
            lda         ar_end
            sbc         #2
            sta         ar_end
            bcs         :+
            dec         ar_end + 1
:
            lda         ar_end
            ldx         ar_end + 1
            jsr         pos_get
            MOVR        h_a, h_v
            jsr         root_get
            MOVR        h_d, h_v
            jsr         cons
            bcs         @fail
            jsr         root_set
            bra         @next
@done:
            jsr         root_get
            jsr         root_drop
            clc
            rts
@fail:
            jsr         root_drop
            sec
@no:
            rts

; The arguments left moved down to ev_base, a call's arguments (ev_a their place), the stack's top after them
args_down:
            MOVR        ev_a, ev_base
@move:
            jsr         args_none
            beq         @done
            lda         ar_at
            ldx         ar_at + 1
            jsr         pos_get
            lda         ev_base
            ldx         ev_base + 1
            jsr         pos_put
            clc
            lda         ar_at
            adc         #2
            sta         ar_at
            bcc         :+
            inc         ar_at + 1
:
            clc
            lda         ev_base
            adc         #2
            sta         ev_base
            bcc         @move
            inc         ev_base + 1
            bra         @move
@done:
            lda         ev_base
            ldx         ev_base + 1
            jmp         ev_set

; C = 1 if h_v is an error (h_v kept; h_is_error_v: and ev_v = it, if it is)
h_is_error:
            lda         h_v
            lsr
            bcs         @no
            ldx         h_v + 1
            lda         page_kind,x
            cmp         #PK_C8
            bcc         @no
            jsr         deref
            lda         (h_p)
            cmp         #OT_ERROR
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

h_is_error_v:
            jsr         h_is_error
            bcc         :+
            MOVR        ev_v, h_v
:
            rts

; ev_v = the error No room
bi_no_room:
            MOVR        ev_v, err_room
            rts

; ev_v = T if C = 1, else NIL
bi_bool:
            LDR         ev_v, NIL
            bcc         :+
            LDR         ev_v, T_VAL
:
            rts

; ---- Errors, and their messages

; A message begun: the text at r0
msg_start:
            stz         msg_n
            ; (falls into msg_text)

; The text at r0 added
msg_text:
            ldy         #0
:
            lda         (r0),y
            beq         :+
            jsr         msg_char
            iny
            bne         :-
:
            rts

; .A added.  Keeps .X, .Y
msg_char:
            phx
            ldx         msg_n
            cpx         #126
            bcs         :+
            sta         msg,x
            inc         msg_n
:
            plx
            rts

; .A added, a number (0-255)
msg_dec:
            ldx         #0                                  ; (Its digits, pushed: the last first)
:
            ldy         #0
            sec
@div:
            sbc         #10
            bcc         @digit
            iny
            bra         @div
@digit:
            adc         #'0' + 10
            pha
            inx
            tya
            bne         :-
:
            pla
            jsr         msg_char
            dex
            bne         :-
            rts

; h_v's name added (a symbol's, an atom's, or a symbol reference's symbol's)
msg_name:
            jsr         deref
            lda         (h_p)
            cmp         #OT_SYMREF
            bne         :+
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         deref
:
            ldy         #1
            lda         (h_p),y
            and         #$7F                                ; (Not SYM_LOCAL)
            clc
            adc         #SYM_NAME
            sta         ob_n
            ldy         #SYM_NAME
:
            cpy         ob_n
            beq         :+
            lda         (h_p),y
            jsr         msg_char
            iny
            bra         :-
:
            rts

; The message made, an error: ev_v = it
msg_error:
            ldx         msg_n
            stz         msg,x
            LDR         r0, msg
            ; (falls into ev_error_text)

; ev_v = an error whose message is the text at r0 (no room: the error No room).  OUT: C = 1
ev_error_text:
            jsr         error_text
            bcc         :+
            MOVR        h_v, err_room
:
            MOVR        ev_v, h_v
            sec
            rts

es_unbound: .byte       "Unbound Symbol '", 0
.code
