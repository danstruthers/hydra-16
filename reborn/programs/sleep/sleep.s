; ****************************************************************************
; sleep seconds - nothing for that many seconds (SLEEP, 100 seconds at a time at most); a note (Ctrl-C) ends it
; sooner.  Not a number: the usage said, and sleep ends ("usage").

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "sleep", main

STEP            = 100           ; Seconds a SLEEP at most (its ticks under 32768)

.code
main:
            jsr         tl_start
            MOVR        r0, tl_arg
            jsr         tl_atoi
            bcc         :+
            jmp         tl_badusage

:
            lda         tl_num + 2                          ; (65535 at most)
            ora         tl_num + 3
            beq         @left
            lda         #$FF
            sta         tl_num
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
@left:
            lda         tl_num + 1                          ; The seconds left: tl_num (16 bits: 65535 at most)
            ora         tl_num + 2
            ora         tl_num + 3
            bne         @step
            lda         tl_num
            beq         @end
            cmp         #STEP
            bcc         @some
@step:
            lda         #STEP
@some:
            pha                                             ; Those seconds' ticks: .A * TICK_HZ
            sta         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #8                                  ; (r1 = r0 * TICK_HZ, shift and add)
            lda         #TICK_HZ
            sta         r2
@bit:
            lsr         r2
            bcc         :+
            clc
            lda         r1
            adc         r0
            sta         r1
            lda         r1 + 1
            adc         r0 + 1
            sta         r1 + 1
:
            asl         r0
            rol         r0 + 1
            dex
            bne         @bit
            lda         r1
            ldx         r1 + 1
            jsr         SLEEP
            pla
            bcs         @end                                ; (A note)
            eor         #$FF                                ; The seconds left: less those
            sec
            adc         tl_num
            sta         tl_num
            lda         tl_num + 1
            sbc         #0
            sta         tl_num + 1
            bra         @left

@end:
            jmp         tl_end

.rodata
tl_name:    .byte       "sleep", 0
tl_flagset: .byte       0
tl_usage:   .byte       "sleep seconds", 0

.include "toollib.s"
