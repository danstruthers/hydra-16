; ****************************************************************************
; forms.inc - hylang's special forms (phase 3), in its first bank, with the evaluator (eval.inc): each gets its
; arguments as they're written (cr; bi_count of them) in ee, and runs the machine its own way, a frame for each
; value it waits for; and the built-ins that run it too (BF_MACHINE: map, filter, the folds, any?, all?, find,
; count, sum, product and sort, from phase 4; load).  Each as danlang's (Builtins.cs), its errors' messages too.

.bss
ou_start:   .res        2                                   ; (output-of's capture: where it began)
fm_kind:    .res        1                                   ; (def's, set's or set!'s: 0, 1, 2)
fm_n:       .res        2                                   ; (A count, a number: <=>'s, dotimes', def's)
fm_m:       .res        2

.code

; ex, written as an argument, evaluated in ee: a Q-expression run as code (its items a call, here), anything else
; evaluated (while's, each's, dotimes', try's, let's)
ev_evalarg:
            lda         ex
            ldx         ex + 1
            jsr         list_kind
            cmp         #PK_QCONS
            bne         :+
            jmp         ev_call
:
            jmp         ev_eval

; The error .A, .X (a text in this bank) as a special form's value
sf_error:
            jmp         ev_error

; ---- if, do, and, or, eval

; (if test then [else]): then's value if test isn't NIL, else else's (NIL if there's none), in tail position
sf_if:
            lda         bi_count
            cmp         #2
            bcs         :+
            LDAX        s_iffew
            jmp         sf_error
:
            jsr         cr_next                             ; (The condition: not a call, or a quick one, here;
            jsr         is_call                             ;   else in a frame)
            bcc         @here
            jsr         ev_quick
            bcc         if_value
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_IF
            jmp         ev_eval
@here:
            jsr         ev_simple
            jsr         is_err
            bcc         if_value
            jmp         ev_return
k_if:
            POPV        ee
            POPV        cr
if_value:
            jsr         is_nil
            bcs         @else
            jsr         cr_first
            jmp         ev_eval
@else:
            lda         cr                                  ; (else: the second, if there is one)
            ldx         cr + 1
            ldy         #1
            jsr         cell_get
            sta         cr
            stx         cr + 1
            ora         cr + 1
            bne         :+
            stz         ex
            stz         ex + 1
            jmp         ev_return
:
            jsr         cr_first
            jmp         ev_eval

; (newns): the default namespace built again in hylang's own (a child's is its own once it changes it, so not by
; rc's newns): (load "newns"), /lib/hylang/newns.hl, which does it each time it's loaded, in tail position
sf_newns:
            LDHQ        s_newnsf, s_newnsf_n
            jsr         string_make
            bcs         @nomem
            MOVW        et, hv
            stz         eu
            stz         eu + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @nomem
            MOVW        eu, hv
            lda         #<(BUILTIN0 + 2 * BIN_LOAD)
            sta         et
            lda         #>(BUILTIN0 + 2 * BIN_LOAD)
            sta         et + 1
            lda         #PK_SCONS
            jsr         make2
            bcs         @nomem
            MOVW        ex, hv
            jmp         ev_eval
@nomem:
            jmp         ev_nomem

.rodata
s_newnsf:   .byte       "newns"
s_newnsf_n = * - s_newnsf

.code

; (hold body...): the body as do has it, with no task switch meanwhile (PREEMPT_OFF, for a few ticks' timing; a note
; still comes); preemption back on after it, and after an error (k_hold, which takes one)
sf_hold:
            jsr         PREEMPT_OFF
            inc         hold_n
            jsr         frame_check
            PUSHK       K_HOLD
            jmp         sf_do
k_hold:
            jsr         PREEMPT_ON
            dec         hold_n
            jmp         ev_return

; (do x...): each in turn (an error stops them); the last's value, in tail position; NIL for none
sf_do:
            lda         cr
            ora         cr + 1
            bne         do_next
            stz         ex
            stz         ex + 1
            jmp         ev_return
do_next:
            jsr         cr_next
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         ev_eval
:
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_DO
            jmp         ev_eval
k_do:
            POPV        ee
            POPV        cr
            bra         do_next

; (and x...): the first that's NIL (the rest not evaluated), or the last's value; T for none.  An error stops it
sf_and:
            lda         cr
            ora         cr + 1
            bne         and_next
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         ev_return
and_next:
            jsr         cr_next
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         ev_eval
:
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_AND
            jmp         ev_eval
k_and:
            POPV        ee
            POPV        cr
            jsr         is_nil
            bcc         and_next
            jmp         ev_return

; (or x...): the first value that isn't NIL (the rest not evaluated), or NIL.  An error stops it
sf_or:
            lda         cr
            ora         cr + 1
            bne         or_next
            stz         ex
            stz         ex + 1
            jmp         ev_return
or_next:
            jsr         cr_next
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         ev_eval
:
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_OR
            jmp         ev_eval
k_or:
            POPV        ee
            POPV        cr
            jsr         is_nil
            bcs         or_next
            jmp         ev_return

; (eval x): x's value run: a Q-expression as code, in the scope it remembers; an S-expression or a symbol (an
; fexpr's argument) evaluated in its scope; anything else itself.  In tail position
sf_eval:
            lda         bi_count
            cmp         #1
            beq         :+
            LDAX        s_evaln
            jmp         sf_error
:
            jsr         cr_first
            jsr         frame_check
            PUSHV       ee
            PUSHK       K_EVAL
            jmp         ev_eval
k_eval:
            POPV        ee
ev_run_value:
            lda         ex
            ldx         ex + 1
            jsr         list_kind
            cmp         #PK_SCOPED
            bne         @plain
            lda         ex                                  ; (Scoped: its expression, in its scope)
            ldx         ex + 1
            ldy         #1
            jsr         cell_get
            sta         ee
            stx         ee + 1
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            sta         ex
            stx         ex + 1
            jsr         list_kind
@plain:
            cmp         #PK_QCONS                           ; (A list: run, as a call)
            beq         @run
            cmp         #PK_SCONS
            beq         @run
            lda         ex                                  ; (A symbol: its value)
            lsr
            bcs         @self
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @self
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @self
            jmp         lookup_return
@run:
            jmp         ev_call
@self:
            jmp         ev_return

; ex, a symbol, looked up: its value given to the frame on top
lookup_return:
            jsr         lookup
            jmp         ev_return

; ---- def, set, set!

; (def names values...): NIL, each name bound to its value, globally (set: in this scope; set!: where it's bound
; nearest).  The names: a symbol, a list of them, or an S-expression whose value is one
sf_def:
            lda         #0
            bra         sf_var
sf_set:
            lda         #1
            bra         sf_var
sf_setb:
            lda         #2
sf_var:
            sta         fm_kind
            lda         cr
            ora         cr + 1
            bne         :+
            LDAX        s_varfew
            jmp         sf_var_error
:
            jsr         cr_next
            jsr         is_call
            bcc         var_names
            jsr         frame_check                         ; (An S-expression: evaluated first)
            PUSHV       cr
            PUSHV       ee
            lda         fm_kind
            ldx         #0
            jsr         fix_ax
            jsr         push
            PUSHK       K_DEFN
            jmp         ev_eval
k_defn:
            jsr         pop
            jsr         unfix
            sta         fm_kind
            POPV        ee
            POPV        cr
; ex, def's names (a symbol, a list of them): checked against cr, its values; then each bound in turn
var_names:
            lda         ex                                  ; (A symbol: a list of one)
            lsr
            bcs         @list
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @list
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @list
            MOVW        et, ex
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         make2
            bcc         :+
            jmp         ev_nomem
:
            MOVW        ex, hv
@list:
            lda         ex                                  ; (A list of symbols)
            ldx         ex + 1
            jsr         q_conses
            bcs         @nonsym
            sta         et
            stx         et + 1
            sta         ht
            stx         ht + 1
@sym:
            lda         ht
            ora         ht + 1
            beq         @count
            lda         ht
            ldx         ht + 1
            jsr         deref
            ldy         #1
            lda         (hp),y
            tax
            lda         (hp)
            lsr
            bcs         @nonsym
            cpx         #IMM_PAGES
            bcc         @nonsym
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @nonsym
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            sta         ht + 1
            pla
            sta         ht
            bra         @sym
@nonsym:
            jsr         err_begin                           ; ("'def' cannot define non-symbols")
            jsr         out_form
            LDAX        s_nonsym
            jsr         msg_out
            jmp         ev_err_end
@count:
            lda         et                                  ; (As many values as names)
            ldx         et + 1
            jsr         count_list
            sta         fm_n
            jsr         count_cr
            sta         fm_m
            cmp         fm_n
            beq         var_next
            jsr         err_begin                           ; ("'def' passed too many arguments or symbols.
            jsr         out_form                            ;   Expected 2, got 1")
            LDAX        s_toomanyvars
            jsr         msg_out
            lda         fm_n
            ldx         #0
            jsr         out_dec
            LDAX        s_got
            jsr         msg_out
            lda         fm_m
            ldx         #0
            jsr         out_dec
            jmp         ev_err_end

; et: the names left, cr: their values (as written): the next evaluated, then bound; NIL at the end
var_next:
            lda         et
            ora         et + 1
            bne         :+
            stz         ex
            stz         ex + 1
            jmp         ev_return
:
            jsr         cr_next
            jsr         frame_check
            PUSHV       et
            PUSHV       cr
            PUSHV       ee
            lda         fm_kind
            ldx         #0
            jsr         fix_ax
            jsr         push
            PUSHK       K_DEFV
            jmp         ev_eval
k_defv:
            jsr         pop                                 ; (Its value, an error too: bound)
            jsr         unfix
            sta         fm_kind
            POPV        ee
            POPV        cr
            POPV        et
            lda         et                                  ; (The name: eu the value, et the names left)
            ldx         et + 1
            jsr         deref
            lda         (hp)
            sta         lk_s
            ldy         #1
            lda         (hp),y
            sta         lk_s + 1
            iny
            lda         (hp),y
            sta         et
            iny
            lda         (hp),y
            sta         et + 1
            MOVW        eu, ex
            lda         fm_kind
            beq         @global
            cmp         #1
            beq         @here
            MOVW        ex, lk_s                            ; (set!: where it's bound nearest)
            jsr         find_bound
            bcs         @global_set
            lda         eu
            sta         (hp),y
            iny
            lda         eu + 1
            sta         (hp),y
            jmp         var_next
@global_set:
            lda         ex                                  ; (Not in a frame: its global value, if it has one)
            ldx         ex + 1
            jsr         deref
            ldy         #2
            lda         (hp),y
            cmp         #<UNBOUND
            bne         @global
            iny
            lda         (hp),y
            cmp         #>UNBOUND
            bne         @global
            jsr         err_begin                           ; ("Unbound Symbol 'x'")
            LDAX        s_unbsym
            jsr         msg_out
            lda         ex
            ldx         ex + 1
            jsr         out_name
            lda         #$27
            jsr         out_byte
            jmp         ev_err_end
@here:
            lda         ee                                  ; (set: in this scope; the global one, globally)
            ora         ee + 1
            beq         @global
            lda         lk_s
            ldx         lk_s + 1
            jsr         frame_add
            jmp         var_next
@global:
            lda         lk_s
            ldx         lk_s + 1
            jsr         deref
            jsr         vm_rebind                           ; (A built-in's name: the machine's code compiled again)
            ldy         #2
            lda         eu
            sta         (hp),y
            iny
            lda         eu + 1
            sta         (hp),y
            jmp         var_next

; ex, dotimes' count, not a whole number: ex = its error ("'dotimes': a count must be a whole number, not :a"); each's
; list, not one: ex = its error.  OUT: C = 1 if there's no room (the bytecode machine's DOTN, EACHL)
dot_err:
            jsr         err_begin
            LDAX        s_dotcount
            jsr         msg_out
            lda         ex
            ldx         ex + 1
            ldy         #0
            FARN        2, print_val
            jmp         err_end
each_err:
            LDAX        s_eachlist
            jmp         err_text

; set! of lk_s to eu (the bytecode machine's SETBG): where it's bound nearest, from ee; else its global value (none:
; the error "Unbound Symbol 'x'").  OUT: ex = NIL, or the error; C = 1 if there's no room
set_bang:
            MOVW        ex, lk_s
            jsr         find_bound
            bcs         @global
            lda         eu
            sta         (hp),y
            iny
            lda         eu + 1
            sta         (hp),y
            bra         @nil
@global:
            lda         ex
            ldx         ex + 1
            jsr         deref
            ldy         #3
            lda         (hp),y
            cmp         #>UNBOUND
            bne         @set
            dey
            lda         (hp),y
            cmp         #<UNBOUND
            bne         @set
            jsr         err_begin                           ; ("Unbound Symbol 'x'")
            LDAX        s_unbsym
            jsr         msg_out
            lda         ex
            ldx         ex + 1
            jsr         out_name
            lda         #$27
            jsr         out_byte
            jmp         err_end
@set:
            jsr         vm_rebind
            ldy         #2
            lda         eu
            sta         (hp),y
            iny
            lda         eu + 1
            sta         (hp),y
@nil:
            stz         ex
            stz         ex + 1
            clc
            rts

; The form's name ('def', 'set' or 'set!', in quotes) out
out_form:
            lda         #$27
            jsr         out_byte
            ldx         fm_kind
            lda         form_hi,x
            pha
            lda         form_lo,x
            plx
            jsr         msg_out
            lda         #$27
            jmp         out_byte

sf_var_error:
            jsr         err_begin
            LDAX        s_toofew
            jsr         msg_out
            jsr         out_form
            jmp         ev_err_end

; ---- let

; (let body) or (let {{name value} ...} body...): the body in a scope of its own, the names bound in it first, each
; value evaluated there in turn; a body expression written as a Q-expression run as code; the last's value, in
; tail position
sf_let:
            lda         bi_count
            bne         :+
            LDAX        s_letfew
            jmp         sf_error
:
            MOVW        eu, ee                              ; (Its scope)
            lda         #3
            jsr         frame_new
            MOVW        ee, hv
            lda         bi_count
            cmp         #2
            bcs         :+
            jmp         let_body
:
            jsr         cr_next                             ; (Its bindings: a list)
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcc         :+
            LDAX        s_letlist
            jmp         sf_error
:
            sta         et
            stx         et + 1
; et: the bindings left, cr: the body, ee: the let's scope
let_bind:
            lda         et
            ora         et + 1
            bne         :+
            jmp         let_body
:
            lda         et                                  ; (A binding: ex; et the rest)
            ldx         et + 1
            jsr         deref
            lda         (hp)
            sta         ex
            ldy         #1
            lda         (hp),y
            sta         ex + 1
            iny
            lda         (hp),y
            sta         et
            iny
            lda         (hp),y
            sta         et + 1
            lda         ex                                  ; ({name} or {name value})
            ldx         ex + 1
            jsr         q_conses
            bcc         @lb9
            jmp         @bad
@lb9:
            sta         ht
            stx         ht + 1
            ora         ht + 1
            bne         @lb8
            jmp         @bad
@lb8:
            lda         ht
            jsr         count_list
            cmp         #3
            bcs         @bad
            sta         fm_n
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            ldy         #0
            jsr         cell_get                            ; (Its name: a symbol)
            sta         lk_s
            stx         lk_s + 1
            lsr
            bcs         @bad
            cpx         #IMM_PAGES
            bcc         @bad
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @bad
            lda         fm_n
            cmp         #2
            beq         @value
            stz         eu                                  ; ({name}: NIL)
            stz         eu + 1
            lda         lk_s
            ldx         lk_s + 1
            jsr         frame_add
            bra         let_bind
@value:
            lda         ex                                  ; (Its value: evaluated in the let's scope)
            ldx         ex + 1
            jsr         q_conses
            ldy         #1
            jsr         cell_get
            phx
            pha
            ldy         #0
            jsr         cell_get
            sta         ex
            stx         ex + 1
            pla
            plx
            jsr         frame_check
            PUSHV       et
            PUSHV       cr
            PUSHV       ee
            lda         lk_s
            ldx         lk_s + 1
            jsr         push
            PUSHK       K_LETB
            jmp         ev_eval
@bad:
            LDAX        s_letbind
            jmp         sf_error
k_letb:
            POPV        lk_s
            POPV        ee
            POPV        cr
            POPV        et
            MOVW        eu, ex
            lda         lk_s
            ldx         lk_s + 1
            jsr         frame_add
            jmp         let_bind

; cr: let's body expressions left (one at least), in ee: each, the last in tail position
let_body:
            jsr         cr_next
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         ev_evalarg
:
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_LETX
            jmp         ev_evalarg
k_letx:
            POPV        ee
            POPV        cr
            bra         let_body

; ---- try

; (try x [handler]): x's value; if that's an error, NIL, or the handler's value, evaluated in a scope with &err its
; message and &code its code (a function given the message); a Q-expression run as code
sf_try:
            lda         bi_count
            bne         :+
            LDAX        s_tryargs
            jmp         sf_error
:
            jsr         cr_next
            lda         #<UNBOUND                           ; (No handler: UNBOUND)
            sta         et
            stz         et + 1
            lda         cr
            ora         cr + 1
            beq         :+
            lda         cr
            ldx         cr + 1
            ldy         #0
            jsr         cell_get
            sta         et
            stx         et + 1
:
            jsr         frame_check
            PUSHV       et
            PUSHV       ee
            PUSHK       K_TRY
            jmp         ev_evalarg
k_try:
            POPV        ee
            POPV        et
            jsr         is_err
            bcs         :+
            jmp         ev_return
:
            lda         et                                  ; (An error, and no handler: NIL)
            cmp         #<UNBOUND
            bne         @handler
            lda         et + 1
            bne         @handler
            stz         ex
            stz         ex + 1
            jmp         ev_return
@handler:
            MOVW        eu, ee                              ; (A scope with &err and &code)
            lda         #2
            jsr         frame_new
            MOVW        ee, hv
            lda         ex                                  ; (&err: the message)
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            sta         eu
            stx         eu + 1
            lda         sym_err
            ldx         sym_err + 1
            jsr         frame_add
            lda         ex                                  ; (&code: the code)
            ldx         ex + 1
            ldy         #1
            jsr         cell_get
            sta         eu
            stx         eu + 1
            lda         sym_code
            ldx         sym_code + 1
            jsr         frame_add
            jsr         frame_check
            lda         ex                                  ; (The message kept, for a function)
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            jsr         push
            MOVW        ex, et
            PUSHK       K_TRYH
            jmp         ev_evalarg
k_tryh:
            jsr         pop                                 ; (A function: given the message)
            sta         et
            stx         et + 1
            jsr         is_function
            bcs         :+
            jmp         ev_return
:
            MOVW        cf, ex
            PUSHV       et
            lda         #1
            sta         cn
            stz         cn + 1
            jmp         ev_apply_values

; OUT: C = 1 if ex is a function (a built-in, a partially applied one, a function)
is_function:
            lda         ex
            lsr
            bcs         @no
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcs         @yes
@no:
            clc
            rts
@cell:
            lda         pk,x
            cmp         #PK_FUNC
            beq         @yes
            cmp         #PK_PARTIAL
            bne         @no
@yes:
            sec
            rts

; ---- while

; (while test body...): the body's expressions in turn as long as test isn't NIL (each a Q-expression run as code);
; the last value, or NIL if the body never ran
sf_while:
            lda         bi_count
            bne         :+
            LDAX        s_whilefew
            jmp         sf_error
:
            MOVW        et, cr                              ; (et: its arguments; eu: the last value)
            stz         eu
            stz         eu + 1
while_test:
            bit         intr
            bpl         :+
            jmp         ev_intr
:
            lda         et
            ldx         et + 1
            ldy         #0
            jsr         cell_get
            sta         ex
            stx         ex + 1
            jsr         frame_check
            PUSHV       et
            PUSHV       eu
            PUSHV       ee
            PUSHK       K_WTEST
            jmp         ev_evalarg
k_wtest:
            POPV        ee
            POPV        eu
            POPV        et
            jsr         is_nil
            bcc         :+
            MOVW        ex, eu
            jmp         ev_return
:
            lda         et                                  ; (Its body)
            ldx         et + 1
            ldy         #1
            jsr         cell_get
            sta         cr
            stx         cr + 1
            ora         cr + 1
            beq         while_test
while_body:
            jsr         cr_next
            jsr         frame_check
            PUSHV       et
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_WBODY
            jmp         ev_evalarg
k_wbody:
            POPV        ee
            POPV        cr
            POPV        et
            MOVW        eu, ex
            lda         cr
            ora         cr + 1
            bne         while_body
            jmp         while_test

; ---- each

; (each f list) or (each {x list} body...): f called with each item (or character of a string) in turn, or the
; body run with x each of them, in a scope of its own; NIL, or the first error
sf_each:
            lda         bi_count
            cmp         #2
            bcs         :+
            LDAX        s_eachfew
            jmp         sf_error
:
            jsr         cr_first
            lda         ex
            ldx         ex + 1
            jsr         list_kind
            cmp         #PK_QCONS
            beq         @spec
            lda         bi_count                            ; (A function, and a list)
            cmp         #2
            beq         :+
            LDAX        s_eachfn
            jmp         sf_error
:
            jsr         cr_next
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_EACHF2
            jmp         ev_eval
@spec:
            jsr         loop_spec                           ; ({x list}: x, and list's expression)
            bcc         :+
            LDAX        s_eachspec
            jmp         sf_error
:
            lda         cr                                  ; (The body: the rest)
            ldx         cr + 1
            ldy         #1
            jsr         cell_get
            sta         cr
            stx         cr + 1
            jsr         frame_check
            PUSHV       cr
            PUSHV       lk_s
            PUSHV       ee
            PUSHK       K_EACHL
            jmp         ev_eval

; ex, a loop's binding as written, {name x}: lk_s = name, ex = x.  OUT: C = 1 if it isn't one
loop_spec:
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcs         @no
            sta         fm_m                                ; (Its conses: count_list's ht is its own)
            stx         fm_m + 1
            jsr         count_list
            cmp         #2
            bne         @no
            lda         fm_m
            ldx         fm_m + 1
            ldy         #0
            jsr         cell_get
            sta         lk_s
            stx         lk_s + 1
            lsr
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @no
            lda         fm_m
            ldx         fm_m + 1
            ldy         #1
            jsr         cell_get
            jsr         cell_first
            sta         ex
            stx         ex + 1
            clc
            rts
@no:
            sec
            rts

; IN: .A, .X = a cons.  OUT: .A, .X = its first item
cell_first:
            ldy         #0
            jmp         cell_get

k_eachf2:
            POPV        ee                                  ; (each's function: then its list)
            POPV        cr
            jsr         is_function
            bcs         :+
            LDAX        s_eachfirst
            jmp         sf_error
:
            PUSHV       ex
            jsr         cr_first
            PUSHK       K_EACHF3
            jmp         ev_eval
k_eachf3:
            POPV        cf
            jsr         loop_items                          ; (Its items: et)
            bcc         each_f
            LDAX        s_eachlist
            jmp         sf_error
; cf called with each of et's items in turn
each_f:
            lda         et
            ora         et + 1
            bne         :+
            stz         ex
            stz         ex + 1
            jmp         ev_return
:
            lda         et
            ldx         et + 1
            jsr         deref
            ldy         #2
            lda         (hp),y
            sta         cr
            iny
            lda         (hp),y
            sta         cr + 1
            lda         (hp)
            sta         eu
            ldy         #1
            lda         (hp),y
            sta         eu + 1
            jsr         frame_check
            PUSHV       cf
            PUSHV       cr
            PUSHK       K_EACHF
            PUSHV       eu
            lda         #1
            sta         cn
            stz         cn + 1
            jmp         ev_apply_values
k_eachf:
            POPV        et
            POPV        cf
            bra         each_f

; ex, a loop's list: et = its items (a string's characters, made a list).  OUT: C = 1 if it isn't a list or a string
loop_items:
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcs         @string
            sta         et
            stx         et + 1
            clc
            rts
@string:
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            sec
            rts
:
            PUSHV       cf                                  ; (Its characters: hv, made with cf, cr and eu kept)
            PUSHV       cr
            PUSHV       eu
            FARN        2, str_chars
            php
            POPV        eu
            POPV        cr
            POPV        cf
            plp
            bcc         :+
            jmp         ev_nomem
:
            MOVW        et, hv
            clc
            rts

k_eachl:
            POPV        ee                                  ; (each's list: its items, then the body for each)
            POPV        lk_s
            POPV        cr
            jsr         loop_items
            bcc         :+
            LDAX        s_eachlist
            jmp         sf_error
:
            MOVW        eu, cr                              ; (eu: the body)
            MOVW        cr, et                              ; (cr: the items)
; cr: the items left, eu: the body, lk_s: the name, ee: the scope it's in
each_item:
            lda         cr
            ora         cr + 1
            bne         :+
            stz         ex
            stz         ex + 1
            jmp         ev_return
:
            bit         intr
            bpl         :+
            jmp         ev_intr
:
            jsr         cr_next                             ; (Its scope: the item bound)
            MOVW        et, eu
            MOVW        eu, ee
            lda         #1
            jsr         frame_new
            MOVW        ee, hv
            MOVW        eu, ex
            lda         lk_s
            ldx         lk_s + 1
            jsr         frame_add
            MOVW        eu, et
            MOVW        et, eu                              ; (et: the body expressions left)
each_body:
            lda         et
            ora         et + 1
            bne         :+
            jmp         each_next
:
            lda         et
            ldx         et + 1
            jsr         deref
            lda         (hp)
            sta         ex
            ldy         #1
            lda         (hp),y
            sta         ex + 1
            iny
            lda         (hp),y
            sta         et
            iny
            lda         (hp),y
            sta         et + 1
            jsr         frame_check
            PUSHV       cr
            PUSHV       et
            PUSHV       eu
            PUSHV       lk_s
            PUSHV       ee
            PUSHK       K_EACHB
            jmp         ev_evalarg
k_eachb:
            POPV        ee
            POPV        lk_s
            POPV        eu
            POPV        et
            POPV        cr
            bra         each_body
; The next item, in the scope the loop's in
each_next:
            lda         ee
            ldx         ee + 1
            ldy         #0
            jsr         cell_get
            sta         ee
            stx         ee + 1
            jmp         each_item

; ---- dotimes

; (dotimes n f) or (dotimes {i n} body...): f called with 0, 1 ... n-1 in turn, or the body run with i each of
; them; NIL, or the first error
sf_dotimes:
            lda         bi_count
            cmp         #2
            bcs         :+
            LDAX        s_dotfew
            jmp         sf_error
:
            jsr         cr_first
            lda         ex
            ldx         ex + 1
            jsr         list_kind
            cmp         #PK_QCONS
            beq         @spec
            lda         bi_count                            ; (A count, and a function)
            cmp         #2
            beq         :+
            LDAX        s_dotfn
            jmp         sf_error
:
            jsr         cr_next
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_DOTF2
            jmp         ev_eval
@spec:
            jsr         loop_spec
            bcc         :+
            LDAX        s_dotspec
            jmp         sf_error
:
            lda         cr
            ldx         cr + 1
            ldy         #1
            jsr         cell_get
            sta         cr
            stx         cr + 1
            jsr         frame_check
            PUSHV       cr
            PUSHV       lk_s
            PUSHV       ee
            PUSHK       K_DOTN
            jmp         ev_eval

k_dotf2:
            POPV        ee                                  ; (dotimes' count: then its function)
            POPV        cr
            PUSHV       ex
            jsr         cr_first
            PUSHK       K_DOTF3
            jmp         ev_eval
k_dotf3:
            POPV        eu                                  ; (eu: the count)
            jsr         is_function
            bcs         :+
            LDAX        s_dotsecond
            jmp         sf_error
:
            MOVW        cf, ex
            MOVW        ex, eu
            jsr         loop_count
            stz         fm_m                                ; (fm_m: i)
            stz         fm_m + 1
; cf called with fm_m, then on, up to fm_n
dot_f:
            lda         fm_m
            cmp         fm_n
            lda         fm_m + 1
            sbc         fm_n + 1
            bvc         :+
            eor         #$80
:
            bmi         :+
            stz         ex
            stz         ex + 1
            jmp         ev_return
:
            jsr         frame_check
            PUSHV       cf
            lda         fm_m
            ldx         fm_m + 1
            jsr         fix_ax
            jsr         push
            lda         fm_n
            ldx         fm_n + 1
            jsr         fix_ax
            jsr         push
            PUSHK       K_DOTF
            lda         fm_m
            ldx         fm_m + 1
            jsr         fix_ax
            jsr         push
            lda         #1
            sta         cn
            stz         cn + 1
            jmp         ev_apply_values
k_dotf:
            jsr         pop
            jsr         unfix
            sta         fm_n
            stx         fm_n + 1
            jsr         pop
            jsr         unfix
            sta         fm_m
            stx         fm_m + 1
            POPV        cf
            inc         fm_m
            bne         dot_f
            inc         fm_m + 1
            bra         dot_f

; ex, a loop's count: fm_n its number.  Not a whole number: dotimes' error (given to the frame on top)
loop_count:
            lda         ex
            lsr
            bcc         @bad
            lda         ex
            ldx         ex + 1
            jsr         unfix
            sta         fm_n
            stx         fm_n + 1
            rts
@bad:
            pla                                             ; (Not back to the caller: the error)
            pla
            jsr         err_begin                           ; ("'dotimes': a count must be a whole number, not :a")
            LDAX        s_dotcount
            jsr         msg_out
            lda         ex
            ldx         ex + 1
            ldy         #0
            FARN        2, print_val
            jmp         ev_err_end

k_dotn:
            POPV        ee                                  ; (dotimes' count: then the body for each)
            POPV        lk_s
            POPV        eu
            jsr         loop_count
            stz         fm_m
            stz         fm_m + 1
; fm_m: i, fm_n: n, eu: the body, lk_s: the name, ee: the scope it's in
dot_item:
            lda         fm_m
            cmp         fm_n
            lda         fm_m + 1
            sbc         fm_n + 1
            bvc         :+
            eor         #$80
:
            bmi         :+
            stz         ex
            stz         ex + 1
            jmp         ev_return
:
            bit         intr
            bpl         :+
            jmp         ev_intr
:
            MOVW        et, eu                              ; (Its scope: i bound)
            MOVW        eu, ee
            lda         #1
            jsr         frame_new
            MOVW        ee, hv
            lda         fm_m
            ldx         fm_m + 1
            jsr         fix_ax
            sta         eu
            stx         eu + 1
            lda         lk_s
            ldx         lk_s + 1
            jsr         frame_add
            MOVW        eu, et
            MOVW        cr, eu                              ; (cr: the body expressions left)
dot_body:
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         dot_next
:
            jsr         cr_next
            jsr         frame_check
            lda         fm_m
            ldx         fm_m + 1
            jsr         fix_ax
            jsr         push
            lda         fm_n
            ldx         fm_n + 1
            jsr         fix_ax
            jsr         push
            PUSHV       cr
            PUSHV       eu
            PUSHV       lk_s
            PUSHV       ee
            PUSHK       K_DOTB
            jmp         ev_evalarg
k_dotb:
            POPV        ee
            POPV        lk_s
            POPV        eu
            POPV        cr
            jsr         pop
            jsr         unfix
            sta         fm_n
            stx         fm_n + 1
            jsr         pop
            jsr         unfix
            sta         fm_m
            stx         fm_m + 1
            jmp         dot_body
dot_next:
            lda         ee                                  ; (The next i, in the scope the loop's in)
            ldx         ee + 1
            ldy         #0
            jsr         cell_get
            sta         ee
            stx         ee + 1
            inc         fm_m
            bne         :+
            inc         fm_m + 1
:
            jmp         dot_item

; ---- <=>

; (<=> n case...): n a number; each case {test value} (the last may be {value}): the first whose test is true of
; n's sign (=, <>, >, <, >= or =>, <= or =<), its value evaluated; NIL if none
sf_space:
            lda         bi_count
            cmp         #2
            bcs         :+
            LDAX        s_spacefew
            jmp         sf_error
:
            lda         cr                                  ; (Its cases: Q-expressions, as written)
            ldx         cr + 1
            ldy         #1
            jsr         cell_get
            sta         et
            stx         et + 1
            sta         ht
            stx         ht + 1
@case:
            lda         ht
            ora         ht + 1
            beq         @ok
            lda         ht
            ldx         ht + 1
            jsr         cell_first
            jsr         q_conses
            bcc         :+
            LDAX        s_spacecase
            jmp         sf_error
:
            lda         ht
            ldx         ht + 1
            ldy         #1
            jsr         cell_get
            sta         ht
            stx         ht + 1
            bra         @case
@ok:
            jsr         cr_first
            jsr         frame_check
            PUSHV       et
            PUSHV       ee
            PUSHK       K_SPACE
            jmp         ev_eval
k_space:
            POPV        ee
            POPV        et
            lda         ex                                  ; (A number: its sign)
            lsr
            bcs         :+
            LDAX        s_spacenum
            jmp         sf_error
:
            lda         #0
            ldx         ex + 1
            bmi         @sign
            inc
            ldx         ex
            cpx         #<FIX(0)
            bne         :+
            ldx         ex + 1
            beq         @sign
:
            inc
@sign:
            sta         fm_n                                ; (fm_n: 0 below 0, 1 at it, 2 above)
@case:
            lda         et
            ora         et + 1
            bne         :+
            stz         ex                                  ; (None: NIL)
            stz         ex + 1
            jmp         ev_return
:
            lda         et                                  ; (The case's items: cr; et the cases left)
            ldx         et + 1
            jsr         deref
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            iny
            lda         (hp),y
            sta         et
            iny
            lda         (hp),y
            sta         et + 1
            pla
            jsr         q_conses
            sta         cr
            stx         cr + 1
            jsr         count_cr
            sta         fm_m
            cmp         #2
            bcs         @test
            lda         et                                  ; (One item: only the last may be so)
            ora         et + 1
            beq         @match
            LDAX        s_spacelast
            jmp         sf_error
@test:
            jsr         cr_first                            ; (Its test: a symbol)
            jsr         space_test
            bcc         @match
            bra         @case
@match:
            lda         cr                                  ; (Its last item: evaluated)
            ldx         cr + 1
@last:
            sta         ht
            stx         ht + 1
            ldy         #1
            jsr         cell_get
            cmp         #0
            bne         @last
            cpx         #0
            bne         @last
            lda         ht
            ldx         ht + 1
            jsr         cell_first
            sta         ex
            stx         ex + 1
            jmp         ev_eval

; ex, a <=> test (=, <>, >, <, >=, =>, <=, =<): C = 0 if it's true of fm_n's sign.  Another: the error "Unknown
; comparison test x" (given to the frame on top)
space_test:
            lda         ex
            lsr
            bcs         @unknown
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @unknown
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @unknown
            lda         ex
            ldx         ex + 1
            jsr         deref                               ; (Its name: up to 2 bytes, in fm_m)
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            lda         hn
            cmp         #3
            bcs         @unknown
            stz         fm_m + 1
            lda         (hp)
            sta         fm_m
            lda         hn
            cmp         #2
            bne         :+
            ldy         #1
            lda         (hp),y
            sta         fm_m + 1
:
            ldx         #SPACE_TESTS - 1
@test:
            lda         fm_m
            cmp         space_a,x
            bne         @next
            lda         fm_m + 1
            cmp         space_b,x
            beq         @found
@next:
            dex
            bpl         @test
@unknown:
            pla                                             ; (Not back to the caller: the error)
            pla
            jsr         err_begin
            LDAX        s_spacetest
            jsr         msg_out
            lda         ex
            ldx         ex + 1
            ldy         #1
            FARN        2, print_val
            jmp         ev_err_end
@found:
            lda         space_true,x                        ; (Its signs: bit 0 <, 1 =, 2 >)
            ldx         fm_n
            and         sign_bit,x
            beq         :+
            clc
            rts
:
            sec
            rts

; ---- output-of

; (output-of x...): what the expressions wrote (to stdout), a string; the first error stops them
sf_output:
            jsr         cap_begin
            sta         ou_start
            stx         ou_start + 1
output_next:
            lda         cr
            ora         cr + 1
            bne         @more
            lda         ou_start                            ; (Its end: the string)
            ldx         ou_start + 1
            jsr         cap_end
            bcc         :+
            jmp         ev_nomem
:
            MOVW        ex, hv
            jmp         ev_return
@more:
            jsr         cr_next
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            lda         ou_start
            ldx         ou_start + 1
            jsr         fix_ax
            jsr         push
            PUSHK       K_OUTPUT
            jmp         ev_eval
k_output:
            jsr         pop
            jsr         unfix
            sta         ou_start
            stx         ou_start + 1
            POPV        ee
            POPV        cr
            jsr         is_err
            bcc         output_next
            lda         ou_start                            ; (An error: the capture given up, the error on)
            ldx         ou_start + 1
            jsr         cap_drop
            jmp         ev_return

; ---- defined?, expr?

; (defined? x): T if x (as written) is T, or a name bound where it's evaluated (&1, &_ ...: in this call's
; scopes, up to its function's own), or a list of such names; else NIL
sf_defined:
            stz         ex
            stz         ex + 1
            lda         bi_count
            bne         :+
            jmp         ev_return
:
            jsr         cr_first
            jsr         defined_one
            bcs         @yes
            lda         ex                                  ; (A list: each of its items)
            ldx         ex + 1
            jsr         q_conses
            bcs         @no
            sta         cr
            stx         cr + 1
@item:
            lda         cr
            ora         cr + 1
            beq         @yes
            jsr         cr_next
            jsr         defined_one
            bcs         @item
@no:
            stz         ex
            stz         ex + 1
            jmp         ev_return
@yes:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         ev_return

; ex: T, or a symbol bound here?  OUT: C = 1 if it's so
defined_one:
            lda         ex
            cmp         #<T_VAL
            bne         :+
            lda         ex + 1
            beq         @yes
:
            lda         ex
            lsr
            bcs         @no
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @no
            lda         ex                                  ; (&N, &_ ...: this call's own)
            ldx         ex + 1
            jsr         deref
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            lda         (hp)
            cmp         #'&'
            beq         @amp
            jsr         find_bound
            bcc         @yes
            lda         ex                                  ; (Its global value)
            ldx         ex + 1
            ldy         #1
            jsr         cell_get
            cmp         #<UNBOUND
            bne         @yes
            cpx         #>UNBOUND
            bne         @yes
@no:
            clc
            rts
@yes:
            sec
            rts
@amp:
            MOVW        lk_f, ee                            ; (Each scope, up to the one that binds &_)
@frame:
            lda         lk_f
            ora         lk_f + 1
            beq         @no
            lda         ex
            ldx         ex + 1
            jsr         frame_binds
            bcs         @yes
            lda         amp_syms
            ldx         amp_syms + 1
            jsr         frame_binds
            bcs         @no
            lda         lk_f
            ldx         lk_f + 1
            ldy         #0
            jsr         cell_get
            sta         lk_f
            stx         lk_f + 1
            bra         @frame

; IN: .A, .X = a symbol; lk_f = a frame.  OUT: C = 1 if the frame (its cells) binds it
frame_binds:
            sta         lk_s
            stx         lk_s + 1
            MOVW        lk_c, lk_f
@cell:
            lda         lk_c
            ldx         lk_c + 1
            jsr         deref
            lda         pk,x
            tax
            lda         kd_size,x
            sta         lk_end
            ldy         #4
@pair:
            cpy         lk_end
            bcs         @next
            lda         (hp),y
            iny
            ora         (hp),y
            beq         @no
            dey
            lda         (hp),y
            cmp         lk_s
            bne         @skip
            iny
            lda         (hp),y
            cmp         lk_s + 1
            beq         @yes
            dey
@skip:
            iny
            iny
            iny
            iny
            bra         @pair
@next:
            ldy         #2
            lda         (hp),y
            sta         lk_c
            iny
            lda         (hp),y
            sta         lk_c + 1
            ora         lk_c
            bne         @cell
@no:
            clc
            rts
@yes:
            sec
            rts

; (expr? x): T if x, as written, is an expression (a string, a character, a number, T, a symbol, an atom, a list);
; else NIL
sf_expr:
            lda         bi_count
            beq         @no
            jsr         cr_first
            lda         ex
            lsr
            bcs         @yes
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #0
            bne         @yes                                ; (A character, a built-in)
            lda         ex
            cmp         #<EXIT_VAL
            beq         @no
            cmp         #<UNBOUND
            beq         @no
@yes:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         ev_return
@cell:
            lda         pk,x
            cmp         #PK_ERROR
            beq         @no
            cmp         #PK_HASH
            beq         @no
            cmp         #PK_BUFFER
            beq         @no
            cmp         #PK_STREAM
            bne         @yes
@no:
            stz         ex
            stz         ex + 1
            jmp         ev_return

; ---- map, the folds, sort, load: built-ins that run the machine (their values on the stack, at argp)

; map, filter, foldl, foldr, any?, all?, find, count, sum and product: a list walked, its function given each item in
; turn (foldl's the value so far and the item; foldr's the item and the value so far, from the last item), as
; danlang's (LibraryBuiltins.cs); the first error stops it.  Its frame, on the stack as it walks (its code pushed for
; each call): the function, the items left, the value so far (map's and filter's list: its first cons, and its last),
; the item given, the walk (W_*, a fixnum)
W_MAP           = 0             ; The values, a list
W_FILTER        = 1             ; The items whose value isn't NIL, a list
W_FOLDL         = 2             ; The last value (sum's and product's too: + from 0, * from 1)
W_FOLDR         = 3
W_ANY           = 4             ; T at the first value that isn't NIL; else NIL
W_ALL           = 5             ; NIL at the first value that's NIL; else T
W_FIND          = 6             ; The item whose value first isn't NIL; else NIL
W_COUNT         = 7             ; The values that aren't NIL, counted
W_SUM           = 8
W_PRODUCT       = 9
WK_F            = 0             ; (The frame's words: their places)
WK_ITEMS        = 2
WK_ACC          = 4
WK_LAST         = 6
WK_ITEM         = 8
WK_OP           = 10
WK_SIZE         = 12

bi_map:
            lda         #W_MAP
            bra         walk
bi_filter:
            lda         #W_FILTER
            bra         walk
bi_foldl:
            lda         #W_FOLDL
            bra         walk
bi_foldr:
            lda         #W_FOLDR
            bra         walk
bi_any:
            lda         #W_ANY
            bra         walk
bi_all:
            lda         #W_ALL
            bra         walk
bi_find:
            lda         #W_FIND
            bra         walk
bi_count_if:
            lda         #W_COUNT
            bra         walk
bi_sum:
            lda         #W_SUM
            bra         walk
bi_product:
            lda         #W_PRODUCT
walk:
            sta         wk_op
            tax
            ldy         wk_list,x                           ; (Its list: its last argument)
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            jsr         q_conses
            bcc         :+
            jsr         err_begin                           ; ("'map' expects a function and a list")
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            ldx         wk_op
            lda         wk_what_lo,x
            pha
            lda         wk_what_hi,x
            tax
            pla
            jsr         msg_out
            jmp         ev_err_end_args
:
            sta         cr
            stx         cr + 1
            stz         et                                  ; (The value so far: none)
            stz         et + 1
            ldx         wk_op
            cpx         #W_SUM
            bcc         @fn
            lda         #<FIX(0)                            ; (sum: + from 0; product: * from 1)
            ldy         #<(BUILTIN0 + 2 * BIN_ADD)
            cpx         #W_SUM
            beq         :+
            lda         #<FIX(1)
            ldy         #<(BUILTIN0 + 2 * BIN_MUL)
:
            sta         et
            sty         cf
            lda         #>(BUILTIN0 + 2 * BIN_ADD)
            sta         cf + 1
            lda         #W_FOLDL
            sta         wk_op
            bra         @frame
@fn:
            lda         (argp)                              ; (Its function)
            sta         cf
            ldy         #1
            lda         (argp),y
            sta         cf + 1
            cpx         #W_COUNT
            bne         :+
            lda         #<FIX(0)
            sta         et
            bra         @frame
:
            cpx         #W_FOLDL
            beq         @from
            cpx         #W_FOLDR
            bne         @frame
            jsr         wk_reverse
@from:
            ldy         #2                                  ; (The folds': from their value)
            lda         (argp),y
            sta         et
            iny
            lda         (argp),y
            sta         et + 1
@frame:
            jsr         frame_check
            stz         ev_inargs
            jsr         drop_args
            PUSHV       cf
            PUSHV       cr
            PUSHV       et
            lda         #0
            tax
            jsr         push
            jsr         push
            lda         wk_op
            asl
            ora         #1
            jsr         push
; The walk's frame on top (its code not): the next item given to its function, or none left, its value
walk_next:
            jsr         wk_at
            ldy         #WK_ITEMS
            lda         (hq),y
            sta         cr
            iny
            lda         (hq),y
            sta         cr + 1
            ora         cr
            beq         walk_end
            jsr         cr_next                             ; (ex: the item)
            ldy         #WK_ITEMS
            lda         cr
            sta         (hq),y
            iny
            lda         cr + 1
            sta         (hq),y
            ldy         #WK_ITEM
            lda         ex
            sta         (hq),y
            iny
            lda         ex + 1
            sta         (hq),y
            ldy         #WK_F
            lda         (hq),y
            sta         cf
            iny
            lda         (hq),y
            sta         cf + 1
            ldy         #WK_ACC
            lda         (hq),y
            sta         et
            iny
            lda         (hq),y
            sta         et + 1
            ldy         #WK_OP
            lda         (hq),y
            lsr
            sta         wk_op
            PUSHK       K_WALK
            lda         wk_op
            cmp         #W_FOLDL
            beq         @acc_item
            cmp         #W_FOLDR
            beq         @item_acc
            PUSHV       ex
            lda         #1
            bra         @apply
@acc_item:
            PUSHV       et
            PUSHV       ex
            bra         @two
@item_acc:
            PUSHV       ex
            PUSHV       et
@two:
            lda         #2
@apply:
            sta         cn
            stz         cn + 1
            jmp         ev_apply_values
; No items left: the walk's value (hq, its frame)
walk_end:
            ldy         #WK_OP
            lda         (hq),y
            lsr
            tax
            ldy         #WK_ACC                             ; (map's, filter's, the folds', count's: the value so far)
            lda         (hq),y
            sta         ex
            iny
            lda         (hq),y
            sta         ex + 1
            cpx         #W_ANY
            bcc         walk_done
            cpx         #W_COUNT
            beq         walk_done
            stz         ex                                  ; (any?'s and find's: NIL; all?'s: T)
            stz         ex + 1
            cpx         #W_ALL
            bne         walk_done
            lda         #<T_VAL
            sta         ex
; The walk's value, ex: its frame dropped
walk_done:
            lda         #WK_SIZE / 2
            jsr         drop
            jmp         ev_return

; The function's value, ex, for the walk on top
k_walk:
            ldy         #0                                  ; (.Y = 1 if it isn't NIL)
            lda         ex
            ora         ex + 1
            beq         :+
            iny
:
            jsr         wk_at
            phy
            ldy         #WK_OP
            lda         (hq),y
            lsr
            tax
            ply
            cpx         #W_FOLDL
            bcs         @fold
            cpx         #W_FILTER
            bne         @add                                ; (map's: the value put in its list)
            tya                                             ; (filter's: the item, if the value isn't NIL)
            bne         @lb103
            jmp         walk_next
@lb103:
            ldy         #WK_ITEM
            lda         (hq),y
            sta         ex
            iny
            lda         (hq),y
            sta         ex + 1
@add:
            MOVW        et, ex                              ; (In a cons, at the list's end)
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         make2
            bcc         :+
            lda         #WK_SIZE / 2
            jsr         pop_n
            jmp         ev_nomem
:
            MOVW        ex, hv
            jsr         wk_at
            ldy         #WK_LAST
            lda         (hq),y
            sta         hv
            iny
            lda         (hq),y
            sta         hv + 1
            ora         hv
            bne         @link
            ldy         #WK_ACC                             ; (The first)
            lda         ex
            sta         (hq),y
            iny
            lda         ex + 1
            sta         (hq),y
            bra         @last
@link:
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         cell_set
@last:
            ldy         #WK_LAST
            lda         ex
            sta         (hq),y
            iny
            lda         ex + 1
            sta         (hq),y
            jmp         walk_next
@fold:
            cpx         #W_ANY
            bcs         @test
            ldy         #WK_ACC                             ; (The folds': the value so far)
            lda         ex
            sta         (hq),y
            iny
            lda         ex + 1
            sta         (hq),y
            jmp         walk_next
@test:
            cpx         #W_ALL
            bne         :+
            tya                                             ; (all?: NIL, at the first NIL)
            beq         @lb102
            jmp         walk_next
@lb102:
            jmp         walk_done
:
            tya
            bne         @lb101
            jmp         walk_next
@lb101:
            cpx         #W_COUNT
            bne         :+
            ldy         #WK_ACC                             ; (count: one more)
            lda         (hq),y
            clc
            adc         #2
            sta         (hq),y
            iny
            lda         (hq),y
            adc         #0
            sta         (hq),y
            jmp         walk_next
:
            cpx         #W_FIND
            bne         :+
            ldy         #WK_ITEM                            ; (find: the item)
            lda         (hq),y
            sta         ex
            iny
            lda         (hq),y
            sta         ex + 1
            jmp         walk_done
:
            lda         #<T_VAL                             ; (any?: T)
            sta         ex
            stz         ex + 1
            jmp         walk_done

; hq = the walk's frame (on top, its code not)
wk_at:
            sec
            lda         sp
            sbc         #WK_SIZE
            sta         hq
            lda         sp + 1
            sbc         #0
            sta         hq + 1
            rts

; cr, a list's conses: its items the other way round (foldr's), in new conses.  The arguments are on the stack
wk_reverse:
            stz         eu
            stz         eu + 1
@item:
            lda         cr
            ora         cr + 1
            beq         @done
            jsr         cr_next
            MOVW        et, ex
            lda         #PK_QCONS
            jsr         make2
            bcc         :+
            jmp         ev_nomem
:
            MOVW        eu, hv
            bra         @item
@done:
            MOVW        cr, eu
            rts

; (sort l [less]): the items in order, stable: by cmp, or by less, a function of two that's T when its first goes
; first.  A merge sort, its runs merged a pair at a time (lists.inc's sort_begin and sort_step: its frame, K_SORT's,
; with its code on top as it goes); less called here, given b's first item and a's: T, b's goes first
bi_sort:
            jsr         frame_check
            FARN        2, sort_begin
            bcc         :+
            jmp         ev_nomem
:
            beq         sort_value
            PUSHK       K_SORT
            lda         #$FF
sort_on:
            FARN        2, sort_step
            beq         @done
            PUSHV       q_a                                 ; (less wanted: cf)
            PUSHV       q_b
            lda         #2
            sta         cn
            stz         cn + 1
            jmp         ev_apply_values
@done:
            lda         depth                               ; (Its frame dropped)
            bne         :+
            dec         depth + 1
:
            dec         depth
sort_value:
            jmp         ev_return
; less's value, ex: 1 (b's first goes first) if it isn't NIL, else 0
k_sort:
            PUSHK       K_SORT
            lda         ex
            ora         ex + 1
            beq         :+
            lda         #1
:
            bra         sort_on

; (hash-create [entries]), (to# entries), (hash-put h entry...): each entry's value evaluated in the caller's scope,
; then put (hashes.inc's h_begin and h_step: a frame K_HASH while one is evaluated)
bi_hash_create:
            lda         #HM_CREATE
            bra         hash_begin
bi_to_hash:
            lda         #HM_TO
            bra         hash_begin
bi_hash_put:
            lda         #HM_PUT
hash_begin:
            sta         h_mode
            jsr         frame_check
            FARN        4, h_begin
            bcc         :+
            jmp         ev_nomem
:
            beq         hash_value
            PUSHV       ee                                  ; (Its frame: [ee hash items entry state first last])
            PUSHV       et
            PUSHV       cr
            lda         #0
            tax
            jsr         push
            lda         h_st
            ldx         #0
            jsr         fix_ax
            jsr         push
            lda         #0
            tax
            jsr         push
            jsr         push
            PUSHK       K_HASH
            lda         #$FF
hash_on:
            FARN        4, h_step
            bcc         :+
            lda         #HF_SIZE / 2 + 1                    ; (No room: its frame dropped, then the error)
            jsr         drop
            jsr         hash_done
            jmp         ev_nomem
:
            bne         hash_eval
            jsr         hash_done
hash_value:
            jmp         ev_return
hash_eval:
            jmp         ev_eval
; Its frame dropped (h_step's)
hash_done:
            lda         depth
            bne         :+
            dec         depth + 1
:
            dec         depth
            rts
; A value evaluated, ex
k_hash:
            PUSHK       K_HASH
            lda         #0
            bra         hash_on

; (hash-call h key x...): the method at key applied to the values after it (&0 the hash), as (h key x...) is
bi_hash_call:
            FARN        4, h_call
            bcc         :+
            jmp         ev_nomem
:
            bne         :+
            stz         ev_inargs
            jsr         drop_args
            jmp         ev_return
:
            jmp         ev_apply_values

; (load path...): each file's expressions evaluated in turn, in the global scope; the last's value.  Its error is
; "path: message" (a file not there: the system's error, :noent), a reader's "path:LINE: message" (the line where
; it stopped; a bracket left open, the innermost one's).  Read an item at a time (read_item),
; the reader's text refilled from the file; a load while it waits for a value (one nested) uses the text too, so when
; the value comes, if the text isn't its own any more (rd_owner), it seeks the file back to its next byte (its
; frame's) and reads on from there
bi_load:
            ldy         #0                                  ; (Its paths: strings)
@path:
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            iny
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_loadstr
            jmp         ev_error_args
:
            tya
            lsr
            cmp         cn
            bcc         @path
            jsr         args_list                           ; (cf: the first; cr: the rest)
            lda         hv
            ldx         hv + 1
            ldy         #1
            jsr         cell_get
            sta         cr
            stx         cr + 1
            lda         hv
            ldx         hv + 1
            ldy         #0
            jsr         cell_get
            sta         cf
            stx         cf + 1
            stz         ev_inargs
            jsr         drop_args
            stz         ex
            stz         ex + 1
; cf, a path: its file opened (load's next), cr the paths after it
load_open:
            FARN        4, file_open                        ; (The file opened: its fd)
            bcc         :+
            lda         ld_err                              ; ("path: not found", :noent)
            FARN        5, y_loaderr
            bcs         @lb402
            jmp         load_value
@lb402:
            jmp         ev_nomem
:
            sta         ld_fdv
            stz         ld_off
            stz         ld_off + 1
            lda         #1                                  ; (Its first line)
            sta         rd_line
            stz         rd_line + 1
            jsr         ld_text
            stz         ex
            stz         ex + 1
; cf: the path, ld_fdv: its fd (the reader's source); ex the last value: the next item read, and evaluated
load_next:
            lda         ld_fdv
            sta         rd_fd
            FARN        2, read_item
            bcc         :+
            jsr         ld_close
            jmp         ev_nomem
:
            cmp         #RD_DONE
            beq         @item
            pha
            jsr         ld_close
            pla
            cmp         #RD_END
            bne         :+
            lda         cr                                  ; (Its end: the next file's, or the last value)
            ora         cr + 1
            bne         @lb401
            jmp         load_value
@lb401:
            lda         cr
            ldx         cr + 1
            ldy         #0
            jsr         cell_get
            sta         cf
            stx         cf + 1
            lda         cr
            ldx         cr + 1
            ldy         #1
            jsr         cell_get
            sta         cr
            stx         cr + 1
            jmp         load_open
:
            cmp         #RD_ERROR
            bne         :+
            MOVW        ex, hv
            lda         rd_line                             ; ("path:LINE: message": where it stopped)
            ldx         rd_line + 1
            jmp         load_rderr
:
            jsr         err_begin                           ; ("path:LINE: missing )": the innermost bracket's)
            ldx         rd
            lda         lv_line_hi,x
            pha
            lda         lv_line_lo,x
            plx
            jsr         out_pathline
            LDAX        s_missing
            jsr         msg_out
            ldx         #0
:
            cpx         cl_len
            bcs         :+
            lda         closers,x
            jsr         out_byte
            inx
            bra         :-
:
            jmp         ev_err_end
@item:
            lda         re                                  ; (Its next byte: the bytes read, less those left)
            sec
            sbc         rp
            sta         ld_off
            lda         re + 1
            sbc         rp + 1
            sta         ld_off + 1
            lda         rd_total
            sec
            sbc         ld_off
            sta         ld_off
            lda         rd_total + 1
            sbc         ld_off + 1
            sta         ld_off + 1
            jsr         frame_check
            PUSHV       cr
            PUSHV       cf
            lda         ld_fdv
            ldx         #0
            jsr         fix_ax
            jsr         push
            lda         ld_off
            ldx         #0
            jsr         fix_ax
            jsr         push
            lda         ld_off + 1
            ldx         #0
            jsr         fix_ax
            jsr         push
            lda         rd_line
            ldx         #0
            jsr         fix_ax
            jsr         push
            lda         rd_line + 1
            ldx         #0
            jsr         fix_ax
            jsr         push
            PUSHK       K_LOAD
            MOVW        ex, hv                              ; (The item, in the global scope)
            stz         ee
            stz         ee + 1
            jmp         ev_eval
k_load:
            jsr         pop                                 ; (Its frame: its line, its next byte, its fd, its path)
            jsr         unfix
            sta         ld_line + 1
            jsr         pop
            jsr         unfix
            sta         ld_line
            jsr         pop
            jsr         unfix
            sta         ld_off + 1
            jsr         pop
            jsr         unfix
            sta         ld_off
            jsr         pop
            jsr         unfix
            sta         ld_fdv
            POPV        cf
            POPV        cr
            jsr         is_err
            bcc         :+
            jsr         ld_close
            jmp         load_error
:
            lda         ex                                  ; (exit: the end)
            cmp         #<EXIT_VAL
            bne         @on
            lda         ex + 1
            bne         @on
            jsr         ld_close
            jsr         out_flush
            LDR         r0, 0
            lda         #0
            jmp         EXITS
@on:
            lda         vm_full                             ; (The machine's arena full, and this load the line's own
            beq         :+                                  ;   (a script's: nothing under it, no frame of the
            lda         depth + 1                           ;   machine's): emptied, as at a line's start, its code
            bne         :+                                  ;   made again)
            lda         depth
            cmp         #3
            bcs         :+
            FARN        7, vm_reset
:
            lda         rd_owner                            ; (The text its own still: on)
            cmp         ld_fdv
            bne         :+
            MOVW        rd_line, ld_line
            jmp         load_next
:
            lda         ld_off                              ; (Else the file back to its next byte)
            sta         r0
            lda         ld_off + 1
            sta         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0
            lda         ld_fdv
            jsr         SEEK
            jsr         ld_text
            MOVW        rd_line, ld_line                    ; (Its line, as it was)
            jmp         load_next
; The last value
load_value:
            jmp         ev_return

; The reader's text empty, the file ld_fdv's from its byte ld_off on
ld_text:
            lda         #<text
            sta         rp
            sta         re
            lda         #>text
            sta         rp + 1
            sta         re + 1
            lda         #0
            sta         text
            stz         rd_eof
            MOVW        rd_total, ld_off
            lda         ld_fdv
            sta         rd_owner
            rts

; The file ld_fdv closed (the text no one's)
ld_close:
            lda         ld_fdv
            jsr         CLOSE
            lda         #$FF
            sta         rd_owner
            rts

; ex, an error in the file cf: "path: message", given to the frame on top
load_error:
            jsr         err_begin
            jsr         out_path
            lda         ex                                  ; (Its message)
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            ldy         #1
            FARN        2, print_val
            jmp         ev_err_end

; ex, a reader's error in the file cf, on line .A, .X: "path:LINE: message", given to the frame on top
load_rderr:
            pha
            phx
            jsr         err_begin
            plx
            pla
            jsr         out_pathline
            lda         ex                                  ; (Its message)
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            ldy         #1
            FARN        2, print_val
            jmp         ev_err_end

; cf, a path (a string), out, a colon, the line .A, .X, and ": "
out_pathline:
            pha
            phx
            lda         cf
            ldx         cf + 1
            ldy         #1
            FARN        2, print_val
            lda         #':'
            jsr         out_byte
            plx
            pla
            jsr         out_dec
            LDAX        s_colon
            jmp         msg_out

; cf, a path (a string), out, and ": "
out_path:
            lda         cf
            ldx         cf + 1
            ldy         #1
            FARN        2, print_val
            LDAX        s_colon
            jmp         msg_out

; The system's error .A's text out
out_errstr:
            sta         ld_err
            LDR         r0, ld_errbuf
            lda         ld_err
            jsr         ERRSTR
            LDAX        ld_errbuf
            jmp         out_text

.bss
ld_err:     .res        1                                   ; (load's: the system's error, its text; the file's fd,
ld_errbuf:  .res        32                                  ;   the offset of its next byte)
ld_fdv:     .res        1
ld_off:     .res        2
ld_line:    .res        2                                   ; (load's frame's: the line it's on)
wk_op:      .res        1                                   ; (A walk: map, filter ... W_*)
fo_at:      .res        1                                   ; (file_open's: its name's place in scratch)
fo_fd:      .res        1                                   ; (fo_open's: the file opened)

.code

.segment "CODE4"

; cf, a path: its file opened to read: as it is, or with .hl; then, a bare name (no /), /lib/hylang's (as danlang's
; FindFile, lib/'s).  OUT: C = 0, .A = its fd; or C = 1 and ld_err = the system's error (the first's)
LIB_LEN         = 12            ; (/lib/hylang/)
file_open:
            lda         cf                                  ; (Its name: in scratch, after room for /lib/hylang/)
            ldx         cf + 1
            jsr         deref
            lda         (hp)
            sta         fm_n
            ldy         #1
            lda         (hp),y
            bne         @long
            lda         fm_n
            cmp         #PATH_MAX + 1
            bcc         :+
@long:
            lda         #E_NAMETOOLONG
            sta         ld_err
            sec
            rts
:
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldy         #0
:
            cpy         fm_n
            beq         :+
            lda         (hp),y
            sta         scratch + LIB_LEN,y
            iny
            bne         :-
:
            ldx         #LIB_LEN
            jsr         fo_name
            bcc         @rts
            sta         ld_err
            ldy         fm_n                                ; (A bare name?)
@slash:
            dey
            cpy         #$FF
            beq         @bare
            lda         scratch + LIB_LEN,y
            cmp         #'/'
            bne         @slash
            sec
@rts:
            rts
@bare:
            ldx         #LIB_LEN - 1
:
            lda         s_libdir,x
            sta         scratch,x
            dex
            bpl         :-
            ldx         #0
            jsr         fo_name
            rts

; The name at scratch + .X (to scratch + LIB_LEN + fm_n) opened; or, not found, with .hl after it (unless it ends
; with .hl).  OUT: C = 0, .A = the fd; or C = 1, .A = the error (the first's)
fo_name:
            stx         fo_at
            lda         fm_n
            clc
            adc         #LIB_LEN
            tay
            lda         #0
            sta         scratch,y
            jsr         fo_open
            bcc         @rts
            cmp         #E_NOENT
            bne         @fail
            sta         fm_m
            ldx         #2                                  ; (Ends with .hl?)
:
            dey
            lda         scratch,y
            cmp         s_hl,x
            bne         @hl
            dex
            bpl         :-
            bra         @nf
@hl:
            lda         fm_n
            clc
            adc         #LIB_LEN
            tay
            ldx         #0
:
            lda         s_hl,x
            sta         scratch,y
            iny
            inx
            cpx         #3
            bcc         :-
            lda         #0
            sta         scratch,y
            jsr         fo_open
            bcc         @rts
@nf:
            lda         fm_m
@fail:
            sec
@rts:
            rts

; scratch + fo_at opened to read.  OUT: as OPEN's; a directory is none (E_NOENT, closed): a bare name at / isn't
; /dev, /proc or /pc, but the library of its name
fo_open:
            lda         fo_at
            clc
            adc         #<scratch
            sta         r0
            lda         #>scratch
            adc         #0
            sta         r0 + 1
            lda         #O_READ
            jsr         OPEN
            bcs         @rts
            sta         fo_fd
            LDR         r0, y_buf                           ; (A directory?)
            lda         fo_fd
            jsr         FSTAT
            bcs         @file
            lda         y_buf + SR_MODE + 1
            and         #DM_DIR
            beq         @file
            lda         fo_fd
            jsr         CLOSE
            lda         #E_NOENT
            sec
            rts
@file:
            lda         fo_fd
            clc
@rts:
            rts

.code


.rodata
s_varfew:   .byte       0
form_lo:    .lobytes    fn_def, fn_set, fn_setb
form_hi:    .hibytes    fn_def, fn_set, fn_setb

; <=>'s tests: their names (a byte or two), and the signs each is true of (bit 0 <, 1 =, 2 >)
SPACE_TESTS     = 8
space_a:    .byte       '=', '<', '>', '<', '>', '=', '<', '='
space_b:    .byte       0, '>', 0, 0, '=', '>', '=', '<'
space_true: .byte       %010, %101, %100, %001, %110, %110, %011, %011
sign_bit:   .byte       %001, %010, %100                    ; (Indexed by fm_n: 0 <, 1 =, 2 >)

; The walks' lists: the argument (its place) and what its error says it wants (W_* order)
wk_list:    .byte       2, 2, 4, 4, 2, 2, 2, 2, 0, 0
wk_what_lo: .lobytes    s_wfnlist, s_wfnlist, s_wfold, s_wfold, s_wfnlist, s_wfnlist, s_wfnlist, s_wfnlist, s_wlist, s_wlist
wk_what_hi: .hibytes    s_wfnlist, s_wfnlist, s_wfold, s_wfold, s_wfnlist, s_wfnlist, s_wfnlist, s_wfnlist, s_wlist, s_wlist

.segment "RODATA4"                                          ; (Messages: read through msg_out, msg_err)
s_iffew:    .byte       "'if' supplied too few parameters", 0
s_evaln:    .byte       "Incorrect number of parameters passed to 'eval'", 0
s_toofew:   .byte       "Too few parameters passed to ", 0
s_nonsym:   .byte       " cannot define non-symbols", 0
s_toomanyvars: .byte    " passed too many arguments or symbols.  Expected ", 0
s_got:      .byte       ", got ", 0
s_letfew:   .byte       "'let' supplied too few parameters", 0
s_letlist:  .byte       "'let' expects a list of bindings first", 0
s_letbind:  .byte       "A 'let' binding is {name value}", 0
s_tryargs:  .byte       "'try' expects an expression and maybe a handler", 0
s_whilefew: .byte       "'while' supplied too few parameters", 0
s_eachfew:  .byte       "'each' supplied too few parameters", 0
s_eachfn:   .byte       "'each' expects a function and a list", 0
s_eachspec: .byte       "'each' expects {name value} or a function first", 0
s_eachfirst: .byte      "First 'each' parameter must be a function or {name list}", 0
s_eachlist: .byte       "'each' expects a list or a String", 0
s_dotfew:   .byte       "'dotimes' supplied too few parameters", 0
s_dotfn:    .byte       "'dotimes' expects a Number and a function", 0
s_dotspec:  .byte       "'dotimes' expects {name value} or a function first", 0
s_dotsecond: .byte      "Second 'dotimes' parameter must be a function", 0
s_dotcount: .byte       "'dotimes': a count must be a whole number, not ", 0
s_spacefew: .byte       "Too few parameters passed to '<=>' operator", 0
s_spacecase: .byte      "Operator '<=>' received one or more invalid case blocks (not QExpr)", 0
s_spacenum: .byte       "First parameter to '<=>' must evaluate to a Number", 0
s_spacelast: .byte      "All but last body block of a '<=>' operator must have comparator", 0
s_spacetest: .byte      "Unknown comparison test ", 0
s_wfnlist:  .byte       "' expects a function and a list", 0
s_wfold:    .byte       "' expects a function, a value and a list", 0
s_wlist:    .byte       "' expects a list", 0
s_loadstr:  .byte       "'load' passed non-string parameter(s)", 0
s_missing:  .byte       "missing ", 0
s_colon:    .byte       ": ", 0
s_libdir:   .byte       "/lib/hylang/"                      ; (load's library: file_open's, LIB_LEN bytes)
s_hl:       .byte       ".hl"
fn_def:     .byte       "def", 0
fn_set:     .byte       "set", 0
fn_setb:    .byte       "set!", 0

.code
