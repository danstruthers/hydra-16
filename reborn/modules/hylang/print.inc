; ****************************************************************************
; print.inc - hylang's printer (phase 2), in its second bank: a value as danlang's REPL shows it (repr) or as print
; does (a string's or a character's text as it is): its reference.md, section 2's "Printing", and LVal.cs's ToStr.
; Into the output (out.inc's out_byte).
;   Without recursion: each list being printed is on a stack (the cons it's at, its closer), PRINT_DEPTH deep (one
; deeper is "..."); so is a function (its formals, then its body), and a hash's items (<hash>, then its entries
; {key value tag...} and its tags as a list; a hash in a hash, its list alone).  Nothing here allocates, so the
; values stay where they are.  Streams get their own form in the phase that makes them (as yet, a placeholder).

PRINT_DEPTH     = 255

.zeropage
pv:         .res        2                                   ; The printer's: a value ...

.bss
pn:         .res        2                                   ;   a count ...
pm:         .res        1                                   ;   the form (0: repr, 1: print's, 2: save's) ...
pd:         .res        1                                   ;   and the lists open ...
p_inhash:   .res        1                                   ;   and the hashes' among them
ps_lo:      .res        PRINT_DEPTH                         ; Each list open: the cons it's at (its car next) ...
ps_hi:      .res        PRINT_DEPTH
ps_close:   .res        PRINT_DEPTH                         ;   its closer (bit 7: a hash's) ...
ps_mode:    .res        PRINT_DEPTH                         ;   and what it is: 0 a list; 1, 2 a function, its formals
                                                            ;   or its body out
p_digit:    .res        1                                   ; (char_name's: the code looked for)

.segment "CODE2"

; A value out.  IN: .A, .X = it; .Y = the form: 0 repr (the REPL's), 1 print's (a string or a character, as it is;
; what's in a list, as repr has it)
print_val:
            sty         pm
            stz         pd
            stz         p_inhash
@value:
            sta         pv
            stx         pv + 1
            lsr
            bcc         @lb747
            jmp         @fixnum
@lb747:
            cpx         #IMM_PAGES
            bcs         @lb746
            jmp         @imm
@lb746:
            lda         pk,x
            cmp         #PK_SCONS
            bne         @lb21
            jmp         @slist
@lb21:
            cmp         #PK_QCONS
            bne         @lb20
            jmp         @qlist
@lb20:
            cmp         #PK_SCOPED
            bne         @lb19
            jmp         @scoped
@lb19:
            cmp         #PK_STRING
            bne         @lb205
            jmp         @string
@lb205:
            cmp         #PK_SYMBOL
            bne         @lb204
            jmp         @symbol
@lb204:
            cmp         #PK_ATOM
            bne         @lb203
            jmp         @atom
@lb203:
            cmp         #PK_ERROR
            bne         @lb200
            jmp         @error
@lb200:
            cmp         #PK_FUNC
            bne         @lb202
            jmp         @func
@lb202:
            cmp         #PK_PARTIAL
            bne         @lb201
            jmp         @partial
@lb201:
            cmp         #PK_HASH
            bne         @lb18
            jmp         @hash
@lb18:
            cmp         #PK_BUFFER
            bne         @lb17b
            jsr         print_buffer
            jmp         @after
@lb17b:
            cmp         #PK_NUMBER                          ; (A number: numlib.inc's)
            bne         :+
            lda         pv
            ldx         pv + 1
            FARN        3, num_print
            jmp         @after
:
            tax                                             ; (The rest, as yet: a placeholder)
            lda         pt_hi,x
            pha
            lda         pt_lo,x
            plx
            jsr         out_text
            jmp         @after
@fixnum:
            lda         nl_dec                              ; (In a base not decimal, or with its prefix: numlib.inc's)
            bne         :+
            lda         pv
            ldx         pv + 1
            FARN        3, num_print
            jmp         @after
:
            lda         pv + 1
            cmp         #$80
            ror
            tax
            lda         pv
            ror
            jsr         out_dec
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
@func:
            lda         #<s_fnform                          ; (<function>(fn formals body): an entry on the stack)
            ldx         #>s_fnform
            jsr         out_text
            ldx         pd
            cpx         #PRINT_DEPTH
            bcc         @lb17
            jmp         @deep
@lb17:
            lda         #1
            sta         ps_mode,x
            lda         pv
            sta         ps_lo,x
            lda         pv + 1
            sta         ps_hi,x
            inc         pd
            lsr         pm                                  ; (What's in it: as repr has it, as danlang's ToStr;
            asl         pm                                  ;   save's as save's)
            lda         pv                                  ; (Its formals first)
            ldx         pv + 1
            ldy         #0
            jsr         cell_get
            jmp         @value
@partial:
            lda         #<s_fnopen                          ; (<function>(eq 1): the built-in, its values)
            ldx         #>s_fnopen
            jsr         out_text
            lda         pv
            ldx         pv + 1
            ldy         #0
            jsr         cell_get                            ; (Its number: the value's bits 8-1)
            jsr         pr_bname
            lda         pv                                  ; (Its values: a list's items, after a space)
            ldx         pv + 1
            ldy         #1
            jsr         cell_get
            sta         pv
            stx         pv + 1
            ora         pv + 1
            bne         :+
            lda         #')'
            jsr         out_byte
            jmp         @after
:
            ldx         pd
            cpx         #PRINT_DEPTH
            bcc         @lb301
            jmp         @deep
@lb301:
            lda         #' '
            jsr         out_byte
            lda         #')'
            sta         ps_close,x
            stz         ps_mode,x
            lda         pv
            sta         ps_lo,x
            lda         pv + 1
            sta         ps_hi,x
            inc         pd
            lsr         pm                                  ; (What's in it: as repr has it, as danlang's ToStr;
            asl         pm                                  ;   save's as save's)
            jmp         @item
@hash:
            lda         pv                                  ; (A hash: a proxy's, its own)
            ldx         pv + 1
            ldy         #H_FLAGS
            jsr         cell_get
            and         #H_PROXY
            beq         :+
            lda         pv
            ldx         pv + 1
            ldy         #H_REAL
            jsr         cell_get
            sta         pv
            stx         pv + 1
:
            lda         pm                                  ; (save's: #( ... ), and in a hash too)
            cmp         #2
            bne         :+
            lda         #<s_hashopen
            ldx         #>s_hashopen
            jsr         out_text
            bra         :++
:
            lda         p_inhash                            ; (<hash>, unless it's in a hash)
            bne         :+
            lda         #<s_hash
            ldx         #>s_hash
            jsr         out_text
:
            lda         pv                                  ; (Its items: a list; none, NIL)
            ldx         pv + 1
            ldy         #H_ITEMS
            jsr         cell_get
            sta         pv
            stx         pv + 1
            ora         pv + 1
            bne         :+
            lda         #<s_nil                             ; (None: NIL; save's {})
            ldx         #>s_nil
            ldy         pm
            cpy         #2
            bne         @nilout
            lda         #<s_hashnone
            ldx         #>s_hashnone
@nilout:
            jsr         out_text
            jmp         @after
:
            inc         p_inhash
            lda         #'{'
            ldy         #'}' | $80
            jmp         @list
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
            stz         ps_mode,x
            lda         pv
            sta         ps_lo,x
            lda         pv + 1
            sta         ps_hi,x
            inc         pd
            lsr         pm                                  ; (What's in it: as repr has it, as danlang's ToStr;
            asl         pm                                  ;   save's as save's)
            bra         @item
@deep:
            lda         #<s_deep
            ldx         #>s_deep
            jsr         out_text
@after:                                                     ; (An item out: on with the list it's in)
            ldx         pd
            beq         @done
            dex
            lda         ps_mode,x
            bne         @fmode
            lda         ps_lo,x
            ora         ps_hi,x
            bne         @space
            lda         ps_close,x
            bpl         :+
            dec         p_inhash                            ; (A hash's end: save's, })
            and         #$7F
            ldy         pm
            cpy         #2
            bne         :+
            jsr         out_byte
            lda         #')'
:
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
@fmode:
            cmp         #1                                  ; (A function: its formals out, its body next ...
            bne         @fend
            inc         ps_mode,x
            lda         #' '
            jsr         out_byte
            lda         ps_lo,x
            pha
            lda         ps_hi,x
            tax
            pla
            ldy         #1
            jsr         cell_get
            jmp         @value
@fend:
            lda         #')'                                ;   its body out: its end)
            jsr         out_byte
            dec         pd
            jmp         @after

; An immediate (pv) out: NIL, T, (), exit, a character, a built-in
print_imm:
            ldx         pv + 1
            cpx         #>CHAR0
            bcs         @char
            cpx         #0
            bne         @other                              ; (Page 1: none)
            lda         pm                                  ; (save's: NIL {}, () ())
            cmp         #2
            bne         :+
            lda         pv
            cmp         #<SNIL + 1
            bcs         :+
            lsr
            tax
            lda         ser_lo,x
            pha
            lda         ser_hi,x
            tax
            pla
            jmp         out_text
:
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
            cpx         #1
            beq         @raw
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
            lda         #<s_fnopen                          ; (<function>(+))
            ldx         #>s_fnopen
            jsr         out_text
            lda         pv
            ldx         pv + 1
            jsr         pr_bname
            lda         #')'
            jmp         out_byte

; The built-in .A, .X's name out (the first table's or the second's)
pr_bname:
            pha
            txa
            lsr
            pla
            ror
            cpx         #>BUILTIN2
            tax
            bcs         :+
            jmp         bi_name
:
            jmp         bi_name2

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
            cmp         #1
            beq         @bytes
            lda         #'"'
            jsr         out_byte
@bytes:
            lda         pn
            ora         pn + 1
            beq         @end
            lda         (hp)
            ldx         pm
            cpx         #1
            beq         @raw
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
            cmp         #1
            beq         @done
            lda         #'"'
            jsr         out_byte
@done:
            rts

; A buffer (pv) out: <buffer>{1 2}, both forms; save's (buffer {1 2}), as it reads back
print_buffer:
            ldx         #<s_buffer
            ldy         #>s_buffer
            lda         pm
            cmp         #2
            bne         :+
            ldx         #<s_bufsave
            ldy         #>s_bufsave
:
            txa
            phy
            plx
            jsr         out_text
            lda         pv                                  ; (Its length, its bytes)
            ldx         pv + 1
            jsr         deref
            lda         (hp)
            sta         pn
            ldy         #1
            lda         (hp),y
            sta         pn + 1
            iny
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ldx         #0                                  ; (A space before each but the first)
@byte:
            lda         pn
            ora         pn + 1
            beq         @end
            txa
            beq         :+
            lda         #' '
            jsr         out_byte
:
            lda         (hp)
            ldx         #0
            jsr         out_dec
            inc         hp
            bne         :+
            inc         hp + 1
:
            lda         pn
            bne         :+
            dec         pn + 1
:
            dec         pn
            ldx         #1
            bra         @byte
@end:
            lda         #'}'
            jsr         out_byte
            lda         pm
            cmp         #2
            bne         :+
            lda         #')'
            jsr         out_byte
:
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

.segment "RODATA2"

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
            .lobytes    s_scope, s_imm
pt_hi:      .hibytes    s_imm, s_imm, s_imm, s_imm, s_imm, s_imm, s_imm, s_number, s_number, s_number, s_number
            .hibytes    s_imm, s_function, s_imm, s_imm, s_function, s_hash, s_stream, s_scope, s_scope, s_scope
            .hibytes    s_scope, s_imm
s_nil:      .byte       "NIL", 0
s_t:        .byte       "T", 0
s_exitv:    .byte       "exit", 0
s_unbound:  .byte       "<unbound>", 0
s_imm:      .byte       "<?>", 0
s_fnform:   .byte       "<function>(fn ", 0
s_fnopen:   .byte       "<function>(", 0
s_number:   .byte       "<number>", 0
s_function: .byte       "<function>", 0
s_hash:     .byte       "<hash>", 0
s_hashopen: .byte       "#(", 0
s_hashnone: .byte       "{})", 0
ser_lo:     .lobytes    s_qnone, s_t, s_snone
ser_hi:     .hibytes    s_qnone, s_t, s_snone
s_qnone:    .byte       "{}", 0
s_snone:    .byte       "()", 0
s_stream:   .byte       "<stream>", 0
s_buffer:   .byte       "<buffer>{", 0
s_bufsave:  .byte       "(buffer {", 0
s_scope:    .byte       "<scope>", 0
s_error:    .byte       "Error: ", 0
s_deep:     .byte       "...", 0
