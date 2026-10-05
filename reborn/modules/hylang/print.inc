; ****************************************************************************
; print.inc - hylang's printer: a value as the REPL shows it (print_repr: danlang's ToStr), or as print shows it
; (print_display: a string's or a character's text as it is, anything else as print_repr has it).  A list's items are
; always as the REPL shows them; lists nested deeper than PR_DEPTH show as "...".

PR_DEPTH        = 20

.bss
pr_display: .res        1                                   ; Not 0: the display form, for the outermost value
pr_depth:   .res        1                                   ; The lists open
pr_hi:      .res        1                                   ; A number's high byte
.segment "CODE2"

; h_v as the REPL shows it.  (Changes h_v)
print_repr:
            stz         pr_display
            bra         print_top

; h_v as print shows it.  (Changes h_v)
print_display:
            lda         #1
            sta         pr_display
print_top:
            stz         pr_depth
            jmp         pr_value

; h_v, at pr_depth
pr_value:
            jsr         type_of
            tax
            lda         pr_jump_lo - 1,x
            sta         ob_p
            lda         pr_jump_hi - 1,x
            sta         ob_p + 1
            jmp         (ob_p)

pr_num:
            lda         h_v                                 ; (A fixnum: its value, h_v >> 1, its sign kept; another,
            lsr                                             ; num_print's, in the third bank)
            bcs         :+
            FARX        lib_num, num_print
            rts
:
            lda         h_v + 1
            cmp         #$80
            ror
            tax
            lda         h_v
            ror
            jmp         out_dec

pr_list:
            jsr         list_of                             ; (NIL, or a Q-expression's chain)
            lda         #'{'
            ldx         #'}'
            jmp         pr_chain

pr_sexpr:
            jsr         list_of
            lda         #'('
            ldx         #')'
            jmp         pr_chain

pr_t:
            LDR         r0, ps_t
            jmp         out_text

pr_exit:
            LDR         r0, ps_exit
            jmp         out_text

pr_func:
            lda         h_v + 1                             ; (A built-in: a constant)
            cmp         #>BUILTIN_END
            bcs         :+
            LDR         r0, ps_builtin
            jmp         out_text
:
            jsr         deref                               ; (A function: <function>(fn {formals} {body}))
            lda         (h_p)
            cmp         #OT_PBUILT
            jeq         pr_pbuilt
            ldy         #1
            lda         (h_p),y
            and         #FN_FEXPR
            pha
            ldy         #5                                  ; (Its body, kept)
            lda         (h_p),y
            pha
            dey
            lda         (h_p),y
            pha
            dey
            lda         (h_p),y                             ; (Its formals)
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            LDR         r0, ps_function
            jsr         out_text
            pla                                             ; (The body under the formals, the flag under it)
            sta         ob_p
            pla
            sta         ob_p + 1
            pla
            beq         :+
            LDR         r0, ps_fexpr
            bra         :++
:
            LDR         r0, ps_fn
:
            lda         ob_p + 1
            pha
            lda         ob_p
            pha
            jsr         out_text
            lda         #'{'
            ldx         #'}'
            jsr         pr_chain
            lda         #' '
            jsr         out_char
            pla
            sta         h_v
            pla
            sta         h_v + 1
            lda         #'{'
            ldx         #'}'
            jsr         pr_chain
            lda         #')'
            jmp         out_char

; A chain of pairs (h_v) as a list between .A and .X, each item as the REPL shows it (NIL if it's empty; "..."
; inside it, if it's nested too deeply)
pr_chain:
            ldy         h_v
            bne         :+
            ldy         h_v + 1
            bne         :+
            LDR         r0, ps_nil
            jmp         out_text
:
            phx
            jsr         out_char
            stz         pr_display
            lda         pr_depth
            cmp         #PR_DEPTH
            bcc         :+
            LDR         r0, ps_dots
            jsr         out_text
            pla
            jmp         out_char
:
            inc         pr_depth
@item:
            lda         h_v
            ora         h_v + 1
            beq         @end
            lda         h_v + 1
            pha
            lda         h_v
            pha
            jsr         car
            jsr         pr_value
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            lda         h_v
            ora         h_v + 1
            beq         @end
            lda         #' '
            jsr         out_char
            bra         @item
@end:
            dec         pr_depth
            pla
            jmp         out_char

pr_char:
            lda         h_v                                 ; (Its code: (h_v - CHARS) / 2)
            lsr
            ldx         h_v + 1
            cpx         #>CHARS + 1
            bcc         :+
            ora         #$80
:
            ldx         pr_display
            beq         @name
            jmp         out_char
@name:
            pha
            lda         #'\'
            jsr         out_char
            pla
            bne         :+
            LDR         r0, ps_null                         ; (0: null)
            jmp         out_text
:
            sta         ob_t                                ; (A name, if it has one: the table's more than
            LDR         ob_p, pr_charnames                  ; 256 bytes, so through a pointer)
@look:
            lda         (ob_p)
            beq         @itself
            cmp         ob_t
            beq         @named
            ldy         #0                                  ; (The next: past the code, the name and its 0)
:
            iny
            lda         (ob_p),y
            bne         :-
            iny
            jsr         @skip
            bra         @look
@named:
            ldy         #1
            jsr         @skip
            jmp         out_text_p
@itself:
            lda         ob_t
            jmp         out_char
@skip:
            tya                                             ; (ob_p moved on by .Y)
            clc
            adc         ob_p
            sta         ob_p
            bcc         :+
            inc         ob_p + 1
:
            rts

pr_string:
            lda         pr_display
            bne         :+
            lda         #'"'
            jsr         out_char
:
            jsr         deref
            lda         (h_p)
            cmp         #OT_LSTR
            beq         @long
            jsr         @cell
            bra         @end
@long:
            ldy         #3                                  ; (Long: each cell of its chain)
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
@next:
            lda         h_v
            ora         h_v + 1
            beq         @end
            lda         h_v + 1
            pha
            lda         h_v
            pha
            jsr         car
            jsr         deref
            jsr         @cell
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            bra         @next
@end:
            lda         pr_display
            bne         :+
            lda         #'"'
            jmp         out_char
:
            rts
@cell:
            ldy         #1                                  ; (A cell's bytes, as they are, or escaped)
            lda         (h_p),y
            sta         ob_n
            clc
            lda         h_p
            adc         #2
            sta         ob_p
            lda         h_p + 1
            sta         ob_p + 1
            lda         pr_display
            beq         @quoted
            ldy         ob_n
            jmp         out_bytes
@quoted:
            ldy         #0
@byte:
            cpy         ob_n
            beq         @cell_end
            lda         (ob_p),y
            sta         ob_t
            ldx         #0                                  ; (An escape, if it's one)
@esc:
            lda         pr_escapes,x
            cmp         #$FF
            beq         @plain
            cmp         ob_t
            beq         @escaped
            inx
            inx
            bra         @esc
@escaped:
            lda         #'\'
            jsr         out_char
            lda         pr_escapes + 1,x
            bra         @out
@plain:
            lda         ob_t
@out:
            jsr         out_char
            iny
            bra         @byte
@cell_end:
            rts

pr_symbol:
            jsr         deref
            lda         (h_p)                               ; (A reference to one: its symbol)
            cmp         #OT_SYMREF
            bne         pr_name
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            jsr         deref
            bra         pr_name

pr_atom:
            jsr         deref
            lda         #':'
            jsr         out_char
pr_name:
            ldy         #1
            lda         (h_p),y
            and         #$7F                                ; (Not SYM_LOCAL)
            pha
            clc
            lda         h_p
            adc         #SYM_NAME
            sta         ob_p
            lda         h_p + 1
            sta         ob_p + 1
            ply
            jmp         out_bytes

pr_error:
            jsr         deref                               ; (Error: and its message's text)
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            LDR         r0, ps_error
            jsr         out_text
            lda         pr_display                          ; (Its message's text as it is)
            pha
            lda         #1
            sta         pr_display
            jsr         pr_string
            pla
            sta         pr_display
            rts

pr_frame:
            LDR         r0, ps_env
            jmp         out_text

; A partial built-in: <function>(its name, the values given it)
pr_pbuilt:
            ldy         #5                                  ; (Its values, for after)
            lda         (h_p),y
            pha
            dey
            lda         (h_p),y
            pha
            dey                                             ; (Its built-in's number)
            lda         (h_p),y
            lsr
            dey
            lda         (h_p),y
            ror
            eor         #$80
            pha
            LDR         r0, ps_function
            jsr         out_text
            pla
            jsr         bi_entry                            ; (Its name)
            lda         (ob_p)
            sta         pr_hi
            ldy         #0
:
            cpy         pr_hi
            beq         :+
            iny
            lda         (ob_p),y
            jsr         out_char
            bra         :-
:
            pla
            sta         h_v
            pla
            sta         h_v + 1
            stz         pr_display
            lda         #')'
            ldx         #1
            jmp         pr_items

; A hash: <hash>, then its entries and tags as from# has them (a hash an entry's value, without its <hash>)
pr_hash:
            LDR         r0, ps_hash
            jsr         out_text
pr_hash_body:
            lda         pr_depth
            cmp         #PR_DEPTH
            bcc         :+
            LDR         r0, ps_dots
            jmp         out_text
:
            inc         pr_depth
            stz         pr_display
            jsr         hash_real
            ldy         #5                                  ; (Its tags, for after its entries)
            lda         (h_p),y
            pha
            dey
            lda         (h_p),y
            pha
            dey
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
            lda         #'{'
            jsr         out_char
            lda         #0                                  ; (The first item: no space before it)
            pha
@entry:
            lda         h_v
            ora         h_v + 1
            beq         @tags
            jsr         @space
            lda         h_v + 1                             ; (Each entry: {key value tag ...})
            pha
            lda         h_v
            pha
            jsr         car
            lda         #'{'
            jsr         out_char
            lda         h_v + 1
            pha
            lda         h_v
            pha
            jsr         car
            jsr         pr_value                            ; (Its key ...
            lda         #' '
            jsr         out_char
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            lda         h_v + 1
            pha
            lda         h_v
            pha
            jsr         car
            jsr         type_of                             ;   its value ...
            cmp         #TY_HASH
            bne         :+
            jsr         pr_hash_body
            bra         :++
:
            jsr         pr_value
:
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            lda         #'}'                                ;   and its tags)
            ldx         #1
            jsr         pr_items
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            bra         @entry
@tags:
            pla                                             ; (Its tags: after a space, if there were entries)
            tax
            pla
            sta         h_v
            pla
            sta         h_v + 1
            lda         #'}'
            jsr         pr_items
            dec         pr_depth
            rts
@space:
            pla                                             ; (A space before an item but the first)
            sta         ob_p
            pla
            sta         ob_p + 1
            pla
            beq         :+
            lda         #' '
            jsr         out_char
:
            lda         #1
            pha
            lda         ob_p + 1
            pha
            lda         ob_p
            pha
            rts

; The chain h_v's items, each after a space (but the first, if .X = 0), then the character .A
pr_items:
            pha
@item:
            lda         h_v
            ora         h_v + 1
            beq         @end
            txa
            beq         :+
            lda         #' '
            jsr         out_char
:
            ldx         #1
            lda         h_v + 1
            pha
            lda         h_v
            pha
            jsr         car
            jsr         pr_value
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            ldx         #1
            bra         @item
@end:
            pla
            jmp         out_char

pr_other:
            LDR         r0, ps_other
            jmp         out_text

.segment "RODATA2"
pr_jump_lo: .lobytes    pr_num, pr_list, pr_t, pr_exit, pr_func, pr_char, pr_string, pr_symbol, pr_atom, pr_sexpr
            .lobytes    pr_error, pr_frame, pr_other, pr_other, pr_hash
pr_jump_hi: .hibytes    pr_num, pr_list, pr_t, pr_exit, pr_func, pr_char, pr_string, pr_symbol, pr_atom, pr_sexpr
            .hibytes    pr_error, pr_frame, pr_other, pr_other, pr_hash

; A string's escapes (danlang's: LVal.StrEscape): the byte, and the letter after the \
pr_escapes: .byte       0, '0', 7, 'a', 8, 'b', 12, 'f', LF, 'n', CR, 'r', TAB, 't', 11, 'v', '\', '\', '"', '"', $FF

; The characters' names in the REPL's form (danlang's: LVal.CharName): the character, its name (zero-terminated)
.macro PRNAME code, name
            .byte       code, name, 0
.endmacro
pr_charnames:
            PRNAME      '\', "backslash"
            PRNAME      '`', "backtick"
            PRNAME      8, "bell"
            PRNAME      LF, "lf"
            PRNAME      12, "ff"
            PRNAME      '"', "quote"
            PRNAME      CR, "cr"
            PRNAME      ')', "rparen"
            PRNAME      '(', "lparen"
            PRNAME      '}', "rcurly"
            PRNAME      '{', "lcurly"
            PRNAME      ']', "rbracket"
            PRNAME      '[', "lbracket"
            PRNAME      '/', "slash"
            PRNAME      ' ', "space"
            PRNAME      TAB, "tab"
            PRNAME      $27, "tick"
            PRNAME      '~', "tilde"
            PRNAME      7, "vspace"
            PRNAME      11, "vtab"
            PRNAME      '!', "bang"
            PRNAME      '@', "at"
            PRNAME      '#', "hash"
            PRNAME      '$', "dollar"
            PRNAME      '%', "percent"
            PRNAME      '^', "caret"
            PRNAME      '&', "amp"
            PRNAME      '|', "pipe"
            PRNAME      '*', "star"
            PRNAME      '>', "gt"
            PRNAME      '<', "lt"
            PRNAME      ',', "comma"
            PRNAME      ':', "colon"
            PRNAME      ';', "semicolon"
            PRNAME      '.', "dot"
            PRNAME      '?', "qmark"
            PRNAME      '_', "underbar"
            PRNAME      '-', "minus"
            PRNAME      '+', "plus"
            .byte       0

ps_nil:      .byte       "NIL", 0
ps_null:     .byte       "null", 0
ps_t:        .byte       "T", 0
ps_exit:     .byte       "exit", 0
ps_builtin:  .byte       "<builtin>", 0
ps_function: .byte       "<function>(", 0
ps_fn:       .byte       "fn ", 0
ps_fexpr:    .byte       "fexpr ", 0
ps_dots:     .byte       "...", 0
ps_error:    .byte       "Error: ", 0
ps_env:      .byte       "<env>", 0
ps_other:    .byte       "<?>", 0
ps_hash:     .byte       "<hash>", 0
.code
