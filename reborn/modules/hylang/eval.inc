; ****************************************************************************
; eval.inc - hylang's evaluator (phase 3), in its first bank: an expression's value, as danlang's (reference.md,
; section 3).  A machine: ex the expression (or its value), ee the scope it's in, and the evaluation stack
; (stack.inc), whose frames say what's to be done with a value when it comes.  ev_eval evaluates ex in ee; ev_return
; gives ex to the frame on top.  The 65C02's stack never holds the machine's state, so calls nest as deep as the
; frames go (MAX_DEPTH), and a call in tail position (a function's body, if's branches, do's last, let's, eval's)
; takes its caller's place: it pushes nothing, and a tail loop runs in constant space.
;   A frame is its words, then its code (K_*, a fixnum) on top; every word on the stack is a value or a fixnum, so
; the collector marks it all.  A call's arguments go on the stack as they're evaluated (cn of them, ev_inargs set);
; one that isn't a call (a name, a constant, a Q-expression) is evaluated where it's met, with no frame.  An error
; given to a frame that doesn't take one drops the frame and goes on down (try's, a call's whose built-in takes
; errors, the end's stop it).
;   A scope is a frame cell (PK_FRAME*): its parent, the cell its pairs go on in, then name-value pairs.  A symbol
; never bound in one (SF_LOCAL clear) is looked up at its global value at once.  A Q-expression evaluated remembers
; its scope (a PK_SCOPED cell), where eval runs it; so does an fexpr's argument.

MAX_DEPTH       = 2500          ; Frames at most (danlang's calls nested: 10,000)
EV_ROOTS        = 12            ; (The registers ex to eu: the collector marks them)

.bss
ev_s:       .res        1                                   ; The 65C02's stack as ev_run began (an abort's)
ev_inargs:  .res        1                                   ; <> 0: a call's cn arguments on top of the stack
depth:      .res        2                                   ; Frames on the stack
ev_jv:      .res        2                                   ; (A jump's address: a frame's, a built-in's)
bi_n:       .res        1                                   ; (A built-in called: its number, its arguments counted,
bi_count:   .res        1                                   ;   to 255)
bf_t:       .res        1                                   ; (bi_fast's: its first's high byte, the second less its
bf_s:       .res        1                                   ;   tag bit)
lk_f:       .res        2                                   ; (lookup's and frame_add's: the frame, the cell, its end,
lk_c:       .res        2                                   ;   the symbol)
lk_end:     .res        1
lk_s:       .res        2
fa_k:       .res        1                                   ; (A function applied: its formals counted, its flags,
fa_flags:   .res        1                                   ;   its formals left, the argument at)
fa_f:       .res        2
fa_i:       .res        2
eb_start:   .res        2                                   ; (An error's message: where its capture began)
amp_name:   .res        8                                   ; (A name &N being made)

PSTATE_SEG
amp_syms:   .res        2 * 10                              ; The symbols &_, &1 ... &9
sym_err:    .res        2                                   ; &err, &code
sym_code:   .res        2
atom_intr:  .res        2                                   ; :intr

.bss

; The frames' codes: k_tab has what a value given to each does; k_info, whether it takes an error (else one passes
; it by), and its words but the code ($FF: K_ARG's, 4 and its cn arguments)
K_TOP           = 0             ; []: the machine's end, back to ev_run's caller
K_HEAD          = 1             ; [cr ee]: a call's function evaluated
K_ARG           = 2             ; [arguments... cf cr ee cn]: an argument evaluated
K_IF            = 3             ; [cr ee]: if's test evaluated (cr: then and else)
K_DO            = 4             ; [cr ee]: do's expression evaluated (cr: the rest)
K_AND           = 5             ; [cr ee]
K_OR            = 6             ; [cr ee]
K_EVAL          = 7             ; [ee]: eval's argument evaluated
K_EXIT          = 8             ; []: exit's status evaluated
K_DEFN          = 9             ; [cr ee kind]: def's names evaluated (written as an S-expression)
K_DEFV          = 10            ; [names cr ee kind]: def's value evaluated
K_LETB          = 11            ; [bindings body ee name]: a let binding's value evaluated (ee: the let's scope)
K_LETX          = 12            ; [body ee]: a let body expression evaluated (not the last)
K_TRY           = 13            ; [handler ee]: try's expression evaluated
K_TRYH          = 14            ; [message]: try's handler evaluated
K_WTEST         = 15            ; [args result ee]: while's test evaluated
K_WBODY         = 16            ; [args rest ee]: a while body expression evaluated
K_EACHF         = 17            ; [f items]: each's function given an item
K_EACHB         = 18            ; [items rest body name ee]: an each body expression evaluated (ee: the item's)
K_EACHL         = 19            ; [body name ee]: each's list evaluated
K_DOTF          = 20            ; [f i n]: dotimes' function given a number
K_DOTB          = 21            ; [i n rest body name ee]: a dotimes body expression evaluated
K_DOTN          = 22            ; [body name ee]: dotimes' count evaluated
K_SPACE         = 23            ; [cases ee]: <=>'s number evaluated
K_OUTPUT        = 24            ; [cr ee start]: an output-of expression evaluated
K_WALK          = 25            ; [f items acc last item walk]: map's (filter's, a fold's ...) function given an item
K_LOAD          = 26            ; [rest path fd next]: a loaded file's expression evaluated (next: two fixnums)
K_EACHF2        = 27            ; [cr ee]: each's function evaluated (its list next)
K_EACHF3        = 28            ; [f]: each's list evaluated (its function first)
K_DOTF2         = 29            ; [cr ee]: dotimes' count evaluated (its function next)
K_DOTF3         = 30            ; [n]: dotimes' function evaluated (its count first)
K_SORT          = 31            ; [less runs next nlast a b out olast]: sort's less given two items
K_HKEY          = 32            ; [cf cr ee]: a hash call's key evaluated (cf: the hash)
K_HASH          = 33            ; [ee hash items entry state first last]: a hash's entry evaluated, or its value
K_KINDS         = 34

.macro PUSHK k
            lda         #<FIX(k)
            jsr         pushk
.endmacro

; A word pushed
.macro PUSHV v
            lda         v
            ldx         v + 1
            jsr         push
.endmacro

; A word popped into v
.macro POPV v
            jsr         pop
            sta         v
            stx         v + 1
.endmacro

.code

; ---- The machine

; The machine made: its stack, empty
ev_init:
            jsr         stk_init
            stz         ev_inargs
            rts

; Its symbols (&_ ..., &err, &code, :intr) and the error kept for no room: made (a snapshot has them).  OUT: C = 1 if
; there's no room
ev_syms:
            lda         #'&'
            sta         amp_name
            ldx         #0                                  ; (&_, &1 ... &9)
@amp:
            lda         amp_digits,x
            sta         amp_name + 1
            phx
            LDHQ        amp_name, 2
            lda         #PK_SYMBOL
            jsr         intern
            plx
            bcc         @lb5
            jmp         @rts
@lb5:
            txa
            asl
            tay
            lda         hv
            sta         amp_syms,y
            lda         hv + 1
            sta         amp_syms + 1,y
            inx
            cpx         #10
            bcc         @amp
            LDHQ        n_err, 4
            lda         #PK_SYMBOL
            jsr         intern
            bcs         @rts
            MOVW        sym_err, hv
            LDHQ        n_code, 5
            lda         #PK_SYMBOL
            jsr         intern
            bcs         @rts
            MOVW        sym_code, hv
            LDHQ        n_intr, 4
            lda         #PK_ATOM
            jsr         intern
            bcs         @rts
            MOVW        atom_intr, hv
            LDHQ        s_nomem, s_nomem_n                  ; (The error kept for when there's no room)
            jsr         error_make
            bcs         @rts
            MOVW        nomem_err, hv
            clc
@rts:
            rts

; ex evaluated in ee.  IN: ex, ee.  OUT: ex = its value (an error, too).  The machine runs till then
ev_run:
            tsx
            stx         ev_s
            stz         depth
            stz         depth + 1
            stz         ev_inargs
            PUSHK       K_TOP
            jmp         ev_eval

; The end: its value back to ev_run's caller
k_top:
            jsr         cap_reset
            ldx         ev_s
            txs
            rts

; No room: the error "out of memory", given to the frame on top (a call's arguments dropped first)
ev_nomem:
            ldx         ev_s
            txs
            MOVW        ex, nomem_err
ev_abort:
            lda         ev_inargs
            beq         :+
            stz         ev_inargs
            jsr         discard_args
:
            jmp         ev_return

; No room on the stack (spill's): everything given up, its error back to ev_run's caller
ev_toodeep:
            ldx         ev_s
            txs
            jsr         stk_init
            stz         ev_inargs
            jsr         cap_reset
            LDAX        s_full
            jsr         err_text
            bcc         :+
            MOVW        ex, nomem_err
:
            rts

; ex evaluated in ee, its value given to the frame on top
ev_eval:
            lda         ex
            lsr
            bcs         ev_return                           ; (A fixnum: itself)
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @simple
            lda         pk,x
            cmp         #PK_SCONS
            bne         @simple
            jmp         ev_call
@simple:
            jsr         ev_simple
; ex, a value, given to the frame on top (an error passing by those that don't take one)
ev_return:
            jsr         pop                                 ; (The frame's code: its index * 2)
            and         #$FE
            sta         ev_jv
            lda         depth
            bne         :+
            dec         depth + 1
:
            dec         depth
            jsr         is_err
            bcc         @give
            ldx         ev_jv
            lda         k_info,x
            bne         @give
            jsr         k_drop                              ; (An error: the frame dropped, the next given it)
            bra         ev_return
@give:
            ldx         ev_jv
            jmp         (k_tab,x)

; The rest of the frame whose code was popped (ev_jv, its index * 2) dropped
k_drop:
            ldx         ev_jv
            lda         k_info + 1,x
            cmp         #$FF
            bne         pop_n
            jsr         pop                                 ; (K_ARG's: its cn arguments too)
            jsr         unfix
            sta         cn
            stx         cn + 1
            lda         #3
            jsr         pop_n
            jmp         discard_args

; .A words popped (each in RAM as it's popped)
pop_n:
            tax
            beq         @done
@word:
            phx
            jsr         pop
            plx
            dex
            bne         @word
@done:
            rts

; The cn words on top (a call's arguments) dropped, wherever they are
discard_args:
            jsr         args_at
            bcs         @pop
            jmp         drop_args
@pop:
            lda         cn
            ora         cn + 1
            beq         @done
            jsr         pop
            lda         cn
            bne         :+
            dec         cn + 1
:
            dec         cn
            bra         @pop
@done:
            rts

; .A = a frame's code (a fixnum's low byte), pushed: a frame more (frame_check said there's room)
pushk:
            ldx         #0
            inc         depth
            bne         :+
            inc         depth + 1
:
            jmp         push

; Room for a frame more?  No: the error "Too deep", given to the frame on top (as ev_nomem's are)
frame_check:
            lda         depth + 1
            cmp         #>MAX_DEPTH
            bcc         @ok
            bne         @deep
            lda         depth
            cmp         #<MAX_DEPTH
            bcc         @ok
@deep:
            ldx         ev_s
            txs
            LDAX        s_toodeep
            jsr         err_text
            bcc         :+
            jmp         ev_nomem
:
            jmp         ev_abort
@ok:
            rts

; ---- Values

; OUT: C = 1 if ex is an error.  Keeps .Y
is_err:
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

; OUT: C = 1 if ex is an S-expression's cons (a call).  Keeps .Y
is_call:
            lda         ex
            lsr
            bcs         @no
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_SCONS
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

; OUT: C = 1 if ex is false: NIL or ().  Keeps .X, .Y
is_nil:
            lda         ex + 1
            bne         @no
            lda         ex
            beq         @yes
            cmp         #<SNIL
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

.segment "DATA4"                                            ; (These in the task's RAM: every bank's)

; IN: .A, .X = a value.  OUT: .A = its kind as a list: 0 not one; 1 NIL or (); PK_SCONS or PK_QCONS (a cons);
; PK_SCOPED (a scoped one).  Keeps .X, .Y
list_kind:
            lsr
            bcs         @not
            cpx         #IMM_PAGES
            bcc         @imm
            lda         pk,x
            cmp         #PK_SCONS
            beq         @rts
            cmp         #PK_QCONS
            beq         @rts
            cmp         #PK_SCOPED
            beq         @rts
@not:
            lda         #0
@rts:
            rts
@imm:
            cpx         #0
            bne         @not
            asl
            beq         @nil                                ; (NIL)
            cmp         #<SNIL
            bne         @not
@nil:
            lda         #1
            rts

; IN: .A, .X = a value.  OUT: C = 0 and .A, .X = its conses (NIL: none), if it's a Q-expression (a cons of one,
; NIL, a scoped one whose expression is one); C = 1 if it isn't.  Keeps .Y
q_conses:
            phy
            pha
            jsr         list_kind
            cmp         #PK_QCONS
            beq         @cons
            cmp         #PK_SCOPED
            beq         @scoped
            pla
            cmp         #0                                  ; (NIL: only NIL, not ())
            bne         @not
            cpx         #0
            bne         @not
            ply
            clc
            rts
@cons:
            pla
            ply
            clc
            rts
@scoped:
            pla
            ldy         #0
            jsr         cell_get
            pha
            jsr         list_kind                           ; (A list in it: a Q-expression's)
            cmp         #PK_QCONS
            beq         @cons
            cmp         #1
            bne         :+
            pla
            pha
            beq         @cons
:
            pla
@not:
            ply
            sec
            rts

; IN: .A, .X = a fixnum.  OUT: .A, .X = its number.  Keeps .Y
unfix:
            pha
            txa
            cmp         #$80
            ror
            tax
            pla
            ror
            rts

; IN: .A, .X = a number (0 to 16383).  OUT: .A, .X = its fixnum.  Keeps .Y
fix_ax:
            asl
            pha
            txa
            rol
            tax
            pla
            ora         #1
            rts

; IN: .A, .X = a list (its conses).  OUT: .A = its items counted (to 255)
count_list:
            sta         ht
            stx         ht + 1
            ldy         #0
@item:
            lda         ht
            ora         ht + 1
            beq         @done
            iny
            beq         @most
            lda         ht
            ldx         ht + 1
            jsr         deref
            phy
            ldy         #2
            lda         (hp),y
            sta         ht
            iny
            lda         (hp),y
            sta         ht + 1
            ply
            bra         @item
@most:
            dey
@done:
            tya
            rts

.code

; ex (not a call) evaluated in ee: ex its value (a name's, a Q-expression with its scope, () NIL, anything else
; itself).  It may collect
ev_simple:
            lda         ex
            lsr
            bcs         @done
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @imm
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         :+
            jmp         lookup
:
            cmp         #PK_QCONS
            beq         @q
@done:
            rts
@imm:
            cpx         #0
            bne         @done
            lda         ex
            cmp         #<SNIL
            bne         @done
            stz         ex
            rts
@q:
            MOVW        et, ex
            MOVW        eu, ee
            lda         #PK_SCOPED
            jsr         make2
            bcc         :+
            jmp         ev_nomem
:
            MOVW        ex, hv
            rts

; ex, as written, made a scoped expression with ee if it means something only where it is (a symbol, a list);
; anything else as it is (an fexpr's argument).  It may collect
ex_scoped:
            lda         ex
            ldx         ex + 1
            jsr         list_kind
            cmp         #PK_SCONS
            beq         @scope
            cmp         #PK_QCONS
            beq         @scope
            lda         ex
            lsr
            bcs         @done
            cpx         #IMM_PAGES
            bcc         @done
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @done
@scope:
            MOVW        et, ex
            MOVW        eu, ee
            lda         #PK_SCOPED
            jsr         make2
            bcc         :+
            jmp         ev_nomem
:
            MOVW        ex, hv
@done:
            rts

; ex = cr's first item.  Keeps .Y
cr_first:
            phy
            lda         cr
            ldx         cr + 1
            ldy         #0
            jsr         cell_get
            sta         ex
            stx         ex + 1
            ply
            rts

; ex = cr's first item; cr = the rest.  Keeps .Y
cr_next:
            phy
            lda         cr
            ldx         cr + 1
            jsr         deref
            lda         (hp)
            sta         ex
            ldy         #1
            lda         (hp),y
            sta         ex + 1
            iny
            lda         (hp),y
            sta         cr
            iny
            lda         (hp),y
            sta         cr + 1
            ply
            rts

; OUT: .A = cr's items counted (to 255)
count_cr:
            lda         cr
            ldx         cr + 1
            jmp         count_list

; ---- Calls

; ex, a cons (an S-expression's, or a function's body): a call.  Its function evaluated, then its arguments
ev_call:
            bit         intr                                ; (Ctrl-C: the error interrupted, at each call)
            bpl         :+
            jmp         ev_intr
:
            lda         ex
            ldx         ex + 1
            jsr         deref
            ldy         #2
            lda         (hp),y
            sta         cr
            iny
            lda         (hp),y
            sta         cr + 1
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            sta         ex + 1
            pla
            sta         ex
            jsr         is_call
            bcs         @frame
            jsr         ev_simple
            jmp         ev_head
@frame:
            jsr         frame_check
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_HEAD
            jmp         ev_eval

k_head:
            POPV        ee
            POPV        cr
; ex, a call's function (cr its arguments, as written): the call
ev_head:
            jsr         is_err
            bcc         :+
            jmp         ev_return
:
            lda         ex
            lsr
            bcs         @notfn
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcc         :+
            jmp         ev_builtin
:
            cpx         #0                                  ; (exit, with a status: the end)
            bne         @notfn
            lda         ex
            cmp         #<EXIT_VAL
            bne         @notfn
            lda         cr
            ora         cr + 1
            beq         @notfn
            jmp         ev_exit
@cell:
            lda         pk,x
            cmp         #PK_FUNC
            bne         :+
            jmp         ev_fcall
:
            cmp         #PK_PARTIAL
            bne         :+
            jmp         ev_pcall
:
            cmp         #PK_HASH
            bne         @notfn
            jmp         ev_hcall
@notfn:
            lda         cr                                  ; (No arguments: itself)
            ora         cr + 1
            bne         :+
            jmp         ev_return
:
            jsr         err_begin
            LDAX        s_starts
            jsr         msg_out
            FARN        3, out_type
            LDAX        s_expected
            jsr         msg_out
            jmp         ev_err_end

; A hash called (ex), (h key arg...): with no arguments, itself; else its key evaluated, then the value there
; (hashes.inc's h_at): a function, a method (&0 the hash), called with the arguments after it as any function is;
; anything else the value, and arguments after it an error
ev_hcall:
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         ev_return
:
            MOVW        cf, ex
            jsr         cr_next                             ; (Its key)
            jsr         is_call
            bcs         @frame
            jsr         ev_simple
            bra         hcall_key
@frame:
            jsr         frame_check
            PUSHV       cf
            PUSHV       cr
            PUSHV       ee
            PUSHK       K_HKEY
            jmp         ev_eval
k_hkey:
            POPV        ee
            POPV        cr
            POPV        cf
hcall_key:
            jsr         is_err
            bcc         :+
            jmp         ev_return
:
            FARN        4, h_at
            bcc         :+
            jmp         ev_nomem
:
            jmp         ev_head

; (exit n): the status evaluated, then the end
ev_exit:
            jsr         cr_first
            jsr         is_call
            bcs         @frame
            jsr         ev_simple
            jmp         k_exit
@frame:
            jsr         frame_check
            PUSHK       K_EXIT
            jmp         ev_eval
k_exit:
            jsr         is_err
            bcc         :+
            jmp         ev_return
:
            lda         ex
            lsr
            bcs         :+
            LDAX        s_exitnum
            jmp         ev_error
:
            jsr         out_flush
            LDR         r0, 0
            lda         ex
            ldx         ex + 1
            jsr         unfix
            jmp         EXITS

; A built-in called (ex): a special form gets its arguments as they're written (too many: an error), an ordinary one
; their values (too many: an error then, ev_bapply's, as danlang has it: an argument's error first)
ev_builtin:
            lda         ex + 1                              ; (Its number: the value's bits 8-1)
            lsr
            lda         ex
            ror
            sta         bi_n
            tax
            lda         bi_flags,x
            bpl         @ordinary
            jsr         count_cr                            ; (A special form: its arguments counted)
            sta         bi_count
            ldx         bi_n
            cmp         bi_max,x
            beq         :+
            bcc         :+
            jmp         ev_toomany
:
            lda         bi_lo,x                             ; (Its own way)
            sta         ev_jv
            lda         bi_hi,x
            sta         ev_jv + 1
            jmp         (ev_jv)
@ordinary:
            MOVW        cf, ex
            stz         cn
            stz         cn + 1
            jmp         ev_args

; A partially applied built-in called (ex): too many arguments is an error; else its values given first, then the
; arguments'
ev_pcall:
            MOVW        cf, ex
            lda         ex                                  ; (The built-in, and its number)
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            sta         ex
            stx         ex + 1
            txa
            lsr
            lda         ex
            ror
            sta         bi_n
            lda         cf                                  ; (Its values, counted, and the arguments)
            ldx         cf + 1
            ldy         #1
            jsr         cell_get
            jsr         count_list
            sta         bi_count
            jsr         count_cr
            clc
            adc         bi_count
            bcc         :+
            lda         #255
:
            sta         bi_count
            ldx         bi_n
            cmp         bi_max,x
            beq         :+
            bcc         :+
            jmp         ev_toomany
:
            lda         cf                                  ; (Its values pushed: the call's first)
            ldx         cf + 1
            ldy         #1
            jsr         cell_get
            sta         et
            stx         et + 1
            MOVW        cf, ex
            stz         cn
            stz         cn + 1
            lda         #1
            sta         ev_inargs
@value:
            lda         et
            ora         et + 1
            bne         :+
            jmp         ev_args
:
            lda         et
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
            jsr         push
            inc         cn
            bra         @value

; A function called (ex): its arguments evaluated, or, for an fexpr, each as it's written, with this scope
ev_fcall:
            MOVW        cf, ex
            stz         cn
            stz         cn + 1
            lda         ex
            ldx         ex + 1
            ldy         #3
            jsr         cell_get                            ; (Its flags)
            and         #FN_FEXPR
            bne         :+
            jmp         ev_args
:
            lda         #1
            sta         ev_inargs
@arg:
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         ev_apply
:
            jsr         cr_next
            jsr         ex_scoped
            PUSHV       ex
            inc         cn
            bne         @arg
            inc         cn + 1
            bra         @arg

; cf's arguments (cr: those not evaluated yet), evaluated onto the stack (cn so far); then cf applied to them
ev_args:
            lda         #1
            sta         ev_inargs
@next:
            lda         cr
            ora         cr + 1
            bne         :+
            jmp         ev_apply
:
            jsr         cr_next
            jsr         is_call
            bcs         @frame
            jsr         ev_simple
            jsr         is_err
            bcs         @error
@push:
            PUSHV       ex
            inc         cn
            bne         @next
            inc         cn + 1
            bra         @next
@frame:
            jsr         frame_check                         ; (A call: its value first, in a frame)
            stz         ev_inargs
            PUSHV       cf
            PUSHV       cr
            PUSHV       ee
            lda         cn
            ldx         cn + 1
            jsr         fix_ax
            jsr         push
            PUSHK       K_ARG
            jmp         ev_eval
@error:
            jsr         takes_err                           ; (An error: the call's value, unless cf takes errors)
            bcs         @push
            stz         ev_inargs
            jsr         discard_args
            jmp         ev_return

; An argument's value: on the stack with the rest
k_arg:
            jsr         pop
            jsr         unfix
            sta         cn
            stx         cn + 1
            POPV        ee
            POPV        cr
            POPV        cf
            lda         #1
            sta         ev_inargs
            jsr         is_err
            bcc         @push
            jsr         takes_err
            bcs         @push
            stz         ev_inargs
            jsr         discard_args
            jmp         ev_return
@push:
            PUSHV       ex
            inc         cn
            bne         :+
            inc         cn + 1
:
            jmp         ev_args

; OUT: C = 1 if cf takes errors as values (an ordinary built-in that looks at values: error?, type-of ...)
takes_err:
            lda         cf + 1
            cmp         #IMM_PAGES
            bcs         @no
            lsr
            lda         cf
            ror
            tax
            lda         bi_flags,x
            and         #BF_ERRORS
            beq         @no
            sec
            rts
@no:
            clc
            rts

; ---- Applying

; cf applied to the cn values on top of the stack (a call's arguments, evaluated: ev_inargs set): a built-in or a
; function
ev_apply:
            bit         intr                                ; (Ctrl-C: the error interrupted)
            bpl         :+
            stz         ev_inargs
            jsr         discard_args
            jmp         ev_intr
:
            lda         cf + 1
            cmp         #IMM_PAGES
            bcc         ev_bapply
            jmp         ev_fapply

; A built-in (cf) applied to its values: fewer than it needs is it partially applied, waiting for the rest
ev_bapply:
            lsr
            lda         cf
            ror
            sta         bi_n
            tax
            lda         cn + 1
            bne         @many
            lda         cn
            cmp         bi_min,x
            bcc         @partial
            cmp         bi_max,x
            beq         @call
            bcc         @call
@many:
            lda         #255                                ; (More than it takes: given by map, apply ...)
            ldx         cn + 1
            bne         :+
            lda         cn
:
            sta         bi_count
            stz         ev_inargs
            jsr         discard_args
            jmp         ev_toomany
@call:
            jsr         args_at
            bcc         :+
            LDAX        s_manyargs
            jmp         ev_error_args
:
            ldx         bi_n
            lda         bi_flags,x                          ; (Fixnums, for some: worked here)
            and         #BF_FAST
            beq         :+
            jsr         bi_fast
            bcc         @done
            ldx         bi_n
:
            lda         bi_lo,x
            sta         ev_jv
            lda         bi_hi,x
            sta         ev_jv + 1
            lda         bi_flags,x
            and         #BF_MACHINE
            beq         :+
            jmp         (ev_jv)                             ; (One that runs the machine: it takes over)
:
            lda         bi_flags,x
            and         #BF_BANK
            bne         @far
            jsr         bi_jump
            bra         @done
@far:
            tax                                             ; (In another bank: called there)
            lda         hyx2_bank1,x
            sta         hyx2_n
            MOVW        hyx2_to, ev_jv
            jsr         hyx2_farn
@done:
            bcc         :+
            jmp         ev_nomem
:
            stz         ev_inargs
            jsr         drop_args
            jmp         ev_return
@partial:
            jsr         args_list                           ; (Its values, a list: the partially applied built-in)
            MOVW        eu, hv
            MOVW        et, cf
            lda         #PK_PARTIAL
            jsr         make2
            bcc         :+
            jmp         ev_nomem
:
            MOVW        ex, hv
            stz         ev_inargs
            jsr         drop_args
            jmp         ev_return

bi_jump:
            jmp         (ev_jv)

; bi_n (BF_FAST's) on its cn values at argp, if they're fixnums and so is its answer (or it's T or NIL): + and - of
; two, 1+, 1-, zero?, one?, and the comparisons.  OUT: C = 0, ex = its value; C = 1, not here (its own code's)
bi_fast:
            lda         cn + 1
            bne         @no
            lda         (argp)                              ; (The first a fixnum)
            lsr
            bcc         @no
            ldy         #1
            lda         (argp),y
            sta         bf_t                                ; (Its high byte: ex is set only when it's done)
            lda         cn
            cmp         #2
            beq         @two
            cmp         #1
            bne         @no
            lda         (argp)
            ldx         bi_n
            cpx         #BIN_ZEROP
            beq         @zero
            cpx         #BIN_ONEP
            beq         @one
            cpx         #BIN_INC
            beq         @inc
            cpx         #BIN_DEC
            beq         @dec
@no:
            sec
            rts
@zero:
            cmp         #<FIX(0)                            ; (FIX(0) and FIX(1): their high bytes 0)
            bra         @is
@one:
            cmp         #<FIX(1)
@is:
            bne         @false
            lda         bf_t
            bne         @false
@true:
            lda         #<T_VAL
            bra         @bool
@false:
            lda         #<NIL
@bool:
            sta         ex
            stz         ex + 1
            clc
            rts
@inc:
            clc                                             ; (+ 2: the next fixnum)
            adc         #2
            tax
            lda         bf_t
            adc         #0
            bra         @fix
@dec:
            sec
            sbc         #2
            tax
            lda         bf_t
            sbc         #0
@fix:
            bvs         @no                                 ; (Past a fixnum: the built-in's own)
            sta         ex + 1
            stx         ex
            clc
            rts
@two:
            ldy         #2                                  ; (The second a fixnum too)
            lda         (argp),y
            lsr
            bcc         @no
            ldx         bi_n
            cpx         #BIN_ADD
            beq         @add
            cpx         #BIN_SUB
            beq         @sub
            lda         (argp)                              ; (A comparison: the first less the second, as the
            cmp         (argp),y                            ;   words are ordered as the fixnums are)
            ldy         #1
            lda         (argp),y
            ldy         #3
            sbc         (argp),y
            bvc         :+
            eor         #$80
:
            bmi         @less                               ; (N: the first is less)
            lda         (argp)                              ; (Equal?)
            ldy         #2
            cmp         (argp),y
            bne         @more
            lda         bf_t
            ldy         #3
            cmp         (argp),y
            bne         @more
            cpx         #BIN_EQ                             ; (Equal: ==, <= and >= are T)
            beq         @true
            cpx         #BIN_LE
            beq         @true
            cpx         #BIN_GE
            beq         @true
            bra         @false
@less:
            cpx         #BIN_LT                             ; (Less: <, <= are T)
            beq         @true
            cpx         #BIN_LE
            beq         @true
            bra         @false
@more:
            cpx         #BIN_GT                             ; (More: >, >= are T)
            beq         @true
            cpx         #BIN_GE
            beq         @true
            bra         @false
@add:
            lda         (argp),y                            ; (The second, less its tag bit, and the first)
            and         #$FE
            clc
            adc         (argp)
            tax
            ldy         #3
            lda         (argp),y
            adc         bf_t
            bra         @fix
@sub:
            lda         (argp),y                            ; (The first less the second, its tag bit dropped)
            and         #$FE
            sta         bf_s
            lda         (argp)
            sec
            sbc         bf_s
            tax
            lda         bf_t
            ldy         #3
            sbc         (argp),y
            jmp         @fix

; OUT: hv = the cn values on top of the stack (at argp), a list.  It may collect (ev_nomem, if there's no room)
args_list:
            jsr         args_at
            bcc         :+
            LDAX        s_manyargs
            jmp         ev_error_args
:
            stz         eu                                  ; (From the last: each consed on)
            stz         eu + 1
            lda         cn
            asl
            sta         fa_i
            lda         cn + 1
            rol
            sta         fa_i + 1
@item:
            lda         fa_i
            ora         fa_i + 1
            beq         @done
            lda         fa_i
            sec
            sbc         #2
            sta         fa_i
            bcs         :+
            dec         fa_i + 1
:
            lda         fa_i                                ; (The item at argp + fa_i)
            clc
            adc         argp
            sta         hq
            lda         fa_i + 1
            adc         argp + 1
            sta         hq + 1
            lda         (hq)
            sta         et
            ldy         #1
            lda         (hq),y
            sta         et + 1
            lda         #PK_QCONS
            jsr         make2
            bcc         :+
            jmp         ev_nomem
:
            MOVW        eu, hv
            bra         @item
@done:
            MOVW        hv, eu
            rts

; A function (cf) applied to its cn values: fewer than its formals is it partially applied (those bound, in a scope
; of their own); more is an error, unless its body takes them (FN_EXTRAS: &1, &2 ... and &_).  Else its body runs,
; in the caller's place, in a scope of its own whose parent is the scope the function was made in
ev_fapply:
            jsr         args_at
            bcc         :+
            LDAX        s_manyargs
            jmp         ev_error_args
:
            lda         cf                                  ; (Its flags, and its formals counted: fn_make's)
            ldx         cf + 1
            ldy         #3
            jsr         cell_get
            sta         fa_flags
            stx         fa_k
            lda         cn + 1                              ; (cn against them)
            bne         @more
            lda         cn
            cmp         fa_k
            bcc         @partial
            bne         @lb4
            jmp         @bind
@lb4:
@more:
            lda         fa_flags
            and         #FN_EXTRAS
            beq         @lb3
            jmp         @bind
@lb3:
            jsr         err_begin                           ; ("The function takes 1 argument, not 2")
            LDAX        s_takes
            jsr         msg_out
            lda         fa_k
            ldx         #0
            jsr         out_dec
            LDAX        s_argument
            jsr         msg_out
            lda         fa_k
            cmp         #1
            beq         :+
            lda         #'s'
            jsr         out_byte
:
            LDAX        s_not
            jsr         msg_out
            lda         cn
            ldx         cn + 1
            jsr         out_dec
            jmp         ev_err_end_args
@partial:
            lda         cn                                  ; (Those bound, in a scope whose parent is its own ...
            jsr         fa_frame
            jsr         fa_formals
            lda         #PK_FUNC                            ;   and a function of the rest, made there)
            jsr         cell_alloc
            bcc         :+
            jmp         ev_nomem
:
            MOVW        ex, hv
            lda         fa_f                                ; (Its formals: those left)
            ldx         fa_f + 1
            ldy         #0
            jsr         ex_set
            lda         cf                                  ; (Its body)
            ldx         cf + 1
            ldy         #1
            jsr         cell_get
            ldy         #1
            jsr         ex_set
            lda         ee                                  ; (Its scope: the one made)
            ldx         ee + 1
            ldy         #2
            jsr         ex_set
            lda         fa_k                                ; (Its flags, and its formals: those left)
            sec
            sbc         cn
            tax
            lda         fa_flags
            ldy         #3
            jsr         ex_set
            stz         ev_inargs
            jsr         drop_args
            jmp         ev_return
@bind:
            lda         fa_flags                            ; (Its scope: its formals, and, if it takes them, its
            and         #FN_EXTRAS                          ;   extras and &_)
            beq         @formals
            lda         cn + 1
            bne         @big
            lda         cn
            inc
            bne         @frame
@big:
            lda         #255
            bra         @frame
@formals:
            lda         fa_k
@frame:
            jsr         fa_frame
            jsr         fa_formals
            lda         fa_flags
            and         #FN_EXTRAS
            beq         @run
            jsr         fa_extras
@run:
            stz         ev_inargs                           ; (Its body, as a call, there)
            jsr         drop_args
            lda         cf
            ldx         cf + 1
            ldy         #1
            jsr         cell_get
            sta         ex
            stx         ex + 1
            ora         ex + 1
            bne         :+
            jmp         ev_return                           ; (No body: NIL)
:
            jmp         ev_call

; ex's word .Y set to .A, .X
ex_set:
            pha
            phx
            lda         ex
            ldx         ex + 1
            jsr         deref
            tya
            asl
            tay
            plx
            pla
            sta         (hp),y
            iny
            txa
            sta         (hp),y
            rts

; A function's scope: ee = a new frame for .A pairs, its parent cf's scope.  It may collect
fa_frame:
            pha
            lda         cf
            ldx         cf + 1
            ldy         #2
            jsr         cell_get
            sta         eu
            stx         eu + 1
            pla
            jsr         frame_new
            MOVW        ee, hv
            rts

; cf's formals bound in ee to the arguments at argp, as many as there are of both.  OUT: fa_f = the formals left
; (none, unless it's partially applied); fa_i = the arguments bound
fa_formals:
            lda         cf
            ldx         cf + 1
            ldy         #0
            jsr         cell_get
            sta         fa_f
            stx         fa_f + 1
            stz         fa_i
            stz         fa_i + 1
@formal:
            lda         fa_f
            ora         fa_f + 1
            beq         @done
            lda         fa_i + 1                            ; (Arguments left?)
            cmp         cn + 1
            bcc         :+
            lda         fa_i
            cmp         cn
            bcs         @done
:
            jsr         fa_arg                              ; (The argument: eu)
            lda         fa_f                                ; (The formal: bound to it)
            ldx         fa_f + 1
            ldy         #0
            jsr         cell_get
            jsr         frame_add
            lda         fa_f
            ldx         fa_f + 1
            ldy         #1
            jsr         cell_get
            sta         fa_f
            stx         fa_f + 1
            inc         fa_i
            bne         @formal
            inc         fa_i + 1
            bra         @formal
@done:
            rts

; eu = the argument fa_i (at argp)
fa_arg:
            lda         fa_i
            asl
            sta         hq
            lda         fa_i + 1
            rol
            sta         hq + 1
            lda         hq
            clc
            adc         argp
            sta         hq
            lda         hq + 1
            adc         argp + 1
            sta         hq + 1
            lda         (hq)
            sta         eu
            ldy         #1
            lda         (hq),y
            sta         eu + 1
            rts

; cf's extras (the arguments past its formals, fa_i on) bound in ee: &1, &2 ..., and &_ a list of them
fa_extras:
            lda         fa_i
            sta         fa_f                                ; (The first extra's index, kept)
            lda         fa_i + 1
            sta         fa_f + 1
@extra:
            lda         fa_i + 1
            cmp         cn + 1
            bcc         :+
            lda         fa_i
            cmp         cn
            bcs         @rest
:
            jsr         fa_arg
            lda         fa_i                                ; (Its number, from 1)
            sec
            sbc         fa_f
            tax
            inx
            jsr         amp_sym
            jsr         frame_add
            inc         fa_i
            bne         @extra
            inc         fa_i + 1
            bra         @extra
@rest:
            stz         eu                                  ; (&_: from the last extra back, each consed on)
            stz         eu + 1
@item:
            lda         fa_i + 1
            cmp         fa_f + 1
            bne         :+
            lda         fa_i
            cmp         fa_f
            beq         @bound
:
            lda         fa_i
            bne         :+
            dec         fa_i + 1
:
            dec         fa_i
            lda         eu                                  ; (The list so far, kept while the item's cons is made)
            pha
            lda         eu + 1
            pha
            jsr         fa_arg
            MOVW        et, eu
            pla
            sta         eu + 1
            pla
            sta         eu
            lda         #PK_QCONS
            jsr         make2
            bcc         :+
            jmp         ev_nomem
:
            MOVW        eu, hv
            bra         @item
@bound:
            lda         amp_syms
            ldx         amp_syms + 1
            jmp         frame_add

; IN: .X = n (1 to 255).  OUT: .A, .X = the symbol &n.  It may collect (ev_nomem)
amp_sym:
            cpx         #10
            bcs         @make
            txa
            asl
            tax
            lda         amp_syms,x
            pha
            lda         amp_syms + 1,x
            tax
            pla
            rts
@make:
            txa                                             ; (&10 on: made, its digits after the &)
            ldy         #1
            ldx         #0
@hundreds:
            cmp         #100
            bcc         @tens0
            sbc         #100
            inx
            bra         @hundreds
@tens0:
            cpx         #0
            beq         @tens
            pha
            txa
            ora         #'0'
            sta         amp_name,y
            iny
            pla
@tens:
            ldx         #0
@ten:
            cmp         #10
            bcc         @ones
            sbc         #10
            inx
            bra         @ten
@ones:
            pha
            txa
            ora         #'0'
            sta         amp_name,y
            iny
            pla
            ora         #'0'
            sta         amp_name,y
            iny
            sty         hn
            stz         hn + 1
            lda         #<amp_name
            sta         hq
            lda         #>amp_name
            sta         hq + 1
            lda         #PK_SYMBOL
            jsr         intern
            bcc         :+
            jmp         ev_nomem
:
            lda         hv
            ldx         hv + 1
            rts

; cf applied to cn values on the stack, as they are (not evaluated again: map's, each's, try's handler ...): a
; partially applied built-in's values put first; a special form, or anything that isn't a function, an error
ev_apply_values:
            lda         #1
            sta         ev_inargs
            lda         cf
            lsr
            bcs         @notfn
            ldx         cf + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcc         @notfn
            txa                                             ; (A built-in: not a special form)
            lsr
            lda         cf
            ror
            tax
            lda         bi_flags,x
            bmi         @special
            bra         @values
@cell:
            lda         pk,x
            cmp         #PK_FUNC
            beq         @values
            cmp         #PK_PARTIAL
            beq         @partial
@notfn:
            MOVW        ex, cf
            jsr         err_begin
            LDAX        s_starts
            jsr         msg_out
            FARN        3, out_type
            LDAX        s_expected
            jsr         msg_out
            jmp         ev_err_end_args
@special:
            stx         bi_n
            jsr         err_begin
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            LDAX        s_special
            jsr         msg_out
            jmp         ev_err_end_args
@values:
            jsr         args_errors                         ; (An error among them: it, unless cf takes errors)
            bcc         :+
            jmp         ev_return
:
            jmp         ev_apply
@partial:
            jsr         partial_values
            bra         @values

; An error among the cn values on the stack (at argp), and cf doesn't take errors: ex = the first, the values
; dropped, C = 1.  Else C = 0
args_errors:
            jsr         args_at
            bcs         @none
            lda         cf + 1                              ; (A function: an error stops it, but an fexpr's)
            cmp         #IMM_PAGES
            bcc         @builtin
            lda         cf
            ldx         cf + 1
            ldy         #3
            jsr         cell_get
            and         #FN_FEXPR
            bne         @none
            bra         @look
@builtin:
            jsr         takes_err
            bcs         @none
@look:
            ldy         #0
            stz         fa_i
            stz         fa_i + 1
@value:
            lda         fa_i + 1
            cmp         cn + 1
            bcc         :+
            lda         fa_i
            cmp         cn
            bcs         @none
:
            jsr         fa_arg
            MOVW        ex, eu
            jsr         is_err
            bcs         @error
            inc         fa_i
            bne         @value
            inc         fa_i + 1
            bra         @value
@error:
            stz         ev_inargs
            jsr         drop_args
            sec
            rts
@none:
            clc
            rts

; cf, a partially applied built-in, given cn values on the stack: its own values put below them (the stack
; grown for them), cf its built-in, cn their count with them
partial_values:
            lda         cf                                  ; (Its values, counted)
            ldx         cf + 1
            ldy         #1
            jsr         cell_get
            sta         et
            stx         et + 1
            jsr         count_list
            sta         fa_k
            bne         @lb2
            jmp         @done
@lb2:
            lda         cn                                  ; (The values given: moved up, from the last down)
            sta         fa_i
            lda         cn + 1
            sta         fa_i + 1
            ldx         fa_k
@room:
            lda         #0
            phx
            jsr         push
            plx
            dex
            bne         @room
            lda         cn
            clc
            adc         fa_k
            sta         cn
            bcc         :+
            inc         cn + 1
:
            jsr         args_at                             ; (argp: where its first goes; the given ones from there)
            lda         fa_i                                ; (hq: the last given; hb: where it goes)
            asl
            sta         hq
            lda         fa_i + 1
            rol
            sta         hq + 1
            lda         hq
            clc
            adc         argp
            sta         hq
            lda         hq + 1
            adc         argp + 1
            sta         hq + 1
            lda         fa_k
            asl
            clc
            adc         hq
            sta         hb
            lda         hq + 1
            adc         #0
            sta         hb + 1
@move:
            lda         fa_i
            ora         fa_i + 1
            beq         @values
            lda         hq
            sec
            sbc         #2
            sta         hq
            bcs         :+
            dec         hq + 1
:
            lda         hb
            sec
            sbc         #2
            sta         hb
            bcs         :+
            dec         hb + 1
:
            lda         (hq)
            sta         (hb)
            ldy         #1
            lda         (hq),y
            sta         (hb),y
            lda         fa_i
            bne         :+
            dec         fa_i + 1
:
            dec         fa_i
            bra         @move
@values:
            ldy         #0                                  ; (Its own, at argp on)
@value:
            lda         et
            ora         et + 1
            beq         @done
            phy
            lda         et
            ldx         et + 1
            jsr         deref
            lda         (hp)
            sta         hq
            ldy         #1
            lda         (hp),y
            sta         hq + 1
            iny
            lda         (hp),y
            sta         et
            iny
            lda         (hp),y
            sta         et + 1
            ply
            lda         hq
            sta         (argp),y
            iny
            lda         hq + 1
            sta         (argp),y
            iny
            bra         @value
@done:
            lda         cf                                  ; (cf: its built-in)
            ldx         cf + 1
            ldy         #0
            jsr         cell_get
            sta         cf
            stx         cf + 1
            rts

; ---- Errors

; An error made of the zero-terminated message at .A, .X (in the fourth bank: msg_err): ex.  OUT: C = 1 if there's
; no room
err_text:
            jsr         msg_err
            bcs         @rts
            MOVW        ex, hv
@rts:
            rts

; The error .A, .X (a message), given to the frame on top
ev_error:
            jsr         err_text
            bcc         :+
            jmp         ev_nomem
:
            jmp         ev_return

; The same, the call's arguments dropped first
ev_error_args:
            jsr         err_text
            bcc         :+
            jmp         ev_nomem
:
            stz         ev_inargs
            jsr         discard_args
            jmp         ev_return

; An error's message begun (its text out till err_end)
err_begin:
            jsr         cap_begin
            sta         eb_start
            stx         eb_start + 1
            rts

; The message ended: ex = the error.  OUT: C = 1 if there's no room
err_end:
            lda         eb_start
            ldx         eb_start + 1
            jsr         cap_end
            bcs         @rts
            lda         hv                                  ; (An error already: "Too long")
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

; The message ended, the error given to the frame on top
ev_err_end:
            jsr         err_end
            bcc         :+
            jmp         ev_nomem
:
            jmp         ev_return

; The same, the call's arguments dropped first
ev_err_end_args:
            jsr         err_end
            bcc         :+
            jmp         ev_nomem
:
            stz         ev_inargs
            jsr         discard_args
            jmp         ev_return

; Ctrl-C: the error interrupted (:intr), given to the frame on top
ev_intr:
            LDAX        s_intr
            jsr         err_text
            bcc         :+
            jmp         ev_nomem
:
            lda         ex                                  ; (Its code)
            ldx         ex + 1
            jsr         deref
            ldy         #2
            lda         atom_intr
            sta         (hp),y
            iny
            lda         atom_intr + 1
            sta         (hp),y
            jmp         ev_return

; Built-in bi_n given bi_count arguments, more than it takes: "'len' takes 1 argument, not 2"
ev_toomany:
            jsr         err_begin
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            LDAX        s_btakes
            jsr         msg_out
            ldx         bi_n
            lda         bi_max,x
            cmp         bi_min,x
            beq         :+
            LDAX        s_atmost
            jsr         msg_out
:
            ldx         bi_n
            lda         bi_max,x
            ldx         #0
            jsr         out_dec
            LDAX        s_argument
            jsr         msg_out
            ldx         bi_n
            lda         bi_max,x
            cmp         #1
            beq         :+
            lda         #'s'
            jsr         out_byte
:
            LDAX        s_not
            jsr         msg_out
            lda         bi_count
            ldx         #0
            jsr         out_dec
            jmp         ev_err_end

; ---- Scopes

; IN: .A = the pairs a frame will hold (it grows past them), eu = its parent.  OUT: hv = the frame, empty.  It may
; collect (ev_nomem, if there's no room)
frame_new:
            ldx         #PK_FRAME8
            cmp         #2
            bcc         @kind
            inx
            cmp         #4
            bcc         @kind
            inx
            cmp         #8
            bcc         @kind
            inx
@kind:
            txa
            jsr         cell_alloc
            bcc         :+
            jmp         ev_nomem
:
            lda         eu
            sta         (hp)
            ldy         #1
            lda         eu + 1
            sta         (hp),y
            rts

; IN: .A, .X = a symbol; eu = a value.  The symbol bound to the value in ee's frame (its pair there changed, if it
; has one; else a pair more, in a cell more if it's full).  It may collect (ev_nomem)
frame_add:
            sta         lk_s
            stx         lk_s + 1
            jsr         deref                               ; (The symbol: bound in a frame)
            ldy         #6
            lda         (hp),y
            ora         #SF_LOCAL
            sta         (hp),y
            MOVW        lk_c, ee
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
            beq         @free
            dey
            lda         (hp),y
            cmp         lk_s
            bne         @skip
            iny
            lda         (hp),y
            cmp         lk_s + 1
            bne         @skip1
            iny                                             ; (Its pair: the value changed)
            bra         @value
@skip:
            iny
@skip1:
            iny
            iny
            iny
            bra         @pair
@free:
            dey                                             ; (A free pair: the name, the value)
            lda         lk_s
            sta         (hp),y
            iny
            lda         lk_s + 1
            sta         (hp),y
            iny
@value:
            lda         eu
            sta         (hp),y
            iny
            lda         eu + 1
            sta         (hp),y
            rts
@next:
            ldy         #2                                  ; (The cell its pairs go on in)
            lda         (hp),y
            tax
            iny
            lda         (hp),y
            bne         @on
            cpx         #0
            bne         @on
            lda         #PK_FRAME64                         ; (None: a cell more, linked from this one)
            jsr         cell_alloc
            bcc         :+
            jmp         ev_nomem
:
            lda         lk_c
            ldx         lk_c + 1
            jsr         deref
            ldy         #2
            lda         hv
            sta         (hp),y
            iny
            lda         hv + 1
            sta         (hp),y
            MOVW        lk_c, hv
            jmp         @cell
@on:
            stx         lk_c
            sta         lk_c + 1
            jmp         @cell

; IN: ex = a symbol.  OUT: C = 0 and lk_c, .Y = the cell and place of its value in ee's frames (the nearest);
; C = 1 if none binds it, and .A = 0 if no frame ever has (hp its symbol's cell, then).  (lookup's and set!'s)
find_bound:
            lda         ex
            ldx         ex + 1
            jsr         deref
            ldy         #6
            lda         (hp),y
            and         #SF_LOCAL
            bne         @lb1
            sec                                             ; (.A 0: hp the symbol's)
            rts
@lb1:
            MOVW        lk_f, ee
@frame:
            lda         lk_f
            ora         lk_f + 1
            beq         @none
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
            cmp         ex
            bne         @skip
            iny
            lda         (hp),y
            cmp         ex + 1
            bne         @skip1
            iny                                             ; (Found: its value at .Y)
            clc
            rts
@skip:
            ora         #0                                  ; (A name NIL: the frame's pairs end)
            bne         :+
            iny
            lda         (hp),y
            beq         @parent
            dey
:
            iny
@skip1:
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
@parent:
            lda         lk_f                                ; (The frame's parent)
            ldx         lk_f + 1
            jsr         deref
            lda         (hp)
            sta         lk_f
            ldy         #1
            lda         (hp),y
            sta         lk_f + 1
            bra         @frame
@none:
            lda         #1                                  ; (hp a frame's)
            sec
            rts

; ex, a symbol: its value in ee (the nearest frame binding it, else its global value).  Unbound: the error
; "Unbound Symbol 'x'"
lookup:
            jsr         find_bound
            bcs         @global
            lda         (hp),y                              ; (hp: the cell, from find_bound)
            pha
            iny
            lda         (hp),y
            sta         ex + 1
            pla
            sta         ex
            rts
@global:
            cmp         #0                                  ; (hp the symbol's already?)
            beq         :+
            lda         ex
            ldx         ex + 1
            jsr         deref
:
            ldy         #3
            lda         (hp),y
            tax
            dey
            lda         (hp),y
            cmp         #<UNBOUND
            bne         @value
            cpx         #>UNBOUND
            bne         @value
            jsr         err_begin                           ; ("Unbound Symbol 'x'")
            LDAX        s_unbsym
            jsr         msg_out
            lda         ex
            ldx         ex + 1
            jsr         out_name
            lda         #$27
            jsr         out_byte
            jsr         err_end
            bcc         :+
            jmp         ev_nomem
:
            rts
@value:
            sta         ex
            stx         ex + 1
            rts

; IN: .A, .X = a symbol or an atom.  Its name out
out_name:
            jsr         deref
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldy         #0
@byte:
            cpy         hn
            beq         @done
            lda         (hp),y
            jsr         out_byte
            iny
            bne         @byte
@done:
            rts

.rodata
amp_digits: .byte       "_123456789"
n_err:      .byte       "&err"
n_code:     .byte       "&code"
n_intr:     .byte       "intr"
s_nomem:    .byte       "out of memory"
s_nomem_n = * - s_nomem

; The frames: what each does with a value, whether it takes an error, its words but its code
k_tab:      .word       k_top, k_head, k_arg, k_if, k_do, k_and, k_or, k_eval, k_exit, k_defn, k_defv, k_letb
            .word       k_letx, k_try, k_tryh, k_wtest, k_wbody, k_eachf, k_eachb, k_eachl, k_dotf, k_dotb
            .word       k_dotn, k_space, k_output, k_walk, k_load, k_eachf2, k_eachf3, k_dotf2, k_dotf3, k_sort
            .word       k_hkey, k_hash
k_info:     .byte       1, 0,   0, 2,   1, $FF, 0, 2,   0, 2,   0, 2,   0, 2,   0, 1,   0, 0,   0, 3,   1, 4
            .byte       0, 4,   0, 2,   1, 2,   0, 1,   0, 3,   0, 3,   0, 2,   0, 5,   0, 3,   0, 3,   0, 6
            .byte       0, 3,   0, 2,   1, 3,   0, 6,   1, 5,   0, 2,   0, 1,   0, 2,   0, 1,   0, 8
            .byte       0, 3,   1, 7

.code

.segment "RODATA4"                                          ; (Messages: read through msg_out, msg_err)
s_full:     .byte       "Too deep: the evaluation stack is full", 0
s_toodeep:     .byte       "Too deep: more than 2500 calls nested", 0
s_intr:     .byte       "interrupted", 0
s_starts:   .byte       "S-Expression starts with incorrect type. Got ", 0
s_expected: .byte       ", Expected Function.", 0
s_exitnum:  .byte       "'exit' expects a Number", 0
s_manyargs: .byte       "Too many arguments: more than the stack holds", 0
s_takes:    .byte       "The function takes ", 0
s_btakes:   .byte       "' takes ", 0
s_atmost:   .byte       "at most ", 0
s_argument: .byte       " argument", 0
s_not:      .byte       ", not ", 0
s_unbsym:  .byte       "Unbound Symbol '", 0
s_special:  .byte       "' can't be applied to values: it's a special form", 0

.code
