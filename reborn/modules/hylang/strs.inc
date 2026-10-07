; ****************************************************************************
; strs.inc - hylang's strings and characters (phase 6), in its fourth bank, as danlang's (Builtins.cs): char-at,
; str-split, str-upper, str-lower, str-trim, str-replace, str-join, str-chars, str-pad-left, str-pad-right,
; str-repeat, char-code, code-char, and the character tests.  A string argument may be a character (a string of one,
; as StrArg has it).  Two arguments' bytes are had at once, each in a slot (s_desc): its address in the window, its
; length, its bank, set for each byte read (s_byte), as they may be in two banks; a slot is set again after anything
; that may collect (blobs move).  What's made is captured output.  Each built-in sets the hashes' abort point
; (h_enter): no room goes back from there.

.bss
s_ptr:      .res        4                                   ; The slots: each one's bytes (in the window), length,
s_len:      .res        4                                   ;   bank; a character's byte
s_bank:     .res        2
s_ch:       .res        2
s_i:        .res        2                                   ; (Places: s_byte's, s_bytej's and s_bytek's, in turn)
s_ij:       .res        2
s_j:        .res        2
s_n:        .res        2                                   ; (A count; a pad's)
s_m:        .res        2
s_from:     .res        2                                   ; (str-split's piece: where it starts)
s_c:        .res        1                                   ; (A built-in's own: what it does; a byte compared; the
s_c2:       .res        1                                   ;   pad's byte; str-split's separators: S_*)
s_pc:       .res        1
s_mode:     .res        1
s_list:     .res        2                                   ; (A list walked)
s_w:        .res        2
s_slot:     .res        1                                   ; (s_desc's)
s_dv:       .res        2

S_FOUR          = 0             ; (str-split's separators: space, tab, LF and CR; white space; one string; a list)
S_WHITE         = 1
S_ONE           = 2
S_LIST          = 3

.segment "CODE4"

; ---- Arguments and slots

; Slot .Y (0, 1) = the value .A, .X, a string or a character.  OUT: C = 1 if it's neither
s_desc:
            sta         s_dv
            stx         s_dv + 1
            sty         s_slot
            jsr         kind_of
            cmp         #PK_STRING
            beq         @string
            lda         s_dv                                ; (A character: its byte, in RAM)
            lsr
            bcs         @no
            lda         s_dv + 1
            and         #$FE
            cmp         #>CHAR0
            bne         @no
            lda         s_dv + 1
            lsr
            lda         s_dv
            ror
            ldx         s_slot
            sta         s_ch,x
            txa
            asl
            tay
            txa
            clc
            adc         #<s_ch
            sta         s_ptr,y
            lda         #>s_ch
            adc         #0
            sta         s_ptr + 1,y
            lda         #1
            sta         s_len,y
            lda         #0
            sta         s_len + 1,y
            lda         $00
            sta         s_bank,x
            clc
            rts
@string:
            lda         s_dv                                ; (A string: its length, its blob's bytes)
            ldx         s_dv + 1
            jsr         deref
            lda         s_slot
            asl
            tay
            lda         (hp)
            sta         s_len,y
            phy
            ldy         #1
            lda         (hp),y
            ply
            sta         s_len + 1,y
            phy
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            pla
            jsr         blob_data
            ply
            lda         hp
            sta         s_ptr,y
            lda         hp + 1
            sta         s_ptr + 1,y
            ldx         s_slot
            lda         $00
            sta         s_bank,x
            clc
            rts
@no:
            sec
            rts

; Slot .X = the argument at .Y (its place: 0, 2 ...), a string or a character.  OUT: C = 1 and ex = the error "'fn'
; expects a String" if it isn't
s_arg:
            phx
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            ply
            jsr         s_desc
            bcc         @rts
            jsr         h_begin_msg
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            LDAX        s_sexpects
            jsr         out_text
            jsr         h_end_msg
            sec
@rts:
            rts

; Slot 0 = the argument 0
s_subject:
            ldx         #0
            ldy         #0
            jmp         s_arg

; s_n = the argument at .Y, a whole number for 'fn' as what (.A, .X: "an index", "a number"), past 16 bits -32768 or
; 32767; ex = the argument.  OUT: C = 1 and ex = the error "'fn': what must be a whole number, not x" if it isn't one
s_whole:
            sta         s_w
            stx         s_w + 1
            lda         (argp),y
            sta         ex
            iny
            lda         (argp),y
            sta         ex + 1
            lda         ex
            ldx         ex + 1
            FARN        3, n_whole
            bcs         @not
            sta         s_n
            stx         s_n + 1
            rts
@not:
            jsr         h_begin_msg
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            LDAX        s_swhole1
            jsr         out_text
            lda         s_w
            ldx         s_w + 1
            jsr         out_text
            LDAX        s_swhole2
            jsr         out_text
            lda         ex
            ldx         ex + 1
            jsr         h_repr
            jsr         h_end_msg
            sec
            rts

; The number s_whole gave out (past 16 bits: its argument, ex, as it prints)
s_outn:
            lda         s_n
            ldx         s_n + 1
            cmp         #$FF
            bne         :+
            cpx         #$7F
            beq         @arg
:
            cmp         #0
            bne         @dec
            cpx         #$80
            beq         @arg
@dec:
            jmp         out_dec
@arg:
            lda         ex
            ldx         ex + 1
            jmp         h_repr

; A capture (h_begin_msg's) ended: ex the string made
s_end:
            lda         h_start
            ldx         h_start + 1
            jsr         cap_end
            bcc         :+
            jmp         h_nomem
:
            MOVW        ex, hv
            clc
            rts

; ex = the error "'fn' message" (the built-in's name in quotes, then the message at .A, .X); C = 0
s_error_fn:
            pha
            phx
            jsr         h_begin_msg
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            lda         #$27
            jsr         out_byte
            plx
            pla
            jsr         out_text
            jmp         h_end_msg

; .A = slot .X's byte s_i (s_byte), s_ij (s_bytej) or s_j (s_bytek), its bank set in the window.  Keeps .X
s_byte:
            ldy         #0
            bra         s_at
s_bytej:
            ldy         #2
            bra         s_at
s_bytek:
            ldy         #4
s_at:
            lda         s_bank,x
            sta         $00
            phx
            txa
            asl
            tax
            lda         s_ptr,x
            clc
            adc         s_i,y
            sta         nd
            lda         s_ptr + 1,x
            adc         s_i + 1,y
            sta         nd + 1
            plx
            lda         (nd)
            rts

; Is slot 1's string at slot 0's byte s_i?  OUT: C = 1 if it is
s_match:
            clc                                             ; (Room for it: s_i + its length, to slot 0's)
            lda         s_i
            adc         s_len + 2
            sta         s_ij
            lda         s_i + 1
            adc         s_len + 3
            sta         s_ij + 1
            lda         s_len
            cmp         s_ij
            lda         s_len + 1
            sbc         s_ij + 1
            bcc         @no
            stz         s_j
            stz         s_j + 1
@byte:
            lda         s_j
            cmp         s_len + 2
            bne         :+
            lda         s_j + 1
            cmp         s_len + 3
            beq         @yes
:
            ldx         #1                                  ; (Its byte s_j, and slot 0's s_i + s_j)
            jsr         s_bytek
            sta         s_c2
            clc
            lda         s_i
            adc         s_j
            sta         s_ij
            lda         s_i + 1
            adc         s_j + 1
            sta         s_ij + 1
            ldx         #0
            jsr         s_bytej
            cmp         s_c2
            bne         @no
            inc         s_j
            bne         @byte
            inc         s_j + 1
            bra         @byte
@yes:
            sec
            rts
@no:
            clc
            rts

; Slot 0's bytes from s_i, s_n of them, out (s_i past them)
s_out:
            ldx         #0
@byte:
            lda         s_n
            ora         s_n + 1
            beq         @rts
            jsr         s_byte
            jsr         out_byte
            inc         s_i
            bne         :+
            inc         s_i + 1
:
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @byte
@rts:
            rts

; Slot .X's bytes, all of them, out
s_outall:
            stz         s_i
            stz         s_i + 1
@byte:
            txa
            asl
            tay
            lda         s_i
            cmp         s_len,y
            bne         :+
            lda         s_i + 1
            cmp         s_len + 1,y
            beq         @rts
:
            jsr         s_byte
            jsr         out_byte
            inc         s_i
            bne         @byte
            inc         s_i + 1
            bra         @byte
@rts:
            rts

; ex = the character .A; C = 0
s_char:
            stz         ex + 1
            asl
            sta         ex
            rol         ex + 1
            lda         ex + 1
            ora         #>CHAR0
            sta         ex + 1
s_done:
            clc
            rts

; ---- The built-ins

; (char-at s i): the character at i (from 0)
bi_char_at:
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcs         s_done
            LDAX        s_sindex
            ldy         #2
            jsr         s_whole
            bcs         s_done
            lda         s_n + 1                             ; (0 to its length less 1)
            bmi         @outside
            lda         s_n
            cmp         s_len
            lda         s_n + 1
            sbc         s_len + 1
            bcs         @outside
            MOVW        s_i, s_n
            ldx         #0
            jsr         s_byte
            bra         s_char
@outside:
            jsr         h_begin_msg                         ; ("'char-at': index 5 is outside the string")
            LDAX        s_scharat
            jsr         out_text
            jsr         s_outn
            LDAX        s_soutside
            jsr         out_text
            jsr         h_end_msg
            bra         s_done

; (str-upper s), (str-lower s): ASCII's letters changed; (str-trim s): without the spaces at its ends (space, tab, LF,
; VT, FF, CR).  A character stays one (unless it's trimmed to nothing)
bi_str_upper:
            lda         #0
            bra         s_map
bi_str_lower:
            lda         #1
            bra         s_map
bi_str_trim:
            lda         #2
s_map:
            sta         s_c
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcs         s_done
            ldy         #1                                  ; (A character: itself, changed)
            lda         (argp),y
            and         #$FE
            cmp         #>CHAR0
            bne         @string
            lda         s_ch
            jsr         s_map1
            bcs         @lb321
            jmp         s_char
@lb321:
@string:
            jsr         h_begin_msg
            stz         s_i
            stz         s_i + 1
            MOVW        s_n, s_len
            lda         s_c
            cmp         #2
            beq         @trim
            ldx         #0
@byte:
            lda         s_n
            ora         s_n + 1
            beq         @made
            jsr         s_byte
            jsr         s_map1
            jsr         out_byte
            ldx         #0
            inc         s_i
            bne         :+
            inc         s_i + 1
:
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @byte
@trim:
            lda         s_n                                 ; (Its spaces at its start ...
            ora         s_n + 1
            beq         @made
            ldx         #0
            jsr         s_byte
            jsr         s_trim_p
            bne         @end
            inc         s_i
            bne         :+
            inc         s_i + 1
:
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @trim
@end:
            clc                                             ;   and at its end: s_n less them)
            lda         s_i
            adc         s_n
            sta         s_ij
            lda         s_i + 1
            adc         s_n + 1
            sta         s_ij + 1
@back:
            lda         s_ij
            bne         :+
            dec         s_ij + 1
:
            dec         s_ij
            ldx         #0
            jsr         s_bytej
            jsr         s_trim_p
            bne         @kept
            lda         s_n
            bne         :+
            dec         s_n + 1
:
            dec         s_n
            bra         @back
@kept:
            jsr         s_out
@made:
            jmp         s_end

; The byte .A as s_c has it (0 upper case, 1 lower case, 2 trimmed).  OUT: .A = it; C = 1 if it's trimmed away
s_map1:
            ldx         s_c
            beq         @upper
            dex
            beq         @lower
            pha
            jsr         s_trim_p
            pla
            sec
            beq         @rts
            clc
@rts:
            rts
@upper:
            cmp         #'a'
            bcc         @same
            cmp         #'z' + 1
            bcs         @same
            and         #$DF
@same:
            clc
            rts
@lower:
            cmp         #'A'
            bcc         @same
            cmp         #'Z' + 1
            bcs         @same
            ora         #$20
            clc
            rts

; Is .A a space str-trim takes (space, tab, LF, VT, FF, CR)?  OUT: Z = 1 if it is
s_trim_p:
            cmp         #' '
            beq         @rts
            cmp         #9                                  ; (Tab to CR)
            bcc         @no
            cmp         #13 + 1
            bcs         @no
            lda         #0
            rts
@no:
            lda         #1
@rts:
            rts

; (str-replace s old new): s with each old (not empty) new
bi_str_replace:
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcc         @lb320
            jmp         s_done2
@lb320:
            ldx         #1
            ldy         #2
            jsr         s_arg
            bcc         @lb319
            jmp         s_done2
@lb319:
            ldx         #1                                  ; (new a string, too; then old again)
            ldy         #4
            jsr         s_arg
            bcs         s_done2
            ldx         #1
            ldy         #2
            jsr         s_arg
            lda         s_len + 2
            ora         s_len + 3
            bne         :+
            LDAX        s_sreplace0
            jsr         h_error
            bra         s_done2
:
            jsr         h_begin_msg
            stz         s_i
            stz         s_i + 1
@at:
            lda         s_i                                 ; (At its end: done)
            cmp         s_len
            lda         s_i + 1
            sbc         s_len + 1
            bcs         @made
            jsr         s_match
            bcs         @new
            ldx         #0
            jsr         s_byte
            jsr         out_byte
            inc         s_i
            bne         @at
            inc         s_i + 1
            bra         @at
@new:
            lda         s_i                                 ; (new out, then s_i past old)
            pha
            lda         s_i + 1
            pha
            ldx         #1
            ldy         #4
            jsr         s_arg
            ldx         #1
            jsr         s_outall
            ldx         #1
            ldy         #2
            jsr         s_arg
            pla
            sta         s_i + 1
            pla
            clc
            adc         s_len + 2
            sta         s_i
            lda         s_i + 1
            adc         s_len + 3
            sta         s_i + 1
            bra         @at
@made:
            jmp         s_end
s_done2:
            clc
            rts

; (str-join l [sep]): the items as print shows them, sep between them
bi_str_join:
            jsr         h_enter
            jsr         h_arg0
            jsr         q_conses
            bcc         :+
            jsr         h_begin_msg                         ; ("'str-join' expects a QExpr")
            lda         #$27
            jsr         out_byte
            ldx         bi_n
            FARN        2, bi_name
            LDAX        s_sqexpr
            jsr         out_text
            jsr         h_end_msg
            bra         s_done2
:
            sta         s_list
            stx         s_list + 1
            lda         cn
            cmp         #2
            bcc         :+
            ldx         #1
            ldy         #2
            jsr         s_arg
            bcs         s_done2
:
            jsr         h_begin_msg
            stz         s_c                                 ; (None out yet)
@item:
            lda         s_list
            ora         s_list + 1
            beq         @made
            lda         s_c                                 ; (sep between)
            beq         @first
            lda         cn
            cmp         #2
            bcc         @first
            ldx         #1
            ldy         #2
            jsr         s_arg
            ldx         #1
            jsr         s_outall
@first:
            lda         #1
            sta         s_c
            lda         s_list
            ldx         s_list + 1
            ldy         #0
            jsr         cell_get
            ldy         #1
            FARN        2, print_val
            lda         s_list
            ldx         s_list + 1
            ldy         #1
            jsr         cell_get
            sta         s_list
            stx         s_list + 1
            bra         @item
@made:
            jmp         s_end

; (str-chars s): its characters, a list (lists.inc's str_chars)
bi_str_chars:
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcs         s_done3
            jsr         h_arg0
            sta         ex
            stx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         @char
            FARN        2, str_chars
            bcc         :+
            jmp         h_nomem
:
            MOVW        ex, hv
            bra         s_done3
@char:
            MOVW        et, ex                              ; (A character: a list of it)
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         h_make2
            MOVW        ex, hv
s_done3:
            clc
            rts

; (str-pad-left s n [c]), (str-pad-right s n [c]): s made n long, c (a space; a string of one) before it or after it
bi_str_pad_left:
            lda         #0
            bra         s_pad
bi_str_pad_right:
            lda         #1
s_pad:
            sta         s_c
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcs         s_done3
            LDAX        s_snumber
            ldy         #2
            jsr         s_whole
            bcs         s_done3
            lda         s_n + 1
            bpl         :+
            LDAX        s_snegpad
            jsr         s_error_fn
            bra         s_done3
:
            lda         #' '
            sta         s_pc
            lda         cn
            cmp         #3
            bcc         @pad
            ldx         #1
            ldy         #4
            jsr         s_arg
            bcs         s_done3
            lda         s_len + 2
            cmp         #1
            bne         @one
            lda         s_len + 3
            beq         :+
@one:
            LDAX        s_spadone
            jsr         s_error_fn
            bra         s_done3
:
            stz         s_i
            stz         s_i + 1
            ldx         #1
            jsr         s_byte
            sta         s_pc
@pad:
            sec                                             ; (The pad: n less its length, if that's above 0)
            lda         s_n
            sbc         s_len
            sta         s_m
            lda         s_n + 1
            sbc         s_len + 1
            sta         s_m + 1
            bpl         :+
            stz         s_m
            stz         s_m + 1
:
            jsr         h_begin_msg
            lda         s_c
            bne         :+
            jsr         s_padout
:
            stz         s_i
            stz         s_i + 1
            MOVW        s_n, s_len
            jsr         s_out
            lda         s_c
            beq         :+
            jsr         s_padout
:
            jmp         s_end

; s_m pads (s_pc) out
s_padout:
            lda         s_m
            ora         s_m + 1
            beq         @rts
            lda         s_pc
            jsr         out_byte
            lda         s_m
            bne         :+
            dec         s_m + 1
:
            dec         s_m
            bra         s_padout
@rts:
            rts

; (str-repeat s n): s n times
bi_str_repeat:
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcc         @lb318
            jmp         s_done4
@lb318:
            LDAX        s_snumber
            ldy         #2
            jsr         s_whole
            bcs         s_done4
            lda         s_n + 1
            bpl         :+
            LDAX        s_snegrep
            jsr         s_error_fn
            bra         s_done4
:
            MOVW        s_m, s_n
            jsr         h_begin_msg
@one:
            lda         s_m
            ora         s_m + 1
            beq         @made
            stz         s_i
            stz         s_i + 1
            MOVW        s_n, s_len
            jsr         s_out
            lda         s_m
            bne         :+
            dec         s_m + 1
:
            dec         s_m
            bra         @one
@made:
            jmp         s_end

; (char-code c): a character's code (or a string of one's)
bi_char_code:
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcs         s_done4
            lda         s_len
            cmp         #1
            bne         @one
            lda         s_len + 1
            beq         :+
@one:
            LDAX        s_scode1
            jsr         h_error
            bra         s_done4
:
            stz         s_i
            stz         s_i + 1
            ldx         #0
            jsr         s_byte
            ldx         #0
            jsr         fix_ax
            sta         ex
            stx         ex + 1
s_done4:
            clc
            rts

; (code-char n): the character of code n (0-255)
bi_code_char:
            jsr         h_enter
            LDAX        s_snumber
            ldy         #0
            jsr         s_whole
            bcs         s_done4
            lda         s_n + 1
            bne         @outside
            lda         s_n
            jmp         s_char
@outside:
            jsr         h_begin_msg                         ; ("'code-char': 256 isn't a character's code (0-255)")
            LDAX        s_scodechar
            jsr         out_text
            jsr         s_outn
            LDAX        s_scodechar2
            jsr         out_text
            jsr         h_end_msg
            bra         s_done4

; (alpha? c), (digit? c), (space? c), (upper? c), (lower? c): whether a character is one, or every character of a
; string that isn't empty
bi_alpha_p:
            lda         #0
            bra         s_test
bi_digit_p:
            lda         #1
            bra         s_test
bi_space_p:
            lda         #2
            bra         s_test
bi_upper_p:
            lda         #3
            bra         s_test
bi_lower_p:
            lda         #4
s_test:
            sta         s_c
            jsr         h_enter
            ldx         #0
            ldy         #0
            jsr         s_arg
            bcs         s_done4
            stz         ex                                  ; (Empty: NIL)
            stz         ex + 1
            stz         s_i
            stz         s_i + 1
            lda         s_len
            ora         s_len + 1
            beq         s_done4
@byte:
            lda         s_i
            cmp         s_len
            bne         :+
            lda         s_i + 1
            cmp         s_len + 1
            beq         @yes
:
            ldx         #0
            jsr         s_byte
            ldx         s_c
            jsr         s_class
            bcs         @lb317
            jmp         s_done4
@lb317:
            inc         s_i
            bne         @byte
            inc         s_i + 1
            bra         @byte
@yes:
            lda         #<T_VAL
            sta         ex
            jmp         s_done4

; Is .A of class .X (0 a letter, 1 a digit, 2 a space, 3 upper case, 4 lower case: ASCII's)?  OUT: C = 1 if it is
s_class:
            cpx         #1
            beq         @digit
            cpx         #2
            beq         @space
            cpx         #3
            beq         @upper
            cpx         #4
            beq         @lower
            ora         #$20                                ; (A letter: either case)
@lower:
            cmp         #'a'
            bcc         @no
            cmp         #'z' + 1
            bcs         @no
            sec
            rts
@upper:
            cmp         #'A'
            bcc         @no
            cmp         #'Z' + 1
            bcs         @no
            sec
            rts
@digit:
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcs         @no
            sec
            rts
@space:
            cmp         #' '
            beq         @yes
            cmp         #9                                  ; (Tab to CR)
            bcc         @no
            cmp         #13 + 1
            bcs         @no
@yes:
            sec
            rts
@no:
            clc
            rts

; (str-split s [sep]): the pieces of s between its separators (sep a string, or a list of strings; none given, space,
; tab, LF and CR; none but empty ones, .NET's white space), empty pieces kept: at each place, the first separator
; there (in the list's order) is taken
bi_str_split:
            jsr         h_enter
            jsr         h_arg0                              ; (s: a string)
            jsr         kind_of
            cmp         #PK_STRING
            beq         :+
            LDAX        s_ssplit1
            jsr         h_error
            jmp         s_split_done
:
            stz         s_mode                              ; (Its separators: S_*)
            lda         cn
            cmp         #2
            bcs         :+
            jmp         @go
:
            ldy         #2
            lda         (argp),y
            sta         s_w
            iny
            lda         (argp),y
            sta         s_w + 1
            tax
            lda         s_w
            jsr         kind_of
            cmp         #PK_STRING
            bne         @list
            lda         #S_ONE
            sta         s_mode
            lda         s_w
            ldx         s_w + 1
            jsr         s_empty
            bne         @go
            lda         #S_WHITE
            sta         s_mode
            bra         @go
@list:
            lda         s_w
            ldx         s_w + 1
            jsr         q_conses
            bcs         @bad
            sta         s_w
            stx         s_w + 1
            ora         s_w + 1
            beq         @bad
            lda         #S_WHITE
            sta         s_mode
@each:
            lda         s_w                                 ; (Each a string; one not empty: S_LIST)
            ora         s_w + 1
            beq         @go
            lda         s_w
            ldx         s_w + 1
            ldy         #0
            jsr         cell_get
            sta         s_dv
            stx         s_dv + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         @bad
            lda         s_dv
            ldx         s_dv + 1
            jsr         s_empty
            beq         :+
            lda         #S_LIST
            sta         s_mode
:
            lda         s_w
            ldx         s_w + 1
            ldy         #1
            jsr         cell_get
            sta         s_w
            stx         s_w + 1
            bra         @each
@bad:
            LDAX        s_ssplit2
            jsr         h_error
            jmp         s_split_done
@go:
            stz         cf                                  ; (The pieces: cf, cr; each from s_from)
            stz         cf + 1
            stz         cr
            stz         cr + 1
            stz         s_i
            stz         s_i + 1
            stz         s_from
            stz         s_from + 1
            jsr         s_subject
@at:
            lda         s_i
            cmp         s_len
            lda         s_i + 1
            sbc         s_len + 1
            bcs         @last
            jsr         s_sep                               ; (A separator here: s_m its length)
            bcs         @piece
            inc         s_i
            bne         @at
            inc         s_i + 1
            bra         @at
@piece:
            jsr         s_piece                             ; (The piece before it; on past it)
            jsr         s_subject
            clc
            lda         s_i
            adc         s_m
            sta         s_i
            sta         s_from
            lda         s_i + 1
            adc         s_m + 1
            sta         s_i + 1
            sta         s_from + 1
            bra         @at
@last:
            jsr         s_piece
            MOVW        ex, cf
s_split_done:
            clc
            rts

; Is the string .A, .X empty?  OUT: Z = 1 if it is
s_empty:
            ldy         #0
            jsr         cell_get
            stx         s_c2
            ora         s_c2
            rts

; Is a separator at s_i (slot 0's, s)?  OUT: C = 1 and s_m = its length if one is
s_sep:
            lda         s_mode
            cmp         #S_ONE
            bcs         @given
            ldx         #0                                  ; (Space, tab, LF, CR; or white space)
            jsr         s_byte
            ldy         s_mode
            bne         @white
            cmp         #' '
            beq         @one
            cmp         #9
            beq         @one
            cmp         #10
            beq         @one
            cmp         #13
            beq         @one
            clc
            rts
@white:
            cmp         #' '
            beq         @one
            cmp         #$85
            beq         @one
            cmp         #$A0
            beq         @one
            cmp         #9                                  ; (Tab to CR)
            bcc         @no
            cmp         #13 + 1
            bcs         @no
@one:
            lda         #1
            sta         s_m
            stz         s_m + 1
            sec
            rts
@no:
            clc
            rts
@given:
            bne         @list
            ldx         #1                                  ; (One string)
            ldy         #2
            jsr         s_arg
            jmp         s_sepat
@list:
            ldy         #2                                  ; (A list of them: the first there)
            lda         (argp),y
            pha
            iny
            lda         (argp),y
            tax
            pla
            jsr         q_conses
@item:
            sta         s_w
            stx         s_w + 1
            ora         s_w + 1
            beq         @none
            lda         s_w
            pha
            lda         s_w + 1
            pha
            lda         s_w
            ldx         s_w + 1
            ldy         #0
            jsr         cell_get
            ldy         #1
            jsr         s_desc
            jsr         s_sepat
            pla
            tax
            pla
            bcs         @found
            ldy         #1
            jsr         cell_get
            bra         @item
@none:
            clc
@found:
            rts

; Is slot 1 (a separator, not empty) at s_i?  OUT: C = 1 and s_m = its length if it is
s_sepat:
            lda         s_len + 2
            ora         s_len + 3
            beq         @no
            jsr         s_match
            bcc         @no
            MOVW        s_m, s_len + 2
            sec
            rts
@no:
            clc
            rts

; The piece of s from s_from to s_i: a string, put at the pieces' end (cf, cr).  It may collect: slot 0 to be set again
s_piece:
            sec                                             ; (Its length)
            lda         s_i
            sbc         s_from
            sta         s_n
            lda         s_i + 1
            sbc         s_from + 1
            sta         s_n + 1
            lda         s_i
            pha
            lda         s_i + 1
            pha
            MOVW        s_i, s_from
            jsr         h_begin_msg
            jsr         s_out
            pla
            sta         s_i + 1
            pla
            sta         s_i
            jsr         s_end
            lda         ex
            ldx         ex + 1
            jmp         h_append

.segment "RODATA4"
s_sexpects: .byte       "' expects a String", 0
s_sqexpr:   .byte       "' expects a QExpr", 0
s_swhole1:  .byte       "': ", 0
s_swhole2:  .byte       " must be a whole number, not ", 0
s_sindex:   .byte       "an index", 0
s_snumber:  .byte       "a number", 0
s_scharat:  .byte       "'char-at': index ", 0
s_soutside: .byte       " is outside the string", 0
s_sreplace0: .byte      "'str-replace' cannot replace an empty String", 0
s_snegpad:  .byte       " length cannot be negative", 0
s_spadone:  .byte       " pads with one character", 0
s_snegrep:  .byte       " count cannot be negative", 0
s_scode1:   .byte       "'char-code' expects one character", 0
s_scodechar: .byte      "'code-char': ", 0
s_scodechar2: .byte     " isn't a character's code (0-255)", 0
s_ssplit1:  .byte       "First 'str-split' parameter must be a string", 0
s_ssplit2:  .byte       "Second 'str-split' parameter must be a list of strings or a single string", 0

.segment "CODE4"
