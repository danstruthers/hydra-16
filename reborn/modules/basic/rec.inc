; ****************************************************************************
; rec.inc - records (the third bank): QuickBASIC's TYPE ... END TYPE, its fields read in the first pass (numbers,
; strings, fixed-length strings, records of a type before it) and passed over in the second.  A record is a block of
; values in the heap (an array's of one element, no dimensions), a value a field, a record's fields' flattened into
; it (p.pos.x one value); an array of records has its elements so, a record's values each.  Its type's values' kinds
; (ty_kinds) make its defaults.  A field's read and written by its first value's number (FGET k, FPUT k; AGETF n k,
; APUTF n k), a record copied whole (ST_RCOPY).

TY_MAX          = 32            ; Types at most (PK_REC's 5 bits)
TY_LEAVES       = 64            ; A record's values at most
TY_KINDS        = 255           ; All the types' values' kinds at most

.bss
ty_n:       .res        TY_MAX                              ; Each type's values ...
ty_off:     .res        TY_MAX                              ;   their kinds' place in ty_kinds ...
ty_ent:     .res        TY_MAX * 2                          ;   and its symbol's entry
ty_kinds:   .res        TY_KINDS                            ; The values' kinds (LK_NUM, LK_STR), each type's in turn
ntypes:     .res        1
ty_top:     .res        1                                   ; (ty_kinds' next)
ty_open:    .res        1                                   ; <> 0: a TYPE's lines (read in pass 1, passed over in 2)
rc_dt:      .res        1                                   ; (r_copy's: the destination's type, its first value's
rc_doff:    .res        1                                   ;   offset, the source's)
rc_soff:    .res        1
ty_cur:     .res        1                                   ; (The TYPE being read: its number, its fields' place, its
ty_place:   .res        2                                   ;   fields so far)
ty_nf:      .res        1
tp_n:       .res        1                                   ; (ty_pay's: a type's values, its fields' place and count)
tp_place:   .res        2
tp_nf:      .res        1
fs_name:    .res        NAME_MAX + 2                        ; (A field's name sought, its length)
fs_len:     .res        1
fn_tmp:     .res        NAME_MAX + 2                        ; (A field's name read, its length)
fn_len:     .res        1
fp_buf:     .res        NAME_MAX + 2                        ; (A field's path: x, or pos.x; its length, the next)
fp_len:     .res        1
fp_i:       .res        1
fl_kind:    .res        1                                   ; (A field found: its kind (0 a number, 1 a string, 2 a
fl_info:    .res        2                                   ;   fixed-length one, 3 a record), its length or type, its
fl_foff:    .res        1                                   ;   first value's offset in its type ...
fl_off:     .res        1                                   ;   and in the record (its path's)
fl_type:    .res        1
fr_full:    .res        NAME_MAX + 2                        ; (A dotted name: kept whole; its length)
fr_fl:      .res        1
fr_slot:    .res        2                                   ; (The record variable's slot, its type; the indices)
fr_type:    .res        1
fr_n:       .res        1

.segment "CODE3"

; ---- TYPE ... END TYPE

; Pass 1: TYPE name (stmt.inc's c_scan, TYPE current): the type made (its number the next), its fields' lines next
scan_type:
            C2          lex_next
            jsr         kw_name3                            ; (A keyword's name will do: TYPE point)
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syn3
:
            lda         ntypes
            cmp         #TY_MAX
            bcc         :+
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error
:
            lda         #SC_TYPE                            ; (Twice: Duplicate definition)
            sta         scls
            stz         sscope
            C2          sym_find
            bcs         :+
            jmp         dup3
:
            C2          spl_clear
            lda         ntypes
            sta         spl
            sta         ty_cur
            C2          sym_add
            MOVW        ty_place, symtop                    ; (Its fields after it)
            lda         ty_cur
            asl         a
            tax
            lda         sent
            sta         ty_ent,x
            lda         sent + 1
            sta         ty_ent + 1,x
            ldx         ty_cur
            lda         ty_top
            sta         ty_off,x
            stz         ty_n,x
            stz         ty_nf
            lda         #1
            sta         ty_open
            rts

; Pass 1: a line of a TYPE's (stmt.inc's c_scan, while ty_open): a field, name AS type; or END TYPE (the type's
; entry written: its values, fields)
scan_field:
            C2          lex_next
            lda         tk                                  ; (A number first: the line's, past it)
            cmp         #TK_NUM
            bne         :+
            C2          lex_next
:
            lda         tk
            bne         :+
            rts
:
            lda         #K_END
            jsr         is_kw3
            bne         @field
            C2          lex_next
            lda         #K_TYPE
            jsr         need_kw3
            stz         ty_open
            C2          spl_clear                           ; (Its entry: number, values, fields' place, fields)
            ldx         ty_cur
            stx         spl
            lda         ty_n,x
            sta         spl + 1
            MOVW        spl + 2, ty_place
            lda         ty_nf
            sta         spl + 4
            txa
            asl         a
            tax
            lda         ty_ent,x
            sta         sent
            lda         ty_ent + 1,x
            sta         sent + 1
            C2          sym_put
            inc         ntypes
            rts
@field:
            jsr         kw_name3
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syn3
:
            jsr         name_keep
            C2          lex_next
            lda         #K_AS
            jsr         need_kw3
            C2          as_type                             ; (PK_NUM, PK_STR (as_fix), PK_REC | type << 3)
            pha
            and         #7
            ldx         #0                                  ; (Its kind, info, values)
            stz         fl_info
            stz         fl_info + 1
            lda         #1
            sta         fl_foff
            pla
            pha
            and         #7
            cmp         #PK_NUM
            beq         @kind
            inx
            cmp         #PK_STR
            bne         @rec
            lda         as_fix
            ora         as_fix + 1
            beq         @kind
            inx
            MOVW        fl_info, as_fix
            bra         @kind
@rec:
            ldx         #3
            pla
            pha
            lsr         a
            lsr         a
            lsr         a
            sta         fl_info
            tay
            lda         ty_n,y
            sta         fl_foff
@kind:
            stx         fl_kind
            pla
            ldx         ty_cur                              ; (Room: TY_LEAVES a record, TY_KINDS all of them)
            clc
            lda         ty_n,x
            adc         fl_foff
            cmp         #TY_LEAVES + 1
            bcc         @lb902
            jmp         @full
@lb902:
            clc
            lda         ty_top
            adc         fl_foff
            bcc         @lb901
            jmp         @full
@lb901:
            jsr         name_back                           ; (The field at symtop: its length, name, kind, info,
            clc                                             ;   offset: below the fixups)
            lda         symtop
            adc         tnl
            lda         symtop + 1
            adc         #1
            cmp         fixtop + 1
            bcc         @lb900
            jmp         @full
@lb900:
            MOVW        t0, symtop
            lda         #LB_SYM
            C2          ca_reg
            lda         tnl
            jsr         ca_wr
            ldx         #0
:
            lda         tname,x
            jsr         ca_wr
            inx
            cpx         tnl
            bne         :-
            lda         fl_kind
            jsr         ca_wr
            lda         fl_info
            jsr         ca_wr
            lda         fl_info + 1
            jsr         ca_wr
            ldx         ty_cur
            lda         ty_n,x
            jsr         ca_wr
            jsr         code_sel
            clc
            lda         symtop
            adc         tnl
            tax
            lda         symtop + 1
            adc         #0
            tay
            txa
            clc
            adc         #5
            sta         symtop
            tya
            adc         #0
            sta         symtop + 1
            ldy         ty_top                              ; (Its values' kinds)
            lda         fl_kind
            cmp         #3
            beq         @recv
            ldx         #LK_NUM
            cmp         #0
            beq         :+
            ldx         #LK_STR
:
            txa
            sta         ty_kinds,y
            iny
            bra         @kinds
@recv:
            ldx         fl_info                             ; (A record's: its type's)
            lda         ty_off,x
            tax
            lda         fl_foff
            sta         fp_i
:
            lda         ty_kinds,x
            sta         ty_kinds,y
            inx
            iny
            dec         fp_i
            bne         :-
@kinds:
            sty         ty_top
            ldx         ty_cur
            clc
            lda         ty_n,x
            adc         fl_foff
            sta         ty_n,x
            inc         ty_nf
            rts
@full:
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error

; Pass 2: a TYPE's line passed over (stmt.inc's c_line, while ty_open), END TYPE its end
ty_skip:
            C2          lex_next
            lda         tk                                  ; (A number first: the line's, past it)
            cmp         #TK_NUM
            bne         :+
            C2          lex_next
:
            lda         #K_END
            jsr         is_kw3
            bne         @done
            C2          lex_next
            lda         #K_TYPE
            jsr         is_kw3
            bne         @done
            stz         ty_open
            C2          lex_next
@done:
            rts

; Pass 2: TYPE name: its lines passed over to END TYPE
cs_type:
            lda         #1
            sta         ty_open
            C2          lex_next
            rts

; A TYPE's name (tname): C = 0 and .A its number; C = 1 none
type_find:
            lda         #SC_TYPE
            sta         scls
            stz         sscope
            C2          sym_find
            bcs         :+
            lda         spl
            clc
:
            rts

; A keyword token made a name's (tname its text, TK_NAME): a TYPE's or a field's may be one's
kw_name3:
            lda         tk
            cmp         #TK_KW
            bne         :+
            lda         tkv
            C2          kw_tname
            bcs         :+
            lda         #TK_NAME
            sta         tk
:
            rts

; ---- Fields

; Type .A's entry's payload: tp_n its values, tp_place its fields' place, tp_nf their count
ty_pay:
            asl         a
            tax
            lda         ty_ent + 1,x
            pha
            lda         ty_ent,x
            plx
            C2          sym_ca
            ldx         #4                                  ; (Past its next, class, scope)
:
            jsr         ca_rd
            dex
            bne         :-
            jsr         ca_rd                               ; (Its name)
            tax
:
            jsr         ca_rd
            dex
            bne         :-
            jsr         ca_rd                               ; (Its number, values, fields' place, fields)
            jsr         ca_rd
            sta         tp_n
            jsr         ca_rd
            sta         tp_place
            jsr         ca_rd
            sta         tp_place + 1
            jsr         ca_rd
            sta         tp_nf
            jmp         code_sel

; The field fs_name of type .A: C = 0, fl_kind, fl_info, fl_foff its; C = 1 none
fld_find:
            jsr         ty_pay
            MOVW        t0, tp_place
            lda         #LB_SYM
            C2          ca_reg
            ldx         tp_nf
@field:
            bne         :+
            jsr         code_sel
            sec
            rts
:
            jsr         ca_rd                               ; (Its name, to fn_tmp)
            sta         fn_len
            ldy         #0
:
            cpy         fn_len
            beq         :+
            jsr         ca_rd
            sta         fn_tmp,y
            iny
            bra         :-
:
            jsr         ca_rd                               ; (Its kind, info, offset)
            sta         fl_kind
            jsr         ca_rd
            sta         fl_info
            jsr         ca_rd
            sta         fl_info + 1
            jsr         ca_rd
            sta         fl_foff
            lda         fn_len                              ; (Its name fs_name?)
            cmp         fs_len
            bne         @next
            tay
:
            dey
            bmi         @found
            lda         fn_tmp,y
            cmp         fs_name,y
            beq         :-
@next:
            dex
            bra         @field
@found:
            jsr         code_sel
            clc
            rts

; The field path fp_buf (fp_len: x, pos.x) in type .A: C = 0, fl_off its first value's in the record, fl_kind and
; fl_info its last field's; C = 1 none
fld_path:
            sta         fl_type
            stz         fl_off
            stz         fp_i
@seg:
            ldx         fp_i                                ; (Its next name: to a . or the end)
            ldy         #0
:
            cpx         fp_len
            beq         :+
            lda         fp_buf,x
            cmp         #'.'
            beq         :+
            sta         fs_name,y
            inx
            iny
            bra         :-
:
            sty         fs_len
            phx
            cpy         #0
            beq         @bad
            lda         fl_type
            jsr         fld_find
            bcs         @bad
            clc
            lda         fl_off
            adc         fl_foff
            sta         fl_off
            plx
            cpx         fp_len
            beq         @done
            inx                                             ; (A . and more: a record's field)
            stx         fp_i
            lda         fl_kind
            cmp         #3
            bne         @bad2
            lda         fl_info
            sta         fl_type
            bra         @seg
@done:
            clc
            rts
@bad:
            plx
@bad2:
            sec
            rts

; A dotted name (tname: rec.field ...): tname the record's name, fp_buf the path after its first . (fr_full the whole
; name).  OUT: C = 1 not dotted
fr_split:
            ldx         #NAME_MAX + 1                       ; (Kept)
:
            lda         tname,x
            sta         fr_full,x
            dex
            bpl         :-
            lda         tnl
            sta         fr_fl
            ldx         #0                                  ; (Its first .)
:
            cpx         tnl
            beq         @none
            lda         tname,x
            cmp         #'.'
            beq         :+
            inx
            bra         :-
:
            cpx         #0
            beq         @none
            stx         tnl                                 ; (The record's name: before it)
            ldy         #0
            inx
:
            cpx         fr_fl
            beq         :+
            lda         fr_full,x
            sta         fp_buf,y
            inx
            iny
            bra         :-
:
            sty         fp_len
            clc
            rts
@none:
            sec
            rts

; tname as it was (fr_split's)
fr_back:
            ldx         #NAME_MAX + 1
:
            lda         fr_full,x
            sta         tname,x
            dex
            bpl         :-
            lda         fr_fl
            sta         tnl
            rts

; The record variable tname (here: a procedure's own, a shared one): C = 0, fr_slot its slot, fr_type its type
fr_var:
            lda         #SC_REC
            sta         scls
            C2          var_find
            bcs         :+
            MOVW        fr_slot, spl + SP_SLOT
            lda         spl + SP_TYPE
            sta         fr_type
            clc
:
            rts

; The type (ET_NUM, ET_STR) of the field found (fl_kind), or Type mismatch (a record)
fl_etype:
            lda         fl_kind
            cmp         #3
            bcc         :+
            jmp         type3
:
            cmp         #0
            beq         :+
            lda         #ET_STR
:
            rts

; REFV fr_slot emitted
fr_refv:
            lda         #OP_REFV
            ldx         #3
            C2          emit_op
            lda         fr_slot
            ldx         fr_slot + 1
            C2          emit_w
            rts

; The op .A and fl_off (FGET, FPUT) emitted
fr_op:
            ldx         #2
            C2          emit_op
            lda         fl_off
            C2          emit
            rts

; The op .A, fr_n and fl_off (AGETF, APUTF) emitted
fr_aop:
            ldx         #3
            C2          emit_op
            lda         fr_n
            C2          emit
            lda         fl_off
            C2          emit
            rts

; A field of a record variable in an expression (expr.inc's e_name: tname dotted): its value pushed (REFV, FGET),
; etype its type, the token after it current.  OUT: C = 0; C = 1 not a record variable's (tname as it was)
e_field:
            jsr         fr_split
            bcs         @no
            jsr         fr_var
            bcs         @back
            lda         fr_type
            jsr         fld_path
            bcs         @nofield
            jsr         fl_etype
            sta         etype
            jsr         fr_refv
            lda         #OP_FGET
            jsr         fr_op
            jsr         fl_fix
            C2          lex_next
            clc
            rts
@back:
            jsr         fr_back
@no:
            sec
            rts
@nofield:
            jmp         no_field

no_field:
            lda         #ER_FIELD
            ldx         #0
            jmp         b_error

; An array of records' element's field (expr.inc's e_name: the array's REFV and its .A indices compiled, its type .X;
; the . after its ) current): AGETF n k, etype its type
e_afield:
            sta         fr_n
            stx         fr_type
            jsr         fp_name
            jsr         fl_etype
            sta         etype
            lda         #OP_AGETF
            jsr         fr_aop
            jsr         fl_fix
            C2          lex_next
            rts

; A fixed-length string's field (fl_kind 2): FIXLEN, its length (a record not written yet has "" there)
fl_fix:
            lda         fl_kind
            cmp         #2
            bne         :+
            lda         #OP_FIXLEN
            ldx         #3
            C2          emit_op
            lda         fl_info
            ldx         fl_info + 1
            C2          emit_w
:
            rts

; A call's argument that's a field alone (rec.field, arr(i).field), a number's or a string's, for a parameter of
; that kind (ao_kind; another: Type mismatch): its place compiled (REFV, [its indices, AREF n,] FREF k), the token
; after it current; C = 0.  C = 1 not one, nothing compiled (a fixed-length string's, a record's: a copy)
arg_field:
            lda         tk
            cmp         #TK_NAME
            bne         @no
            ldy         #0
            C2          sc_space
            lda         (lp),y
            cmp         #'('
            beq         @elem
            C2          has_dot                             ; rec.field
            bcs         @no
            ldy         #0
            C2          sc_space
            C2          sc_ends
            bcs         @no
            jsr         fr_split
            bcs         @no
            jsr         fr_var
            bcs         @back
            lda         fr_type
            jsr         fld_path
            bcs         @back
            jsr         af_kind
            bcs         @back
            jsr         fr_refv
            lda         #OP_FREF
            jsr         fr_op
            C2          lex_next
            clc
            rts
@back:
            jsr         fr_back
@no:
            sec
            rts
@elem:
            iny                                             ; arr(i).field: its path after the )
            C2          sc_group
            bcs         @no
            lda         (lp),y
            cmp         #'.'
            bne         @no
            iny
            ldx         #0
:
            lda         (lp),y
            C2          is_namec
            bcs         :+
            cpx         #NAME_MAX
            bcs         @no
            cmp         #'a'                                ; (In capitals, as names are kept)
            bcc         @upper
            cmp         #'z' + 1
            bcs         @upper
            and         #$DF
@upper:
            sta         fp_buf,x
            inx
            iny
            bra         :-
:
            cpx         #0
            beq         @no
            stx         fp_len
            C2          sc_space
            C2          sc_ends
            bcs         @no
            lda         #SC_RARR                            ; (An array of records already)
            sta         scls
            C2          var_find
            bcs         @no
            MOVW        fr_slot, spl + SP_SLOT
            lda         spl + SP_TYPE
            jsr         fld_path
            bcs         @no
            jsr         af_kind
            bcs         @no
            jsr         fr_refv
            C2          lex_next
            C2          lex_next
            lda         fl_off
            pha
            C2          emit_indices
            pha
            lda         #OP_AREF
            ldx         #2
            C2          emit_op
            pla
            C2          emit
            pla
            sta         fl_off
            lda         #OP_FREF
            jsr         fr_op
            C2          lex_next                            ; (The . current: past its path)
            C2          lex_next
            clc
            rts

; The field found (fl_kind) a number's or a string's: C = 0 (ao_kind another's: Type mismatch); else C = 1
af_kind:
            lda         fl_kind
            cmp         #2
            bcs         @no
            cmp         ao_kind
            bne         @type
            clc
@no:
            rts
@type:
            jmp         type3

; The field's path after a . (the . current): read, found in fr_type (fl_*); the path's token current.  None: no such
; field
fp_name:
            C2          lex_next
            jsr         kw_name3
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         syn3
:
            ldx         tnl
            stx         fp_len
            dex
:
            lda         tname,x
            sta         fp_buf,x
            dex
            bpl         :-
            lda         fr_type
            jsr         fld_path
            bcc         :+
            jmp         no_field
:
            rts

; ---- Assignments

; rec.field = value (stmt.inc's st_assign: tname dotted).  OUT: C = 0 done; C = 1 not a record variable's field
a_field:
            jsr         fr_split
            bcs         @no
            jsr         fr_var
            bcs         @back
            lda         fr_type
            jsr         fld_path
            bcc         :+
            jmp         no_field
:
            jsr         fr_refv
            C2          lex_next
            lda         fl_kind                             ; (A field that's a record: copied whole)
            cmp         #3
            bne         :+
            lda         fl_off
            sta         rc_doff
            lda         fl_info
            sta         fr_type
            jsr         r_copy
            clc
            rts
:
            lda         #'='
            jsr         need_op3
            jsr         fl_value
            lda         #OP_FPUT
            jsr         fr_op
            clc
            rts
@back:
            jsr         fr_back
@no:
            sec
            rts

; The field's value (fl_* kept over it): its expression, of its type; a fixed-length string's padded or cut
fl_value:
            lda         fl_off
            pha
            lda         fl_kind
            pha
            lda         fl_info
            pha
            lda         fl_info + 1
            pha
            C2          expr
            pla
            sta         fl_info + 1
            pla
            sta         fl_info
            pla
            sta         fl_kind
            pla
            sta         fl_off
            jsr         fl_etype
            cmp         etype
            beq         :+
            jmp         type3
:
            jmp         fl_fix

; The field found (fl_*) a record's: the source's offset and type (rc_soff, fr_type).  Else Type mismatch
rc_field:
            lda         fl_kind
            cmp         #3
            beq         :+
            jmp         type3
:
            lda         fl_off
            sta         rc_soff
            lda         fl_info
            sta         fr_type
            rts

; arr(i).field = value (st_assign: the array's REFV and its .A indices compiled, its type .X; the . current); a field
; that's a record: copied whole (the element's place, AREF n)
a_efield:
            sta         fr_n
            stx         fr_type
            jsr         fp_name
            C2          lex_next
            lda         fl_kind
            cmp         #3
            bne         :+
            lda         fl_off
            sta         rc_doff
            lda         fl_info
            sta         fr_type
            lda         #OP_AREF
            ldx         #2
            C2          emit_op
            lda         fr_n
            C2          emit
            jmp         r_copy
:
            lda         #'='
            jsr         need_op3
            jsr         fl_value
            lda         #OP_APUTF
            jmp         fr_aop

; rec = record (st_assign: spl the record variable, its name current): copied whole
a_record:
            MOVW        fr_slot, spl + SP_SLOT
            lda         spl + SP_TYPE
            sta         fr_type
            jsr         fr_refv
            C2          lex_next
            stz         rc_doff
            jmp         r_copy

; arr(i) = record (st_assign: the array's REFV and its .A indices compiled, its type .X): the element's place
a_erecord:
            stx         fr_type
            stz         rc_doff
            pha
            lda         #OP_AREF
            ldx         #2
            C2          emit_op
            pla
            C2          emit
            ; (fall through)

; = record: the source's place (a record variable, an element of an array of records, or a field of either that's a
; record: of the type fr_type), the offsets of the destination's first value (rc_doff) and the source's, its values'
; count, ST_RCOPY
r_copy:
            lda         fr_type
            sta         rc_dt
            stz         rc_soff
            lda         #'='
            jsr         need_op3
            lda         tk
            cmp         #TK_NAME
            beq         :+
            jmp         type3
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
            jsr         fr_split                            ; (rec.field: a field that's a record)
            bcs         @var
            jsr         fr_var
            bcs         @type
            lda         fr_type
            jsr         fld_path
            bcc         :+
            jmp         no_field
:
            jsr         rc_field
            bra         @have
@var:
            jsr         fr_var                              ; (A record variable)
            bcs         @type
@have:
            lda         fr_type
            cmp         rc_dt
            bne         @type
            jsr         fr_refv
            C2          lex_next
            bra         @n
@elem:
            C2          arr_lookup
            lda         scls
            cmp         #SC_RARR
            bne         @type
            lda         spl + SP_TYPE
            sta         fr_type
            lda         #OP_REFV
            C2          emit_slot
            C2          lex_next
            C2          lex_next
            C2          emit_indices
            pha
            lda         #OP_AREF
            ldx         #2
            C2          emit_op
            pla
            C2          emit
            lda         #'.'                                ; (arr(i).field: a field that's a record)
            jsr         is_op3
            bne         :+
            jsr         fp_name
            jsr         rc_field
            C2          lex_next
:
            lda         fr_type
            cmp         rc_dt
            beq         @n
@type:
            jmp         type3
@n:
            lda         rc_doff                             ; (The offsets, its values: the type's)
            jsr         emit_i8
            lda         rc_soff
            jsr         emit_i8
            ldx         rc_dt
            lda         ty_n,x
            jsr         emit_i8
            lda         #ST_RCOPY
            jmp         emit_st3

; ---- A field as a target (READ's, INPUT's: stmt.inc's tgt_begin, tgt_end)

; rec.field (tname dotted): REFV emitted; tgt_type, tgt_fk (its first value + 1), tgt_flags and tgt_fix (fixed).
; OUT: C = 0; C = 1 not a record variable's
t_field:
            jsr         fr_split
            bcs         @no
            jsr         fr_var
            bcs         @back
            lda         fr_type
            jsr         fld_path
            bcc         :+
            jmp         no_field
:
            jsr         fr_refv
            jsr         t_set
            stz         tgt_n
            C2          lex_next
            clc
            rts
@back:
            jsr         fr_back
@no:
            sec
            rts

; arr(i).field (the indices compiled: .A their count, .X the type; the . current)
t_efield:
            sta         tgt_n
            stx         fr_type
            jsr         fp_name
            jsr         t_set
            C2          lex_next
            rts

; tgt_type, tgt_fk, tgt_flags, tgt_fix from the field found
t_set:
            jsr         fl_etype
            sta         tgt_type
            ldx         fl_off
            inx
            stx         tgt_fk
            stz         tgt_flags
            lda         fl_kind
            cmp         #2
            bne         :+
            lda         #SF_FIXED
            sta         tgt_flags
            MOVW        tgt_fix, fl_info
:
            rts
