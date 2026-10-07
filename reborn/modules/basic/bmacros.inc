; ****************************************************************************
; bmacros.inc - the keyword and error tables' macros (included by basic.s).  The keywords: two tables laid down at
; once, the names (KEYWORDS: each with its last character's bit 7 set) and the statements' and functions' addresses
; (VECTORS), a token each from $80 in order (TOKEN_N counts them).  An alias (EhyBASIC's short forms) is a name at the
; end whose token the tokenizer turns into another's (ALIAS_MAP), so LIST shows the full name.  The errors: a table of
; their messages' addresses (ERRTAB), an error a byte offset in it, and the messages (ERROR).

; htasc - set the hi bit on the last byte of a string for termination
; (by Tom Greene)
.macro htasc str
	.repeat	.strlen(str)-1,I
		.byte	.strat(str,I)
	.endrep
	.byte	.strat(str,.strlen(str)-1) | $80
.endmacro

.macro init_token_tables
        .segment "VECTORS"
TOKEN_ADDRESS_TABLE:
        .segment "KEYWORDS"
TOKEN_NAME_TABLE:
TOKEN_N .set $80
.endmacro

; optionally define token symbol
; count up token number
.macro define_token token
		.ifnblank token
			token := TOKEN_N
		.endif
TOKEN_N .set TOKEN_N + 1
.endmacro

; lay down a keyword, optionally define a token symbol
.macro keyword key, token
		.segment "KEYWORDS"
		htasc	key
		define_token token
.endmacro

; lay down a keyword and an address (RTS style),
; optionally define a token symbol
.macro keyword_rts key, vec, token
        .segment "VECTORS"
		.word	vec-1
		keyword key, token
.endmacro

; lay down a keyword and an address,
; optionally define a token symbol
.macro keyword_addr key, vec, token
        .segment "VECTORS"
		.addr	vec
		keyword key, token
.endmacro

.macro count_tokens
		NUM_TOKENS := TOKEN_N - $80
.endmacro

; The aliases, after every keyword: each a name whose token becomes token (ALIAS_MAP, from FIRST_ALIAS)
.macro init_aliases
FIRST_ALIAS := TOKEN_N
        .segment "VECTORS"
ALIAS_MAP:
.endmacro

.macro alias key, token
        .segment "VECTORS"
		.byte	token
		keyword key
.endmacro

.macro init_error_table
        .segment "ERRTAB"
ERROR_ADDRS:
.endmacro

.macro define_error error, msg
        .segment "ERRTAB"
		error := <(*-ERROR_ADDRS)
		.addr	.ident(.concat("EM_", .string(error)))
        .segment "ERROR"
.ident(.concat("EM_", .string(error))):
		htasc msg
.endmacro
