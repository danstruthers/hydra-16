; ****************************************************************************
; top - the tasks in use each second (TASKINFO), the terminal cleared and each drawn again: its number, state, the
; CPU it took in that second (%) and its name; till a note (Ctrl-C) ends it.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "top", main

.bss
task:       .res        1
info:       .res        TI_SIZE
before:     .res        3 * 16                              ; Each task's CPU time (ticks) a second ago
then:       .res        2                                   ; The tick count then ...
gone:       .res        2                                   ;   and the ticks since

.code
main:
            jsr         tl_start
            jsr         look                                ; (Each one's time, to start from)
@second:
            lda         #<TICK_HZ
            ldx         #>TICK_HZ
            jsr         SLEEP
            bcs         @end                                ; (A note)
            jsr         TICKS                               ; The ticks gone (about TICK_HZ)
            sec
            sbc         then
            sta         gone
            txa
            sbc         then + 1
            sta         gone + 1
            LDR         r0, s_clear
            jsr         tl_puts
            LDR         r0, s_head
            jsr         tl_puts
            jsr         look
            jsr         tl_flush
            bra         @second

@end:
            jmp         tl_end

; Each task in use: its line (if there's a second to show), and its time now kept
look:
            jsr         TICKS
            sta         then
            stx         then + 1
            stz         task
@task:
            LDR         r0, info
            lda         task
            jsr         TASKINFO
            bcs         @next
            lda         task                                ; (The kernel task's, always)
            beq         :+
            lda         info + TI_STATE
            beq         @next
:
            lda         gone                                ; (None the first time)
            ora         gone + 1
            beq         :+
            jsr         line
:
            lda         task                                ; Its time, for the next
            asl
            adc         task
            tax
            lda         info + TI_CPU
            sta         before,X
            lda         info + TI_CPU + 1
            sta         before + 1,X
            lda         info + TI_CPU + 2
            sta         before + 2,X
@next:
            inc         task
            lda         task
            cmp         #16
            bne         @task
            rts

; Task task's line: "  3  ready    12%  rc"
line:
            lda         task
            jsr         tl_setnum
            lda         #4
            jsr         tl_dec
            jsr         tl_space
            jsr         tl_space
            lda         #STATES + 1                         ; Its state ("stopped", whatever it is)
            bit         info + TI_FLAGS
            bmi         :+
            lda         info + TI_STATE
            cmp         #STATES
            bcc         :+
            lda         #STATES
:
            asl
            tax
            lda         states,X
            sta         r0
            lda         states + 1,X
            sta         r0 + 1
            lda         #8
            jsr         tl_field
            lda         task                                ; Its share: (now - before) * 100 / gone
            asl
            adc         task
            tax
            sec
            lda         info + TI_CPU
            sbc         before,X
            sta         tl_num
            lda         info + TI_CPU + 1
            sbc         before + 1,X
            sta         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            ldx         #0                                  ; (* 100: * 4 + * 32 + * 64)
            stz         tl_den
            stz         tl_den + 1
            stz         tl_den + 2
            ldy         #7
@bit:
            lda         s_100bits,X
            beq         :+
            clc
            lda         tl_den
            adc         tl_num
            sta         tl_den
            lda         tl_den + 1
            adc         tl_num + 1
            sta         tl_den + 1
            lda         tl_den + 2
            adc         tl_num + 2
            sta         tl_den + 2
:
            asl         tl_num
            rol         tl_num + 1
            rol         tl_num + 2
            inx
            dey
            bne         @bit
            lda         tl_den
            sta         tl_num
            lda         tl_den + 1
            sta         tl_num + 1
            lda         tl_den + 2
            sta         tl_num + 2
            stz         tl_num + 3
            lda         gone
            ldx         gone + 1
            ldy         #0
            jsr         tl_by
            lda         #3
            jsr         tl_dec
            LDR         r0, s_pct
            jsr         tl_puts
            LDR         r0, info + TI_NAME
            jsr         tl_puts
            jmp         tl_nl

.rodata
s_100bits:  .byte       0, 0, 1, 0, 0, 1, 1                 ; (100: bits 2, 5, 6)
STATES      = 9                                             ; (TASKINFO's states: 0-8, any other; stopped)
states:     .word       s_free, s_ready, s_wait, s_call, s_idle, s_new, s_sleep, s_blocked, s_event, s_other
            .word       s_stopped
s_free:     .byte       "free", 0
s_ready:    .byte       "ready", 0
s_wait:     .byte       "wait", 0
s_call:     .byte       "call", 0
s_idle:     .byte       "idle", 0
s_new:      .byte       "new", 0
s_sleep:    .byte       "sleep", 0
s_blocked:  .byte       "blocked", 0
s_event:    .byte       "event", 0
s_other:    .byte       "?", 0
s_stopped:  .byte       "stopped", 0
s_pct:      .byte       "%  ", 0
s_clear:    .byte       $1B, "[H", $1B, "[2J", 0            ; (ANSI: home, and clear)
s_head:     .byte       "task  state    cpu  name", LF, 0
tl_name:    .byte       "top", 0
tl_flagset: .byte       0
tl_usage:   .byte       "top", 0

.include "toollib.s"
