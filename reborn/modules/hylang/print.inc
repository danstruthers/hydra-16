; ****************************************************************************
; print.inc - hylang's printer (phase 2), in its second bank: a value as danlang's REPL shows it (repr) or as print
; does (a string's or a character's text as it is): its reference.md, section 2's "Printing", and LVal.cs's ToStr.
; Into the output (out_byte: hylang.s's).
;   Without recursion: each list being printed is on a stack (the cons it's at, its closer), PRINT_DEPTH deep (one
; deeper is "...").  Nothing here allocates, so the values stay where they are.  Numbers past a fixnum, functions,
; hashes and streams get their own forms in the phases that make them (as yet, a placeholder).

PRINT_DEPTH     = 255

.zeropage
pv:         .res        2                                   ; The printer's: a value ...
pn:         .res        2                                   ;   a count ...
pm:         .res        1                                   ;   the form (0: repr, 1: print's) ...
pd:         .res        1                                   ;   and the lists open

.bss
ps_lo:      .res        PRINT_DEPTH                         ; Each list open: the cons it's at (its car next) ...
ps_hi:      .res        PRINT_DEPTH
ps_close:   .res        PRINT_DEPTH                         ;   and its closer
p_digit:    .res        1                                   ; (out_int's: a digit, and a difference)
p_diff:     .res        1

.segment "CODE2"

; A value out.  IN: .A, .X = it; .Y = the form: 0 repr (the REPL's), 1 print's
print_val:
            sty         pm
            stz         pd
@value:
            sta         pv
            stx         pv + 1
            lsr
            bcs         @fixnum
            cpx         #IMM_PAGES
            bcc         @imm
            lda         pk,x
            cmp         #PK_SCONS
            beq         @slist
            cmp         #PK_QCONS
            beq         @qlist
            cmp         #PK_SCOPED
            beq         @scoped
            cmp         #PK_STRING
            beq         @string
            cmp         #PK_SYMBOL
            beq         @symbol
            cmp         #PK_ATOM
            beq         @atom
            cmp         #PK_ERROR
            beq         @error
            tax                                             ; (The rest, as yet: a placeholder)
            lda         pt_hi,x
            pha
            lda         pt_lo,x
            plx
            jsr         out_text
            jmp         @after
@fixnum:
            lda         pv + 1
            cmp         #$80
            ror
            tax
            lda         pv
            ror
            jsr         out_int
            jmp         @after
@imm:
            jsr         print_imm
            jmp         @after
@string:
            jsr         print_string
            jmp         @after
@atom:
            lda         #':'
            jsr         out_byte
@symbol:
            jsr         print_name
            jmp         @after
@error:
            lda         #<s_error
            ldx         #>s_error
            jsr         out_text
            lda         pm                                  ; (Its message, as print shows it)
            pha
            lda         #1
            sta         pm
            lda         pv
            ldx         pv + 1
            ldy         #0
            jsr         cell_get
            sta         pv
            stx         pv + 1
            jsr         print_string
            pla
            sta         pm
            jmp         @after
@scoped:
            lda         pv                                  ; (A Q-expression and its scope: the Q-expression)
            ldx         pv + 1
            ldy         #0
            jsr         cell_get
            jmp         @value
@slist:
            lda         #'('
            ldy         #')'
            bra         @list
@qlist:
            lda         #'{'
            ldy         #'}'
@list:
            ldx         pd
            cpx         #PRINT_DEPTH
            bcs         @deep
            jsr         out_byte
            tya
            sta         ps_close,x
            lda         pv
            sta         ps_lo,x
            lda         pv + 1
            sta         ps_hi,x
            inc         pd
            bra         @item
@deep:
            lda         #<s_deep
            ldx         #>s_deep
            jsr         out_text
@after:                                                     ; (An item out: on with the list it's in)
            ldx         pd
            beq         @done
            dex
            lda         ps_lo,x
            ora         ps_hi,x
            bne         @space
            lda         ps_close,x
            jsr         out_byte
            dec         pd
            bra         @after
@space:
            lda         #' '
            jsr         out_byte
@item:                                                      ; (The cons the list's at: its car out, its cdr kept)
            ldx         pd
            dex
            phx
            lda         ps_lo,x
            pha
            lda         ps_hi,x
            tax
            pla
            jsr         deref
            plx
            ldy         #2
            lda         (hp),y
            sta         ps_lo,x
            iny
            lda         (hp),y
            sta         ps_hi,x
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            jmp         @value
@done:
            rts

; An immediate (pv) out: NIL, T, (), exit, a character, a built-in
print_imm:
            ldx         pv + 1
            cpx         #>CHAR0
            bcs         @char
            cpx         #0
            bne         @other                              ; (Page 1: none)
            lda         pv
            cmp         #UNBOUND + 2
            bcs         @other
            lsr
            tax
            lda         imm_hi,x
            pha
            lda         imm_lo,x
            plx
            jmp         out_text
@other:
            lda         #<s_imm
            ldx         #>s_imm
            jmp         out_text
@char:
            cpx         #>BUILTIN0
            bcs         @builtin
            txa                                             ; (Its code: the value's bits 8-1)
            lsr
            lda         pv
            ror
            ldx         pm
            bne         @raw
            pha
            lda         #'\'
            jsr         out_byte
            pla
            jsr         char_name
            bcs         @raw
            ldy         #2                                  ; (Its name)
@name:
            lda         (hq),y
            jsr         out_byte
            iny
            dex
            bne         @name
            rts
@raw:
            jmp         out_byte
@builtin:
            lda         #<s_builtin
            ldx         #>s_builtin
            jmp         out_text

; The name a character (.A) prints by.  OUT: C = 0, hq = its entry in cn_table, .X = its name's length; C = 1 if
; it has none (.A kept)
char_name:
            cmp         #$80
            bcs         @none                               ; (A byte past ASCII: none)
            ora         #CN_PRINT
            sta         p_digit
            lda         #<cn_table
            sta         hq
            lda         #>cn_table
            sta         hq + 1
@entry:
            lda         (hq)
            beq         @end
            ldy         #1
            lda         (hq),y
            cmp         p_digit
            beq         @found
            lda         (hq)
            sec
            adc         #1
            adc         hq
            sta         hq
            bcc         @entry
            inc         hq + 1
            bra         @entry
@found:
            lda         (hq)
            tax
            clc
            rts
@end:
            lda         p_digit
            and         #$7F
@none:
            sec
            rts

; A string (pv) out: repr, between quotes, escaped (\n ..., \xHH, as it reads back); print's, as it is
print_string:
            lda         pv
            ldx         pv + 1
            jsr         deref
            lda         (hp)                                ; (Its length ...
            sta         pn
            ldy         #1
            lda         (hp),y
            sta         pn + 1
            iny
            lda         (hp),y                              ;   its bytes)
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            lda         pm
            bne         @bytes
            lda         #'"'
            jsr         out_byte
@bytes:
            lda         pn
            ora         pn + 1
            beq         @end
            lda         (hp)
            ldx         pm
            bne         @raw
            jsr         out_escaped
            bra         @on
@raw:
            jsr         out_byte
@on:
            inc         hp
            bne         :+
            inc         hp + 1
:
            lda         pn
            bne         :+
            dec         pn + 1
:
            dec         pn
            bra         @bytes
@end:
            lda         pm
            bne         @done
            lda         #'"'
            jsr         out_byte
@done:
            rts

; A string's byte (.A) out as repr has it: \" \\, a control character's escape (\n, or \xHH), any other as it is
out_escaped:
            cmp         #'"'
            beq         @slash
            cmp         #'\'
            beq         @slash
            cmp         #' '
            bcc         @control
            cmp         #127
            beq         @hex
            jmp         out_byte
@control:
            tax
            lda         esc_letter,x
            beq         @code
@slash:
            pha
            lda         #'\'
            jsr         out_byte
            pla
            jmp         out_byte
@code:
            txa
@hex:
            pha
            lda         #'\'
            jsr         out_byte
            lda         #'x'
            jsr         out_byte
            pla
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         @digit
            pla
            and         #$0F
@digit:
            tax
            lda         hex_digit,x
            jmp         out_byte

; A symbol's or an atom's name (pv) out
print_name:
            lda         pv
            ldx         pv + 1
            ldy         #0
            jsr         cell_get
            jsr         blob_data
            ldy         #0
@byte:
            cpy         hn
            beq         @done
            lda         (hp),y
            jsr         out_byte
            iny
            bne         @byte
@done:
            rts

; A number (.A, .X: -32768 to 32767) out, in decimal
out_int:
            cpx         #$80
            bcc         @plus
            pha
            lda         #'-'
            jsr         out_byte
            pla
            eor         #$FF
            clc
            adc         #1
            pha
            txa
            eor         #$FF
            adc         #0
            tax
            pla
@plus:
            sta         pn
            stx         pn + 1
            ldy         #0                                  ; (Digits out: none, so 0s before them aren't)
            ldx         #3
@power:
            lda         #'0'
            sta         p_digit
@less:
            lda         pn                                  ; (The power of ten taken while it can be)
            sec
            sbc         pow_lo,x
            sta         p_diff
            lda         pn + 1
            sbc         pow_hi,x
            bcc         @digit
            sta         pn + 1
            lda         p_diff
            sta         pn
            inc         p_digit
            bra         @less
@digit:
            lda         p_digit
            cpy         #0
            bne         @out
            cmp         #'0'
            beq         @next
@out:
            jsr         out_byte
            iny
@next:
            dex
            bpl         @power
            lda         pn
            ora         #'0'
            jmp         out_byte

.segment "RODATA2"

pow_lo:     .lobytes    10, 100, 1000, 10000
pow_hi:     .hibytes    10, 100, 1000, 10000
hex_digit:  .byte       "0123456789ABCDEF"

; A control character's escape in a string (0: \xHH)
esc_letter: .byte       '0', 0, 0, 0, 0, 0, 0, 'a', 'b', 't', 'n', 'v', 'f', 'r', 0, 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'e', 0, 0, 0, 0

; The immediates' forms (NIL, T, (), exit, unbound) ...
imm_lo:     .lobytes    s_nil, s_t, s_nil, s_exitv, s_unbound
imm_hi:     .hibytes    s_nil, s_t, s_nil, s_exitv, s_unbound
; ... and, as yet, the placeholders for what later phases print (by kind)
pt_lo:      .lobytes    s_imm, s_imm, s_imm, s_imm, s_imm, s_imm, s_imm, s_number, s_number, s_number, s_number
            .lobytes    s_imm, s_function, s_imm, s_imm, s_function, s_hash, s_stream, s_scope, s_scope, s_scope
            .lobytes    s_scope
pt_hi:      .hibytes    s_imm, s_imm, s_imm, s_imm, s_imm, s_imm, s_imm, s_number, s_number, s_number, s_number
            .hibytes    s_imm, s_function, s_imm, s_imm, s_function, s_hash, s_stream, s_scope, s_scope, s_scope
            .hibytes    s_scope
s_nil:      .byte       "NIL", 0
s_t:        .byte       "T", 0
s_exitv:    .byte       "exit", 0
s_unbound:  .byte       "<unbound>", 0
s_imm:      .byte       "<?>", 0
s_builtin:  .byte       "<function>(?)", 0
s_number:   .byte       "<number>", 0
s_function: .byte       "<function>", 0
s_hash:     .byte       "<hash>", 0
s_stream:   .byte       "<stream>", 0
s_scope:    .byte       "<scope>", 0
s_error:    .byte       "Error: ", 0
s_deep:     .byte       "...", 0
