; ****************************************************************************
; run.inc - the interpreter (the first bank): the compiled code's ops, each a byte and its operands, run from the code
; region by ip (the code's bank at $8000, cbank), on the value stack (vsp: 5-byte values, in the task's RAM) and the
; frame of the procedure running (frp: its parameters, its frame's mark, its locals: slots 0, 1 ... from frp).  An op
; never crosses a bank's end: the compiler puts BNK there, and the code goes on at the next bank's start.
;   A slot operand (v16) is a frame's slot below 256 (a by-reference parameter's VT_REF followed to its slot), else a
; global's address in the RAM.  A code address (a16) is a logical bank's 3 bits and 13.


.bss
fr_sign:    .res        1                                   ; (FOR's: the step's sign, the variable's slot)
fr_v:       .res        2
ar_n:       .res        1                                   ; (The arrays': indices, the slot, the element's index and
ar_slot:    .res        2                                   ;   offset, the values an element has, a dimension's lowest
ar_idx:     .res        2                                   ;   and count, the value stored)
ar_off:     .res        2
ar_esize:   .res        1
ar_lo:      .res        2
ar_cnt:     .res        2
ar_val:     .res        2
rf_val:     .res        5                                   ; (RETF's: the result; RET's: the return)
rf_ret:     .res        2
sw_v:       .res        2 * VAL_SIZE                        ; (SWAP's: the two values)
sw_dir:     .res        1

.enum
OP_END                                                      ; The end (the program's, a line's at the prompt)
OP_BNK                                                      ; On at the next bank's start
OP_JMP                                                      ; a16
OP_JMPB                                                     ; a16, backwards: Ctrl-C looked for
OP_JF                                                       ; a16: a value taken; to a16 if it's 0
OP_JT                                                       ; a16: if it isn't
OP_GOSUB                                                    ; a16
OP_RETURN
OP_RETL                                                     ; a16 (RETURN label)
OP_ONGO                                                     ; n, a16 * n: a value taken, to its a16 (1 to n)
OP_ONSUB
OP_STOP
OP_CALL                                                     ; p (a procedure's number): its arguments on the stack
OP_RET                                                      ; A SUB's end: its frame dropped, its arguments left
OP_RETF                                                     ; A FUNCTION's end: its value put after its arguments
OP_POPN                                                     ; n: n values dropped
OP_SLIDE                                                    ; n: the top value down over the n below it
OP_WBK                                                      ; d, l: the value d below the top put at the place
                                                            ;   (VT_LOC) l below it
OP_FORT                                                     ; v16 l16 s16 a16: FOR's first test (to a16 if done)
OP_FORN                                                     ; v16 l16 s16 a16: NEXT (v + s, to a16 if not done)
OP_PUSHI8                                                   ; b: an integer -128 to 127
OP_PUSHI32                                                  ; b4
OP_PUSHN                                                    ; n16, bytes: a number in the stored format, in the code
OP_PUSHS                                                    ; n16, bytes: a string in the code
OP_PUSHE                                                    ; ""
OP_PUSHNH                                                   ; As PUSHN and PUSHS, the value a copy in the heap (the
OP_PUSHSH                                                   ;   prompt's line: its code goes)
OP_LDV                                                      ; v16: a slot's value pushed
OP_STV                                                      ; v16: a value taken, put in a slot
OP_REFV                                                     ; v16: the slot's reference pushed (VT_REF)
OP_DUP
OP_DROP
OP_AGET                                                     ; n: n indices and an array's VT_REF taken, its element
OP_APUT                                                     ; n: a value, n indices and the VT_REF: stored
OP_AREF                                                     ; n: an element's place (VT_LOC)
OP_LDLOC                                                    ; The value at the place on top, pushed after it
OP_FGET                                                     ; k: a record's VT_REF taken, its field k pushed
OP_FPUT                                                     ; k: a value and the VT_REF: stored
OP_AGETF                                                    ; n, k: an element's field
OP_APUTF                                                    ; n, k
OP_ADD                                                      ; The numbers' operators: two values taken, one given
OP_SUB
OP_MUL
OP_DIV
OP_IDIV
OP_MOD
OP_POW
OP_NEG                                                      ; (One)
OP_EQ                                                       ; Comparisons: -1 or 0
OP_NE
OP_LT
OP_GT
OP_LE
OP_GE
OP_SEQ                                                      ; The strings'
OP_SNE
OP_SLT
OP_SGT
OP_SLE
OP_SGE
OP_NOT                                                      ; The bits'
OP_AND
OP_OR
OP_XOR
OP_EQV
OP_IMP
OP_CAT                                                      ; Two strings joined
OP_FIXLEN                                                   ; n16: a string padded or cut to n
OP_FN                                                       ; f: a function, its arguments on the stack
OP_ST                                                       ; s: a statement's routine
OP_SWAP                                                     ; The values at two places (VT_REF, VT_LOC) exchanged
OP_ONERR                                                    ; a16: ON ERROR GOTO's handler ($FFFF: none)
OP_RESUME                                                   ; n [a16]: 0 the statement again, 1 the next, 2 a16
OP_COUNT
.endenum

.code

; The interpreter's start: the code at .A/.X (a code address) run, the value stack as it is, till END (C = 0) or
; STOP (C = 1).  An error that no handler takes doesn't come back here (err_entry)
run_code:
            jsr         ip_set
            tsx
            stx         run_s
            jmp         next

; ip = the code address .A/.X (its bank selected)
ip_set:
            sta         ip
            txa
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            sta         ipl
            txa
            and         #$1F
            ora         #WIN
            sta         ip + 1
            ldx         ipl
            lda         ltab,x
            sta         cbank
            sta         RAM_BANK
            rts

; .A/.X = ip's code address (as ip_set takes it)
ip_get:
            lda         ipl
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            sta         t0
            lda         ip + 1
            and         #$1F
            ora         t0
            tax
            lda         ip
            rts

; ---- The loop

; ip on past n bytes (the op's), then the next op
nx1:
            inc         ip
            bne         next
            inc         ip + 1
            bra         next
nx2:
            lda         #2
            bra         nxa
nx3:
            lda         #3
nxa:
            clc
            adc         ip
            sta         ip
            bcc         next
            inc         ip + 1
next:
            lda         (ip)
            asl         a
            tax
            jmp         (op_tab,x)

op_tab:
            .word       op_end, op_bnk, op_jmp, op_jmpb, op_jf, op_jt, op_gosub, op_return, op_retl, op_ongo, op_onsub
            .word       op_stop, op_call, op_ret, op_retf, op_popn, op_slide, op_wbk, op_fort, op_forn
            .word       op_pushi8, op_pushi32, op_pushn, op_pushs, op_pushe, op_pushnh, op_pushsh
            .word       op_ldv, op_stv, op_refv, op_dup, op_drop
            .word       op_aget, op_aput, op_aref, op_ldloc, op_fget, op_fput, op_agetf, op_aputf
            .word       op_add, op_sub, op_mul, op_div, op_idiv, op_mod, op_pow, op_neg
            .word       op_eq, op_ne, op_lt, op_gt, op_le, op_ge
            .word       op_seq, op_sne, op_slt, op_sgt, op_sle, op_sge
            .word       op_not, op_and, op_or, op_xor, op_eqv, op_imp
            .word       op_cat, op_fixlen, op_fn, op_st, op_swap, op_onerr, op_resume
.assert     * - op_tab = OP_COUNT * 2, error, "op_tab and the ops' enum differ"

; ---- The end, STOP, the banks

op_end:
            clc
            bra         :+
op_stop:
            jsr         ip_get                              ; (Where it stopped: CONT's)
            sta         cont_ip
            stx         cont_ip + 1
            sec
:
            ldx         run_s
            txs
            rts

op_bnk:
            inc         ipl
            ldx         ipl
            lda         ltab,x
            sta         cbank
            sta         RAM_BANK
            stz         ip
            lda         #WIN
            sta         ip + 1
            jmp         next

; ---- Jumps

; ip = the a16 after the op
jump:
            ldy         #2
            lda         (ip),y
            tax
            dey
            lda         (ip),y
            jsr         ip_set
            jmp         next

op_jmp:
            bra         jump

op_jmpb:
            bit         intr
            bmi         brk_here
            bra         jump

; Ctrl-C at an op: the program stopped there (CONT runs the op again)
brk_here:
            lda         #$80
            trb         intr
            lda         #ER_BREAK
            ldx         #0
            jmp         err_entry

; A value taken: is it 0?  OUT: Z = 1 yes
pop_zero:
            jsr         pop_vt
            lda         (vt)
            bne         @slow                               ; (Not an integer: the numbers')
            ldy         #4
            lda         (vt),y
            dey
            ora         (vt),y
            dey
            ora         (vt),y
            dey
            ora         (vt),y
            rts
@slow:
            FARN        4, num_is_zero
            rts

op_jf:
            jsr         pop_zero
            beq         jump
            jmp         nx3
op_jt:
            jsr         pop_zero
            bne         jump
            jmp         nx3

; ---- GOSUB, RETURN, ON

op_gosub:
            jsr         stack_room
            jsr         ip_get                              ; (Its return: past its a16)
            clc
            adc         #3
            bcc         :+
            inx
:
            sta         t0
            stx         t0 + 1
            ldy         #0
            lda         #VT_GOS
            sta         (vsp),y
            iny
            lda         t0
            sta         (vsp),y
            iny
            lda         t0 + 1
            sta         (vsp),y
            jsr         vsp_up
            jmp         jump

; The return GOSUB left, taken from the stack (Return without GOSUB if it isn't there): .A/.X
pop_gos:
            jsr         pop_vt
            lda         (vt)
            cmp         #VT_GOS
            beq         :+
            jsr         vsp_up                              ; (Left as it was)
            lda         #ER_RETURN
            ldx         #0
            jmp         err_entry
:
            ldy         #2
            lda         (vt),y
            tax
            dey
            lda         (vt),y
            rts

op_return:
            jsr         pop_gos
            jsr         ip_set
            bit         intr
            bmi         :+
            jmp         next
:
            jmp         brk_here

op_retl:
            jsr         pop_gos
            jmp         jump

; ON n GOTO / GOSUB: n taken; its a16 (1 to the count), else on past them
op_onsub:
            sec
            bra         :+
op_ongo:
            clc
:
            php
            jsr         pop_int16                           ; (t0: n)
            ldy         #1
            lda         (ip),y                              ; (The count)
            sta         t4
            plp
            php
            lda         t0 + 1
            bne         @past
            lda         t0
            beq         @past
            cmp         t4
            beq         :+
            bcs         @past
:
            asl         a                                   ; (Its a16: at 2 + 2(n - 1) = 2n)
            tay
            lda         (ip),y
            sta         t4 + 1
            iny
            lda         (ip),y
            sta         t4 + 2
            plp
            bcc         @go
            lda         t4                                  ; (GOSUB's return: past the list)
            asl         a
            sec
            adc         #1
            jsr         ip_plus
            jsr         stack_room
            jsr         ip_get
            sta         t0
            stx         t0 + 1
            ldy         #0
            lda         #VT_GOS
            sta         (vsp),y
            iny
            lda         t0
            sta         (vsp),y
            iny
            lda         t0 + 1
            sta         (vsp),y
            jsr         vsp_up
@go:
            lda         t4 + 1
            ldx         t4 + 2
            jsr         ip_set
            jmp         next
@past:
            plp
            lda         t4
            asl         a
            sec
            adc         #1
            jsr         ip_plus
            jmp         next

; ip += .A
ip_plus:
            clc
            adc         ip
            sta         ip
            bcc         :+
            inc         ip + 1
:
            rts

; ---- The value stack

; Room on the value stack for a frame's or a GOSUB's (a value and STACK_SPARE's more), else Out of stack space
stack_room:
            lda         vsp + 1
            cmp         stk_lim + 1
            bcc         :+
            bne         @out
            lda         vsp
            cmp         stk_lim
            bcs         @out
:
            rts
@out:
            lda         #ER_STACK
            ldx         #0
            jmp         err_entry

; ---- Pushes

; The value at (vu) pushed (5 bytes).  Keeps .X
push_vu:
            ldy         #4
:
            lda         (vu),y
            sta         (vsp),y
            dey
            bpl         :-
            jmp         vsp_up

op_pushi8:
            ldy         #1
            lda         (ip),y
            ldy         #1
            sta         (vsp),y
            ldx         #0                                  ; (Its sign, in the rest)
            cmp         #$80
            bcc         :+
            dex
:
            txa
            iny
            sta         (vsp),y
            iny
            sta         (vsp),y
            iny
            sta         (vsp),y
            lda         #VT_INT
            sta         (vsp)
            jsr         vsp_up
            jmp         nx2

op_pushi32:
            ldy         #4
:
            lda         (ip),y
            sta         (vsp),y
            dey
            bne         :-
            lda         #VT_INT
            sta         (vsp)
            jsr         vsp_up
            lda         #5
            jsr         ip_plus
            jmp         next

; A value of tag .A whose far address is ip's + t0 (its length's two bytes, then its bytes): the code's own
push_code:
            sta         (vsp)
            ldy         #1
            lda         ip
            clc
            adc         t0
            sta         (vsp),y
            iny
            lda         ip + 1
            adc         #0
            sta         (vsp),y
            iny
            lda         ipl
            sta         (vsp),y
            jmp         vsp_up

op_pushn:
            lda         #VT_NUM
            bra         push_lenb
op_pushs:
            lda         #VT_STR
push_lenb:
            ldx         #1
            stx         t0
            jsr         push_code
            ldy         #1                                  ; (Past it: 3 + n16)
            lda         (ip),y
            clc
            adc         #3
            sta         t0
            iny
            lda         (ip),y
            adc         #0
            sta         t0 + 1
            lda         t0
            clc
            adc         ip
            sta         ip
            lda         t0 + 1
            adc         ip + 1
            sta         ip + 1
            jmp         next

op_pushe:
            lda         #VT_STR
            sta         (vsp)
            ldy         #3
            lda         #NOBANK
            sta         (vsp),y
            jsr         vsp_up
            jmp         nx1

op_pushnh:
            FARN        5, push_num_heap
            jmp         next
op_pushsh:
            FARN        5, push_str_heap
            jmp         next

; ---- Slots

; vt = the slot of the v16 at ip + 1 (a frame's slot followed to its own, if it's a reference)
slot_vt:
            ldy         #2
            lda         (ip),y
            bne         slot_abs
            dey
            lda         (ip),y
slot_x:
            tax
            clc
            lda         frp
            adc         mul5_lo,x
            sta         vt
            lda         frp + 1
            adc         mul5_hi,x
            sta         vt + 1
            lda         (vt)
            cmp         #VT_REF
            bne         :+
            ldy         #1
            lda         (vt),y
            tax
            iny
            lda         (vt),y
            sta         vt + 1
            stx         vt
:
            rts
slot_abs:
            sta         vt + 1
            dey
            lda         (ip),y
            sta         vt
            rts

op_ldv:
            jsr         slot_vt
            ldy         #4
:
            lda         (vt),y
            sta         (vsp),y
            dey
            bpl         :-
            jsr         vsp_up
            jmp         nx3

op_stv:
            jsr         slot_vt
            jsr         pop_vu
            ldy         #4
:
            lda         (vu),y
            sta         (vt),y
            dey
            bpl         :-
            jmp         nx3

op_refv:
            jsr         slot_vt
            lda         #VT_REF
            sta         (vsp)
            ldy         #1
            lda         vt
            sta         (vsp),y
            iny
            lda         vt + 1
            sta         (vsp),y
            jsr         vsp_up
            jmp         nx3

op_dup:
            jsr         top_vt
            ldy         #4
:
            lda         (vt),y
            sta         (vsp),y
            dey
            bpl         :-
            jsr         vsp_up
            jmp         nx1

op_drop:
            jsr         pop_vt
            jmp         nx1

op_popn:
            ldy         #1
            lda         (ip),y
            tax
            lda         vsp
            sec
            sbc         mul5_lo,x
            sta         vsp
            lda         vsp + 1
            sbc         mul5_hi,x
            sta         vsp + 1
            jmp         nx2

; SLIDE n: the top value put n below it, those between dropped
op_slide:
            jsr         pop_vu
            ldy         #1
            lda         (ip),y
            tax
            lda         vsp
            sec
            sbc         mul5_lo,x
            sta         vsp
            sta         vt
            lda         vsp + 1
            sbc         mul5_hi,x
            sta         vsp + 1
            sta         vt + 1
            ldy         #4
:
            lda         (vu),y
            sta         (vt),y
            dey
            bpl         :-
            jsr         vsp_up
            jmp         nx2

; ---- Numbers: integers here, the rest the numbers' (num.inc, the fourth bank)

; The numbers' way for the op at ip: its two values (or one) worked by the numbers library, the result in their place
num_slow:
            lda         (ip)
            FARN        4, num_op
            jmp         nx1

op_add:
            jsr         two_vals
            bcs         num_slow
            clc
            ldy         #1
            lda         (vt),y
            adc         (vu),y
            sta         t0
            iny
            lda         (vt),y
            adc         (vu),y
            sta         t0 + 1
            iny
            lda         (vt),y
            adc         (vu),y
            sta         t0 + 2
            iny
            lda         (vt),y
            adc         (vu),y
            bvs         num_slow
            sta         t0 + 3
int_t0:
            ldy         #4                                  ; (vt = t0, the top dropped)
:
            lda         t0 - 1,y
            sta         (vt),y
            dey
            bne         :-
            lda         vu
            sta         vsp
            lda         vu + 1
            sta         vsp + 1
            jmp         nx1

op_sub:
            jsr         two_vals
            bcs         num_slow
            sec
            ldy         #1
            lda         (vt),y
            sbc         (vu),y
            sta         t0
            iny
            lda         (vt),y
            sbc         (vu),y
            sta         t0 + 1
            iny
            lda         (vt),y
            sbc         (vu),y
            sta         t0 + 2
            iny
            lda         (vt),y
            sbc         (vu),y
            bvs         num_slow
            sta         t0 + 3
            bra         int_t0

; MUL: two integers each of 16 bits (signed), their product here; else the numbers'
op_mul:
            jsr         two_vals
            bcs         num_slow
            jsr         small2
            bcc         @lb900
            jmp         num_slow
@lb900:
            FARN        4, mul16
            jmp         nx1

; Are vt's and vu's integers both of 16 bits, signed?  C = 0 yes
small2:
            ldx         vt
            ldy         vt + 1
            jsr         small1
            bcs         :+
            ldx         vu
            ldy         vu + 1
            jmp         small1
:
            rts
small1:
            stx         t4
            sty         t4 + 1
            ldy         #2
            lda         (t4),y
            asl         a                                   ; (Bit 15 into C)
            ldy         #3
            lda         (t4),y
            adc         #0                                  ; (0 + C: 0 or 1; $FF + C: $FF or 0)
            bne         @no
            iny
            lda         (t4),y
            dey
            cmp         (t4),y
            bne         @no
            clc
            rts
@no:
            sec
            rts

op_div:
op_idiv:
op_mod:
op_pow:
            jmp         num_slow

op_neg:
            jsr         top_vt
            lda         (vt)
            bne         @slow
            sec
            ldy         #1
            lda         #0
            sbc         (vt),y
            sta         t0
            iny
            lda         #0
            sbc         (vt),y
            sta         t0 + 1
            iny
            lda         #0
            sbc         (vt),y
            sta         t0 + 2
            iny
            lda         #0
            sbc         (vt),y
            bvs         @slow
            sta         (vt),y
            dey
            lda         t0 + 2
            sta         (vt),y
            dey
            lda         t0 + 1
            sta         (vt),y
            dey
            lda         t0
            sta         (vt),y
            jmp         nx1
@slow:
            jmp         num_slow

; Comparisons: two integers here (their order: t4 = $FF, 0, 1); else the numbers' CMP
cmp_ints:
            jsr         two_vals
            bcs         @slow
            sec                                             ; (vt - vu, signed)
            ldy         #1
            lda         (vt),y
            sbc         (vu),y
            sta         t0
            iny
            lda         (vt),y
            sbc         (vu),y
            ora         t0
            sta         t0
            iny
            lda         (vt),y
            sbc         (vu),y
            ora         t0
            sta         t0
            iny
            lda         (vt),y
            sbc         (vu),y
            bvc         :+
            eor         #$80
:
            bmi         @below
            ora         t0
            beq         @equal
            lda         #1
            bra         @have
@below:
            lda         #$FF
            bra         @have
@equal:
            lda         #0
@have:
            sta         t4
            rts
@slow:
            FARN        4, num_cmp2                         ; (t4: the order)
            rts

; The comparison's truth (C = 1 true) put in the second value's place (-1 or 0), the top dropped
cmp_put:
            lda         #0
            sbc         #0                                  ; (C = 1: 0; C = 0: $FF)
            eor         #$FF
            sta         t0
            sta         t0 + 1
            sta         t0 + 2
            sta         t0 + 3
            jsr         two_vals                            ; (vt, vu again: the slow ways' banks may change them)
            lda         #VT_INT
            sta         (vt)
            jmp         int_t0

; Each comparison: the order (t4: $FF, 0, 1), then its test
op_eq:
            jsr         cmp_ints
is_eq:
            lda         t4
            beq         is_true
is_false:
            clc
            jmp         cmp_put
is_true:
            sec
            jmp         cmp_put
op_ne:
            jsr         cmp_ints
is_ne:
            lda         t4
            bne         is_true
            bra         is_false
op_lt:
            jsr         cmp_ints
is_lt:
            lda         t4
            cmp         #$FF
            beq         is_true
            bra         is_false
op_gt:
            jsr         cmp_ints
is_gt:
            lda         t4
            cmp         #1
            beq         is_true
            bra         is_false
op_le:
            jsr         cmp_ints
is_le:
            lda         t4
            cmp         #1
            bne         is_true
            bra         is_false
op_ge:
            jsr         cmp_ints
is_ge:
            lda         t4
            cmp         #$FF
            bne         is_true
            bra         is_false

; The strings' comparisons (str.inc: t4 the order), then as the numbers'
str_cmp:
            jsr         two_vals
            FARN        5, str_order
            rts
op_seq:
            jsr         str_cmp
            bra         is_eq
op_sne:
            jsr         str_cmp
            bra         is_ne
op_slt:
            jsr         str_cmp
            bra         is_lt
op_sgt:
            jsr         str_cmp
            bra         is_gt
op_sle:
            jsr         str_cmp
            bra         is_le
op_sge:
            jsr         str_cmp
            bra         is_ge

; The bits' operators: integers here, and the rest the numbers'
op_not:
            jsr         top_vt
            lda         (vt)
            bne         :+
            ldy         #4
@l:
            lda         (vt),y
            eor         #$FF
            sta         (vt),y
            dey
            bne         @l
            jmp         nx1
:
            jmp         num_slow

op_and:
            jsr         two_vals
            bcs         :+
            ldy         #4
@l:
            lda         (vt),y
            and         (vu),y
            sta         t0 - 1,y
            dey
            bne         @l
            jmp         int_t0
:
            jmp         num_slow

op_or:
            jsr         two_vals
            bcs         :+
            ldy         #4
@l:
            lda         (vt),y
            ora         (vu),y
            sta         t0 - 1,y
            dey
            bne         @l
            jmp         int_t0
:
            jmp         num_slow

op_xor:
            jsr         two_vals
            bcs         :+
            ldy         #4
@l:
            lda         (vt),y
            eor         (vu),y
            sta         t0 - 1,y
            dey
            bne         @l
            jmp         int_t0
:
            jmp         num_slow

op_eqv:
            jsr         two_vals
            bcs         :+
            ldy         #4
@l:
            lda         (vt),y
            eor         (vu),y
            eor         #$FF
            sta         t0 - 1,y
            dey
            bne         @l
            jmp         int_t0
:
            jmp         num_slow

op_imp:
            jsr         two_vals
            bcs         :+
            ldy         #4
@l:
            lda         (vt),y
            eor         #$FF
            ora         (vu),y
            sta         t0 - 1,y
            dey
            bne         @l
            jmp         int_t0
:
            jmp         num_slow

; ---- Strings (str.inc, the fifth bank)

op_cat:
            FARN        5, str_cat
            jmp         nx1

op_fixlen:
            FARN        5, str_fixlen
            jmp         nx3

; ---- Functions and statements: fn.inc's dispatch to their banks (each routine leaves ip as it was)
op_fn:
            ldy         #1
            lda         (ip),y
            jsr         fn_call
            jmp         nx2

op_st:
            ldy         #1
            lda         (ip),y
            jsr         st_call
            jmp         nx2

; ---- FOR and NEXT: FORT v l s a16 (its first test), FORN v l s a16 (v + s, then the test), each slot a v16

; vt = the slot whose v16 is at ip + .Y (its low byte), a frame's followed if it holds a reference
slot_at:
            iny
            lda         (ip),y
            bne         @abs
            dey
            lda         (ip),y
            jmp         slot_x
@abs:
            sta         vt + 1
            dey
            lda         (ip),y
            sta         vt
            rts

; C = 1 if the loop is done: v past l, as the step's sign says (above it for a step of 0 or more, below it for one
; below 0)
for_test:
            ldy         #5                                  ; (The step's sign)
            jsr         slot_at
            lda         (vt)
            bne         @ssign
            ldy         #4
            lda         (vt),y
            bra         :+
@ssign:
            FARN        4, num_sign_vt                      ; (.A: $FF, 0, 1)
:
            sta         fr_sign
            ldy         #3                                  ; (vu: the limit, vt: the variable)
            jsr         slot_at
            MOVW        vu, vt
            ldy         #1
            jsr         slot_at
            lda         (vt)
            ora         (vu)
            bne         @slow
            jsr         cmp_vv
            bra         :+
@slow:
            FARN        4, num_cmp2
:
            lda         fr_sign
            bmi         @down
            lda         t4                                  ; (Up: done if v > l)
            cmp         #1
            beq         @done
            clc
            rts
@down:
            lda         t4                                  ; (Down: done if v < l)
            cmp         #$FF
            beq         @done
            clc
            rts
@done:
            sec
            rts

op_fort:
            jsr         for_test
            bcc         :+
            ldy         #7
            jmp         jump_y
:
            lda         #9
            jsr         ip_plus
            jmp         next

op_forn:
            ldy         #1                                  ; v + s, into v
            jsr         slot_at
            MOVW        fr_v, vt
            ldy         #5
            jsr         slot_at
            MOVW        vu, vt
            MOVW        vt, fr_v
            lda         (vt)
            ora         (vu)
            bne         @slow
            clc
            ldy         #1
            lda         (vt),y
            adc         (vu),y
            sta         t0
            iny
            lda         (vt),y
            adc         (vu),y
            sta         t0 + 1
            iny
            lda         (vt),y
            adc         (vu),y
            sta         t0 + 2
            iny
            lda         (vt),y
            adc         (vu),y
            bvs         @slow
            sta         (vt),y
            dey
            lda         t0 + 2
            sta         (vt),y
            dey
            lda         t0 + 1
            sta         (vt),y
            dey
            lda         t0
            sta         (vt),y
            bra         @test
@slow:
            jsr         push_vt                             ; (v and s pushed, added, the sum taken back to v)
            MOVW        vt, vu
            jsr         push_vt
            lda         #OP_ADD
            FARN        4, num_op
            jsr         pop_vu
            MOVW        vt, fr_v
            ldy         #4
:
            lda         (vu),y
            sta         (vt),y
            dey
            bpl         :-
@test:
            jsr         for_test
            bcs         :+
            bit         intr                                ; (Back to the body: Ctrl-C looked for)
            bmi         @brk
            ldy         #7
            jmp         jump_y
:
            lda         #9
            jsr         ip_plus
            jmp         next
@brk:
            jmp         brk_here

; The value at (vt) pushed
push_vt:
            ldy         #4
:
            lda         (vt),y
            sta         (vsp),y
            dey
            bpl         :-
            jmp         vsp_up

; ip = the a16 at ip + .Y (its low byte), then the next op
jump_y:
            iny
            lda         (ip),y
            tax
            dey
            lda         (ip),y
            jsr         ip_set
            jmp         next

; t4 = the order of the integers at (vt) and (vu): $FF, 0, 1
cmp_vv:
            sec
            ldy         #1
            lda         (vt),y
            sbc         (vu),y
            sta         t0
            iny
            lda         (vt),y
            sbc         (vu),y
            ora         t0
            sta         t0
            iny
            lda         (vt),y
            sbc         (vu),y
            ora         t0
            sta         t0
            iny
            lda         (vt),y
            sbc         (vu),y
            bvc         :+
            eor         #$80
:
            bmi         @below
            ora         t0
            beq         @equal
            lda         #1
            bra         @have
@below:
            lda         #$FF
            bra         @have
@equal:
            lda         #0
@have:
            sta         t4
            rts

; ---- Arrays: AGET n (n indices and an array's VT_REF: its element), APUT n (and a value: stored), AREF n (its place)

; Cursor A at the element (its bank selected); vt the reference's place on the stack (the result's).  n (ip + 1) its
; indices, each from the array's lowest in its dimension (else Subscript out of range); an array not made yet made
; now (0 to 10 in each dimension: OPTION BASE's lowest)
arr_elem:
            jsr         arr_index
            lda         ar_slot                             ; (The array's far address)
            sta         vu
            lda         ar_slot + 1
            sta         vu + 1
            ldy         #1
            lda         (vu),y
            sta         ca
            iny
            lda         (vu),y
            sta         ca + 1
            iny
            lda         (vu),y
            sta         cal
            lda         ar_off                              ; (Past its header and to the element: ar_off bytes)
            ldx         ar_off + 1
            jsr         ca_plus
            jmp         ca_sel

; ar_slot = the array's slot, ar_idx its element's index, ar_off the element's offset in its block (its dimensions
; read past, at 5 * esize * index); vt the reference's place on the stack
arr_index:
            ldy         #1
            lda         (ip),y
            sta         ar_n
            inc         a                                   ; (vt = vsp - 5 (n + 1): the reference)
            tax
            sec
            lda         vsp
            sbc         mul5_lo,x
            sta         vt
            lda         vsp + 1
            sbc         mul5_hi,x
            sta         vt + 1
            ldy         #1                                  ; (Its slot)
            lda         (vt),y
            sta         ar_slot
            sta         vu
            iny
            lda         (vt),y
            sta         ar_slot + 1
            sta         vu + 1
            ldy         #3                                  ; (Not made yet: made now)
            lda         (vu),y
            cmp         #NOBANK
            bne         :+
            lda         ar_n
            FARN        5, arr_auto
:
            ldy         #1                                  ; (Cursor A at its dimensions)
            lda         (vu),y
            sta         ca
            iny
            lda         (vu),y
            sta         ca + 1
            iny
            lda         (vu),y
            sta         cal
            jsr         ca_sel
            jsr         ca_rd                               ; (Its dimensions: as many as the indices)
            cmp         ar_n
            beq         :+
            jmp         subscript
:
            jsr         ca_rd                               ; (Each element's values)
            sta         ar_esize
            stz         ar_idx
            stz         ar_idx + 1
            ldx         #1                                  ; (Each dimension: its index, from the stack)
@dim:
            phx
            jsr         ca_rd                               ; (Its lowest, its count)
            sta         ar_lo
            jsr         ca_rd
            sta         ar_lo + 1
            jsr         ca_rd
            sta         ar_cnt
            jsr         ca_rd
            sta         ar_cnt + 1
            plx
            phx
            jsr         arr_sub                             ; (Index x's: t0 = it less the lowest, checked)
            lda         ar_idx                              ; (ar_idx = ar_idx * count + t0)
            ldx         ar_idx + 1
            jsr         mul_cnt
            clc
            adc         t0
            sta         ar_idx
            txa
            adc         t0 + 1
            sta         ar_idx + 1
            plx
            inx
            cpx         ar_n
            bcc         @dim
            beq         @dim
            lda         ar_idx                              ; (Its offset: 2 + 4n + 5 * esize * index)
            ldx         ar_idx + 1
            ldy         ar_esize
            cpy         #1
            beq         :+
            jsr         mul_esize
:
            sta         t0
            stx         t0 + 1
            asl         t0                                  ; (* 5: * 4 + itself)
            rol         t0 + 1
            asl         t0
            rol         t0 + 1
            clc
            adc         t0
            sta         ar_off
            txa
            adc         t0 + 1
            sta         ar_off + 1
            lda         ar_n
            asl         a
            asl         a
            clc
            adc         #2
            adc         ar_off
            sta         ar_off
            bcc         :+
            inc         ar_off + 1
:
            jmp         code_sel

; t0 = index .X's value (the stack: vt + 5x), less ar_lo; past the count: Subscript out of range
arr_sub:
            txa
            tay                                             ; (vu = vt + 5x)
            clc
            lda         vt
            adc         mul5_lo,y
            sta         vu
            lda         vt + 1
            adc         mul5_hi,y
            sta         vu + 1
            lda         (vu)                                ; (Not an integer: rounded, the numbers')
            beq         :+
            lda         vt
            pha
            lda         vt + 1
            pha
            lda         ca                                  ; (Cursor A kept: it's reading the dimensions)
            pha
            lda         ca + 1
            pha
            lda         cal
            pha
            MOVW        vt, vu
            FARN        4, num_to_int
            pla
            sta         cal
            pla
            sta         ca + 1
            pla
            sta         ca
            jsr         ca_sel
            pla
            sta         vt + 1
            pla
            sta         vt
:
            ldy         #4                                  ; (16 bits: the top two bytes its sign's)
            lda         (vu),y
            dey
            cmp         (vu),y
            bne         subscript
            ldy         #1
            sec
            lda         (vu),y
            sbc         ar_lo
            sta         t0
            iny
            lda         (vu),y
            sbc         ar_lo + 1
            sta         t0 + 1
            ldy         #3                                  ; (Its sign, and the borrow: below the lowest?)
            lda         (vu),y
            sbc         #0
            bmi         subscript
            lda         t0                                  ; (Below the count)
            cmp         ar_cnt
            lda         t0 + 1
            sbc         ar_cnt + 1
            bcs         subscript
            rts
subscript:
            jsr         code_sel
            lda         #ER_SUBSCRIPT
            ldx         #0
            jmp         err_entry

; .A/.X = .A/.X * ar_cnt (16 bits)
mul_cnt:
            sta         t4
            stx         t4 + 1
            lda         ar_cnt
            sta         t4 + 2
            lda         ar_cnt + 1
            sta         t4 + 3
            bra         mul16_t4
; .A/.X = .A/.X * .Y
mul_esize:
            sta         t4
            stx         t4 + 1
            sty         t4 + 2
            stz         t4 + 3
mul16_t4:
            lda         #0                                  ; (t4 * t4 + 2, the low 16 bits)
            tax
            ldy         #16
@bit:
            asl         a
            pha
            txa
            rol         a
            tax
            pla
            asl         t4
            rol         t4 + 1
            bcc         :+
            clc
            adc         t4 + 2
            pha
            txa
            adc         t4 + 3
            tax
            pla
:
            dey
            bne         @bit
            rts

; Cursor A on by .A/.X bytes (into the banks after, past the window's end)
ca_plus:
            clc
            adc         ca
            sta         ca
            txa
            adc         ca + 1
@wrap:
            cmp         #WIN_END
            bcc         :+
            sbc         #WIN_END - WIN
            inc         cal
            bra         @wrap
:
            sta         ca + 1
            rts

op_aget:
            jsr         arr_elem
            ldy         #0
:
            jsr         ca_rd
            sta         (vt),y
            iny
            cpy         #VAL_SIZE
            bne         :-
            jsr         code_sel
            clc
            lda         vt
            adc         #VAL_SIZE
            sta         vsp
            lda         vt + 1
            adc         #0
            sta         vsp + 1
            jmp         nx2

op_aput:
            jsr         pop_vu
            MOVW        ar_val, vu
            jsr         arr_elem
            MOVW        vu, ar_val
            ldy         #0
:
            lda         (vu),y
            jsr         ca_wr
            iny
            cpy         #VAL_SIZE
            bne         :-
            jsr         code_sel
            MOVW        vsp, vt
            jmp         nx2

; AREF n: the element's place, a VT_LOC (its array's slot, its index's values from the block's first: 5 bytes each)
op_aref:
            jsr         arr_index
            lda         #VT_LOC
            sta         (vt)
            ldy         #1
            lda         ar_slot
            sta         (vt),y
            iny
            lda         ar_slot + 1
            sta         (vt),y
            iny
            lda         ar_off                              ; (Its offset: in values, past the dimensions')
            sta         (vt),y
            iny
            lda         ar_off + 1
            sta         (vt),y
            clc
            lda         vt
            adc         #VAL_SIZE
            sta         vsp
            lda         vt + 1
            adc         #0
            sta         vsp + 1
            jmp         nx2

; ---- Procedures: CALL p, RET np, RETF np res; an element's place's value (LDLOC d), its writing back (WBK dp dl)

; CALL p: its arguments pushed (its parameters); the frame's mark (its return, the caller's frp), its locals' defaults
; (by their kinds, after its code), then its code
op_call:
            ldy         #1
            lda         (ip),y
            jsr         pe_of
            ldy         #6                                  ; (Never compiled: a DECLARE alone)
            lda         (pe),y
            bne         :+
            lda         #ER_NOSUB
            ldx         #0
            jmp         err_entry
:
            bit         intr
            bpl         :+
            jmp         brk_here
:
            ldy         #3                                  ; (Room: the mark and its locals, and the spare)
            lda         (pe),y
            inc         a
            tax
            clc
            lda         vsp
            adc         mul5_lo,x
            sta         t0
            lda         vsp + 1
            adc         mul5_hi,x
            cmp         stk_lim + 1
            bcc         :+
            bne         @deep
            lda         t0
            cmp         stk_lim
            bcc         :+
@deep:
            lda         #ER_STACK
            ldx         #0
            jmp         err_entry
:
            jsr         ip_get                              ; (Its return: past the op)
            clc
            adc         #2
            bcc         :+
            inx
:
            sta         t0
            stx         t0 + 1
            lda         #VT_FRM
            sta         (vsp)
            ldy         #1
            lda         t0
            sta         (vsp),y
            iny
            lda         t0 + 1
            sta         (vsp),y
            iny
            lda         frp
            sta         (vsp),y
            iny
            lda         frp + 1
            sta         (vsp),y
            ldy         #2                                  ; (frp: its first parameter, np below the mark)
            lda         (pe),y
            tax
            sec
            lda         vsp
            sbc         mul5_lo,x
            sta         frp
            lda         vsp + 1
            sbc         mul5_hi,x
            sta         frp + 1
            jsr         vsp_up
            ldy         #4                                  ; (Its locals' kinds: in the code, after its RET)
            lda         (pe),y
            pha
            iny
            lda         (pe),y
            tax
            pla
            jsr         ip_set
            ldy         #3
            lda         (pe),y
            sta         t4
            ldy         #0
@local:
            cpy         t4
            beq         @code
            lda         (ip),y
            jsr         push_default
            iny
            bra         @local
@code:
            ldy         #0                                  ; (Its code)
            lda         (pe),y
            pha
            iny
            lda         (pe),y
            tax
            pla
            jsr         ip_set
            jmp         next

; The default value of kind .A (LK_*) pushed.  Keeps .Y
push_default:
            phy
            tax
            lda         dk_tag,x
            sta         (vsp)
            ldy         #1
            lda         dk_b1,x
            sta         (vsp),y
            iny
            lda         #0
            sta         (vsp),y
            iny
            lda         dk_bank,x
            sta         (vsp),y
            iny
            lda         #0
            sta         (vsp),y
            jsr         vsp_up
            ply
            rts
dk_tag:     .byte       VT_INT, VT_STR, VT_ARR, VT_REC, VT_ARR
dk_bank:    .byte       0, NOBANK, NOBANK, NOBANK, NOBANK
dk_b1:      .byte       0, 0, 0, 0, 1

; RET np: the frame dropped but its parameters (the caller's WBK reads them), back to the caller
op_ret:
            jsr         ret_frame
            jmp         ret_go

; RETF np res: as RET, the result (slot res) pushed after the parameters
op_retf:
            ldy         #2
            lda         (ip),y
            tax
            clc
            lda         frp
            adc         mul5_lo,x
            sta         vt
            lda         frp + 1
            adc         mul5_hi,x
            sta         vt + 1
            ldy         #4
:
            lda         (vt),y
            sta         rf_val,y
            dey
            bpl         :-
            jsr         ret_frame
            ldy         #4
:
            lda         rf_val,y
            sta         (vsp),y
            dey
            bpl         :-
            jsr         vsp_up
            ; (fall through)
ret_go:
            lda         rf_ret
            ldx         rf_ret + 1
            jsr         ip_set
            bit         intr
            bmi         :+
            jmp         next
:
            jmp         brk_here

; The frame (np at ip + 1) dropped: vsp past its parameters, frp the caller's; rf_ret its return
ret_frame:
            ldy         #1
            lda         (ip),y
            tax
            clc
            lda         frp
            adc         mul5_lo,x
            sta         vt
            sta         vsp
            lda         frp + 1
            adc         mul5_hi,x
            sta         vt + 1
            sta         vsp + 1
            ldy         #1
            lda         (vt),y
            sta         rf_ret
            iny
            lda         (vt),y
            sta         rf_ret + 1
            iny
            lda         (vt),y
            sta         frp
            iny
            lda         (vt),y
            sta         frp + 1
            rts

; Cursor A at the place (a VT_LOC) at (vu): its variable's array or record, the offset in it.  C = 1 none (the variable
; holds none now)
loc_ca:
            ldy         #1
            lda         (vu),y
            sta         t0
            iny
            lda         (vu),y
            sta         t0 + 1
            ldy         #3
            lda         (t0),y
            cmp         #NOBANK
            beq         @none
            sta         cal
            dey
            lda         (t0),y
            sta         ca + 1
            dey
            lda         (t0),y
            sta         ca
            ldy         #4
            lda         (vu),y
            tax
            dey
            lda         (vu),y
            jsr         ca_plus
            jsr         ca_sel
            clc
            rts
@none:
            sec
            rts

; vu = the value .A values below the top (1: the top)
vu_depth:
            tax
            sec
            lda         vsp
            sbc         mul5_lo,x
            sta         vu
            lda         vsp + 1
            sbc         mul5_hi,x
            sta         vu + 1
            rts

; LDLOC d: the value at the place d below the top, pushed
op_ldloc:
            ldy         #1
            lda         (ip),y
            jsr         vu_depth
            jsr         loc_ca
            bcs         @none
            ldy         #0
:
            jsr         ca_rd
            sta         (vsp),y
            iny
            cpy         #VAL_SIZE
            bne         :-
            jsr         code_sel
            jsr         vsp_up
            jmp         nx2
@none:
            lda         #ER_SUBSCRIPT
            ldx         #0
            jmp         err_entry

; WBK dp dl: the value dp below the top put at the place dl below it
op_wbk:
            ldy         #1
            lda         (ip),y
            jsr         vu_depth
            MOVW        vt, vu
            ldy         #2
            lda         (ip),y
            jsr         vu_depth
            jsr         loc_ca
            bcs         :++
            ldy         #0
:
            lda         (vt),y
            jsr         ca_wr
            iny
            cpy         #VAL_SIZE
            bne         :-
            jsr         code_sel
:
            jmp         nx3

; SWAP: the values at the two places on top (VT_REF, VT_LOC) exchanged, the places taken off
op_swap:
            lda         #2
            jsr         vu_depth
            ldx         #0
            jsr         place_in
            lda         #1
            jsr         vu_depth
            ldx         #VAL_SIZE
            jsr         place_in
            ldx         #0                                  ; (The first's value to the second's place ...
            jsr         place_out
            lda         #2                                  ;   and the second's to the first's)
            jsr         vu_depth
            ldx         #VAL_SIZE
            jsr         place_out
            jsr         pop_vt
            jsr         pop_vt
            jmp         nx1

; The value at the place at (vu) (a VT_REF or a VT_LOC) copied to sw_v + .X (place_in), or from there (place_out)
place_in:
            stz         sw_dir
            bra         place_io
place_out:
            lda         #1
            sta         sw_dir
place_io:
            lda         (vu)
            cmp         #VT_LOC
            beq         @loc
            ldy         #1                                  ; (A slot: its address)
            lda         (vu),y
            sta         t0
            iny
            lda         (vu),y
            sta         t0 + 1
            ldy         #0
@ram:
            lda         sw_dir
            bne         :+
            lda         (t0),y
            sta         sw_v,x
            bra         :++
:
            lda         sw_v,x
            sta         (t0),y
:
            inx
            iny
            cpy         #VAL_SIZE
            bne         @ram
            rts
@loc:
            phx
            jsr         loc_ca
            plx
            bcs         @none
            ldy         #VAL_SIZE
@far:
            lda         sw_dir
            bne         :+
            jsr         ca_rd
            sta         sw_v,x
            bra         :++
:
            lda         sw_v,x
            jsr         ca_wr
:
            inx
            dey
            bne         @far
            jmp         code_sel
@none:
            lda         #ER_SUBSCRIPT
            ldx         #0
            jmp         err_entry

; pe = procs' entry of the procedure .A
pe_of:
            stz         pe + 1
            asl         a
            rol         pe + 1
            asl         a
            rol         pe + 1
            asl         a
            rol         pe + 1
            clc
            adc         #<procs
            sta         pe
            lda         pe + 1
            adc         #>procs
            sta         pe + 1
            rts

; ---- Errors' handlers

; ON ERROR GOTO a16 (or 0: $FFFF).  ON ERROR GOTO 0 in the handler: the error stops the program, as if there were none
op_onerr:
            ldy         #1
            lda         (ip),y
            sta         eh_addr
            iny
            lda         (ip),y
            sta         eh_addr + 1
            cmp         #$FF
            bne         :+
            lda         eh_in
            beq         :+
            stz         eh_in
            jmp         err_said
:
            jmp         nx3

; RESUME (n 0): the failing statement again; RESUME NEXT (1): the one after it; RESUME label (2, a16).  The value stack
; as the failing statement began.  Not in a handler: RESUME without error
op_resume:
            lda         eh_in
            bne         :+
            lda         #ER_RESUME
            ldx         #0
            jmp         err_entry
:
            stz         eh_in
            MOVW        vsp, err_vsp
            ldy         #1
            lda         (ip),y
            beq         @stmt
            cmp         #1
            beq         @next
            ldy         #2
            lda         (ip),y
            pha
            iny
            lda         (ip),y
            tax
            pla
            bra         @go
@next:
            lda         err_next
            ldx         err_next + 1
            bra         @go
@stmt:
            lda         err_ip
            ldx         err_ip + 1
@go:
            jsr         ip_set
            jmp         next

; An error to ON ERROR's handler (err_entry's): the 6502's stack the interpreter's, the value stack as the failing
; statement began (err_vsp), the handler run
err_trap:
            ldx         run_s
            txs
            jsr         err_base
            MOVW        vsp, err_vsp
            lda         #1
            sta         eh_in
            lda         eh_addr
            ldx         eh_addr + 1
            jsr         ip_set
            jmp         next

; err_vsp = the failing statement's value stack's start: past its frame's slots (a procedure's: its mark's return
; follows its CALL, which names it: its parameters, mark and locals) and past the GOSUBs' returns over them
err_base:
            MOVW        vt, frp
@find:
            lda         vt                                  ; (A frame's mark from frp up: a procedure's)
            cmp         vsp
            lda         vt + 1
            sbc         vsp + 1
            bcs         @main
            lda         (vt)
            cmp         #VT_FRM
            beq         @frame
            jsr         vt_next
            bra         @find
@main:
            MOVW        vt, frp
            bra         @gos
@frame:
            ldy         #1                                  ; (Its CALL's p: the byte before its return)
            lda         (vt),y
            sec
            sbc         #1
            sta         t0
            iny
            lda         (vt),y
            sbc         #0
            sta         t0 + 1
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            sta         cal
            lda         t0 + 1
            and         #$1F
            ora         #WIN
            sta         ca + 1
            lda         t0
            sta         ca
            jsr         ca_sel
            jsr         ca_rd
            jsr         code_sel
            jsr         pe_of
            ldy         #3                                  ; (Past the mark and its locals)
            lda         (pe),y
            inc         a
            tax
            clc
            lda         vt
            adc         mul5_lo,x
            sta         vt
            lda         vt + 1
            adc         mul5_hi,x
            sta         vt + 1
@gos:
            lda         vt                                  ; (Past GOSUBs' returns)
            cmp         vsp
            lda         vt + 1
            sbc         vsp + 1
            bcs         :+
            lda         (vt)
            cmp         #VT_GOS
            bne         :+
            jsr         vt_next
            bra         @gos
:
            MOVW        err_vsp, vt
            rts

; vt on past a value
vt_next:
            clc
            lda         vt
            adc         #VAL_SIZE
            sta         vt
            bcc         :+
            inc         vt + 1
:
            rts

; ---- Not yet
op_fget:
op_fput:
op_agetf:
op_aputf:
            lda         #ER_SYNTAX
            ldx         #0
            jmp         err_entry

; ---- 5 * n, for a frame's slots
mul5_lo:
.repeat 256, n
            .byte       <(n * 5)
.endrepeat
mul5_hi:
.repeat 256, n
            .byte       >(n * 5)
.endrepeat
