; ****************************************************************************
; fns5.inc - the strings' built-in functions (the fifth bank: fn.inc's F_B5 group), and the arrays' bounds.  A string
; made is allocated first, its source read again after (the collector may have moved it: its value is on the stack).

.bss
ss_off:     .res        2                                   ; (substr's: from, how many)
ss_n:       .res        2
ss_case:    .res        1                                   ; (0: as it is; 1 capitals; 2 small letters)
in_i:       .res        2                                   ; (INSTR's: from, the lengths, a place)
in_s:       .res        2
in_t:       .res        2
in_p:       .res        3
in_q:       .res        3
in_cnt:     .res        1

.segment "CODE5"

fn_jt5:
            .word       f_asc, f_chr, f_lcase, f_left, f_len, f_ltrim, f_mid, f_right, f_rtrim, f_space, f_string
            .word       f_ucase, f_instr, f_lbound, f_ubound, f_fre
.assert     * - fn_jt5 = (F_B6 - F_B5) * 2, error, "fn_jt5 and fn.inc's F_B5 group differ"

illegal:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; The integer .A/.X (0-65535) put at vt
int_ax_vt:
            pha
            lda         #VT_INT
            sta         (vt)
            ldy         #1
            pla
            sta         (vt),y
            iny
            txa
            sta         (vt),y
            lda         #0
            iny
            sta         (vt),y
            iny
            sta         (vt),y
            jmp         code_sel

f_len:
            jsr         top_vt
            jsr         str_get_a
            lda         sla
            ldx         sla + 1
            jmp         int_ax_vt

f_asc:
            jsr         top_vt
            jsr         str_get_a
            lda         sla
            ora         sla + 1
            beq         illegal
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            jsr         ca_sel
            jsr         ca_rd
            ldx         #0
            jmp         int_ax_vt

; CHR$(n): a string of the character n (0-255)
f_chr:
            jsr         pop_int16
            lda         t0 + 1
            bne         illegal
            lda         t0
            pha
            jsr         push_slot                           ; (Its place: the stack's top again)
            lda         #1
            ldx         #0
            jsr         heap_str
            pla
            jsr         cb_sel
            jsr         cb_wr
            jsr         top_vt
            jmp         str_hres_vt

; vsp on past the value just taken (its place used again)
push_slot:
            jmp         vsp_up

; vt's value: VT_STR at hres
str_hres_vt:
            lda         #VT_STR
            sta         (vt)
            ldy         #1
            lda         hres
            sta         (vt),y
            iny
            lda         hres + 1
            sta         (vt),y
            iny
            lda         hres + 2
            sta         (vt),y
            ldy         #4
            lda         #0
            sta         (vt),y
            jmp         code_sel

; LEFT$(s, n), RIGHT$(s, n)
f_left:
            jsr         pop_count
            jsr         top_vt
            jsr         str_get_a
            jsr         n_le_len
            stz         ss_off
            stz         ss_off + 1
            jmp         substr
f_right:
            jsr         pop_count
            jsr         top_vt
            jsr         str_get_a
            jsr         n_le_len
            sec
            lda         sla
            sbc         ss_n
            sta         ss_off
            lda         sla + 1
            sbc         ss_n + 1
            sta         ss_off + 1
            jmp         substr

; ss_n = the count taken off the stack (below 0: Illegal function call)
pop_count:
            jsr         pop_int16
            lda         t0 + 1
            bpl         @lb904
            jmp         illegal
@lb904:
            MOVW        ss_n, t0
            rts

; ss_n made no more than the string's length (sla)
n_le_len:
            lda         ss_n + 1
            cmp         sla + 1
            bcc         @ok
            bne         @len
            lda         ss_n
            cmp         sla
            bcc         @ok
@len:
            MOVW        ss_n, sla
@ok:
            rts

; MID$(s, i [, n]): (its count pushed) n from the ith (from 1)
f_mid:
            jsr         pop_int16                           ; (The count: 2 or 3)
            lda         #$FF
            sta         ss_n
            sta         ss_n + 1
            lda         t0
            cmp         #3
            bne         :+
            jsr         pop_count
:
            jsr         pop_int16                           ; (i: 1 on)
            lda         t0
            ora         t0 + 1
            bne         @lb903
            jmp         illegal
@lb903:
            lda         t0 + 1
            bpl         @lb902
            jmp         illegal
@lb902:
            sec
            lda         t0
            sbc         #1
            sta         ss_off
            lda         t0 + 1
            sbc         #0
            sta         ss_off + 1
            jsr         top_vt
            jsr         str_get_a
            sec                                             ; (Past its end: "")
            lda         sla
            sbc         ss_off
            sta         t0
            lda         sla + 1
            sbc         ss_off + 1
            sta         t0 + 1
            bcs         :+
            stz         t0
            stz         t0 + 1
:
            lda         ss_n + 1                            ; (n: no more than what's left)
            cmp         t0 + 1
            bcc         :+
            bne         @left
            lda         ss_n
            cmp         t0
            bcc         :+
@left:
            MOVW        ss_n, t0
:
            jmp         substr

; The string at vt made its ss_n bytes from ss_off (as they are, or in capitals or small letters: ss_case), in its
; place; "" for none
substr:
            stz         ss_case
substr_case:
            lda         ss_n
            ora         ss_n + 1
            bne         :+
            ldy         #3                                  ; ("")
            lda         #NOBANK
            sta         (vt),y
            jmp         code_sel
:
            lda         vt                                  ; (Its place on the stack, kept)
            pha
            lda         vt + 1
            pha
            lda         ss_n
            ldx         ss_n + 1
            jsr         heap_str
            pla
            sta         vt + 1
            pla
            sta         vt
            jsr         str_get_a                           ; (Again: it may have moved)
            jsr         cb_after_len
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            lda         ss_off
            ldx         ss_off + 1
            jsr         ca_plus5
            MOVW        cn, ss_n
            lda         ss_case
            bne         @case
            jsr         far_copy
            jmp         str_hres_vt
@case:
            jsr         ca_sel                              ; (A byte at a time, its case changed)
            jsr         ca_rd
            ldx         ss_case
            cpx         #1
            bne         @small
            cmp         #'a'
            bcc         @put
            cmp         #'z' + 1
            bcs         @put
            and         #$DF
            bra         @put
@small:
            cmp         #'A'
            bcc         @put
            cmp         #'Z' + 1
            bcs         @put
            ora         #$20
@put:
            pha
            jsr         cb_sel
            pla
            jsr         cb_wr
            lda         cn
            bne         :+
            dec         cn + 1
:
            dec         cn
            lda         cn
            ora         cn + 1
            bne         @case
            jmp         str_hres_vt

; Cursor A on by .A/.X bytes
ca_plus5:
            clc
            adc         ca
            sta         ca
            txa
            adc         ca + 1
@wrap:
            cmp         #WIN_END
            bcc         :+
            sbc         #WIN_END - WIN
            inc         cal
            bra         @wrap
:
            sta         ca + 1
            rts

f_ucase:
            lda         #1
            bra         :+
f_lcase:
            lda         #2
:
            sta         ss_case
            jsr         top_vt
            jsr         str_get_a
            stz         ss_off
            stz         ss_off + 1
            MOVW        ss_n, sla
            jmp         substr_case

; LTRIM$(s), RTRIM$(s): the spaces at its start, or its end, taken off
f_ltrim:
            jsr         top_vt
            jsr         str_get_a
            stz         ss_off
            stz         ss_off + 1
            MOVW        ss_n, sla
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            jsr         ca_sel
@sp:
            lda         ss_n
            ora         ss_n + 1
            beq         @done
            jsr         ca_rd
            cmp         #' '
            bne         @done
            inc         ss_off
            bne         :+
            inc         ss_off + 1
:
            lda         ss_n
            bne         :+
            dec         ss_n + 1
:
            dec         ss_n
            bra         @sp
@done:
            jmp         substr

f_rtrim:
            jsr         top_vt
            jsr         str_get_a
            stz         ss_off
            stz         ss_off + 1
            MOVW        ss_n, sla
@sp:
            lda         ss_n                                ; (Its last byte: a space?)
            ora         ss_n + 1
            beq         @done
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            sec
            lda         ss_n
            sbc         #1
            pha
            lda         ss_n + 1
            sbc         #0
            tax
            pla
            jsr         ca_plus5
            jsr         ca_sel
            jsr         ca_rd
            cmp         #' '
            bne         @done
            lda         ss_n
            bne         :+
            dec         ss_n + 1
:
            dec         ss_n
            bra         @sp
@done:
            jmp         substr

; SPACE$(n), STRING$(n, c): n of a character (c a code, or a string's first)
f_space:
            jsr         pop_count
            lda         #' '
            bra         fill_str
f_string:
            jsr         top_vt                              ; (c: a string's first, or a code)
            lda         (vt)
            cmp         #VT_STR
            bne         @code
            jsr         str_get_a
            lda         sla
            ora         sla + 1
            beq         @bad
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            jsr         ca_sel
            jsr         ca_rd
            pha
            jsr         pop_vt
            bra         @n
@code:
            jsr         pop_int16
            lda         t0 + 1
            bne         @bad
            lda         t0
            pha
@n:
            jsr         pop_count
            pla
            bra         fill_str
@bad:
            jmp         illegal

; A string of ss_n bytes of .A pushed
fill_str:
            pha
            jsr         code_sel
            lda         ss_n
            ldx         ss_n + 1
            jsr         heap_str
            jsr         cb_sel
            pla
@byte:
            ldx         ss_n
            bne         :+
            ldx         ss_n + 1
            beq         @done
            dec         ss_n + 1
:
            dec         ss_n
            jsr         cb_wr
            bra         @byte
@done:
            jsr         vsp_up                              ; (The string pushed: its place the stack's top)
            jsr         top_vt
            lda         ss_n                                ; ("" for none)
            jmp         str_hres_vt

; INSTR([i,] s, t): (its count pushed) where t is in s, from the ith (1 on), or 0
f_instr:
            jsr         pop_int16
            lda         #1
            sta         in_i
            stz         in_i + 1
            lda         t0
            sta         in_cnt
            cmp         #3
            bne         @strs
            jsr         two_vals                            ; (i: below the two strings)
            sec
            lda         vt
            sbc         #VAL_SIZE
            sta         vt
            bcs         @i
            dec         vt + 1
@i:
            lda         (vt)
            beq         :+
            jmp         illegal
:
            ldy         #1
            lda         (vt),y
            sta         in_i
            iny
            lda         (vt),y
            sta         in_i + 1
            ora         in_i
            bne         @strs
            jmp         illegal
@strs:
            jsr         two_vals
            jsr         str_get_a                           ; (s: sla, sfa; t: slb, sfb)
            jsr         str_get_b
            MOVW        in_s, sla
            MOVW        in_t, slb
            lda         in_i                                ; (The places to try: from i - 1 to s - t)
            sec
            sbc         #1
            sta         in_i
            bcs         :+
            dec         in_i + 1
:
@try:
            sec                                             ; (Past s - t: 0)
            lda         in_s
            sbc         in_t
            tax
            lda         in_s + 1
            sbc         in_t + 1
            bcc         @none
            cmp         in_i + 1
            bcc         @none
            bne         :+
            cpx         in_i
            bcc         @none
:
            jsr         in_match
            bcc         @found
            inc         in_i
            bne         @try
            inc         in_i + 1
            bra         @try
@found:
            inc         in_i                                ; (Its place, from 1)
            bne         :+
            inc         in_i + 1
:
            lda         in_i
            ldx         in_i + 1
            bra         @put
@none:
            lda         #0
            tax
@put:
            pha
            phx
            jsr         two_vals                            ; (The result in the first argument's place)
            lda         in_cnt                              ; (Three: i's place, two below the top)
            cmp         #3
            bne         :+
            sec
            lda         vt
            sbc         #VAL_SIZE
            sta         vt
            bcs         :+
            dec         vt + 1
:
            clc
            lda         vt
            adc         #VAL_SIZE
            sta         vsp
            lda         vt + 1
            adc         #0
            sta         vsp + 1
            plx
            pla
            jmp         int_ax_vt

; Does t (in_t bytes at sfb) match s (sfa) at in_i?  C = 0 yes
in_match:
            lda         sfa
            sta         in_p
            lda         sfa + 1
            sta         in_p + 1
            lda         sfa + 2
            sta         in_p + 2
            lda         sfb
            sta         in_q
            lda         sfb + 1
            sta         in_q + 1
            lda         sfb + 2
            sta         in_q + 2
            MOVW        ca, in_p
            lda         in_p + 2
            sta         cal
            lda         in_i
            ldx         in_i + 1
            jsr         ca_plus5
            MOVW        cb, in_q
            lda         in_q + 2
            sta         cbl
            MOVW        cn, in_t
@byte:
            lda         cn
            ora         cn + 1
            beq         @yes
            jsr         ca_sel
            jsr         ca_rd
            sta         t4
            jsr         cb_sel
            jsr         cb_rd
            cmp         t4
            bne         @no
            lda         cn
            bne         :+
            dec         cn + 1
:
            dec         cn
            bra         @byte
@yes:
            jsr         code_sel
            clc
            rts
@no:
            jsr         code_sel
            sec
            rts

; LBOUND(a, d), UBOUND(a, d): the array's (VT_REF to its variable) dimension d's lowest or highest
f_lbound:
            stz         t4 + 3
            bra         :+
f_ubound:
            lda         #1
            sta         t4 + 3
:
            jsr         pop_int16                           ; (d: 1 on)
            lda         t0 + 1
            beq         @lb901
            jmp         @bad
@lb901:
            lda         t0
            bne         @lb900
            jmp         @bad
@lb900:
            sta         t4 + 2
            jsr         top_vt                              ; (The array: made if it isn't yet)
            ldy         #1
            lda         (vt),y
            sta         vu
            iny
            lda         (vt),y
            sta         vu + 1
            ldy         #3
            lda         (vu),y
            cmp         #NOBANK
            bne         :+
            lda         t4 + 2
            pha
            lda         t4 + 3
            pha
            lda         #1
            jsr         arr_auto
            pla
            sta         t4 + 3
            pla
            sta         t4 + 2
:
            ldy         #1
            lda         (vu),y
            sta         ca
            iny
            lda         (vu),y
            sta         ca + 1
            iny
            lda         (vu),y
            sta         cal
            jsr         ca_sel
            jsr         ca_rd                               ; (Its dimensions: d of them at least)
            cmp         t4 + 2
            bcc         @bad
            jsr         ca_rd
            dec         t4 + 2                              ; (Dimension d's: 4 (d - 1) bytes on)
            beq         @this
:
            jsr         ca_rd
            jsr         ca_rd
            jsr         ca_rd
            jsr         ca_rd
            dec         t4 + 2
            bne         :-
@this:
            jsr         ca_rd
            sta         t0
            jsr         ca_rd
            sta         t0 + 1
            jsr         ca_rd
            sta         t0 + 2
            jsr         ca_rd
            sta         t0 + 3
            lda         t4 + 3                              ; (UBOUND: lowest + count - 1)
            beq         @have
            clc
            lda         t0
            adc         t0 + 2
            sta         t0
            lda         t0 + 1
            adc         t0 + 3
            sta         t0 + 1
            lda         t0
            bne         :+
            dec         t0 + 1
:
            dec         t0
@have:
            jsr         top_vt                              ; (A signed 16-bit integer)
            lda         #VT_INT
            sta         (vt)
            ldy         #1
            lda         t0
            sta         (vt),y
            iny
            lda         t0 + 1
            sta         (vt),y
            ldx         #0
            cmp         #$80
            bcc         :+
            dex
:
            txa
            iny
            sta         (vt),y
            iny
            sta         (vt),y
            jmp         code_sel
@bad:
            jmp         illegal

; FRE(), FRE(x): (its count pushed) the heap's room left, in bytes (its banks not taken yet, to LB_HEAP_END)
f_fre:
            jsr         pop_int16
            lda         t0
            beq         :+
            jsr         pop_vt
:
            jsr         vsp_up                              ; (Its place)
            jsr         top_vt
            lda         #LB_HEAP_END                        ; ((LB_HEAP_END - hp's bank) * 8K, less hp's offset in it)
            sec
            sbc         hp + 2
            sta         t0 + 1
            stz         t0 + 2
            ldx         #5
:
            asl         t0 + 1
            rol         t0 + 2
            dex
            bne         :-
            stz         t0
            sec
            lda         t0
            sbc         hp
            sta         t0
            lda         hp + 1
            and         #$1F
            sta         t4
            lda         t0 + 1
            sbc         t4
            sta         t0 + 1
            lda         t0 + 2
            sbc         #0
            sta         t0 + 2
            lda         #VT_INT
            sta         (vt)
            ldy         #1
            lda         t0
            sta         (vt),y
            iny
            lda         t0 + 1
            sta         (vt),y
            iny
            lda         t0 + 2
            sta         (vt),y
            iny
            lda         #0
            sta         (vt),y
            jmp         code_sel
