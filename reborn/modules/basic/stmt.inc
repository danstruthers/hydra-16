; ****************************************************************************
; stmt.inc - statements compiled (the second bank: those of control, assignment and PRINT; the rest in the third,
; stmt3.inc).  Each starts at its first token and ends with the token after it current (a : or the line's end).
;   Blocks (IF, FOR, DO, WHILE, SELECT, SUB, FUNCTION) are a stack as they're compiled (blk): each entry its kind,
; then its own (BE_*).  A jump forward to a place not known yet is a chain through the jumps' operands, each holding
; the one before (0: none), patched as the place is known (chain_add, chain_fix).

B_IF            = 1             ; The blocks' kinds
B_FOR           = 2
B_DO            = 3
B_WHILE         = 4
B_SELECT        = 5
B_SUB           = 6
B_FUNC          = 7
B_ELSE          = 8             ; (An IF after its ELSE)
; An entry: its kind (0), then: IF's next test's JF (1, 2: 0 none) and its end's chain (3, 4); FOR's variable,
; limit and step slots (1-6), its body's address (7, 8) and its exit's chain (9, 10); DO's and WHILE's top (1, 2)
; and exit's chain (3, 4); SELECT's selector slot (1, 2), its next case's JF (3, 4), its end's chain (5, 6), its
; type (7); SUB's and FUNCTION's procedure (1), its skip's operand (2, 3), its exit's chain (4, 5)

.bss
st_kw:      .res        1                                   ; (The statement's keyword ...
st_vec:     .res        2                                   ;   and its routine)
pr_last:    .res        1                                   ; (PRINT's last item: 0, or ; or ,)
ch_head:    .res        2                                   ; (chain_add's and chain_fix's)
fr_slots:   .res        6                                   ; (FOR's)
tgt_slot:   .res        2                                   ; (tgt_begin's: the target's slot, flags, fixed length,
tgt_flags:  .res        1                                   ;   type, indices (0: a variable))
tgt_fix:    .res        2
tgt_type:   .res        1
tgt_n:      .res        1
tgt_fk:     .res        1                                   ; (A record's field's first value + 1: 0 none)
sel_type:   .res        1

.segment "CODE2"

; ---- A statement

c_stmt:
            lda         tk
            cmp         #TK_KW
            beq         @kw
            cmp         #TK_NAME
            bne         :+
            jmp         st_assign_or_call
:
            jmp         syntax_err
@kw:
            lda         tkv
            sta         st_kw
            ldx         #0
@find:
            lda         st_tab,x
            beq         @other
            cmp         st_kw
            beq         @found
            inx
            inx
            inx
            bra         @find
@found:
            lda         st_tab + 1,x
            sta         st_vec
            lda         st_tab + 2,x
            sta         st_vec + 1
            jsr         lex_next                            ; (Past its keyword)
            jmp         (st_vec)
@other:
            FARN        3, c_stmt3                          ; (The third bank's: not its keyword read past)
            rts

; The second bank's statements: each its keyword and its routine
st_tab:
            .byte       K_LET
            .word       st_let
            .byte       K_PRINT
            .word       st_print
            .byte       K_IF
            .word       st_if
            .byte       K_ELSEIF
            .word       st_elseif
            .byte       K_ELSE
            .word       st_else
            .byte       K_END
            .word       st_end
            .byte       K_FOR
            .word       st_for
            .byte       K_NEXT
            .word       st_next
            .byte       K_DO
            .word       st_do
            .byte       K_LOOP
            .word       st_loop
            .byte       K_WHILE
            .word       st_while
            .byte       K_WEND
            .word       st_wend
            .byte       K_GOTO
            .word       st_goto
            .byte       K_GOSUB
            .word       st_gosub
            .byte       K_RETURN
            .word       st_return
            .byte       K_ON
            .word       st_on
            .byte       K_STOP
            .word       st_stop
            .byte       K_EXIT
            .word       st_exit
            .byte       K_SELECT
            .word       st_select
            .byte       K_CASE
            .word       st_case
            .byte       K_SUB
            .word       st_sub_kw
            .byte       K_FUNCTION
            .word       st_sub_kw
            .byte       K_CALL
            .word       st_callkw
            .byte       K_DECLARE
            .word       st_declare
            .byte       0

; ---- Chains of jumps forward

; The jump op .A emitted, its operand added to the chain at (t4) (a word: the chain's head, in the block's entry)
chain_add:
            ldx         #3
            jsr         emit_op
            jsr         cpc_get
            pha
            phx
            ldy         #0                                  ; (The operand: the old head)
            lda         (t4),y
            iny
            pha
            lda         (t4),y
            tax
            pla
            jsr         emit_w
            plx
            pla
            ldy         #0                                  ; (The head: it)
            sta         (t4),y
            iny
            txa
            sta         (t4),y
            rts

; Every jump in the chain at (t4) sent to the code's next address; the chain emptied
chain_fix:
            ldy         #0
            lda         (t4),y
            sta         ch_head
            iny
            lda         (t4),y
            sta         ch_head + 1
            lda         #0
            sta         (t4),y
            dey
            sta         (t4),y
@each:
            lda         ch_head
            ora         ch_head + 1
            beq         @done
            MOVW        t0, ch_head                         ; (Its next: the operand's word)
            lda         #LB_CODE
            jsr         ca_reg
            jsr         ca_rd
            sta         t4 + 2
            jsr         ca_rd
            sta         t4 + 3
            MOVW        t0, ch_head                         ; (This operand: patched; the next, the head)
            MOVW        ch_head, t4 + 2
            jsr         cpc_get
            jsr         patch_w
            bra         @each
@done:
            jmp         code_sel

; t4 = the place of word .A of the innermost block's entry
blk_word:
            pha
            ldx         nblk
            dex
            txa
            jsr         blk_entry
            pla
            clc
            adc         t4
            sta         t4
            bcc         :+
            inc         t4 + 1
:
            rts

; t4 = block .A's entry (0 the outermost)
blk_entry:
            ldx         #BE_SIZE
            stx         t4 + 2
            stz         t4
            stz         t4 + 1
            tax
            beq         @add
:
            lda         t4
            clc
            adc         t4 + 2
            sta         t4
            bcc         :+
            inc         t4 + 1
:
            dex
            bne         :--
@add:
            lda         t4
            clc
            adc         #<blk
            sta         t4
            lda         t4 + 1
            adc         #>blk
            sta         t4 + 1
            rts

; A block of kind .A opened (its entry cleared): t4 its entry.  Too many: Syntax error
blk_open:
            pha
            lda         nblk
            cmp         #BLK_MAX
            bcc         :+
            jmp         syntax_err
:
            jsr         blk_entry
            ldy         #BE_SIZE - 1
            lda         #0
:
            sta         (t4),y
            dey
            bpl         :-
            pla
            sta         (t4)
            ldy         #BE_LINE                            ; (Its line: an error's, if it's left open)
            lda         cline
            sta         (t4),y
            iny
            lda         cline + 1
            sta         (t4),y
            inc         nblk
            rts

; The innermost block: its kind (.A, 0 none), t4 its entry
blk_top:
            lda         nblk
            beq         @none
            dec         a
            jsr         blk_entry
            lda         (t4)
            rts
@none:
            lda         #0
            rts

; The innermost block must be of kind .A (else the error .X), and is closed: t4 its entry
blk_close:
            sta         t0
            stx         t0 + 1
            jsr         blk_top
            cmp         t0
            beq         :+
            lda         t0 + 1
            ldx         #0
            jmp         c_error
:
            dec         nblk
            rts

; ---- Assignment, and a SUB's call

st_let:
            lda         tk
            cmp         #TK_NAME
            beq         st_assign
            jmp         syntax_err

; A name first: an assignment if = follows it (or its indices or fields), else a SUB's call
st_assign_or_call:
            jsr         proc_find
            bcs         st_assign
            lda         spl + SPR_FUNC                      ; (A FUNCTION's name: its value set, in it)
            bne         st_assign
            jmp         sub_call                            ; (procs.inc)

; name [(indices)] [.field] = expression
st_assign:
            jsr         const_find                          ; (A constant: not assignable)
            bcs         :+
            lda         #ER_DUP
            ldx         #0
            jmp         c_error
:
            ldy         #0                                  ; An array's element?
:
            lda         (lp),y
            cmp         #' '
            bne         :+
            iny
            bra         :-
:
            cmp         #'('
            bne         @lb900
            jmp         @elem
@lb900:
            jsr         has_dot                             ; (rec.field = ...: the third bank's)
            bcs         :+
            FARN        3, a_field
            bcs         :+
            rts
:
            jsr         fn_result                           ; (A FUNCTION's own name, in it: its result's slot)
            bcc         :+
            jsr         var_lookup
:
            lda         scls                                ; (A record: copied whole)
            cmp         #SC_REC
            bne         :+
            FARN        3, a_record
            rts
:
            lda         spl + SP_SLOT                       ; (Its slot, its class, kept on the 6502's stack)
            pha
            lda         spl + SP_SLOT + 1
            pha
            lda         spl + SP_FLAGS
            pha
            lda         spl + SP_INFO
            pha
            lda         spl + SP_INFO + 1
            pha
            lda         scls
            pha
            jsr         lex_next
            lda         #'='
            jsr         need_op
            jsr         expr
            pla
            jsr         class_type                          ; (Its type: the value's must be it)
            cmp         etype
            beq         :+
            jmp         type_err
:
            pla                                             ; (A fixed-length string: padded or cut)
            sta         t4 + 1
            pla
            sta         t4
            pla
            and         #SF_FIXED
            beq         :+
            lda         #OP_FIXLEN
            ldx         #3
            jsr         emit_op
            lda         t4
            ldx         t4 + 1
            jsr         emit_w
:
            pla
            sta         spl + SP_SLOT + 1
            pla
            sta         spl + SP_SLOT
            lda         #OP_STV
            jmp         emit_slot
@elem:
            jsr         arr_lookup                          ; name(indices) = ...: its reference, indices, value
            lda         scls
            pha
            lda         spl + SP_TYPE                       ; (An array of records': its type)
            pha
            lda         #OP_REFV
            jsr         emit_slot
            jsr         lex_next
            jsr         lex_next
            jsr         emit_indices
            pha
            lda         #'.'                                ; (arr(i).field = ...: the third bank's)
            jsr         is_op
            bne         :+
            pla
            plx
            ply
            FARN        3, a_efield
            rts
:
            tsx                                             ; (A record, the element whole: copied)
            lda         $0103,x
            cmp         #SC_RARR
            bne         :+
            pla
            plx
            ply
            FARN        3, a_erecord
            rts
:
            pla                                             ; (The count over the class: the type dropped)
            plx
            pha
            lda         #'='
            jsr         need_op
            jsr         expr
            pla
            tay
            lda         etype                               ; (The value's type, against the element's)
            sta         t0
            pla
            sta         scls
            jsr         arr_etype
            lda         etype
            cmp         t0
            beq         :+
            jmp         type_err
:
            lda         #OP_APUT
            ldx         #2
            jsr         emit_op
            tya
            jmp         emit

; ---- A value's target (READ's, INPUT's ...): tgt_begin reads it (a variable, or an array's element) and emits what
; comes before its value (an element's array and indices); the caller emits the value; tgt_end emits its store.
; tgt_type its type (ET_*)

tgt_begin:
            stz         tgt_fk
            lda         tk
            cmp         #TK_NAME
            beq         :+
            lda         #ER_VARREQ
            ldx         #0
            jmp         c_error
:
            jsr         const_find                          ; (A constant: not assignable)
            bcs         :+
            lda         #ER_DUP
            ldx         #0
            jmp         c_error
:
            ldy         #0                                  ; (An element?)
:
            lda         (lp),y
            cmp         #' '
            bne         :+
            iny
            bra         :-
:
            cmp         #'('
            beq         @elem
            jsr         has_dot                             ; (rec.field: the third bank's)
            bcs         :+
            FARN        3, t_field
            bcs         :+
            rts
:
            jsr         fn_result
            bcc         :+
            jsr         var_lookup
:
            lda         spl + SP_SLOT
            sta         tgt_slot
            lda         spl + SP_SLOT + 1
            sta         tgt_slot + 1
            lda         spl + SP_FLAGS
            sta         tgt_flags
            lda         spl + SP_INFO
            sta         tgt_fix
            lda         spl + SP_INFO + 1
            sta         tgt_fix + 1
            lda         scls
            jsr         class_type
            sta         tgt_type
            stz         tgt_n
            jmp         lex_next
@elem:
            jsr         arr_lookup
            jsr         arr_etype
            lda         etype
            sta         tgt_type
            stz         tgt_flags
            lda         spl + SP_TYPE                       ; (An array of records': its type)
            pha
            lda         #OP_REFV
            jsr         emit_slot
            jsr         lex_next
            jsr         lex_next
            jsr         emit_indices
            sta         tgt_n
            lda         #'.'                                ; (arr(i).field: the third bank's)
            jsr         is_op
            beq         :+
            pla
            rts
:
            plx
            lda         tgt_n
            FARN        3, t_efield
            rts

; The target's store emitted (its value emitted after tgt_begin)
tgt_end:
            lda         tgt_flags                           ; (A fixed-length string: padded or cut)
            and         #SF_FIXED
            beq         :+
            lda         #OP_FIXLEN
            ldx         #3
            jsr         emit_op
            lda         tgt_fix
            ldx         tgt_fix + 1
            jsr         emit_w
:
            ldx         tgt_fk                              ; (A record's field: FPUT k, APUTF n k)
            beq         @plain
            dex
            phx
            lda         tgt_n
            bne         @efld
            lda         #OP_FPUT
            ldx         #2
            jsr         emit_op
            pla
            jmp         emit
@efld:
            lda         #OP_APUTF
            ldx         #3
            jsr         emit_op
            lda         tgt_n
            jsr         emit
            pla
            jmp         emit
@plain:
            lda         tgt_n
            bne         @elem
            lda         #OP_STV
            ldx         #3
            jsr         emit_op
            lda         tgt_slot
            ldx         tgt_slot + 1
            jmp         emit_w
@elem:
            lda         #OP_APUT
            ldx         #2
            jsr         emit_op
            lda         tgt_n
            jmp         emit

; A variable's or an element's place pushed (a VT_REF, a VT_LOC: SWAP's), its type tgt_type
place_emit:
            jsr         tgt_begin
            lda         tgt_fk                              ; (A record's field: not a place, SWAP's)
            beq         :+
            jmp         type_err
:
            lda         tgt_n
            beq         :+
            lda         #OP_AREF
            ldx         #2
            jsr         emit_op
            lda         tgt_n
            jmp         emit
:
            lda         #OP_REFV
            ldx         #3
            jsr         emit_op
            lda         tgt_slot
            ldx         tgt_slot + 1
            jmp         emit_w

; The type (ET_*) of a variable's class .A
class_type:
            ldx         #ET_STR
            cmp         #SC_STR
            beq         :+
            cmp         #SC_SARR
            beq         :+
            ldx         #ET_REC
            cmp         #SC_REC
            beq         :+
            cmp         #SC_RARR
            beq         :+
            ldx         #ET_NUM
:
            txa
            rts


; ---- PRINT

st_print:
            jsr         is_hash                             ; PRINT #n: a file's (the third bank's)
            bne         :+
            FARN        3, print_file
            rts
:
            lda         #K_USING
            jsr         is_kw
            bne         :+
            FARN        3, print_using
            rts
:
            stz         pr_last
print_items:
@item:
            lda         tk                                  ; (The statement's end)
            beq         @end
            cmp         #TK_OP
            bne         @expr
            lda         tkv
            cmp         #':'
            beq         @end
            cmp         #';'
            bne         :+
            sta         pr_last
            jsr         lex_next
            bra         @item
:
            cmp         #','
            bne         @expr
            sta         pr_last
            lda         #ST_PRZONE
            jsr         emit_st
            jsr         lex_next
            bra         @item
@expr:
            lda         #K_ELSE                             ; (A one-line IF's ELSE ends it)
            jsr         is_kw
            beq         @end
            lda         #K_TAB
            jsr         is_kw
            beq         @tab
            lda         #K_SPC
            jsr         is_kw
            beq         @spc
            stz         pr_last
            jsr         expr
            lda         etype
            cmp         #ET_REC
            bne         :+
            jmp         type_err
:
            lda         #ST_PRN
            jsr         emit_st
            bra         @item
@tab:
            lda         #ST_PRTAB
            bra         :+
@spc:
            lda         #ST_PRSPC
:
            pha
            stz         pr_last
            jsr         lex_next
            lda         #'('
            jsr         need_op
            jsr         expr_num
            lda         #')'
            jsr         need_op
            pla
            jsr         emit_st
            lda         #';'                                ; (Nothing between it and what follows)
            sta         pr_last
            jmp         @item
@end:
            lda         pr_last
            bne         :+
            lda         #ST_PRNL
            jsr         emit_st
:
            rts

; The op ST of statement routine .A emitted
emit_st:
            pha
            lda         #OP_ST
            ldx         #2
            jsr         emit_op
            pla
            jmp         emit

; ---- IF

st_if:
            jsr         expr_num
            lda         #K_GOTO                             ; IF c GOTO label
            jsr         is_kw
            bne         :+
            jsr         lex_next
            jmp         if_goto
:
            lda         #K_THEN
            jsr         need_kw
            lda         tk                                  ; IF c THEN alone: a block
            beq         @block
            cmp         #TK_NUM                             ; IF c THEN 100: GOTO 100
            bne         @line
            jmp         if_goto
@block:
            lda         #B_IF
            jsr         blk_open
            lda         #OP_JF
            jsr         emit_fwd                            ; (Its JF: the next test's, at word 1)
            lda         #1
            jsr         blk_word
            lda         t0
            sta         (t4)
            ldy         #1
            lda         t0 + 1
            sta         (t4),y
            rts
@line:
            lda         #OP_JF                              ; IF c THEN statements [ELSE statements]
            jsr         emit_fwd
            lda         t0 + 1
            pha
            lda         t0
            pha
            jsr         stmts_line
            lda         #K_ELSE
            jsr         is_kw
            beq         @else
            pla                                             ; (No ELSE: the JF here)
            sta         t0
            pla
            sta         t0 + 1
            jsr         cpc_get
            jmp         patch_w
@else:
            jsr         lex_next
            jsr         lt_add                              ; (Past the ELSE's statements: RESUME NEXT's too)
            lda         #OP_JMP
            jsr         emit_fwd
            pla
            sta         t4
            pla
            sta         t4 + 1
            lda         t0
            pha
            lda         t0 + 1
            pha
            MOVW        t0, t4
            jsr         cpc_get
            jsr         patch_w
            lda         tk                                  ; (ELSE 100: GOTO 100)
            cmp         #TK_NUM
            bne         :+
            jsr         goto_num
            bra         :++
:
            jsr         stmts_line
:
            pla
            sta         t0 + 1
            pla
            sta         t0
            jsr         cpc_get
            jmp         patch_w

; IF c GOTO label, IF c THEN 100: a JT to it
if_goto:
            lda         #OP_JT
            ldx         #3
            jsr         emit_op
            jmp         label_operand

; A one-line IF's statements: to ELSE or the line's end (a : between them)
stmts_line:
@stmt:
            lda         tk
            beq         @done
            lda         #K_ELSE
            jsr         is_kw
            beq         @done
            jsr         lt_add                              ; (Each a statement of the line table's)
            jsr         c_stmt
            lda         #':'
            jsr         is_op
            bne         @done
            jsr         lex_next
            bra         @stmt
@done:
            rts

; ELSEIF c THEN (in an IF block)
st_elseif:
            jsr         blk_top
            cmp         #B_IF
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            jsr         if_next                             ; (The last part's end: a JMP to END IF; its JF here)
            jsr         expr_num
            lda         #K_THEN
            jsr         need_kw
            lda         #OP_JF
            jsr         emit_fwd
            lda         #1
            jsr         blk_word
            lda         t0
            sta         (t4)
            ldy         #1
            lda         t0 + 1
            sta         (t4),y
            rts

; The IF's part so far ended: a JMP to its END IF (its chain), and its JF (if any) sent here
if_next:
            lda         #3
            jsr         blk_word
            lda         #OP_JMP
            jsr         chain_add
            lda         #1                                  ; (Its JF: here)
            jsr         blk_word
            lda         (t4)
            sta         t0
            ldy         #1
            lda         (t4),y
            sta         t0 + 1
            ora         t0
            beq         :+
            lda         #0
            sta         (t4)
            sta         (t4),y
            jsr         cpc_get
            jsr         patch_w
:
            rts

; ELSE (in an IF block; a one-line IF's is its own)
st_else:
            jsr         blk_top
            cmp         #B_IF
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            jsr         if_next
            jsr         blk_top
            lda         #B_ELSE
            sta         (t4)
            rts

; END IF, END SELECT, END SUB, END FUNCTION, END TYPE (the third's); END alone: the program's end
st_end:
            lda         #K_IF
            jsr         is_kw
            bne         @select
            jsr         lex_next
            jsr         blk_top                             ; (IF or ELSE)
            cmp         #B_IF
            beq         :+
            cmp         #B_ELSE
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            lda         #1                                  ; (Its last JF here, its chain here)
            jsr         blk_word
            lda         (t4)
            sta         t0
            ldy         #1
            lda         (t4),y
            sta         t0 + 1
            ora         t0
            beq         :+
            jsr         cpc_get
            jsr         patch_w
:
            lda         #3
            jsr         blk_word
            jsr         chain_fix
            dec         nblk
            rts
@select:
            lda         #K_SELECT
            jsr         is_kw
            bne         @other
            jsr         lex_next
            jmp         select_end
@other:
            lda         tk
            cmp         #TK_KW
            bne         @end
            lda         tkv
            cmp         #K_SUB
            beq         :+
            cmp         #K_FUNCTION
            bne         :++
:
            jmp         end_sub
:
            FARN        3, end_proc                         ; (END TYPE)
            rts
@end:
            lda         tk                                  ; END n: its status
            beq         :+
            cmp         #TK_OP
            beq         :+
            jsr         expr_num
            lda         #ST_ENDN
            jmp         emit_st
:
            lda         #OP_END
            ldx         #1
            jmp         emit_op

st_stop:
            lda         #OP_STOP
            ldx         #1
            jmp         emit_op

; ---- FOR and NEXT

; FOR v = a TO b [STEP s]: v = a, the limit and the step in slots of their own; FORT; the body ...; NEXT's FORN
st_for:
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syntax_err
:
            jsr         var_lookup
            lda         scls
            cmp         #SC_NUM
            beq         :+
            jmp         type_err
:
            lda         spl + SP_SLOT
            sta         fr_slots
            lda         spl + SP_SLOT + 1
            sta         fr_slots + 1
            jsr         lex_next
            lda         #'='
            jsr         need_op
            jsr         expr_num
            lda         fr_slots
            sta         spl + SP_SLOT
            lda         fr_slots + 1
            sta         spl + SP_SLOT + 1
            lda         #OP_STV
            jsr         emit_slot
            lda         #K_TO
            jsr         need_kw
            jsr         hidden_slot                         ; (The limit's slot)
            sta         fr_slots + 2
            stx         fr_slots + 3
            jsr         expr_num
            lda         #OP_STV
            ldx         #3
            jsr         emit_op
            lda         fr_slots + 2
            ldx         fr_slots + 3
            jsr         emit_w
            jsr         hidden_slot                         ; (The step's)
            sta         fr_slots + 4
            stx         fr_slots + 5
            lda         #K_STEP
            jsr         is_kw
            bne         :+
            jsr         lex_next
            jsr         expr_num
            bra         :++
:
            lda         #OP_PUSHI8
            ldx         #2
            jsr         emit_op
            lda         #1
            jsr         emit
:
            lda         #OP_STV
            ldx         #3
            jsr         emit_op
            lda         fr_slots + 4
            ldx         fr_slots + 5
            jsr         emit_w
            lda         #B_FOR                              ; The block: its slots
            jsr         blk_open
            ldy         #1
:
            lda         fr_slots - 1,y
            sta         (t4),y
            iny
            cpy         #7
            bne         :-
            lda         #OP_FORT                            ; FORT v l s exit
            ldx         #9
            jsr         emit_op
            jsr         for_slots
            jsr         cpc_get                             ; (Its exit: on the chain)
            pha
            phx
            lda         #0
            tax
            jsr         emit_w
            lda         #9
            jsr         blk_word
            plx
            pla
            sta         (t4)
            ldy         #1
            txa
            sta         (t4),y
            jsr         cpc_get                             ; (The body's start)
            pha
            phx
            lda         #7
            jsr         blk_word
            ldy         #1
            pla
            sta         (t4),y
            pla
            sta         (t4)
            rts

; A hidden slot (FOR's limit and step, SELECT's selector): a local in a procedure, else a global.  OUT: .A/.X
hidden_slot:
            lda         cscope
            beq         :+
            ldy         #LK_NUM
            jsr         local_new
            ldx         #0
            rts
:
            ldy         #LK_NUM
            jmp         global_new

; The innermost FOR's three slots emitted (its entry at t4)
for_slots:
            ldy         #1
:
            lda         (t4),y
            jsr         emit
            iny
            cpy         #7
            bne         :-
            rts

; NEXT [v [, w ...]]: the innermost FOR closed (its variable v's, if it's named)
st_next:
@each:
            jsr         blk_top
            cmp         #B_FOR
            beq         :+
            lda         #ER_NEXT
            ldx         #0
            jmp         c_error
:
            lda         tk                                  ; (Its variable named: the FOR's?)
            cmp         #TK_NAME
            bne         @close
            jsr         var_lookup
            jsr         blk_top
            ldy         #1
            lda         (t4),y
            cmp         spl + SP_SLOT
            bne         @wrong
            iny
            lda         (t4),y
            cmp         spl + SP_SLOT + 1
            bne         @wrong
            jsr         lex_next
@close:
            jsr         blk_top
            lda         #OP_FORN                            ; FORN v l s body
            ldx         #9
            jsr         emit_op
            jsr         for_slots
            ldy         #7
            lda         (t4),y
            pha
            iny
            lda         (t4),y
            tax
            pla
            jsr         emit_w
            lda         #9                                  ; (Its exits: here)
            jsr         blk_word
            jsr         chain_fix
            dec         nblk
            lda         #','                                ; (NEXT i, j: the next one too)
            jsr         is_op
            bne         :+
            jsr         lex_next
            bra         @each
:
            rts
@wrong:
            lda         #ER_NEXT
            ldx         #0
            jmp         c_error

; ---- DO, LOOP, WHILE, WEND

; DO [WHILE c | UNTIL c]
st_do:
            lda         #B_DO
            jsr         blk_open
            jsr         cpc_get                             ; (Its top)
            ldy         #1
            sta         (t4),y
            iny
            txa
            sta         (t4),y
            lda         #K_WHILE
            jsr         is_kw
            bne         :+
            jsr         lex_next
            jsr         expr_num
            lda         #3
            jsr         blk_word
            lda         #OP_JF
            jmp         chain_add
:
            lda         #K_UNTIL
            jsr         is_kw
            bne         :+
            jsr         lex_next
            jsr         expr_num
            lda         #3
            jsr         blk_word
            lda         #OP_JT
            jmp         chain_add
:
            rts

; LOOP [WHILE c | UNTIL c]
st_loop:
            jsr         blk_top
            cmp         #B_DO
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            ldy         #1                                  ; (Its top)
            lda         (t4),y
            sta         t4 + 2
            iny
            lda         (t4),y
            sta         t4 + 3
            lda         #K_WHILE
            jsr         is_kw
            bne         :+
            jsr         lex_next
            jsr         expr_num
            lda         #OP_JT
            bra         @back
:
            lda         #K_UNTIL
            jsr         is_kw
            bne         :+
            jsr         lex_next
            jsr         expr_num
            lda         #OP_JF
            bra         @back
:
            lda         #OP_JMP
@back:
            pha
            MOVW        t4, t4 + 2
            pla
            jsr         emit_jmp
            lda         #3
            jsr         blk_word
            jsr         chain_fix
            dec         nblk
            rts

; WHILE c
st_while:
            lda         #B_WHILE
            jsr         blk_open
            jsr         cpc_get
            ldy         #1
            sta         (t4),y
            iny
            txa
            sta         (t4),y
            jsr         expr_num
            lda         #3
            jsr         blk_word
            lda         #OP_JF
            jmp         chain_add

st_wend:
            jsr         blk_top
            cmp         #B_WHILE
            beq         :+
            lda         #ER_WEND
            ldx         #0
            jmp         c_error
:
            ldy         #1
            lda         (t4),y
            sta         t4 + 2
            iny
            lda         (t4),y
            sta         t4 + 3
            MOVW        t4, t4 + 2
            lda         #OP_JMP
            jsr         emit_jmp
            lda         #3
            jsr         blk_word
            jsr         chain_fix
            dec         nblk
            rts

; ---- EXIT FOR, DO, SUB, FUNCTION: a JMP on the exit's chain of the innermost of its kind
st_exit:
            lda         tk
            cmp         #TK_KW
            beq         :+
            jmp         syntax_err
:
            lda         tkv
            ldx         #B_FOR
            ldy         #9
            cmp         #K_FOR
            beq         @have
            ldx         #B_DO
            ldy         #3
            cmp         #K_DO
            beq         @have
            ldx         #B_SUB
            ldy         #4
            cmp         #K_SUB
            beq         @have
            ldx         #B_FUNC
            cmp         #K_FUNCTION
            beq         @have
            jmp         syntax_err
@have:
            stx         t0
            sty         t0 + 1
            jsr         lex_next
            lda         nblk                                ; (The innermost of the kind)
@find:
            beq         @none
            dec         a
            pha
            jsr         blk_entry
            pla
            tax
            lda         (t4)
            cmp         t0
            beq         @found
            txa
            bra         @find
@found:
            lda         t4                                  ; (Its exit's chain)
            clc
            adc         t0 + 1
            sta         t4
            bcc         :+
            inc         t4 + 1
:
            lda         #OP_JMP
            jmp         chain_add
@none:
            lda         #ER_BLOCK                           ; (EXIT SUB outside one: Not in a procedure)
            ldx         t0
            cpx         #B_SUB
            beq         :+
            cpx         #B_FUNC
            bne         :++
:
            lda         #ER_INSUB
:
            ldx         #0
            jmp         c_error

; ---- GOTO, GOSUB, RETURN, ON

st_goto:
            lda         #OP_JMP
            ldx         #3
            jsr         emit_op
            jmp         label_operand

; GOTO a line number (ELSE 100)
goto_num:
            lda         #OP_JMP
            ldx         #3
            jsr         emit_op
            ; (fall through)

; A label's (or a line number's) code address emitted, the token after it current
label_operand:
            lda         tk
            cmp         #TK_NUM
            bne         :+
            jsr         num_label
            bra         @have
:
            cmp         #TK_NAME
            beq         @have
            jmp         syntax_err
@have:
            jsr         label_ref
            jmp         lex_next

st_gosub:
            lda         #OP_GOSUB
            ldx         #3
            jsr         emit_op
            jmp         label_operand

; RETURN [label]; in a FUNCTION, RETURN value (procs)
st_return:
            jsr         fn_return_check                     ; (A FUNCTION's RETURN value: the third bank's)
            bcc         @done
            lda         tk
            beq         @plain
            cmp         #TK_OP
            beq         @plain
            lda         #OP_RETL
            ldx         #3
            jsr         emit_op
            jmp         label_operand
@plain:
            lda         #OP_RETURN
            ldx         #1
            jmp         emit_op
@done:
            rts

; In a FUNCTION: RETURN [value] compiled (the third bank's).  OUT: C = 0 it was
fn_return_check:
            lda         cscope
            beq         :+
            jmp         fn_return
:
            sec
            rts

; ON n GOTO a, b, c (ON ERROR: the third bank's)
st_on:
            lda         #K_ERROR
            jsr         is_kw
            bne         :+
            FARN        3, on_error
            rts
:
            jsr         expr_num
            lda         #K_GOTO
            jsr         is_kw
            bne         :+
            lda         #OP_ONGO
            bra         @op
:
            lda         #K_GOSUB
            jsr         is_kw
            beq         :+
            jmp         syntax_err
:
            lda         #OP_ONSUB
@op:
            pha
            jsr         lex_next
            jsr         lex_mark                            ; (The labels counted first: then emitted)
            ldy         #1
@count:
            phy
            lda         tk
            cmp         #TK_NUM
            beq         :+
            cmp         #TK_NAME
            beq         :+
            jmp         syntax_err
:
            jsr         lex_next
            ply
            lda         #','
            jsr         is_op
            bne         :+
            iny
            phy
            jsr         lex_next
            ply
            bra         @count
:
            pla
            phy
            pha
            tya
            asl         a
            clc
            adc         #2
            tax
            pla
            jsr         emit_op
            pla
            jsr         emit
            jsr         lex_back
@each:
            jsr         label_operand
            lda         #','
            jsr         is_op
            bne         :+
            jsr         lex_next
            bra         @each
:
            rts

; ---- SELECT CASE

; SELECT CASE x: x in a hidden slot
st_select:
            lda         #K_CASE
            jsr         need_kw
            jsr         expr
            lda         etype
            cmp         #ET_REC
            bne         :+
            jmp         type_err
:
            sta         sel_type
            jsr         hidden_slot
            sta         fr_slots
            stx         fr_slots + 1
            lda         #OP_STV
            ldx         #3
            jsr         emit_op
            lda         fr_slots
            ldx         fr_slots + 1
            jsr         emit_w
            lda         #B_SELECT
            jsr         blk_open
            ldy         #1
            lda         fr_slots
            sta         (t4),y
            iny
            lda         fr_slots + 1
            sta         (t4),y
            ldy         #7
            lda         sel_type
            sta         (t4),y
            rts

; CASE items, or CASE ELSE: the case before it ended (a JMP to the end), its test's JF sent here
st_case:
            jsr         blk_top
            cmp         #B_SELECT
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            ldy         #7
            lda         (t4),y
            sta         sel_type
            ldy         #4                                  ; (A case before this: a JMP to the end)
            lda         (t4),y
            dey
            ora         (t4),y
            ldy         #8
            ora         (t4),y
            beq         @first
            lda         #5
            jsr         blk_word
            lda         #OP_JMP
            jsr         chain_add
            jsr         case_jf_here
@first:
            jsr         blk_top
            ldy         #8                                  ; (A case seen)
            lda         #1
            sta         (t4),y
            lda         #K_ELSE
            jsr         is_kw
            bne         @items
            jmp         lex_next
@items:
            jsr         case_item
@more:
            lda         #','
            jsr         is_op
            bne         @jf
            jsr         lex_next
            jsr         case_item
            lda         #OP_OR
            ldx         #1
            jsr         emit_op
            bra         @more
@jf:
            lda         #OP_JF                              ; (Not this case: on to the next's test)
            jsr         emit_fwd
            lda         #3
            jsr         blk_word
            lda         t0
            sta         (t4)
            ldy         #1
            lda         t0 + 1
            sta         (t4),y
            rts

; The last case's JF (if any) sent here
case_jf_here:
            lda         #3
            jsr         blk_word
            lda         (t4)
            sta         t0
            ldy         #1
            lda         (t4),y
            sta         t0 + 1
            ora         t0
            beq         :+
            lda         #0
            sta         (t4)
            sta         (t4),y
            jsr         cpc_get
            jsr         patch_w
:
            rts

; One case's test, its truth pushed: x, IS rel x, or x TO y
case_item:
            lda         #K_IS
            jsr         is_kw
            bne         @value
            jsr         lex_next
            lda         tk                                  ; (Its comparison)
            cmp         #TK_OP
            beq         :+
            jmp         syntax_err
:
            ldx         #rel_n - 1
            lda         tkv
:
            cmp         rel_ch,x
            beq         :+
            dex
            bpl         :-
            jmp         syntax_err
:
            lda         rel_op,x
            pha
            jsr         lex_next
            jsr         sel_load
            jsr         sel_expr
            pla
            jmp         sel_op
@value:
            jsr         sel_load
            jsr         sel_expr
            lda         #K_TO
            jsr         is_kw
            beq         @range
            lda         #OP_EQ
            jmp         sel_op
@range:
            lda         #OP_GE
            jsr         sel_op
            jsr         lex_next
            jsr         sel_load
            jsr         sel_expr
            lda         #OP_LE
            jsr         sel_op
            lda         #OP_AND
            ldx         #1
            jmp         emit_op

; The selector's value pushed
sel_load:
            lda         #OP_LDV
            ldx         #3
            jsr         emit_op
            jsr         blk_top
            ldy         #1
            lda         (t4),y
            pha
            iny
            lda         (t4),y
            tax
            pla
            jmp         emit_w

; An expression of the selector's type
sel_expr:
            jsr         expr
            lda         etype
            cmp         sel_type
            beq         :+
            jmp         type_err
:
            rts

; The comparison op .A emitted, a string's if the selector is one
sel_op:
            ldx         sel_type
            beq         :+
            clc
            adc         #OP_SEQ - OP_EQ
:
            ldx         #1
            jmp         emit_op

; END SELECT: the last case's JF and every case's JMP here
select_end:
            jsr         blk_top
            cmp         #B_SELECT
            beq         :+
            lda         #ER_BLOCK
            ldx         #0
            jmp         c_error
:
            jsr         case_jf_here
            lda         #5
            jsr         blk_word
            jsr         chain_fix
            dec         nblk
            rts
