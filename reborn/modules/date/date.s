; ****************************************************************************
; date [-n] - the clock: as /dev/time has it ("2026-10-03 15:04:05"), or (-n) in seconds since 2000-01-01.  The
; time is set by writing /dev/time (echo 2026-10-03 15:04:05 >/dev/time); /dev/rtc says if there's a DS1747.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "date", main

F_N             = $01           ; -n

.bss
buf:        .res        32                                  ; /dev/time's text ...
got:        .res        1                                   ;   its bytes (or the read's error)
fd:         .res        1

.code
main:
            jsr         tl_start
            lda         (tl_arg)                            ; (No names)
            beq         :+
            jmp         tl_badusage
:
            lda         tl_flags
            and         #F_N
            bne         @seconds
            LDR         r0, s_time                          ; /dev/time's text, out
            lda         #O_READ
            jsr         OPEN
            bcs         @error
            sta         fd
            LDR         r0, buf
            LDR         r1, 32
            lda         fd
            jsr         READ
            sta         got
            php
            lda         fd
            jsr         CLOSE
            plp
            lda         got
            bcs         @error
            ldy         #0
:
            cpy         got
            beq         @done
            lda         buf,Y
            jsr         tl_putc
            iny
            bra         :-
@done:
            jmp         tl_end

@seconds:
            jsr         TIME                                ; The seconds
            ldx         #3
:
            lda         r0,X
            sta         tl_num,X
            dex
            bpl         :-
            lda         #0
            jsr         tl_dec
            jsr         tl_nl
            jmp         tl_end

@error:
            pha
            LDR         r0, s_time
            pla
            jsr         tl_err
            jmp         tl_end

.rodata
s_time:     .byte       "/dev/time", 0
tl_name:    .byte       "date", 0
tl_flagset: .byte       "n", 0
tl_usage:   .byte       "date [-n]", 0

.include "toollib.s"
