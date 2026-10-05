; ****************************************************************************
; read.inc - hylang's reader: danlang's syntax (its C# reader's: Parser.cs, and LVal.Sym's for a symbol's token),
; from text in the task's RAM (read_start).  ( ) an S-expression, { } a Q-expression, ; a comment to the line's end;
; strings with escapes (\n \t \r \0 \a \b \f \v \e \\ \" \xHH), or here strings (an odd number of quotes, their text
; as it is, CR LF read as LF); \name a character (by name or as itself), :name an atom, T, NIL and exit; a prefix
; before ( or { a shorthand (' list, ^ head, $ tail and so on: rd_prefix).  A number is a decimal integer for now
; (step 7.3 brings the rest); a token that isn't one is a symbol, in lower case.
;   The lists being read are on the reader's stack, each open list's head a root (root_push), so a collection may come
; as a list is made.

; (Its long branches: longbranch's jeq and the rest, which hylang.s loads)
RD_DEPTH        = 32            ; Lists open at once, at most
TOK_SIZE        = 255           ; A token's text, at most (a number's: math.pi's is 151)
CNAME_SIZE      = 20            ; A character's name, at most (the longest is 16)

RD_END          = 1             ; read_expr's ends: no more text ...
RD_MORE         = 2             ;   an expression not finished: more text wanted ...
RD_ERR          = 3             ;   or the text's error (h_v)

.zeropage
rd_p:       .res        2                                   ; The text's next byte ...
rd_end:     .res        2                                   ;   and its end
rd_d:       .res        1                                   ; The lists open
rd_q:       .res        1                                   ; A string's quotes
rd_x:       .res        1                                   ; Scratch

.bss
tok:        .res        TOK_SIZE                            ; A token's text ...
tok_n:      .res        1                                   ;   and its length
rd_cname:   .res        CNAME_SIZE                          ; A character's name, as the table has it
rd_root:    .res        1                                   ; The first open list's root
rd_kind:    .res        RD_DEPTH                            ; Each open list's kind (OT_SEXPR, OT_QEXPR) ...
rd_close:   .res        RD_DEPTH                            ;   the character that closes it ...
rd_tail_lo: .res        RD_DEPTH                            ;   and its last pair (its first: its root)
rd_tail_hi: .res        RD_DEPTH
.segment "CODE2"

; The reader's text: .A/.X bytes at r0
read_start:
            clc
            adc         r0
            sta         rd_end
            txa
            adc         r0 + 1
            sta         rd_end + 1
            MOVR        rd_p, r0
            rts

; ---- The text, a byte at a time

; .A = the next byte, not taken.  OUT: C = 1, none left
rd_peek:
            lda         rd_p
            cmp         rd_end
            lda         rd_p + 1
            sbc         rd_end + 1
            bcs         :+
            lda         (rd_p)
            clc
:
            rts

; The byte after the next: as rd_peek
rd_peek2:
            clc
            lda         rd_p
            adc         #1
            tax
            lda         rd_p + 1
            adc         #0
            cpx         rd_end
            sbc         rd_end + 1
            bcs         :+
            ldy         #1
            lda         (rd_p),y
            clc
:
            rts

; The next byte taken
rd_skip1:
            inc         rd_p
            bne         :+
            inc         rd_p + 1
:
            rts

; .A = the next byte, taken.  OUT: C = 1, none left
rd_get:
            jsr         rd_peek
            bcs         :+
            pha
            jsr         rd_skip1
            pla
            clc
:
            rts

; Spaces and comments passed.  OUT: C = 1, the text's end
rd_space:
            jsr         rd_peek
            bcs         @done
            cmp         #';'
            beq         @comment
            jsr         is_space
            bcc         @done
            jsr         rd_skip1
            bra         rd_space
@comment:
            jsr         rd_get
            bcs         @done
            cmp         #LF
            beq         rd_space
            cmp         #CR
            beq         rd_space
            bra         @comment
@done:
            rts

; C = 1 if .A is a space (a space, a tab, CR, LF, FF or VT).  Keeps .A
is_space:
            cmp         #' '
            beq         @yes
            cmp         #TAB
            bcc         @no
            cmp         #CR + 1
            bcs         @no
@yes:
            sec
            rts
@no:
            clc
            rts

; C = 1 if .A ends a symbol's or a number's token (a space, ) or }).  Keeps .A
is_end:
            cmp         #')'
            beq         @yes
            cmp         #'}'
            beq         @yes
            jmp         is_space
@yes:
            sec
            rts

; ---- Expressions

; The next expression.  OUT: C = 0, h_v = it; or C = 1 and .A = RD_END (no more), RD_MORE (one begun, not finished:
; more text wanted), RD_ERR (h_v an error: what's wrong with the text)
read_expr:
            lda         root_n
            sta         rd_root
            stz         rd_d
@token:
            jsr         rd_space
            bcs         @eof
            cmp         #'('
            beq         @sexpr
            cmp         #'{'
            beq         @qexpr
            cmp         #')'
            beq         @close
            cmp         #'}'
            beq         @close
            cmp         #'"'
            bne         :+
            jmp         @string
:
            cmp         #128
            bcs         @nonascii
            cmp         #' '
            bcc         @control
            cmp         #127
            beq         @control
            jsr         rd_prefix                           ; (A shorthand: its symbol, the list opened)
            bcc         @token
            cpx         #RD_ERR
            jeq         @fail
            jmp         @atom
@sexpr:
            jsr         rd_skip1
            ldx         #OT_SEXPR
            lda         #')'
            jsr         rd_open
            bcs         @fail
            bra         @token
@qexpr:
            jsr         rd_skip1
            ldx         #OT_QEXPR
            lda         #'}'
            jsr         rd_open
            bcs         @fail
            bra         @token
@close:
            jsr         rd_skip1
            ldx         rd_d
            beq         @unopened
            cmp         rd_close - 1,x
            bne         @unopened
            dec         rd_d                                ; ---- A list closed: its value
            ldx         rd_d
            lda         rd_kind,x
            pha
            jsr         root_get
            jsr         root_drop
            pla
            jsr         make_list
            bcs         @room
            bra         @value
@eof:
            lda         rd_d
            beq         @end
            lda         rd_root                             ; (Lists still open: more text wanted)
            sta         root_n
            lda         #RD_MORE
            sec
            rts
@end:
            lda         #RD_END
            sec
            rts
@nonascii:
            LDR         r0, s_nonascii
            bra         @error
@control:
            LDR         r0, s_control
            bra         @error
@unopened:
            cmp         #')'
            beq         :+
            LDR         r0, s_unopened_q
            bra         @error
:
            LDR         r0, s_unopened_s
            bra         @error
@room:
            LDR         r0, s_room
@error:
            jsr         error_text
@fail:
            lda         rd_root                             ; (The lists open let go)
            sta         root_n
            lda         #RD_ERR
            sec
            rts
@string:
            jsr         rd_string
            bcc         @value
            cpx         #RD_MORE
            bne         @fail
            lda         rd_root
            sta         root_n
            lda         #RD_MORE
            sec
            rts
@atom:
            jsr         rd_atom                             ; (A number or a symbol's token)
            bcs         @fail
@value:
            lda         rd_d                                ; ---- A value: the expression, or the next in its list
            bne         @add
            clc
            rts
@add:
            jsr         rd_add
            bcs         @room
            jmp         @token

; A list opened: .X its kind, .A the character that closes it.  OUT: C = 1, h_v an error (too deep, or no room)
rd_open:
            ldy         rd_d
            cpy         #RD_DEPTH
            bcs         @deep
            sta         rd_close,y
            txa
            sta         rd_kind,y
            lda         #0
            sta         rd_tail_lo,y
            sta         rd_tail_hi,y
            LDR         h_v, NIL
            jsr         root_push
            bcs         @deep
            inc         rd_d
            clc
            rts
@deep:
            LDR         r0, s_deep
            jsr         error_text
            sec
            rts

; h_v added to the end of the innermost open list.  OUT: C = 1, no room
rd_add:
            MOVR        h_a, h_v                            ; (A pair of it and NIL)
            LDR         h_d, NIL
            jsr         cons
            bcs         @no
            ldx         rd_d
            lda         rd_tail_hi - 1,x
            bne         @after
            jsr         root_set                            ; (The list's first: its root)
            bra         @tail
@after:
            MOVR        h_a, h_v                            ; (After its last)
            lda         rd_tail_lo - 1,x
            sta         h_v
            lda         rd_tail_hi - 1,x
            sta         h_v + 1
            jsr         set_cdr
            MOVR        h_v, h_a
@tail:
            ldx         rd_d
            lda         h_v
            sta         rd_tail_lo - 1,x
            lda         h_v + 1
            sta         rd_tail_hi - 1,x
            clc
@no:
            rts

; A prefix's shorthand: if the next byte is one (rd_prefixes) and a ( or { follows it, the list opened with the
; prefix's symbol in it.  OUT: C = 0, it was; C = 1, it wasn't (.X = 0), or an error (.X = RD_ERR, h_v it)
rd_prefix:
            ldx         #0
@find:
            lda         rd_prefixes,x
            beq         @not
            jsr         rd_peek
            cmp         rd_prefixes,x
            beq         @found
            inx
            bra         @find
@found:
            stx         rd_x
            jsr         rd_peek2
            bcs         @not
            ldx         #OT_SEXPR
            ldy         #')'
            cmp         #'('
            beq         @open
            ldx         #OT_QEXPR
            ldy         #'}'
            cmp         #'{'
            bne         @not
@open:
            jsr         rd_skip1                            ; (The prefix, and the paren)
            jsr         rd_skip1
            tya
            jsr         rd_open
            bcs         @err
            ldx         rd_x                                ; (Its symbol, the list's first)
            lda         rd_prefix_lo,x
            sta         r0
            lda         rd_prefix_hi,x
            sta         r0 + 1
            ldy         #0
:
            lda         (r0),y
            beq         :+
            iny
            bra         :-
:
            lda         #OT_SYMBOL
            jsr         intern
            bcs         @room
            jsr         rd_add
            bcs         @room
            clc
            rts
@not:
            ldx         #0
            sec
            rts
@room:
            LDR         r0, s_room
            jsr         error_text
@err:
            ldx         #RD_ERR
            sec
            rts

; ---- Tokens

; A number's or a symbol's token (to a space, ) or }), its value: a decimal integer, a symbol, an atom, a
; character, T, NIL or exit.  OUT: C = 0, h_v = it; or C = 1, h_v an error
rd_atom:
            stz         tok_n
@byte:
            jsr         rd_peek
            bcs         @got
            jsr         is_end
            bcs         @got
            jsr         rd_skip1
            ldx         tok_n
            cpx         #TOK_SIZE
            bcs         @long
            sta         tok,x
            inc         tok_n
            bra         @byte
@got:
            jsr         rd_number
            bcc         @done
            cpx         #RD_ERR
            beq         @fail
            lda         tok                                 ; ---- Not a number
            cmp         #':'
            beq         @atomic
            cmp         #'\'
            beq         @char
            jsr         tok_lower
            jsr         rd_keyword
            bcc         @done
            LDR         r0, tok
            ldy         tok_n
            lda         #OT_SYMBOL
            jsr         intern
            bcs         @room
@done:
            clc
            rts
@atomic:
            jsr         tok_lower                           ; (An atom: its name, after the :)
            LDR         r0, tok + 1
            ldy         tok_n
            dey
            beq         @empty
            lda         #OT_ATOM
            jsr         intern
            bcs         @room
            clc
            rts
@empty:
            LDR         r0, s_empty_atom
            bra         @error
@char:
            jsr         rd_char
            bcs         @fail
            clc
            rts
@long:
            LDR         r0, s_long
            bra         @error
@room:
            LDR         r0, s_room
@error:
            jsr         error_text
@fail:
            sec
            rts

; The token in lower case
tok_lower:
            ldx         #0
@byte:
            cpx         tok_n
            beq         @done
            lda         tok,x
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            bcs         :+
            ora         #$20
            sta         tok,x
:
            inx
            bra         @byte
@done:
            rts

; The token (in lower case) T, NIL or exit: h_v its value.  OUT: C = 1, it's none of them
rd_keyword:
            ldx         tok_n
            cpx         #1
            bne         @nil
            lda         tok
            cmp         #'t'
            bne         @no
            LDR         h_v, T_VAL
            clc
            rts
@nil:
            cpx         #3
            bne         @exit
            lda         tok
            cmp         #'n'
            bne         @no
            lda         tok + 1
            cmp         #'i'
            bne         @no
            lda         tok + 2
            cmp         #'l'
            bne         @no
            LDR         h_v, NIL
            clc
            rts
@exit:
            cpx         #4
            bne         @no
            lda         tok
            cmp         #'e'
            bne         @no
            lda         tok + 1
            cmp         #'x'
            bne         @no
            lda         tok + 2
            cmp         #'i'
            bne         @no
            lda         tok + 3
            cmp         #'t'
            bne         @no
            LDR         h_v, EXIT_VAL
            clc
            rts
@no:
            sec
            rts

; The token as a number, if it starts as one does (a digit, +, - or #): num_parse (numbers.inc, the third bank)
; reads it, in the case it's written in.  OUT: C = 0, h_v = it; or C = 1: .X = 0, it isn't a number; .X = RD_ERR,
; an error (h_v: too big, division by zero)
rd_number:
            lda         tok
            cmp         #'+'
            beq         @parse
            cmp         #'-'
            beq         @parse
            cmp         #'#'
            beq         @parse
            sec
            sbc         #'0'
            cmp         #10
            bcc         @parse
            ldx         #0
            sec
            rts
@parse:
            LDR         r0, tok
            ldy         tok_n
            FARX        lib_num, num_parse
            rts

; The token \name: a character, by its name (rd_charnames: in either case, a - in it skipped), or as itself (its
; first byte after the \).  OUT: C = 0, h_v = it; or C = 1, h_v an error (\ alone)
rd_char:
            ldx         tok_n
            cpx         #2
            bcs         :+
            LDR         r0, s_no_char
            jsr         error_text
            sec
            rts
:
            ldy         #0                                  ; (The name in lower case, its -s out, in rd_cname)
            ldx         #1
@squeeze:
            cpx         tok_n
            beq         @squeezed
            lda         tok,x
            inx
            cmp         #'-'
            beq         @squeeze
            cpy         #CNAME_SIZE                         ; (Longer than any name: as itself)
            bcs         @itself
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            bcs         :+
            ora         #$20
:
            sta         rd_cname,y
            iny
            bra         @squeeze
@squeezed:
            sty         rd_x                                ; (Its length)
            LDR         ob_p, rd_charnames
@name:
            lda         (ob_p)                              ; (Each name: its length, it, its character)
            beq         @itself
            cmp         rd_x
            bne         @next
            ldy         #1
@cmp:
            lda         (ob_p),y                            ; (The entry's byte y, against the name's y - 1)
            cmp         rd_cname - 1,y
            bne         @next
            cpy         rd_x
            beq         @match
            iny
            bra         @cmp
@match:
            iny                                             ; (Its character)
            lda         (ob_p),y
            bra         @char
@next:
            lda         (ob_p)
            clc
            adc         #2
            adc         ob_p
            sta         ob_p
            bcc         @name
            inc         ob_p + 1
            bra         @name
@itself:
            lda         tok + 1
@char:
            asl                                             ; (CHARS + 2c)
            sta         h_v
            lda         #>CHARS
            adc         #0
            sta         h_v + 1
            clc
            rts

; A string: "..." with escapes, or a here string (an odd number of quotes, the same number closing it, its text as it
; is).  OUT: C = 0, h_v = it; or C = 1: .X = RD_MORE (a here string not closed: more text wanted), or RD_ERR (h_v an
; error)
rd_string:
            stz         rd_q                                ; (The quotes that open it)
@open:
            jsr         rd_peek
            bcs         @opened
            cmp         #'"'
            bne         @opened
            jsr         rd_skip1
            inc         rd_q
            bra         @open
@opened:
            stz         tok_n
            lda         rd_q
            lsr
            bcs         @text
            jmp         @made                               ; (An even number: "" and the like, empty)
@text:
            jsr         rd_get
            jcs         @short
            cmp         #'"'
            beq         @quote
            cmp         #'\'
            beq         @escape
            jsr         is_space
            bcc         @notspace
            cmp         #LF
            beq         @newline
            cmp         #CR
            jne         @byte
            ldx         rd_q                                ; (CR: in a plain string, an error; in a here
            cpx         #1                                  ; string, CR LF is LF)
            jeq         @nl_err
            jsr         rd_peek
            cmp         #LF
            beq         @text
            lda         #CR
            jmp         @byte
@newline:
            ldx         rd_q
            cpx         #1
            jeq         @nl_err
            bra         @byte
@notspace:
            cmp         #' '
            jcc         @ctl
            cmp         #127
            jeq         @ctl
            bra         @byte
@quote:
            ldx         #1                                  ; (Quotes: as many as opened it close it)
@quotes:
            cpx         rd_q
            beq         @made
            jsr         rd_peek
            bcs         @fewer
            cmp         #'"'
            bne         @fewer
            jsr         rd_skip1
            inx
            bra         @quotes
@fewer:
            lda         #'"'                                ; (Fewer: they're its text)
:
            jsr         @put
            jcs         @long
            dex
            bne         :-
            bra         @text
@escape:
            lda         rd_q
            cmp         #1
            beq         :+
            lda         #'\'                                ; (A here string's: as it is)
            bra         @byte
:
            jsr         rd_get
            bcs         @short
            ldx         #0
@esc:
            ldy         rd_escapes,x
            beq         @hex
            cmp         rd_escapes,x
            beq         @escaped
            inx
            inx
            bra         @esc
@escaped:
            lda         rd_escapes + 1,x
            bra         @byte
@hex:
            cmp         #'x'
            bne         @bad_esc
            jsr         rd_hexdigit
            bcs         @no_hex
            sta         rd_x
            jsr         rd_hexdigit
            bcs         :+
            pha
            lda         rd_x
            asl
            asl
            asl
            asl
            sta         rd_x
            pla
            ora         rd_x
            bra         @byte
:
            lda         rd_x
@byte:
            jsr         @put
            bcs         @long
            jmp         @text
@made:
            LDR         r0, tok
            lda         tok_n
            jsr         make_string
            bcs         @room
            clc
            rts
@short:
            lda         rd_q                                ; (The text's end: a here string's more lines,
            cmp         #1                                  ; or an error)
            beq         @incomplete
            ldx         #RD_MORE
            sec
            rts
@put:
            ldy         tok_n                               ; (A byte of the string: C = 1, too many)
            cpy         #126
            bcs         :+
            sta         tok,y
            inc         tok_n
            clc
:
            rts
@incomplete:
            LDR         r0, s_incomplete
            bra         @error
@nl_err:
            LDR         r0, s_newline
            bra         @error
@ctl:
            LDR         r0, s_control
            bra         @error
@bad_esc:
            LDR         r0, s_escape
            bra         @error
@no_hex:
            LDR         r0, s_hex
            bra         @error
@long:
            LDR         r0, s_strlong
            bra         @error
@room:
            LDR         r0, s_room
@error:
            jsr         error_text
            ldx         #RD_ERR
            sec
            rts

; A hex digit, if the next byte is one: taken.  OUT: C = 0, .A = its value; C = 1, it isn't
rd_hexdigit:
            jsr         rd_peek
            bcs         @no
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @digit
            ora         #$20
            cmp         #'a'
            bcc         @no
            cmp         #'f' + 1
            bcs         @no
            sbc         #'a' - 10 - 1                       ; (C = 0: - 1 more)
            pha
            jsr         rd_skip1
            pla
            clc
            rts
@digit:
            sbc         #'0' - 1                            ; (C = 0)
            pha
            jsr         rd_skip1
            pla
            clc
            rts
@no:
            sec
            rts

.segment "RODATA2"
; The prefixes' shorthands: the prefix, and its symbol's name
rd_prefixes:    .byte   "'^$.|~=:@!?#<>*", 0
rd_prefix_lo:   .lobytes s_list, s_head, s_tail, s_unpack, s_join, s_format, s_set, s_def, s_fn, s_eval, s_if
                .lobytes s_hash_create, s_hash_get, s_hash_put, s_hash_call
rd_prefix_hi:   .hibytes s_list, s_head, s_tail, s_unpack, s_join, s_format, s_set, s_def, s_fn, s_eval, s_if
                .hibytes s_hash_create, s_hash_get, s_hash_put, s_hash_call
s_list:         .byte   "list", 0
s_head:         .byte   "head", 0
s_tail:         .byte   "tail", 0
s_unpack:       .byte   "unpack", 0
s_join:         .byte   "join", 0
s_format:       .byte   "format", 0
s_set:          .byte   "set", 0
s_def:          .byte   "def", 0
s_fn:           .byte   "fn", 0
s_eval:         .byte   "eval", 0
s_if:           .byte   "if", 0
s_hash_create:  .byte   "hash-create", 0
s_hash_get:     .byte   "hash-get", 0
s_hash_put:     .byte   "hash-put", 0
s_hash_call:    .byte   "hash-call", 0

; A plain string's escapes: the letter, and the byte
rd_escapes:     .byte   'n', LF, 't', TAB, 'r', CR, '0', 0, 'a', 7, 'b', 8, 'f', 12, 'v', 11, 'e', 27
                .byte   '\', '\', '"', '"', 0

; The characters' names (danlang's: LVal.Character): each its length, the name (lower case, no -), the character
.macro CHARNAME name, code
                .byte   .strlen(name), name, code
.endmacro
rd_charnames:
                CHARNAME "backslash", '\'
                CHARNAME "bslash", '\'
                CHARNAME "bs", '\'
                CHARNAME "backtick", '`'
                CHARNAME "btick", '`'
                CHARNAME "bt", '`'
                CHARNAME "bell", 8
                CHARNAME "linefeed", LF
                CHARNAME "newline", LF
                CHARNAME "lf", LF
                CHARNAME "nl", LF
                CHARNAME "formfeed", 12
                CHARNAME "ff", 12
                CHARNAME "null", 0
                CHARNAME "quote", '"'
                CHARNAME "doublequote", '"'
                CHARNAME "return", CR
                CHARNAME "carriagereturn", CR
                CHARNAME "cr", CR
                CHARNAME "rparen", ')'
                CHARNAME "rightparen", ')'
                CHARNAME "rp", ')'
                CHARNAME "lparen", '('
                CHARNAME "leftparen", '('
                CHARNAME "lp", '('
                CHARNAME "rbrace", '}'
                CHARNAME "rightcurly", '}'
                CHARNAME "rcurly", '}'
                CHARNAME "rc", '}'
                CHARNAME "lbrace", '{'
                CHARNAME "leftcurly", '{'
                CHARNAME "lcurly", '{'
                CHARNAME "lc", '{'
                CHARNAME "rbracket", ']'
                CHARNAME "rightbracket", ']'
                CHARNAME "rb", ']'
                CHARNAME "lbracket", '['
                CHARNAME "leftbracket", '['
                CHARNAME "lb", '['
                CHARNAME "slash", '/'
                CHARNAME "sl", '/'
                CHARNAME "space", ' '
                CHARNAME "sp", ' '
                CHARNAME "tab", TAB
                CHARNAME "tick", $27
                CHARNAME "singlequote", $27
                CHARNAME "tilde", '~'
                CHARNAME "verticalspace", 7
                CHARNAME "vspace", 7
                CHARNAME "vs", 7
                CHARNAME "verticaltab", 11
                CHARNAME "vtab", 11
                CHARNAME "vt", 11
                CHARNAME "bang", '!'
                CHARNAME "warning", '!'
                CHARNAME "exclaim", '!'
                CHARNAME "exclamation", '!'
                CHARNAME "exclamationpoint", '!'
                CHARNAME "at", '@'
                CHARNAME "poundsign", '#'
                CHARNAME "pound", '#'
                CHARNAME "hash", '#'
                CHARNAME "dollarsign", '$'
                CHARNAME "dollar", '$'
                CHARNAME "dollars", '$'
                CHARNAME "ds", '$'
                CHARNAME "percentsign", '%'
                CHARNAME "percent", '%'
                CHARNAME "mod", '%'
                CHARNAME "modulo", '%'
                CHARNAME "modulus", '%'
                CHARNAME "pc", '%'
                CHARNAME "caret", '^'
                CHARNAME "uparrow", '^'
                CHARNAME "amp", '&'
                CHARNAME "and", '&'
                CHARNAME "ampersand", '&'
                CHARNAME "pipe", '|'
                CHARNAME "or", '|'
                CHARNAME "vbar", '|'
                CHARNAME "verticalbar", '|'
                CHARNAME "vb", '|'
                CHARNAME "star", '*'
                CHARNAME "splat", '*'
                CHARNAME "mult", '*'
                CHARNAME "st", '*'
                CHARNAME "gt", '>'
                CHARNAME "greater", '>'
                CHARNAME "greaterthan", '>'
                CHARNAME "lt", '<'
                CHARNAME "less", '<'
                CHARNAME "lessthan", '<'
                CHARNAME "comma", ','
                CHARNAME "colon", ':'
                CHARNAME "semicolon", ';'
                CHARNAME "semi", ';'
                CHARNAME "sc", ';'
                CHARNAME "dot", '.'
                CHARNAME "peroid", '.'
                CHARNAME "point", '.'
                CHARNAME "qmark", '?'
                CHARNAME "question", '?'
                CHARNAME "questionmark", '?'
                CHARNAME "qm", '?'
                CHARNAME "underbar", '_'
                CHARNAME "ub", '_'
                CHARNAME "underscore", '_'
                CHARNAME "minus", '-'
                CHARNAME "hyphen", '-'
                CHARNAME "dash", '-'
                CHARNAME "sub", '-'
                CHARNAME "subtract", '-'
                CHARNAME "plus", '+'
                CHARNAME "add", '+'
                .byte   0

s_nonascii:     .byte   "Non-ASCII character outside of string literal", 0
s_control:      .byte   "Control sequences not allowed", 0
s_unopened_s:   .byte   "Closed a SExpr without opening", 0
s_unopened_q:   .byte   "Closed a QExpr without opening", 0
s_deep:         .byte   "Lists nested too deeply", 0
s_room:         .byte   "No room", 0
s_long:         .byte   "A name too long", 0
s_empty_atom:   .byte   "An atom with no name", 0
s_no_char:      .byte   "A character with no name", 0
s_incomplete:   .byte   "Incomplete string literal", 0
s_newline:      .byte   "Newlines are not allowed in regular strings", 0
s_escape:       .byte   "Unknown escape sequence", 0
s_hex:          .byte   "\x needs a hex digit", 0
s_strlong:      .byte   "A string too long (as yet)", 0
.code
