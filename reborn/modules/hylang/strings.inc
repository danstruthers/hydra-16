; ****************************************************************************
; strings.inc - hylang's strings, names and lists (step 7.4a), in the module's fourth bank: danlang's str- functions
; (str-split, str-upper, str-lower, str-trim, str-replace, str-join, str-chars, str-pad-left, str-pad-right,
; str-repeat), the character tests (alpha? digit? space? upper? lower?), to-sym, to-atom and gensym, sort and subset.
; A string argument is copied whole to text (str_flat: 4096 bytes at most), a pattern (a separator, what str-replace
; replaces) to str_b (126 bytes at most), and a string made is written to cap_buf (cap_start ... cap_string), so a
; string of any length goes in and comes out.

.bss
s_kind:     .res        1                                   ; A string argument's kind (str_is's: 0, a character)
s_left:     .res        1                                   ; $80: str-pad-left (else str-pad-right)
s_c:        .res        1                                   ; A character (str-pad's), a test (the tests'), a length
s_lo:       .res        1                                   ; The arguments wanted: so many at least ...
s_hi:       .res        1                                   ;   and at most
s_n:        .res        2                                   ; A count
s_from:     .res        2                                   ; A piece's first byte (str-split's)
s_t:        .res        2                                   ; A value (sort's)
s_pn:       .res        1                                   ; str-split's separators: their bytes' length ...
s_pats:     .res        256                                 ;   and each its length, its bytes; then $FF
gensym_n:   .res        2                                   ; gensym's count
.segment "STRCODE"

; ---- Arguments, and what's made

; .A to .X arguments wanted (else ev_v the error, the text at r0).  OUT: C = 1, not so many
s_args:
            sta         s_lo
            stx         s_hi
            jsr         count_a
            cmp         s_lo
            bcc         @no
            cmp         s_hi
            beq         :+
            bcs         @no
:
            clc
            rts
@no:
            jmp         ev_error_text

; h_v, a string (or a character) argument: its bytes in text, fl_n1 of them, s_kind its kind (str_is's: 0, a
; character).  OUT: C = 1, it isn't one (ev_v an error)
s_str:
            jsr         str_is
            bcs         s_not_string
            sta         s_kind
            LDR         io_p, text
            jsr         str_flat
            sta         fl_n1
            stx         fl_n1 + 1
            rts
s_not_string:
            LDR         r0, ss_string
            jmp         ev_error_text

; h_v, a pattern argument (a string or a character, STR_MAX bytes at most): its bytes in str_b, str_bn of them.
; OUT: C = 1, it isn't one (ev_v an error)
s_pat:
            jsr         str_len
            bcs         s_not_string
            cpx         #0
            bne         s_pat_long
            cmp         #STR_MAX + 1
            bcs         s_pat_long
            LDR         io_p, str_b
            jsr         str_flat
            sta         str_bn
            rts
s_pat_long:
            LDR         r0, ss_pattern
            jmp         ev_error_text

; h_v, a number argument (a fixnum): .A/.X its value.  OUT: C = 1, it isn't one (ev_v an error)
s_int:
            lda         h_v
            lsr
            bcc         @not
            lda         h_v + 1
            cmp         #$80
            ror
            tax
            lda         h_v
            ror
            clc
            rts
@not:
            LDR         r0, ss_number
            jmp         ev_error_text

; h_v, a list argument: h_v = its chain.  OUT: C = 1, it isn't one (ev_v an error)
s_list_arg:
            jsr         type_of
            cmp         #TY_LIST
            bne         @not
            jsr         list_of
            clc
            rts
@not:
            LDR         r0, ss_list
            jmp         ev_error_text

; text's bytes, one by one: s_first, then s_next (.A, each; C = 1, past the last)
s_first:
            LDR         bi_q, text
            lda         fl_n1
            sta         s_n
            lda         fl_n1 + 1
            sta         s_n + 1
            rts
s_next:
            lda         s_n
            ora         s_n + 1
            sec
            beq         @done
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            lda         (bi_q)
            inc         bi_q
            bne         :+
            inc         bi_q + 1
:
            clc
@done:
            rts

; ev_v = the string caught (an error, if it's too long)
s_made:
            jsr         cap_string
            MOVR        ev_v, h_v
            rts

; ev_v = a string of the .A/.X bytes at r0
s_bytes:
            jsr         buf_string
            MOVR        ev_v, h_v
            rts

; ev_v = the character .A
s_char:
            asl
            sta         ev_v
            lda         #>CHARS
            adc         #0
            sta         ev_v + 1
            rts

; ev_v = a Q-expression of the chain h_v
s_qexpr:
            lda         #OT_QEXPR
            jsr         make_list
            bcs         :+
            MOVR        ev_v, h_v
            rts
:
            jmp         bi_no_room

; str_b's bytes in lower case
s_lower_b:
            ldx         str_bn
@byte:
            dex
            bmi         @done
            lda         str_b,x
            cmp         #'A'
            bcc         @byte
            cmp         #'Z' + 1
            bcs         @byte
            ora         #$20
            sta         str_b,x
            bra         @byte
@done:
            rts

; C = 1 if .A passes the test s_c (0 alpha?, 1 digit?, 2 space?, 3 upper?, 4 lower?)
s_is:
            ldx         s_c
            beq         @alpha
            dex
            beq         @digit
            dex
            beq         s_space
            dex
            beq         @upper
@lower:
            cmp         #'a'
            bcc         @no
            cmp         #'z' + 1
            bcc         @yes
@no:
            clc
            rts
@yes:
            sec
            rts
@upper:
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            bcc         @yes
            bra         @no
@digit:
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @yes
            bra         @no
@alpha:
            ora         #$20                                ; (A letter: its lower case one)
            bra         @lower

; C = 1 if .A is white space (a space, a tab, LF, VT, FF, CR)
s_space:
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

; ---- Strings

; (str-upper s), (str-lower s): s in upper case, or lower (a character stays one)
ss_upper:
            lda         #$80
            bra         s_case
ss_lower:
            lda         #0
s_case:
            sta         s_c
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         s_str
            bcs         @done
            jsr         cap_start
            jsr         s_first
@byte:
            jsr         s_next
            bcs         @end
            bit         s_c
            bpl         @lower
            cmp         #'a'
            bcc         @out
            cmp         #'z' + 1
            bcs         @out
            eor         #$20
            bra         @out
@lower:
            cmp         #'A'
            bcc         @out
            cmp         #'Z' + 1
            bcs         @out
            eor         #$20
@out:
            jsr         out_char
            bra         @byte
@end:
            lda         s_kind                              ; (A character: one again)
            jne         s_made
            stz         cap_on
            lda         cap_buf
            jmp         s_char
@done:
            rts

; (str-trim s): s without the white space at either end
ss_trim:
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            jcs         @done
            jsr         arg_next
            jsr         s_str
            jcs         @done
            stz         s_from                              ; (Its first byte that isn't white space ...
            stz         s_from + 1
@lead:
            lda         s_from
            cmp         fl_n1
            lda         s_from + 1
            sbc         fl_n1 + 1
            bcs         @empty
            clc
            lda         #<text
            adc         s_from
            sta         bi_q
            lda         #>text
            adc         s_from + 1
            sta         bi_q + 1
            lda         (bi_q)
            jsr         s_space
            bcc         @trail
            inc         s_from
            bne         @lead
            inc         s_from + 1
            bra         @lead
@trail:
            lda         s_kind                              ; (A character that isn't: itself)
            bne         :+
            lda         text
            jmp         s_char
:
            clc                                             ;   and its last)
            lda         #<(text - 1)
            adc         fl_n1
            sta         bi_q
            lda         #>(text - 1)
            adc         fl_n1 + 1
            sta         bi_q + 1
@back:
            lda         (bi_q)
            jsr         s_space
            bcc         @made
            lda         bi_q
            bne         :+
            dec         bi_q + 1
:
            dec         bi_q
            bra         @back
@made:
            clc                                             ; (The bytes between: bi_q + 1 - r0 of them)
            lda         #<text
            adc         s_from
            sta         r0
            lda         #>text
            adc         s_from + 1
            sta         r0 + 1
            sec
            lda         bi_q
            sbc         r0
            sta         s_n
            lda         bi_q + 1
            sbc         r0 + 1
            tax
            lda         s_n
            inc
            bne         :+
            inx
:
            jmp         s_bytes
@empty:
            LDR         r0, text
            lda         #0
            tax
            jmp         s_bytes
@done:
            rts

; (str-replace s old new): s with each old (not empty) new
ss_replace:
            LDR         r0, ss_replace_args
            lda         #3
            tax
            jsr         s_args
            jcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         arg_next
            jsr         s_pat                               ; (old, in str_b)
            jcs         @done
            lda         str_bn
            bne         :+
            LDR         r0, ss_replace_empty
            jmp         ev_error_text
:
            jsr         arg_next
            MOVR        ev_t, h_v                           ; (new)
            jsr         str_is
            jcs         s_not_string
            MOVR        h_v, ev_x
            jsr         s_str
            bcs         @done
            jsr         cap_start
            stz         io_n
            stz         io_n + 1
@at:
            lda         io_n                                ; (At its end?)
            cmp         fl_n1
            lda         io_n + 1
            sbc         fl_n1 + 1
            bcs         @end
            jsr         text_at
            bcc         @plain
            MOVR        h_v, ev_t                           ; (old: new instead)
            jsr         str_out
            clc
            lda         io_n
            adc         str_bn
            sta         io_n
            bcc         @at
            inc         io_n + 1
            bra         @at
@plain:
            clc
            lda         #<text
            adc         io_n
            sta         bi_q
            lda         #>text
            adc         io_n + 1
            sta         bi_q + 1
            lda         (bi_q)
            jsr         out_char
            inc         io_n
            bne         @at
            inc         io_n + 1
            bra         @at
@end:
            jmp         s_made
@done:
            rts

; (str-split s), (str-split s sep): the pieces of s between its separators, as .NET's Split has them (two
; separators together: an empty piece between them): sep's (a string, or a list of them, each tried in turn), or
; white space's (a space, a tab, LF, CR)
ss_split:
            LDR         r0, ss_split_args
            lda         #1
            ldx         #2
            jsr         s_args
            jcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v                           ; (s)
            stz         s_pn
            jsr         arg_next
            bcs         @white
            jsr         str_is                              ; (A string: the one separator)
            bcs         @list
            jsr         s_add_pat
            jcs         @done
            bra         @seps
@list:
            jsr         type_of                             ; (A list of strings)
            cmp         #TY_LIST
            bne         @not_seps
            jsr         list_of
@sep:
            lda         h_v + 1
            beq         @seps
            lda         h_v + 1
            pha
            lda         h_v
            pha
            jsr         car
            jsr         str_is
            bcs         @not_sep
            jsr         s_add_pat
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jcs         @done
            jsr         cdr
            bra         @sep
@not_sep:
            pla
            pla
@not_seps:
            LDR         r0, ss_split_seps
            jmp         ev_error_text
@seps:
            lda         s_pn                                ; (None, or each empty: white space's)
            bne         @split
@white:
            ldx         #ss_white_end - ss_white - 1
:
            lda         ss_white,x
            sta         s_pats,x
            dex
            bpl         :-
@split:
            MOVR        h_v, ev_x
            jsr         s_str
            jcs         @done
            LDR         ev_w, NIL                           ; (The pieces: ev_w, its last pair ev_r)
            LDR         ev_r, NIL
            stz         s_from
            stz         s_from + 1
            stz         io_n
            stz         io_n + 1
@at:
            lda         io_n                                ; (At its end?)
            cmp         fl_n1
            lda         io_n + 1
            sbc         fl_n1 + 1
            bcs         @last
            jsr         s_sep_at
            bcc         @next
            pha                                             ; (A separator: the piece before it)
            jsr         s_piece
            pla
            bcs         @room
            clc
            adc         io_n
            sta         io_n
            sta         s_from
            lda         io_n + 1
            adc         #0
            sta         io_n + 1
            sta         s_from + 1
            bra         @at
@next:
            inc         io_n
            bne         @at
            inc         io_n + 1
            bra         @at
@last:
            jsr         s_piece
            bcs         @room
            MOVR        h_v, ev_w
            jmp         s_qexpr
@room:
            jmp         bi_no_room
@done:
            rts

; The string h_v (a separator) added to s_pats (if it isn't empty).  OUT: C = 1, too long (ev_v an error)
s_add_pat:
            jsr         str_len
            cpx         #0
            jne         s_pat_long
            cmp         #0
            beq         @done
            sta         s_c
            sec                                             ; (Room: its length, its bytes, then $FF)
            adc         s_pn
            jcs         s_pat_long
            ldx         s_pn
            lda         s_c
            sta         s_pats,x
            sec
            txa
            adc         #<s_pats
            sta         io_p
            lda         #>s_pats
            adc         #0
            sta         io_p + 1
            jsr         str_flat
            lda         s_c
            sec
            adc         s_pn
            sta         s_pn
            tax
            lda         #$FF
            sta         s_pats,x
@done:
            clc
            rts

; C = 1 if one of s_pats is in text at io_n: .A its length
s_sep_at:
            clc
            lda         #<text
            adc         io_n
            sta         io_p
            lda         #>text
            adc         io_n + 1
            sta         io_p + 1
            sec                                             ; (The bytes from there on)
            lda         fl_n1
            sbc         io_n
            sta         s_n
            lda         fl_n1 + 1
            sbc         io_n + 1
            sta         s_n + 1
            ldx         #0
@pat:
            lda         s_pats,x
            cmp         #$FF
            beq         @none
            sta         s_c
            lda         s_n + 1                             ; (Room for it?)
            bne         :+
            lda         s_n
            cmp         s_c
            bcc         @skip
:
            phx
            inx
            ldy         #0
@byte:
            cpy         s_c
            beq         @match
            lda         (io_p),y
            cmp         s_pats,x
            bne         @differ
            iny
            inx
            bra         @byte
@differ:
            plx
@skip:
            txa
            sec
            adc         s_c
            tax
            bra         @pat
@match:
            plx
            lda         s_c
            sec
            rts
@none:
            clc
            rts

; The piece of text from s_from to io_n, a string, added to ev_w (its last pair ev_r).  OUT: C = 1, no room
s_piece:
            clc
            lda         #<text
            adc         s_from
            sta         r0
            lda         #>text
            adc         s_from + 1
            sta         r0 + 1
            sec
            lda         io_n
            sbc         s_from
            pha
            lda         io_n + 1
            sbc         s_from + 1
            tax
            pla
            jsr         buf_string
            bcs         @no
            MOVR        ev_v, h_v
            ldx         #ev_w - ev_x
            jmp         ev_append_x
@no:
            rts

; (str-join list), (str-join list sep): the items as print shows them, sep (a string) between them
ss_join:
            LDR         r0, ss_join_args
            lda         #1
            ldx         #2
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         s_list_arg
            bcs         @done
            MOVR        ev_w, h_v
            LDR         ev_t, NIL                           ; (sep: none)
            jsr         arg_next
            bcs         :+
            MOVR        ev_t, h_v
            jsr         str_is
            jcs         s_not_string
:
            jsr         cap_start
            lda         ev_w + 1
            beq         @end
@item:
            MOVR        h_v, ev_w
            jsr         car
            FARN        2, print_display
            MOVR        h_v, ev_w
            jsr         cdr
            MOVR        ev_w, h_v
            lda         ev_w + 1
            beq         @end
            lda         ev_t + 1
            beq         @item
            MOVR        h_v, ev_t
            jsr         str_out
            bra         @item
@end:
            jmp         s_made
@done:
            rts

; (str-chars s): s's characters, a list
ss_chars:
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         s_str
            bcs         @done
            LDR         ev_w, NIL
            LDR         ev_r, NIL
            jsr         s_first
@byte:
            jsr         s_next
            bcs         @end
            jsr         s_char
            ldx         #ev_w - ev_x
            jsr         ev_append_x
            bcc         @byte
            jmp         bi_no_room
@end:
            MOVR        h_v, ev_w
            jmp         s_qexpr
@done:
            rts

; (str-pad-left s n), (str-pad-left s n c), and str-pad-right's: s made n long, c (a space) before it, or after
ss_pad_left:
            lda         #$80
            bra         s_pad
ss_pad_right:
            lda         #0
s_pad:
            sta         s_left
            LDR         r0, ss_pad_args
            lda         #2
            ldx         #3
            jsr         s_args
            jcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         str_len
            jcs         s_not_string
            sta         s_n
            stx         s_n + 1
            jsr         arg_next
            jsr         s_int
            jcs         @done
            sec                                             ; (How many: n - its length, or none)
            sbc         s_n
            sta         s_n
            txa
            sbc         s_n + 1
            sta         s_n + 1
            bpl         :+
            stz         s_n
            stz         s_n + 1
:
            lda         #' '
            sta         s_c
            jsr         arg_next
            bcs         @make
            jsr         str_len
            jcs         s_not_string
            cpx         #0
            bne         @not_one
            cmp         #1
            bne         @not_one
            LDR         io_p, str_b
            jsr         str_flat
            lda         str_b
            sta         s_c
@make:
            jsr         cap_start
            bit         s_left
            bpl         @right
            jsr         @pad
            MOVR        h_v, ev_x
            jsr         str_out
            jmp         s_made
@right:
            MOVR        h_v, ev_x
            jsr         str_out
            jsr         @pad
            jmp         s_made
@pad:
            lda         s_n
            ora         s_n + 1
            beq         @done
            lda         s_c
            jsr         out_char
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @pad
@not_one:
            LDR         r0, ss_pad_one
            jmp         ev_error_text
@done:
            rts

; (str-repeat s n): s n times (n not negative)
ss_repeat:
            LDR         r0, ss_repeat_args
            lda         #2
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            MOVR        ev_x, h_v
            jsr         str_is
            jcs         s_not_string
            jsr         arg_next
            jsr         s_int
            bcs         @done
            cpx         #$80
            bcc         :+
            LDR         r0, ss_repeat_neg
            jmp         ev_error_text
:
            sta         s_n
            stx         s_n + 1
            jsr         cap_start
@again:
            lda         s_n
            ora         s_n + 1
            beq         @end
            lda         cap_n + 1                           ; (Too long already: no more)
            cmp         #>STR_LONG
            bcc         :+
            lda         cap_n
            bne         @end
:
            MOVR        h_v, ev_x
            jsr         str_out
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @again
@end:
            jmp         s_made
@done:
            rts

; (alpha? s), (digit? s), (space? s), (upper? s), (lower? s): T if s (a character, or a string, not empty) is one:
; each of its characters
ss_alpha_p:
            lda         #0
            bra         s_test
ss_digit_p:
            lda         #1
            bra         s_test
ss_space_p:
            lda         #2
            bra         s_test
ss_upper_p:
            lda         #3
            bra         s_test
ss_lower_p:
            lda         #4
s_test:
            sta         s_c
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         s_str
            bcs         @done
            lda         fl_n1
            ora         fl_n1 + 1
            beq         @no
            jsr         s_first
@byte:
            jsr         s_next
            bcs         @yes
            jsr         s_is
            bcs         @byte
@no:
            clc
            jmp         bi_bool
@yes:
            sec
            jmp         bi_bool
@done:
            rts

; ---- Names

; (to-sym x): a symbol: x itself; an atom's name, or a string's text (in lower case), as a symbol's
ss_to_sym:
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            bcs         @done
            jsr         arg_next
            jsr         type_of
            cmp         #TY_SYMBOL
            beq         @symbol
            cmp         #TY_ATOM
            beq         @atom
            jsr         str_is
            bcc         @string
            LDR         r0, ss_sym_type
            jmp         ev_error_text
@symbol:
            jsr         s_unref
            MOVR        ev_v, h_v
            rts
@atom:
            lda         #OT_SYMBOL
            jmp         s_rename
@string:
            lda         #OT_SYMBOL
            jmp         s_text_name
@done:
            rts

; (to-atom x): an atom: x itself; a symbol's name, a string's text (in lower case), or an integer's digits (a
; number's, truncated), as an atom's
ss_to_atom:
            LDR         r0, ss_one
            lda         #1
            tax
            jsr         s_args
            jcs         @done
            jsr         arg_next
            jsr         type_of
            cmp         #TY_ATOM
            beq         @atom
            cmp         #TY_SYMBOL
            beq         @symbol
            cmp         #TY_NUM
            beq         @number
            jsr         str_is
            bcc         @string
            LDR         r0, ss_atom_type
            jmp         ev_error_text
@atom:
            MOVR        ev_v, h_v
            rts
@symbol:
            jsr         s_unref
            lda         #OT_ATOM
            jmp         s_rename
@string:
            lda         #OT_ATOM
            jmp         s_text_name
@number:
            FARX        lib_num, num_trunc                  ; (Its digits, caught)
            jsr         cap_start
            FARN        2, print_display
            jsr         cap_end
            lda         fl_n2 + 1
            jne         s_long_name
            lda         fl_n2
            cmp         #SYM_MAX + 1
            jcs         s_long_name
            tay
            LDR         r0, cap_buf
            lda         #OT_ATOM
            jmp         s_intern
@done:
            rts

; h_v, a symbol (or a reference to one): h_v = the symbol
s_unref:
            jsr         deref
            lda         (h_p)
            cmp         #OT_SYMREF
            bne         :+
            ldy         #3
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
            sta         h_v
            stx         h_v + 1
:
            rts

; ev_v = the symbol or atom (.A = OT_SYMBOL, OT_ATOM) of h_v's name (an atom's, or a symbol's)
s_rename:
            pha
            jsr         deref
            ldy         #1
            lda         (h_p),y
            and         #$7F
            sta         str_bn
            tax
            ldy         #SYM_NAME
:
            dex
            bmi         :+
            lda         (h_p),y
            sta         str_b,y
            iny
            bra         :-
:
            LDR         r0, str_b + SYM_NAME
            ldy         str_bn
            pla
            bra         s_intern

; ev_v = the symbol or atom (.A = OT_SYMBOL, OT_ATOM) of h_v's text (a string's, in lower case)
s_text_name:
            pha
            jsr         str_len
            cpx         #0
            bne         @long
            cmp         #0
            beq         @empty
            cmp         #SYM_MAX + 1
            bcs         @long
            LDR         io_p, str_b
            jsr         str_flat
            sta         str_bn
            jsr         s_lower_b
            LDR         r0, str_b
            ldy         str_bn
            pla
            bra         s_intern
@empty:
            pla
            LDR         r0, ss_empty_name
            jmp         ev_error_text
@long:
            pla
s_long_name:
            LDR         r0, ss_long_name
            jmp         ev_error_text

; ev_v = the symbol or atom (.A = OT_SYMBOL, OT_ATOM) named by .Y bytes at r0
s_intern:
            jsr         intern
            jcs         bi_no_room
            MOVR        ev_v, h_v
            rts

; (gensym), (gensym prefix): a new symbol, prefix__n (g__n), n counting from 1
ss_gensym:
            LDR         r0, ss_gensym_args
            lda         #0
            ldx         #1
            jsr         s_args
            bcs         @done
            lda         #'g'
            sta         str_b
            lda         #1
            sta         str_bn
            jsr         arg_next
            bcs         @named
            jsr         s_pat
            bcs         @done
            jsr         s_lower_b
@named:
            inc         gensym_n
            bne         :+
            inc         gensym_n + 1
:
            lda         str_bn                              ; (Room for __ and the count)
            cmp         #SYM_MAX - 6
            jcs         s_long_name
            jsr         cap_start
            lda         gensym_n
            ldx         gensym_n + 1
            jsr         out_dec
            jsr         cap_end
            ldx         str_bn
            lda         #'_'
            sta         str_b,x
            inx
            sta         str_b,x
            inx
            ldy         #0
:
            lda         cap_buf,y
            sta         str_b,x
            inx
            iny
            cpy         fl_n2
            bne         :-
            txa
            tay
            LDR         r0, str_b
            lda         #OT_SYMBOL
            jmp         s_intern
@done:
            rts

; ---- Lists

; (subset list from), (subset list from n): the list's items from from on (n of them, at most); NIL if from is
; past its end
ss_subset:
            LDR         r0, ss_subset_args
            lda         #2
            ldx         #3
            jsr         s_args
            jcs         @done
            jsr         arg_next
            jsr         s_list_arg
            jcs         @done
            MOVR        ev_r, h_v
            jsr         arg_next
            jsr         s_int
            jcs         @done
            cpx         #$80                                ; (Before the first: the first)
            bcc         :+
            lda         #0
            tax
:
            sta         s_n
            stx         s_n + 1
@skip:
            lda         s_n
            ora         s_n + 1
            beq         @skipped
            lda         ev_r + 1
            jeq         @nil
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @skip
@skipped:
            lda         ev_r + 1
            jeq         @nil
            jsr         arg_next
            bcs         @rest
            jsr         s_int
            bcs         @done
            cpx         #$80
            bcs         @nil
            sta         s_n
            stx         s_n + 1
            LDR         ev_a, NIL                           ; (n of them: copied)
            LDR         ev_t, NIL
@take:
            lda         s_n
            ora         s_n + 1
            beq         @taken
            lda         ev_r + 1
            beq         @taken
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_v, h_v
            jsr         ev_append
            jcs         bi_no_room
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @take
@taken:
            MOVR        h_v, ev_a
            jmp         s_qexpr
@rest:
            MOVR        h_v, ev_r                           ; (The rest: as it is)
            jmp         s_qexpr
@nil:
            LDR         ev_v, NIL
@done:
            rts

; (sort list), (sort list less): the list's items in order, a stable merge sort: by cmp's order, or by less, a
; function of two items that's T when the first goes before the second (an error from it: the value)
ss_sort:
            LDR         r0, ss_sort_args
            lda         #1
            ldx         #2
            jsr         s_args
            jcs         @done
            jsr         arg_next
            jsr         s_list_arg
            jcs         @done
            MOVR        ev_r, h_v                           ; (The items)
            LDR         ev_f, NIL                           ; (less: none)
            jsr         arg_next
            bcs         @runs
            MOVR        ev_f, h_v
            jsr         type_of
            cmp         #TY_FUNC
            beq         @runs
            LDR         r0, ss_sort_fn
            jmp         ev_error_text
@runs:
            LDR         ev_a, NIL                           ; ---- Each item a run of its own (ev_a, its last
            LDR         ev_t, NIL                           ; ev_t)
@run:
            lda         ev_r + 1
            beq         @pass
            MOVR        h_v, ev_r
            jsr         car
            MOVR        h_a, h_v
            LDR         h_d, NIL
            jsr         cons
            jcs         bi_no_room
            MOVR        ev_v, h_v
            jsr         ev_append
            jcs         bi_no_room
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
            bra         @run
@pass:
            lda         ev_a + 1                            ; ---- The runs merged in twos, till there's one
            jeq         @empty
            MOVR        h_v, ev_a
            jsr         cdr
            lda         h_v + 1
            jeq         @sorted
            MOVR        ev_s, ev_a                          ; (ev_s: the runs; ev_a, ev_t: those merged)
            LDR         ev_a, NIL
            LDR         ev_t, NIL
@two:
            lda         ev_s + 1
            beq         @pass
            MOVR        h_v, ev_s
            jsr         car
            MOVR        ev_w, h_v
            MOVR        h_v, ev_s
            jsr         cdr
            MOVR        ev_s, h_v
            LDR         ev_r, NIL
            lda         ev_s + 1
            beq         :+
            MOVR        h_v, ev_s
            jsr         car
            MOVR        ev_r, h_v
            MOVR        h_v, ev_s
            jsr         cdr
            MOVR        ev_s, h_v
:
            jsr         s_merge
            bcs         @done
            MOVR        ev_v, ev_x
            jsr         ev_append
            jcs         bi_no_room
            bra         @two
@sorted:
            MOVR        h_v, ev_a
            jsr         car
            jmp         s_qexpr
@empty:
            LDR         ev_v, NIL
@done:
            rts

; The runs ev_w and ev_r merged: ev_x (its last pair ev_e).  OUT: C = 1, an error (ev_v)
s_merge:
            LDR         ev_x, NIL
            LDR         ev_e, NIL
@step:
            lda         ev_w + 1
            beq         @rest_r
            lda         ev_r + 1
            beq         @rest_w
            jsr         s_less
            bmi         @error
            bne         @take_r
            MOVR        h_v, ev_w                           ; (ev_w's first)
            jsr         car
            MOVR        ev_v, h_v
            MOVR        h_v, ev_w
            jsr         cdr
            MOVR        ev_w, h_v
            bra         @add
@take_r:
            MOVR        h_v, ev_r                           ; (ev_r's first)
            jsr         car
            MOVR        ev_v, h_v
            MOVR        h_v, ev_r
            jsr         cdr
            MOVR        ev_r, h_v
@add:
            ldx         #0
            jsr         ev_append_x
            bcc         @step
            jsr         bi_no_room
@error:
            sec
            rts
@rest_w:
            MOVR        h_a, ev_w                           ; (The rest of one, after)
            bra         @rest
@rest_r:
            MOVR        h_a, ev_r
@rest:
            lda         ev_x + 1
            bne         :+
            MOVR        ev_x, h_a
            clc
            rts
:
            MOVR        h_v, ev_e
            jsr         set_cdr
            clc
            rts

; Whether ev_r's first goes before ev_w's: .A = 1, it does; 0, it doesn't; $FF, an error (ev_v)
s_less:
            lda         ev_f + 1
            bne         @fn
            PUSHV       ev_x                                ; ---- By compare's order (the second bank's)
            PUSHV       ev_w
            MOVR        h_v, ev_w
            jsr         car
            MOVR        s_t, h_v
            MOVR        h_v, ev_r
            jsr         car
            MOVR        ev_x, h_v
            MOVR        h_v, s_t
            lda         #8
            sta         ev_c
            FARN        2, compare
            sta         s_c
            POPV        ev_w
            POPV        ev_x
            lda         s_c
            cmp         #$FF
            beq         @yes
            lda         #0
            rts
@yes:
            lda         #1
            rts
@fn:
            PUSHV       ev_x                                ; ---- By less (everything kept: the evaluator
            PUSHV       ev_e                                ; changes the registers)
            PUSHV       ev_f
            PUSHV       ev_a
            PUSHV       ev_t
            PUSHV       ev_w
            PUSHV       ev_r
            PUSHV       ev_s
            MOVR        h_v, ev_w                           ; (Its arguments: ev_r's first, ev_w's)
            jsr         car
            MOVR        h_a, h_v
            LDR         h_d, NIL
            jsr         cons
            bcs         @room
            MOVR        ev_a, h_v
            MOVR        h_v, ev_r
            jsr         car
            MOVR        h_a, h_v
            MOVR        h_d, ev_a
            jsr         cons
            bcs         @room
            MOVR        ev_a, h_v
            jsr         apply_nested
            stz         s_c
            jsr         ev_is_error
            bcc         :+
            dec         s_c
            bra         @back
:
            lda         ev_v
            ora         ev_v + 1
            beq         @back
            inc         s_c
            bra         @back
@room:
            MOVR        ev_v, err_room
            lda         #$FF
            sta         s_c
@back:
            MOVR        s_t, ev_v
            POPV        ev_s
            POPV        ev_r
            POPV        ev_w
            POPV        ev_t
            POPV        ev_a
            POPV        ev_f
            POPV        ev_e
            POPV        ev_x
            MOVR        ev_v, s_t
            lda         s_c
            rts

.segment "STRDATA"
ss_white:   .byte       1, ' ', 1, TAB, 1, LF, 1, CR, $FF   ; (str-split's white space)
ss_white_end:
ss_one:     .byte       "Expects 1 parameter", 0
ss_string:  .byte       "Expects a String", 0
ss_number:  .byte       "Expects a Number", 0
ss_list:    .byte       "Expects a QExpr", 0
ss_pattern: .byte       "A string too long here (126 bytes at most)", 0
ss_replace_args: .byte  "'str-replace' expects 3 Strings", 0
ss_replace_empty: .byte "'str-replace' cannot replace an empty String", 0
ss_split_args: .byte    "'str-split' expects a String and maybe separators", 0
ss_split_seps: .byte    "Second 'str-split' parameter must be a list of strings or a single string", 0
ss_join_args: .byte     "'str-join' expects a list and maybe a separator", 0
ss_pad_args: .byte      "'str-pad' expects a String, a length and maybe a character", 0
ss_pad_one: .byte       "'str-pad' pads with one character", 0
ss_repeat_args: .byte   "'str-repeat' expects a String and a Number", 0
ss_repeat_neg: .byte    "'str-repeat' count cannot be negative", 0
ss_sym_type: .byte      "Only atoms and strings can be converted into symbols", 0
ss_atom_type: .byte      "Only symbols, strings and numbers can be converted to atoms", 0
ss_long_name: .byte     "A name too long", 0
ss_empty_name: .byte    "A name can't be empty", 0
ss_gensym_args: .byte   "'gensym' expects maybe a prefix", 0
ss_subset_args: .byte   "'subset' expects a list, an index and maybe a count", 0
ss_sort_args: .byte     "'sort' expects a list and maybe a function", 0
ss_sort_fn: .byte       "Second 'sort' parameter must be a function", 0
.segment "STRCODE"
