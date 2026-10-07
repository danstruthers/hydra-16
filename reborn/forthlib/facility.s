; ****************************************************************************
; facility.s - HyForth's Facility library (/lib/forth/facility.fl): key?, ms, time&date, at-xy and page (the
; terminal's ANSI sequences), the structures (begin-structure ... +field), and the Facility Extension's keys (ekey,
; k-up ...); and the terminal's other sequences, as docs/hyforth.md has them (clear-line, the cursor's, form, colours
; and attributes, beep).  key? reads the console's cons non-blocking, in raw mode (till a line's read: the core's
; raw_off), and leaves its key for key (key_pend).

.include "forthlib.inc"

.bss
kq_fd:      .res        1                                   ; KEY?'s fd (/dev/cons, non-blocking), $FF: not open
en_buf:     .res        4                                   ; FORM: $LINES's or $COLUMNS's value ...
FS_BUF      = 96
fs_buf:     .res        FS_BUF                              ;   consctl's text ...
fs_cols:    .res        1                                   ;   and its size line's columns and rows
fs_rows:    .res        1
.code

; Its start: KEY?'s fd not open yet
lib_init:
            lda         #$FF
            sta         kq_fd
            rts

            HEADER      "key?", 0
keyq:                                                       ; ( -- flag ): a key waiting (the console in raw mode,
            lda         key_pend                            ;   till a line's read); not the console: true
            bne         @yes
            lda         interactive
            beq         @yes
            jsr         flush
            jsr         raw_on
            stx         xsave
            lda         kq_fd
            bpl         :+
            LDR         r0, s_cons
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            bcs         @no_x
            sta         kq_fd
:
            LDR         r0, key_char
            LDR         r1, 1
            lda         kq_fd
            jsr         READ
            ldx         xsave
            bcs         @no
            cmp         #0
            beq         @no
            inc         key_pend
@yes:
            dex
            jmp         true_tos
@no_x:
            ldx         xsave
@no:
            dex
            jmp         zero_tos

s_cons:     .byte       "/dev/cons", 0

            HEADER      "ms", 0
ms:                                                         ; ( u -- ): u milliseconds (in ticks: 5 ms each, at least
            lda         dlo,x                               ;   that long), the output out first
            sta         numacc
            lda         dhi,x
            sta         numacc + 1
            stz         numacc + 2
            stz         numacc + 3
            inx
            clc
            lda         numacc
            adc         #<(1000 / TICK_HZ - 1)
            sta         numacc
            bcc         :+
            inc         numacc + 1
            bne         :+
            inc         numacc + 2
:
            lda         #1000 / TICK_HZ
            jsr         div32_8
            jsr         flush
            stx         xsave
            lda         numacc
            ldx         numacc + 1
            jsr         SLEEP
            ldx         xsave
            bcc         :+
            cmp         #E_INTR
            bne         :+
            jsr         intr_wait
:
            rts

; numacc (32 bits) / .A (8 bits): numacc the quotient, .A the remainder.  Keeps .X
div32_8:
            sta         cnt
            lda         #0
            ldy         #32
@bit:
            asl         numacc
            rol         numacc + 1
            rol         numacc + 2
            rol         numacc + 3
            rol         a
            bcs         @sub
            cmp         cnt
            bcc         @next
@sub:
            sbc         cnt
            inc         numacc
@next:
            dey
            bne         @bit
            rts

            HEADER      "time&date", 0
timedate:                                                   ; ( -- +n1 +n2 +n3 +n4 +n5 +n6 ): the second, minute,
            stx         xsave                               ;   hour, day, month and year (the clock's: 2000 on)
            jsr         TIME
            ldx         xsave
            ldy         #3
:
            lda         r0,y
            sta         numacc,y
            dey
            bpl         :-
            lda         #60
            jsr         @part
            lda         #60
            jsr         @part
            lda         #24
            jsr         @part
            lda         #<2000                              ; The year: tmp (numacc: the days into it)
            sta         tmp
            lda         #>2000
            sta         tmp + 1
@year:
            jsr         @leap                               ; (cnt: 1 in a leap year)
            lda         numacc                              ; Fewer days than it has?
            cmp         #<365
            lda         numacc + 1
            sbc         #>365
            bcc         @month
            lda         numacc + 1
            cmp         #>365
            bne         :+
            lda         numacc
            cmp         #<365
            bne         :+
            lda         cnt                                 ; (365: the leap year's last day)
            bne         @month
:
            sec
            lda         numacc
            sbc         #<365
            sta         numacc
            lda         numacc + 1
            sbc         #>365
            sta         numacc + 1
            sec
            lda         numacc
            sbc         cnt
            sta         numacc
            bcs         :+
            dec         numacc + 1
:
            inc         tmp
            bne         @year
            inc         tmp + 1
            bra         @year
@month:
            ldy         #0                                  ; The month (.Y), from the days into the year
@mon:
            lda         month_days,y
            cpy         #1
            bne         :+
            clc
            adc         cnt
:
            sta         tmp2
            lda         numacc + 1
            bne         :+
            lda         numacc
            cmp         tmp2
            bcc         @found
:
            sec
            lda         numacc
            sbc         tmp2
            sta         numacc
            bcs         :+
            dec         numacc + 1
:
            iny
            bra         @mon
@found:
            inc                                             ; The day, the month, the year
            phy
            ldy         #0
            PUSHAY
            pla
            inc
            ldy         #0
            PUSHAY
            lda         tmp
            ldy         tmp + 1
            PUSHAY
            rts
@part:                                                      ; numacc / .A: the remainder pushed
            jsr         div32_8
            ldy         #0
            PUSHAY
            rts
@leap:                                                      ; cnt = 1 if tmp is a leap year (2100 isn't)
            stz         cnt
            lda         tmp
            and         #3
            bne         :+
            lda         tmp
            cmp         #<2100
            bne         @is
            lda         tmp + 1
            cmp         #>2100
            beq         :+
@is:
            inc         cnt
:
            rts

month_days: .byte       31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31

            HEADER      "at-xy", 0
atxy:                                                       ; ( u1 u2 -- ): the cursor to column u1, row u2 (from 0)
            jsr         esc_csi
            jsr         oneplus
            jsr         dec_out
            lda         #';'
            jsr         emit_a
            jsr         oneplus
            jsr         dec_out
            lda         #'H'
            jmp         emit_a

            HEADER      "page", 0
page:                                                       ; The screen cleared, the cursor at its top left
            jsr         esc_csi
            lda         #'2'
            jsr         emit_a
            lda         #'J'
            jsr         emit_a
            jsr         esc_csi
            lda         #'H'
            jmp         emit_a

; ESC [ out
esc_csi:
            lda         #27
            jsr         emit_a
            lda         #'['
            jmp         emit_a

; ( u -- ): u out in decimal
dec_out:
            lda         base
            pha
            lda         #10
            sta         base
            jsr         u_text
            jsr         type
            pla
            sta         base
            rts

; ---- The terminal's other sequences (docs/hyforth.md: hylang's screen.hl's names, conio's colours)

            HEADER      "clear-line", 0
clearline:                                                  ; ( -- ): the rest of the line cleared (CSI K)
            lda         #'K'
            bra         csi_a

            HEADER      "clear-below", 0
clearbelow:                                                 ; ( -- ): the rest of the screen cleared (CSI J)
            lda         #'J'
csi_a:                                                      ; (CSI .A)
            pha
            jsr         esc_csi
            pla
            jmp         emit_a

            HEADER      "cursor-up", 0
cursorup:                                                   ; ( n -- ): the cursor n lines up (CSI n A; 0: not moved)
            lda         #'A'
            bra         csi_move

            HEADER      "cursor-down", 0
cursordown:                                                 ; ( n -- ): n lines down (CSI n B)
            lda         #'B'
            bra         csi_move

            HEADER      "cursor-right", 0
cursorright:                                                ; ( n -- ): n columns right (CSI n C)
            lda         #'C'
            bra         csi_move

            HEADER      "cursor-left", 0
cursorleft:                                                 ; ( n -- ): n columns left (CSI n D)
            lda         #'D'
csi_move:                                                   ; (CSI n .A, or nothing for 0: the terminal's 0 is 1)
            pha
            lda         dlo,x
            ora         dhi,x
            bne         :+
            pla
            inx
            rts
:
            jsr         esc_csi
            jsr         dec_out
            pla
            jmp         emit_a

            HEADER      "cursor-save", 0
cursorsave:                                                 ; ( -- ): the cursor's place (and attributes) kept (ESC 7)
            lda         #'7'
            bra         esc_a

            HEADER      "cursor-restore", 0
cursorrestore:                                              ; ( -- ): back to it (ESC 8)
            lda         #'8'
esc_a:                                                      ; (ESC .A)
            pha
            lda         #27
            jsr         emit_a
            pla
            jmp         emit_a

            HEADER      "cursor-off", 0
cursoroff:                                                  ; ( -- ): the cursor hidden (CSI ?25l)
            lda         #'l'
            bra         cursor_25

            HEADER      "cursor-on", 0
cursoron:                                                   ; ( -- ): shown (CSI ?25h)
            lda         #'h'
cursor_25:
            pha
            jsr         esc_csi
            lda         #'?'
            jsr         emit_a
            lda         #'2'
            jsr         emit_a
            lda         #'5'
            jsr         emit_a
            pla
            jmp         emit_a

            HEADER      "form", 0
form:                                                       ; ( -- rows cols ): the window's size, its consctl's size
            jsr         con_size                            ;   line (the smaller of the terminals it's shown on);
            bcs         @env                                ;   no console, $LINES and $COLUMNS (as conio's), else
            lda         fs_rows                             ;   24 and 80
            ldy         #0
            PUSHAY
            lda         fs_cols
            ldy         #0
            PUSHAY
            rts
@env:
            LDR         r0, s_lines
            lda         #24
            jsr         env_num
            LDR         r0, s_columns
            lda         #80
            jmp         env_num

s_lines:    .byte       "LINES", 0
s_columns:  .byte       "COLUMNS", 0
s_size_w:   .byte       "size ", 0

; fs_cols, fs_rows: the window's size, its consctl's size line.  OUT: C = 1, none (no console).  Keeps .X
con_size:
            stx         xsave
            LDR         r0, s_consctl
            lda         #O_READ
            jsr         OPEN
            bcs         @none
            sta         tmp2                                ; (Its fd)
            LDR         r0, fs_buf
            LDR         r1, FS_BUF
            lda         tmp2
            jsr         READ
            bcc         :+
            lda         #0
:
            sta         tmp                                 ; (Its length)
            lda         tmp2
            jsr         CLOSE
            ldy         #0
@line:
            ldx         #0                                  ; A line: size?
:
            lda         s_size_w,x
            beq         @size
            cpy         tmp
            bcs         @none
            cmp         fs_buf,y
            bne         @skip
            iny
            inx
            bra         :-
@skip:
            cpy         tmp                                 ; Else on to the next
            bcs         @none
            lda         fs_buf,y
            iny
            cmp         #LF
            bne         @skip
            bra         @line
@size:
            jsr         @num                                ; Its columns, its rows
            sta         fs_cols
            iny
            jsr         @num
            sta         fs_rows
            beq         @none
            lda         fs_cols
            beq         @none
            ldx         xsave
            clc
            rts
@none:
            ldx         xsave
            sec
            rts

@num:                                                       ; .A = the number at fs_buf,y, past it (3 digits at most)
            stz         tmp + 1
:
            cpy         tmp
            bcs         :+
            lda         fs_buf,y
            sec
            sbc         #'0'
            cmp         #10
            bcs         :+
            pha
            lda         tmp + 1
            asl
            asl
            clc
            adc         tmp + 1
            asl
            sta         tmp + 1
            pla
            clc
            adc         tmp + 1
            sta         tmp + 1
            iny
            bra         :-
:
            lda         tmp + 1
            rts

; ( -- n ): the environment's variable r0, a number (decimal, 1-255), or .A if it hasn't one
env_num:
            sta         tmp2
            LDR         r1, en_buf
            LDR         r2, 4
            stz         r3
            stz         r3 + 1
            lda         #$FF
            stx         xsave
            jsr         ENV_GET
            sta         tmp                                 ; (Its length)
            ldx         xsave
            bcs         @default
            stz         tmp + 1                             ; (Its number: 3 digits at most)
            ldy         #0
@digit:
            cpy         tmp
            beq         @got
            cpy         #3
            beq         @got
            lda         en_buf,y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @got
            pha
            lda         tmp + 1
            asl
            asl
            clc
            adc         tmp + 1
            asl
            sta         tmp + 1
            pla
            clc
            adc         tmp + 1
            sta         tmp + 1
            iny
            bra         @digit
@got:
            lda         tmp + 1
            bne         @push
@default:
            lda         tmp2
@push:
            ldy         #0
            PUSHAY
            rts

            HEADER      "color", 0
color:                                                      ; ( c -- ): the text's colour, 0-15 (CSI 30-37 m, 90-97 m)
            lda         #30
            bra         colour

            HEADER      "bgcolor", 0
bgcolor:                                                    ; ( c -- ): the background's (CSI 40-47 m, 100-107 m)
            lda         #40
colour:
            sta         tmp
            lda         dlo,x
            and         #$0F
            cmp         #8
            bcc         :+
            adc         #60 - 8 - 1                         ; (C = 1: 8-15, the bright ones, 60 on)
:
            clc
            adc         tmp
            sta         dlo,x
            stz         dhi,x
            jmp         sgr

            HEADER      "black", 0
black:
            CONSTCODE   0

            HEADER      "red", 0
red:
            CONSTCODE   1

            HEADER      "green", 0
green:
            CONSTCODE   2

            HEADER      "yellow", 0
yellow:
            CONSTCODE   3

            HEADER      "blue", 0
blue:
            CONSTCODE   4

            HEADER      "magenta", 0
magenta:
            CONSTCODE   5

            HEADER      "cyan", 0
cyan:
            CONSTCODE   6

            HEADER      "white", 0
white:
            CONSTCODE   7

            HEADER      "bright", 0
bright:                                                     ; ( c -- c' ): its bright one (8-15)
            lda         dlo,x
            ora         #8
            sta         dlo,x
            rts

            HEADER      "plain", 0
plain:                                                      ; ( -- ): every attribute off, the terminal's own colours
            lda         #0                                  ;   (CSI 0m)
            bra         sgr_a

            HEADER      "bold", 0
bold:                                                       ; ( -- ): CSI 1m
            lda         #1
            bra         sgr_a

            HEADER      "dim", 0
dim:                                                        ; ( -- ): CSI 2m
            lda         #2
            bra         sgr_a

            HEADER      "underline", 0
underline:                                                  ; ( -- ): CSI 4m
            lda         #4
            bra         sgr_a

            HEADER      "blink", 0
blink:                                                      ; ( -- ): CSI 5m
            lda         #5
            bra         sgr_a

            HEADER      "reverse", 0
reverse:                                                    ; ( -- ): CSI 7m
            lda         #7
sgr_a:                                                      ; (CSI .A m)
            dex
            sta         dlo,x
            stz         dhi,x
            bra         sgr

            HEADER      "sgr", 0
sgr:                                                        ; ( n -- ): any attribute (CSI n m: the old Acol)
            jsr         esc_csi
            jsr         dec_out
            lda         #'m'
            jmp         emit_a

            HEADER      "beep", 0
beep:                                                       ; ( -- ): a BEL: the console rings the bell
            lda         #7
            jmp         emit_a

; ---- The keys, raw (Facility Extension's): the console's raw mode gives the terminal's cursor and function keys as
; one code each (KEY_UP ...), which are the k- words' values; the terminal sends no modifiers the console decodes

            HEADER      "ekey", 0
ekey:                                                       ; ( -- u ): a key, raw (KEY's way)
            jmp         key

            HEADER      "ekey?", 0
ekeyq:                                                      ; ( -- flag ): one waiting (KEY?'s way)
            jmp         keyq

            HEADER      "ekey>char", 0
ekeytochar:                                                 ; ( u -- u false | char true )
            jsr         is_fkey
            dex
            bcc         :+
            jmp         true_tos
:
            jmp         zero_tos

            HEADER      "ekey>fkey", 0
ekeytofkey:                                                 ; ( u -- u false | x true ): x a k- word's value
            jsr         is_fkey
            dex
            bcs         :+
            jmp         true_tos
:
            jmp         zero_tos

; Is the top a cursor or function key's code, or the window's resize or focus (KEY_UP to KEY_FOCUS)?  OUT: C = 0 yes
is_fkey:
            lda         dhi,x
            bne         :+
            lda         dlo,x
            sec
            sbc         #KEY_UP
            cmp         #KEY_FOCUS - KEY_UP + 1
            rts
:
            sec
            rts

            HEADER      "emit?", 0
emitq:                                                      ; ( -- flag ): EMIT can go on: always
            dex
            jmp         true_tos

            HEADER      "k-up", 0
kup:
            CONSTCODE   KEY_UP

            HEADER      "k-down", 0
kdown:
            CONSTCODE   KEY_DOWN

            HEADER      "k-left", 0
kleft:
            CONSTCODE   KEY_LEFT

            HEADER      "k-right", 0
kright:
            CONSTCODE   KEY_RIGHT

            HEADER      "k-home", 0
khome:
            CONSTCODE   KEY_HOME

            HEADER      "k-end", 0
kend:
            CONSTCODE   KEY_END

            HEADER      "k-prior", 0
kprior:
            CONSTCODE   KEY_PGUP

            HEADER      "k-next", 0
knext:
            CONSTCODE   KEY_PGDN

            HEADER      "k-insert", 0
kinsert:
            CONSTCODE   KEY_INS

            HEADER      "k-delete", 0
kdelete:
            CONSTCODE   KEY_DEL

            HEADER      "k-f1", 0
kf1:
            CONSTCODE   KEY_F1

            HEADER      "k-f2", 0
kf2:
            CONSTCODE   KEY_F2

            HEADER      "k-f3", 0
kf3:
            CONSTCODE   KEY_F3

            HEADER      "k-f4", 0
kf4:
            CONSTCODE   KEY_F4

            HEADER      "k-f5", 0
kf5:
            CONSTCODE   KEY_F5

            HEADER      "k-f6", 0
kf6:
            CONSTCODE   KEY_F6

            HEADER      "k-f7", 0
kf7:
            CONSTCODE   KEY_F7

            HEADER      "k-f8", 0
kf8:
            CONSTCODE   KEY_F8

            HEADER      "k-f9", 0
kf9:
            CONSTCODE   KEY_F9

            HEADER      "k-f10", 0
kf10:
            CONSTCODE   KEY_F10

            HEADER      "k-f11", 0
kf11:
            CONSTCODE   KEY_F11

            HEADER      "k-f12", 0
kf12:
            CONSTCODE   KEY_F12

            HEADER      "k-resize", 0
kresize:                                                    ; (Not a key: the window's size changed)
            CONSTCODE   KEY_RESIZE

            HEADER      "k-focus", 0
kfocus:                                                     ; (Not a key: the focus moved in its group, the window's
            CONSTCODE   KEY_FOCUS                           ;   number the next)

            HEADER      "k-shift-mask", 0
kshiftmask:
            CONSTCODE   $0100

            HEADER      "k-ctrl-mask", 0
kctrlmask:
            CONSTCODE   $0200

            HEADER      "k-alt-mask", 0
kaltmask:
            CONSTCODE   $0400

            HEADER      "begin-structure", 0
beginstructure:                                             ; ( "name" -- addr 0 ): name a constant, its size
            lda         #0                                  ;   (END-STRUCTURE's: addr is its literal's place)
            jsr         make_hdr
            clc
            lda         here
            adc         #2
            pha
            lda         here + 1
            adc         #0
            tay
            pla
            PUSHAY
            lda         #0
            tay
            jsr         comp_lit
            lda         #RTS_OP
            jsr         ccomma_a
            dex
            jmp         zero_tos

            HEADER      "end-structure", 0
endstructure:                                               ; ( addr +n -- ): the structure's size n
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         (w)
            ldy         #4                                  ; (The literal's high byte: 4 on)
            lda         dhi,x
            sta         (w),y
            inx
            inx
            rts

            HEADER      "+field", 0
plusfield:                                                  ; ( n1 n2 "name" -- n3 ): name adds n1; n3 = n1 + n2
            lda         #0
            jsr         make_hdr
            lda         dlo + 1,x
            ldy         dhi + 1,x
            jsr         comp_lit
            lda         #<plus
            ldy         #>plus
            jsr         comp_jmp
            jmp         plus

            HEADER      "field:", 0
fieldc:                                                     ; ( n1 "name" -- n2 ): a cell's
            lda         #2
            bra         :+

            HEADER      "cfield:", 0
cfieldc:                                                    ; ( n1 "name" -- n2 ): a char's
            lda         #1
:
            dex
            sta         dlo,x
            stz         dhi,x
            bra         plusfield
