.debuginfo

; ****************************************************************************
; The clock chip (BIOS ROM page 9, included inside `.scope PAGE9`, see all.s): a DS1747 in U7, the task RAM
; (hw.inc: its clock registers are task RTC_TASK's RTC_REGS-$7FFF).  At boot the shell looks for it
; (RTC_BOOT): its registers must hold a date and time, and its seconds must count; then the clock
; (ZP_CLOCK, time_srv.s) is set from it, at the start of one of its seconds.  /dev/time keeps them
; together: setting the time sets the chip too (RTC_SAVE), and reading it takes the chip's seconds
; (RTC_LOAD).  Whether there's one: RTC_STATE, in the system's shared bank.
;   The chip is reached a byte at a time, with IRQs off and T switched to task RTC_TASK and back for each
; (no stack or zero page use while it's switched), through RTC_ZBUF in the caller's zero page: its 8
; registers, in their order.  Its fields are BCD, and go to and from the clock as the text /dev/time reads
; and writes ("YYYY-MM-DD hh:mm:ss": TIME_TEXT, TIME_PARSE), so the calendar is in one place.  The text is
; made at (ZP_IO_REQ), ZP_PROC_IDX long (PROC_PUT).

.segment "SYS_P9"

RTC_ZBUF        = ZP_TIME                                   ; (8 bytes: ZP_TIME and ZP_TIME_M)
.assert     ZP_TIME_M = ZP_TIME + 4, error, "RTC_ZBUF: ZP_TIME and ZP_TIME_M must be one 8-byte area"

RTC_ST_NONE     = $00                                       ; RTC_STATE, and what RTC_PROBE finds: none,
RTC_ST_FOUND    = $80                                       ;   a DS1747 that keeps time,
RTC_ST_STOPPED  = $40                                       ;   one with a time, its oscillator stopped (OSC)
RTC_ST_BATTERY  = $01                                       ;   (and found: its battery flag says it's flat)

; ****************************************************************************
; The chip's registers -> RTC_ZBUF: read with its updates halted (R set, then a 0 written, which clears R
; and leaves the century).  Preserves .X, .Y (and the caller's I flag).  Modifies: .A
RTC_GET:
            php
            sei
            phx
            phy
            ldy         T_REGISTER                          ; .Y = this task, .X = the chip's
            ldx         #RTC_TASK
            stx         T_REGISTER                          ; (No stack or zero page use until it's back!)
            lda         #RTC_R
            sta         RTC_CTL
.repeat     8, I
            stx         T_REGISTER
            lda         RTC_REGS + I
            sty         T_REGISTER
            sta         RTC_ZBUF + I
.endrepeat
            stx         T_REGISTER
            stz         RTC_CTL                             ; (Updates go on)
            sty         T_REGISTER
            ply
            plx
            plp
            rts

; Set the chip from RTC_ZBUF: W set, its registers after the control, then the control with W clear and
; the century (RTC_ZBUF's first byte), which starts the chip from them.  Preserves .X, .Y (and the caller's
; I flag).  Modifies: .A
RTC_PUT:
            php
            sei
            phx
            phy
            ldy         T_REGISTER                          ; .Y = this task, .X = the chip's
            ldx         #RTC_TASK
            stx         T_REGISTER                          ; (No stack or zero page use until it's back!)
            lda         #RTC_W
            sta         RTC_CTL
.repeat     7, I
            sty         T_REGISTER
            lda         RTC_ZBUF + 1 + I
            stx         T_REGISTER
            sta         RTC_REGS + 1 + I
.endrepeat
            sty         T_REGISTER
            lda         RTC_ZBUF
            stx         T_REGISTER
            sta         RTC_CTL
            sty         T_REGISTER
            ply
            plx
            plp
            rts

; .A = the chip's seconds register as it is (no halt: to watch it change).  Preserves .X, .Y (and the
; caller's I flag)
RTC_SECS:
            php
            sei
            phy
            ldy         T_REGISTER
            lda         #RTC_TASK
            sta         T_REGISTER                          ; (No stack or zero page use until it's back!)
            lda         RTC_SEC
            sty         T_REGISTER
            ply
            plp
            rts

; Does RTC_ZBUF hold a date and time the clock can have?  Each field BCD and in its range (the century 20
; or 21), and the bits that aren't the fields' 0 (but OSC, BF, W and R).  (The day against its month's
; days, and the year up to 2135: TIME_PARSE.)  OUT: C = 0: it does; or C = 1.  Modifies: .A, .X
RTC_VALID:
            ldx         #7

@field:
            lda         RTC_ZBUF,X
            and         RTC_UNUSED,X
            bne         @no
            lda         RTC_ZBUF,X
            and         RTC_FIELD,X
            cmp         RTC_MIN,X
            bcc         @no
            cmp         RTC_MAX,X
            beq         :+
            bcs         @no
:
            and         #$0F                                ; (The low digit: 0-9)
            cmp         #10
            bcs         @no
            dex
            bpl         @field
            clc
            rts

@no:
            sec
            rts

;                         ctl  sec  min  hour day  date mon  year
RTC_FIELD:      .byte   $3F, $7F, $7F, $3F, $07, $3F, $1F, $FF
RTC_UNUSED:     .byte   $00, $00, $80, $C0, $78, $C0, $E0, $00     ; (Day: FT too, never set)
RTC_MIN:        .byte   $20, $00, $00, $00, $01, $01, $01, $00
RTC_MAX:        .byte   $21, $59, $59, $23, $07, $31, $12, $99

; RTC_ZBUF as text: "YYYY-MM-DD hh:mm:ss" (PROC_PUT).  Modifies: .A, .X, .Y
RTC_TEXT:
            ldx         #0

@field:
            lda         RTC_T_SEP,X                         ; (The separator before it)
            beq         :+
            jsr         PROC_PUT
:
            ldy         RTC_T_REG,X
            lda         RTC_ZBUF,Y
            and         RTC_FIELD,Y
            pha
            lsr
            lsr
            lsr
            lsr
            ora         #'0'
            jsr         PROC_PUT
            pla
            and         #$0F
            ora         #'0'
            jsr         PROC_PUT
            inx
            cpx         #7
            bne         @field
            rts

RTC_T_REG:      .byte   0, 7, 6, 5, 3, 2, 1                 ; The century, year, month, date, hours ...
RTC_T_SEP:      .byte   0, 0, '-', '-', ' ', ':', ':'
RTC_T_OFS:      .byte   0, 17, 14, 11, 0, 8, 5, 2           ; By register: where its digits are in the text

; ****************************************************************************
; ZP_TIME = the chip's time (seconds since 2000-01-01).  OUT: C = 0; or C = 1: it hasn't a date and time
; the clock can have.  Modifies: .A, .X, .Y, ZP_TIME ..., ZP_PROC_*
RTC_LOAD:
            jsr         RTC_GET
            jsr         RTC_VALID
            bcs         @done
            stz         ZP_PROC_IDX
            jsr         RTC_TEXT
            lda         ZP_PROC_IDX
            sta         ZP_PROC_LEN
            jsr         TIME_PARSE

@done:
            rts

; Set the chip to ZP_TIME (seconds since 2000-01-01): its fields from the text, the day of the week
; (1-7, 1 = Sunday) from the days.  Modifies: .A, .X, .Y, ZP_TIME ..., ZP_PROC_*
RTC_SAVE:
            ldx         #3                                  ; ZP_TIME kept
:
            lda         ZP_TIME,X
            pha
            dex
            bpl         :-
            lda         #60                                 ; The days ...
            jsr         TIME_DIV8
            lda         #60
            jsr         TIME_DIV8
            lda         #24
            jsr         TIME_DIV8
            lda         #7                                  ; ... the day of the week: 2000-01-01 was a
            jsr         TIME_DIV8                           ;   Saturday, 7
            clc
            adc         #6
            cmp         #7
            bcc         :+
            sbc         #7
:
            inc
            sta         ZP_PROC_FG
            ldx         #0
:
            pla
            sta         ZP_TIME,X
            inx
            cpx         #4
            bne         :-
            stz         ZP_PROC_IDX                         ; The text: "YYYY-MM-DD hh:mm:ss" CR LF
            jsr         TIME_TEXT
            ldx         #7                                  ; Each register's two digits

@field:
            ldy         RTC_T_OFS,X
            lda         (ZP_IO_REQ),Y
            and         #$0F
            asl
            asl
            asl
            asl
            sta         ZP_PROC_OWN
            iny
            lda         (ZP_IO_REQ),Y
            and         #$0F
            ora         ZP_PROC_OWN
            sta         RTC_ZBUF,X
            dex
            bpl         @field
            lda         ZP_PROC_FG                          ; (W clear, OSC clear: it runs; the day's BF and
            sta         RTC_ZBUF + 4                        ;   FT 0)
            jmp         RTC_PUT

; Is there a DS1747?  Its registers hold a date and time (RTC_VALID), and its seconds change within 1.1 s
; (watched every 10 ms, so it returns within 10 ms of the start of one of its seconds).  A plain HM628512
; in U7 holds junk there: nearly always no date, and no wait.  RTC_STATE = what it finds.
; OUT: .A = RTC_ST_* (N = 1: found).  Modifies: .X, .Y, RTC_ZBUF
RTC_PROBE:
            jsr         RTC_GET
            jsr         RTC_VALID
            lda         #RTC_ST_NONE
            bcs         @state
            lda         #RTC_ST_STOPPED
            bit         RTC_ZBUF + 1                        ; (OSC: its oscillator stopped)
            bmi         @state
            ldx         #110

@watch:
            lda         #2
            ldy         #0
            jsr         TASK_SLEEP                          ; (Preserves .X)
            jsr         RTC_SECS
            cmp         RTC_ZBUF + 1
            bne         @runs
            dex
            bne         @watch
            lda         #RTC_ST_NONE
            bra         @state

@runs:
            lda         #RTC_ST_FOUND
            bit         RTC_ZBUF + 4                        ; (BF: 1 while its battery is good)
            bmi         @state
            ora         #RTC_ST_BATTERY

@state:
            pha
            _M_SYS_ENTER
            sta         RTC_STATE
            _M_SYS_LEAVE
            pla
            rts

; .A = RTC_STATE (N = 1: there's a DS1747).  Modifies: .Y
RTC_STATE_GET:
            _M_SYS_ENTER
            lda         RTC_STATE
            _M_SYS_LEAVE
            ora         #0
            rts

; ****************************************************************************
; At boot (the shell's SH_BOOT, before anything reads /dev/time): look for the chip, set the clock from it,
; and make the line that says so, at (ZP_IO_REQ), ZP_PROC_IDX long: "clock 2026-09-30 14:05:00" (and
; " battery low" when its battery flag says so), "clock stopped: set the time", or "no clock"; with CR LF.
; Modifies: .A, .X, .Y, ZP_TIME ..., ZP_PROC_*
RTC_BOOT:
            jsr         RTC_PROBE
            pha
            bpl         @line
            jsr         RTC_LOAD                            ; Found: the clock from it, at the start of
            bcs         @line                               ;   one of its seconds (RTC_PROBE)
            ldx         #ZP_TIME
            jsr         CLOCK_SET

@line:
            stz         ZP_PROC_IDX
            pla
            bmi         @found
            ldx         #RTC_S_NONE - RTC_S
            and         #RTC_ST_STOPPED
            beq         @puts
            ldx         #RTC_S_STOPPED - RTC_S

@puts:
            jmp         RTC_PUTS

@found:
            pha
            ldx         #RTC_S_CLOCK - RTC_S
            jsr         RTC_PUTS
            ldx         #ZP_TIME
            jsr         CLOCK_GET
            jsr         TIME_TEXT                           ; (With CR LF)
            pla
            and         #RTC_ST_BATTERY
            beq         @done
            dec         ZP_PROC_IDX                         ; (Before the CR LF)
            dec         ZP_PROC_IDX
            ldx         #RTC_S_BATTERY - RTC_S
            jmp         RTC_PUTS

@done:
            rts

; Add string .X of RTC_S (to its 0) to the text.  Modifies: .A, .X, .Y
RTC_PUTS:
            lda         RTC_S,X
            beq         @done
            jsr         PROC_PUT
            inx
            bra         RTC_PUTS

@done:
            rts

RTC_S:
RTC_S_CLOCK:    .byte   "clock ", 0
RTC_S_STOPPED:  .byte   "clock stopped: set the time", ASCII_CR, ASCII_LF, 0
RTC_S_NONE:     .byte   "no clock", ASCII_CR, ASCII_LF, 0
RTC_S_BATTERY:  .byte   " battery low", ASCII_CR, ASCII_LF, 0
