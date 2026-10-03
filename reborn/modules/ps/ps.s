; ****************************************************************************
; ps [-a] - the tasks in use (TASKINFO), a line each under a heading: its number, state, parent, CPU time (in
; seconds, to a tenth), note group and name; -a: its arguments after its name (TASKREAD).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "ps", main

F_A             = $01           ; -a

.bss
task:       .res        1
info:       .res        TI_SIZE
args:       .res        ARGS_MAX

.code
main:
            jsr         tl_start
            LDR         r0, s_head
            jsr         tl_puts
            stz         task
@task:
            LDR         r0, info
            lda         task
            jsr         TASKINFO
            bcs         @next
            lda         task                                ; (The kernel task's, always)
            beq         :+
            lda         info + TI_STATE
            beq         @next                               ; (Free)
:
            jsr         line
@next:
            inc         task
            lda         task
            cmp         #16
            bne         @task
            jmp         tl_end

; Task task's line
line:
            lda         task                                ; Its number
            jsr         tl_setnum
            lda         #4
            jsr         tl_dec
            jsr         tl_space
            jsr         tl_space
            lda         info + TI_STATE                     ; Its state
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
            lda         info + TI_PARENT                    ; Its parent
            bpl         :+
            LDR         r0, s_none
            lda         #7
            jsr         tl_field
            bra         @cpu

:
            jsr         tl_setnum
            lda         #2
            jsr         tl_dec
            LDR         r0, s_none + 1
            lda         #5
            jsr         tl_field
@cpu:
            lda         info + TI_CPU                       ; Its CPU time: tenths of a second, then whole ones
            sta         tl_num
            lda         info + TI_CPU + 1
            sta         tl_num + 1
            lda         info + TI_CPU + 2
            sta         tl_num + 2
            stz         tl_num + 3
            lda         #TICK_HZ / 10
            ldx         #0
            ldy         #0
            jsr         tl_by
            lda         #10
            ldx         #0
            ldy         #0
            jsr         tl_by
            lda         tl_rem
            pha
            lda         #5
            jsr         tl_dec
            lda         #'.'
            jsr         tl_putc
            pla
            ora         #'0'
            jsr         tl_putc
            lda         info + TI_GROUP                     ; Its note group
            jsr         tl_setnum
            lda         #6
            jsr         tl_dec
            jsr         tl_space
            jsr         tl_space
            LDR         r0, info + TI_NAME                  ; Its name
            jsr         tl_puts
            lda         tl_flags                            ; -a: its arguments
            and         #F_A
            beq         @nl
            LDR         r0, args
            lda         task
            ldx         #TR_ARGS
            jsr         TASKREAD
            bcs         @nl
            ldy         #0
@arg:
            lda         args,Y                              ; (An empty one: the end)
            beq         @nl
            jsr         tl_space
@char:
            cpy         #ARGS_MAX
            bcs         @nl
            lda         args,Y
            iny
            cmp         #0
            beq         @arg
            jsr         tl_putc
            bra         @char

@nl:
            jmp         tl_nl

.rodata
STATES      = 9                                             ; (TASKINFO's states: 0-8, then any other)
states:     .word       s_free, s_ready, s_wait, s_call, s_idle, s_new, s_sleep, s_blocked, s_event, s_other
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
s_none:     .byte       "-", 0
s_head:     .byte       "task  state   parent     cpu group  name", LF, 0
tl_name:    .byte       "ps", 0
tl_flagset: .byte       "a", 0
tl_usage:   .byte       "ps [-a]", 0

.include "toollib.s"
