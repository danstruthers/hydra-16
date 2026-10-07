; ****************************************************************************
; time.s - the clock (docs/reimplementation-from-scratch.md, phase 5.4; BIOS ROM page 1): seconds since 2000-01-01,
; kept as the boot's time (K0_BOOT: the clock at tick 0) and the ticks since, so the tick stays a count (no seconds
; counter in it); and the DS1747, if there's one in U7: its 8 registers at task F's $7FF8-$7FFF (task F is always a
; driver, its RAM below $7F00), reached a byte at a time by a quick look.  The calendar is kdev's (#t's /dev/time),
; which sets the clock from the chip as it starts, and the chip from the clock when the time is set.
;   TIME          r0/r1 = the clock
;   TIME_SET      the clock = r0/r1 (from now: the boot's time is the clock less the ticks since)
;   RTC           .A = 0: the chip's registers into the 8 bytes at r0 (its updates halted for the copy: R); 1: the
;                 chip set from them (W set, the other 7 written, then the control, its century, with W clear:
;                 the chip starts from them).  The bytes as the chip has them: control (the century), seconds
;                 (OSC: bit 7), minutes, hours, day (BF: bit 7), date, month, year, BCD.  No chip: the RAM there
; Each look at task 0's K0_TICKS and K0_BOOT, and each byte of the chip, is a moment with IRQs off.

.include "kdefs.inc"

.segment "KCODE_P1"

; TIME: r0/r1 = the clock.  Modifies .A, .X, .Y, r2, r3
K_TIME:
            jsr         ticks_secs                          ; r2/r3: the seconds since the boot
            php                                             ; + the boot's time
            sei
            ldy         T_REGISTER
            stz         T_REGISTER
            ldx         #0
            clc
:
            lda         K0_BOOT,X
            sty         T_REGISTER
            adc         r2,X
            sta         r0,X
            stz         T_REGISTER
            inx
            txa                                             ; (eor, not cpx: the carry goes on)
            eor         #4
            bne         :-
            sty         T_REGISTER
            plp
            clc
            rts

; TIME_SET: the clock = r0/r1.  Modifies .A, .X, .Y, r2, r3
K_TIME_SET:
            jsr         ticks_secs                          ; The boot's time = it less the seconds since
            ldx         #0
            sec
:
            lda         r0,X
            sbc         r2,X
            sta         r2,X
            inx
            txa                                             ; (eor, not cpx: the carry goes on)
            eor         #4
            bne         :-
            php
            sei
            ldy         T_REGISTER
            ldx         #3
:
            sty         T_REGISTER
            lda         r2,X
            stz         T_REGISTER
            sta         K0_BOOT,X
            dex
            bpl         :-
            sty         T_REGISTER
            plp
            clc
            rts

; r2/r3 = the ticks since the boot (K0_TICKS) / TICK_HZ: whole seconds.  Modifies .A, .X, .Y
ticks_secs:
            php
            sei
            ldy         T_REGISTER
            ldx         #3
:
            stz         T_REGISTER
            lda         K0_TICKS,X
            sty         T_REGISTER
            sta         r2,X
            dex
            bpl         :-
            plp
            lda         #0                                  ; r2/r3 / TICK_HZ: shifted out as the quotient's
            ldx         #32                                 ;   shifted in (.A the remainder)
@bit:
            asl         r2
            rol         r2 + 1
            rol         r3
            rol         r3 + 1
            rol         a
            bcs         @sub                                ; (Past 255: more than TICK_HZ)
            cmp         #TICK_HZ
            bcc         @next
@sub:
            sbc         #TICK_HZ                            ; (C = 1)
            inc         r2
@next:
            dex
            bne         @bit
            rts

.assert     TICK_HZ < 256, error, "ticks_secs: TICK_HZ a byte"

; RTC: .A = 0, the chip's registers into r0's 8 bytes; 1, the chip set from them.  OUT: C = 0; or C = 1, .A = E_INVAL
K_RTC:
            cmp         #2
            bcc         :+
            FAIL        E_INVAL
:
            ldx         T_REGISTER                          ; .X: the caller (all of T)
            cmp         #0
            bne         @write
            lda         #RTC_R                              ; Read: its updates halted ...
            jsr         @ctl
            ldy         #0
:
            php                                             ;   each register, task F's to the caller's buffer ...
            sei
            lda         #RTC_TASK
            sta         T_REGISTER
            lda         RTC_REGS,Y
            stx         T_REGISTER
            plp
            sta         (r0),Y
            iny
            cpy         #8
            bne         :-
            lda         #0                                  ;   and its updates on again (a 0: R clear, the century
            jsr         @ctl                                ;   as it was: it's written only with W)
            clc
            rts

@write:
            ldy         #7                                  ; Write: the buffer into the caller's TA_SCRATCH (an
:                                                           ;   address that's the same while T is switched: its
            lda         (r0),Y                              ;   zero page and stack aren't) ...
            sta         TA_SCRATCH,Y
            dey
            bpl         :-
            lda         #RTC_W                              ;   its updates halted (W) ...
            jsr         @ctl
            txa                                             ;   its registers but the control, each a moment with
            tay                                             ;   T switched (.Y: the caller, .X: the chip's task:
            ldx         #RTC_TASK                           ;   no stack or zero page while it's switched) ...
.repeat     7, I
            php
            sei
            sty         T_REGISTER
            lda         TA_SCRATCH + 1 + I
            stx         T_REGISTER
            sta         RTC_REGS + 1 + I
            sty         T_REGISTER
            plp
.endrepeat
            tya
            tax
            lda         TA_SCRATCH                          ;   then the control, W clear: it starts from them
            and         #<~(RTC_W | RTC_R)
            jsr         @ctl
            clc
            rts

@ctl:                                                       ; The control register = .A (.X: the caller).  Keeps .X
            php
            sei
            phy
            tay                                             ; (.Y: the byte, kept in a register while T is
            lda         #RTC_TASK                           ;   switched: no stack then)
            sta         T_REGISTER
            sty         RTC_REGS
            stx         T_REGISTER
            ply
            plp
            rts
