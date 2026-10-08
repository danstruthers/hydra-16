; ****************************************************************************
; expr.inc - expressions compiled (the second bank): QuickBASIC's precedence, by recursive descent, the code a stack
; machine's (run.inc), each expression's type checked as it's compiled (etype: a number, a string, a record).  The
; token after an expression is the current one (tk) as each routine ends; each starts at its first.
;   From the last done: IMP, EQV, XOR, OR, AND, NOT, the comparisons, + and -, MOD, \, * and /, - (one operand),
; ^ (whose right operand may have a - of its own: 2 ^ -1).

ET_NUM          = 0             ; etype
ET_STR          = 1
ET_REC          = 2             ; (etinfo its type)

.bss
etype:      .res        1
etinfo:     .res        1
e_op:       .res        1                                   ; (An operator's op)
fn_ent:     .res        2                                   ; (A built-in's entry in fn_tab)
fn_argn:    .res        1                                   ; (Its arguments, counted)
dname:      .res        NAME_MAX + 2                        ; (A dotted name's whole: designator's)
dnl:        .res        1

.segment "CODE2"

; The current token: is it the operator .A?  OUT: Z = 1 yes
is_op:
            ldx         tk
            cpx         #TK_OP
            bne         :+
            cmp         tkv
:
            rts

; Is it the keyword .A?  OUT: Z = 1 yes
is_kw:
            ldx         tk
            cpx         #TK_KW
            bne         :+
            cmp         tkv
:
            rts

; The operator .A must be next (then read past): else Syntax error
need_op:
            jsr         is_op
            beq         :+
            jmp         syntax_err
:
            jmp         lex_next

need_kw:
            jsr         is_kw
            beq         :+
            jmp         syntax_err
:
            jmp         lex_next

syntax_err:
            lda         #ER_SYNTAX
            ldx         #0
            jmp         c_error

type_err:
            lda         #ER_TYPE
            ldx         #0
            jmp         c_error

; An expression that must be a number (expr_num) or a string (expr_str)
expr_num:
            jsr         expr
            lda         etype
            bne         type_err
            rts
expr_str:
            jsr         expr
            lda         etype
            cmp         #ET_STR
            bne         type_err
            rts

; ---- The levels

expr:
e_imp:
            jsr         e_eqv
@more:
            lda         #K_IMP
            jsr         is_kw
            bne         :+
            lda         #OP_IMP
            jsr         bits_op
            bra         @more
:
            rts

e_eqv:
            jsr         e_xor
@more:
            lda         #K_EQV
            jsr         is_kw
            bne         :+
            lda         #OP_EQV
            jsr         bits_op
            bra         @more
:
            rts

e_xor:
            jsr         e_or
@more:
            lda         #K_XOR
            jsr         is_kw
            bne         :+
            lda         #OP_XOR
            jsr         bits_op
            bra         @more
:
            rts

e_or:
            jsr         e_and
@more:
            lda         #K_OR
            jsr         is_kw
            bne         :+
            lda         #OP_OR
            jsr         bits_op
            bra         @more
:
            rts

e_and:
            jsr         e_not
@more:
            lda         #K_AND
            jsr         is_kw
            bne         :+
            lda         #OP_AND
            sta         e_op
            lda         etype                               ; (A number first)
            bne         @type
            jsr         lex_next
            jsr         e_not
            jmp         num_op_emit
:
            rts
@type:
            jmp         type_err

; An operator of the bits (.A its op) whose left operand's compiled: its right one (the level below's), then it
bits_op:
            sta         e_op
            lda         etype
            beq         :+
            jmp         type_err
:
            jsr         lex_next
            lda         e_op
            pha
            cmp         #OP_IMP
            bne         :+
            jsr         e_eqv
            bra         @emit
:
            cmp         #OP_EQV
            bne         :+
            jsr         e_xor
            bra         @emit
:
            cmp         #OP_XOR
            bne         :+
            jsr         e_or
            bra         @emit
:
            jsr         e_and
@emit:
            pla
            sta         e_op
            jmp         num_op_emit

; e_op emitted, its right operand compiled: a number's
num_op_emit:
            lda         etype
            beq         :+
            jmp         type_err
:
            lda         e_op
            ldx         #1
            jmp         emit_op

e_not:
            lda         #K_NOT
            jsr         is_kw
            bne         e_rel
            jsr         lex_next
            jsr         e_not
            lda         etype
            beq         :+
            jmp         type_err
:
            lda         #OP_NOT
            ldx         #1
            jmp         emit_op

; Comparisons: numbers' or strings' (both the same)
e_rel:
            jsr         e_add
@more:
            lda         tk
            cmp         #TK_OP
            bne         @done
            ldx         #rel_n - 1
            lda         tkv
:
            cmp         rel_ch,x
            beq         @found
            dex
            bpl         :-
@done:
            rts
@found:
            lda         rel_op,x
            sta         e_op
            lda         etype
            pha
            lda         e_op
            pha
            jsr         lex_next
            jsr         e_add
            pla
            sta         e_op
            pla
            cmp         etype
            beq         :+
            jmp         type_err
:
            cmp         #ET_STR
            bne         :+
            lda         e_op                                ; (A string's: the S ops, 6 past)
            clc
            adc         #OP_SEQ - OP_EQ
            sta         e_op
            bra         :++
:
            cmp         #ET_NUM
            beq         :+
            jmp         type_err
:
            lda         e_op
            ldx         #1
            jsr         emit_op
            stz         etype                               ; (-1 or 0)
            bra         @more
rel_ch:     .byte       '=', TO_NE, '<', '>', TO_LE, TO_GE
rel_op:     .byte       OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE
rel_n       = rel_op - rel_ch

e_add:
            jsr         e_mod
@more:
            lda         #'+'
            jsr         is_op
            beq         @add
            lda         #'-'
            jsr         is_op
            beq         @sub
            rts
@add:
            lda         etype
            pha
            jsr         lex_next
            jsr         e_mod
            pla
            cmp         etype
            beq         :+
            jmp         type_err
:
            ldx         #OP_ADD
            cmp         #ET_STR
            bne         :+
            ldx         #OP_CAT
            bra         @emit
:
            cmp         #ET_NUM
            beq         @emit
            jmp         type_err
@emit:
            txa
            ldx         #1
            jsr         emit_op
            bra         @more
@sub:
            lda         etype
            beq         :+
            jmp         type_err
:
            jsr         lex_next
            jsr         e_mod
            lda         #OP_SUB
            sta         e_op
            jsr         num_op_emit
            bra         @more

e_mod:
            jsr         e_idiv
@more:
            lda         #K_MOD
            jsr         is_kw
            bne         :+
            lda         etype
            bne         @type
            jsr         lex_next
            jsr         e_idiv
            lda         #OP_MOD
            sta         e_op
            jsr         num_op_emit
            bra         @more
:
            rts
@type:
            jmp         type_err

e_idiv:
            jsr         e_mul
@more:
            lda         #'\'
            jsr         is_op
            bne         :+
            lda         etype
            bne         @type
            jsr         lex_next
            jsr         e_mul
            lda         #OP_IDIV
            sta         e_op
            jsr         num_op_emit
            bra         @more
:
            rts
@type:
            jmp         type_err

e_mul:
            jsr         e_neg
@more:
            lda         #'*'
            jsr         is_op
            bne         :+
            lda         #OP_MUL
            bra         @op
:
            lda         #'/'
            jsr         is_op
            beq         :+
            rts
:
            lda         #OP_DIV
@op:
            pha
            lda         etype
            bne         @type
            jsr         lex_next
            jsr         e_neg
            pla
            sta         e_op
            jsr         num_op_emit
            bra         @more
@type:
            jmp         type_err

e_neg:
            lda         #'-'
            jsr         is_op
            bne         @plus
            jsr         lex_next
            jsr         e_neg
            lda         etype
            beq         :+
            jmp         type_err
:
            lda         #OP_NEG
            ldx         #1
            jmp         emit_op
@plus:
            lda         #'+'
            jsr         is_op
            bne         e_pow
            jsr         lex_next
            jmp         e_neg

; a ^ b ^ c, from the left; an exponent may have - before it
e_pow:
            jsr         e_atom
@more:
            lda         #'^'
            jsr         is_op
            bne         @done
            lda         etype
            bne         @type
            jsr         lex_next
            jsr         e_expo
            lda         #OP_POW
            sta         e_op
            jsr         num_op_emit
            bra         @more
@done:
            rts
@type:
            jmp         type_err
e_expo:
            lda         #'-'
            jsr         is_op
            bne         e_atom
            jsr         lex_next
            jsr         e_expo
            lda         etype
            bne         @type
            lda         #OP_NEG
            ldx         #1
            jmp         emit_op
@type:
            jmp         type_err

; ---- An operand

e_atom:
            lda         tk
            cmp         #TK_NUM
            bne         :+
            jsr         emit_num
            stz         etype
            jmp         lex_next
:
            cmp         #TK_STR
            bne         :+
            jsr         emit_str
            lda         #ET_STR
            sta         etype
            jmp         lex_next
:
            cmp         #TK_NAME
            bne         :+
            jmp         e_name
:
            cmp         #TK_KW
            bne         :+
            jmp         e_func
:
            cmp         #TK_OP
            bne         @syntax
            lda         tkv
            cmp         #'('
            bne         :+
            jsr         lex_next
            jsr         expr
            lda         #')'
            jmp         need_op
:
            cmp         #'#'
            bne         @syntax
            jsr         e_based
            stz         etype
            rts
@syntax:
            jmp         syntax_err

; A number with a # base of its own (#xFF, #16r1F, #c+-0), at tstart: the longest start that's one, as a program's
; text has it
e_based:
            MOVW        r0, tstart
            LDW         r1, 255
            LDW         r4, cbase
            LDW         r2, tnum
            LDW         r3, TNUM_MAX
            ldy         #NPARSE_PROGRAM
            FARN        4, num_parse_ram                    ; (r5: the characters it took)
            bcc         :+
            jmp         syntax_err
:
            sta         tnl
            stx         tnl + 1
            FARN        4, num_isint32
            lda         tstart                              ; (lp: past it)
            clc
            adc         r5
            sta         lp
            lda         tstart + 1
            adc         r5 + 1
            sta         lp + 1
            jsr         emit_num
            jmp         lex_next

; The number token's push emitted: an integer of 8 or 32 bits, or its bytes (a copy in the heap at the prompt)
emit_num:
            lda         tisint
            beq         @bytes
            lda         tint + 1                            ; (-128 to 127: PUSHI8)
            ora         tint + 2
            ora         tint + 3
            bne         :+
            lda         tint
            bpl         @i8
:
            lda         tint + 1
            and         tint + 2
            and         tint + 3
            cmp         #$FF
            bne         @i32
            lda         tint
            bmi         @i8
@i32:
            lda         #OP_PUSHI32
            ldx         #5
            jsr         emit_op
            ldx         #0
:
            lda         tint,x
            jsr         emit
            inx
            cpx         #4
            bne         :-
            rts
@i8:
            lda         #OP_PUSHI8
            ldx         #2
            jsr         emit_op
            lda         tint
            jmp         emit
@bytes:
            lda         tnl
            clc
            adc         #3
            tax
            lda         #OP_PUSHN
            ldy         cmode
            beq         :+
            lda         #OP_PUSHNH
:
            jsr         emit_op
            lda         tnl
            ldx         #0
            jsr         emit_w
            ldx         #0
:
            lda         tnum,x
            jsr         emit
            inx
            cpx         tnl
            bne         :-
            rts

; The string token's push emitted (tsp, tsl): its bytes in the code ("": PUSHE)
emit_str:
            lda         tsl
            bne         :+
            lda         #OP_PUSHE
            ldx         #1
            jmp         emit_op
:
            clc
            adc         #3
            tax
            lda         #OP_PUSHS
            ldy         cmode
            beq         :+
            lda         #OP_PUSHSH
:
            jsr         emit_op
            lda         tsl
            ldx         #0
            jsr         emit_w
            ldy         #0
:
            lda         (tsp),y
            jsr         emit
            iny
            cpy         tsl
            bne         :-
            rts

; ---- Names: variables, arrays, constants, FUNCTIONs

; A name in an expression: its value pushed (e_name), with its type (etype)
e_name:
            jsr         const_find                          ; A constant?
            bcs         :+
            jmp         const_emit
:
            lda         (lp)                                ; (A ( right after it, or after spaces: an array, a
            ldy         #0                                  ;   FUNCTION's arguments)
:
            lda         (lp),y
            cmp         #' '
            bne         :+
            iny
            bra         :-
:
            cmp         #'('
            bne         @scalar
            jsr         proc_find                           ; A FUNCTION's call?
            bcs         :+
            jmp         e_fcall
:
            jsr         arr_lookup                          ; An array's element: its reference, its indices
            jsr         arr_etype
            lda         etype
            pha
            lda         #OP_REFV
            jsr         emit_slot
            jsr         lex_next                            ; (Past the name: its ()
            jsr         lex_next
            jsr         emit_indices                        ; (Its indices, and its ): .A their count)
            pha
            lda         #OP_AGET
            ldx         #2
            jsr         emit_op
            pla
            jsr         emit
            pla
            sta         etype
            rts
@scalar:
            jsr         proc_find                           ; (A FUNCTION with no parameters: called)
            bcs         :+
            jmp         e_fcall
:
            jsr         var_lookup
            lda         scls                                ; (Its type)
            jsr         class_type
            sta         etype
            lda         #OP_LDV
            jsr         emit_slot
            jmp         lex_next

; The op .A and spl's slot (a v16) emitted
emit_slot:
            ldx         #3
            jsr         emit_op
            lda         spl + SP_SLOT
            ldx         spl + SP_SLOT + 1
            jmp         emit_w

; An array's indices, its ( read: each a number, then its ), the token after it current.  OUT: .A = their count
emit_indices:
            lda         #1
            pha
@each:
            jsr         expr_num
            lda         #','
            jsr         is_op
            bne         :+
            pla
            inc         a
            pha
            jsr         lex_next
            bra         @each
:
            lda         #')'
            jsr         need_op
            pla
            rts

; etype = the type of the array (scls) whose element it is
arr_etype:
            stz         etype
            lda         scls
            cmp         #SC_SARR
            bne         :+
            lda         #ET_STR
            sta         etype
:
            rts

; ---- Variables found (or made)

; A variable's class for tname (tsuf): a string's ($, AS STRING, DEFSTR) or a record's or a number's.  OUT: scls
scalar_class:
            lda         tsuf
            cmp         #'$'
            beq         @str
            cmp         #0
            bne         @num                                ; (% & ! #: a number's)
            lda         #SC_REC                             ; (Declared a record, or AS STRING: so)
            jsr         class_known
            bcc         @have
            lda         #SC_STR
            jsr         class_known
            bcc         @have
            jsr         is_defstr
            bcc         @str
@num:
            lda         #SC_NUM
            bra         @have
@str:
            lda         #SC_STR
@have:
            sta         scls
            rts

; An array's: of strings ($, DEFSTR), of records (declared so), of numbers
array_class:
            lda         tsuf
            cmp         #'$'
            beq         @str
            cmp         #0
            bne         @num
            lda         #SC_RARR
            jsr         class_known
            bcc         @have
            lda         #SC_SARR
            jsr         class_known
            bcc         @have
            jsr         is_defstr
            bcc         @str
@num:
            lda         #SC_NARR
            bra         @have
@str:
            lda         #SC_SARR
@have:
            sta         scls
            rts

; Is tname known in class .A here (this scope's, or a shared global)?  OUT: C = 0 yes (sent, spl)
class_known:
            sta         scls
            jmp         var_find

; Does tname start with one of DEFSTR's letters?  OUT: C = 0 yes
is_defstr:
            lda         tname
            sec
            sbc         #'A'
            tax
            and         #7
            tay
            txa
            lsr         a
            lsr         a
            lsr         a
            tax
            lda         defstr,x
            and         bit_of,y
            beq         :+
            clc
            rts
:
            sec
            rts
bit_of:     .byte       $01, $02, $04, $08, $10, $20, $40, $80

; The variable tname of class scls, here: in a procedure its own (a local, a SHARED or STATIC one), else a global
; that's every procedure's (DIM SHARED); in the main program the global.  OUT: C = 0 found (sent, spl); C = 1 none
var_find:
            lda         cscope
            beq         @global
            sta         sscope
            jsr         sym_find
            bcs         :+
            rts
:
            stz         sscope
            jsr         sym_find
            bcs         @none
            lda         spl + SP_FLAGS
            and         #SF_SHARED
            beq         @none
            clc
            rts
@global:
            stz         sscope
            jmp         sym_find
@none:
            sec
            rts

; The scalar variable tname (its class by scalar_class), found or made: spl
var_lookup:
            jsr         scalar_class
var_lookup_cls:
            jsr         var_find
            bcc         @have
            lda         cscope
            sta         sscope
            ldy         #LK_NUM
            lda         scls
            cmp         #SC_STR
            bne         :+
            ldy         #LK_STR
:
            cmp         #SC_REC
            bne         :+
            ldy         #LK_REC
:
            jsr         var_new
@have:
            rts

; The array tname (its class by array_class), found or made (0 to 10 in each dimension when it's used, as it isn't
; DIMmed): spl
arr_lookup:
            jsr         array_class
            jsr         var_find
            bcc         @have
            lda         cscope
            sta         sscope
            ldy         #LK_ARR
            lda         scls
            cmp         #SC_SARR
            bne         :+
            ldy         #LK_SARR
:
            jsr         var_new
@have:
            rts

; ---- Constants

; Is tname a constant?  OUT: C = 0 yes (spl)
const_find:
            lda         tsuf                                ; (A constant's name: as it was declared, its $ too)
            pha
            lda         #SC_CONST
            sta         scls
            stz         sscope
            jsr         sym_find
            pla
            rts

; The constant's push emitted (spl: its kind, its value; its bytes past the entry's payload)
const_emit:
            lda         spl                                 ; (0: an integer)
            bne         @bytes
            ldx         #3
:
            lda         spl + 1,x
            sta         tint,x
            dex
            bpl         :-
            lda         #1
            sta         tisint
            jsr         emit_num
            stz         etype
            jmp         lex_next
@bytes:
            pha
            jsr         const_bytes                         ; (Its bytes into tnum, tnl)
            pla
            cmp         #1
            bne         @str
            stz         tisint
            jsr         emit_num
            stz         etype
            jmp         lex_next
@str:
            LDW         tsp, tnum
            MOVW        tsl, tnl
            jsr         emit_str
            lda         #ET_STR
            sta         etype
            jmp         lex_next

; A constant's bytes (past its entry's payload, sent) into tnum: tnl of them (spl + 1, 2)
const_bytes:
            lda         spl + 1
            sta         tnl
            lda         spl + 2
            sta         tnl + 1
            lda         sent
            ldx         sent + 1
            jsr         sym_ca
            ldx         #4                                  ; (Past its next, class and scope)
:
            jsr         ca_rd
            dex
            bne         :-
            jsr         ca_rd                               ; (Its name's length)
            clc
            adc         #SPL
            tax
:
            jsr         ca_rd
            dex
            bne         :-
            ldx         #0
:
            cpx         tnl
            beq         :+
            jsr         ca_rd
            sta         tnum,x
            inx
            bra         :-
:
            jmp         code_sel

; ---- Procedures

; Is tname a FUNCTION (or a SUB)?  OUT: C = 0 yes (spl: SPR_*)
proc_find:
            lda         #SC_PROC
            sta         scls
            stz         sscope
            jmp         sym_find

; A FUNCTION's call in an expression (procs.inc)
e_fcall:
            jmp         fcall_expr

; ---- The built-in functions (fn_tab): each its keyword, its routine (F_*), its result (ET_*), its arguments (the
; least and the most), their types (N a number, S a string, A either), F_NONE as an operand that's itself (CSNG)

e_func:
            ldx         #0
@find:
            lda         fn_tab,x
            beq         @none
            cmp         tkv
            beq         @found
            txa
            clc
            adc         #FN_ENTRY
            tax
            bra         @find
@none:
            jmp         syntax_err
@found:
            stx         fn_ent
            jsr         lex_next
            ldx         fn_ent
            lda         fn_tab + 1,x                        ; (Special forms: their own)
            cmp         #F_SPECIAL
            bcc         :+
            jmp         fn_special
:
            stz         fn_argn
            lda         fn_tab + 4,x                        ; (None at most: () may follow)
            bne         @args
            lda         #'('
            jsr         is_op
            bne         @call
            jsr         lex_next
            lda         #')'
            jsr         need_op
            bra         @call
@args:
            lda         #'('
            jsr         is_op
            bne         @noparen
            jsr         lex_next
@arg:
            ldx         fn_ent                              ; (Its type)
            txa
            clc
            adc         fn_argn
            tax
            lda         fn_tab + 5,x
            pha
            jsr         expr
            pla
            cmp         #'A'
            beq         @ok
            ldx         #ET_NUM
            cmp         #'N'
            beq         :+
            ldx         #ET_STR
:
            cpx         etype
            beq         @ok
            jmp         type_err
@ok:
            inc         fn_argn
            lda         #','
            jsr         is_op
            bne         @close
            ldx         fn_ent
            lda         fn_argn
            cmp         fn_tab + 4,x
            bcs         @syntax
            jsr         lex_next
            bra         @arg
@close:
            lda         #')'
            jsr         need_op
            ldx         fn_ent
            lda         fn_argn
            cmp         fn_tab + 3,x
            bcc         @syntax
            bra         @call
@noparen:
            ldx         fn_ent                              ; (No arguments given: only if none are needed: RND)
            lda         fn_tab + 3,x
            bne         @syntax
@call:
            ldx         fn_ent
            lda         fn_tab + 3,x                        ; (Some optional: their count pushed)
            cmp         fn_tab + 4,x
            beq         :+
            lda         #OP_PUSHI8
            ldx         #2
            jsr         emit_op
            lda         fn_argn
            jsr         emit
            ldx         fn_ent
:
            lda         fn_tab + 2,x
            sta         etype
            lda         fn_tab + 1,x
            beq         @done                               ; (F_NONE: the operand itself)
            pha
            lda         #OP_FN
            ldx         #2
            jsr         emit_op
            pla
            jmp         emit
@done:
            rts
@syntax:
            jmp         syntax_err

; Functions of a form of their own (fn_special: procs.inc's and the rest's)
fn_special:
            FARN        3, fn_special3
            rts

FN_ENTRY        = 9
.macro FN kw, f, res, lo, hi, types
            .byte       kw, f, res, lo, hi
            .byte       types
            .res        4 - .strlen(types), 0
.endmacro

fn_tab:
            FN          K_ABS, F_ABS, ET_NUM, 1, 1, "N"
            FN          K_ASC, F_ASC, ET_NUM, 1, 1, "S"
            FN          K_ATN, F_ATN, ET_NUM, 1, 1, "N"
            FN          K_BIT, F_BIT, ET_NUM, 2, 2, "NN"
            FN          K_CDBL, F_NONE, ET_NUM, 1, 1, "N"
            FN          K_CSNG, F_NONE, ET_NUM, 1, 1, "N"
            FN          K_CHR, F_CHR, ET_STR, 1, 1, "N"
            FN          K_CINT, F_CINT, ET_NUM, 1, 1, "N"
            FN          K_CLNG, F_CINT, ET_NUM, 1, 1, "N"
            FN          K_COMMAND, F_COMMAND, ET_STR, 0, 0, ""
            FN          K_COMPLEX, F_COMPLEX, ET_NUM, 2, 2, "NN"
            FN          K_COS, F_COS, ET_NUM, 1, 1, "N"
            FN          K_CSRLIN, F_CSRLIN, ET_NUM, 0, 0, ""
            FN          K_CVN, F_CVN, ET_NUM, 1, 1, "S"
            FN          K_DATE, F_DATE, ET_STR, 0, 0, ""
            FN          K_DENOMINATOR, F_DEN, ET_NUM, 1, 1, "N"
            FN          K_DIGITS, F_DIGITS, ET_NUM, 0, 0, ""
            FN          K_DIR, F_DIR, ET_STR, 0, 1, "S"
            FN          K_ENV, F_ENV, ET_STR, 1, 1, "S"
            FN          K_ENVIRONS, F_ENV, ET_STR, 1, 1, "S"
            FN          K_EOF, F_EOF, ET_NUM, 1, 1, "N"
            FN          K_ERL, F_ERL, ET_NUM, 0, 0, ""
            FN          K_ERR, F_ERR, ET_NUM, 0, 0, ""
            FN          K_ERRS, F_ERRS, ET_STR, 0, 0, ""
            FN          K_EXP, F_EXP, ET_NUM, 1, 1, "N"
            FN          K_FIB, F_FIB, ET_NUM, 1, 1, "N"
            FN          K_FIX, F_FIX, ET_NUM, 1, 1, "N"
            FN          K_FIXED, F_FIXED, ET_NUM, 2, 2, "NN"
            FN          K_FRE, F_FRE, ET_NUM, 0, 1, "A"
            FN          K_FREEFILE, F_FREEFILE, ET_NUM, 0, 0, ""
            FN          K_GCD, F_GCD, ET_NUM, 2, 2, "NN"
            FN          K_HEX, F_HEX, ET_STR, 1, 1, "N"
            FN          K_IMAG, F_IMAG, ET_NUM, 1, 1, "N"
            FN          K_INKEY, F_INKEY, ET_STR, 0, 0, ""
            FN          K_INT, F_INT, ET_NUM, 1, 1, "N"
            FN          K_LCASE, F_LCASE, ET_STR, 1, 1, "S"
            FN          K_LEFT, F_LEFT, ET_STR, 2, 2, "SN"
            FN          K_LEN, F_LEN, ET_NUM, 1, 1, "S"
            FN          K_LOC, F_LOC, ET_NUM, 1, 1, "N"
            FN          K_LOF, F_LOF, ET_NUM, 1, 1, "N"
            FN          K_LOG, F_LOG, ET_NUM, 1, 1, "N"
            FN          K_LTRIM, F_LTRIM, ET_STR, 1, 1, "S"
            FN          K_MID, F_MID, ET_STR, 2, 3, "SNN"
            FN          K_MKN, F_MKN, ET_STR, 1, 1, "N"
            FN          K_NUMERATOR, F_NUMER, ET_NUM, 1, 1, "N"
            FN          K_OCT, F_OCT, ET_STR, 1, 1, "N"
            FN          K_PEEK, F_PEEK, ET_NUM, 1, 1, "N"
            FN          K_PI, F_PI, ET_NUM, 0, 0, ""
            FN          K_POINT, F_POINT, ET_NUM, 2, 2, "NN"
            FN          K_POS, F_POS, ET_NUM, 0, 1, "A"
            FN          K_RATIONAL, F_RATIONAL, ET_NUM, 1, 1, "N"
            FN          K_REAL, F_REAL, ET_NUM, 1, 1, "N"
            FN          K_RIGHT, F_RIGHT, ET_STR, 2, 2, "SN"
            FN          K_RND, F_RND, ET_NUM, 0, 1, "N"
            FN          K_RTRIM, F_RTRIM, ET_STR, 1, 1, "S"
            FN          K_SEEK, F_SEEK, ET_NUM, 1, 1, "N"
            FN          K_SGN, F_SGN, ET_NUM, 1, 1, "N"
            FN          K_SHELLS, F_SHELLS, ET_STR, 1, 1, "S"
            FN          K_SHL, F_SHL, ET_NUM, 2, 2, "NN"
            FN          K_SHR, F_SHR, ET_NUM, 2, 2, "NN"
            FN          K_SIN, F_SIN, ET_NUM, 1, 1, "N"
            FN          K_SPACE, F_SPACE, ET_STR, 1, 1, "N"
            FN          K_SQR, F_SQR, ET_NUM, 1, 1, "N"
            FN          K_STATUS, F_STATUS, ET_NUM, 0, 0, ""
            FN          K_STR, F_STR, ET_STR, 1, 2, "NS"
            FN          K_STRINGS, F_STRING, ET_STR, 2, 2, "NA"
            FN          K_TAN, F_TAN, ET_NUM, 1, 1, "N"
            FN          K_TIME, F_TIME, ET_STR, 0, 0, ""
            FN          K_TIMER, F_TIMER, ET_NUM, 0, 0, ""
            FN          K_UCASE, F_UCASE, ET_STR, 1, 1, "S"
            FN          K_VAL, F_VAL, ET_NUM, 1, 1, "S"
            FN          K_ARG, F_ARG, ET_STR, 1, 1, "N"
            FN          K_BANK, F_BANK, ET_NUM, 0, 0, ""
            FN          K_BASES, F_BASES, ET_STR, 0, 0, ""
            FN          K_INSTR, F_SP_INSTR, ET_NUM, 2, 3, "A"
            FN          K_INPUTS, F_SP_INPUT, ET_STR, 1, 2, "N"
            FN          K_LBOUND, F_SP_LBOUND, ET_NUM, 1, 2, "A"
            FN          K_UBOUND, F_SP_UBOUND, ET_NUM, 1, 2, "A"
            .byte       0
