; ****************************************************************************
; fn.inc - the built-in functions' and the statements' routines (OP_FN f, OP_ST s), numbered in groups by the bank
; their code is in: the numbers' (the fourth), the strings' (the fifth), input and output's (the sixth), the
; system's (the seventh).  op_fn and op_st (run.inc) find the bank by the number, and call that bank's dispatcher
; (fn_disp4 ...: a jump table) with the number less its group's first.  A routine takes its arguments from the value
; stack and leaves its result there (a function), ip as it was (op_fn and op_st go past their 2 bytes).

.enum                                                       ; Functions
F_NONE                                                      ; (CSNG, CDBL: the operand itself, no op)
F_B4                                                        ; The numbers' (num.inc)
F_ABS = F_B4
F_ATN
F_BIT
F_CINT
F_COMPLEX
F_COS
F_CVN
F_DEN
F_DIGITS
F_EXP
F_FIB
F_FIX
F_FIXED
F_GCD
F_IMAG
F_INT
F_LOG
F_NUMER
F_PI
F_RATIONAL
F_REAL
F_RND
F_SGN
F_SHL
F_SHR
F_SIN
F_SQR
F_TAN
F_VAL
F_STR
F_HEX
F_OCT
F_MKN
F_BASES
F_B5                                                        ; The strings' (str.inc)
F_ASC = F_B5
F_CHR
F_LCASE
F_LEFT
F_LEN
F_LTRIM
F_MID
F_RIGHT
F_RTRIM
F_SPACE
F_STRING
F_UCASE
F_INSTR
F_LBOUND
F_UBOUND
F_FRE
F_B6                                                        ; Input and output's (io.inc)
F_CSRLIN = F_B6
F_EOF
F_FREEFILE
F_INKEY
F_LOC
F_LOF
F_POS
F_SEEK
F_INPUTS
F_ERL
F_ERR
F_ERRS
F_B7                                                        ; The system's (sys.inc)
F_COMMAND = F_B7
F_DATE
F_DIR
F_ENV
F_PEEK
F_POINT
F_SHELLS
F_STATUS
F_TIME
F_TIMER
F_ARG
F_BANK
F_END
F_SPECIAL = $F0                                             ; (fn_tab's: forms of their own, compiled by fn_special3)
F_SP_INSTR = F_SPECIAL
F_SP_INPUT
F_SP_LBOUND
F_SP_UBOUND
.endenum

.enum                                                       ; Statements
ST_B6                                                       ; Input and output's (io.inc)
ST_PRN = ST_B6                                              ; PRINT's value
ST_PRNL                                                     ; Its new line
ST_PRZONE                                                   ; , (the next zone)
ST_PRTAB                                                    ; TAB(n)
ST_PRSPC                                                    ; SPC(n)
ST_B7                                                       ; The system's (sys.inc)
ST_ENDN = ST_B7                                             ; END n
ST_END
.endenum

.code

; OP_FN's and OP_ST's dispatch: the routine numbered .A (its group's bank found) called
fn_call:
            ldx         #4
            cmp         #F_B5
            bcc         @have
            inx
            cmp         #F_B6
            bcc         @have
            inx
            cmp         #F_B7
            bcc         @have
            inx
@have:
            sec
            sbc         fn_first - 4,x
            asl         a
            pha
            lda         fn_disp_lo - 4,x
            sta         hyx2_to
            lda         fn_disp_hi - 4,x
            sta         hyx2_to + 1
            lda         hyx2_bank1 - 1,x
            sta         hyx2_n
            pla
            jmp         hyx2_farn
fn_first:   .byte       F_B4, F_B5, F_B6, F_B7
fn_disp_lo: .byte       <fn_disp4, <fn_disp5, <fn_disp6, <fn_disp7
fn_disp_hi: .byte       >fn_disp4, >fn_disp5, >fn_disp6, >fn_disp7

st_call:
            ldx         #6
            cmp         #ST_B7
            bcc         :+
            inx
:
            sec
            sbc         st_first - 6,x
            asl         a
            pha
            lda         st_disp_lo - 6,x
            sta         hyx2_to
            lda         st_disp_hi - 6,x
            sta         hyx2_to + 1
            lda         hyx2_bank1 - 1,x
            sta         hyx2_n
            pla
            jmp         hyx2_farn
st_first:   .byte       ST_B6, ST_B7
st_disp_lo: .byte       <st_disp6, <st_disp7
st_disp_hi: .byte       >st_disp6, >st_disp7
