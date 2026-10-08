; ****************************************************************************
; stmt3.inc - the third bank's statements: declarations (DIM, CONST, TYPE, DEF*, OPTION), procedures (SUB, FUNCTION,
; CALL, their calls), data, files, the console's, the system's, sound and graphics; and the built-in functions of a
; form of their own (fn_special3).  Each as stmt.inc's: from its first token (c_stmt3: its keyword current).

.segment "CODE3"

; A statement the second bank hasn't: its keyword current
c_stmt3:
not_yet3:
            lda         #73
            ldx         #0
            jmp         b_error

; Pass 1: a SUB's or FUNCTION's name and parameters; a TYPE and its fields
scan_type:
            rts

; END TYPE; the built-in functions of a form of their own; PRINT #, PRINT USING, ON ERROR
end_proc:
fn_special3:
print_file:
print_using:
on_error:
            jmp         not_yet3

; A TYPE's name (tname): C = 0 and .A its number; C = 1 none (as yet: none)
type_find:
            sec
            rts
