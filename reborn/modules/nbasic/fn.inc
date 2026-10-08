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
ST_B4                                                       ; The numbers' (fns4.inc)
ST_RANDOMIZE = ST_B4                                        ; RANDOMIZE n (0: from the clock)
ST_BASE                                                     ; BASE b$
ST_DIGITS                                                   ; DIGITS n
ST_B5                                                       ; The heap's (heap.inc)
ST_DIM = ST_B5                                              ; DIM, REDIM: an array made
ST_ERASE                                                    ; ERASE: an array dropped
ST_MIDS                                                     ; MID$(v$, start, len) = new$
ST_B6                                                       ; Input and output's (io.inc)
ST_PRN = ST_B6                                              ; PRINT's value
ST_PRNL                                                     ; Its new line
ST_PRZONE                                                   ; , (the next zone)
ST_PRTAB                                                    ; TAB(n)
ST_PRSPC                                                    ; SPC(n)
ST_READ                                                     ; READ: DATA's next item pushed, a string
ST_RESTORE                                                  ; RESTORE: DATA's next item's place set
ST_INPUT                                                    ; INPUT's prompt, its line read
ST_INFLD                                                    ; Its next field pushed (a number or a string)
ST_INEND                                                    ; Its end: -1 pushed if it's to be typed again
ST_LGET                                                     ; LINE INPUT's line pushed, a string
ST_CLS                                                      ; CLS
ST_LOCATE                                                   ; LOCATE row, col, cursor (each -1: not given)
ST_COLOR                                                    ; COLOR fg, bg (each -1: not given)
ST_WIDTH                                                    ; WIDTH n
ST_BEEP                                                     ; BEEP
ST_SLEEP                                                    ; SLEEP: ticks (below 0: till a key)
ST_WRVAL                                                    ; WRITE's value
ST_WRCOM                                                    ; Its ,
ST_OPEN                                                     ; OPEN: path, number, mode (FM_*)
ST_CLOSE                                                    ; CLOSE #n
ST_CLOSEALL                                                 ; CLOSE
ST_FOUT                                                     ; PRINT #n's, WRITE #n's output to the file
ST_FEND                                                     ;   and back to the console
ST_FINPUT                                                   ; INPUT #n's, LINE INPUT #n's line read
ST_SEEK                                                     ; SEEK #n, pos
ST_GSET                                                     ; GET's and PUT's file and place
ST_GET                                                      ; GET's value read (the variable's on the stack)
ST_PUT                                                      ; PUT's value written
ST_KILL                                                     ; KILL path
ST_NAME                                                     ; NAME old AS new
ST_MKDIR                                                    ; MKDIR path
ST_RMDIR                                                    ; RMDIR path
ST_CHDIR                                                    ; CHDIR path
ST_FILES                                                    ; FILES [path]
ST_USTART                                                   ; PRINT USING's format
ST_UITEM                                                    ; Its next item, by its next field
ST_UEND                                                     ; Its end: the format's text to its next field
ST_B7                                                       ; The system's (sys.inc)
ST_ENDN = ST_B7                                             ; END n
ST_ERROR                                                    ; ERROR n
ST_SHELL                                                    ; SHELL line$ ("": rc alone)
ST_ENVIRON                                                  ; ENVIRON "name=value"
ST_POKE                                                     ; POKE addr, b
ST_BANK                                                     ; BANK n
ST_B8                                                       ; The Hydra's (machine.inc)
ST_SOUND = ST_B8                                            ; SOUND ch, note, patch, vol (each -1: not given)
ST_SOUNDS                                                   ; SOUND "text"
ST_PLAYCH                                                   ; PLAY's channel (-1: none)
ST_PLAY                                                     ; PLAY's line, or file
ST_SYS                                                      ; SYS addr or name, a, x, y
ST_RREGV                                                    ; RREG's register n, pushed
ST_SCREEN                                                   ; SCREEN n (gfx.inc)
ST_PSET                                                     ; PSET (x, y), c
ST_PRESET                                                   ; PRESET (x, y), c
ST_LINE                                                     ; LINE (x1, y1)-(x2, y2), c, kind (0 a line, 1 B, 2 BF)
ST_GMOVE                                                    ; The last point the point taken off
ST_GLX                                                      ; The last point's x pushed (STEP's)
ST_GLY                                                      ;   its y
ST_CIRCLE                                                   ; CIRCLE (x, y), r, c, fill
ST_GPRINT                                                   ; GPRINT (x, y), text$
ST_PALETTE                                                  ; PALETTE n, rgb
ST_PAINT                                                    ; PAINT (x, y), c, border
ST_DRAW                                                     ; DRAW s$
ST_SPRITE                                                   ; SPRITE n, x, y
ST_SPRITEI                                                  ; SPRITE n, x, y, image$
ST_SPRITEOFF                                                ; SPRITE n OFF
ST_WINDOW                                                   ; WINDOW (x1, y1)-(x2, y2), mode (0 none, 1, 2 SCREEN)
ST_VIEW                                                     ; VIEW (x1, y1)-(x2, y2), mode (0 none, 1, 2 SCREEN)
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
            ldx         #4
            cmp         #ST_B5
            bcc         @have
            inx
            cmp         #ST_B6
            bcc         @have
            inx
            cmp         #ST_B7
            bcc         @have
            inx
            cmp         #ST_B8
            bcc         @have
            inx
@have:
            sec
            sbc         st_first - 4,x
            asl         a
            pha
            lda         st_disp_lo - 4,x
            sta         hyx2_to
            lda         st_disp_hi - 4,x
            sta         hyx2_to + 1
            lda         hyx2_bank1 - 1,x
            sta         hyx2_n
            pla
            jmp         hyx2_farn
st_first:   .byte       ST_B4, ST_B5, ST_B6, ST_B7, ST_B8
st_disp_lo: .byte       <st_disp4, <st_disp5, <st_disp6, <st_disp7, <st_disp8
st_disp_hi: .byte       >st_disp4, >st_disp5, >st_disp6, >st_disp7, >st_disp8
