; ****************************************************************************
; facility.s - HyForth's Facility library (/lib/forth/facility.fl): KEY?, MS, TIME&DATE, AT-XY and PAGE (the
; terminal's ANSI sequences), and the structures (BEGIN-STRUCTURE ... +FIELD).  KEY? reads the console's cons
; non-blocking, in raw mode (till a line's read: the core's raw_off), and leaves its key for KEY (key_pend).

.include "forthlib.inc"

.bss
kq_fd:      .res        1                                   ; KEY?'s fd (/dev/cons, non-blocking), $FF: not open
.code

; Its start: KEY?'s fd not open yet
lib_init:
            lda         #$FF
            sta         kq_fd
            rts

            HEADER      "KEY?", 0
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

            HEADER      "MS", 0
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
            jmp         intr_throw
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

            HEADER      "TIME&DATE", 0
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

            HEADER      "AT-XY", 0
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

            HEADER      "PAGE", 0
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

            HEADER      "BEGIN-STRUCTURE", 0
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

            HEADER      "END-STRUCTURE", 0
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

            HEADER      "+FIELD", 0
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

            HEADER      "FIELD:", 0
fieldc:                                                     ; ( n1 "name" -- n2 ): a cell's
            lda         #2
            bra         :+

            HEADER      "CFIELD:", 0
cfieldc:                                                    ; ( n1 "name" -- n2 ): a char's
            lda         #1
:
            dex
            sta         dlo,x
            stz         dhi,x
            bra         plusfield
