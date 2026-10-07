; ****************************************************************************
; parse.s - rc's grammar (Plan 9's, as rc.inc's nodes have it), a command at a time, into a tree in the arena.
;   line:     cmds, to a new line (or the end)
;   body:     cmds, new lines between them too, to } or )
;   cmds:     andor, each ended by ; or & (a background one) or a new line
;   andor:    not { (&& | ||) not }                 (new lines may follow && and ||)
;   not:      ! not | pipeline
;   pipeline: unit { |[n] unit }                    (a new line may follow |)
;   unit:     { body } redirections | if(body) andor | if not andor | for(word [in words]) andor |
;             while(body) andor | switch word { body: case words; ... } | ~ word words | fn words [{ body }] |
;             case words (in a switch's body) | simple
;   simple:   name=word ... then words and redirections, in any order
;   word:     piece { ^ piece }  (pieces next to each other join: a ^ between them)
;   piece:    text | $name | $name(words) | $#name | $"name | `{ body } | ( words )
; Errors: rc_error ("syntax error"), which doesn't return.  In rc's second bank, with lex.s (the arena, errors: FAR1;
; rc.s calls parse_line and lex_init by FAR2).

.include "rc.inc"

.zeropage
pl:         .res        2                                   ; A node's parts, as it's made
pr:         .res        2
pq:         .res        2
pn:         .res        2                                   ; A new node

.segment "CODE2"

; ****************************************************************************
; A command from the input: what to run, to the end of a line.  OUT: C = 0, .A/.X = its tree (0: an empty line);
; or C = 1: the input's end (nothing in it)
parse_line:
            jsr         lex_peek
            cmp         #T_EOF
            bne         :+
            sec
            rts
:
            stz         mode
            jsr         seq
            clc
            rts

; ****************************************************************************
; cmds: a sequence (C_SEQ nodes, each +1 a command, +3 the next), in mode (0: to a new line, taken; 1: to } or ),
; not taken).  OUT: .A/.X = its first (0: none)
seq:
            PUSHW       pl                                  ; (pl: the first; pr: the last)
            PUSHW       pr
            stz         pl
            stz         pl + 1
            stz         pr
            stz         pr + 1
@cmd:
            lda         mode
            beq         :+
            jsr         skipnl
:
            jsr         lex_peek
            cmp         #T_EOF
            beq         @to_done
            cmp         #T_NL
            bne         :+
            jsr         lex_next                            ; (mode 0: a new line ends it)
@to_done:
            jmp         @done
:
            cmp         #T_RBRACE
            beq         @to_end
            cmp         #T_RPAREN
            bne         @go
@to_end:
            jmp         @end

@go:
            PUSHW       pl                                  ; (The sequence's first and last: a command uses them)
            PUSHW       pr
            jsr         lex_pos                             ; Its text's start, for a background one
            pha
            phx
            jsr         andor
            sta         pn
            stx         pn + 1
            jsr         lex_peek
            cmp         #T_AMP
            bne         @plain
            jsr         lex_pos                             ; A background one: its text, to the &
            sta         pq
            stx         pq + 1
            jsr         lex_next
            lda         #7
            jsr         node
            lda         #C_BG
            sta         (p0)
            ldy         #1
            lda         pn
            sta         (p0),Y
            iny
            lda         pn + 1
            sta         (p0),Y
            iny
            plx
            pla
            sta         (p0),Y
            iny
            txa
            sta         (p0),Y
            iny
            lda         pq
            sta         (p0),Y
            iny
            lda         pq + 1
            sta         (p0),Y
            MOVR        pn, p0
            bra         @add

@plain:
            pla                                             ; (Its start: not needed)
            pla
            jsr         lex_peek
            cmp         #T_SEMI
            bne         :+
            jsr         lex_next
            bra         @add
:
            cmp         #T_NL
            beq         @add                                ; (Taken at @cmd: mode 0 ends there)
            cmp         #T_EOF
            beq         @add
            cmp         #T_RBRACE
            beq         @add
            cmp         #T_RPAREN
            beq         @add
            jmp         syntax

@add:                                                       ; pn, a new C_SEQ node at the end
            PULLW       pr
            PULLW       pl
            lda         #5
            jsr         node
            lda         #C_SEQ
            sta         (p0)
            ldy         #1
            lda         pn
            sta         (p0),Y
            iny
            lda         pn + 1
            sta         (p0),Y
            iny
            lda         #0
            sta         (p0),Y
            iny
            sta         (p0),Y
            lda         pr                                  ; After the last, or the first
            ora         pr + 1
            bne         :+
            MOVR        pl, p0
            bra         :++
:
            ldy         #3
            lda         p0
            sta         (pr),Y
            iny
            lda         p0 + 1
            sta         (pr),Y
:
            MOVR        pr, p0
            jmp         @cmd

@end:                                                       ; (At } or ): mode 0 has none to end)
            lda         mode
            bne         @done
            jmp         syntax

@done:
            lda         pl
            ldx         pl + 1
            sta         pn
            stx         pn + 1
            PULLW       pr
            PULLW       pl
            lda         pn
            ldx         pn + 1
            rts

; New lines skipped
skipnl:
            jsr         lex_peek
            cmp         #T_NL
            bne         :+
            jsr         lex_next
            bra         skipnl
:
            rts

; andor: not { (&& | ||) not }.  OUT: .A/.X
andor:
            jsr         notcmd
@more:
            sta         pn
            stx         pn + 1
            jsr         lex_peek
            ldy         #C_AND
            cmp         #T_ANDAND
            beq         @op
            ldy         #C_OR
            cmp         #T_OROR
            beq         @op
            lda         pn
            ldx         pn + 1
            rts

@op:
            phy
            PUSHW       pn
            jsr         lex_next
            jsr         skipnl
            jsr         notcmd
            sta         pr
            stx         pr + 1
            PULLW       pl
            pla
            jsr         node2
            bra         @more

; not: ! not | pipeline.  OUT: .A/.X
notcmd:
            jsr         lex_peek
            cmp         #T_WORD
            bne         pipeline
            lda         tok_kw
            cmp         #T_BANG
            bne         pipeline
            jsr         lex_next
            jsr         notcmd
            sta         pl
            stx         pl + 1
            lda         #C_NOT
            jmp         node1

; pipeline: unit { |[n] unit }: a C_PIPE node for each |, with each side's text.  OUT: .A/.X
pipeline:
            jsr         lex_pos                             ; The left's text: from here ...
            pha
            phx
            jsr         unit
@more:
            sta         pn
            stx         pn + 1
            jsr         lex_peek
            cmp         #T_PIPE
            beq         @pipe
            pla
            pla
            lda         pn
            ldx         pn + 1
            rts

@pipe:
            jsr         lex_pos                             ;   ... to here
            sta         pq
            stx         pq + 1
            lda         tok_fd
            pha
            jsr         lex_next
            jsr         skipnl
            PUSHW       pn                                  ; (The left)
            PUSHW       pq                                  ; (Its text's end)
            jsr         lex_pos                             ; The right's text: from here ...
            pha
            phx
            jsr         unit
            sta         pr
            stx         pr + 1
            lda         #14
            jsr         node
            lda         #C_PIPE
            sta         (p0)
            ldy         #12                                 ; Its text's end: here
            jsr         lex_pos
            sta         pq
            stx         pq + 1
            ldy         #14 - 1
            lda         pq + 1
            sta         (p0),Y
            dey
            lda         pq
            sta         (p0),Y
            dey                                             ; +10: its start
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            dey                                             ; +8: the left's end
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            dey                                             ; +5 the fd, +6 the left's start: under the left
            dey
            dey
            pla                                             ; (The left: +1)
            sta         pl + 1
            pla
            sta         pl
            pla                                             ; The fd
            sta         (p0),Y                              ; (+5)
            iny                                             ; +6, +7: the left's start (the stack's next)
            pla
            sta         pq + 1
            pla
            sta         (p0),Y
            iny
            lda         pq + 1
            sta         (p0),Y
            ldy         #1
            lda         pl
            sta         (p0),Y
            iny
            lda         pl + 1
            sta         (p0),Y
            iny
            lda         pr
            sta         (p0),Y
            iny
            lda         pr + 1
            sta         (p0),Y
            ldy         #6                                  ; The next one's left: this, from the same start
            lda         (p0),Y
            pha
            iny
            lda         (p0),Y
            pha
            lda         p0
            ldx         p0 + 1
            jmp         @more

; unit: a command a pipe can join.  OUT: .A/.X
unit:
            jsr         lex_peek
            cmp         #T_LBRACE
            beq         @brace
            cmp         #T_WORD
            bne         @simple
            ldx         #NKEYS - 1                          ; A keyword where a command starts?
            lda         tok_kw
            beq         @simple
:
            cmp         key_toks,X
            beq         @key
            dex
            bpl         :-
@simple:
            jmp         simple

@key:
            phx
            jsr         lex_next
            pla
            asl
            tax
            jmp         (key_vec,X)

@brace:                                                     ; { body } and its redirections
            jsr         lex_next
            jsr         body
            sta         pl
            stx         pl + 1
            PUSHW       pl
            jsr         epilog
            sta         pr
            stx         pr + 1
            PULLW       pl
            lda         #C_BRACE
            jmp         node2

; body: { cmds } after the {, with its }.  OUT: .A/.X
body:
            lda         mode
            pha
            lda         #1
            sta         mode
            jsr         seq
            sta         pn
            stx         pn + 1
            pla
            sta         mode
            lda         #T_RBRACE
            jsr         expect
            lda         pn
            ldx         pn + 1
            rts

; (body): its condition, after the keyword.  OUT: .A/.X
paren:
            lda         #T_LPAREN
            jsr         expect
            lda         mode
            pha
            lda         #1
            sta         mode
            jsr         seq
            sta         pn
            stx         pn + 1
            pla
            sta         mode
            lda         #T_RPAREN
            jsr         expect
            jsr         skipnl
            lda         pn
            ldx         pn + 1
            rts

; if(body) andor, or if not andor
k_if:
            jsr         lex_peek
            cmp         #T_WORD
            bne         :+
            lda         tok_kw
            cmp         #T_NOT
            bne         :+
            jsr         lex_next
            jsr         andor
            sta         pl
            stx         pl + 1
            lda         #C_IFNOT
            jmp         node1
:
            jsr         paren
            PUSHW       pn
            jsr         andor
            sta         pr
            stx         pr + 1
            PULLW       pl
            lda         #C_IF
            jmp         node2

; while(body) andor
k_while:
            jsr         paren
            PUSHW       pn
            jsr         andor
            sta         pr
            stx         pr + 1
            PULLW       pl
            lda         #C_WHILE
            jmp         node2

; for(word [in words]) andor
k_for:
            lda         #T_LPAREN
            jsr         expect
            jsr         word
            PUSHW       pn                                  ; (The name)
            stz         pq                                  ; (No list)
            stz         pq + 1
            lda         #0
            pha
            jsr         lex_peek
            cmp         #T_WORD
            bne         @close
            lda         tok_kw
            cmp         #T_IN
            bne         @close
            jsr         lex_next
            pla
            lda         #1
            pha
            jsr         words
            sta         pq
            stx         pq + 1
@close:
            PUSHW       pq
            lda         #T_RPAREN
            jsr         expect
            jsr         skipnl
            jsr         andor                               ; The body
            sta         pr
            stx         pr + 1
            lda         #8
            jsr         node
            lda         #C_FOR
            sta         (p0)
            ldy         #6
            lda         pr + 1
            sta         (p0),Y
            dey
            lda         pr
            sta         (p0),Y
            dey
            pla                                             ; (The list)
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            ldy         #7
            pla                                             ; (A list given)
            sta         (p0),Y
            ldy         #2
            pla                                             ; (The name)
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            lda         p0
            ldx         p0 + 1
            rts

; switch word { body }
k_switch:
            jsr         word
            PUSHW       pn
            jsr         skipnl
            lda         #T_LBRACE
            jsr         expect
            jsr         body
            sta         pr
            stx         pr + 1
            PULLW       pl
            lda         #C_SWITCH
            jmp         node2

; case words (in a switch's body)
k_case:
            jsr         words
            sta         pl
            stx         pl + 1
            lda         #C_CASE
            jmp         node1

; ~ word words
k_twiddle:
            jsr         word
            PUSHW       pn
            jsr         words
            sta         pr
            stx         pr + 1
            PULLW       pl
            lda         #C_MATCH
            jmp         node2

; fn words { body } (its text kept), or fn words (they're removed)
k_fn:
            jsr         words
            sta         pl
            stx         pl + 1
            jsr         lex_peek
            cmp         #T_LBRACE
            beq         :+
            lda         #C_FNDEL
            jmp         node1
:
            PUSHW       pl
            jsr         lex_next
            jsr         lex_pos                             ; Its body's text: from after the { ...
            pha
            phx
            lda         mode
            pha
            lda         #1
            sta         mode
            jsr         seq                                 ; (Its tree: only to find its end)
            pla
            sta         mode
            jsr         lex_pos                             ;   ... to the }
            sta         pq
            stx         pq + 1
            lda         #T_RBRACE
            jsr         expect
            lda         #7
            jsr         node
            lda         #C_FN
            sta         (p0)
            ldy         #6
            lda         pq + 1
            sta         (p0),Y
            dey
            lda         pq
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            lda         p0
            ldx         p0 + 1
            rts

; ****************************************************************************
; simple: assignments, then words and redirections.  OUT: .A/.X (a C_SIMPLE node)
simple:
            PUSHW       pl                                  ; (Words: pl first, pr last; redirections: pq;
            PUSHW       pr                                  ;   assignments: pn ... saved below as we go)
            lda         #0
            pha                                             ; Assignments' first (2) ...
            pha
            pha                                             ;   redirections' first (2) ...
            pha
            pha                                             ;   words' first (2) ...
            pha
            pha                                             ;   and last (2)
            pha
            lda         #1                                  ; (Assignments may come: till the first word)
            sta         stop_eq
@next:                                                      ; (The stack: +1/+2 words' last, +3/+4 words' first,
            jsr         lex_peek                            ;   +5/+6 redirections', +7/+8 assignments')
            cmp         #T_REDIR
            bne         @word
            jsr         redir
            tsx
            lda         $0105,X                             ; At the front of the redirections
            ldy         #0
            sta         (p0),Y
            lda         $0106,X
            iny
            sta         (p0),Y
            lda         p0
            sta         $0105,X
            lda         p0 + 1
            sta         $0106,X
            bra         @next

@word:
            jsr         wordstart
            bcc         :+
            jmp         @end
:
            jsr         word
            lda         stop_eq                             ; An assignment?  A name, then =
            beq         @arg
            jsr         lex_peek
            cmp         #T_EQ
            bne         @first
            lda         tok_adj
            beq         @first
            jsr         isname
            bcs         @first
            jsr         lex_next                            ; name=value
            PUSHW       pn
            jsr         lex_peek
            jsr         wordstart
            bcc         :+
            jsr         emptyword                           ; (Nothing after it: the empty list)
            bra         :++
:
            jsr         word
:
            lda         #6
            jsr         node
            ldy         #5
            lda         pn + 1
            sta         (p0),Y
            dey
            lda         pn
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            tsx                                             ; At the front of the assignments
            lda         $0107,X
            ldy         #0
            sta         (p0),Y
            lda         $0108,X
            iny
            sta         (p0),Y
            lda         p0
            sta         $0107,X
            lda         p0 + 1
            sta         $0108,X
            jmp         @next

@first:
            stz         stop_eq
@arg:
            lda         #4                                  ; A word list node, at the end
            jsr         node
            ldy         #0
            tya
            sta         (p0),Y
            iny
            sta         (p0),Y
            iny
            lda         pn
            sta         (p0),Y
            iny
            lda         pn + 1
            sta         (p0),Y
            tsx
            lda         $0101,X                             ; After the last, or the first
            ora         $0102,X
            bne         :+
            lda         p0
            sta         $0103,X
            lda         p0 + 1
            sta         $0104,X
            bra         :++
:
            lda         $0101,X
            sta         pq
            lda         $0102,X
            sta         pq + 1
            ldy         #0
            lda         p0
            sta         (pq),Y
            iny
            lda         p0 + 1
            sta         (pq),Y
:
            tsx
            lda         p0
            sta         $0101,X
            lda         p0 + 1
            sta         $0102,X
            jmp         @next

@end:
            lda         #7
            jsr         node
            lda         #C_SIMPLE
            sta         (p0)
            pla                                             ; (Words' last)
            pla
            ldy         #1                                  ; +1 words, +3 redirections, +5 assignments
:
            pla
            sta         (p0),Y
            iny
            cpy         #7
            bne         :-
            stz         stop_eq
            PULLW       pr
            PULLW       pl
            ldy         #1                                  ; Nothing at all?  Then it isn't a command
            lda         (p0),Y
            ldy         #3
            ora         (p0),Y
            ldy         #5
            ora         (p0),Y
            ldy         #2
            ora         (p0),Y
            ldy         #4
            ora         (p0),Y
            ldy         #6
            ora         (p0),Y
            bne         :+
            jmp         syntax
:
            lda         p0
            ldx         p0 + 1
            rts

; A redirection: +0 the next, +2 its type, +3 its fd, +4 the other fd, +5 its file (a word).  OUT: p0 = it
redir:
            lda         tok_rtype
            pha
            lda         tok_fd
            pha
            lda         tok_fd2
            pha
            lda         tok_rtype
            sta         t1
            jsr         lex_next
            stz         pn
            stz         pn + 1
            lda         t1                                  ; (A file, or not?)
            cmp         #RD_DUP
            bcs         :+
            jsr         word                                ; (pn: its file)
:
            lda         #7
            jsr         node
            ldy         #6
            lda         pn + 1
            sta         (p0),Y
            dey
            lda         pn
            sta         (p0),Y
            dey
            pla                                             ; fd2
            sta         (p0),Y
            dey
            pla                                             ; fd
            sta         (p0),Y
            dey
            pla                                             ; type
            sta         (p0),Y
            rts

; Redirections after a { body }: a list.  OUT: .A/.X
epilog:
            stz         pq
            stz         pq + 1
@more:
            jsr         lex_peek
            cmp         #T_REDIR
            bne         @done
            PUSHW       pq
            jsr         redir
            ldy         #0
            pla
            iny
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            MOVR        pq, p0
            bra         @more

@done:
            lda         pq
            ldx         pq + 1
            rts

; ****************************************************************************
; Words

; words: a word list, as long as words come.  OUT: .A/.X (0: none)
words:
            PUSHW       pl
            PUSHW       pr
            stz         pl
            stz         pl + 1
            stz         pr
            stz         pr + 1
@more:
            jsr         lex_peek
            jsr         wordstart
            bcs         @done
            PUSHW       pl                                  ; (The list's first and last: a word uses them)
            PUSHW       pr
            jsr         word
            PULLW       pr
            PULLW       pl
            lda         #4
            jsr         node
            ldy         #0
            tya
            sta         (p0),Y
            iny
            sta         (p0),Y
            iny
            lda         pn
            sta         (p0),Y
            iny
            lda         pn + 1
            sta         (p0),Y
            lda         pr                                  ; After the last, or the first
            ora         pr + 1
            bne         :+
            MOVR        pl, p0
            bra         :++
:
            ldy         #0
            lda         p0
            sta         (pr),Y
            iny
            lda         p0 + 1
            sta         (pr),Y
:
            MOVR        pr, p0
            bra         @more

@done:
            lda         pl
            ldx         pl + 1
            sta         pn
            stx         pn + 1
            PULLW       pr
            PULLW       pl
            lda         pn
            ldx         pn + 1
            rts

; C = 0 if token .A can start a word
wordstart:
            ldx         #NWSTART - 1
:
            cmp         wstarts,X
            beq         @yes
            dex
            bpl         :-
            sec
            rts

@yes:
            clc
            rts

; word: pieces joined (^, or next to each other).  OUT: pn = it (.A/.X too)
word:
            jsr         piece
@more:
            jsr         lex_peek
            cmp         #T_CARET
            beq         @caret
            ldx         tok_adj                             ; (Next to it: joined)
            beq         @done
            ldx         stop_eq                             ; (= after an assignment's name: not)
            beq         :+
            cmp         #T_EQ
            beq         @done
:
            jsr         wordstart
            bcs         @done
            bra         @join

@caret:
            jsr         lex_next
@join:
            PUSHW       pn
            jsr         piece
            MOVR        pr, pn
            PULLW       pl
            lda         #W_CAT
            jsr         node2
            sta         pn
            stx         pn + 1
            bra         @more

@done:
            lda         pn
            ldx         pn + 1
            rts

; piece: text, $name, $name(words), $#name, $"name, `{body}, (words).  OUT: pn = it
piece:
            jsr         lex_next
            cmp         #T_WORD
            bne         :+
            jmp         lit
:
            cmp         #T_EQ
            bne         :+
            LDR         p1, s_eq                            ; (= as text)
            lda         #1
            jmp         litp1
:
            cmp         #T_DOLLAR
            beq         @var
            cmp         #T_COUNT
            beq         @count
            cmp         #T_FLAT
            beq         @count
            cmp         #T_BQ
            bne         :+
            jmp         @bq
:
            cmp         #T_LPAREN
            bne         :+
            jmp         @list
:
            jmp         syntax

@var:
            jsr         varname
            PUSHW       pn
            stz         pr
            stz         pr + 1
            jsr         lex_peek                            ; $name(words): its subscripts
            cmp         #T_LPAREN
            bne         :+
            lda         tok_adj
            beq         :+
            jsr         lex_next
            jsr         words
            sta         pr
            stx         pr + 1
            lda         #T_RPAREN
            jsr         expect
:
            PULLW       pl
            lda         #W_VAR
            jsr         node2
            jmp         @set

@count:
            pha                                             ; (T_COUNT or T_FLAT)
            jsr         varname
            MOVR        pl, pn
            pla
            ldx         #W_COUNT
            cmp         #T_COUNT
            beq         :+
            ldx         #W_FLAT
:
            txa
            jsr         node1
            jmp         @set

@bq:
            lda         #T_LBRACE
            jsr         expect
            jsr         lex_pos                             ; Its text, for a subshell
            pha
            phx
            jsr         body
            sta         pl
            stx         pl + 1
            lda         #7
            jsr         node
            lda         #W_BQ
            sta         (p0)
            ldy         #1
            lda         pl
            sta         (p0),Y
            iny
            lda         pl + 1
            sta         (p0),Y
            ldy         #4
            pla
            sta         (p0),Y
            dey
            pla
            sta         (p0),Y
            ldy         #5                                  ; Its end: where the } was (lex_pos before it, kept
            lda         bq_end                              ;   by body's expect)
            sta         (p0),Y
            iny
            lda         bq_end + 1
            sta         (p0),Y
            lda         p0
            ldx         p0 + 1
            bra         @set

@list:
            jsr         words
            sta         pl
            stx         pl + 1
            lda         #T_RPAREN
            jsr         expect
            lda         #W_LIST
            jsr         node1
@set:
            sta         pn
            stx         pn + 1
            rts

; A $name's name (tok_text): a W_LIT.  OUT: pn
varname:
            lda         tok_len
            bne         lit
            LDR         r0, s_dollar
            FAR1        rc_error                            ; (It doesn't come back)

; The token's text: a W_LIT node.  OUT: pn = it (.A/.X too)
lit:
            LDR         p1, tok_text
            lda         tok_len
; .A bytes at p1: a W_LIT node.  OUT: pn
litp1:
            pha
            clc
            adc         #2
            ldx         #0
            bcc         :+
            inx
:
            FAR1        arena_alloc
            sta         p0
            stx         p0 + 1
            lda         #W_LIT
            sta         (p0)
            pla
            ldy         #1
            sta         (p0),Y
            tax
            beq         @done
:
            dey
            lda         (p1),Y
            iny
            iny
            sta         (p0),Y
            dex
            bne         :-
@done:
            lda         p0
            ldx         p0 + 1
            sta         pn
            stx         pn + 1
            rts

; The empty list as a word: ().  OUT: pn
emptyword:
            stz         pl
            stz         pl + 1
            lda         #W_LIST
            jsr         node1
            sta         pn
            stx         pn + 1
            rts

; C = 0 if the word pn is a name: text, unquoted, letters, digits and _ (an assignment's)
isname:
            lda         (pn)
            cmp         #W_LIT
            bne         @no
            ldy         #1
            lda         (pn),Y
            beq         @no
            tax
            iny
:
            lda         (pn),Y
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         :+
            cmp         #'_'
            beq         :+
            and         #$DF
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            bcs         @no
:
            iny
            dex
            bne         :--
            clc
            rts

@no:
            sec
            rts

; ****************************************************************************
; Nodes

; .A bytes from the arena: p0 = them
node:
            ldx         #0
            FAR1        arena_alloc
            sta         p0
            stx         p0 + 1
            rts

; A node of type .A with one part, pl.  OUT: .A/.X = it
node1:
            pha
            lda         #3
            jsr         node
            pla
            sta         (p0)
            ldy         #1
            lda         pl
            sta         (p0),Y
            iny
            lda         pl + 1
            sta         (p0),Y
            lda         p0
            ldx         p0 + 1
            rts

; A node of type .A with two parts, pl and pr.  OUT: .A/.X = it
node2:
            pha
            lda         #5
            jsr         node
            pla
            sta         (p0)
            ldy         #1
            lda         pl
            sta         (p0),Y
            iny
            lda         pl + 1
            sta         (p0),Y
            iny
            lda         pr
            sta         (p0),Y
            iny
            lda         pr + 1
            sta         (p0),Y
            lda         p0
            ldx         p0 + 1
            rts

; Token .A next, taken; anything else is a syntax error.  (The position before it kept in bq_end)
expect:
            pha
            jsr         lex_pos
            sta         bq_end
            stx         bq_end + 1
            jsr         lex_next
            sta         t0
            pla
            cmp         t0
            bne         syntax
            rts

; A syntax error: the rest of the line dropped, and the command ended (rc_error)
syntax:
            jsr         lex_flush
            LDR         r0, s_syntax
            FAR1        rc_error                            ; (It doesn't come back)

; ****************************************************************************
; The text of a function, or a subshell's: parsed as a body (to its end), from the source rc.s has pushed.
; OUT: .A/.X = its tree
parse_text:
            lda         #1
            sta         mode
            jsr         seq
            jsr         lex_peek
            cmp         #T_EOF
            bne         syntax
            lda         pn
            ldx         pn + 1
            rts

.bss
mode:       .res        1                                   ; seq's: 0 a line, 1 a body
stop_eq:    .res        1                                   ; word's: <> 0: = ends it (an assignment's name)
bq_end:     .res        2

.segment "RODATA2"
key_toks:   .byte       T_IF, T_FOR, T_WHILE, T_SWITCH, T_CASE, T_TWIDDLE, T_FN
NKEYS       = * - key_toks
key_vec:    .word       k_if, k_for, k_while, k_switch, k_case, k_twiddle, k_fn
wstarts:    .byte       T_WORD, T_DOLLAR, T_COUNT, T_FLAT, T_BQ, T_LPAREN, T_EQ
NWSTART     = * - wstarts
s_eq:       .byte       "="

.rodata                                                     ; (The first bank's: rc_error reads them there)
s_syntax:   .byte       "syntax error", 0
s_dollar:   .byte       "bad $", 0
