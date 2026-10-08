; ****************************************************************************
; io.inc - input and output (the sixth bank): PRINT's values (a string's bytes; a number's text, DISPLAY's, in the
; base, a space before it if it isn't below 0 and one after, QuickBASIC's), its zones (14 columns), TAB and SPC; the
; column the output's at (ocol, for them and POS).

ZONE            = 14

.bss
ocol:       .res        1                                   ; The output's column (0 the first)
owidth:     .res        1                                   ; WIDTH's (80)
dptr:       .res        2                                   ; READ's next item: its place in DATA's region
in_flags:   .res        1                                   ; (INPUT's: its prompt's flags, the next field's place in
in_pos:     .res        1                                   ;   lbuf, fields wrong (Redo from start), the line's end
in_bad:     .res        1                                   ;   reached, a field's type, start and length)
in_end:     .res        1
in_type:    .res        1
in_start:   .res        1
in_len:     .res        1

.segment "CODE6"

; .A out, the column followed (a new line: 0).  Keeps .A, .X, .Y
pr_ch:
            jsr         out_byte
            cmp         #LF
            bne         :+
            stz         ocol
            rts
:
            inc         ocol
            rts

; ST_PRN: the top value written (a string's bytes, or a number's text), taken off
st_prn:
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            beq         @str
            stz         nt_base                             ; (A number: in the base)
            stz         nt_base + 1
            FARN        4, num_text                         ; (Its text in the text bank, .A/.X its length)
            sta         t0
            stx         t0 + 1
            lda         nsign                               ; (Not below 0: a space first)
            bmi         :+
            lda         #' '
            jsr         pr_ch
:
            LDW         ca, $8000
            lda         #LB_TXB
            sta         cal
            jsr         pr_far
            lda         #' '
            jsr         pr_ch
            jsr         pop_vt
            rts
@str:
            FARN        5, str_get_a                        ; (sla: its length, sfa: its bytes)
            MOVW        t0, sla
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            jsr         pr_far
            jsr         pop_vt
            rts

; t0/t0+1 bytes from cursor A written.  The code's bank after
pr_far:
            lda         t0
            ora         t0 + 1
            beq         @done
            jsr         ca_sel
@byte:
            jsr         ca_rd
            jsr         pr_ch
            lda         t0
            bne         :+
            dec         t0 + 1
:
            dec         t0
            lda         t0
            ora         t0 + 1
            bne         @byte
@done:
            jmp         code_sel

; ST_PRNL: a new line
st_prnl:
            lda         #LF
            jmp         pr_ch

; ST_PRZONE: spaces to the next zone (a new line past the width's last)
st_przone:
            lda         ocol
            clc
            adc         #ZONE
            cmp         owidth
            bcc         @spaces
            lda         #LF
            jmp         pr_ch
@spaces:
            lda         #' '
            jsr         pr_ch
            lda         ocol
:
            cmp         #ZONE
            bcc         :+
            sbc         #ZONE
            bra         :-
:
            cmp         #0
            bne         @spaces
            rts

; ST_PRTAB: to column n (from 1; past it already: on the next line)
st_prtab:
            jsr         pop_int16
            lda         t0
            beq         @done
            dec         a
            sta         t0
            cmp         ocol
            bcs         :+
            lda         #LF
            jsr         pr_ch
:
            lda         ocol
            cmp         t0
            bcs         @done
            lda         #' '
            jsr         pr_ch
            bra         :-
@done:
            rts

; ST_PRSPC: n spaces
st_prspc:
            jsr         pop_int16
:
            lda         t0
            beq         @done
            lda         #' '
            jsr         pr_ch
            dec         t0
            bra         :-
@done:
            rts

; ---- DATA's items: READ, RESTORE

; ST_READ: DATA's next item pushed, a string (its bytes in DATA's region).  None left: Out of DATA
st_read:
            lda         dptr                                ; (Its far address: t0-t2, cursor A)
            sta         ca
            sta         t0
            lda         dptr + 1
            and         #$1F
            ora         #WIN
            sta         ca + 1
            sta         t0 + 1
            lda         dptr + 1
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            clc
            adc         #LB_DATA
            sta         cal
            sta         t0 + 2
            jsr         ca_sel
            jsr         ca_rd                               ; (Its length: $FFFF the end)
            sta         t4
            jsr         ca_rd
            sta         t4 + 1
            jsr         code_sel
            and         t4
            cmp         #$FF
            bne         :+
            lda         #ER_DATA
            ldx         #0
            jmp         b_error
:
            lda         #VT_STR
            sta         (vsp)
            ldy         #1
            lda         t0
            sta         (vsp),y
            iny
            lda         t0 + 1
            sta         (vsp),y
            iny
            lda         t0 + 2
            sta         (vsp),y
            jsr         vsp_up
            sec                                             ; (dptr past it: 2 + its length)
            lda         dptr
            adc         #1
            sta         dptr
            bcc         :+
            inc         dptr + 1
:
            clc
            lda         dptr
            adc         t4
            sta         dptr
            lda         dptr + 1
            adc         t4 + 1
            sta         dptr + 1
            rts

; ST_RESTORE: READ's next item the one at the place taken off the stack (0: the first)
st_restore:
            jsr         pop_int16
            MOVW        dptr, t0
            rts

; ---- INPUT, LINE INPUT

; ST_INPUT: the prompt written (the string under the flags: bit 0, "? " after it), a line read into lbuf (its first
; 255 bytes), its fields for ST_INFLD.  Ctrl-C: Break; stdin's end: Input past end of file
st_input:
            jsr         pop_int16
            lda         t0
            sta         in_flags
            jsr         top_vt                              ; (The prompt)
            FARN        5, str_get_a
            MOVW        t0, sla
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            jsr         pr_far
            jsr         pop_vt
            lda         in_flags
            and         #1
            beq         :+
            lda         #'?'
            jsr         pr_ch
            lda         #' '
            jsr         pr_ch
:
            stz         got_intr
            FARN        1, read_line
            bcc         @have
            ldx         #0
            lda         #ER_BREAK
            ldy         got_intr
            bne         :+
            lda         #ER_PASTEND
:
            jmp         b_error
@have:
            stz         lbuf + 255
            lda         ocol                                ; (stdin not the console: no echo, so a new line)
            beq         :+
            lda         interactive
            bne         :+
            lda         #LF
            jsr         out_byte
:
            stz         ocol
            stz         in_pos
            stz         in_bad
            stz         in_end
            rts

; ST_INFLD: the line's next field pushed, of the type taken off the stack (ET_NUM: a number, read whole; ET_STR: a
; string: "text", or text up to a , with the spaces around it dropped).  None left, or not a number: in_bad (0 or ""
; pushed)
st_infld:
            jsr         pop_int16
            lda         t0
            sta         in_type
            ldx         in_pos
            lda         in_end
            beq         @skip
            inc         in_bad                              ; (None left)
            stz         in_len
            bra         @push
@skip:
            lda         lbuf,x                              ; (The spaces before it)
            cmp         #' '
            bne         :+
            inx
            bra         @skip
:
            cmp         #'"'
            bne         @plain
            inx                                             ; ("text": up to its ")
            stx         in_start
:
            lda         lbuf,x
            beq         :+
            cmp         #'"'
            beq         :+
            inx
            bra         :-
:
            txa
            sec
            sbc         in_start
            sta         in_len
            lda         lbuf,x
            beq         @after
            inx                                             ; (Past its ", and the spaces after it)
:
            lda         lbuf,x
            cmp         #' '
            bne         @after
            inx
            bra         :-
@plain:
            stx         in_start                            ; (Up to a , or the end, the spaces at its end dropped)
:
            lda         lbuf,x
            beq         :+
            cmp         #','
            beq         :+
            inx
            bra         :-
:
            phx
:
            cpx         in_start
            beq         :+
            lda         lbuf - 1,x
            cmp         #' '
            bne         :+
            dex
            bra         :-
:
            txa
            sec
            sbc         in_start
            sta         in_len
            plx
@after:
            lda         lbuf,x                              ; (A , after it: another; the end: none; else not a field)
            bne         :+
            inc         in_end
            bra         @have
:
            inx
            cmp         #','
            beq         @have
            inc         in_bad
@have:
            stx         in_pos
@push:
            lda         in_type
            bne         in_str
            lda         in_len                              ; (A number: none, 0)
            beq         @zero
            clc
            lda         #<lbuf
            adc         in_start
            sta         r0
            lda         #>lbuf
            adc         #0
            sta         r0 + 1
            lda         in_len
            sta         r1
            stz         r1 + 1
            FARN        4, num_field
            bcc         :+
            inc         in_bad
:
            rts
@zero:
            lda         #0
            bra         push_int_a

; The field (in_start, in_len in lbuf) pushed, a string
in_str:
            lda         in_len
            bne         :+
            lda         #VT_STR                             ; ("")
            sta         (vsp)
            ldy         #3
            lda         #NOBANK
            sta         (vsp),y
            jmp         vsp_up
:
            ldx         #0
            FARN        5, heap_str                         ; (hres: its block, cursor B at its bytes)
            jsr         cb_sel
            ldx         in_start
            ldy         in_len
:
            lda         lbuf,x
            jsr         cb_wr
            inx
            dey
            bne         :-
            jsr         code_sel
            lda         #VT_STR
            sta         (vsp)
            ldy         #1
            lda         hres
            sta         (vsp),y
            iny
            lda         hres + 1
            sta         (vsp),y
            iny
            lda         hres + 2
            sta         (vsp),y
            jmp         vsp_up

; .A (0 or $FF: 0 or -1) pushed, an integer
push_int_a:
            ldy         #4
:
            sta         (vsp),y
            dey
            bne         :-
            lda         #VT_INT
            sta         (vsp)
            jmp         vsp_up

; ST_INEND: INPUT's end: -1 pushed if the line's to be typed again (its fields too few, too many, a number not one),
; after ?Redo from start; else 0
st_inend:
            lda         in_end
            bne         :+
            inc         in_bad                              ; (Fields left over)
:
            lda         in_bad
            beq         push_int_a
            ldx         #0
:
            lda         s_redo,x
            beq         :+
            jsr         pr_ch
            inx
            bra         :-
:
            lda         #$FF
            bra         push_int_a
s_redo:     .byte       "?Redo from start", LF, 0

; ST_LGET: the line read (LINE INPUT's) pushed, a string
st_lget:
            ldx         #0
:
            lda         lbuf,x
            beq         :+
            inx
            bne         :-
:
            stx         in_len
            stz         in_start
            jmp         in_str

; ---- The sixth bank's functions and statements (fn.inc)
fn_disp6:
            tax
            jmp         (fn_jt6,x)
fn_jt6:
            .repeat     F_B7 - F_B6
            .word       rt_todo6
            .endrepeat

st_disp6:
            tax
            jmp         (st_jt6,x)
st_jt6:
            .word       st_prn, st_prnl, st_przone, st_prtab, st_prspc, st_read, st_restore, st_input, st_infld
            .word       st_inend, st_lget
.assert     * - st_jt6 = (ST_B7 - ST_B6) * 2, error, "st_jt6 and fn.inc's ST_B6 group differ"

rt_todo6:
            lda         #73
            ldx         #0
            jmp         b_error
