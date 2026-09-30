.debuginfo

; ****************************************************************************
; The clock (BIOS ROM page 9, included inside `.scope PAGE9`, see all.s): CLOCK_GET and CLOCK_SET, and
; /dev/time.  The clock is the seconds since 2000-01-01 00:00:00 (ZP_CLOCK, in the system task's ZP),
; counted by the scheduler's tick (VIA_IRQ_FAST, page 2).  The Hydra has no clock that keeps the time while
; it's off, so it starts at 0 at power-up: until it's set, it's early on 2000-01-01, and so are the stamps
; HydraFS gives files.  It counts to 2135.
;   /dev/time   read: the date and time, as text: "2026-09-29 18:05:00" and CR LF
;               write: set them: "YYYY-MM-DD hh:mm:ss" (the seconds can be left out, or the whole time:
;               0); e.g. echo 2026-09-29 18:05 > /dev/time
; The server runs in the client's task (IO_DEV_CALLER_TASK; the shell registers it, as it does env:
; SH_BOOT).  Its scratch is ZP_PROC_* (the text: PROC_PUT, PROC_TEXT_OUT), ZP_TIME, ZP_TIME_M, ZP_TIME_D.

.segment "SYS_P9"

; The clock: its 4 bytes to the zero page address .X (in this task).  From any task.
; Preserves .A, .X, .Y (and the caller's I flag)
CLOCK_GET:
            php
            sei
            pha
            phy
            ldy         T_REGISTER
            stz         T_REGISTER                          ; Quick looks at the system task (no stack use!)
            lda         ZP_CLOCK
            sty         T_REGISTER
            sta         0,X
            stz         T_REGISTER
            lda         ZP_CLOCK + 1
            sty         T_REGISTER
            sta         1,X
            stz         T_REGISTER
            lda         ZP_CLOCK + 2
            sty         T_REGISTER
            sta         2,X
            stz         T_REGISTER
            lda         ZP_CLOCK + 3
            sty         T_REGISTER
            sta         3,X
            ply
            pla
            plp
            rts

; Set the clock to the 4 bytes at the zero page address .X (in this task), from the start of a second.
; From any task.  Preserves .A, .X, .Y (and the caller's I flag)
CLOCK_SET:
            php
            sei
            pha
            phy
            ldy         T_REGISTER
            lda         0,X
            stz         T_REGISTER                          ; Quick looks at the system task (no stack use!)
            sta         ZP_CLOCK
            sty         T_REGISTER
            lda         1,X
            stz         T_REGISTER
            sta         ZP_CLOCK + 1
            sty         T_REGISTER
            lda         2,X
            stz         T_REGISTER
            sta         ZP_CLOCK + 2
            sty         T_REGISTER
            lda         3,X
            stz         T_REGISTER
            sta         ZP_CLOCK + 3
            lda         #SCHED_TICK_HZ
            sta         ZP_CLOCK_SUB
            sty         T_REGISTER
            ply
            pla
            plp
            rts

; ****************************************************************************
; /dev/time's requests.  IN: .A = request, .X = client, .Y = fid (0)
TIME_SERVE:
            cmp         #H9_CREATE
            bcs         TIME_BAD                            ; (The filesystem's requests)
            cmp         #H9_OPEN
            beq         TIME_OPEN
            cmp         #H9_READ
            beq         TIME_READ
            cmp         #H9_WRITE
            bne         :+
            jmp         TIME_WRITE
:
            cmp         #H9_STAT
            bne         :+
            jsr         STAT_ZERO
            bra         TIME_OK
:
            cmp         #H9_CTL
            beq         TIME_BAD                            ; H9_CLUNK, H9_DUP: nothing to do

TIME_OK:
            lda         #0
            clc
            rts

TIME_BAD:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; The rest of the name (in the data area) must be empty: /dev/time.  OUT: .A = the fid (0)
TIME_OPEN:
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1
            lda         (ZP_IO_REQ)
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            cmp         #0
            beq         TIME_OK
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

; Read: the date and time, made in the data area, then what's after the fd's offset (PROC_TEXT_OUT)
TIME_READ:
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; (The data area: PROC_PUT)
            stz         ZP_PROC_IDX
            ldx         #ZP_TIME
            jsr         CLOCK_GET
            jsr         TIME_TEXT
            dec         ZP_IO_REQ + 1
            jmp         PROC_TEXT_OUT

; ZP_TIME (seconds since 2000-01-01) as text (PROC_PUT): "YYYY-MM-DD hh:mm:ss" and CR LF.
; Modifies: .A, .X, .Y, ZP_TIME, ZP_TIME_M, ZP_TIME_D
TIME_TEXT:
            lda         #60                                 ; The seconds, minutes and hours, kept ...
            jsr         TIME_DIV8
            pha
            lda         #60
            jsr         TIME_DIV8
            pha
            lda         #24
            jsr         TIME_DIV8
            pha
            stz         ZP_TIME_D                           ; ... and the days (under 65536): the year's

@year:
            jsr         TIME_YEAR_DAYS                      ; (.A.Y = its days)
            sta         ZP_TIME_M
            sty         ZP_TIME_M + 1
            lda         ZP_TIME
            cmp         ZP_TIME_M
            lda         ZP_TIME + 1
            sbc         ZP_TIME_M + 1
            bcc         @month                              ; (In this year)
            lda         ZP_TIME
            sbc         ZP_TIME_M
            sta         ZP_TIME
            lda         ZP_TIME + 1
            sbc         ZP_TIME_M + 1
            sta         ZP_TIME + 1
            inc         ZP_TIME_D
            bra         @year

@month:
            ldx         #'0'                                ; The year: 20YY, or 21YY
            lda         ZP_TIME_D
            cmp         #100
            bcc         :+
            sbc         #100
            inx
:
            pha
            lda         #'2'
            jsr         PROC_PUT
            txa
            jsr         PROC_PUT
            pla
            jsr         TIME_PUT2
            lda         #'-'
            jsr         PROC_PUT
            ldx         #0                                  ; (.X = the month - 1)

@days:
            jsr         TIME_MLEN
            sta         ZP_TIME_M
            lda         ZP_TIME + 1
            bne         :+                                  ; (256 days or more: past this month)
            lda         ZP_TIME
            cmp         ZP_TIME_M
            bcc         @day
:
            sec
            lda         ZP_TIME
            sbc         ZP_TIME_M
            sta         ZP_TIME
            lda         ZP_TIME + 1
            sbc         #0
            sta         ZP_TIME + 1
            inx
            bra         @days

@day:
            inx
            txa
            jsr         TIME_PUT2
            lda         #'-'
            jsr         PROC_PUT
            lda         ZP_TIME
            inc
            jsr         TIME_PUT2
            lda         #' '
            jsr         PROC_PUT
            pla                                             ; The hours, minutes, seconds
            jsr         TIME_PUT2
            lda         #':'
            jsr         PROC_PUT
            pla
            jsr         TIME_PUT2
            lda         #':'
            jsr         PROC_PUT
            pla
            jsr         TIME_PUT2
            lda         #ASCII_CR
            jsr         PROC_PUT
            lda         #ASCII_LF
            jmp         PROC_PUT

; Add .A (0-99) to the text as two digits.  Modifies: .A, .X, .Y
TIME_PUT2:
            ldx         #'0' - 1
:
            inx
            sec
            sbc         #10
            bcs         :-
            adc         #10 + '0'                           ; (C = 0)
            pha
            txa
            jsr         PROC_PUT
            pla
            jmp         PROC_PUT

; Write: set the clock from "YYYY-MM-DD[ hh:mm[:ss]]" (the whole write is taken).
; OUT: C = 0; or C = 1, .A = ERR_IO_BAD_REQ (not a date and time the clock can hold)
TIME_WRITE:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            bne         :+
            dec                                             ; (256 bytes: look at 255)
:
            sta         ZP_PROC_LEN
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
            jsr         TIME_NUM                            ; The year: 2000-2135
            bcs         @bad
            sec
            lda         ZP_TIME_M
            sbc         #<2000
            tax
            lda         ZP_TIME_M + 1
            sbc         #>2000
            bne         @bad
            cpx         #136
            bcs         @bad
            stx         ZP_PROC_OWN
            ldx         #3                                  ; ZP_TIME = the days before it
:
            stz         ZP_TIME,X
            dex
            bpl         :-
            stz         ZP_TIME_D                           ; (.. counting the years: ZP_TIME_D, the year's
                                                            ;   number from 2000, which TIME_MLEN wants)
@years:
            lda         ZP_TIME_D
            cmp         ZP_PROC_OWN
            beq         @month
            phy                                             ; (Where the text's read to)
            jsr         TIME_YEAR_DAYS
            clc
            adc         ZP_TIME
            sta         ZP_TIME
            tya
            adc         ZP_TIME + 1
            sta         ZP_TIME + 1
            ply
            inc         ZP_TIME_D
            bra         @years

@bad:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            jmp         TIME_BAD

@month:
            lda         #'-'                                ; The month: 1-12 (and its days before it)
            jsr         TIME_SEP
            bcs         @bad
            jsr         TIME_NUM
            bcs         @bad
            lda         ZP_TIME_M + 1
            bne         @bad
            lda         ZP_TIME_M
            beq         @bad
            cmp         #13
            bcs         @bad
            sta         ZP_PROC_FG
            ldx         #0

@months:
            inx
            cpx         ZP_PROC_FG
            beq         @day
            dex
            jsr         TIME_MLEN                           ; (.X = the month - 1)
            jsr         TIME_ADD8
            inx
            bra         @months

@day:
            lda         #'-'                                ; The day: 1 to its month's days
            jsr         TIME_SEP
            bcs         @bad
            jsr         TIME_NUM
            bcs         @bad
            lda         ZP_TIME_M + 1
            bne         @bad
            ldx         ZP_PROC_FG                          ; (.X = the month - 1)
            dex
            jsr         TIME_MLEN
            cmp         ZP_TIME_M
            bcc         @bad
            lda         ZP_TIME_M
            beq         @bad
            dec
            jsr         TIME_ADD8
            lda         #24                                 ; The hours
            jsr         TIME_MUL8
            lda         #' '
            jsr         TIME_SEP
            bcs         @midnight
            lda         #24
            jsr         TIME_FIELD
            bcs         @bad
            lda         #60                                 ; The minutes
            jsr         TIME_MUL8
            lda         #':'
            jsr         TIME_SEP
            bcs         @bad
            lda         #60
            jsr         TIME_FIELD
            bcs         @bad
            lda         #60                                 ; The seconds
            jsr         TIME_MUL8
            lda         #':'
            jsr         TIME_SEP
            bcs         @set
            lda         #60
            jsr         TIME_FIELD
            bcc         @far2
            jmp         @bad
@far2:
            bra         @set

@midnight:
            lda         #60 * 60 / 16                       ; (No time: 00:00:00.  The day's hours * 3600)
            jsr         TIME_MUL8
            lda         #16
            jsr         TIME_MUL8

@set:
            cpy         ZP_PROC_LEN                         ; The end: the write's, or a CR, LF or 0
            beq         :+
            lda         (ZP_IO_REQ),Y
            beq         :+
            cmp         #ASCII_CR
            beq         :+
            cmp         #ASCII_LF
            beq         @far1
            jmp         @bad
@far1:
:
            ldx         #ZP_TIME
            jsr         CLOCK_SET
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of it taken)
            jmp         TIME_OK

; A number below .A (a field of the time) at (ZP_IO_REQ),Y, added to ZP_TIME.
; OUT: C = 0 (.Y past it); or C = 1.  Modifies: .A, .X, ZP_TIME_M
TIME_FIELD:
            pha
            jsr         TIME_NUM
            pla
            bcs         @done
            ldx         ZP_TIME_M + 1
            bne         @no
            cmp         ZP_TIME_M
            beq         @no
            bcc         @no
            lda         ZP_TIME_M
            jsr         TIME_ADD8
            clc
            rts

@no:
            sec

@done:
            rts

; Is the character at (ZP_IO_REQ),Y .A?  OUT: C = 0: it is, and .Y is past it; or C = 1
TIME_SEP:
            cpy         ZP_PROC_LEN
            beq         @no
            cmp         (ZP_IO_REQ),Y
            bne         @no
            iny
            clc
            rts

@no:
            sec
            rts

; The decimal number at (ZP_IO_REQ),Y (1-4 digits) -> ZP_TIME_M (2 bytes).
; OUT: C = 0 (.Y past it); or C = 1 (no digits, or too many).  Modifies: .A, .X, ZP_TIME_M
TIME_NUM:
            stz         ZP_TIME_M
            stz         ZP_TIME_M + 1
            ldx         #0                                  ; (The digits)

@digit:
            cpy         ZP_PROC_LEN
            beq         @end
            lda         (ZP_IO_REQ),Y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @end
            cpx         #4
            bcs         @no
            pha
            asl         ZP_TIME_M                           ; * 10: * 2, kept, * 4, + that
            rol         ZP_TIME_M + 1
            lda         ZP_TIME_M
            sta         ZP_TIME_M + 2
            lda         ZP_TIME_M + 1
            sta         ZP_TIME_M + 3
            asl         ZP_TIME_M
            rol         ZP_TIME_M + 1
            asl         ZP_TIME_M
            rol         ZP_TIME_M + 1
            clc
            lda         ZP_TIME_M
            adc         ZP_TIME_M + 2
            sta         ZP_TIME_M
            lda         ZP_TIME_M + 1
            adc         ZP_TIME_M + 3
            sta         ZP_TIME_M + 1
            pla                                             ; + the digit
            clc
            adc         ZP_TIME_M
            sta         ZP_TIME_M
            bcc         :+
            inc         ZP_TIME_M + 1
:
            iny
            inx
            bra         @digit

@end:
            txa
            beq         @no
            clc
            rts

@no:
            sec
            rts

; ****************************************************************************
; Arithmetic on ZP_TIME (4 bytes)

; ZP_TIME = ZP_TIME / .A (2-127); .A = the remainder.  Modifies: .X, ZP_TIME_D
TIME_DIV8:
            sta         ZP_TIME_D
            lda         #0
            ldx         #32

@bit:
            asl         ZP_TIME
            rol         ZP_TIME + 1
            rol         ZP_TIME + 2
            rol         ZP_TIME + 3
            rol
            cmp         ZP_TIME_D
            bcc         :+
            sbc         ZP_TIME_D
            inc         ZP_TIME                             ; (The quotient's bit: it was 0)
:
            dex
            bne         @bit
            rts

; ZP_TIME = ZP_TIME * .A.  Modifies: .A, .X, ZP_TIME_M, ZP_TIME_D
TIME_MUL8:
            sta         ZP_TIME_D
            ldx         #3
:
            lda         ZP_TIME,X
            sta         ZP_TIME_M,X
            stz         ZP_TIME,X
            dex
            bpl         :-

@bit:
            lsr         ZP_TIME_D
            bcc         @shift
            clc
            lda         ZP_TIME
            adc         ZP_TIME_M
            sta         ZP_TIME
            lda         ZP_TIME + 1
            adc         ZP_TIME_M + 1
            sta         ZP_TIME + 1
            lda         ZP_TIME + 2
            adc         ZP_TIME_M + 2
            sta         ZP_TIME + 2
            lda         ZP_TIME + 3
            adc         ZP_TIME_M + 3
            sta         ZP_TIME + 3

@shift:
            asl         ZP_TIME_M
            rol         ZP_TIME_M + 1
            rol         ZP_TIME_M + 2
            rol         ZP_TIME_M + 3
            lda         ZP_TIME_D
            bne         @bit
            rts

; ZP_TIME = ZP_TIME + .A.  Modifies: .A
TIME_ADD8:
            clc
            adc         ZP_TIME
            sta         ZP_TIME
            bcc         @done
            inc         ZP_TIME + 1
            bne         @done
            inc         ZP_TIME + 2
            bne         @done
            inc         ZP_TIME + 3

@done:
            rts

; ****************************************************************************
; The calendar (2000 to 2135: every fourth year is a leap year, but 2100 isn't)

; Is year 2000 + ZP_TIME_D a leap year?  OUT: C = 1: it is.  Modifies: .A
TIME_LEAP:
            lda         ZP_TIME_D
            and         #3
            bne         @no
            lda         ZP_TIME_D
            cmp         #100
            beq         @no
            sec
            rts

@no:
            clc
            rts

; .A.Y = the days in year 2000 + ZP_TIME_D (365 or 366).  Modifies: .A, .Y
TIME_YEAR_DAYS:
            jsr         TIME_LEAP
            lda         #<365
            adc         #0
            ldy         #>365
            rts

; .A = the days in month .X + 1 (0-11) of year 2000 + ZP_TIME_D.  Preserves .X, .Y
TIME_MLEN:
            lda         TIME_MONTHS,X
            cpx         #1
            bne         @done
            pha
            jsr         TIME_LEAP                           ; (February: 29 days in a leap year)
            pla
            adc         #0

@done:
            rts

TIME_MONTHS:    .byte   31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31

.assert     <365 < $FF, error, "TIME_YEAR_DAYS: 366's low byte is 365's + 1"
