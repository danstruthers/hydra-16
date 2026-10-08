; ****************************************************************************
; procs.inc - SUBs and FUNCTIONs compiled (the second bank).  Pass 1 finds each one's name, its parameters' kinds and
; its number (scan_proc); pass 2 compiles its body in a scope of its own: its parameters frame slots 0, 1 ..., then
; the frame's mark (CALL's), then a FUNCTION's result, then its locals, each made as it's first named.  Its code is
; jumped over by the main program's; at its end, RET (or RETF, its result after its arguments), then its locals'
; kinds (CALL makes their defaults), and its entry in procs: its code, parameters, locals, their kinds' place.
;   A call: its arguments pushed as its parameters take them.  A variable (a name alone, of the parameter's kind) is
; given by reference (REFV: VT_REF); an array's element, by its place (AREF, before the arguments: VT_LOC), its value
; given (LDLOC) and written back after the call (WBK); an array, by its variable's reference (a()); a record, by its
; variable's; anything else, a copy (its value).  After the call, the arguments dropped (POPN), a FUNCTION's value slid
; down over them (SLIDE).

ARG_MAX         = 16            ; A procedure's parameters at most

.bss
pk_buf:     .res        ARG_MAX                             ; A procedure's parameters' kinds (PK_*)
pk_n:       .res        1                                   ;   and their count
pr_num:     .res        1                                   ; (The procedure called or compiled, its kind
pr_func:    .res        1                                   ;   (SPR_FUNC: 0 a SUB))
pr_paren:   .res        1                                   ; (A call's arguments in parentheses)
pr_elem:    .res        ARG_MAX                             ; (Each argument: 0, or 1 + its place's number (an element's))
pr_nloc:    .res        1                                   ; (The places pushed)
pr_argi:    .res        1                                   ; (The argument being compiled)
pr_res:     .res        1                                   ; (A FUNCTION's result's slot, as it's compiled; 0: a SUB)
pr_name:    .res        NAME_MAX + 2                        ; (Its name, for its result's assignment)
pr_nlen:    .res        1
pr_static:  .res        1                                   ; (SUB ... STATIC: its locals all static)
sc_depth:   .res        1                                   ; (arg_skip's)
pr_mklocal: .res        1                                   ; (param_kinds: each parameter its local too)
pr_np:      .res        1                                   ; (The procedure's parameters, as it's compiled)
pp_suf:     .res        1                                   ; (A parameter's suffix)
as_fix:     .res        2                                   ; (as_type's: STRING * n's n)

.segment "CODE2"

; ---- Pass 1: SUB name [(parameters)] [STATIC], FUNCTION, DECLARE's: the procedure made (its kind, its parameters'
; kinds, its number), unless it's known (DECLARE, or a second SUB of the name: pass 2 says so)
scan_proc:
            lda         tkv
            sta         pr_func
            jsr         lex_next
            lda         tk
            cmp         #TK_NAME
            beq         :+
            rts                                             ; (Pass 2 says what's wrong)
:
            jsr         proc_find
            bcs         :+
            rts
:
            ldx         #0                                  ; (Its kind: a SUB, a number's or a string's FUNCTION)
            lda         pr_func
            cmp         #K_FUNCTION
            bne         :+
            inx
            lda         tsuf
            cmp         #'$'
            bne         :+
            inx
:
            stx         pr_func
            ldx         #NAME_MAX + 1                       ; (Its name kept: the parameters' names are read too)
:
            lda         tname,x
            sta         pr_name,x
            dex
            bpl         :-
            lda         tnl
            sta         pr_nlen
            jsr         lex_next
            jsr         param_kinds                         ; (pk_buf, pk_n)
            ldx         #NAME_MAX + 1
:
            lda         pr_name,x
            sta         tname,x
            dex
            bpl         :-
            lda         pr_nlen
            sta         tnl
            lda         nprocs
            cmp         #PROC_MAX
            bcc         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         c_error
:
            jsr         spl_clear
            lda         nprocs
            sta         spl + SPR_NUM
            inc         nprocs
            lda         pr_func
            sta         spl + SPR_FUNC
            lda         pk_n
            sta         spl + SPR_NP
            lda         symtop                              ; (Its parameters' kinds: after its entry)
            sta         t4 + 2
            lda         symtop + 1
            sta         t4 + 3
            lda         #SC_PROC
            sta         scls
            stz         sscope
            jsr         sym_add
            MOVW        spl + SPR_PK, symtop
            jsr         sym_put
            lda         symtop
            ldx         symtop + 1
            jsr         sym_ca
            ldx         #0
:
            cpx         pk_n
            beq         :+
            lda         pk_buf,x
            jsr         ca_wr
            inx
            bra         :-
:
            lda         #LB_SYM
            jsr         ca_addr
            MOVW        symtop, t0
            jmp         code_sel

; The parameters' list, its ( current (or none): pk_buf their kinds, pk_n their count.  (pass 2: param_locals
; makes each its local too, when pr_mklocal is set)
param_kinds:
            stz         pk_n
            lda         #'('
            jsr         is_op
            beq         :+
            rts
:
            jsr         lex_next
            lda         #')'
            jsr         is_op
            bne         @param
            jmp         lex_next
@param:
            lda         tk                                  ; (BYVAL: taken, as a copy is what an expression gives)
            cmp         #TK_NAME
            beq         :+
            jmp         syntax_err
:
            ldy         #PK_NUM                             ; (Its kind: $ a string's)
            lda         tsuf
            cmp         #'$'
            bne         :+
            ldy         #PK_STR
:
            sty         t4 + 1
            ldx         #NAME_MAX + 1                       ; (Its name, for its local: pass 2)
:
            lda         tname,x
            sta         dname,x
            dex
            bpl         :-
            lda         tnl
            sta         dnl
            lda         tsuf
            sta         pp_suf
            jsr         lex_next
            lda         #'('                                ; (name(): an array)
            jsr         is_op
            bne         @as
            jsr         lex_next
            lda         #')'
            jsr         need_op
            lda         t4 + 1
            clc
            adc         #PK_NARR
            sta         t4 + 1
@as:
            lda         #K_AS                               ; (AS type)
            jsr         is_kw
            bne         @have
            jsr         lex_next
            jsr         as_type                             ; (.A: PK_NUM or PK_STR, or a record's PK_REC | type << 3)
            ldx         t4 + 1                              ; (An array of them: past PK_NARR)
            cpx         #PK_NARR
            bcc         :+
            cmp         #PK_REC
            bcs         @rarr
            clc
            adc         #PK_NARR
            bra         :+
@rarr:
            clc
            adc         #PK_RARR - PK_REC
:
            sta         t4 + 1
@have:
            ldx         pk_n
            cpx         #ARG_MAX
            bcc         :+
            jmp         syntax_err
:
            lda         t4 + 1
            sta         pk_buf,x
            inc         pk_n
            lda         pr_mklocal
            beq         :+
            jsr         param_local
:
            lda         #','
            jsr         is_op
            bne         :+
            jsr         lex_next
            jmp         @param
:
            lda         #')'
            jmp         need_op

; AS type: a number's names, STRING (or STRING * n), or a TYPE's name.  OUT: .A = PK_NUM, PK_STR, or PK_REC | its
; type << 3 (as_fix: a fixed length's n, 0 none)
as_type:
            stz         as_fix
            stz         as_fix + 1
            lda         tk
            cmp         #TK_KW
            bne         @type
            lda         tkv
            ldx         #as_nums_n - 1
:
            cmp         as_nums,x
            beq         @num
            dex
            bpl         :-
            cmp         #K_STRING
            bne         @bad
            jsr         lex_next
            lda         #'*'                                ; (STRING * n)
            jsr         is_op
            bne         :+
            jsr         lex_next
            lda         tk
            cmp         #TK_NUM
            bne         @bad
            lda         tisint
            beq         @bad
            lda         tint
            sta         as_fix
            lda         tint + 1
            sta         as_fix + 1
            jsr         lex_next
:
            lda         #PK_STR
            rts
@num:
            jsr         lex_next
            lda         #PK_NUM
            rts
@type:
            cmp         #TK_NAME
            bne         @bad
            FARN        3, type_find                        ; (C = 0: .A its number)
            bcs         @bad
            asl         a
            asl         a
            asl         a
            ora         #PK_REC
            pha
            jsr         lex_next
            pla
            rts
@bad:
            jmp         syntax_err
as_nums:    .byte       K_INTEGER, K_LONG, K_SINGLE, K_DOUBLE, K_NUMBER
as_nums_n   = * - as_nums

; Pass 2: a parameter's local, of its kind (pk_buf's last), the name dname
param_local:
            ldx         #NAME_MAX + 1
:
            lda         dname,x
            sta         tname,x
            dex
            bpl         :-
            lda         dnl
            sta         tnl
            lda         pp_suf
            sta         tsuf
            ldx         pk_n
            lda         pk_buf - 1,x
            and         #7
            tax
            lda         pk_class,x
            sta         scls
            lda         cscope
            sta         sscope
            jsr         sym_find                            ; (Twice in the list: Duplicate definition)
            bcs         :+
            lda         #ER_DUP
            ldx         #0
            jmp         c_error
:
            ldx         pk_n
            lda         pk_buf - 1,x
            ldy         #LK_NUM
            jsr         var_new
            ldx         pk_n                                ; (Its record's type, or an array's)
            lda         pk_buf - 1,x
            lsr         a
            lsr         a
            lsr         a
            sta         spl + SP_TYPE
            lda         #SF_DECL                            ; (Declared: its kind's its own, DEFSTR or not)
            sta         spl + SP_FLAGS
            jsr         sym_put
            jmp         lex_next_keep
pk_class:   .byte       SC_NUM, SC_STR, SC_NARR, SC_SARR, SC_REC, SC_RARR
; (param_local is called with the token after the parameter current: it's kept)
lex_next_keep:
            rts

; DECLARE SUB ..., DECLARE FUNCTION ...: known already (pass 1): the rest of the statement passed over
st_declare:
@skip:
            lda         tk
            beq         @done
            lda         #':'
            jsr         is_op
            beq         @done
            jsr         lex_next
            bra         @skip
@done:
            rts

; ---- Pass 2: SUB name [(parameters)] [STATIC], FUNCTION: the body's start
st_sub_kw:
st_sub:
            stz         pr_static                           ; (Its parameters: locals, STATIC or not)
            lda         st_kw
            sta         pr_func
            lda         nblk                                ; (Not in a block, nor in another procedure)
            ora         cscope
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syntax_err
:
            jsr         proc_find
            bcc         :+
            jmp         syntax_err
:
            lda         spl + SPR_NUM
            sta         pr_num
            lda         spl + SPR_FUNC
            sta         pr_func
            ldx         #NAME_MAX + 1                       ; (Its name: a FUNCTION's result is assigned by it)
:
            lda         tname,x
            sta         pr_name,x
            dex
            bpl         :-
            lda         tnl
            sta         pr_nlen
            jsr         proc_entry                          ; (t4: its procs entry; defined twice: Duplicate)
            ldy         #6
            lda         (t4),y
            beq         :+
            lda         #ER_DUP
            ldx         #0
            jmp         c_error
:
            lda         #1
            sta         (t4),y
            lda         #OP_JMP                             ; (The main program's way past it)
            jsr         emit_fwd
            lda         pr_func
            beq         :+
            lda         #B_FUNC
            bra         :++
:
            lda         #B_SUB
:
            jsr         blk_open
            ldy         #1
            lda         pr_num
            sta         (t4),y
            iny
            lda         t0
            sta         (t4),y
            iny
            lda         t0 + 1
            sta         (t4),y
            jsr         proc_entry                          ; (Its code: here)
            jsr         cpc_get
            sta         (t4)
            ldy         #1
            txa
            sta         (t4),y
            lda         pr_num                              ; (Its scope)
            inc         a
            sta         cscope
            stz         lnext
            stz         pr_res
            jsr         lex_next
            lda         #1                                  ; (Its parameters: its first locals)
            sta         pr_mklocal
            jsr         param_kinds
            stz         pr_mklocal
            lda         pk_n
            sta         pr_np
            ldy         #LK_NUM                             ; (The frame's mark)
            jsr         local_new
            lda         pr_func                             ; (A FUNCTION's result)
            beq         @static
            ldy         #LK_NUM
            cmp         #2
            bne         :+
            ldy         #LK_STR
:
            jsr         local_new
            sta         pr_res
@static:
            stz         pr_static
            lda         #K_STATIC
            jsr         is_kw
            bne         :+
            inc         pr_static
            jsr         lex_next
:
            rts

; t4 = procs' entry of pr_num
proc_entry:
            lda         pr_num
            asl         a
            asl         a
            asl         a
            clc
            adc         #<procs
            sta         t4
            lda         #>procs
            adc         #0
            sta         t4 + 1
            rts

; END SUB, END FUNCTION (its keyword current): the exit chain here, RET or RETF, the locals' kinds, its entry
end_sub:
            lda         tkv
            ldx         #B_SUB
            cmp         #K_SUB
            beq         :+
            ldx         #B_FUNC
:
            stx         t0
            jsr         blk_top
            cmp         t0
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            jsr         lex_next
            lda         #4                                  ; (EXIT SUB's jumps: here)
            jsr         blk_word
            jsr         chain_fix
            jsr         proc_ret                            ; (RET np, or RETF np res)
            jsr         proc_entry                          ; (Its entry: parameters, locals, their kinds' place)
            ldy         #2
            lda         pr_np
            sta         (t4),y
            iny
            lda         lnext                               ; (Its locals: past the mark)
            sec
            sbc         pr_np
            sbc         #1
            sta         (t4),y
            sta         t4 + 2
            inc         a
            jsr         emit_room
            jsr         cpc_get
            ldy         #4
            sta         (t4),y
            iny
            txa
            sta         (t4),y
            ldx         pr_np                               ; (The kinds: from the mark's slot + 1)
            inx
@kind:
            cpx         lnext
            bcs         @kinds
            lda         lkinds,x
            jsr         emit
            inx
            bra         @kind
@kinds:
            jsr         blk_top                             ; (The main program's jump: here)
            ldy         #2
            lda         (t4),y
            sta         t0
            iny
            lda         (t4),y
            sta         t0 + 1
            jsr         cpc_get
            jsr         patch_w
            dec         nblk
            stz         cscope
            rts

; RET np (a SUB's), or RETF np res (a FUNCTION's): emitted
proc_ret:
            lda         pr_res
            bne         :+
            lda         #OP_RET
            ldx         #2
            jsr         emit_op
            lda         pr_np
            jmp         emit
:
            lda         #OP_RETF
            ldx         #3
            jsr         emit_op
            lda         pr_np
            jsr         emit
            lda         pr_res
            jmp         emit

; In a FUNCTION, its name assigned (tname): its result's slot.  OUT: C = 0 yes (spl's slot, scls its class)
fn_result:
            lda         pr_res
            beq         @no
            lda         cscope
            beq         @no
            lda         tnl
            cmp         pr_nlen
            bne         @no
            ldx         #0
:
            lda         tname,x
            cmp         pr_name,x
            bne         @no
            inx
            cpx         tnl
            bne         :-
            lda         pr_res
            sta         spl + SP_SLOT
            stz         spl + SP_SLOT + 1
            stz         spl + SP_FLAGS
            ldx         #SC_NUM
            lda         pr_func
            cmp         #2
            bne         :+
            ldx         #SC_STR
:
            stx         scls
            clc
            rts
@no:
            sec
            rts

; RETURN in a procedure: in a FUNCTION its return (RETURN value: the value its result first); in a SUB, GOSUB's (C = 1)
fn_return:
            lda         pr_res
            bne         :+
            sec
            rts
:
            lda         tk                                  ; (A value: its result)
            beq         @ret
            cmp         #TK_OP
            bne         @value
            lda         tkv
            cmp         #':'
            beq         @ret
@value:
            jsr         expr
            ldx         #ET_NUM
            lda         pr_func
            cmp         #2
            bne         :+
            ldx         #ET_STR
:
            cpx         etype
            beq         :+
            jmp         type_err
:
            lda         #OP_STV
            ldx         #3
            jsr         emit_op
            lda         pr_res
            ldx         #0
            jsr         emit_w
@ret:
            jsr         proc_ret
            clc
            rts

; ---- Calls

; A SUB's call statement: name [args] (CALL name [(args)]: st_call); its name current, spl its symbol's
sub_call:
            lda         spl + SPR_NUM
            sta         pr_num
            jsr         call_kinds
            jsr         lex_next
            stz         pr_paren
            jsr         call_args
            jmp         call_after

; CALL name [(args)]
st_callkw:
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syntax_err
:
            jsr         proc_find
            bcc         :+
            lda         #ER_NOSUB
            ldx         #0
            jmp         c_error
:
            lda         spl + SPR_NUM
            sta         pr_num
            jsr         call_kinds
            jsr         lex_next
            stz         pr_paren
            lda         #'('
            jsr         is_op
            bne         :+
            inc         pr_paren
            jsr         lex_next
:
            jsr         call_args
            jmp         call_after

; A FUNCTION's call in an expression: name [(args)], its name current, spl its symbol's.  Its value's type: etype
fcall_expr:
            lda         spl + SPR_FUNC
            bne         :+
            lda         #ER_TYPE                            ; (A SUB's name in an expression)
            ldx         #0
            jmp         c_error
:
            sta         pr_func
            lda         spl + SPR_NUM
            sta         pr_num
            jsr         call_kinds
            lda         pr_func                             ; (Kept: the arguments' expressions call others)
            pha
            jsr         lex_next
            stz         pr_paren
            lda         #'('
            jsr         is_op
            bne         :+
            inc         pr_paren
            jsr         lex_next
:
            jsr         call_args
            pla
            sta         pr_func
            jsr         call_after
            stz         etype                               ; (Its value's type)
            lda         pr_func
            cmp         #2
            bne         :+
            lda         #ET_STR
            sta         etype
:
            rts

; pk_buf, pk_n: the procedure's parameters' kinds (spl: its symbol), pr_func its kind
call_kinds:
            lda         spl + SPR_FUNC
            sta         pr_func
            lda         spl + SPR_NP
            sta         pk_n
            lda         spl + SPR_PK
            ldx         spl + SPR_PK + 1
            jsr         sym_ca
            ldx         #0
:
            cpx         pk_n
            beq         :+
            jsr         ca_rd
            sta         pk_buf,x
            inx
            bra         :-
:
            jmp         code_sel

; The arguments compiled (pk_buf's kinds), as many as the parameters (else Argument-count mismatch), the token after
; them current (past the ) if pr_paren).  Then CALL p.  Recursive: what's kept is on the 6502's stack
call_args:
            lda         pr_num                              ; (Kept for the call: an argument's FUNCTION uses them)
            pha
            lda         pr_func
            pha
            lda         pk_n
            pha
            lda         pr_paren
            pha
            ldx         pk_n                                ; (Its kinds, kept too)
:
            dex
            bmi         :+
            lda         pk_buf,x
            pha
            bra         :-
:
            jsr         lex_mark                            ; Pass 1: each element's place, pushed
            stz         pr_nloc
            stz         pr_argi
@place:
            jsr         args_end
            beq         @places
            ldx         pr_argi
            stz         pr_elem,x
            jsr         arg_is_elem
            bcs         @skip1
            lda         pr_argi                             ; (Its parameter takes a number or a string: by place)
            jsr         kind_of_arg
            cmp         #PK_NARR
            bcs         @skip1
            jsr         elem_place
            inc         pr_nloc
            ldx         pr_argi
            lda         pr_nloc
            sta         pr_elem,x
            bra         @next1
@skip1:
            jsr         arg_skip
@next1:
            inc         pr_argi
            lda         #','
            jsr         is_op
            bne         @places
            jsr         lex_next
            bra         @place
@places:
            jsr         lex_back                            ; Pass 2: each argument
            stz         pr_argi
@arg:
            jsr         args_end
            beq         @done
            ldx         pr_argi
            cpx         pk_n
            bcc         :+
            jmp         count_err
:
            lda         pr_elem,x                           ; (An element: its value, from its place)
            beq         :+
            jsr         elem_value
            jsr         arg_skip
            bra         @next
:
            jsr         arg_one
@next:
            inc         pr_argi
            lda         #','
            jsr         is_op
            bne         @done
            jsr         lex_next
            bra         @arg
@done:
            lda         pr_argi
            cmp         pk_n
            beq         :+
count_err:
            lda         #ER_ARGS
            ldx         #0
            jmp         c_error
:
            ldx         #0                                  ; (The kinds back)
:
            cpx         pk_n
            beq         :+
            pla
            sta         pk_buf,x
            inx
            bra         :-
:
            pla
            sta         pr_paren
            pla
            sta         pk_n
            pla
            sta         pr_func
            pla
            sta         pr_num
            lda         pr_paren
            beq         :+
            lda         #')'
            jsr         need_op
:
            lda         #OP_CALL
            ldx         #2
            jsr         emit_op
            lda         pr_num
            jmp         emit

; After CALL: each element's value written back (WBK), the arguments and places dropped (a FUNCTION's value slid
; down over them)
call_after:
            ldx         #0
@each:
            cpx         pk_n
            bcs         @drop
            lda         pr_elem,x
            beq         @next
            phx
            sta         t4                                  ; (Its place's number: 1 on)
            lda         #OP_WBK
            phx
            ldx         #3
            jsr         emit_op
            plx
            stx         t4 + 1                              ; (Its value: depth n - i (+ 1 a FUNCTION's))
            lda         pk_n
            sec
            sbc         t4 + 1
            ldy         pr_func
            beq         :+
            inc         a
:
            jsr         emit
            lda         pk_n                                ; (Its place: depth n + m - (k - 1) (+ 1))
            clc
            adc         pr_nloc
            sec
            sbc         t4
            inc         a
            ldy         pr_func
            beq         :+
            inc         a
:
            jsr         emit
            plx
@next:
            inx
            bra         @each
@drop:
            lda         pk_n
            clc
            adc         pr_nloc
            beq         @none
            pha
            ldx         #2
            lda         #OP_POPN
            ldy         pr_func
            beq         :+
            lda         #OP_SLIDE
:
            jsr         emit_op
            pla
            jmp         emit
@none:
            rts

; Is the current token the arguments' end (Z = 1)?  ) in parentheses, else the statement's end (:, ELSE, its end)
args_end:
            lda         tk
            beq         @end
            cmp         #TK_OP
            bne         @kw
            lda         tkv
            cmp         #':'
            beq         @end
            lda         pr_paren
            beq         @no
            lda         tkv
            cmp         #')'
            rts
@kw:
            cmp         #TK_KW
            bne         @no
            lda         tkv
            cmp         #K_ELSE
            rts
@no:
            lda         #1
            rts
@end:
            lda         #0
            rts

; .A = argument .A's parameter's kind (past the parameters: PK_NUM)
kind_of_arg:
            tax
            cpx         pk_n
            bcc         :+
            lda         #PK_NUM
            rts
:
            lda         pk_buf,x
            rts

; Is the argument an array's element alone (name(indices) then its end: , or ) or the statement's)?  C = 0 yes.  The
; token: its first
arg_is_elem:
            lda         tk
            cmp         #TK_NAME
            bne         @no
            ldy         #0                                  ; (Its ( after its name)
            jsr         sc_space
            lda         (lp),y
            cmp         #'('
            bne         @no
            jsr         proc_find                           ; (A FUNCTION's call isn't)
            bcc         @no
            iny
            jsr         sc_group                            ; (Past its ))
            bcs         @no
            jsr         sc_space
            jmp         sc_ends
@no:
            sec
            rts

; Is the argument a variable alone (a name, then its end)?  C = 0 yes
arg_is_var:
            lda         tk
            cmp         #TK_NAME
            bne         @no
            ldy         #0
            jsr         sc_space
            jmp         sc_ends
@no:
            sec
            rts

; .Y past spaces in the line at lp
sc_space:
            lda         (lp),y
            cmp         #' '
            bne         :+
            iny
            bra         sc_space
:
            rts

; Is (lp),y an argument's end: , ) : the line's end, ELSE?  C = 0 yes
sc_ends:
            lda         (lp),y
            beq         @yes
            cmp         #','
            beq         @yes
            cmp         #')'
            beq         @yes
            cmp         #':'
            beq         @yes
            and         #$DF                                ; (ELSE)
            cmp         #'E'
            bne         @no
            iny
            lda         (lp),y
            and         #$DF
            cmp         #'L'
            bne         @no
            iny
            lda         (lp),y
            and         #$DF
            cmp         #'S'
            bne         @no
            iny
            lda         (lp),y
            and         #$DF
            cmp         #'E'
            bne         @no
@yes:
            clc
            rts
@no:
            sec
            rts

; .Y past a group in parentheses at (lp),y (after its (: to its matching ), strings passed over).  C = 1 none
sc_group:
            lda         #1
            sta         sc_depth
@ch:
            lda         (lp),y
            beq         @none
            iny
            cmp         #'"'
            beq         @str
            cmp         #'('
            bne         :+
            inc         sc_depth
            bra         @ch
:
            cmp         #')'
            bne         @ch
            dec         sc_depth
            bne         @ch
            clc
            rts
@str:
            lda         (lp),y
            beq         @none
            iny
            cmp         #'"'
            bne         @str
            bra         @ch
@none:
            sec
            rts

; The current argument passed over (lp to its end: a , or ) at its level, or the statement's end); the token after
; it current
arg_skip:
            MOVW        lp, tstart
            ldy         #0
            stz         sc_depth
@ch:
            lda         (lp),y
            beq         @end
            cmp         #'"'
            beq         @str
            cmp         #'('
            bne         :+
            inc         sc_depth
            bra         @next
:
            cmp         #')'
            bne         :+
            lda         sc_depth
            beq         @end
            dec         sc_depth
            bra         @next
:
            ldx         sc_depth
            bne         @next
            cmp         #','
            beq         @end
            cmp         #':'
            beq         @end
            sty         t4                                  ; (ELSE ends it too)
            jsr         sc_ends
            ldy         t4
            bcc         @end
@next:
            iny
            bra         @ch
@str:
            iny
            lda         (lp),y
            beq         @end
            cmp         #'"'
            bne         @str
            bra         @next
@end:
            jsr         lp_plus_y
            jmp         lex_next

; Pass 1's place of an element (its name current): REFV, its indices, AREF n
elem_place:
            jsr         arr_lookup
            lda         #OP_REFV
            jsr         emit_slot
            jsr         lex_next
            jsr         lex_next
            jsr         emit_indices
            pha
            lda         #OP_AREF
            ldx         #2
            jsr         emit_op
            pla
            jmp         emit

; Pass 2's value of an element (pr_elem's place k): LDLOC d, d its place's depth now (m - k + 1 + the arguments
; pushed so far)
elem_value:
            ldx         pr_argi
            lda         pr_elem,x
            sta         t4
            lda         pr_nloc
            sec
            sbc         t4
            inc         a
            clc
            adc         pr_argi
            pha
            lda         #OP_LDLOC
            ldx         #2
            jsr         emit_op
            pla
            jmp         emit

; One argument, its parameter's kind's way: a variable alone by reference, an array (a()) or a record by its
; variable's, else a copy (its type checked)
arg_one:
            lda         pr_argi
            jsr         kind_of_arg
            sta         t4 + 2
            and         #7
            cmp         #PK_NARR
            bcs         @array
            cmp         #PK_REC
            bcs         @record
            jsr         arg_is_var                          ; A number or a string: by reference if it's a variable
            bcs         @value
            jsr         const_find
            bcc         @value
            jsr         proc_find                           ; (A FUNCTION's name: a call)
            bcc         @value
            jsr         fn_result                           ; (In a FUNCTION its own name: a call, not its result)
            jsr         var_lookup
            lda         scls
            jsr         class_type
            sta         etype
            jsr         arg_type
            lda         #OP_REFV
            jsr         emit_slot
            jmp         lex_next
@value:
            jsr         expr
            jmp         arg_type
@array:
            lda         tk                                  ; An array: name()
            cmp         #TK_NAME
            bne         @syntax
            jsr         arr_lookup
            lda         #OP_REFV
            jsr         emit_slot
            jsr         lex_next
            lda         #'('
            jsr         need_op
            lda         #')'
            jmp         need_op
@record:
            lda         tk                                  ; A record: its variable's reference
            cmp         #TK_NAME
            bne         @syntax
            jsr         var_lookup
            lda         scls
            cmp         #SC_REC
            bne         @param
            lda         #OP_REFV
            jsr         emit_slot
            jmp         lex_next
@param:
            lda         #ER_PARAM
            ldx         #0
            jmp         c_error
@syntax:
            jmp         syntax_err

; The argument's type (etype) must be its parameter's (t4 + 2: PK_NUM or PK_STR)
arg_type:
            lda         t4 + 2
            and         #7
            cmp         etype
            beq         :+
            jmp         type_err
:
            rts
