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
f_num:      .res        FMAX                                ; The files open: each its number (0: the slot free), fd,
f_fd:       .res        FMAX                                ;   mode (FM_*), PRINT's column, its read buffer's bytes and
f_mode:     .res        FMAX                                ;   the next of them, its end seen
f_col:      .res        FMAX
f_blen:     .res        FMAX
f_bpos:     .res        FMAX
f_end:      .res        FMAX
f_buf:      .res        FMAX * FBSZ
fsl:        .res        1                                   ; (A file's slot)
f_osl:      .res        1                                   ; (PRINT #n's)
fo_num:     .res        1                                   ; (OPEN's)
fo_mode:    .res        1
fo_omode:   .res        1
in_src:     .res        1                                   ; INPUT's: 0 the console, else 1 + a file's slot
con_col:    .res        1                                   ; (The console's column, while a file's PRINTed)
fl_n:       .res        1                                   ; (fr_line's, gn_byte's count)
g_len:      .res        2                                   ; (GET's string's length)
g_pos:      .res        4                                   ; (SEEK's, GET's and PUT's place, from 0)
us_fmt:     .res        256                                 ; PRINT USING's format, its length, the next field's place
us_len:     .res        1
us_pos:     .res        1
us_wrap:    .res        1                                   ; (us_next's: the format's end passed)
us_kind:    .res        1                                   ; (A field's kind: US_*; its start, width)
us_start:   .res        1
us_w:       .res        1
us_lplus:   .res        1                                   ; (A number's field: a + first, ** fill, $, commas, a point,
us_star:    .res        1                                   ;   its places, a + or - after, a # before the point)
us_dollar:  .res        1
us_comma:   .res        1
us_point:   .res        1
us_dec:     .res        1
us_trail:   .res        1
us_ihash:   .res        1
us_neg:     .res        1                                   ; (The number's sign, its digits' count, the text's length,
us_ndig:    .res        1                                   ;   the integer part's, a comma's count down)
us_olen:    .res        1
us_ilen:    .res        1
us_m3:      .res        1
us_bstr:    .res        32                                  ; ({x}'s base, zero-terminated)

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
            stz         in_src
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
            lda         in_src                              ; (A file's: its next line)
            beq         :+
            dec         a
            sta         fsl
            jsr         fr_line
            stz         in_end
            ldx         #0
            bra         @skip
:
            inc         in_bad                              ; (None left)
            stz         in_len
            jmp         @push
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
            stx         in_start                            ; (Up to a , or the end, the spaces at its end dropped;
:                                                           ;   a file's number up to a space too)
            lda         lbuf,x
            beq         :+
            cmp         #','
            beq         :+
            cmp         #' '
            bne         @in
            ldy         in_src
            beq         @in
            ldy         in_type
            beq         :+
@in:
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
:
            lda         lbuf,x                              ; (The spaces after it)
            cmp         #' '
            bne         @after
            inx
            bra         :-
@after:
            lda         lbuf,x                              ; (A , after it: another; the end: none; a file's next
            bne         :+                                  ;   field; else not a field)
            inc         in_end
            bra         @have
:
            cmp         #','
            bne         :+
            inx
            bra         @have
:
            lda         in_src
            bne         @have
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
            lda         in_src                              ; (A file's: never again)
            beq         :+
            stz         in_src
            lda         #0
            jmp         push_int_a
:
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
            jsr         pop_int16                           ; (Its arguments' count: 2, a file's)
            lda         t0
            cmp         #2
            bne         :+
            jmp         f_inputs_f
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

; ---- Files: #1 to #255, FMAX open at once

; fsl = the slot of the file #t0.  OUT: C = 1 not open
f_find:
            lda         t0 + 1
            bne         @no
            lda         t0
            beq         @no
            ldx         #FMAX - 1
:
            cmp         f_num,x
            beq         @yes
            dex
            bpl         :-
@no:
            sec
            rts
@yes:
            stx         fsl
            clc
            rts

; fsl = the slot of the file whose number is taken off the stack (not open: Bad file name or number); its mode one
; of .A's (else Bad file mode)
f_need:
            pha
            jsr         pop_int16
            jsr         f_find
            pla
            bcs         @bad
            ldx         fsl
            and         f_mode,x
            beq         @mode
            rts
@bad:
            lda         #ER_BADFILE
            ldx         #0
            jmp         b_error
@mode:
            lda         #ER_MODE
            ldx         #0
            jmp         b_error

; A system error (.A) said (sys_err's)
f_syserr:
            FARN        7, sys_err

; ST_OPEN: its path, its number and its mode (FM_*) taken off the stack: opened (OUTPUT a file made or emptied;
; APPEND at its end, made if it isn't there; BINARY read and written, made if it isn't there).  Open already: File
; already open; FMAX open: Too many files
st_open:
            jsr         pop_int16
            lda         t0
            sta         fo_mode
            jsr         pop_int16
            jsr         f_find
            bcs         :+
            lda         #ER_OPEN
            ldx         #0
            jmp         b_error
:
            lda         t0 + 1                              ; (1 to 255)
            beq         @lb904
            jmp         @badnum
@lb904:
            lda         t0
            bne         @lb903
            jmp         @badnum
@lb903:
            sta         fo_num
            ldx         #FMAX - 1                           ; (A slot free)
:
            lda         f_num,x
            beq         :+
            dex
            bpl         :-
            lda         #ER_TOOMANY
            ldx         #0
            jmp         b_error
:
            stx         fsl
            FARN        7, str_lbuf
            lda         fo_mode
            ldx         #O_READ
            cmp         #FM_INPUT
            beq         @open
            ldx         #O_WRITE | O_TRUNC
            cmp         #FM_OUTPUT
            beq         @open
            ldx         #O_WRITE
            cmp         #FM_APPEND
            beq         @open
            ldx         #O_RDWR
@open:
            stx         fo_omode
            LDR         r0, lbuf
            txa
            jsr         OPEN
            bcc         @have
            cmp         #E_NOENT                            ; (Not there: made, but for INPUT)
            beq         @lb902
            jmp         f_syserr
@lb902:
            ldx         fo_mode
            cpx         #FM_INPUT
            bne         @lb901
            jmp         f_syserr
@lb901:
            LDR         r0, lbuf
            lda         fo_omode
            and         #3
            ldx         #0
            jsr         CREATE
            bcc         @lb900
            jmp         f_syserr
@lb900:
@have:
            ldx         fsl
            sta         f_fd,x
            lda         fo_num
            sta         f_num,x
            lda         fo_mode
            sta         f_mode,x
            stz         f_col,x
            stz         f_blen,x
            stz         f_bpos,x
            stz         f_end,x
            cmp         #FM_APPEND                          ; (APPEND: at its end)
            bne         @done
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         f_fd,x
            ldx         #2
            jsr         SEEK
@done:
            rts
@badnum:
            lda         #ER_BADFILE
            ldx         #0
            jmp         b_error

; ST_CLOSE: the file (its number taken off) closed
st_close:
            lda         #$FF
            jsr         f_need
            ; (fall through)
; The file in slot fsl closed
f_close:
            ldx         fsl
            stz         f_num,x
            lda         f_fd,x
            jmp         CLOSE

; ST_CLOSEALL (CLOSE; a run's start): every file closed
fl_closeall:
            ldx         #FMAX - 1
@each:
            lda         f_num,x
            beq         :+
            stx         fsl
            phx
            jsr         f_close
            plx
:
            dex
            bpl         @each
            rts

; ST_FOUT: PRINT #n's and WRITE #n's output to the file (its number taken off), PRINT's column its
st_fout:
            lda         #FM_OUTPUT | FM_APPEND | FM_BINARY
            jsr         f_need
            jsr         out_flush
            lda         ocol
            sta         con_col
            ldx         fsl
            stx         f_osl
            lda         f_col,x
            sta         ocol
            lda         f_fd,x
            sta         ofd
            rts

; ST_FEND: the output the console's again
st_fend:
            jsr         out_flush
            ldx         f_osl
            lda         ocol
            sta         f_col,x
            lda         con_col
            sta         ocol
            lda         #1
            sta         ofd
            rts

; ST_FINPUT: INPUT #n's and LINE INPUT #n's line read from the file (its number taken off), its fields INPUT's
st_finput:
            lda         #FM_INPUT | FM_BINARY
            jsr         f_need
            ldx         fsl
            inx
            stx         in_src
            jsr         fr_line
            stz         in_pos
            stz         in_bad
            stz         in_end
            rts

; A line of the file in slot fsl into lbuf (zero-terminated, its CR dropped; past 255 bytes the rest dropped).  At
; its end already: Input past end of file
fr_line:
            stz         fl_n
            stz         t4                                  ; (A byte read: t4 <> 0)
@byte:
            jsr         fr_getc
            bcs         @end
            inc         t4
            cmp         #LF
            beq         @done
            cmp         #CR
            beq         @byte
            ldy         fl_n
            cpy         #255
            bcs         @byte
            sta         lbuf,y
            inc         fl_n
            bra         @byte
@end:
            lda         t4
            bne         @done
            lda         #ER_PASTEND
            ldx         #0
            jmp         b_error
@done:
            ldy         fl_n
            lda         #0
            sta         lbuf,y
            rts

; The next byte of the file in slot fsl: .A.  OUT: C = 1 at its end.  Keeps .Y
fr_getc:
            ldx         fsl
            lda         f_bpos,x
            cmp         f_blen,x
            bcc         @have
            phy
            jsr         fr_fill
            ply
            bcs         @end
            ldx         fsl
@have:
            txa                                             ; (Its buffer: slot * FBSZ)
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            clc
            adc         f_bpos,x
            inc         f_bpos,x
            tax
            lda         f_buf,x
            clc
@end:
            rts

; The file in slot fsl's buffer filled.  OUT: C = 1 at its end
fr_fill:
            ldx         fsl
            lda         f_end,x
            bne         @end
            txa
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            clc
            adc         #<f_buf
            sta         r0
            lda         #>f_buf
            adc         #0
            sta         r0 + 1
            LDR         r1, FBSZ
            lda         f_fd,x
            jsr         READ
            bcc         :+
            jmp         f_syserr
:
            ldx         fsl
            stz         f_bpos,x
            sta         f_blen,x
            cmp         #0
            bne         @ok
            lda         #1
            sta         f_end,x
@end:
            sec
            rts
@ok:
            clc
            rts

; INPUT$(k, #n): k bytes of the file
f_inputs_f:
            lda         #FM_INPUT | FM_BINARY
            jsr         f_need
            jsr         pop_int16
            lda         t0 + 1
            beq         :+
            jmp         illegal6
:
            lda         t0
            sta         in_len
            stz         in_start
            ldy         #0
@byte:
            cpy         in_len
            beq         @done
            jsr         fr_getc
            bcc         :+
            lda         #ER_PASTEND
            ldx         #0
            jmp         b_error
:
            sta         lbuf,y
            iny
            bra         @byte
@done:
            jmp         in_str

; EOF(n): -1 at the file's end (an output file always), else 0
f_eof:
            lda         #$FF
            jsr         f_need
            ldx         fsl
            lda         f_mode,x
            cmp         #FM_BINARY
            beq         @bin
            and         #FM_INPUT
            beq         @true
            lda         f_bpos,x
            cmp         f_blen,x
            bcc         @false
            jsr         fr_fill
            bcs         @true
@false:
            lda         #0
            jmp         push_int_a
@true:
            lda         #$FF
            jmp         push_int_a
@bin:
            jsr         f_where                             ; (Its place at or past its length)
            jsr         f_length
            lda         t0
            cmp         t4
            lda         t0 + 1
            sbc         t4 + 1
            lda         t0 + 2
            sbc         t4 + 2
            lda         t0 + 3
            sbc         t4 + 3
            bcs         @true
            bra         @false

; t0-t3 = the file's (slot fsl's) place: its fd's, less what's in its buffer not read yet
f_where:
            ldx         fsl
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         f_fd,x
            ldx         #1
            jsr         SEEK
            bcc         :+
            jmp         f_syserr
:
            ldx         fsl
            sec
            lda         f_blen,x
            sbc         f_bpos,x
            sta         t4
            sec
            lda         r0
            sbc         t4
            sta         t0
            lda         r0 + 1
            sbc         #0
            sta         t0 + 1
            lda         r1
            sbc         #0
            sta         t0 + 2
            lda         r1 + 1
            sbc         #0
            sta         t0 + 3
            rts

; t4-t7 = the file's length (FSTAT's)
f_length:
            ldx         fsl
            LDR         r0, sr_buf
            lda         f_fd,x
            jsr         FSTAT
            bcc         :+
            jmp         f_syserr
:
            ldx         #3
:
            lda         sr_buf + SR_LENGTH,x
            sta         t4,x
            dex
            bpl         :-
            rts

; t0-t3 pushed, an integer
push_t0:
            lda         #VT_INT
            sta         (vsp)
            ldy         #4
:
            lda         t0 - 1,y
            sta         (vsp),y
            dey
            bne         :-
            jmp         vsp_up

; LOF(n): the file's length
f_lof:
            lda         #$FF
            jsr         f_need
            jsr         f_length
            ldx         #3
:
            lda         t4,x
            sta         t0,x
            dex
            bpl         :-
            bra         push_t0

; LOC(n): the bytes before its place; SEEK(n): its place (from 1)
f_loc:
            lda         #$FF
            jsr         f_need
            jsr         f_where
            bra         push_t0
f_seek:
            lda         #$FF
            jsr         f_need
            jsr         f_where
            inc         t0
            bne         push_t0
            inc         t0 + 1
            bne         push_t0
            inc         t0 + 2
            bne         push_t0
            inc         t0 + 3
            bra         push_t0

; FREEFILE: the first number (1 on) not open
f_freefile:
            stz         t0 + 1
            lda         #1
            sta         t0
:
            jsr         f_find
            bcs         :+
            inc         t0
            bne         :-
:
            lda         t0
            jmp         push_int_a1

; .A (0-255) pushed, an integer
push_int_a1:
            ldx         #0
            jmp         push_ax

; t0-t3 = the integer taken off the stack (32 bits; a number rounded)
pop_int32:
            jsr         top_vt
            lda         (vt)
            beq         :+
            FARN        4, num_to_int
:
            jsr         pop_vt
            ldy         #4
:
            lda         (vt),y
            sta         t0 - 1,y
            dey
            bne         :-
            rts

; ST_SEEK: the file's place (pos from 1, taken off the stack, then the file's number)
st_seek:
            jsr         pop_int32
            jsr         pos_from0
            bcs         illegal6
            lda         #$FF
            jsr         f_need
            bra         f_goto

; g_pos = t0-t3 less 1 (a place from 1, made from 0).  OUT: C = 1 it was below 1
pos_from0:
            lda         t0 + 3
            bmi         @bad
            ora         t0 + 2
            ora         t0 + 1
            ora         t0
            beq         @bad
            sec
            lda         t0
            sbc         #1
            sta         g_pos
            lda         t0 + 1
            sbc         #0
            sta         g_pos + 1
            lda         t0 + 2
            sbc         #0
            sta         g_pos + 2
            lda         t0 + 3
            sbc         #0
            sta         g_pos + 3
            clc
            rts
@bad:
            sec
            rts

; The file (slot fsl) at the place g_pos (from 0), its buffer emptied
f_goto:
            lda         g_pos
            sta         r0
            lda         g_pos + 1
            sta         r0 + 1
            lda         g_pos + 2
            sta         r1
            lda         g_pos + 3
            sta         r1 + 1
            ldx         fsl
            stz         f_blen,x
            stz         f_bpos,x
            stz         f_end,x
            lda         f_fd,x
            ldx         #0
            jsr         SEEK
            bcc         :+
            jmp         f_syserr
:
            rts

illegal6:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; ST_GSET: GET's and PUT's file (BINARY) and place (from 1; below 1: where it is), taken off the stack
st_gset:
            jsr         pop_int32
            jsr         pos_from0
            php
            lda         #FM_BINARY
            jsr         f_need
            plp
            bcs         :+
            jmp         f_goto
:
            rts

; ST_GET: the variable's value on the stack (a string: its length's worth read; a number: the stored format's bytes,
; as CVN reads them) replaced by what's read from GET's file
st_get:
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            beq         @str
            jsr         pop_vt                              ; (A number: its bytes, by their tags)
            stz         fl_n
            jsr         gn_number
            LDW         rs_ptr, lbuf
            lda         fl_n
            jsr         ram_str
            FARN        4, f_cvn
            rts
@str:
            FARN        5, str_get_a                        ; (Its length)
            MOVW        g_len, sla
            jsr         pop_vt
            lda         g_len
            ora         g_len + 1
            bne         :+
            lda         #0
            jmp         ram_str
:
            lda         g_len
            ldx         g_len + 1
            FARN        5, heap_str
            MOVW        cn, g_len
@chunk:
            lda         cn + 1                              ; (128 at a time, to the string's bytes)
            bne         :+
            lda         cn
            cmp         #128
            bcc         :++
:
            lda         #128
:
            sta         r1
            stz         r1 + 1
            sta         t4
            LDR         r0, lbuf
            ldx         fsl
            lda         f_fd,x
            jsr         READ
            bcc         :+
            jmp         f_syserr
:
            cmp         #0                                  ; (Its end: the rest zeros)
            bne         :+
            lda         #0
            ldy         #0
@zero:
            sta         lbuf,y
            iny
            cpy         t4
            bne         @zero
            lda         t4
:
            sta         t4 + 1
            jsr         cb_sel
            ldy         #0
:
            lda         lbuf,y
            jsr         cb_wr
            iny
            cpy         t4 + 1
            bne         :-
            sec
            lda         cn
            sbc         t4 + 1
            sta         cn
            bcs         :+
            dec         cn + 1
:
            lda         cn
            ora         cn + 1
            bne         @chunk
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

; A number's bytes read from GET's file into lbuf (fl_n), by its tags (NUMBERS.md's stored format)
gn_number:
            jsr         gn_byte
            cmp         #$C2                                ; (A complex number: two real ones)
            bne         gn_real
            jsr         gn_byte
            jsr         gn_real
            jsr         gn_byte
gn_real:
            cmp         #$C1                                ; (A rational: two integers)
            bne         :+
            jsr         gn_int
            jmp         gn_int
:
            cmp         #$C0                                ; (A fixed decimal: its places, its digits)
            bne         :+
            jsr         gn_byte
            jsr         gn_byte
            jmp         gn_int
:
            cmp         #$B0
            bcc         gn_int_t
            cmp         #$C0
            bcs         @bad
            jmp         gn_int
@bad:
            jmp         illegal6
gn_int:
            jsr         gn_byte
gn_int_t:
            cmp         #$80
            bcc         @done
            cmp         #$A0
            bcs         @long
            and         #$0F
            tax
            inx
@bytes:
            jsr         gn_byte
            dex
            bne         @bytes
@done:
            rts
@long:
            cmp         #$A2
            bcc         :+
            jmp         illegal6
:
            jsr         gn_byte
            tax
            beq         @done
            bra         @bytes

; The file's next byte (GET's, its fd's own: no buffer) to lbuf (fl_n on).  Its end: Input past end of file; past
; 255: Illegal function call.  Keeps .X
gn_byte:
            phx
            LDR         r0, kbuf
            LDR         r1, 1
            ldx         fsl
            lda         f_fd,x
            jsr         READ
            bcc         :+
            jmp         f_syserr
:
            cmp         #0
            bne         :+
            lda         #ER_PASTEND
            ldx         #0
            jmp         b_error
:
            ldy         fl_n
            cpy         #255
            bcc         :+
            jmp         illegal6
:
            lda         kbuf
            sta         lbuf,y
            inc         fl_n
            plx
            rts

; ST_PUT: the value taken off the stack written to PUT's file (a string's bytes; a number's in the stored format)
st_put:
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            beq         :+
            FARN        4, f_mkn                            ; (A number: its bytes, a string)
:
            jsr         top_vt
            FARN        5, str_get_a
            MOVW        cn, sla
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
@chunk:
            lda         cn
            ora         cn + 1
            beq         @done
            lda         cn + 1                              ; (128 at a time, through lbuf)
            bne         :+
            lda         cn
            cmp         #128
            bcc         :++
:
            lda         #128
:
            sta         t4
            jsr         ca_sel
            ldy         #0
:
            jsr         ca_rd
            sta         lbuf,y
            iny
            cpy         t4
            bne         :-
            jsr         code_sel
            LDR         r0, lbuf
            lda         t4
            sta         r1
            stz         r1 + 1
            ldx         fsl
            lda         f_fd,x
            jsr         WRITE
            bcc         :+
            jmp         f_syserr
:
            sec
            lda         cn
            sbc         t4
            sta         cn
            bcs         @chunk
            dec         cn + 1
            bra         @chunk
@done:
            jmp         pop_vt

; ---- Files and directories by their names: KILL, NAME, MKDIR, RMDIR, CHDIR, FILES

; ST_KILL, ST_RMDIR: the file (or the empty directory) removed
st_kill:
st_rmdir:
            FARN        7, str_lbuf
            LDR         r0, lbuf
            jsr         REMOVE
            bcc         :+
            jmp         f_syserr
:
            rts

; ST_MKDIR: a directory made
st_mkdir:
            FARN        7, str_lbuf
            LDR         r0, lbuf
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcc         :+
            jmp         f_syserr
:
            jmp         CLOSE

; ST_CHDIR: the current directory
st_chdir:
            FARN        7, str_lbuf
            LDR         r0, lbuf
            jsr         CHDIR
            bcc         :+
            jmp         f_syserr
:
            rts

; ST_NAME: the file (old, under new on the stack) renamed: its name new's last part (a stat record: all else $FF)
st_name:
            FARN        7, str_lbuf                         ; (new)
            ldx         #SR_SIZE - 1
            lda         #$FF
:
            sta         sr_buf,x
            dex
            bpl         :-
            ldy         #0                                  ; (Its last part: after its last /)
            ldx         #0
:
            lda         lbuf,y
            beq         :+
            iny
            cmp         #'/'
            bne         :-
            tya
            tax
            bra         :-
:
            ldy         #0
:
            lda         lbuf,x
            sta         sr_buf + SR_NAME,y
            beq         :+
            inx
            iny
            cpy         #31
            bcc         :-
            lda         #0
            sta         sr_buf + SR_NAME,y
:
            FARN        7, str_lbuf                         ; (old)
            LDR         r0, lbuf
            LDR         r1, sr_buf
            jsr         WSTAT
            bcc         :+
            jmp         f_syserr
:
            rts

; ST_FILES: a directory's names (the string taken off; "": the current one), PRINT's zones apart (a directory's with /)
st_files:
            FARN        7, str_lbuf
            cmp         #0
            bne         :+
            lda         #'.'
            sta         lbuf
            stz         lbuf + 1
:
            LDR         r0, lbuf
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         f_syserr
:
            sta         fo_num
@rec:
            LDR         r0, sr_buf
            LDR         r1, SR_SIZE
            lda         fo_num
            jsr         READ
            bcs         @end
            cmp         #0
            beq         @end
            lda         ocol                                ; (A zone each, a line's end past the width)
            beq         @name
            clc
            adc         #16
            cmp         owidth
            bcc         :+
            lda         #LF
            jsr         pr_ch
            bra         @name
:
            lda         #' '
            jsr         pr_ch
            lda         ocol
            and         #15
            bne         :-
@name:
            ldx         #0
:
            lda         sr_buf + SR_NAME,x
            beq         :+
            jsr         pr_ch
            inx
            cpx         #32
            bcc         :-
:
            lda         sr_buf + SR_MODE + 1
            and         #DM_DIR
            beq         @rec
            lda         #'/'
            jsr         pr_ch
            bra         @rec
@end:
            lda         ocol
            beq         :+
            lda         #LF
            jsr         pr_ch
:
            lda         fo_num
            jmp         CLOSE

; ---- PRINT USING

US_NUM          = 0             ; A field's kinds: a number's picture
US_FIRST        = 1             ; ! (a string's first character)
US_ALL          = 2             ; & (all of it)
US_SOME         = 3             ; \  \ (its first us_w characters)
US_BRACE        = 4             ; {} or {base}
US_DIG          = lbuf + 256    ; (A number's digits)
US_OUT          = lbuf          ; (Its field's text)

; ST_USTART: the format (the string taken off) kept, from its start
st_ustart:
            FARN        7, str_lbuf
            sta         us_len
            tax
            beq         :++
:
            lda         lbuf - 1,x
            sta         us_fmt - 1,x
            dex
            bne         :-
:
            stz         us_pos
            rts

; ST_UEND: the format's text after the last item's field written, to its next field or its end
st_uend:
            lda         us_pos
            cmp         us_len
            bcs         :+
            jsr         us_field
            bcs         st_uend
:
            rts

; ST_UITEM: the value taken off written by the format's next field (its text before it written first, the format
; begun again past its end).  A format with no field: Illegal function call
st_uitem:
            stz         us_wrap
@char:
            lda         us_pos
            cmp         us_len
            bcc         :+
            lda         us_wrap
            bne         @none
            inc         us_wrap
            stz         us_pos
            bra         @char
:
            jsr         us_field
            bcs         @char
            sta         us_kind
            cmp         #US_NUM
            bne         :+
            jmp         us_number
:
            cmp         #US_BRACE
            bne         :+
            jmp         us_brace
:
            jmp         us_string
@none:
            jmp         illegal6

; At the format's us_pos: a field (C = 0: .A its kind, us_w its width for \  \), or its text (C = 1: written, us_pos
; past it; _ and the character after it: that character)
us_field:
            ldx         us_pos
            lda         us_fmt,x
            cmp         #'_'
            bne         @bang
            inx
            cpx         us_len
            bcc         @lb905
            jmp         @lit1
@lb905:
            lda         us_fmt,x
            jmp         @lit
@bang:
            cmp         #'!'
            bne         :+
            lda         #US_FIRST
            clc
            rts
:
            cmp         #'&'
            bne         :+
            lda         #US_ALL
            clc
            rts
:
            cmp         #'\'
            bne         @brace
            ldy         #2                                  ; (\, spaces, \: their count its width)
            phx
:
            inx
            cpx         us_len
            bcs         @nosome
            lda         us_fmt,x
            cmp         #'\'
            beq         :+
            cmp         #' '
            bne         @nosome
            iny
            bra         :-
:
            plx
            sty         us_w
            lda         #US_SOME
            clc
            rts
@nosome:
            plx
            lda         #'\'
            bra         @lit
@brace:
            cmp         #'{'
            bne         @num
            phx
:
            inx
            cpx         us_len
            bcs         :+
            lda         us_fmt,x
            cmp         #'}'
            bne         :-
            plx
            lda         #US_BRACE
            clc
            rts
:
            plx
            lda         #'{'
            bra         @lit
@num:
            cmp         #'#'
            beq         @isnum
            ldy         #0                                  ; (. # , + # , + . , + $$ , + ** , $$ , **: a number's)
            cmp         #'.'
            bne         :+
            ldy         #'#'
:
            cmp         #'+'
            bne         :+
            ldy         #'+'
:
            cmp         #'$'
            bne         :+
            ldy         #'$'
:
            cmp         #'*'
            bne         :+
            ldy         #'*'
:
            cpy         #0
            beq         @lit
            inx                                             ; (Its next)
            cpx         us_len
            bcs         @lit0
            lda         us_fmt,x
            dex
            cpy         #'+'
            beq         @plus
            cpy         #'#'
            bne         :+
            cmp         #'#'
            beq         @isnum
            bra         @lit0
:
            sty         us_m3                               ; ($$ and **: the same twice)
            cmp         us_m3
            beq         @isnum
            bra         @lit0
@plus:
            cmp         #'#'
            beq         @isnum
            cmp         #'.'
            beq         @isnum
            cmp         #'$'
            beq         @isnum
            cmp         #'*'
            beq         @isnum
@lit0:
            ldx         us_pos
            lda         us_fmt,x
@lit:
            jsr         pr_ch
@lit1:
            inx
            stx         us_pos
            sec
            rts
@isnum:
            lda         #US_NUM
            clc
            rts

; A string's field: the top value written (!, &, \  \), taken off; a number: Type mismatch
us_string:
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            beq         :+
            lda         #ER_TYPE
            ldx         #0
            jmp         b_error
:
            lda         us_kind
            cmp         #US_ALL
            bne         us_part
            inc         us_pos
            ; (fall through)

; The top string written whole, taken off
us_whole:
            jsr         top_vt
            FARN        5, str_get_a
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            MOVW        t0, sla
            jsr         pr_far
            jmp         pop_vt

; ! and its kind's width: the string's first us_w characters, padded with spaces
us_part:
            jsr         top_vt
            FARN        5, str_get_a
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            lda         us_kind
            ldx         #1                                ; (!: one; \  \: us_w)
            cmp         #US_FIRST
            beq         :+
            ldx         us_w
:
            stx         us_w
            clc
            txa
            adc         us_pos
            sta         us_pos
            ldy         us_w                                ; (Its characters, then spaces to the width)
            jsr         ca_sel
@ch:
            lda         sla
            ora         sla + 1
            beq         @pad
            jsr         ca_rd
            jsr         pr_ch
            lda         sla
            bne         :+
            dec         sla + 1
:
            dec         sla
            dey
            bne         @ch
            bra         @done
@pad:
            lda         #' '
            jsr         pr_ch
            dey
            bne         @pad
@done:
            jsr         code_sel
            jmp         pop_vt

; {} or {base}: the top value written (a number in the base, or in base; a string as it is), taken off
us_brace:
            ldx         us_pos                              ; (Its base: between the braces)
            ldy         #0
:
            inx
            lda         us_fmt,x
            cmp         #'}'
            beq         :+
            sta         us_bstr,y
            iny
            cpy         #31
            bcc         :-
:
            lda         #0
            sta         us_bstr,y
            inx
            stx         us_pos
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            bne         :+
            jmp         us_whole
:
            stz         nt_base
            stz         nt_base + 1
            cpy         #0
            beq         :+
            LDW         nt_base, us_bstr
:
            FARN        4, num_text
            sta         t0
            stx         t0 + 1
            stz         nt_base
            stz         nt_base + 1
            LDW         ca, $8000
            lda         #LB_TXB
            sta         cal
            jsr         pr_far
            jmp         pop_vt

; A number's field: its picture read (us_*), the top value rounded to its places (a half away from 0), written in it
; (wider than it: % first), taken off; a string: Type mismatch
us_number:
            jsr         us_pic
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            bne         :+
            lda         #ER_TYPE
            ldx         #0
            jmp         b_error
:
            FARN        4, num_sign_vt
            sta         us_neg
            FARN        4, f_abs
            lda         us_dec                              ; (* 10 ^ places)
            beq         :+
            lda         #10
            ldx         #0
            jsr         push_ax
            lda         us_dec
            ldx         #0
            jsr         push_ax
            lda         #OP_POW
            FARN        4, num_op
            lda         #OP_MUL
            FARN        4, num_op
:
            lda         #2                                  ; (Rounded: FIX((2y + 1) / 2))
            jsr         us_op_n
            lda         #OP_MUL
            FARN        4, num_op
            lda         #1
            jsr         us_op_n
            lda         #OP_ADD
            FARN        4, num_op
            lda         #2
            jsr         us_op_n
            lda         #OP_DIV
            FARN        4, num_op
            FARN        4, f_fix
            jsr         top_vt                              ; (Its digits, in decimal: US_DIG)
            lda         #'d'                                ; (Decimal: a base string in the RAM)
            sta         us_bstr
            stz         us_bstr + 1
            LDW         nt_base, us_bstr
            FARN        4, num_text
            stz         nt_base
            stz         nt_base + 1
            cpx         #0
            beq         :+
            lda         #255
:
            sta         us_ndig
            LDW         ca, $8000
            lda         #LB_TXB
            sta         cal
            jsr         ca_sel
            ldx         #0
:
            cpx         us_ndig
            beq         :+
            jsr         ca_rd
            sta         US_DIG,x
            inx
            bra         :-
:
            jsr         code_sel
            jsr         pop_vt
@pad0:
            lda         us_dec                              ; (At least places + 1 digits: 0s first)
            cmp         us_ndig
            bcc         :++
            ldx         us_ndig
:
            lda         US_DIG - 1,x
            sta         US_DIG,x
            dex
            bne         :-
            lda         #'0'
            sta         US_DIG
            inc         us_ndig
            bra         @pad0
:
            sec
            lda         us_ndig
            sbc         us_dec
            sta         us_ilen
            stz         us_olen                             ; (The text: US_OUT)
            lda         us_lplus                            ; (Its sign first: +'s, or a -)
            beq         :+
            lda         #'+'
            ldx         us_neg
            bpl         @sign
            lda         #'-'
            bra         @sign
:
            lda         us_trail
            bne         @nosign
            lda         us_neg
            bpl         @nosign
            lda         #'-'
@sign:
            jsr         us_put
@nosign:
            lda         us_dollar
            beq         :+
            lda         #'$'
            jsr         us_put
:
            lda         us_ilen                             ; (The integer part: none for .## and 0)
            cmp         #1
            bne         @int
            lda         US_DIG
            cmp         #'0'
            bne         @int
            lda         us_ihash
            beq         @frac
@int:
            lda         us_ilen                             ; (Its commas: us_m3 the digits to the next)
:
            cmp         #4
            bcc         :+
            sbc         #3
            bra         :-
:
            sta         us_m3
            ldx         #0
@digit:
            lda         US_DIG,x
            jsr         us_put
            inx
            cpx         us_ilen
            beq         @frac
            dec         us_m3
            bne         @digit
            lda         #3
            sta         us_m3
            lda         us_comma
            beq         @digit
            lda         #','
            jsr         us_put
            bra         @digit
@frac:
            lda         us_point
            beq         @trail
            lda         #'.'
            jsr         us_put
            ldx         us_ilen
:
            cpx         us_ndig
            beq         @trail
            lda         US_DIG,x
            jsr         us_put
            inx
            bra         :-
@trail:
            lda         us_trail                            ; (A sign after it)
            beq         @out
            cmp         #'+'
            bne         :+
            lda         #'+'
            ldx         us_neg
            bpl         @t
            lda         #'-'
            bra         @t
:
            lda         #' '
            ldx         us_neg
            bpl         @t
            lda         #'-'
@t:
            jsr         us_put
@out:
            lda         us_olen                             ; (Wider than the field: % first; else padded to it)
            cmp         us_w
            beq         @text
            bcc         :+
            lda         #'%'
            jsr         pr_ch
            bra         @text
:
            sec
            lda         us_w
            sbc         us_olen
            tax
:
            lda         #' '
            ldy         us_star
            beq         :+
            lda         #'*'
:
            jsr         pr_ch
            dex
            bne         :--
@text:
            ldx         #0
:
            cpx         us_olen
            beq         :+
            lda         US_OUT,x
            jsr         pr_ch
            inx
            bra         :-
:
            rts

; .A (0-255) pushed, an integer (us_number's operands)
us_op_n:
            ldx         #0
            jmp         push_ax

; .A to US_OUT (255 at most).  Keeps .X
us_put:
            ldy         us_olen
            cpy         #255
            bcs         :+
            sta         US_OUT,y
            inc         us_olen
:
            rts

; A number's picture read at us_pos (us_pos past it): [+] [** | $$ | **$] #s and ,s [. #s] [+ | -]; us_w its width
us_pic:
            ldx         #us_ihash - us_lplus                ; (Its flags: none yet)
:
            stz         us_lplus,x
            dex
            bpl         :-
            ldx         us_pos
            stx         us_start
            lda         us_fmt,x
            cmp         #'+'
            bne         :+
            inc         us_lplus
            inx
:
            jsr         us_pair                             ; (** or $$)
            cmp         #'*'
            bne         :+
            inc         us_star
            inx
            inx
            inc         us_ihash
            jsr         us_ch
            cmp         #'$'
            bne         @int
            inc         us_dollar
            inx
            bra         @int
:
            cmp         #'$'
            bne         @int
            inc         us_dollar
            inx
            inx
            inc         us_ihash
@int:
            jsr         us_ch
            cmp         #'#'
            bne         :+
            inc         us_ihash
            inx
            bra         @int
:
            cmp         #','
            bne         :+
            inc         us_comma
            inx
            bra         @int
:
            cmp         #'.'
            bne         @sign
            inc         us_point
            inx
@dec:
            jsr         us_ch
            cmp         #'#'
            bne         @sign
            inc         us_dec
            inx
            bra         @dec
@sign:
            lda         us_lplus
            bne         @end
            jsr         us_ch
            cmp         #'+'
            beq         :+
            cmp         #'-'
            bne         @end
:
            sta         us_trail
            inx
@end:
            stx         us_pos
            txa
            sec
            sbc         us_start
            sta         us_w
            rts

; .A = the format's character at .X (0 past its end)
us_ch:
            lda         #0
            cpx         us_len
            bcs         :+
            lda         us_fmt,x
:
            rts

; .A = the character at .X if the one after it is the same (** or $$), else 0
us_pair:
            jsr         us_ch
            beq         :+
            inx
            pha
            jsr         us_ch
            sta         us_m3
            pla
            dex
            cmp         us_m3
            beq         :+
            lda         #0
:
            rts

; ---- The sixth bank's functions and statements (fn.inc)
fn_disp6:
            tax
            jmp         (fn_jt6,x)
fn_jt6:
            .word       f_csrlin, f_eof, f_freefile, f_inkey, f_loc, f_lof, f_pos, f_seek, f_inputs, f_erl
            .word       f_err, f_errs
.assert     * - fn_jt6 = (F_B7 - F_B6) * 2, error, "fn_jt6 and fn.inc's F_B6 group differ"

st_disp6:
            tax
            jmp         (st_jt6,x)
st_jt6:
            .word       st_prn, st_prnl, st_przone, st_prtab, st_prspc, st_read, st_restore, st_input, st_infld
            .word       st_inend, st_lget, st_cls, st_locate, st_color, st_width, st_beep, st_sleep, st_wrval
            .word       st_wrcom, st_open, st_close, fl_closeall, st_fout, st_fend, st_finput, st_seek, st_gset
            .word       st_get, st_put, st_kill, st_name, st_mkdir, st_rmdir, st_chdir, st_files, st_ustart, st_uitem
            .word       st_uend
.assert     * - st_jt6 = (ST_B7 - ST_B6) * 2, error, "st_jt6 and fn.inc's ST_B6 group differ"

rt_todo6:
            lda         #73
            ldx         #0
            jmp         b_error
