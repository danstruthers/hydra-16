; ****************************************************************************
; io.inc - input and output (the sixth bank): PRINT's values (a string's bytes; a number's text, DISPLAY's, in the
; base, a space before it if it isn't below 0 and one after, QuickBASIC's), its zones (14 columns), TAB and SPC; the
; column the output's at (ocol, for them and POS).

ZONE            = 14

.bss
ocol:       .res        1                                   ; The output's column (0 the first)
owidth:     .res        1                                   ; WIDTH's (80)

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
            .word       st_prn, st_prnl, st_przone, st_prtab, st_prspc
.assert     * - st_jt6 = (ST_B7 - ST_B6) * 2, error, "st_jt6 and fn.inc's ST_B6 group differ"

rt_todo6:
            lda         #73
            ldx         #0
            jmp         b_error
