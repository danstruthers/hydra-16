; ****************************************************************************
; lex.s - rc's tokens, from the input source (rc.s: src_getc).  A token is its type (tok: T_*), and for some its
; text (tok_text, tok_len: a word's, a $name's), its fds (a redirection's, a pipe's), and where it starts in the
; text being parsed (tok_start: for a function's body, a subshell's command).  tok_adj <> 0: no space before it (so
; a word's pieces join: $x.c is $x^.c).  A word's unquoted * ? [ are glob markers (G_*); tok_quoted <> 0: some of
; it was quoted; tok_kw: the keyword it is (T_IF ...: the parser takes it as one where a command starts), or 0.
; In rc's second bank, with parse.s (a character from the source, and errors, by FAR1).
;   Words: anything but white space and ; & | ^ $ ` ' { } ( ) < > = and a new line; '...' quotes ('' a quote in
;   it); # starts a comment where a token would; \ and a new line are a space.  $name: letters, digits, _ and *.

.include "rc.inc"

.zeropage

.bss
tok:        .res        1                                   ; The token (T_*)
tok_len:    .res        1                                   ; Its text's length
tok_fd:     .res        1                                   ; A redirection's fd, or a pipe's
tok_fd2:    .res        1                                   ; >[n=m]'s m
tok_rtype:  .res        1                                   ; A redirection's type (RD_*)
tok_quoted: .res        1
tok_adj:    .res        1                                   ; <> 0: no space before it
tok_start:  .res        2                                   ; Where it starts in the text
tok_kw:     .res        1                                   ; The keyword it is, or 0
lc:         .res        1                                   ; A character
lc_back:    .res        1                                   ; <> 0: lc_bc is back, to be read again ...
lc_bc:      .res        1
lc_bat:     .res        2                                   ;   and where it was
peeked:     .res        1                                   ; <> 0: the token is the next one, read ahead
spaced:     .res        1                                   ; A space since the last token
tok_text:   .res        TOK_MAX + 1

.segment "CODE2"

; Nothing read ahead (a new command, a new source)
lex_init:
            stz         lc_back
            stz         peeked
            lda         #1
            sta         spaced
            rts

; The next token, read ahead if it isn't already: tok ...  OUT: .A = tok
lex_peek:
            lda         peeked
            bne         :+
            jsr         lex
            lda         #1
            sta         peeked
:
            lda         tok
            rts

; The next token, taken.  OUT: .A = tok
lex_next:
            jsr         lex_peek
            stz         peeked
            rts

; The rest of the line skipped (an error's): to its new line
lex_flush:
            stz         peeked                              ; (The last token, read ahead or taken: a new line,
            lda         tok                                 ;   the end?  Then that's all)
            cmp         #T_NL
            beq         @done
            cmp         #T_EOF
            beq         @done
:
            jsr         getc
            bcs         @done
            cmp         #LF
            bne         :-
@done:
            rts

; Where the text is now (past what's been read): .A/.X
lex_pos:
            lda         peeked                              ; (A token read ahead: its start)
            bne         @tok
            lda         lc_back
            bne         @back
            lda         src_at
            ldx         src_at + 1
            clc
            adc         #1
            bcc         :+
            inx
:
            rts

@tok:
            lda         tok_start
            ldx         tok_start + 1
            rts

@back:
            lda         lc_bat
            ldx         lc_bat + 1
            rts

; A character (the one put back, if there is one).  OUT: C = 0, .A = it, src_at = where it is; or C = 1: the end
getc:
            lda         lc_back
            beq         :+
            stz         lc_back
            MOVR        src_at, lc_bat
            lda         lc_bc
            clc
            rts
:
            FAR1        src_getc
            rts

; .A put back (where it was: src_at)
ungetc:
            sta         lc_bc
            MOVR        lc_bat, src_at
            lda         #1
            sta         lc_back
            rts

; ****************************************************************************
; The next token
lex:
            stz         tok_len
            stz         tok_quoted
            stz         tok_kw
@space:
            jsr         getc
            bcc         :+
            lda         #T_EOF
            bra         @set
:
            cmp         #' '
            beq         @spaced
            cmp         #9
            beq         @spaced
            cmp         #'\'                                ; \ and a new line: a space
            bne         @hash
            jsr         getc
            bcs         @word1
            cmp         #LF
            beq         @spaced
            jsr         ungetc
            lda         #'\'
            bra         @word1

@spaced:
            lda         #1
            sta         spaced
            bra         @space

@hash:
            cmp         #'#'                                ; A comment: to the new line
            bne         @start
:
            jsr         getc
            bcs         @space
            cmp         #LF
            bne         :-
@start:
            pha                                             ; Where it starts, and whether it's spaced
            MOVR        tok_start, src_at
            lda         spaced
            eor         #1
            sta         tok_adj
            stz         spaced
            pla
            ldx         #NSINGLE - 1                        ; One character, a token alone?
:
            cmp         singles,X
            beq         @single
            dex
            bpl         :-
            cmp         #'&'
            beq         @amp
            cmp         #'|'
            beq         @pipe
            cmp         #'$'
            beq         @dollar
            cmp         #'<'
            beq         @in
            cmp         #'>'
            beq         @out
@word1:
            jmp         word

@single:
            lda         single_toks,X
@set:
            sta         tok
            rts

@amp:
            ldx         #T_ANDAND
            lda         #'&'
            jsr         twice
            bcs         @set1
            lda         #T_AMP
            bra         @set

@set1:
            txa
            bra         @set

@pipe:
            ldx         #T_OROR
            lda         #'|'
            jsr         twice
            bcs         @set1
            lda         #1
            sta         tok_fd
            jsr         fds                                 ; (|[n])
            lda         #T_PIPE
            bra         @set

@dollar:
            jsr         getc
            bcs         @name
            ldx         #T_COUNT
            cmp         #'#'
            beq         @named
            ldx         #T_FLAT
            cmp         #'"'
            beq         @named
            jsr         ungetc
            ldx         #T_DOLLAR
@named:
            stx         tok
            jmp         name

@name:
            lda         #T_DOLLAR
            sta         tok
            rts

@in:
            stz         tok_fd
            lda         #RD_IN
            bra         @redir

@out:
            lda         #1
            sta         tok_fd
            ldx         #RD_APPEND
            lda         #'>'
            jsr         twice
            txa
            bcs         @redir
            lda         #RD_OUT
@redir:
            sta         tok_rtype
            jsr         fds
            lda         #T_REDIR
            jmp         @set

; The next character .A?  Then it's taken (C = 1); else it's left (C = 0).  Keeps .X
twice:
            sta         lc
            phx
            jsr         getc
            plx
            bcs         @no
            cmp         lc
            beq         @yes
            jsr         ungetc
@no:
            clc
            rts

@yes:
            sec
            rts

; [n], [n=m] or [n=] after a redirection or a pipe, if there is one: tok_fd, and tok_fd2 (RD_DUP) or RD_CLOSE
fds:
            jsr         getc
            bcs         @done
            cmp         #'['
            beq         :+
            jmp         ungetc
:
            jsr         digits
            sta         tok_fd
            lda         lc
            cmp         #'='
            bne         @done
            jsr         getc                                ; [n=]: closed; [n=m]: m's
            bcs         @done
            cmp         #']'
            bne         :+
            lda         #RD_CLOSE
            sta         tok_rtype
            rts
:
            jsr         ungetc
            jsr         digits
            sta         tok_fd2
            lda         #RD_DUP
            sta         tok_rtype
@done:                                                      ; (Its ] read by digits: anything else there is lost)
            rts

; A number's digits, read: .A = it (the character after it: lc)
digits:
            stz         t0
@digit:
            jsr         getc
            bcs         @done
            sta         lc
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            pha
            lda         t0                                  ; t0 * 10 + it
            asl
            asl
            adc         t0
            asl
            sta         t0
            pla
            adc         t0
            sta         t0
            bra         @digit

@done:
            lda         t0
            rts

; A $name's name, into tok_text: letters, digits, _ and *
name:
            jsr         getc
            bcs         @done
            jsr         namechar
            bcs         @end
            ldx         tok_len
            sta         tok_text,X
            inc         tok_len
            bra         name

@end:
            jsr         ungetc
@done:
            ldx         tok_len
            stz         tok_text,X
            rts

; C = 0 if .A can be in a $name.  Keeps .A
namechar:
            cmp         #'*'
            beq         @yes
            cmp         #'_'
            beq         @yes
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @yes
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            bcc         @yes
            cmp         #'a'
            bcc         @no
            cmp         #'z' + 1
            bcc         @yes
@no:
            sec
            rts

@yes:
            clc
            rts

; A word: its characters (quoted ones too) till a special one or a space; its keyword, if it's one
word:
@char:
            cmp         #$27
            beq         @quote
            cmp         #'*'
            bne         :+
            lda         #G_STAR
            bra         @put
:
            cmp         #'?'
            bne         :+
            lda         #G_QUERY
            bra         @put
:
            cmp         #'['
            bne         @put
            lda         #G_BRACKET
@put:
            jsr         put
@next:
            jsr         getc
            bcs         @end
            jsr         special
            bcc         @char
            jsr         ungetc
@end:
            ldx         tok_len
            stz         tok_text,X
            lda         #T_WORD
            sta         tok
            lda         tok_quoted                          ; A keyword?
            bne         @done
            ldx         #0
@kw:
            ldy         #0
:
            lda         keywords,X
            cmp         tok_text,Y
            bne         @nokw
            inx
            iny
            cmp         #0
            bne         :-
            lda         keywords,X
            sta         tok_kw
@done:
            rts

@nokw:
            inx                                             ; To the next keyword
            lda         keywords - 1,X
            bne         @nokw
            inx                                             ; (Past its token)
            lda         keywords,X
            bne         @kw
            rts

@quote:                                                     ; '...': as it is, '' a quote
            lda         #1
            sta         tok_quoted
:
            jsr         getc
            bcs         @unended
            cmp         #$27
            beq         :+
            jsr         put
            bra         :-
:
            jsr         getc
            bcs         @end
            cmp         #$27
            bne         :+
            jsr         put                                 ; ('': a quote)
            bra         :--
:
            jsr         special
            bcs         :+
            jmp         @char
:
            jsr         ungetc
            jmp         @end

@unended:
            LDR         r0, s_quote
            FAR1        rc_error                            ; (It doesn't come back)

; A word's character: .A into tok_text (TOK_MAX at most: an error past them)
put:
            ldx         tok_len
            cpx         #TOK_MAX
            bcs         :+
            sta         tok_text,X
            inc         tok_len
            rts
:
            LDR         r0, s_long
            FAR1        rc_error                            ; (It doesn't come back)

; C = 1 if .A ends a word: a space, a new line, or a special character.  Keeps .A
special:
            ldx         #NSPECIAL - 1
:
            cmp         specials,X
            beq         @yes
            dex
            bpl         :-
            clc
            rts

@yes:
            sec
            rts

.segment "RODATA2"
singles:    .byte       LF, ';', '^', '{', '}', '(', ')', '=', '`'
NSINGLE     = * - singles
single_toks: .byte      T_NL, T_SEMI, T_CARET, T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN, T_EQ, T_BQ
specials:   .byte       ' ', 9, LF, ';', '&', '|', '^', '$', '`', '{', '}', '(', ')', '<', '>', '='
NSPECIAL    = * - specials
keywords:   .byte       "if", 0, T_IF, "not", 0, T_NOT, "for", 0, T_FOR, "in", 0, T_IN, "while", 0, T_WHILE
            .byte       "switch", 0, T_SWITCH, "case", 0, T_CASE, "fn", 0, T_FN, "~", 0, T_TWIDDLE
            .byte       "!", 0, T_BANG, 0

.rodata                                                     ; (The first bank's: rc_error reads them there)
s_quote:    .byte       "eof in quotes", 0
s_long:     .byte       "word too long", 0
