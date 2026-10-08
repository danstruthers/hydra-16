; ****************************************************************************
; stmt3.inc - the third bank's statements: declarations (DIM, REDIM, ERASE, CONST, OPTION BASE, DEF*, SHARED,
; STATIC), data (DATA, READ, RESTORE), SWAP, RANDOMIZE, BASE, DIGITS, the console's input (INPUT, LINE INPUT); files,
; the system's, sound and graphics to come.  Each as stmt.inc's: from the token after its keyword (c_stmt3 reads past
; it), the token after the statement current at its end.  The second bank's routines are called through to2 (C2: 5
; bytes a call): .A, .X, .Y and C pass through it both ways, Z and N don't (is_op3 and is_kw3 are this bank's own).

DATA_END        = $4000         ; DATA's region's end (2 banks)
CONST_CODE      = $1800         ; A CONST's value's code: in the prompt's region, from here (its last 2K)

.bss
c3_kw:      .res        1                                   ; (The statement's keyword)
dm_shared:  .res        1                                   ; (DIM's: SHARED; REDIM ($80); PRESERVE ($40); the
dm_redim:   .res        1                                   ;   dimensions; those whose lowest is given (bit each);
dm_pres:    .res        1                                   ;   AS's type (PK_*: $FF none))
dm_dims:    .res        1
dm_lomask:  .res        1
dm_as:      .res        1
dm_name:    .res        NAME_MAX + 2                        ; (A name kept while expressions are compiled)
dm_nl:      .res        1
dm_suf:     .res        1
c3_t:       .res        1                                   ; (Scratch: DEF*'s letters, a field's length ...)
c3_lo:      .res        1
c3_hi:      .res        1
c3_len:     .res        1
c3_cb:      .res        3                                   ; (CONST's: the code's cursor, the value stack, kept)
c3_vsp:     .res        2
in_loop:    .res        2                                   ; (INPUT's start: Redo from start's way back)
bs_tmp:     .res        BASE_ROOM                           ; (BASE's: its string, written in the program)
xp_x:       .res        1                                   ; (to2's)
xp_y:       .res        1
xp_p:       .res        1

.segment "CODE3"

; A routine of the second bank's called: jsr to2, then its address
.macro C2 routine
            jsr         to2
            .word       routine
.endmacro

to2:
            sta         hyx2_a
            stx         xp_x
            sty         xp_y
            php
            pla
            sta         xp_p
            pla                                             ; (The word after the jsr: the routine)
            sta         xp
            pla
            sta         xp + 1
            ldy         #1
            lda         (xp),y
            sta         hyx2_to
            iny
            lda         (xp),y
            sta         hyx2_to + 1
            clc                                             ; (The way back: past the word)
            lda         xp
            adc         #2
            tax
            lda         xp + 1
            adc         #0
            pha
            phx
            lda         hyx2_bank2
            sta         hyx2_n
            ldx         xp_x
            ldy         xp_y
            lda         xp_p
            pha
            lda         hyx2_a
            plp
            jmp         hyx2_farn

; ---- The statements: by their keyword

; A statement the second bank hasn't (its keyword current): its routine, past its keyword (DATA: its text as it is)
c_stmt3:
            lda         tkv
            sta         c3_kw
            cmp         #K_DATA
            bne         :+
            jmp         cs_data
:
            ldx         #0
@find:
            lda         st3_tab,x
            beq         not_yet3
            cmp         c3_kw
            beq         @found
            inx
            inx
            inx
            bra         @find
@found:
            lda         st3_tab + 2,x                       ; (Its routine, by rts)
            pha
            lda         st3_tab + 1,x
            pha
            C2          lex_next
            rts

not_yet3:
            lda         #73
            ldx         #0
            jmp         b_error

st3_tab:
            .byte       K_DIM
            .word       cs_dim - 1
            .byte       K_REDIM
            .word       cs_redim - 1
            .byte       K_ERASE
            .word       cs_erase - 1
            .byte       K_CONST
            .word       cs_const - 1
            .byte       K_OPTION
            .word       cs_option - 1
            .byte       K_DEFSTR
            .word       cs_defs - 1
            .byte       K_DEFINT
            .word       cs_defs - 1
            .byte       K_DEFLNG
            .word       cs_defs - 1
            .byte       K_DEFSNG
            .word       cs_defs - 1
            .byte       K_DEFDBL
            .word       cs_defs - 1
            .byte       K_SHARED
            .word       cs_shared - 1
            .byte       K_STATIC
            .word       cs_static - 1
            .byte       K_READ
            .word       cs_read - 1
            .byte       K_RESTORE
            .word       cs_restore - 1
            .byte       K_SWAP
            .word       cs_swap - 1
            .byte       K_RANDOMIZE
            .word       cs_randomize - 1
            .byte       K_BASE
            .word       cs_base - 1
            .byte       K_DIGITS
            .word       cs_digits - 1
            .byte       K_INPUT
            .word       cs_input - 1
            .byte       K_LINE
            .word       cs_line - 1
            .byte       0

; ---- This bank's helpers

; The current token: is it the operator .A (is_op3), the keyword .A (is_kw3)?  OUT: Z = 1 yes
is_op3:
            ldx         tk
            cpx         #TK_OP
            bne         :+
            cmp         tkv
:
            rts
is_kw3:
            ldx         tk
            cpx         #TK_KW
            bne         :+
            cmp         tkv
:
            rts

; The operator .A (need_op3) or the keyword .A (need_kw3) must be next (then read past): else Syntax error
need_op3:
            jsr         is_op3
            bne         syn3
            C2          lex_next
            rts
need_kw3:
            jsr         is_kw3
            bne         syn3
            C2          lex_next
            rts

syn3:
            lda         #ER_SYNTAX
            ldx         #0
            jmp         b_error
type3:
            lda         #ER_TYPE
            ldx         #0
            jmp         b_error
dup3:
            lda         #ER_DUP
            ldx         #0
            jmp         b_error
insub3:
            lda         #ER_INSUB
            ldx         #0
            jmp         b_error

; Is the current token a statement's end (the line's, a :, ELSE)?  OUT: C = 1 yes
at_end3:
            lda         tk
            beq         @yes
            lda         #':'
            jsr         is_op3
            beq         @yes
            lda         #K_ELSE
            jsr         is_kw3
            beq         @yes
            clc
            rts
@yes:
            sec
            rts

; OP_ST .A emitted (emit_st3); PUSHI8 .A (emit_i8)
emit_st3:
            pha
            lda         #OP_ST
            ldx         #2
            C2          emit_op
            pla
            C2          emit
            rts
emit_i8:
            pha
            lda         #OP_PUSHI8
            ldx         #2
            C2          emit_op
            pla
            C2          emit
            rts

; tname, tnl and tsuf kept (name_keep), and put back (name_back)
name_keep:
            ldx         #NAME_MAX + 1
:
            lda         tname,x
            sta         dm_name,x
            dex
            bpl         :-
            lda         tnl
            sta         dm_nl
            lda         tsuf
            sta         dm_suf
            rts
name_back:
            ldx         #NAME_MAX + 1
:
            lda         dm_name,x
            sta         tname,x
            dex
            bpl         :-
            lda         dm_nl
            sta         tnl
            stz         tnl + 1
            lda         dm_suf
            sta         tsuf
            rts

; A name's suffix against its class (scls): $ a string's, % & ! # a number's; else Type mismatch
suf_check:
            lda         tsuf
            beq         @ok
            ldx         #ET_STR
            cmp         #'$'
            beq         :+
            ldx         #ET_NUM
:
            stx         c3_t
            lda         scls
            C2          class_type
            cmp         c3_t
            beq         @ok
            jmp         type3
@ok:
            rts

; .Y = the kind (LK_*) of a variable of the class scls
decl_kind:
            ldy         #LK_NUM
            lda         scls
            cmp         #SC_STR
            bne         :+
            ldy         #LK_STR
:
            cmp         #SC_NARR
            bne         :+
            ldy         #LK_ARR
:
            cmp         #SC_SARR
            bne         :+
            ldy         #LK_SARR
:
            rts

; A declared name (SHARED's, STATIC's): name [()] [AS type]; tname its, scls its class
decl_name:
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syn3
:
            jsr         name_keep
            stz         dm_dims
            C2          lex_next
            lda         #'('
            jsr         is_op3
            bne         :+
            C2          lex_next
            lda         #')'
            jsr         need_op3
            inc         dm_dims
:
            jsr         as_clause
            jsr         name_back
            lda         dm_as
            cmp         #$FF
            bne         @typed
            lda         dm_dims
            beq         :+
            C2          array_class
            rts
:
            C2          scalar_class
            rts
@typed:
            and         #7
            ldx         #0                                  ; (A number's, a string's; an array's 2 on)
            cmp         #PK_NUM
            beq         :+
            inx
            cmp         #PK_STR
            beq         :+
            jmp         type3                               ; (A record's: to come)
:
            lda         dm_dims
            beq         :+
            inx
            inx
:
            lda         decl_cls,x
            sta         scls
            jmp         suf_check
decl_cls:   .byte       SC_NUM, SC_STR, SC_NARR, SC_SARR

; [AS type]: dm_as its PK_* ($FF: none), as_fix a fixed length's
as_clause:
            lda         #$FF
            sta         dm_as
            stz         as_fix
            stz         as_fix + 1
            lda         #K_AS
            jsr         is_kw3
            bne         :+
            C2          lex_next
            C2          as_type
            sta         dm_as
:
            rts

; ---- DIM, REDIM, ERASE

; REDIM [PRESERVE] items: as DIM's, the arrays made again (PRESERVE: what's in them kept)
cs_redim:
            lda         #$80
            sta         dm_redim
            stz         dm_pres
            lda         #K_PRESERVE
            jsr         is_kw3
            bne         dim_items
            lda         #$40
            sta         dm_pres
            C2          lex_next
            bra         dim_items

; DIM [SHARED] item, ...: an item a name, (bounds) for an array, [AS type]
cs_dim:
            stz         dm_redim
            stz         dm_pres
dim_items:
            stz         dm_shared
            lda         #K_SHARED
            jsr         is_kw3
            bne         @item
            inc         dm_shared
            C2          lex_next
@item:
            jsr         dim_item
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         @item
:
            rts

; One of DIM's items: name [([lowest TO] highest, ...)] [AS type].  An array's bounds compiled first (then its
; variable, then ST_DIM); a scalar declared (AS STRING * n: its spaces now)
dim_item:
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syn3
:
            jsr         name_keep
            stz         dm_dims
            stz         dm_lomask
            C2          lex_next
            lda         #'('
            jsr         is_op3
            bne         @as
            C2          lex_next
@bound:
            C2          expr_num
            lda         #K_TO
            jsr         is_kw3
            bne         @hi
            C2          lex_next
            ldx         dm_dims                             ; (Its lowest given)
            cpx         #8
            bcs         @many
            lda         bit_of3,x
            ora         dm_lomask
            sta         dm_lomask
            C2          expr_num
@hi:
            inc         dm_dims
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         @bound
:
            lda         #')'
            jsr         need_op3
            lda         dm_dims
            cmp         #9
            bcc         @as
@many:
            lda         #ER_SUBSCRIPT                       ; (8 dimensions at most)
            ldx         #0
            jmp         b_error
@as:
            jsr         as_clause
            jsr         name_back
            lda         dm_dims
            bne         @lb900
            jmp         dim_scalar
@lb900:
            ; (fall through)

; DIM of an array: its variable (found or made), its info, ST_DIM
dim_array:
            lda         dm_as
            cmp         #$FF
            bne         @as
            C2          array_class
            bra         @have
@as:
            and         #7
            ldx         #SC_NARR
            cmp         #PK_NUM
            beq         :+
            ldx         #SC_SARR
            cmp         #PK_STR
            beq         :+
            jmp         type3                               ; (Records: to come)
:
            stx         scls
            jsr         suf_check
@have:
            C2          var_find
            bcc         @found
            lda         cscope
            sta         sscope
            jsr         decl_kind
            C2          var_new
@found:
            jsr         decl_flags
            lda         #OP_REFV
            C2          emit_slot
            lda         #OP_PUSHI32                         ; (The info: dimensions, kind, PRESERVE, REDIM; the
            ldx         #5                                  ;   lowest given; OPTION BASE)
            C2          emit_op
            ldx         #0
            lda         scls
            cmp         #SC_SARR
            bne         :+
            ldx         #LK_STR << 4
:
            txa
            ora         dm_dims
            ora         dm_pres
            ora         dm_redim
            C2          emit
            lda         dm_lomask
            C2          emit
            lda         optbase
            C2          emit
            lda         #0
            C2          emit
            lda         #ST_DIM
            jmp         emit_st3

; DIM of a scalar: DIM x (made, if it isn't), DIM x AS type (declared: if it's here already, Duplicate definition)
dim_scalar:
            lda         dm_as
            cmp         #$FF
            bne         @typed
            C2          var_lookup
            jmp         decl_flags
@typed:
            and         #7
            ldx         #SC_NUM
            cmp         #PK_NUM
            beq         :+
            ldx         #SC_STR
            cmp         #PK_STR
            beq         :+
            jmp         type3                               ; (Records: to come)
:
            stx         scls
            jsr         suf_check
            lda         cscope
            sta         sscope
            C2          sym_find
            bcs         :+
            jmp         dup3
:
            jsr         decl_kind
            C2          var_new
            lda         as_fix                              ; (Its fixed length)
            sta         spl + SP_INFO
            lda         as_fix + 1
            sta         spl + SP_INFO + 1
            lda         #SF_DECL
            ldx         as_fix
            bne         :+
            ldx         as_fix + 1
            beq         :++
:
            ora         #SF_FIXED
:
            sta         spl + SP_FLAGS
            jsr         decl_flags
            lda         spl + SP_FLAGS                      ; (A fixed length: its spaces now)
            and         #SF_FIXED
            beq         @done
            lda         #OP_PUSHE
            ldx         #1
            C2          emit_op
            lda         #OP_FIXLEN
            ldx         #3
            C2          emit_op
            lda         spl + SP_INFO
            ldx         spl + SP_INFO + 1
            C2          emit_w
            lda         #OP_STV
            C2          emit_slot
@done:
            rts

; The variable's (sent, spl) flags, written back: SF_DECL with an AS type; SF_SHARED for DIM SHARED (the main
; program's: every procedure's)
decl_flags:
            lda         spl + SP_FLAGS
            ldx         dm_as
            cpx         #$FF
            beq         :+
            ora         #SF_DECL
:
            ldx         dm_shared
            beq         :+
            ldx         cscope
            bne         :+
            ora         #SF_SHARED
:
            sta         spl + SP_FLAGS
            C2          sym_put
            rts

; ERASE a, ...: each array dropped (made again as it's next used, or by DIM)
cs_erase:
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syn3
:
            C2          arr_lookup
            lda         #OP_REFV
            C2          emit_slot
            lda         #LK_NUM
            ldx         scls
            cpx         #SC_SARR
            bne         :+
            lda         #LK_STR
:
            jsr         emit_i8
            lda         #ST_ERASE
            jsr         emit_st3
            C2          lex_next
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         cs_erase
:
            rts

; ---- CONST, OPTION BASE, DEF*

; CONST name = value, ...: each a constant, its value worked out now (it may name constants, not variables): its
; code compiled at the prompt's region's end and run
cs_const:
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syn3
:
            C2          const_find                          ; (Twice: Duplicate definition)
            bcs         :+
            jmp         dup3
:
            jsr         name_keep
            C2          lex_next
            lda         #'='
            jsr         need_op3
            lda         cb                                  ; (The code's cursor kept: the value's code, its run and
                                                            ;   its bytes' copy use cursor B)
            sta         c3_cb
            lda         cb + 1
            sta         c3_cb + 1
            lda         cbl
            sta         c3_cb + 2
            LDW         t0, CONST_CODE
            lda         #LB_IMM
            C2          cb_reg
            lda         #1
            sta         cconst
            C2          expr
            stz         cconst
            lda         #OP_END
            ldx         #1
            C2          emit_op
            MOVW        c3_vsp, vsp                         ; (Run on an empty stack)
            MOVW        vsp, stk_base
            MOVW        frp, stk_base
            lda         #<((LB_IMM << 13) + CONST_CODE)
            ldx         #>((LB_IMM << 13) + CONST_CODE)
            FARN        1, run_code
            MOVW        vt, stk_base                        ; (Its value)
            jsr         name_back
            lda         (vt)                                ; (Its type, against the name's)
            ldx         #ET_STR
            cmp         #VT_STR
            beq         :+
            ldx         #ET_NUM
:
            stx         c3_t
            lda         tsuf
            beq         @typed
            ldx         #ET_STR
            cmp         #'$'
            beq         :+
            ldx         #ET_NUM
:
            cpx         c3_t
            beq         @typed
            jmp         type3
@typed:
            C2          spl_clear
            lda         #SC_CONST
            sta         scls
            stz         sscope
            lda         (vt)
            bne         @bytes
            ldy         #4                                  ; (An integer: kind 0, its 4 bytes)
:
            lda         (vt),y
            sta         spl,y
            dey
            bne         :-
            C2          sym_add
            jmp         @next
@bytes:
            ldx         #1                                  ; (A number's or a string's bytes: kind 1 or 2, their
            cmp         #VT_STR                             ;   length; the bytes after the entry)
            bne         :+
            inx
:
            stx         spl
            ldy         #3
            lda         (vt),y
            cmp         #NOBANK
            bne         :+
            C2          sym_add                             ; ("": none)
            bra         @next
:
            jsr         const_ca                            ; (Its length)
            jsr         ca_rd
            sta         spl + 1
            jsr         ca_rd
            sta         spl + 2
            jsr         code_sel
            lda         spl + 2                             ; (A constant's bytes: 255 at most)
            bne         @long
            C2          sym_add
            MOVW        t0, symtop                          ; (Its bytes at symtop: cursor B there)
            lda         #LB_SYM
            C2          cb_reg
            jsr         const_ca
            jsr         ca_rd
            jsr         ca_rd
            lda         spl + 1
            sta         cn
            stz         cn + 1
            jsr         far_copy
            clc                                             ; (symtop past them, below the fixups)
            lda         symtop
            adc         spl + 1
            sta         symtop
            lda         symtop + 1
            adc         #0
            sta         symtop + 1
            clc
            adc         #1
            cmp         fixtop + 1
            bcc         @next
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error
@long:
            lda         #ER_STRING
            ldx         #0
            jmp         b_error
@next:
            MOVW        vsp, c3_vsp
            lda         c3_cb                               ; (The code's cursor back)
            sta         cb
            lda         c3_cb + 1
            sta         cb + 1
            lda         c3_cb + 2
            sta         cbl
            jsr         code_sel
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            jmp         cs_const
:
            rts

; Cursor A at the far address of the value at (vt)
const_ca:
            ldy         #1
            lda         (vt),y
            sta         ca
            iny
            lda         (vt),y
            sta         ca + 1
            iny
            lda         (vt),y
            sta         cal
            jmp         ca_sel

; OPTION BASE 0, OPTION BASE 1: the lowest index of an array's dimension whose highest alone is given
cs_option:
            lda         #K_BASE
            jsr         need_kw3
            lda         tk
            cmp         #TK_NUM
            bne         @bad
            lda         tisint
            beq         @bad
            lda         tint + 1
            ora         tint + 2
            ora         tint + 3
            bne         @bad
            lda         tint
            cmp         #2
            bcs         @bad
            sta         optbase
            C2          lex_next
            rts
@bad:
            jmp         syn3

; Pass 1: DEFSTR and the like (their letters: the kinds of names from here on, SUBs' parameters' too)
scan_defs:
            lda         tkv
            ldx         #def_kws_n - 1
:
            cmp         def_kws,x
            beq         :+
            dex
            bpl         :-
            rts
:
            sta         c3_kw
            C2          lex_next
            ; (fall through)

; DEFSTR a-z, ...: names starting with these letters strings'; DEFINT (DEFLNG, DEFSNG, DEFDBL): numbers' again
cs_defs:
@range:
            jsr         def_letter
            sta         c3_lo
            sta         c3_hi
            C2          lex_next
            lda         #'-'
            jsr         is_op3
            bne         @set
            C2          lex_next
            jsr         def_letter
            sta         c3_hi
            C2          lex_next
@set:
            ldx         c3_lo
@each:
            phx
            txa
            and         #7
            tay
            txa
            lsr         a
            lsr         a
            lsr         a
            tax
            lda         c3_kw
            cmp         #K_DEFSTR
            bne         :+
            lda         defstr,x
            ora         bit_of3,y
            bra         :++
:
            lda         bit_of3,y
            eor         #$FF
            and         defstr,x
:
            sta         defstr,x
            plx
            cpx         c3_hi
            bcs         @next
            inx
            bra         @each
@next:
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         @range
:
            rts

; A letter (DEF*'s): .A its number (A 0)
def_letter:
            lda         tk
            cmp         #TK_NAME
            bne         @bad
            lda         tnl
            cmp         #1
            bne         @bad
            lda         tsuf
            bne         @bad
            lda         tname
            sec
            sbc         #'A'
            cmp         #26
            bcs         @bad
            rts
@bad:
            jmp         syn3
def_kws:    .byte       K_DEFSTR, K_DEFINT, K_DEFLNG, K_DEFSNG, K_DEFDBL
def_kws_n   = * - def_kws
bit_of3:    .byte       $01, $02, $04, $08, $10, $20, $40, $80

; ---- SHARED, STATIC

; SHARED name [()] [AS type], ...: in a procedure, the main program's variables (or arrays) its own too
cs_shared:
            stz         dm_shared
            lda         cscope
            bne         @each
            jmp         insub3
@each:
            jsr         decl_name
            lda         cscope                              ; (Here already: Duplicate definition)
            sta         sscope
            C2          sym_find
            bcs         :+
            jmp         dup3
:
            stz         sscope                              ; (The main program's: found, or made)
            C2          sym_find
            bcc         :+
            jsr         decl_kind
            C2          var_new
:
            jsr         decl_flags                          ; (AS: declared so)
            lda         cscope                              ; (Here: its slot)
            sta         sscope
            C2          sym_add
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         @each
:
            rts

; STATIC name [()] [AS type], ...: in a procedure, a variable of its own kept from call to call (a global's slot)
cs_static:
            lda         cscope
            bne         @each
            jmp         insub3
@each:
            jsr         decl_name
            lda         cscope
            sta         sscope
            C2          sym_find
            bcs         :+
            jmp         dup3
:
            C2          spl_clear
            jsr         decl_kind
            C2          global_new
            sta         spl + SP_SLOT
            stx         spl + SP_SLOT + 1
            lda         dm_as                               ; (AS: declared so)
            cmp         #$FF
            beq         :+
            lda         #SF_DECL
            sta         spl + SP_FLAGS
:
            lda         cscope
            sta         sscope
            C2          sym_add
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         @each
:
            rts

; ---- DATA, READ, RESTORE

; DATA items (its keyword current, the line from after it): each put in DATA's region (its length, its bytes): "text",
; or text up to a , (or a :, or the line's end), the spaces around it dropped.  At the prompt: passed over
cs_data:
@item:
            lda         (lp)                                ; (The spaces before it)
            cmp         #' '
            bne         :+
            jsr         lp_inc3
            bra         @item
:
            cmp         #'"'
            bne         @plain
            jsr         lp_inc3                             ; ("text": up to its ")
            ldy         #0
:
            lda         (lp),y
            beq         :+
            cmp         #'"'
            beq         :+
            iny
            bne         :-
            jmp         syn3
:
            sty         c3_len
            jsr         data_put
            jsr         lp_len3
            lda         (lp)
            beq         @end
            jsr         lp_inc3                             ; (Past its ", and the spaces after it)
:
            lda         (lp)
            cmp         #' '
            bne         :+
            jsr         lp_inc3
            bra         :-
:
            cmp         #','
            beq         @comma
            cmp         #':'
            beq         @end
            cmp         #0
            beq         @end
            jmp         syn3
@plain:
            ldy         #0                                  ; (Up to a , : or the end; the spaces at its end dropped)
:
            lda         (lp),y
            beq         :+
            cmp         #','
            beq         :+
            cmp         #':'
            beq         :+
            iny
            bne         :-
            jmp         syn3
:
            phy
:
            cpy         #0
            beq         :+
            dey
            lda         (lp),y
            cmp         #' '
            beq         :-
            iny
:
            sty         c3_len
            jsr         data_put
            ply
            sty         c3_len
            jsr         lp_len3
            lda         (lp)
            cmp         #','
            bne         @end
@comma:
            jsr         lp_inc3
            jmp         @item
@end:
            C2          lex_next                            ; (The token after it: a : or the line's end)
            rts

; lp on past 1 (lp_inc3), past c3_len (lp_len3)
lp_inc3:
            lda         #1
            sta         c3_len
lp_len3:
            clc
            lda         lp
            adc         c3_len
            sta         lp
            bcc         :+
            inc         lp + 1
:
            rts

; c3_len bytes at lp put in DATA's region, an item: its length (2 bytes), its bytes (at the prompt: not).  Past the
; region's end: Out of memory
data_put:
            lda         cmode
            bne         @past
            clc                                             ; (Room: the item, and the end's mark)
            lda         dtop
            adc         c3_len
            tax
            lda         dtop + 1
            adc         #0
            tay
            txa
            clc
            adc         #4
            tya
            adc         #0
            cmp         #>DATA_END
            bcc         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error
:
            MOVW        t0, dtop
            lda         #LB_DATA
            C2          ca_reg
            lda         c3_len
            jsr         ca_wr
            lda         #0
            jsr         ca_wr
            ldy         #0
:
            cpy         c3_len
            beq         :+
            lda         (lp),y
            jsr         ca_wr
            iny
            bra         :-
:
            jsr         code_sel
            sec                                             ; (dtop past it: its length + 2)
            lda         dtop
            adc         #1
            sta         dtop
            bcc         :+
            inc         dtop + 1
:
            clc
            lda         dtop
            adc         c3_len
            sta         dtop
            bcc         @past
            inc         dtop + 1
@past:
            rts

; READ v, ...: each DATA's next item (a number read as VAL reads it)
cs_read:
            C2          tgt_begin
            lda         #ST_READ
            jsr         emit_st3
            lda         tgt_type
            beq         @num
            cmp         #ET_STR
            beq         @put
            jmp         type3
@num:
            lda         #OP_FN
            ldx         #2
            C2          emit_op
            lda         #F_VAL
            C2          emit
@put:
            C2          tgt_end
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         cs_read
:
            rts

; RESTORE [label]: READ from DATA's first item, or from the first after the label
cs_restore:
            jsr         at_end3
            bcs         @first
            lda         #OP_PUSHI32
            ldx         #5
            C2          emit_op
            lda         #1
            sta         lref_data
            C2          label_operand
            stz         lref_data
            lda         #0
            tax
            C2          emit_w
            bra         @st
@first:
            lda         #0
            jsr         emit_i8
@st:
            lda         #ST_RESTORE
            jmp         emit_st3

; ---- SWAP, RANDOMIZE, BASE, DIGITS

; SWAP a, b: two variables' (or elements') values exchanged, of one type
cs_swap:
            C2          place_emit
            lda         tgt_type
            pha
            lda         #','
            jsr         need_op3
            C2          place_emit
            pla
            cmp         tgt_type
            beq         :+
            jmp         type3
:
            lda         #OP_SWAP
            ldx         #1
            C2          emit_op
            rts

; RANDOMIZE [n]: RND seeded with n (none: from the clock)
cs_randomize:
            jsr         at_end3
            bcc         :+
            lda         #0
            jsr         emit_i8
            bra         @st
:
            C2          expr_num
@st:
            lda         #ST_RANDOMIZE
            jmp         emit_st3

; BASE b$: the base numbers are shown and read in (at run time); a string alone written in the program, the
; program's numbers after it read in it too
cs_base:
            stz         bs_tmp
            lda         tk
            cmp         #TK_STR
            bne         @expr
            lda         tsl + 1
            bne         @expr
            ldy         tsl
            cpy         #BASE_ROOM
            bcs         @expr
            lda         #0
            sta         bs_tmp,y
:
            dey
            bmi         @expr
            lda         (tsp),y
            sta         bs_tmp,y
            bra         :-
@expr:
            C2          expr_str
            jsr         at_end3
            bcc         @st
            lda         bs_tmp
            beq         @st
            ldx         #BASE_ROOM - 1
:
            lda         bs_tmp,x
            sta         cbase,x
            dex
            bpl         :-
@st:
            lda         #ST_BASE
            jmp         emit_st3

; DIGITS n: the math functions' precision
cs_digits:
            C2          expr_num
            lda         #ST_DIGITS
            jmp         emit_st3

; ---- INPUT, LINE INPUT

; INPUT [;] ["prompt" ; | ,] v, ...: a line typed, its fields to each v; typed again till they fit (Redo from start)
cs_input:
            lda         #'#'
            jsr         is_op3
            bne         :+
            jmp         not_yet3                            ; (INPUT #n: files', to come)
:
            lda         #';'
            jsr         is_op3
            bne         :+
            C2          lex_next
:
            C2          cpc_get                             ; (Redo from start's way back: here)
            sta         in_loop
            stx         in_loop + 1
            lda         #1                                  ; (? after the prompt, or after none)
            jsr         in_prompt
            jsr         emit_i8
            lda         #ST_INPUT
            jsr         emit_st3
@each:
            C2          tgt_begin
            lda         tgt_type
            cmp         #ET_REC
            bne         :+
            jmp         type3
:
            jsr         emit_i8
            lda         #ST_INFLD
            jsr         emit_st3
            C2          tgt_end
            lda         #','
            jsr         is_op3
            bne         :+
            C2          lex_next
            bra         @each
:
            lda         #ST_INEND
            jsr         emit_st3
            MOVW        t4, in_loop
            lda         #OP_JT
            C2          emit_jmp
            rts

; LINE INPUT [;] ["prompt" ;] v$: a whole line (LINE's graphics to come)
cs_line:
            lda         #K_INPUT
            jsr         is_kw3
            beq         :+
            jmp         not_yet3
:
            C2          lex_next
            lda         #'#'
            jsr         is_op3
            bne         :+
            jmp         not_yet3
:
            lda         #';'
            jsr         is_op3
            bne         :+
            C2          lex_next
:
            lda         #0
            jsr         in_prompt
            lda         #0
            jsr         emit_i8
            lda         #ST_INPUT
            jsr         emit_st3
            C2          tgt_begin
            lda         tgt_type
            cmp         #ET_STR
            beq         :+
            jmp         type3
:
            lda         #ST_LGET
            jsr         emit_st3
            C2          tgt_end
            rts

; INPUT's and LINE INPUT's prompt pushed: "text" then ; (the flags .A) or , (none); no prompt: "" (the flags .A).
; OUT: .A the flags (bit 0: ? after it)
in_prompt:
            sta         c3_t
            lda         tk
            cmp         #TK_STR
            bne         @none
            C2          emit_str
            C2          lex_next
            lda         #';'
            jsr         is_op3
            beq         @semi
            lda         #','
            jsr         need_op3
            lda         #0
            rts
@semi:
            C2          lex_next
            lda         c3_t
            rts
@none:
            lda         #OP_PUSHE
            ldx         #1
            C2          emit_op
            lda         c3_t
            rts

; ---- To come

; Pass 1: a TYPE and its fields
scan_type:
            rts

; END TYPE; PRINT #, PRINT USING, ON ERROR
end_proc:
print_file:
print_using:
on_error:
            jmp         not_yet3

; A TYPE's name (tname): C = 0 and .A its number; C = 1 none (as yet: none)
type_find:
            sec
            rts
