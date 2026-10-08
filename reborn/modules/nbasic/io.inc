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
rs_ptr:     .res        2                                   ; (ram_str's)
kstate:     .res        1                                   ; The console's keys: 0 not opened yet, 1 open, $FF none
kraw:       .res        1                                   ; <> 0: raw now (consctl's rawon)
kfd:        .res        1                                   ; /dev/cons, read waiting ...
nbfd:       .res        1                                   ;   and not
kctl:       .res        1                                   ; /dev/consctl
kpath:      .res        16                                  ; (A path or a control line, for a call: in the RAM)
kbuf:       .res        4                                   ; (A key read)
sl_t:       .res        4                                   ; (SLEEP's ticks)

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
            jsr         con_cooked
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
            clc
            lda         #<lbuf
            adc         in_start
            sta         rs_ptr
            lda         #>lbuf
            adc         #0
            sta         rs_ptr + 1
            lda         in_len
            ; (fall through)

; The .A bytes at rs_ptr (the RAM) pushed, a string
ram_str:
            sta         in_len
            cmp         #0
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
            MOVW        t4, rs_ptr
            ldy         #0
:
            lda         (t4),y
            jsr         cb_wr
            iny
            cpy         in_len
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

; ---- ERR, ERL, ERR$, POS

; ERR: the last error's code (a system error's: 256 + its own)
f_err:
            lda         err_code
            ldx         err_code + 1
            bra         push_ax
; ERL: its line (0: none, or the prompt's)
f_erl:
            lda         err_line
            ldx         err_line + 1
            ; (fall through)

; .A/.X (0 to 65535) pushed, an integer
push_ax:
            pha
            lda         #VT_INT
            sta         (vsp)
            ldy         #1
            pla
            sta         (vsp),y
            iny
            txa
            sta         (vsp),y
            lda         #0
            iny
            sta         (vsp),y
            iny
            sta         (vsp),y
            jmp         vsp_up

; ERR$: its message
f_errs:
            FARN        7, err_text_cap
            LDW         rs_ptr, eo_buf
            lda         eo_len
            jmp         ram_str

; POS, POS(x): the output's column (from 1)
f_pos:
            jsr         pop_int16                           ; (Its arguments' count: x dropped)
            lda         t0
            beq         :+
            jsr         pop_vt
:
            lda         ocol
            inc         a
            ldx         #0
            bra         push_ax

; ---- The console's screen: VT100's sequences

; CSI (ESC [) out
out_csi:
            lda         #ESC
            jsr         out_byte
            lda         #'['
            jmp         out_byte

; .A (0-255) out in decimal
out_dec:
            ldy         #0                                  ; (.Y: digits out so far, a leading 0 dropped)
            ldx         #0
@h:
            cmp         #100
            bcc         :+
            sbc         #100
            inx
            bra         @h
:
            jsr         @digit
            ldx         #0
@t:
            cmp         #10
            bcc         :+
            sbc         #10
            inx
            bra         @t
:
            jsr         @digit
            ora         #'0'
            jmp         out_byte
@digit:                                                     ; (.X: out, unless it's a leading 0.  Keeps .A)
            pha
            txa
            bne         :+
            cpy         #0
            beq         :++
:
            ora         #'0'
            jsr         out_byte
            iny
:
            pla
            rts

; ST_CLS: the screen cleared, the cursor at its top left
st_cls:
            jsr         out_csi
            lda         #'2'
            jsr         out_byte
            lda         #'J'
            jsr         out_byte
            jsr         out_csi
            lda         #'H'
            jsr         out_byte
            stz         ocol
            rts

; ST_LOCATE: the cursor at row, col (from 1; a row alone: col 1; a col alone: on this row); cursor 0 off, 1 on
st_locate:
            jsr         pop_int16                           ; (The cursor)
            lda         t0 + 1
            bmi         @col
            jsr         out_csi
            lda         #'?'
            jsr         out_byte
            lda         #25
            jsr         out_dec
            ldx         #'l'
            lda         t0
            beq         :+
            ldx         #'h'
:
            txa
            jsr         out_byte
@col:
            jsr         pop_int16
            lda         t0 + 1
            bpl         :+
            lda         #1
            sta         t0
            stz         t0 + 1
            lda         #$80
            sta         kbuf                                ; (No col: kbuf bit 7)
            bra         :++
:
            stz         kbuf
:
            jsr         lc_check
            lda         t0
            sta         kbuf + 1
            jsr         pop_int16                           ; (The row)
            lda         t0 + 1
            bmi         @norow
            jsr         lc_check
            jsr         out_csi
            lda         t0
            jsr         out_dec
            lda         #';'
            jsr         out_byte
            lda         kbuf + 1
            jsr         out_dec
            lda         #'H'
            jsr         out_byte
            bra         @at
@norow:
            bit         kbuf                                ; (Neither: nothing)
            bmi         @done
            lda         #CR                                 ; (A col alone: from this row's start)
            jsr         out_byte
            lda         kbuf + 1
            dec         a
            beq         @at
            jsr         out_csi
            lda         kbuf + 1
            dec         a
            jsr         out_dec
            lda         #'C'
            jsr         out_byte
@at:
            lda         kbuf + 1
            dec         a
            sta         ocol
@done:
            rts

; t0 a row's or col's number: 1 to 255, else Illegal function call
lc_check:
            lda         t0 + 1
            bne         :+
            lda         t0
            beq         :+
            rts
:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; ST_COLOR: the text's colour and its background's (0-15, the terminal's: 0-7 its eight, 8-15 their bright ones)
st_color:
            jsr         pop_int16                           ; (The background)
            lda         #40
            jsr         col_set
            jsr         pop_int16
            lda         #30
            ; (fall through)

; t0 a colour (below 0: none) set: its SGR from .A (30 a text's, 40 a background's; 60 more if it's bright)
col_set:
            ldx         t0 + 1
            bmi         @done
            bne         @bad
            ldx         t0
            cpx         #16
            bcs         @bad
            cpx         #8
            bcc         :+
            clc                                             ; (A bright one: 60 more, its number less 8)
            adc         #60
            pha
            txa
            sec
            sbc         #8
            tax
            pla
:
            stx         t0
            clc
            adc         t0
            pha
            jsr         out_csi
            pla
            jsr         out_dec
            lda         #'m'
            jmp         out_byte
@bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error
@done:
            rts

; ST_WIDTH: PRINT's line's width (its zones and wrapping), 1 to 255
st_width:
            jsr         pop_int16
            jsr         lc_check
            lda         t0
            sta         owidth
            rts

; ST_BEEP: the bell
st_beep:
            lda         #BEL
            jmp         out_byte

; ST_SLEEP: ticks waited (TICK_HZ a second: SLEEP's seconds times it, rounded), or (below 0) a key
st_sleep:
            jsr         out_flush
            jsr         top_vt
            lda         (vt)
            beq         :+
            FARN        4, num_to_int
:
            jsr         pop_vt
            ldy         #4
            lda         (vt),y
            bmi         @key
:
            lda         (vt),y
            sta         sl_t - 1,y
            dey
            bne         :-
@chunk:
            lda         sl_t + 3                            ; (32767 at a time)
            ora         sl_t + 2
            bne         @big
            lda         sl_t + 1
            bmi         @big
            lda         sl_t
            ldx         sl_t + 1
            jmp         SLEEP
@big:
            lda         #$FF
            ldx         #$7F
            jsr         SLEEP
            bcs         @done                               ; (Ctrl-C: the program's check stops it)
            sec
            lda         sl_t
            sbc         #$FF
            sta         sl_t
            lda         sl_t + 1
            sbc         #$7F
            sta         sl_t + 1
            lda         sl_t + 2
            sbc         #0
            sta         sl_t + 2
            lda         sl_t + 3
            sbc         #0
            sta         sl_t + 3
            bra         @chunk
@key:
            jsr         con_raw
            bcs         @done
            jsr         key_wait
@done:
            rts

; ---- WRITE

; ST_WRVAL: the top value written, taken off: a string in quotes, a number's text (no spaces)
st_wrval:
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            beq         @str
            stz         nt_base
            stz         nt_base + 1
            FARN        4, num_text
            sta         t0
            stx         t0 + 1
            LDW         ca, $8000
            lda         #LB_TXB
            sta         cal
            jsr         pr_far
            jmp         pop_vt
@str:
            lda         #'"'
            jsr         pr_ch
            FARN        5, str_get_a
            MOVW        t0, sla
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            jsr         pr_far
            lda         #'"'
            jsr         pr_ch
            jmp         pop_vt

; ST_WRCOM: WRITE's ,
st_wrcom:
            lda         #','
            jmp         pr_ch

; ---- The console's keys

; The console's keys raw (consctl's rawon; its fds opened the first time), the output written first.  OUT: C = 1 no
; console
con_raw:
            jsr         out_flush
            lda         kraw
            bne         @yes
            lda         kstate
            bmi         @no
            bne         @on
            LDW         t0, s_consctl
            jsr         k_copy
            LDR         r0, kpath
            lda         #O_WRITE
            jsr         OPEN
            bcs         @none
            sta         kctl
            LDW         t0, s_cons
            jsr         k_copy
            LDR         r0, kpath
            lda         #O_READ
            jsr         OPEN
            bcs         @none
            sta         kfd
            LDR         r0, kpath
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            bcs         @none
            sta         nbfd
            lda         #1
            sta         kstate
@on:
            LDW         t0, s_rawon
            jsr         k_ctl
            lda         #1
            sta         kraw
@yes:
            clc
            rts
@none:
            lda         #$FF
            sta         kstate
@no:
            sec
            rts

; The console's lines cooked again (rawoff), if they were raw
con_cooked:
            lda         kraw
            beq         :+
            stz         kraw
            LDW         t0, s_rawoff
            jmp         k_ctl
:
            rts

; consctl's line at (t0) written
k_ctl:
            jsr         k_copy
            sty         r1
            stz         r1 + 1
            LDR         r0, kpath
            lda         kctl
            jmp         WRITE

; The string at (t0) (zero-terminated, in this bank) copied to kpath.  OUT: .Y its length
k_copy:
            ldy         #0
:
            lda         (t0),y
            sta         kpath,y
            beq         :+
            iny
            bra         :-
:
            rts
s_cons:     .byte       "/dev/cons", 0
s_consctl:  .byte       "/dev/consctl", 0
s_rawon:    .byte       "rawon", 0
s_rawoff:   .byte       "rawoff", 0

; A key waited for (raw): .A.  Ctrl-C: Break
key_wait:
            LDR         r0, kbuf
            LDR         r1, 1
            lda         kfd
            jsr         READ
            bcs         @intr
            cmp         #0
            beq         key_wait
            lda         kbuf
            rts
@intr:
            lda         #$80
            trb         intr
            lda         #ER_BREAK
            ldx         #0
            jmp         b_error

; INKEY$: the key typed, or "" (it doesn't wait)
f_inkey:
            jsr         con_raw
            bcs         @none
            LDR         r0, kbuf
            LDR         r1, 1
            lda         nbfd
            jsr         READ
            bcs         @none
            cmp         #0
            beq         @none
            lda         kbuf
            jmp         key_str
@none:
            lda         #0
            jmp         ram_str

; The key .A pushed, as INKEY$ has it: a character; a cursor or function key (KEY_UP ...), CHR$(0) and QuickBASIC's
; code for it (72 up, 80 down, 59 F1 ...)
key_str:
            cmp         #KEY_UP
            bcc         @char
            cmp         #KEY_F12 + 1
            bcs         @char
            sbc         #KEY_UP - 1                         ; (C = 0: KEY_UP less)
            tax
            lda         qb_keys,x
            sta         kbuf + 1
            stz         kbuf
            lda         #2
            bra         @push
@char:
            sta         kbuf
            lda         #1
@push:
            LDW         rs_ptr, kbuf
            jmp         ram_str
qb_keys:    .byte       72, 80, 77, 75, 71, 79, 82, 83, 73, 81, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 133, 134

; INPUT$(n): n keys waited for (stdin's bytes, with no console), a string
f_inputs:
            jsr         pop_int16                           ; (Its arguments' count: 2, a file's: to come)
            lda         t0
            cmp         #2
            bne         :+
            jmp         rt_todo6
:
            jsr         pop_int16
            lda         t0 + 1
            bne         @bad
            lda         t0
            sta         in_len
            stz         in_start
            beq         @done
            jsr         con_raw
            ldx         #0
@key:
            phx
            bcs         @stdin
            jsr         key_wait
            bra         :+
@stdin:
            FARN        1, getc_in
            bcc         :+
            lda         #ER_PASTEND
            ldx         #0
            jmp         b_error
:
            plx
            sta         lbuf,x
            inx
            cpx         in_len
            beq         @done
            lda         kstate                              ; (C: no console)
            asl         a
            bra         @key
@done:
            jmp         in_str
@bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; CSRLIN: the cursor's row (from 1): the terminal asked (DSR), its answer read (ESC [ row ; col R); 0 with no console
f_csrlin:
            lda         interactive
            beq         @zero
            jsr         con_raw
            bcs         @zero
            jsr         out_csi
            lda         #'6'
            jsr         out_byte
            lda         #'n'
            jsr         out_byte
            jsr         out_flush
            stz         t4                                  ; (The row; past the [, before the ;)
            stz         t4 + 1
@byte:
            jsr         key_wait
            cmp         #'R'
            beq         @have
            cmp         #'['
            bne         :+
            inc         t4 + 1
            bra         @byte
:
            cmp         #';'
            bne         :+
            stz         t4 + 1
            bra         @byte
:
            ldx         t4 + 1
            beq         @byte
            sec
            sbc         #'0'
            cmp         #10
            bcs         @byte
            pha
            lda         t4                                  ; (row * 10 + digit)
            asl         a
            asl         a
            adc         t4
            asl         a
            sta         t4
            pla
            adc         t4
            sta         t4
            bra         @byte
@have:
            lda         t4
            ldx         #0
            jmp         push_ax
@zero:
            lda         #0
            tax
            jmp         push_ax

; ---- The sixth bank's functions and statements (fn.inc)
fn_disp6:
            tax
            jmp         (fn_jt6,x)
fn_jt6:
            .word       f_csrlin, rt_todo6, rt_todo6, f_inkey, rt_todo6, rt_todo6, f_pos, rt_todo6, f_inputs, f_erl
            .word       f_err, f_errs
.assert     * - fn_jt6 = (F_B7 - F_B6) * 2, error, "fn_jt6 and fn.inc's F_B6 group differ"

st_disp6:
            tax
            jmp         (st_jt6,x)
st_jt6:
            .word       st_prn, st_prnl, st_przone, st_prtab, st_prspc, st_read, st_restore, st_input, st_infld
            .word       st_inend, st_lget, st_cls, st_locate, st_color, st_width, st_beep, st_sleep, st_wrval
            .word       st_wrcom
.assert     * - st_jt6 = (ST_B7 - ST_B6) * 2, error, "st_jt6 and fn.inc's ST_B6 group differ"

rt_todo6:
            lda         #73
            ldx         #0
            jmp         b_error
