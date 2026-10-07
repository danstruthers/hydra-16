; ****************************************************************************
; token.inc - the keywords (bmacros.inc): the statements (each its routine, called RTS style), the words of
; statements and the operators, the functions (from SGN: UNFNC, by token), then EhyBASIC's short forms as aliases.
; Some code knows the order: FOR is $81 and DATA $83 (the FOR frames, the tokenizer), the operators + to OR come just
; before > = < (FRMEVL), and LEFT$, RIGHT$ and MID$ are the last functions (their string first: UNARY)

		init_token_tables

		keyword_rts "END", END
		keyword_rts "FOR", FOR
		keyword_rts "NEXT", NEXT
		keyword_rts "DATA", DATA
		keyword_rts "INPUT", INPUT_ST
		keyword_rts "DIM", DIM
		keyword_rts "READ", READ_
		keyword_rts "LET", LET
		keyword_rts "GOTO", GOTO, TOKEN_GOTO
		keyword_rts "RUN", RUN
		keyword_rts "IF", IF
		keyword_rts "RESTORE", RESTORE, TOKEN_RESTORE
		keyword_rts "GOSUB", GOSUB, TOKEN_GOSUB
		keyword_rts "RETURN", POP, TOKEN_RETURN
		keyword_rts "REM", REM, TOKEN_REM
		keyword_rts "STOP", STOP
		keyword_rts "ON", ON
		keyword_rts "WAIT", WAIT_
		keyword_rts "LOAD", LOAD
		keyword_rts "SAVE", SAVE
		keyword_rts "DEF", DEF
		keyword_rts "POKE", POKE
		keyword_rts "PRINT", PRINT_ST, TOKEN_PRINT
		keyword_rts "CONT", CONT
		keyword_rts "LIST", LIST
		keyword_rts "CLEAR", CLEAR, TOKEN_CLEAR
		keyword_rts "GET", GET_ST
		keyword_rts "NEW", NEW
		keyword_rts "BYE", BYE
		keyword_rts "OPEN", OPEN_ST
		keyword_rts "CLOSE", CLOSE_ST
		keyword_rts "SOUND", SOUND_ST
		keyword_rts "BEEP", BEEP_ST
		keyword_rts "SLEEP", SLEEP_ST
		keyword_rts "SYS", SYS_ST
		keyword_rts "RREG", RREG_ST
		keyword_rts "HIMEM", HIMEM_ST

		count_tokens
.assert TOKEN_GOTO = $88 && NUM_TOKENS < $40, error, "FOR $81, DATA $83: the statements' order"

		keyword	"TAB(", TOKEN_TAB
		keyword	"TO", TOKEN_TO
		keyword	"FN", TOKEN_FN
		keyword	"SPC(", TOKEN_SPC
		keyword	"THEN", TOKEN_THEN
		keyword	"NOT", TOKEN_NOT
		keyword	"STEP", TOKEN_STEP
		keyword	"+", TOKEN_PLUS
		keyword	"-", TOKEN_MINUS
		keyword	"*"
		keyword	"/"
		keyword	"^"
		keyword	"AND", TOKEN_AND
		keyword	"OR", TOKEN_OR
		keyword	">", TOKEN_GREATER
		keyword	"=", TOKEN_EQUAL
		keyword	"<"

.segment "VECTORS"
UNFNC:
		keyword_addr "SGN", SGN, TOKEN_SGN
		keyword_addr "INT", INT
		keyword_addr "ABS", ABS
		keyword_addr "USR", USR, TOKEN_USR
		keyword_addr "FRE", FRE
		keyword_addr "POS", POS
		keyword_addr "SQR", SQR
		keyword_addr "RND", RND
		keyword_addr "LOG", LOG
		keyword_addr "EXP", EXP
.segment "VECTORS"
UNFNC_COS:
		keyword_addr "COS", COS
.segment "VECTORS"
UNFNC_SIN:
		keyword_addr "SIN", SIN
.segment "VECTORS"
UNFNC_TAN:
		keyword_addr "TAN", TAN
.segment "VECTORS"
UNFNC_ATN:
		keyword_addr "ATN", ATN
		keyword_addr "PEEK", PEEK
		keyword_addr "LEN", LEN
		keyword_addr "STR$", STR, TOKEN_STR
		keyword_addr "VAL", VAL
		keyword_addr "ASC", ASC
		keyword_addr "CHR$", CHRSTR, TOKEN_CHR
		keyword_addr "EOF", EOF_FN
		keyword_addr "ENV$", ENV_FN
		keyword_addr "ARG$", ARG_FN
		keyword_addr "LEFT$", LEFTSTR, TOKEN_LEFTSTR
		keyword_addr "RIGHT$", RIGHTSTR, TOKEN_RIGHTSTR
		keyword_addr "MID$", MIDSTR, TOKEN_MIDSTR
		keyword	"GO", TOKEN_GO
.assert TOKEN_MIDSTR = TOKEN_LEFTSTR + 2 && TOKEN_GO = TOKEN_MIDSTR + 1, error, "LEFT$, RIGHT$, MID$: the last functions"

		init_aliases
		alias	"JSR", TOKEN_GOSUB
		alias	"RTN", TOKEN_RETURN
		alias	"RSTR", TOKEN_RESTORE
		alias	"CLR", TOKEN_CLEAR
		alias	"ST$", TOKEN_STR
		alias	"CH$", TOKEN_CHR
		alias	"LT$", TOKEN_LEFTSTR
		alias	"RT$", TOKEN_RIGHTSTR
		alias	"MD$", TOKEN_MIDSTR
		alias	"&", TOKEN_AND
		alias	"|", TOKEN_OR
		alias	"!", TOKEN_NOT
.assert TOKEN_N <= $100, error, "Too many keywords for a byte's tokens"
        .segment "KEYWORDS"
		.byte   0

        .segment "VECTORS"
MATHTBL:
        .byte   $79
        .word   FADDT-1
        .byte   $79
        .word   FSUBT-1
        .byte   $7B
        .word   FMULTT-1
        .byte   $7B
        .word   FDIVT-1
        .byte   $7F
        .word   FPWRT-1
        .byte   $50
        .word   TAND-1
        .byte   $46
        .word   OR-1
        .byte   $7D
        .word   NEGOP-1
        .byte   $5A
        .word   EQUOP-1
        .byte   $64
        .word   RELOPS-1
