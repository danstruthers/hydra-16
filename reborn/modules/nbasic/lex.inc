; ****************************************************************************
; lex.inc - the words of a line (the second bank): lex_next reads the next token from lbuf at lp.  A token is the
; line's end (a ' or REM too), a number (its bytes in the stored format, read by the numbers library in the program's
; base: a word of letters, digits, . and _ that starts with a digit or a point, as a program's text has it; &H, &O, &B),
; a string ("..."), a name (letters, digits, _ and ., a letter first, in capitals; its suffix, $ % & ! #, apart), a
; keyword (a name the table has: its number) or an operator (a character, or TO_LE, TO_GE, TO_NE).

NAME_MAX        = 40            ; A name's characters at most
TNUM_MAX        = 260           ; A number's bytes from a line at most (255 characters in base 80: some 210)
LBUF_SIZE       = 512           ; A line (and the lines a _ goes on with) at most

TK_EOL          = 0             ; The tokens (tk)
TK_NUM          = 1             ; tnum (tnl bytes); tisint <> 0: it's an integer of 32 bits, tint
TK_STR          = 2             ; tsp (in lbuf), tsl bytes
TK_NAME         = 3             ; tname (tnl characters), tsuf
TK_KW           = 4             ; tkv: its number (K_*)
TK_OP           = 5             ; tkv: the character, or TO_LE, TO_GE, TO_NE
TO_LE           = $80
TO_GE           = $81
TO_NE           = $82


.bss
lbuf:       .res        LBUF_SIZE                           ; The line (zero-terminated)
tk:         .res        1                                   ; The token
tkv:        .res        1
tname:      .res        NAME_MAX + 2                        ; A name's characters (and, for a keyword, a $)
tnl:        .res        2                                   ; Its length (a number's: its bytes')
tsuf:       .res        1                                   ; A name's suffix (0: none)
tsl:        .res        2                                   ; A string's length
tnum:       .res        TNUM_MAX                            ; A number's bytes
tisint:     .res        1
tint:       .res        4
lx_save:    .res        2                                   ; (lex_mark's)
lx_save2:   .res        2
cbase:      .res        BASE_ROOM                           ; The base the program's numbers are read in ("d")
hexword:    .res        4                                   ; (&H's base: "x", "o", "b")
lx_wlen:    .res        1                                   ; (A number's word: its characters)

.enum                                                       ; The keywords (kw_tab)
K_NONE
K_ABS
K_ABSOLUTE
K_AND
K_APPEND
K_ARG
K_AS
K_ASC
K_ATN
K_BANK
K_BASE
K_BASES
K_BEEP
K_BINARY
K_BIT
K_BYE
K_CALL
K_CASE
K_CDBL
K_CHDIR
K_CHR
K_CINT
K_CIRCLE
K_CLEAR
K_CLNG
K_CLOSE
K_CLS
K_COLOR
K_COMMAND
K_COMPLEX
K_CONST
K_CONT
K_COS
K_CSNG
K_CSRLIN
K_CVN
K_DATA
K_DATE
K_DECLARE
K_DEFDBL
K_DEFINT
K_DEFLNG
K_DEFSNG
K_DEFSTR
K_DELETE
K_DENOMINATOR
K_DIGITS
K_DIM
K_DIR
K_DO
K_DOUBLE
K_DRAW
K_EDIT
K_ELSE
K_ELSEIF
K_END
K_ENV
K_ENVIRON
K_ENVIRONS
K_EOF
K_EQV
K_ERASE
K_ERL
K_ERR
K_ERRS
K_ERROR
K_EXIT
K_EXP
K_FIB
K_FILES
K_FIX
K_FIXED
K_FOR
K_FRE
K_FREEFILE
K_FUNCTION
K_GCD
K_GET
K_GOSUB
K_GOTO
K_GPRINT
K_HEX
K_IF
K_IMAG
K_IMP
K_INCLUDE
K_INKEY
K_INPUT
K_INPUTS
K_INSTR
K_INT
K_INTEGER
K_IS
K_KILL
K_LBOUND
K_LCASE
K_LEFT
K_LEN
K_LET
K_LINE
K_LIST
K_LOAD
K_LOC
K_LOCATE
K_LOF
K_LOG
K_LONG
K_LOOP
K_LTRIM
K_MID
K_MKDIR
K_MKN
K_MOD
K_NAME
K_NEW
K_NEXT
K_NOT
K_NUMBER
K_NUMERATOR
K_OCT
K_OFF
K_ON
K_OPEN
K_OPTION
K_OR
K_OUTPUT
K_PAINT
K_PALETTE
K_PEEK
K_PI
K_PLAY
K_POINT
K_POKE
K_POS
K_PRESERVE
K_PRESET
K_PRINT
K_PSET
K_PUT
K_RANDOMIZE
K_RATIONAL
K_READ
K_REAL
K_REDIM
K_REM
K_RESTORE
K_RESUME
K_RETURN
K_RIGHT
K_RMDIR
K_RND
K_RREG
K_RTRIM
K_RUN
K_SAVE
K_SCREEN
K_SEEK
K_SELECT
K_SGN
K_SHARED
K_SHELL
K_SHELLS
K_SHL
K_SHR
K_SIN
K_SINGLE
K_SLEEP
K_SOUND
K_SPACE
K_SPC
K_SPRITE
K_SQR
K_STATIC
K_STATUS
K_STEP
K_STOP
K_STR
K_STRING
K_STRINGS
K_SUB
K_SWAP
K_SYS
K_SYSTEM
K_TAB
K_TAN
K_THEN
K_TIME
K_TIMER
K_TO
K_TYPE
K_UBOUND
K_UCASE
K_UNTIL
K_USING
K_VAL
K_VIEW
K_WEND
K_WHILE
K_WIDTH
K_WINDOW
K_WRITE
K_XOR
K_COUNT
.endenum

.segment "CODE2"

; The next token (tk and the rest), lp on past it
lex_next:
            ldy         #0
@space:
            lda         (lp),y
            cmp         #' '
            beq         @skip
            cmp         #9
            bne         @start
@skip:
            iny
            bra         @space
@start:
            jsr         lp_plus_y
            MOVW        tstart, lp
            lda         (lp)
            bne         :+
            stz         tk                                  ; (TK_EOL)
            rts
:
            cmp         #$27                                ; (' : the rest a comment)
            bne         :+
            stz         tk
            rts
:
            cmp         #'"'
            bne         :+
            jmp         lex_string
:
            cmp         #'.'
            bne         :+
            ldy         #1
            lda         (lp),y
            jsr         is_digit
            bcs         @op
            jmp         lex_number
:
            jsr         is_digit
            bcs         :+
            jmp         lex_number
:
            jsr         is_letter
            bcs         :+
            jmp         lex_name
:
            cmp         #'&'
            bne         @op
            ldy         #1
            lda         (lp),y
            and         #$DF
            cmp         #'H'
            beq         :+
            cmp         #'O'
            beq         :+
            cmp         #'B'
            bne         @op
:
            jmp         lex_based
@op:
            lda         (lp)                                ; An operator
            cmp         #'?'                                ; (? is PRINT)
            bne         :+
            jsr         lp_inc
            lda         #K_PRINT
            sta         tkv
            lda         #TK_KW
            sta         tk
            rts
:
            sta         tkv
            jsr         lp_inc
            lda         #TK_OP
            sta         tk
            lda         tkv
            cmp         #'<'
            bne         @gt
            lda         (lp)
            cmp         #'='
            bne         :+
            lda         #TO_LE
            bra         @two
:
            cmp         #'>'
            bne         @done
            lda         #TO_NE
            bra         @two
@gt:
            cmp         #'>'
            bne         @done
            lda         (lp)
            cmp         #'='
            bne         @done
            lda         #TO_GE
@two:
            sta         tkv
            jsr         lp_inc
@done:
            rts

; lp += .Y
lp_plus_y:
            tya
            clc
            adc         lp
            sta         lp
            bcc         :+
            inc         lp + 1
:
            rts

lp_inc:
            inc         lp
            bne         :+
            inc         lp + 1
:
            rts

; C = 0 if .A is a digit, a letter (is_letter), a name's character (is_namec: a letter, digit, _ or .).  Keep .A
is_digit:
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcs         @no
            clc
            rts
@no:
            sec
            rts
is_letter:
            pha
            and         #$DF
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            bcs         @no
            pla
            clc
            rts
@no:
            pla
            sec
            rts
is_namec:
            jsr         is_letter
            bcc         :+
            jsr         is_digit
            bcc         :+
            cmp         #'_'
            beq         @yes
            cmp         #'.'
            beq         @yes
            sec
:
            rts
@yes:
            clc
            rts

; A string: "...", to its " or the line's end
lex_string:
            jsr         lp_inc
            MOVW        tsp, lp
            ldy         #0
@l:
            lda         (lp),y
            beq         @end
            cmp         #'"'
            beq         @end
            iny
            bra         @l
@end:
            sty         tsl
            stz         tsl + 1
            jsr         lp_plus_y
            lda         (lp)
            beq         :+
            jsr         lp_inc                              ; (Past its ")
:
            lda         #TK_STR
            sta         tk
            rts

; A name (or a keyword): its characters in capitals, then its suffix
lex_name:
            ldy         #0
@l:
            lda         (lp),y
            jsr         is_namec
            bcs         @end
            cpy         #NAME_MAX
            bcc         :+
            lda         #ER_TOOLONG
            ldx         #0
            jmp         c_error
:
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            sta         tname,y
            iny
            bra         @l
@end:
            sty         tnl
            stz         tnl + 1
            stz         tsuf
            cmp         #'$'
            beq         @suf
            cmp         #'%'
            beq         @suf
            cmp         #'&'
            beq         @suf
            cmp         #'!'
            beq         @suf
            cmp         #'#'
            bne         @kw
            iny                                             ; (# then a name's character or a digit: not a suffix,
            lda         (lp),y                              ;   but # and a based number or a file's: x #1 ...)
            dey
            jsr         is_namec
            bcc         @kw
            lda         #'#'
@suf:
            sta         tsuf
            iny
@kw:
            jsr         lp_plus_y
            lda         tsuf                                ; A keyword?  (A $ is part of one's name: LEFT$)
            cmp         #'$'
            beq         :+
            cmp         #0
            bne         @name                               ; (x% ...: a name)
:
            jsr         kw_find
            bcs         @name
            sta         tkv
            cmp         #K_REM                              ; (REM: the rest a comment)
            bne         :+
            stz         tk
            rts
:
            lda         #TK_KW
            sta         tk
            rts
@name:
            lda         #TK_NAME
            sta         tk
            rts

; The keyword tname is (with tsuf's $ if it has one).  OUT: C = 0, .A its number; or C = 1 none
kw_find:
            lda         tname
            sec
            sbc         #'A'
            asl         a
            tax
            lda         kw_index,x
            sta         t0
            lda         kw_index + 1,x
            sta         t0 + 1
            ldx         tnl                                 ; (The name's length, with its $)
            lda         tsuf
            beq         :+
            lda         #'$'
            sta         tname,x
            inx
:
            stx         t4
@entry:
            lda         (t0)                                ; (An entry: its length, its characters, its number)
            beq         @none
            cmp         t4
            bne         @next
            tay
@cmp:
            lda         (t0),y
            cmp         tname - 1,y
            bne         @next
            dey
            bne         @cmp
            ldy         t4
            iny
            lda         (t0),y
            clc
            rts
@next:
            lda         (t0)
            clc
            adc         #2
            adc         t0
            sta         t0
            bcc         @entry
            inc         t0 + 1
            bra         @entry
@none:
            sec
            rts

; A number: a word (letters, digits, . and _) read whole in the program's base, as a program's text (the numbers
; library's PARSE); a QuickBASIC suffix after it (# ! % &) dropped
lex_number:
            ldy         #0
@l:
            lda         (lp),y
            jsr         is_namec
            bcs         @end
            iny
            bne         @l
@end:
            sty         tnl
            sty         lx_wlen
            LDW         t0, cbase                           ; (In the program's base)
            jsr         num_word
            ldy         lx_wlen
            lda         (lp),y
            cmp         #'#'
            beq         @suf
            cmp         #'!'
            beq         @suf
            cmp         #'%'
            beq         @suf
            cmp         #'&'
            bne         @done
@suf:
            iny
            lda         (lp),y
            dey
            jsr         is_namec                            ; (Not a suffix if a name or a number goes on)
            bcc         @done
            iny
@done:
            jmp         lp_plus_y

; &H, &O, &B and their digits: a number in base x, o or b
lex_based:
            ldy         #1
            lda         (lp),y
            and         #$DF
            ldx         #'x'
            cmp         #'H'
            beq         :+
            ldx         #'o'
            cmp         #'O'
            beq         :+
            ldx         #'b'
:
            stx         hexword
            stz         hexword + 1
            lda         lp                                  ; (Past &H)
            clc
            adc         #2
            sta         lp
            bcc         :+
            inc         lp + 1
:
            ldy         #0
@l:
            lda         (lp),y
            jsr         is_namec
            bcs         @end
            iny
            bne         @l
@end:
            sty         tnl
            sty         lx_wlen
            LDW         t0, hexword
            jsr         num_word
            ldy         lx_wlen
            jmp         lp_plus_y

; The word of tnl characters at lp, a number in the base at t0 (a base string): tnum, tnl its bytes (tisint, tint);
; TK_NUM.  Not one: Syntax error
num_word:
            MOVW        r0, lp
            lda         tnl
            sta         r1
            stz         r1 + 1
            MOVW        r4, t0
            LDW         r2, tnum
            LDW         r3, TNUM_MAX
            ldy         #NPARSE_PROGRAM | NPARSE_WHOLE
            FARN        4, num_parse_ram
            bcc         :+
            lda         #ER_SYNTAX
            ldx         #0
            jmp         c_error
:
            sta         tnl
            stx         tnl + 1
            FARN        4, num_isint32                      ; (tisint, tint)
            lda         #TK_NUM
            sta         tk
            rts

; lex_mark: the place in the line kept; lex_back: back to it (the token read again by lex_next)
lex_mark:
            MOVW        lx_save, tstart
            rts
lex_back:
            MOVW        lp, lx_save
            jmp         lex_next

; ---- The keywords: by their first letter, each its length, its characters, its number; a 0 after each letter's
.macro KW name, num
            .byte       .strlen(name), name, num
.endmacro

kw_index:
            .word       kw_a, kw_b, kw_c, kw_d, kw_e, kw_f, kw_g, kw_h, kw_i, kw_j, kw_k, kw_l, kw_m, kw_n, kw_o, kw_p
            .word       kw_q, kw_r, kw_s, kw_t, kw_u, kw_v, kw_w, kw_x, kw_y, kw_z
kw_a:
            KW          "ABS", K_ABS
            KW          "ABSOLUTE", K_ABSOLUTE
            KW          "AND", K_AND
            KW          "APPEND", K_APPEND
            KW          "ARG$", K_ARG
            KW          "AS", K_AS
            KW          "ASC", K_ASC
            KW          "ATN", K_ATN
            .byte       0
kw_b:
            KW          "BANK", K_BANK
            KW          "BASE", K_BASE
            KW          "BASE$", K_BASES
            KW          "BEEP", K_BEEP
            KW          "BINARY", K_BINARY
            KW          "BIT", K_BIT
            KW          "BYE", K_BYE
            .byte       0
kw_c:
            KW          "CALL", K_CALL
            KW          "CASE", K_CASE
            KW          "CDBL", K_CDBL
            KW          "CHDIR", K_CHDIR
            KW          "CHR$", K_CHR
            KW          "CINT", K_CINT
            KW          "CIRCLE", K_CIRCLE
            KW          "CLEAR", K_CLEAR
            KW          "CLNG", K_CLNG
            KW          "CLOSE", K_CLOSE
            KW          "CLS", K_CLS
            KW          "COLOR", K_COLOR
            KW          "COMMAND$", K_COMMAND
            KW          "COMPLEX", K_COMPLEX
            KW          "CONST", K_CONST
            KW          "CONT", K_CONT
            KW          "COS", K_COS
            KW          "CSNG", K_CSNG
            KW          "CSRLIN", K_CSRLIN
            KW          "CVN", K_CVN
            .byte       0
kw_d:
            KW          "DATA", K_DATA
            KW          "DATE$", K_DATE
            KW          "DECLARE", K_DECLARE
            KW          "DEFDBL", K_DEFDBL
            KW          "DEFINT", K_DEFINT
            KW          "DEFLNG", K_DEFLNG
            KW          "DEFSNG", K_DEFSNG
            KW          "DEFSTR", K_DEFSTR
            KW          "DELETE", K_DELETE
            KW          "DENOMINATOR", K_DENOMINATOR
            KW          "DIGITS", K_DIGITS
            KW          "DIM", K_DIM
            KW          "DIR$", K_DIR
            KW          "DO", K_DO
            KW          "DOUBLE", K_DOUBLE
            KW          "DRAW", K_DRAW
            .byte       0
kw_e:
            KW          "EDIT", K_EDIT
            KW          "ELSE", K_ELSE
            KW          "ELSEIF", K_ELSEIF
            KW          "END", K_END
            KW          "ENV$", K_ENV
            KW          "ENVIRON", K_ENVIRON
            KW          "ENVIRON$", K_ENVIRONS
            KW          "EOF", K_EOF
            KW          "EQV", K_EQV
            KW          "ERASE", K_ERASE
            KW          "ERL", K_ERL
            KW          "ERR", K_ERR
            KW          "ERR$", K_ERRS
            KW          "ERROR", K_ERROR
            KW          "EXIT", K_EXIT
            KW          "EXP", K_EXP
            .byte       0
kw_f:
            KW          "FIB", K_FIB
            KW          "FILES", K_FILES
            KW          "FIX", K_FIX
            KW          "FIXED", K_FIXED
            KW          "FOR", K_FOR
            KW          "FRE", K_FRE
            KW          "FREEFILE", K_FREEFILE
            KW          "FUNCTION", K_FUNCTION
            .byte       0
kw_g:
            KW          "GCD", K_GCD
            KW          "GET", K_GET
            KW          "GOSUB", K_GOSUB
            KW          "GOTO", K_GOTO
            KW          "GPRINT", K_GPRINT
            .byte       0
kw_h:
            KW          "HEX$", K_HEX
            .byte       0
kw_i:
            KW          "IF", K_IF
            KW          "IMAG", K_IMAG
            KW          "IMP", K_IMP
            KW          "INCLUDE", K_INCLUDE
            KW          "INKEY$", K_INKEY
            KW          "INPUT", K_INPUT
            KW          "INPUT$", K_INPUTS
            KW          "INSTR", K_INSTR
            KW          "INT", K_INT
            KW          "INTEGER", K_INTEGER
            KW          "IS", K_IS
            .byte       0
kw_j:
            .byte       0
kw_k:
            KW          "KILL", K_KILL
            .byte       0
kw_l:
            KW          "LBOUND", K_LBOUND
            KW          "LCASE$", K_LCASE
            KW          "LEFT$", K_LEFT
            KW          "LEN", K_LEN
            KW          "LET", K_LET
            KW          "LINE", K_LINE
            KW          "LIST", K_LIST
            KW          "LOAD", K_LOAD
            KW          "LOC", K_LOC
            KW          "LOCATE", K_LOCATE
            KW          "LOF", K_LOF
            KW          "LOG", K_LOG
            KW          "LONG", K_LONG
            KW          "LOOP", K_LOOP
            KW          "LTRIM$", K_LTRIM
            .byte       0
kw_m:
            KW          "MID$", K_MID
            KW          "MKDIR", K_MKDIR
            KW          "MKN$", K_MKN
            KW          "MOD", K_MOD
            .byte       0
kw_n:
            KW          "NAME", K_NAME
            KW          "NEW", K_NEW
            KW          "NEXT", K_NEXT
            KW          "NOT", K_NOT
            KW          "NUMBER", K_NUMBER
            KW          "NUMERATOR", K_NUMERATOR
            .byte       0
kw_o:
            KW          "OCT$", K_OCT
            KW          "OFF", K_OFF
            KW          "ON", K_ON
            KW          "OPEN", K_OPEN
            KW          "OPTION", K_OPTION
            KW          "OR", K_OR
            KW          "OUTPUT", K_OUTPUT
            .byte       0
kw_p:
            KW          "PAINT", K_PAINT
            KW          "PALETTE", K_PALETTE
            KW          "PEEK", K_PEEK
            KW          "PI", K_PI
            KW          "PLAY", K_PLAY
            KW          "POINT", K_POINT
            KW          "POKE", K_POKE
            KW          "POS", K_POS
            KW          "PRESERVE", K_PRESERVE
            KW          "PRESET", K_PRESET
            KW          "PRINT", K_PRINT
            KW          "PSET", K_PSET
            KW          "PUT", K_PUT
            .byte       0
kw_q:
            .byte       0
kw_r:
            KW          "RANDOMIZE", K_RANDOMIZE
            KW          "RATIONAL", K_RATIONAL
            KW          "READ", K_READ
            KW          "REAL", K_REAL
            KW          "REDIM", K_REDIM
            KW          "REM", K_REM
            KW          "RESTORE", K_RESTORE
            KW          "RESUME", K_RESUME
            KW          "RETURN", K_RETURN
            KW          "RIGHT$", K_RIGHT
            KW          "RMDIR", K_RMDIR
            KW          "RND", K_RND
            KW          "RREG", K_RREG
            KW          "RTRIM$", K_RTRIM
            KW          "RUN", K_RUN
            .byte       0
kw_s:
            KW          "SAVE", K_SAVE
            KW          "SCREEN", K_SCREEN
            KW          "SEEK", K_SEEK
            KW          "SELECT", K_SELECT
            KW          "SGN", K_SGN
            KW          "SHARED", K_SHARED
            KW          "SHELL", K_SHELL
            KW          "SHELL$", K_SHELLS
            KW          "SHL", K_SHL
            KW          "SHR", K_SHR
            KW          "SIN", K_SIN
            KW          "SINGLE", K_SINGLE
            KW          "SLEEP", K_SLEEP
            KW          "SOUND", K_SOUND
            KW          "SPACE$", K_SPACE
            KW          "SPC", K_SPC
            KW          "SPRITE", K_SPRITE
            KW          "SQR", K_SQR
            KW          "STATIC", K_STATIC
            KW          "STATUS", K_STATUS
            KW          "STEP", K_STEP
            KW          "STOP", K_STOP
            KW          "STR$", K_STR
            KW          "STRING", K_STRING
            KW          "STRING$", K_STRINGS
            KW          "SUB", K_SUB
            KW          "SWAP", K_SWAP
            KW          "SYS", K_SYS
            KW          "SYSTEM", K_SYSTEM
            .byte       0
kw_t:
            KW          "TAB", K_TAB
            KW          "TAN", K_TAN
            KW          "THEN", K_THEN
            KW          "TIME$", K_TIME
            KW          "TIMER", K_TIMER
            KW          "TO", K_TO
            KW          "TYPE", K_TYPE
            .byte       0
kw_u:
            KW          "UBOUND", K_UBOUND
            KW          "UCASE$", K_UCASE
            KW          "UNTIL", K_UNTIL
            KW          "USING", K_USING
            .byte       0
kw_v:
            KW          "VAL", K_VAL
            KW          "VIEW", K_VIEW
            .byte       0
kw_w:
            KW          "WEND", K_WEND
            KW          "WHILE", K_WHILE
            KW          "WIDTH", K_WIDTH
            KW          "WINDOW", K_WINDOW
            KW          "WRITE", K_WRITE
            .byte       0
kw_x:
            KW          "XOR", K_XOR
            .byte       0
kw_y:
            .byte       0
kw_z:
            .byte       0

; A keyword's name, for LIST and errors: kw_name .A = its number -> t0 its characters, .Y its length (C = 1: none)
kw_name:
            sta         t4 + 1
            LDW         t0, kw_a
@entry:
            lda         (t0)
            bne         :+
            inc         t0                                  ; (A letter's end: the next's)
            bne         :+
            inc         t0 + 1
:
            lda         t0 + 1                              ; (Past the table: none)
            cmp         #>kw_end
            bcc         :+
            lda         t0
            cmp         #<kw_end
            bcs         @none
:
            lda         (t0)
            beq         @entry
            tay
            iny
            lda         (t0),y
            cmp         t4 + 1
            beq         @found
            tya
            sec
            adc         t0
            sta         t0
            bcc         @entry
            inc         t0 + 1
            bra         @entry
@found:
            lda         (t0)
            tay
            inc         t0
            bne         :+
            inc         t0 + 1
:
            clc
            rts
@none:
            sec
            rts
kw_end:
