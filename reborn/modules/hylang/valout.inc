; ****************************************************************************
; valout.inc - hylang's built-ins that write values or make strings of them (phase 3), in its second bank with the
; printer: print, write, repr, format, + of strings, to-str, substring, to-sym, to-atom, gensym (phase 4).  A string
; is made by capturing what's written (out.inc), as danlang's StringBuilder.  Each gets its values at argp (cn of
; them) and gives back ex (C = 1 if there's no room).

.segment "CODE2"

; (print x...): the values as print shows them, a space between, then a line's end; NIL
bo_print:
            lda         #' '
            sta         o_sep
            jsr         o_values
            lda         #LF
            jsr         out_byte
            jmp         o_nil
; (write x...): the values as print shows them; NIL
bo_write:
            stz         o_sep
            jsr         o_values
o_nil:
            stz         ex
            stz         ex + 1
            clc
            rts

; The values at argp, each as print shows it, o_sep (unless it's 0) between them
o_values:
            stz         o_i
            stz         o_i + 1
@value:
            lda         o_i
            cmp         cn
            lda         o_i + 1
            sbc         cn + 1
            bcs         @done
            lda         o_i
            ora         o_i + 1
            beq         :+
            lda         o_sep
            beq         :+
            jsr         out_byte
:
            jsr         o_arg
            ldy         #1
            jsr         print_val
            inc         o_i
            bne         @value
            inc         o_i + 1
            bra         @value
@done:
            rts

; OUT: .A, .X = the argument o_i
o_arg:
            lda         o_i
            asl
            sta         hq
            lda         o_i + 1
            rol
            sta         hq + 1
            clc
            lda         hq
            adc         argp
            sta         hq
            lda         hq + 1
            adc         argp + 1
            sta         hq + 1
            ldy         #1
            lda         (hq),y
            tax
            lda         (hq)
            rts

; (repr x): x as the REPL shows it, a string
bo_repr:
            jsr         o_begin
            jsr         l_arg0
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         print_val
            jmp         o_end

; ex as print shows it, a string: ex.  (error's message)
bo_display:
            jsr         o_begin
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         print_val
            jmp         o_end

; (+ s x...), its first a string or a character: the values as print shows them, joined
bo_join:
            jsr         o_begin
            stz         o_sep
            jsr         o_values
            jmp         o_end

; A string begun (the output captured) ...
o_begin:
            jsr         cap_begin
            sta         o_start
            stx         o_start + 1
            rts
;   and made: ex.  OUT: C = 1 if there's no room
o_end:
            lda         o_start
            ldx         o_start + 1
            jsr         cap_end
            bcs         @rts
            MOVW        ex, hv
@rts:
            rts
;   or given up, for the error .A, .X (a text in this bank)
o_fail:
            pha
            phx
            lda         o_start
            ldx         o_start + 1
            jsr         cap_drop
            plx
            pla
            jmp         l_error

; (to-str x): x as print shows it, a string (a number's base: phase 5)
bo_to_str:
            jsr         l_arg0
            lda         cn
            cmp         #2
            bcc         bo_display
            FARN        3, n_to_str2                        ; (A number in a base: numbi.inc's)
            rts

; (format text x...): the text with each {} the next value, as print shows it ({{ and }}: a brace)
bo_format:
            jsr         l_arg0                              ; (Its text: a string, or a character)
            jsr         o_text
            bcc         :+
            LDAX        s_oformat
            jmp         l_error
:
            jsr         o_begin
            lda         #1                                  ; (The next value: argument 1)
            sta         o_i
            stz         o_i + 1
            stz         o_at
            stz         o_at + 1
@byte:
            jsr         o_byte_at
            bcc         @lb200
            jmp         @done
@lb200:
            cmp         #'{'
            beq         @open
            cmp         #'}'
            bne         @lb201
            jmp         @close
@lb201:
@out:
            jsr         out_byte
            jmp         @next
@open:
            jsr         o_peek                              ; ({{: a brace; {}: the next value; {base}: the next,
            cmp         #'{'                                ;   a number, in that base; a { with no } after it:
            beq         @twice                              ;   itself)
            cmp         #'}'
            beq         @value
            jsr         o_spec
            bcs         @brace
            lda         o_i
            cmp         cn
            lda         o_i + 1
            sbc         cn + 1
            bcc         :+
            LDAX        s_ofew
            jmp         o_fail
:
            jsr         o_arg
            FARN        3, n_fmt_base
            bcc         :+
            lda         o_start                             ; (Its error: ex)
            ldx         o_start + 1
            jsr         cap_drop
            clc
            rts
:
            inc         o_i
            bne         @next
            inc         o_i + 1
            bra         @next
@value:
            lda         o_i
            cmp         cn
            lda         o_i + 1
            sbc         cn + 1
            bcc         :+
            LDAX        s_ofew
            jmp         o_fail
:
            jsr         o_arg
            ldy         #1
            jsr         print_val
            inc         o_i
            bne         :+
            inc         o_i + 1
:
            jsr         o_skip
            bra         @next
@close:
            jsr         o_peek                              ; (}}: a brace)
            cmp         #'}'
            bne         @brace
@twice:
            jsr         o_skip
@brace:
            jsr         o_byte_at
            jmp         @out
@next:
            jsr         o_skip
            jmp         @byte
@done:
            lda         o_i                                 ; (Every value used?)
            cmp         cn
            bne         :+
            lda         o_i + 1
            cmp         cn + 1
            beq         :++
:
            LDAX        s_omany
            jmp         o_fail
:
            jmp         o_end

; ex, a string or a character: o_text it (a character: its byte in o_char).  OUT: C = 1 if it's neither
o_text:
            MOVW        o_str, ex
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         @string
            lda         ex + 1                              ; (A character)
            cmp         #>CHAR0
            bcc         @no
            cmp         #>BUILTIN0
            bcs         @no
            lsr
            lda         ex
            ror
            sta         o_char
            lda         #1
            sta         o_len
            stz         o_len + 1
            stz         o_str + 1                           ; (o_str's high byte 0: a character)
            clc
            rts
@string:
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            sta         o_len
            stx         o_len + 1
            clc
            rts
@no:
            sec
            rts

; OUT: C = 1 at o_text's end; else .A = its byte o_at
o_byte_at:
            lda         o_at
            cmp         o_len
            lda         o_at + 1
            sbc         o_len + 1
            bcs         @rts
            lda         o_str + 1
            bne         :+
            lda         o_char
            clc
            rts
:
            lda         o_str
            ldx         o_str + 1
            jsr         q_strat
            lda         hp
            clc
            adc         o_at
            sta         hp
            lda         hp + 1
            adc         o_at + 1
            sta         hp + 1
            lda         (hp)
            clc
@rts:
            rts

; OUT: .A = the byte after o_at (0 at the end)
o_peek:
            inc         o_at
            bne         :+
            inc         o_at + 1
:
            jsr         o_byte_at
            php
            pha
            lda         o_at
            bne         :+
            dec         o_at + 1
:
            dec         o_at
            pla
            plp
            bcc         :+
            lda         #0
:
            rts

; A } after the { at o_at?  OUT: C = 0, o_at at it, and NL_S the base between (zero-terminated; NL_BASE_MAX - 1
; bytes at most: past them, ".", none); C = 1, o_at as it was, if there's none
o_spec:
            lda         o_at
            pha
            lda         o_at + 1
            pha
            ldx         #0
@byte:
            jsr         o_skip
            phx
            jsr         o_byte_at
            plx
            bcs         @none
            cmp         #'}'
            beq         @found
            cpx         #NL_BASE_MAX - 1
            bcs         :+
            sta         NL_S,x
            inx
            bra         @byte
:
            ldx         #NL_BASE_MAX                        ; (Too long: no base)
            bra         @byte
@found:
            cpx         #NL_BASE_MAX
            bcc         :+
            lda         #'.'
            sta         NL_S
            ldx         #1
:
            stz         NL_S,x
            pla
            pla
            clc
            rts
@none:
            pla
            sta         o_at + 1
            pla
            sta         o_at
            sec
            rts

; o_at on by one
o_skip:
            inc         o_at
            bne         :+
            inc         o_at + 1
:
            rts

; (substring s i [n]): s from index i (its length too: ""), n bytes of it (as many as there are), or the rest
bo_substring:
            LDAX        n_substring
            jsr         l_set_fn
            jsr         l_arg0
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_osubstr
            jmp         l_error
:
            MOVW        o_str, ex
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            sta         o_len
            stx         o_len + 1
            jsr         l_arg1                              ; (Its index: within it, or its length)
            LDAX        s_lindex
            jsr         l_whole
            bcc         :+
            clc
            rts
:
            lda         l_n + 1
            bpl         @lb27
            jmp         @outside
@lb27:
            lda         o_len
            cmp         l_n
            lda         o_len + 1
            sbc         l_n + 1
            bcc         @outside
            MOVW        o_at, l_n
            lda         o_len                               ; (The rest: its length less the index)
            sec
            sbc         o_at
            sta         o_n
            lda         o_len + 1
            sbc         o_at + 1
            sta         o_n + 1
            lda         cn
            cmp         #3
            bcc         @copy
            jsr         l_arg2                              ; (A count: no more than the rest)
            LDAX        s_lcount
            jsr         l_whole
            bcc         :+
            clc
            rts
:
            lda         l_n + 1
            bpl         :+
            LDAX        s_osubneg
            jmp         l_error
:
            lda         l_n
            cmp         o_n
            lda         l_n + 1
            sbc         o_n + 1
            bcs         @copy
            MOVW        o_n, l_n
@copy:
            jsr         o_begin
@byte:
            lda         o_n
            ora         o_n + 1
            beq         @made
            jsr         o_byte_at
            jsr         out_byte
            jsr         o_skip
            lda         o_n
            bne         :+
            dec         o_n + 1
:
            dec         o_n
            bra         @byte
@made:
            jmp         o_end
@outside:
            jsr         l_begin                             ; ("'substring': index 5 is outside the string")
            LDAX        s_osubout
            jsr         out_text
            jsr         l_arg1
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         print_val
            LDAX        s_osubout2
            jsr         out_text
            jmp         l_end

; (to-sym x): a symbol: x itself; an atom's name, or a string's (in lower case), as a symbol
bo_to_sym:
            jsr         l_arg0
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_SYMBOL
            bne         :+
            clc
            rts
:
            cmp         #PK_ATOM
            beq         @atom
            cmp         #PK_STRING
            beq         @string
            LDAX        s_otosym
            jmp         l_error
@atom:
            lda         ex                                  ; (Its name: T, NIL and exit are theirs, as the reader's)
            ldx         ex + 1
            lda         #1
            sta         q_isname
            lda         ex
            jsr         q_strat
            stz         q_isname
            jsr         o_name
            bcs         @long
            jsr         o_special
            bcc         @done
            bra         @intern
@string:
            lda         ex
            ldx         ex + 1
            jsr         q_strat
            jsr         o_name
            bcs         @long
@intern:
            LDHQ        scratch, 0
            lda         o_len
            sta         hn
            lda         #PK_SYMBOL
            jsr         intern
            bcs         @rts
            MOVW        ex, hv
@done:
            clc
@rts:
            rts
@long:
            LDAX        s_otoolong
            jmp         l_error

; (to-atom x): an atom: of an atom, a symbol, a string or a number (its text as print shows it, in lower case)
bo_to_atom:
            jsr         l_arg0
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_ATOM
            bne         :+
            clc
            rts
:
            cmp         #PK_SYMBOL
            beq         @text
            cmp         #PK_STRING
            beq         @text
            cmp         #PK_FIXNUM
            beq         @text
            cmp         #PK_NUMBER
            beq         @text
@not:
            LDAX        s_otoatom
            jmp         l_error
@text:
            jsr         cap_begin
            sta         o_start
            stx         o_start + 1
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         print_val
            lda         #PK_ATOM
            bra         o_intern

; (gensym [prefix]): a symbol no other has: g__1, g__2 ... (prefix__1 ...: the count is hylang's)
bo_gensym:
            lda         cn
            beq         :+
            jsr         l_arg0                              ; (The prefix: a string or a character)
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            lda         ex + 1
            and         #$FE
            cmp         #>CHAR0
            beq         :+
            LDAX        s_ogensym
            jmp         l_error
:
            inc         o_gensym
            bne         :+
            inc         o_gensym + 1
:
            jsr         cap_begin
            sta         o_start
            stx         o_start + 1
            lda         cn
            bne         :+
            lda         #'g'
            jsr         out_byte
            bra         @count
:
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         print_val
@count:
            lda         #'_'
            jsr         out_byte
            jsr         out_byte
            lda         o_gensym
            ldx         o_gensym + 1
            jsr         out_dec
            lda         #PK_SYMBOL
; The capture begun at o_start ended, and its text (in lower case) interned: ex = the symbol or the atom (.A, its
; kind).  OUT: C = 1 if there's no room
o_intern:
            sta         o_kind
            lda         o_start
            ldx         o_start + 1
            jsr         cap_end
            bcs         @rts
            MOVW        ex, hv
            lda         hv
            ldx         hv + 1
            jsr         kind_of
            cmp         #PK_ERROR
            beq         @done                               ; ("Too long")
            lda         ex
            ldx         ex + 1
            jsr         q_strat
            jsr         o_name
            bcs         @long
            LDHQ        scratch, 0
            lda         o_len
            sta         hn
            lda         o_kind
            jsr         intern
            bcs         @rts
            MOVW        ex, hv
@done:
            clc
@rts:
            rts
@long:
            LDAX        s_otoolong
            jmp         l_error

; hp, hn: a name's bytes: copied to scratch, in lower case; o_len its length.  OUT: C = 1 if it's past 255 bytes
o_name:
            lda         hn + 1
            bne         @long
            lda         hn
            sta         o_len
            ldy         #0
@byte:
            cpy         o_len
            beq         @done
            lda         (hp),y
            cmp         #'A'
            bcc         :+
            cmp         #'Z' + 1
            bcs         :+
            ora         #$20
:
            sta         scratch,y
            iny
            bra         @byte
@done:
            clc
            rts
@long:
            sec
            rts

; scratch (o_len bytes): t, nil or exit?  OUT: C = 0 and ex = T, NIL or exit if it's one; C = 1 if not
o_special:
            lda         o_len
            cmp         #1
            bne         @nil
            lda         scratch
            cmp         #'t'
            bne         @no
            lda         #<T_VAL
            bra         @imm
@nil:
            cmp         #3
            bne         @exit
            ldx         #2
:
            lda         scratch,x
            cmp         o_nilname,x
            bne         @no
            dex
            bpl         :-
            lda         #<NIL
            bra         @imm
@exit:
            cmp         #4
            bne         @no
            ldx         #3
:
            lda         scratch,x
            cmp         s_exit,x
            bne         @no
            dex
            bpl         :-
            lda         #<EXIT_VAL
@imm:
            sta         ex
            stz         ex + 1
            clc
            rts
@no:
            sec
            rts

.bss
o_sep:      .res        1                                   ; (print's separator: a space, or none)
o_i:        .res        2                                   ; (The argument at)
o_start:    .res        2                                   ; (The capture's start)
o_str:      .res        2                                   ; (A text's string, its length, the byte at, a count;
o_len:      .res        2                                   ;   a character's byte)
o_at:       .res        2
o_n:        .res        2
o_char:     .res        1
o_kind:     .res        1                                   ; (o_intern's: a symbol's or an atom's)

PSTATE_SEG
o_gensym:   .res        2                                   ; gensym's count
.segment "RODATA2"
n_substring: .byte      "substring", 0
o_nilname:  .byte       "nil"
s_oformat:  .byte       "'format' expects a String", 0
s_ofew:     .byte       "'format' has too few arguments for its {}s", 0
s_omany:    .byte       "'format' has more arguments than {}s", 0
s_osubstr:  .byte       "First 'substring' parameter must be a String", 0
s_osubneg:  .byte       "'substring': a count can't be negative", 0
s_osubout:  .byte       "'substring': index ", 0
s_osubout2: .byte       " is outside the string", 0
s_otosym:   .byte       "Only atoms and strings can be converted into symbols", 0
s_otoolong: .byte       "Too long: a name of more than 255 bytes", 0
s_otoatom:  .byte       "Only symbols, strings and numbers can be converted to atoms", 0
s_ogensym:  .byte       "'gensym' expects a String", 0
