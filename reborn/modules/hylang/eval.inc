; ****************************************************************************
; eval.inc - hylang's evaluator (step 7.2b), in the module's first bank: danlang's evaluation (its C#: LVal.Eval,
; EvalSExpr, Call, Apply, and the built-ins with a tail position), as a machine whose continuations are frames on a
; stack of its own (banks of its own: env.inc), so a program may nest as deeply as that holds, and the 6502's 256
; bytes stay the machine's.
;   ev_eval evaluates ev_x in the scope ev_e: a symbol is looked up (the scope's frames, then its global value), a
; Q-expression keeps the scope it's evaluated in (a copy with it: eval runs it there), and an S-expression has its
; head evaluated, then, for a function, its arguments (an fexpr's as they're written, each with the scope), and the
; function applied.  A value goes to ev_return, which pops the frame waiting for it (its K_* code on top, a fixnum)
; and goes on with it.  A function's body is evaluated in its caller's place (a call takes no frame: a tail call
; takes no stack), and so are if's branches, do's last, let's last and eval's.  Every entry of the stack is a value
; (its K_* codes fixnums), and so are the registers ev_x to ev_s, so the collector takes them all as roots (env.inc
; has the stack, scopes and names, which the built-ins use too).
;   A built-in (builtins.inc) is native code, in the second bank or the third (BF_BANK3: numbers.inc), called through
; hyx2_farn, given its arguments'
; values in ev_a (a chain), or as they're written (BF_RAW: defined?), ev_v its value; an error among the values is
; the call's value, unless the built-in takes errors (BF_ERRORS: print, error-message ...).  A special form
; (BF_SPECIAL: if, do, let, while ...) is the first bank's, given its arguments as they're written (ev_r), and goes
; on its own way, with the frames K_IF, K_DO ... .  Native code that wants a value evaluated, or a function applied,
; calls eval_nested or apply_nested (the machine again, on the same stack, nested EV_NEST deep at most).
;   Ctrl-C (the note handler: ev_intr) is an error, "interrupted" (its code :intr), at the next call or loop step.

EV_NEST         = 12            ; Native code's evaluations nested, at most (each takes some of the 6502's stack)

; The frames' continuations: K_* (on top of each frame, K_* * 2 + 1: a fixnum), and what's under it
K_DONE          = 0             ; []: the end of eval_nested's evaluation (back to its caller)
K_HEAD          = 1             ; [rest, env]: an S-expression's head
K_ARG           = 2             ; [f, rest, args, last, env]: an argument
K_IF            = 3             ; [branches, env]: if's test
K_DO            = 4             ; [rest, env]: one of do's, not its last
K_AND           = 5             ; [rest, env]
K_OR            = 6             ; [rest, env]
K_LETB          = 7             ; [name, bindings left, body, scope]: a let binding's value
K_LETS          = 8             ; [body left, scope]: a let body's expression, not its last
K_LETR          = 9             ; [scope]: a let body's expression that's neither a list nor code: run, if it's code
K_TRY           = 10            ; [handler, env]: try's expression
K_TRYH          = 11            ; [message]: try's handler: called with the message, if it's a function
K_EVAL          = 12            ; [env]: eval's argument
K_EXIT          = 13            ; []: exit's code
K_DEFN          = 14            ; [kind, values, env]: the names of def, set or set! (an S-expression's value)
K_DEF           = 15            ; [kind, names left, values left, env, name]: a value of def, set or set!
K_WHILET        = 16            ; [arguments, value, env]: while's test
K_WHILEB        = 17            ; [arguments, body left, env]: one of while's body
K_ITER1         = 18            ; [kind, second, env]: each's or dotimes's first argument (a function, a number)
K_ITER2         = 19            ; [kind, first's value, env]: its second (or the list of a {name list})
K_ITER          = 20            ; [function, items left, place, env]: the function applied to an item
K_RUN           = 21            ; [rest, env]: one of a body run (each's, dotimes's), not its last
K_OUTPUT        = 22            ; [the text caught before, rest, env]: one of output-of's
K_SPACE         = 23            ; [blocks, env]: <=>'s number

; A built-in's flags (bi_flags)
BF_SPECIAL      = $01           ; Its arguments as they're written (ev_r), its own way
BF_ERRORS       = $02           ; An error among its arguments is an argument (not the call's value)
BF_RAW          = $04           ; Its arguments as they're written, in ev_a (native code, that evaluates none)
BF_BANK3        = $08           ; Its code is in the third bank (numbers.inc), not the second

.code

; ---- Making it

; The evaluator: its stack (EV_BANKS banks), the collector told of its roots, Ctrl-C a note, the names it uses, the
; error "No room" (so there's always one), and the built-ins (builtins.inc).  OUT: C = 1, no room
eval_init:
            lda         #EV_BANKS
            jsr         BANKS_ALLOC
            jcs         @no
            sta         ev_bank
            sta         ev_bank0
            clc
            adc         #EV_BANKS - 1
            sta         ev_bank9
            LDR         ev_sp, $8000
            stz         ev_nest
            stz         ev_intr
            ldx         #EV_REGS - 1                        ; (The registers: NIL)
:
            stz         ev_x,x
            dex
            bpl         :-
            stz         err_room
            stz         err_room + 1
            LDR         gc_hook, hylang_roots
            LDR         r0, ev_notes
            jsr         NOTIFY
            LDR         r0, es_amp_rest                     ; (&_, &err, &code, :intr)
            lda         #OT_SYMBOL
            jsr         intern_counted
            jcs         @no
            MOVR        sym_rest, h_v
            jsr         sym_local
            LDR         r0, es_amp_err
            lda         #OT_SYMBOL
            jsr         intern_counted
            bcs         @no
            MOVR        sym_err, h_v
            jsr         sym_local
            LDR         r0, es_amp_code
            lda         #OT_SYMBOL
            jsr         intern_counted
            bcs         @no
            MOVR        sym_code, h_v
            jsr         sym_local
            LDR         r0, es_intr
            lda         #OT_ATOM
            jsr         intern_counted
            bcs         @no
            MOVR        atom_intr, h_v
            LDR         r0, es_no_room
            jsr         error_text
            bcs         @no
            MOVR        err_room, h_v
            FAR2        builtins_init
            rts
@no:
            sec
            rts

; ---- The machine

; eval_nested's and apply_nested's (env.inc): the machine run till it's done
ev_nested:
            jsr         ev_nest_in
            bcs         :+
            jmp         ev_eval
:
            rts

ap_nested:
            jsr         ev_nest_in
            bcs         :+
            jmp         ev_apply
:
            rts

; One more nested (K_DONE pushed).  OUT: C = 1, too deep (ev_v the error)
ev_nest_in:
            lda         ev_nest
            cmp         #EV_NEST
            bcc         :+
            LDR         r0, es_nested
            jmp         ev_error_text
:
            inc         ev_nest
            PUSHK       K_DONE
            clc
            rts

; ev_x evaluated in ev_e
ev_eval:
            jsr         ev_simple
            bcs         :+
            jmp         ev_return
:
            ldy         #5                                  ; ---- An S-expression: in its scope (an fexpr's
            lda         (h_p),y                             ; argument's), or this one
            dey
            ora         (h_p),y
            beq         :+
            lda         (h_p),y
            sta         ev_e
            iny
            lda         (h_p),y
            sta         ev_e + 1
:
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_r
            stx         ev_r + 1
            ; (falls into ev_chain)

; ev_r (a chain) evaluated as an S-expression in ev_e: its head first (K_HEAD, if it's an S-expression)
ev_chain:
            lda         ev_intr                             ; ---- Ctrl-C: an error, here
            beq         :+
            stz         ev_intr
            jmp         ev_interrupted
:
            jsr         ev_deep                             ; ---- Room on the stack
            bcc         :+
            LDR         r0, es_deep
            jmp         sf_error
:
            lda         ev_r + 1
            bne         :+
            LDR         ev_v, NIL                           ; (Empty: NIL)
            jmp         ev_return
:
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_x, h_v
            jsr         ev_simple
            php
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            plp
            bcs         :+
            jmp         ev_head
:
            PUSHV       ev_r
            PUSHV       ev_e
            PUSHK       K_HEAD
            jmp         ev_eval

; ev_x's value, if the machine needn't evaluate it (it isn't an S-expression): ev_v.  A fixnum, a constant, a
; character, a built-in, a string ...: itself.  A symbol: its value (an error, if it's unbound).  A Q-expression:
; itself, if it has a scope, else a copy with this one.  OUT: C = 0, ev_v; C = 1, an S-expression (h_p its address)
ev_simple:
            lda         ev_x
            lsr
            bcs         @self
            ldx         ev_x + 1
            lda         page_kind,x
            cmp         #PK_C8
            bcc         @self
            MOVR        h_v, ev_x
            jsr         deref
            lda         (h_p)
            cmp         #OT_SYMBOL
            beq         @symbol
            cmp         #OT_SEXPR
            beq         @sexpr
            cmp         #OT_QEXPR
            beq         @qexpr
            cmp         #OT_SYMREF
            beq         @symref
@self:
            MOVR        ev_v, ev_x
            clc
            rts
@sexpr:
            sec
            rts
@symbol:
            jmp         lookup
@symref:
            ldy         #4                                  ; (A symbol written in a scope, an fexpr's argument:
            lda         (h_p),y                             ; its value there)
            sta         ev_w
            iny
            lda         (h_p),y
            sta         ev_w + 1
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_x
            stx         ev_x + 1
            jmp         lookup_w
@qexpr:
            ldy         #5
            lda         (h_p),y
            dey
            ora         (h_p),y
            bne         @self
            MOVR        h_v, ev_x
            jsr         scoped_copy
            bcc         :+
            MOVR        ev_v, err_room
:
            clc
            rts

; ev_v, the value for the frame on top of the stack: its K_* says where it goes
ev_return:
            jsr         ev_pop                              ; (Its K: K_* * 2 + 1)
            lsr
            tax
            lda         k_lo,x
            sta         ev_j
            lda         k_hi,x
            sta         ev_j + 1
            jmp         (ev_j)

; ev_r's first evaluated, for the frame just pushed
ev_first:
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_x, h_v
            jmp         ev_eval

; ---- The continuations

k_done:
            dec         ev_nest
            rts

; An S-expression's head (ev_v), the rest of it ev_r: a function applied (a special form its own way); exit with a
; code, the end; anything else, alone, itself
k_head:
            POPV        ev_e
            POPV        ev_r
ev_head:
            jsr         ev_is_error
            bcc         :+
            jmp         ev_return
:
            lda         ev_v + 1                            ; ---- exit: (exit code)
            bne         @fun
            lda         ev_v
            cmp         #<EXIT_VAL
            bne         @fun
            lda         ev_r + 1
            beq         @alone
            PUSHK       K_EXIT
            jmp         ev_first
@fun:
            MOVR        h_v, ev_v
            jsr         type_of
            cmp         #TY_FUNC
            beq         @apply
            lda         ev_r + 1                            ; (Alone: itself)
            bne         @wrong
@alone:
            jmp         ev_return
@wrong:
            jmp         ev_not_function
@apply:
            MOVR        ev_f, ev_v
            lda         ev_f + 1                            ; ---- A built-in: a special form, its own way
            cmp         #>BUILTIN_END
            bcs         @fn
            jsr         bi_index
            lda         bi_flags,x
            bit         #BF_SPECIAL
            bne         @special
            and         #BF_RAW
            beq         ev_args
            MOVR        ev_a, ev_r                          ; (Native code, given them as they're written)
            jmp         ev_apply
@special:
            lda         bi_lo,x
            sta         ev_j
            lda         bi_hi,x
            sta         ev_j + 1
            jmp         (ev_j)
@fn:
            MOVR        h_v, ev_f                           ; ---- A function: an fexpr's arguments as they're
            jsr         deref                               ; written
            ldy         #1
            lda         (h_p),y
            and         #FN_FEXPR
            beq         ev_args
            jsr         fexpr_args
            bcs         :+
            jmp         apply_fn
:
            jmp         ev_no_room

; ev_f's arguments (ev_r) evaluated in turn, onto ev_a (an S-expression's with K_ARG); then ev_f applied
ev_args:
            LDR         ev_a, NIL
            LDR         ev_t, NIL
ev_next_arg:
            lda         ev_r + 1
            bne         :+
            jmp         ev_apply
:
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_x, h_v
            jsr         ev_simple
            bcc         ev_arg
            PUSHV       ev_f
            PUSHV       ev_r
            PUSHV       ev_a
            PUSHV       ev_t
            PUSHV       ev_e
            PUSHK       K_ARG
            jmp         ev_eval

; An argument's value: onto the arguments (an error, the call's value, unless the function's a built-in that takes
; errors)
k_arg:
            POPV        ev_e
            POPV        ev_t
            POPV        ev_a
            POPV        ev_r
            POPV        ev_f
ev_arg:
            jsr         ev_is_error
            bcc         @add
            lda         ev_f + 1
            cmp         #>BUILTIN_END
            bcs         @error
            jsr         bi_index
            lda         bi_flags,x
            and         #BF_ERRORS
            bne         @add
@error:
            jmp         ev_return
@add:
            jsr         ev_append
            bcs         @room
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            jmp         ev_next_arg
@room:
            jmp         ev_no_room

; ev_f applied to ev_a: a function's body run (apply_fn), a built-in's code called (ev_v its value); a special form
; is given the values as its arguments, as if they were written (it goes on its own way)
ev_apply:
            lda         ev_f + 1
            cmp         #>BUILTIN_END
            bcc         :+
            jmp         apply_fn
:
            jsr         bi_index
            lda         bi_lo,x
            sta         ev_j
            lda         bi_hi,x
            sta         ev_j + 1
            lda         bi_flags,x
            and         #BF_SPECIAL
            beq         :+
            MOVR        ev_r, ev_a
            jmp         (ev_j)
:
            lda         ev_j                                ; (Native code: in the second bank, or the third)
            sta         hyx2_to
            lda         ev_j + 1
            sta         hyx2_to + 1
            ldy         hyx2_bank2
            lda         bi_flags,x
            and         #BF_BANK3
            beq         :+
            ldy         hyx2_bank3
:
            sty         hyx2_n
            jsr         hyx2_farn
            jmp         ev_return

; The built-in ev_f's number: .X (page 3's 0-127, page 4's 128-255)
bi_index:
            lda         ev_f + 1
            lsr
            lda         ev_f
            ror
            eor         #$80
            tax
            rts

; ---- Functions

; ev_f (a function: fn's or fexpr's) applied to ev_a: its formals bound to the arguments in turn, those past them to
; &1, &2 ... by their places, and &_ to a list of those; if formals are left, its value is the function with these
; bound (partial application); if not, its body is evaluated in a new frame of the bindings, the function's closure
; its parent (lexical scope), in the caller's place
apply_fn:
            MOVR        h_v, ev_f
            jsr         deref
            ldy         #3                                  ; (ev_r: the formals left; ev_s: the bindings so far)
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_r
            stx         ev_r + 1
            ldy         #9
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_s
            stx         ev_s + 1
            LDR         ev_t, NIL                           ; (ev_t, ev_w: the arguments past the formals)
            LDR         ev_w, NIL
            stz         ev_i
@arg:
            lda         ev_a + 1
            beq         @bound
            inc         ev_i
            MOVR        h_v, ev_a
            jsr         car
            MOVR        ev_v, h_v
            lda         ev_r + 1
            beq         @extra
            MOVR        h_v, ev_r                           ; (A formal's)
            jsr         car
            MOVR        ev_x, h_v
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            bra         @bind
@extra:
            ldx         #ev_t - ev_x                        ; (Past them: &_'s, and &n's)
            jsr         ev_append_x
            jcs         @room
            jsr         amp_symbol
            jcs         @room
            MOVR        ev_x, h_v
@bind:
            jsr         prepend_binding
            jcs         @room
            MOVR        h_v, ev_a
            jsr         cdr
            MOVR        ev_a, h_v
            bra         @arg
@bound:
            lda         ev_t + 1                            ; (&_: those past the formals, a Q-expression, if there
            beq         @formals                            ; are any; a call's frame has it NIL else)
            MOVR        h_v, ev_t
            lda         #OT_QEXPR
            jsr         make_list
            bcs         @room
            MOVR        ev_v, h_v
            MOVR        ev_x, sym_rest
            jsr         prepend_binding
            bcs         @room
@formals:
            lda         ev_r + 1                            ; (Formals left: the function with these bound)
            beq         @call
            jsr         partial_fn
            bcs         @room
            jmp         ev_return
@call:
            MOVR        h_v, ev_f                           ; ---- Its body, in a frame of its own
            jsr         deref
            ldy         #7
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_e
            stx         ev_e + 1
            ldx         #FR_CALL
            jsr         make_frame_x
            bcs         @room
            MOVR        ev_e, h_v
            MOVR        h_v, ev_f
            jsr         deref
            ldy         #5
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_r
            stx         ev_r + 1
            jmp         ev_chain
@room:
            jmp         ev_no_room

; &n's symbol, n = ev_i (1-255).  OUT: C = 0, h_v = it; C = 1, no room
amp_symbol:
            lda         #'&'
            sta         ev_name
            ldx         #1
            lda         ev_i
            cmp         #100
            bcc         @tens
            ldy         #'0' - 1                            ; (Hundreds)
:
            iny
            sbc         #100
            bcs         :-
            adc         #100
            pha
            tya
            sta         ev_name,x
            pla
            inx
            bra         @tens2
@tens:
            cmp         #10
            bcc         @ones
@tens2:
            ldy         #'0' - 1
            sec
:
            iny
            sbc         #10
            bcs         :-
            adc         #10
            pha
            tya
            sta         ev_name,x
            pla
            inx
@ones:
            ora         #'0'
            sta         ev_name,x
            inx
            txa
            tay
            LDR         r0, ev_name
            lda         #OT_SYMBOL
            jsr         intern
            bcs         :+
            jsr         sym_local
:
            rts

; ev_f partly applied: a copy of it, but its formals ev_r (those left) and its bindings ev_s.  OUT: C = 0, ev_v =
; it; C = 1, no room
partial_fn:
            ldy         #PK_C16
            jsr         alloc
            bcs         @no
            MOVR        ev_v, h_v
            MOVR        h_v, ev_f                           ; (The original's bytes, through msg: the two may be
            jsr         deref                               ; in different banks)
            ldy         #9
:
            lda         (h_p),y
            sta         msg,y
            dey
            bpl         :-
            lda         ev_r
            sta         msg + 2
            lda         ev_r + 1
            sta         msg + 3
            lda         ev_s
            sta         msg + 8
            lda         ev_s + 1
            sta         msg + 9
            MOVR        h_v, ev_v
            jsr         deref
            ldy         #9
:
            lda         msg,y
            sta         (h_p),y
            dey
            bpl         :-
            clc
@no:
            rts

; An fexpr's arguments, as they're written (ev_r), each remembering the scope ev_e: a symbol in a reference to it
; with the scope, an S-expression or a Q-expression in a copy with it, anything else as it is.  OUT: ev_a; C = 1, no
; room
fexpr_args:
            LDR         ev_a, NIL
            LDR         ev_t, NIL
@next:
            lda         ev_r + 1
            beq         @done
            jsr         car_of_r
            MOVR        ev_v, h_v
            jsr         type_of
            cmp         #TY_SYMBOL
            beq         @symbol
            cmp         #TY_SEXPR
            beq         @scoped
            cmp         #TY_LIST
            bne         @add
            lda         ev_v + 1                            ; (NIL: as it is)
            beq         @add
@scoped:
            MOVR        h_v, ev_v
            jsr         scoped_copy
            bcs         @no
            bra         @add
@symbol:
            MOVR        h_a, ev_v
            MOVR        h_d, ev_e
            lda         #OT_SYMREF
            ldx         #0
            jsr         make_cell2
            bcs         @no
            MOVR        ev_v, h_v
@add:
            jsr         ev_append
            bcs         @no
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            bra         @next
@done:
            clc
@no:
            rts

; ---- Errors

; No room: ev_v = the error, for the frame waiting
ev_no_room:
            MOVR        ev_v, err_room
            jmp         ev_return

; A special form's error (the text at r0): ev_v, for the frame waiting
sf_error:
            jsr         ev_error_text
            jmp         ev_return

; Ctrl-C: ev_v = the error interrupted (its code :intr), for the frame waiting
ev_interrupted:
            LDR         r0, es_interrupted
            jsr         error_text
            bcs         ev_no_room
            jsr         deref
            ldy         #4
            lda         atom_intr
            sta         (h_p),y
            iny
            lda         atom_intr + 1
            sta         (h_p),y
            MOVR        ev_v, h_v
            jmp         ev_return

; ev_v isn't a function, but has arguments: S-Expression starts with incorrect type. Got X, Expected Function.
ev_not_function:
            LDR         r0, es_not_fn1
            jsr         msg_start
            MOVR        h_v, ev_v
            jsr         type_of
            tax
            lda         ty_name_lo - 1,x
            sta         r0
            lda         ty_name_hi - 1,x
            sta         r0 + 1
            jsr         msg_text
            LDR         r0, es_not_fn2
            jsr         msg_text
            jsr         msg_error
            jmp         ev_return

; ---- The special forms (BF_SPECIAL: ev_r their arguments as they're written, ev_e the scope)

; (if test then [else]): then's value if test isn't NIL, else else's (NIL if there's none), in the caller's place
sf_if:
            jsr         count_r
            cmp         #2
            bcs         :+
            LDR         r0, es_if_few
            jmp         sf_error
:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_IF
            jmp         ev_first

k_if:
            POPV        ev_e
            POPV        ev_r                                ; (then, [else])
            jsr         ev_is_error
            bcs         @value
            lda         ev_v + 1
            ora         ev_v
            bne         @then
            MOVR        h_v, ev_r                           ; (NIL: else, if there is one)
            jsr         cdr
            lda         h_v + 1
            bne         :+
            LDR         ev_v, NIL
@value:
            jmp         ev_return
:
            MOVR        ev_r, h_v
@then:
            jmp         ev_first

; (do expr ...): each in turn (an error stops them), the last in the caller's place
sf_do:
            lda         ev_r + 1
            bne         do_step
            LDR         ev_v, NIL
            jmp         ev_return
do_step:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v + 1
            bne         :+
            jmp         ev_first                            ; (The last)
:
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_DO
            jmp         ev_first

k_do:
            POPV        ev_e
            POPV        ev_r
            jsr         ev_is_error
            bcc         do_step
            jmp         ev_return

; (and expr ...), (or expr ...): T or NIL, those after it not evaluated once it's known; an error stops them
sf_and:
            lda         ev_r + 1
            bne         :+
            LDR         ev_v, T_VAL
            jmp         ev_return
:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_AND
            jmp         ev_first

k_and:
            POPV        ev_e
            POPV        ev_r
            jsr         ev_is_error
            bcs         @done
            lda         ev_v + 1
            ora         ev_v
            bne         sf_and
            LDR         ev_v, NIL
@done:
            jmp         ev_return

sf_or:
            lda         ev_r + 1
            bne         :+
            LDR         ev_v, NIL
            jmp         ev_return
:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_OR
            jmp         ev_first

k_or:
            POPV        ev_e
            POPV        ev_r
            jsr         ev_is_error
            bcs         @done
            lda         ev_v + 1
            ora         ev_v
            beq         sf_or
            LDR         ev_v, T_VAL
@done:
            jmp         ev_return

; (let body) or (let {{name value} ...} body ...): the body in a scope of its own, the names bound in it first (each
; value evaluated there, in turn); each of the body's expressions an S-expression's value, a Q-expression run as
; code, or a name's value (run, if it's code); the last in the caller's place
sf_let:
            lda         ev_r + 1
            bne         :+
            LDR         r0, es_let_few
            jmp         sf_error
:
            LDR         ev_s, NIL                           ; (Its scope)
            jsr         make_frame
            bcc         :+
            jmp         ev_no_room
:
            MOVR        ev_e, h_v
            MOVR        h_v, ev_r                           ; (One argument: the body alone)
            jsr         cdr
            lda         h_v + 1
            bne         :+
            jmp         let_body
:
            MOVR        ev_s, h_v                           ; (ev_s: the body; ev_r: the bindings)
            jsr         car_of_r
            MOVR        ev_r, h_v
            jsr         type_of
            cmp         #TY_LIST
            beq         :+
            LDR         r0, es_let_bindings
            jmp         sf_error
:
            MOVR        h_v, ev_r
            jsr         list_of
            MOVR        ev_r, h_v
let_binding:
            lda         ev_r + 1                            ; (No more: the body)
            bne         :+
            MOVR        ev_r, ev_s
            jmp         let_body
:
            jsr         car_of_r                            ; (A binding: {name value}, or {name})
            MOVR        ev_t, h_v
            jsr         type_of
            cmp         #TY_LIST
            jne         @bad
            lda         ev_t + 1
            jeq         @bad
            MOVR        h_v, ev_t
            jsr         list_of
            MOVR        ev_t, h_v
            jsr         car
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_SYMBOL
            jne         @bad
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            lda         h_v + 1
            beq         @nil
            jsr         cdr                                 ; (More than {name value}: wrong)
            lda         h_v + 1
            bne         @bad
            PUSHV       ev_x
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_s
            PUSHV       ev_e
            PUSHK       K_LETB
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            jmp         ev_eval
@nil:
            LDR         ev_v, NIL                           ; ({name}: NIL)
            MOVR        ev_w, ev_e
            jsr         bind
            bcs         @room
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            jmp         let_binding
@bad:
            LDR         r0, es_let_binding
            jmp         sf_error
@room:
            jmp         ev_no_room

k_letb:
            POPV        ev_e
            POPV        ev_s
            POPV        ev_r
            POPV        ev_x
            jsr         ev_is_error
            bcc         :+
            jmp         ev_return
:
            MOVR        ev_w, ev_e
            jsr         bind
            bcs         :+
            jmp         let_binding
:
            jmp         ev_no_room

; The let's body, ev_r, in ev_e: each in turn (K_LETS), the last in the caller's place
let_body:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v + 1
            beq         :+
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_LETS
:
            jsr         car_of_r
            MOVR        ev_x, h_v                           ; (An S-expression: its value; a Q-expression: run)
            jsr         type_of
            cmp         #TY_SEXPR
            beq         @eval
            cmp         #TY_LIST
            bne         @value
            lda         ev_x + 1
            beq         @value
            MOVR        h_v, ev_x
            jmp         run_code
@value:
            PUSHV       ev_e                                ; (Else its value, run if it's code)
            PUSHK       K_LETR
@eval:
            jmp         ev_eval

k_lets:
            POPV        ev_e
            POPV        ev_r
            jsr         ev_is_error
            bcs         :+
            jmp         let_body
:
            jmp         ev_return

k_letr:
            POPV        ev_e
            MOVR        h_v, ev_v
            jsr         type_of
            cmp         #TY_LIST
            bne         :+
            lda         ev_v + 1
            beq         :+
            MOVR        h_v, ev_v
            jmp         run_code
:
            jmp         ev_return

; h_v, an argument as it's written: a Q-expression (not NIL) run as code, anything else evaluated, in ev_e
run_or_eval:
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_LIST
            bne         :+
            lda         ev_x + 1
            beq         :+
            MOVR        h_v, ev_x
            jmp         run_code
:
            jmp         ev_eval

; h_v (a Q-expression, not NIL) run as code: its chain evaluated as an S-expression, in its scope (or ev_e)
run_code:
            jsr         deref
            ldy         #5
            lda         (h_p),y
            dey
            ora         (h_p),y
            beq         :+
            lda         (h_p),y
            sta         ev_e
            iny
            lda         (h_p),y
            sta         ev_e + 1
:
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         ev_r
            stx         ev_r + 1
            jmp         ev_chain

; (try expr [handler]): expr's value (a Q-expression run as code); if that's an error, NIL, or the handler's value,
; the handler evaluated (or run) in a scope with &err the error's message and &code its code (an atom, or NIL); if
; that's a function, it's called with the message
sf_try:
            jsr         count_r
            cmp         #1
            bcc         @wrong
            cmp         #3
            bcs         @wrong
            MOVR        h_v, ev_r                           ; (The handler, or UNBOUND)
            jsr         cdr
            lda         h_v + 1
            beq         :+
            jsr         car
            bra         :++
:
            LDR         h_v, UNBOUND
:
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_TRY
            jsr         car_of_r
            jmp         run_or_eval
@wrong:
            LDR         r0, es_try_args
            jmp         sf_error

k_try:
            POPV        ev_e
            POPV        ev_r                                ; (The handler)
            jsr         ev_is_error
            bcs         :+
            jmp         ev_return
:
            lda         ev_r + 1
            bne         @handler
            lda         ev_r
            cmp         #<UNBOUND
            bne         @handler
            LDR         ev_v, NIL                           ; (None: NIL)
            jmp         ev_return
@handler:
            LDR         ev_s, NIL                           ; (Its scope: &err, &code)
            jsr         make_frame
            bcs         @room
            MOVR        ev_w, h_v
            MOVR        ev_e, h_v
            MOVR        h_v, ev_v                           ; (The error's message, and its code)
            jsr         deref
            ldy         #2
            lda         (h_p),y
            sta         ev_t
            iny
            lda         (h_p),y
            sta         ev_t + 1
            iny
            lda         (h_p),y
            sta         ev_s
            iny
            lda         (h_p),y
            sta         ev_s + 1
            MOVR        ev_x, sym_code
            MOVR        ev_v, ev_s
            jsr         bind
            bcs         @room
            MOVR        ev_x, sym_err
            MOVR        ev_v, ev_t
            jsr         bind
            bcs         @room
            PUSHV       ev_t                                ; (The message, for a function)
            PUSHK       K_TRYH
            MOVR        h_v, ev_r
            jmp         run_or_eval
@room:
            jmp         ev_no_room

k_tryh:
            POPV        ev_a                                ; (The message)
            MOVR        h_v, ev_v
            jsr         type_of
            cmp         #TY_FUNC
            beq         :+
            jmp         ev_return
:
            MOVR        h_a, ev_a                           ; (A function: called with it)
            LDR         h_d, NIL
            jsr         cons
            bcs         :+
            MOVR        ev_a, h_v
            MOVR        ev_f, ev_v
            jmp         ev_apply
:
            jmp         ev_no_room

; (eval x): x's value run: a Q-expression as code, in its scope (or here); an fexpr's argument (an S-expression, a
; symbol) evaluated in its scope; anything else, itself.  In the caller's place
sf_eval:
            jsr         count_r
            cmp         #1
            beq         :+
            LDR         r0, es_eval_args
            jmp         sf_error
:
            PUSHV       ev_e
            PUSHK       K_EVAL
            jmp         ev_first

k_eval:
            POPV        ev_e
            MOVR        h_v, ev_v
            jsr         type_of
            cmp         #TY_LIST
            bne         :+
            lda         ev_v + 1
            beq         @value
            MOVR        h_v, ev_v
            jmp         run_code
:
            cmp         #TY_SEXPR
            beq         @eval
            cmp         #TY_SYMBOL
            beq         @eval
@value:
            jmp         ev_return
@eval:
            MOVR        ev_x, ev_v
            jmp         ev_eval

; (exit code): the program's end, with that status (an error is the value)
k_exit:
            MOVR        h_v, ev_v
            jsr         type_of
            cmp         #TY_NUM
            beq         :+
            jsr         ev_is_error
            bcs         @value
            LDR         r0, es_exit_num
            jmp         sf_error
@value:
            jmp         ev_return
:
            jsr         out_flush
            LDR         r0, 0
            lda         ev_v + 1                            ; (The status: the number's low byte)
            lsr
            lda         ev_v
            ror
            jmp         EXITS

; (def names value ...), (set ...), (set! ...): each name bound to its value (each evaluated in turn): def's
; globally, set's in this scope, set!'s where it's bound nearest (else an error).  The names: a Q-expression of them,
; one alone, or an S-expression whose value is a Q-expression of them
sf_def:
            lda         #0
            bra         sf_bind
sf_set:
            lda         #1
            bra         sf_bind
sf_setbang:
            lda         #2
sf_bind:
            sta         ev_i                                ; (0 def, 1 set, 2 set!)
            lda         ev_r + 1
            bne         :+
            LDR         ev_s, NIL
            jmp         def_count
:
            jsr         car_of_r                            ; ---- The names (ev_s), and the values (ev_r)
            MOVR        ev_s, h_v
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            MOVR        h_v, ev_s
            jsr         type_of
            cmp         #TY_LIST
            beq         @list
            cmp         #TY_SYMBOL
            beq         @one
            cmp         #TY_SEXPR
            beq         :+
            jmp         def_not_syms
:
            lda         ev_i                                ; (An S-expression: its value)
            ldx         #0
            jsr         ev_push
            PUSHV       ev_r
            PUSHV       ev_e
            PUSHK       K_DEFN
            MOVR        ev_x, ev_s
            jmp         ev_eval
@list:
            MOVR        h_v, ev_s
            jmp         def_names
@one:
            MOVR        h_a, ev_s                           ; (A name alone: a list of it)
            LDR         h_d, NIL
            jsr         cons
            bcc         def_chain
            jmp         ev_no_room

k_defn:
            POPV        ev_e
            POPV        ev_r
            jsr         ev_pop
            sta         ev_i
            jsr         ev_is_error
            bcc         :+
            jmp         ev_return
:
            jsr         type_of
            cmp         #TY_LIST
            beq         :+
            jmp         def_not_syms
:
            MOVR        h_v, ev_v
def_names:
            jsr         list_of
def_chain:
            MOVR        ev_s, h_v                           ; ---- Every name a symbol, as many as the values
            MOVR        ev_t, h_v
            stz         ev_c
@each:
            lda         ev_t + 1
            beq         @counted
            MOVR        h_v, ev_t
            jsr         car
            jsr         type_of
            cmp         #TY_SYMBOL
            jne         def_not_syms
            inc         ev_c
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            bra         @each
@counted:
            MOVR        h_v, ev_r
            jsr         count_h
            cmp         ev_c
            jne         def_count
            ; (falls into def_next)

; The next name (ev_s) and value (ev_r), for def, set or set! (ev_i)
def_next:
            lda         ev_s + 1
            bne         :+
            LDR         ev_v, NIL
            jmp         ev_return
:
            lda         ev_i
            ldx         #0
            jsr         ev_push
            MOVR        h_v, ev_s
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHV       ev_s
            PUSHK       K_DEF
            jmp         ev_first

k_def:
            POPV        ev_x                                ; (The name's pair)
            POPV        ev_e
            POPV        ev_r
            POPV        ev_s
            jsr         ev_pop
            sta         ev_i
            MOVR        h_v, ev_x
            jsr         car
            MOVR        ev_x, h_v
            lda         ev_i                                ; (def: globally; set: here; set!: nearest)
            beq         @global
            cmp         #1
            beq         @here
            jsr         update
            jcc         def_next
            jmp         ev_return
@global:
            LDR         ev_w, GLOBAL
            bra         :+
@here:
            MOVR        ev_w, ev_e
:
            jsr         bind
            jcc         def_next
            jmp         ev_no_room

; 'def' cannot define non-symbols
def_not_syms:
            jsr         def_quoted
            LDR         r0, es_def_syms
            jsr         msg_text
            jsr         msg_error
            jmp         ev_return

; 'def' passed too many arguments or symbols.  Expected n, got m  (ev_s: the names, ev_r: the values)
def_count:
            jsr         def_quoted
            LDR         r0, es_def_count
            jsr         msg_text
            MOVR        h_v, ev_s
            jsr         count_h
            jsr         msg_dec
            LDR         r0, es_def_got
            jsr         msg_text
            MOVR        h_v, ev_r
            jsr         count_h
            jsr         msg_dec
            jsr         msg_error
            jmp         ev_return

; A message begun with the form's name, quoted ('def', 'set' or 'set!': ev_i)
def_quoted:
            ldx         ev_i
            lda         es_def_lo,x
            sta         r0
            lda         es_def_hi,x
            sta         r0 + 1
            jmp         msg_start

; Ctrl-C noted (a loop's step): ev_v the error interrupted, for the frame waiting.  (Returns only if there's none)
ev_poll:
            lda         ev_intr
            bne         :+
            rts
:
            stz         ev_intr
            pla                                             ; (Not back to the loop)
            pla
            jmp         ev_interrupted

; (while test body ...): the body's expressions in turn (a Q-expression run as code), as long as test (the same) isn't
; NIL; the last value, or NIL if the body never ran (an error stops it)
sf_while:
            lda         ev_r + 1
            bne         :+
            LDR         r0, es_while_few
            jmp         sf_error
:
            LDR         ev_v, NIL
while_test:
            jsr         ev_poll                             ; (ev_r: the arguments, ev_v: the value so far)
            PUSHV       ev_r
            PUSHV       ev_v
            PUSHV       ev_e
            PUSHK       K_WHILET
            jsr         car_of_r
            jmp         run_or_eval

k_whilet:
            POPV        ev_e
            POPV        ev_s
            POPV        ev_r
            jsr         ev_is_error
            bcs         @done
            lda         ev_v + 1
            ora         ev_v
            bne         :+
            MOVR        ev_v, ev_s                          ; (NIL: the value so far)
@done:
            jmp         ev_return
:
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_t, h_v
            MOVR        ev_v, ev_s
while_body:
            lda         ev_t + 1                            ; (ev_t: the body left)
            beq         while_test
            PUSHV       ev_r
            PUSHV       ev_t
            PUSHV       ev_e
            PUSHK       K_WHILEB
            MOVR        h_v, ev_t
            jsr         car
            jmp         run_or_eval

k_whileb:
            POPV        ev_e
            POPV        ev_t
            POPV        ev_r
            jsr         ev_is_error
            bcs         :+
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            bra         while_body
:
            jmp         ev_return

; (each f list), or (each {x list} body ...): f applied to each item of the list (or character of a string) in turn,
; or the body run with x each of them (in a scope of its own: a function made of it, {x} and the body, applied);
; (dotimes n f), or (dotimes {i n} body ...): the same with 0, 1 ... n - 1.  NIL; or the first error
sf_each:
            lda         #0
            bra         :+
sf_dotimes:
            lda         #1
:
            sta         ev_i                                ; (0 each, 1 dotimes; +2: {name x})
            jsr         count_r
            cmp         #2
            bcs         :+
            lda         #0                                  ; ('each' supplied too few parameters)
            jmp         iter_error
:
            jsr         car_of_r                            ; ---- {name x} body ...
            MOVR        ev_s, h_v
            jsr         type_of
            cmp         #TY_LIST
            jne         @two
            lda         ev_s + 1
            jeq         @two
            MOVR        h_v, ev_s
            jsr         list_of
            MOVR        ev_t, h_v
            jsr         count_h
            cmp         #2
            bne         @spec
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            jsr         type_of
            cmp         #TY_SYMBOL
            bne         @spec
            MOVR        h_v, ev_x
            jsr         sym_local
            jsr         make_body_fn                        ; (ev_v: the function: {x}, (run body ...))
            bcs         @room
            inc         ev_i
            inc         ev_i
            lda         ev_i
            ldx         #0
            jsr         ev_push
            PUSHV       ev_v
            PUSHV       ev_e
            PUSHK       K_ITER2
            MOVR        h_v, ev_t                           ; (x's list, or number)
            jsr         cdr
            jsr         car
            MOVR        ev_x, h_v
            jmp         ev_eval
@spec:
            lda         #2                                  ; ('each' expects {name value} or a function first)
            jmp         iter_error
@room:
            jmp         ev_no_room
@two:
            jsr         count_r                             ; ---- f list, n f
            cmp         #2
            beq         :+
            lda         #4                                  ; ('each' expects a function and a list)
            jmp         iter_error
:
            lda         ev_i
            ldx         #0
            jsr         ev_push
            MOVR        h_v, ev_r
            jsr         cdr
            jsr         car
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_ITER1
            jmp         ev_first

; A function of the body ev_r's rest (each's, dotimes's): its formal ev_x, its body (run ...), its closure ev_e.
; OUT: C = 0, ev_v = it; C = 1, no room
make_body_fn:
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        h_d, h_v
            LDR         h_a, BI_RUN
            jsr         cons
            bcs         @no
            MOVR        ev_w, h_v                           ; (Its body)
            MOVR        h_a, ev_x
            LDR         h_d, NIL
            jsr         cons
            bcs         @no
            MOVR        ev_v, h_v                           ; (Its formals, for now)
            ldy         #PK_C16
            jsr         alloc
            bcs         @no
            lda         #OT_FUNC
            sta         (h_p)
            ldy         #1
            lda         #0
            sta         (h_p),y
            iny
            lda         ev_v
            sta         (h_p),y
            iny
            lda         ev_v + 1
            sta         (h_p),y
            iny
            lda         ev_w
            sta         (h_p),y
            iny
            lda         ev_w + 1
            sta         (h_p),y
            iny
            lda         ev_e
            sta         (h_p),y
            iny
            lda         ev_e + 1
            sta         (h_p),y
            iny
            lda         #0
            sta         (h_p),y
            iny
            sta         (h_p),y
            MOVR        ev_v, h_v
            stz         ev_w
            stz         ev_w + 1
            clc
@no:
            rts

k_iter1:
            POPV        ev_e                                ; (Its first argument's value: the second's next)
            POPV        ev_x
            jsr         ev_pop
            sta         ev_i
            jsr         ev_is_error
            bcc         :+
            jmp         ev_return
:
            lda         ev_i
            ldx         #0
            jsr         ev_push
            PUSHV       ev_v
            PUSHV       ev_e
            PUSHK       K_ITER2
            jmp         ev_eval

k_iter2:
            POPV        ev_e                                ; ---- The function, and the items
            POPV        ev_f
            jsr         ev_pop
            sta         ev_i
            jsr         ev_is_error
            bcc         :+
            jmp         ev_return
:
            MOVR        ev_s, ev_v                          ; (ev_s: the items, ev_f: the function)
            lda         ev_i
            cmp         #1
            bne         :+
            MOVR        ev_s, ev_f                          ; (dotimes n f: the other way round)
            MOVR        ev_f, ev_v
:
            MOVR        h_v, ev_f
            jsr         type_of
            cmp         #TY_FUNC
            beq         :+
            lda         #6                                  ; (First 'each' parameter must be a function or ...)
            jmp         iter_error
:
            MOVR        h_v, ev_s
            jsr         type_of
            ldx         ev_i
            cpx         #1
            beq         @number
            cpx         #3
            beq         @number
            cmp         #TY_STRING
            beq         @go
            cmp         #TY_LIST
            beq         @list
            lda         #8                                  ; ('each' expects a list or a String)
            jmp         iter_error
@list:
            MOVR        h_v, ev_s
            jsr         list_of
            MOVR        ev_s, h_v
            bra         @go
@number:
            cmp         #TY_NUM
            beq         :+
            lda         #10                                 ; ('dotimes' expects a Number)
            jmp         iter_error
:
            lda         ev_s                                ; (Not a fixnum: its integer part)
            lsr
            bcs         @go
            MOVR        h_v, ev_s
            FARN        3, num_trunc
            MOVR        ev_s, h_v
@go:
            LDR         ev_t, 1                             ; (The place: 0, a fixnum)
            ; (falls into iter_next)

; The next item of ev_s (a chain, a string, or a number: ev_t the place in it) given to ev_f; at the end, NIL
iter_next:
            jsr         ev_poll
            lda         ev_s
            lsr
            bcs         @number
            lda         ev_s + 1
            jeq         @end
            ldx         ev_s + 1
            lda         page_kind,x
            cmp         #PK_PAIR
            beq         @pair
            MOVR        h_v, ev_s                           ; (A string: its character at ev_t; else a number
            jsr         deref                               ; past a fixnum)
            lda         (h_p)
            cmp         #OT_STRING
            jne         @big
            lda         ev_t + 1
            lsr
            lda         ev_t
            ror
            ldy         #1
            cmp         (h_p),y
            jcs         @end
            clc
            adc         #2
            tay
            lda         (h_p),y
            asl                                             ; (Its value: $0100 + 2c)
            sta         ev_v
            lda         #>CHARS
            adc         #0
            sta         ev_v + 1
            bra         @next
@number:
            lda         ev_t                                ; (A number: ev_t, while it's less)
            lsr
            jcc         @big
            sec
            lda         ev_t
            sbc         ev_s
            lda         ev_t + 1
            sbc         ev_s + 1
            bvc         :+
            eor         #$80
:
            jpl         @end
            MOVR        ev_v, ev_t
            lda         ev_t + 1                            ; (16383: the next isn't a fixnum)
            cmp         #$7F
            bne         @next
            lda         ev_t
            cmp         #$FF
            jeq         @inc
@next:
            clc                                             ; (The place, on)
            lda         ev_t
            adc         #2
            sta         ev_t
            bcc         @apply
            inc         ev_t + 1
            bra         @apply
@pair:
            MOVR        h_v, ev_s                           ; (A list: its first, and the rest)
            jsr         car
            MOVR        ev_v, h_v
            MOVR        h_v, ev_s
            jsr         cdr
            MOVR        ev_s, h_v
@apply:
            PUSHV       ev_f
            PUSHV       ev_s
            PUSHV       ev_t
            PUSHV       ev_e
            PUSHK       K_ITER
            MOVR        h_a, ev_v
            LDR         h_d, NIL
            jsr         cons
            bcs         @room
            MOVR        ev_a, h_v
            jmp         ev_apply
@big:
            MOVR        nx, ev_t                            ; (Past fixnums: numbers.inc's order and sum)
            MOVR        h_v, ev_s
            FARN        3, num_cmp
            cmp         #$FF
            bne         @end
            MOVR        ev_v, ev_t
@inc:
            MOVR        nx, ev_t
            LDR         h_v, 3
            lda         #NO_ADD
            FARN        3, num_op
            MOVR        ev_t, h_v
            jmp         @apply
@end:
            LDR         ev_v, NIL
            jmp         ev_return
@room:
            jmp         ev_no_room

k_iter:
            POPV        ev_e
            POPV        ev_t
            POPV        ev_s
            POPV        ev_f
            jsr         ev_is_error
            bcs         :+
            jmp         iter_next
:
            jmp         ev_return

; each's or dotimes's error .A (an even number: es_iter_lo/hi's each's; dotimes's is one on)
iter_error:
            sta         ev_c
            lda         ev_i
            and         #1
            ora         ev_c
            tax
            lda         es_iter_lo,x
            sta         r0
            lda         es_iter_hi,x
            sta         r0 + 1
            jmp         sf_error

; (run expr ...), each's and dotimes's body (no name: a built-in made, BI_RUN): each expression in turn, a
; Q-expression run as code (an error stops them), the last in the caller's place
sf_run:
            lda         ev_r + 1
            bne         run_step
            LDR         ev_v, NIL
            jmp         ev_return
run_step:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v + 1
            beq         :+
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_RUN
:
            jsr         car_of_r
            jmp         run_or_eval

k_run:
            POPV        ev_e
            POPV        ev_r
            jsr         ev_is_error
            bcc         run_step
            jmp         ev_return

; (output-of expr ...): what the expressions write (to fd 1), as a string, each evaluated in turn; an error stops
; them, and is the value.  The text an outer output-of has caught so far is kept, a string, meanwhile
sf_output:
            LDR         ev_s, UNBOUND                       ; (None)
            bit         oc_on
            bpl         :+
            LDR         r0, oc_buf
            lda         oc_n
            jsr         buf_string
            MOVR        ev_s, h_v
:
            lda         #$80
            sta         oc_on
            stz         oc_n
            LDR         ev_v, NIL
output_step:
            lda         ev_r + 1
            beq         output_end
            PUSHV       ev_s
            PUSHV       ev_r
            PUSHV       ev_e
            PUSHK       K_OUTPUT
            jmp         ev_first

k_output:
            POPV        ev_e
            POPV        ev_r
            POPV        ev_s
            jsr         ev_is_error
            bcs         output_end
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            bra         output_step

output_end:
            jsr         ev_is_error                         ; (ev_v: NIL, or an error)
            bcs         :+
            LDR         r0, oc_buf
            lda         oc_n
            jsr         buf_string
            MOVR        ev_v, h_v
:
            stz         oc_on                               ; (The outer's text back)
            lda         ev_s + 1
            bne         :+
            lda         ev_s
            cmp         #<UNBOUND
            beq         @done
:
            stz         oc_n
            MOVR        h_v, ev_s
            jsr         type_of
            cmp         #TY_STRING
            bne         @on
            jsr         deref
            ldy         #1
            lda         (h_p),y
            sta         oc_n
            ldx         #0
            iny
:
            cpx         oc_n
            beq         @on
            lda         (h_p),y
            sta         oc_buf,x
            inx
            iny
            bra         :-
@on:
            lda         #$80
            sta         oc_on
@done:
            jmp         ev_return

; (<=> n {test value} ... {value}): n compared with 0 by each block's test in turn (=, <>, <, >, <= or =<, >= or =>),
; the first that holds giving the value (its last item evaluated, in the caller's place); a block of one is the
; default.  NIL if none holds
sf_space:
            jsr         count_r
            cmp         #2
            bcs         :+
            LDR         r0, es_space_few
            jmp         sf_error
:
            cmp         #5
            bcc         :+
            LDR         r0, es_space_many
            jmp         sf_error
:
            MOVR        h_v, ev_r                           ; (Each block a Q-expression)
            jsr         cdr
            MOVR        ev_t, h_v
@block:
            lda         ev_t + 1
            beq         @blocks
            MOVR        h_v, ev_t
            jsr         car
            jsr         type_of
            cmp         #TY_LIST
            bne         @bad
            lda         h_v + 1
            beq         @bad
            MOVR        h_v, ev_t
            jsr         cdr
            MOVR        ev_t, h_v
            bra         @block
@bad:
            LDR         r0, es_space_blocks
            jmp         sf_error
@blocks:
            MOVR        h_v, ev_r
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            jsr         ev_push
            PUSHV       ev_e
            PUSHK       K_SPACE
            jmp         ev_first

k_space:
            POPV        ev_e
            POPV        ev_r                                ; (The blocks)
            jsr         ev_is_error
            bcc         :+
            jmp         ev_return
:
            MOVR        h_v, ev_v                           ; (A number: its sign, ev_i: $FF, 0, 1, numbers.inc's)
            FARN        3, num_sign_of
            bcc         :+
            LDR         r0, es_space_num
            jmp         sf_error
:
            sta         ev_i
@block:
            lda         ev_r + 1
            bne         :+
            LDR         ev_v, NIL                           ; (None held)
            jmp         ev_return
:
            jsr         car_of_r                            ; (ev_t: the block's items)
            jsr         list_of
            MOVR        ev_t, h_v
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            MOVR        h_v, ev_t
            jsr         count_h
            cmp         #1
            beq         @match                              ; (The default)
            MOVR        h_v, ev_t                           ; (Its test: a symbol's name)
            jsr         car
            jsr         space_test
            bcs         @unknown
            and         space_holds,x
            beq         @block
@match:
            MOVR        h_v, ev_t                           ; (Its last, evaluated)
@last:
            jsr         cdr
            lda         h_v + 1
            beq         :+
            MOVR        ev_t, h_v
            bra         @last
:
            MOVR        h_v, ev_t
            jsr         car
            MOVR        ev_x, h_v
            jmp         ev_eval
@unknown:
            LDR         r0, es_space_test
            jsr         msg_start
            MOVR        h_v, ev_t
            jsr         car
            jsr         type_of
            cmp         #TY_SYMBOL
            bne         :+
            jsr         msg_name
:
            jsr         msg_error
            jmp         ev_return

; <=>'s test h_v (a symbol: =, <> ...) against the sign ev_i.  OUT: C = 0, .X = the test's number, .A = the sign's
; bit (space_holds,x has it if the test holds); C = 1, it isn't one
space_test:
            jsr         type_of
            cmp         #TY_SYMBOL
            bne         @no
            jsr         deref
            ldy         #1
            lda         (h_p),y
            and         #$7F
            cmp         #3
            bcs         @no
            tax                                             ; (Its name: one or two characters, ob_t and ob_h)
            ldy         #SYM_NAME
            lda         (h_p),y
            sta         ob_t
            stz         ob_h
            cpx         #2
            bne         :+
            iny
            lda         (h_p),y
            sta         ob_h
:
            ldx         #SPACE_TESTS - 1
@look:
            lda         ob_t
            cmp         space_name1,x
            bne         @next
            lda         ob_h
            cmp         space_name2,x
            beq         @found
@next:
            dex
            bpl         @look
@no:
            sec
            rts
@found:
            ldy         ev_i                                ; (The sign's bit: 1 less, 2 the same, 4 more)
            iny
            lda         #1
            cpy         #0
            beq         :++
:
            asl
            dey
            bne         :-
:
            clc
            rts

.rodata
k_lo:       .lobytes    k_done, k_head, k_arg, k_if, k_do, k_and, k_or, k_letb, k_lets, k_letr, k_try, k_tryh, k_eval
            .lobytes    k_exit, k_defn, k_def, k_whilet, k_whileb, k_iter1, k_iter2, k_iter, k_run, k_output, k_space
k_hi:       .hibytes    k_done, k_head, k_arg, k_if, k_do, k_and, k_or, k_letb, k_lets, k_letr, k_try, k_tryh, k_eval
            .hibytes    k_exit, k_defn, k_def, k_whilet, k_whileb, k_iter1, k_iter2, k_iter, k_run, k_output, k_space

; Each type's name (TY_*), as danlang's messages have it (LVal.LEName)
ty_name_lo: .lobytes    es_ty_num, es_ty_qexpr, es_ty_unknown, es_ty_unknown, es_ty_fun, es_ty_char, es_ty_str
            .lobytes    es_ty_sym, es_ty_atom, es_ty_sexpr, es_ty_err, es_ty_unknown, es_ty_unknown, es_ty_unknown
ty_name_hi: .hibytes    es_ty_num, es_ty_qexpr, es_ty_unknown, es_ty_unknown, es_ty_fun, es_ty_char, es_ty_str
            .hibytes    es_ty_sym, es_ty_atom, es_ty_sexpr, es_ty_err, es_ty_unknown, es_ty_unknown, es_ty_unknown
es_ty_num:  .byte       "Number", 0
es_ty_qexpr: .byte      "Q-Expression", 0
es_ty_unknown: .byte    "Unknown", 0
es_ty_fun:  .byte       "Function", 0
es_ty_char: .byte       "Character", 0
es_ty_str:  .byte       "String", 0
es_ty_sym:  .byte       "Symbol", 0
es_ty_atom: .byte       "Atom", 0
es_ty_sexpr: .byte      "S-Expression", 0
es_ty_err:  .byte       "Error", 0

es_def_lo:  .lobytes    es_def, es_set, es_setbang
es_def_hi:  .hibytes    es_def, es_set, es_setbang
es_def:     .byte       "'def'", 0
es_set:     .byte       "'set'", 0
es_setbang: .byte       "'set!'", 0

es_amp_rest: .byte      2, "&_"
es_amp_err: .byte       4, "&err"
es_amp_code: .byte      5, "&code"
es_intr:    .byte       4, "intr"
es_no_room: .byte       "No room", 0
es_nested:  .byte       "Too deep: native code's evaluations nested", 0
es_deep:    .byte       "Too deep: calls nested", 0
es_interrupted: .byte   "interrupted", 0
es_not_fn1: .byte       "S-Expression starts with incorrect type. Got ", 0
es_not_fn2: .byte       ", Expected Function.", 0
es_if_few:  .byte       "'if' supplied too few parameters", 0
es_let_few: .byte       "'let' supplied too few parameters", 0
es_let_bindings: .byte  "'let' expects a list of bindings first", 0
es_let_binding: .byte   "A 'let' binding is {name value}", 0
es_try_args: .byte      "'try' expects an expression and maybe a handler", 0
es_eval_args: .byte     "Incorrect number of parameters passed to 'eval'", 0
es_exit_num: .byte      "'exit' expects a Number", 0
es_def_syms: .byte      " cannot define non-symbols", 0
es_def_count: .byte     " passed too many arguments or symbols.  Expected ", 0
es_def_got: .byte       ", got ", 0
es_while_few: .byte     "'while' supplied too few parameters", 0
es_iter_lo: .lobytes    es_each_few, es_dotimes_few, es_each_spec, es_dotimes_spec, es_each_args, es_dotimes_args
            .lobytes    es_each_fn, es_dotimes_fn, es_each_list, es_dotimes_num, es_each_list, es_dotimes_num
es_iter_hi: .hibytes    es_each_few, es_dotimes_few, es_each_spec, es_dotimes_spec, es_each_args, es_dotimes_args
            .hibytes    es_each_fn, es_dotimes_fn, es_each_list, es_dotimes_num, es_each_list, es_dotimes_num
es_each_few: .byte      "'each' supplied too few parameters", 0
es_dotimes_few: .byte   "'dotimes' supplied too few parameters", 0
es_each_spec: .byte     "'each' expects {name value} or a function first", 0
es_dotimes_spec: .byte  "'dotimes' expects {name value} or a function first", 0
es_each_args: .byte     "'each' expects a function and a list", 0
es_dotimes_args: .byte  "'dotimes' expects a Number and a function", 0
es_each_fn: .byte       "First 'each' parameter must be a function or {name list}", 0
es_dotimes_fn: .byte    "Second 'dotimes' parameter must be a function", 0
es_each_list: .byte     "'each' expects a list or a String", 0
es_dotimes_num: .byte   "'dotimes' expects a Number", 0
es_space_few: .byte     "Too few parameters passed to '<=>' operator", 0
es_space_many: .byte    "Too many parameters passed to '<=>' operator", 0
es_space_blocks: .byte  "Operator '<=>' received one or more invalid case blocks (not QExpr)", 0
es_space_num: .byte     "First parameter to '<=>' must evaluate to a Number", 0
es_space_test: .byte    "Unknown comparison test ", 0

; <=>'s tests: their names (one or two characters), and the signs each holds for (1 less, 2 the same, 4 more)
SPACE_TESTS     = 8
space_name1: .byte      "=<><><=="
space_name2: .byte      0, ">", 0, 0, "=", "=", ">", "<"
space_holds: .byte      2, 5, 4, 1, 6, 3, 6, 3
.code
