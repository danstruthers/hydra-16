; ****************************************************************************
; t_task - the tasks and the scheduler (phase 1), run as init: SPAWN and its errors, EXITS and WAIT (codes,
; messages, any child, orphans), SLEEP and SLEEP_UNTIL, preemption and PREEMPT_OFF, PAUSE and WAKE, running out
; of tasks.  With t_child.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_task", main

.zeropage
child:      .res        1
t0:         .res        2
count:      .res        1
sum:        .res        1
kids:       .res        16

.bss
msg:        .res        32
info:       .res        TI_SIZE                             ; (TASKINFO's answer)

.code

; SPAWN t_child with the arguments at label.  OUT: as SPAWN's
.macro CHILD    label
            LDR         r0, s_child
            LDR         r1, label
            lda         #0
            jsr         SPAWN
.endmacro

; WAIT for a child (what: #$FF for any), with no message wanted.  OUT: as WAIT's
.macro WAITFOR  what
            stz         r0
            stz         r0 + 1
            lda         what
            jsr         WAIT
.endmacro

main:
            stz         T_FAILS
            jsr         GETPID
            EXPECT_A    1, "init is task 1"

; ---- SPAWN's errors, and WAIT with no children
            LDR         r0, s_nope
            stz         r1
            stz         r1 + 1
            lda         #0
            jsr         SPAWN
            EXPECT_ERR  E_NOENT, "SPAWN of a module that isn't there: E_NOENT"
            LDR         r0, s_badpath
            jsr         SPAWN
            EXPECT_ERR  E_NODEV, "SPAWN of a path through a device that isn't there: E_NODEV"
            LDR         r0, s_long
            jsr         SPAWN
            EXPECT_ERR  E_NAMETOOLONG, "SPAWN of a path over 63 characters: E_NAMETOOLONG"
            lda         #$FF
            stz         r0
            stz         r0 + 1
            jsr         WAIT
            EXPECT_ERR  E_CHILD, "WAIT with no children: E_CHILD"

; ---- A child's code and message
            CHILD       s_e7
            sta         child                               ; (Before the check: it prints)
            EXPECT_OK   "SPAWN t_child e7"
            LDR         r0, msg
            lda         child
            jsr         WAIT
            EXPECT_OK   "WAIT for it"
            txa
            EXPECT_A    '7', "its exit code"
            lda         msg
            EXPECT_A    'e', "its message"
            lda         msg + 2
            EXPECT_A    0, "its message's end"

; ---- Three children, WAIT for any
            CHILD       s_e1
            CHILD       s_e2
            CHILD       s_e3
            stz         sum
            lda         #3
            sta         count
@any:
            lda         #$FF
            stz         r0
            stz         r0 + 1
            jsr         WAIT
            bcs         @anyfail
            txa
            clc
            adc         sum
            sta         sum
            dec         count
            bne         @any
@anyfail:
            lda         sum
            EXPECT_A    <('1' + '2' + '3'), "WAIT for any, three times: each child's code"
            WAITFOR     #$FF
            EXPECT_ERR  E_CHILD, "and then none: E_CHILD"

; ---- Sleeping
            jsr         TICKS
            sta         t0
            stx         t0 + 1
            lda         #20
            ldx         #0
            jsr         SLEEP
            jsr         elapsed
            cmp         #20
            bcc         @short
            cmp         #22
            bcc         @slept
@short:
            NOTOK       "SLEEP 20 ticks"
            bra         @until

@slept:
            OK          "SLEEP 20 ticks"
@until:
            jsr         TICKS
            sta         t0
            stx         t0 + 1
            sec                                             ; (A time that's passed)
            sbc         #5
            bcs         :+
            dex
:
            jsr         SLEEP_UNTIL
            jsr         elapsed
            EXPECT_A    0, "SLEEP_UNTIL a time that's passed: at once"

; ---- Preemption: a child that never yields can't keep the CPU
            jsr         TICKS
            sta         t0
            stx         t0 + 1
            CHILD       s_s28                               ; (40 ticks of spinning)
            sta         child
            lda         #5
            ldx         #0
            jsr         SLEEP
            jsr         elapsed
            cmp         #8
            bcc         :+
            NOTOK       "preempted: SLEEP 5 ends while a child spins"
            bra         @spun
:
            OK          "preempted: SLEEP 5 ends while a child spins"
@spun:
            WAITFOR     child
            jsr         elapsed
            cmp         #40
            bcs         :+
            NOTOK       "the spinning child ended after its 40 ticks"
            bra         @info
:
            OK          "the spinning child ended after its 40 ticks"

; ---- TASKINFO: a child spinning while this one sleeps gets the CPU time
@info:
            CHILD       s_s14                               ; (20 ticks of spinning)
            sta         child
            lda         #10
            ldx         #0
            jsr         SLEEP
            LDR         r0, info
            lda         child
            jsr         TASKINFO
            EXPECT_OK   "TASKINFO of a child"
            lda         info + TI_STATE
            EXPECT_A    1, "TASKINFO: it's ready (spinning)"
            lda         info + TI_PARENT
            EXPECT_A    1, "TASKINFO: its parent is init"
            lda         info + TI_NAME
            EXPECT_A    't', "TASKINFO: its name"
            lda         info + TI_CPU + 1
            ora         info + TI_CPU + 2
            bne         @cpubad
            lda         info + TI_CPU
            cmp         #8
            bcc         @cpubad
            cmp         #13
            bcs         @cpubad
            OK          "TASKINFO: its CPU time, 8-12 ticks of the 10 it had"
            bra         @cpudone

@cpubad:
            lda         info + TI_CPU
            NOTOK       "TASKINFO: its CPU time, 8-12 ticks of the 10 it had"
@cpudone:
            WAITFOR     child
            LDR         r0, info
            lda         #16
            jsr         TASKINFO
            EXPECT_ERR  E_SRCH, "TASKINFO of task 16: E_SRCH"

; ---- PREEMPT_OFF: a new child doesn't start till PREEMPT_ON
@preempt:
            jsr         PREEMPT_OFF
            jsr         TICKS
            sta         t0
            stx         t0 + 1
            CHILD       s_k                                 ; (Its code: the tick when it starts)
            sta         child
:
            jsr         elapsed                             ; (5 ticks of spinning, holding the CPU)
            cmp         #5
            bcc         :-
            jsr         PREEMPT_ON
            WAITFOR     child
            txa
            sec
            sbc         t0
            cmp         #5
            bcs         :+
            NOTOK       "PREEMPT_OFF holds the CPU: the child started after PREEMPT_ON"
            bra         @wake
:
            OK          "PREEMPT_OFF holds the CPU: the child started after PREEMPT_ON"

; ---- PAUSE and WAKE
@wake:
            CHILD       s_p
            sta         child
            lda         #2
            ldx         #0
            jsr         SLEEP
            lda         child
            jsr         WAKE
            WAITFOR     child
            txa
            EXPECT_A    'p', "a paused child, woken, ends"

; ---- Every task: then E_NOTASK
            stz         count
@more:
            CHILD       s_p
            bcs         @full
            ldx         count
            sta         kids,X
            inc         count
            lda         count
            cmp         #16
            bne         @more
@full:
            EXPECT_ERR  E_NOTASK, "SPAWN with every task in use: E_NOTASK"
            lda         count
            EXPECT_A    13, "13 tasks for programs (2-E: kdev is F)"
            lda         #2                                  ; (Time to pause, all of them)
            ldx         #0
            jsr         SLEEP
            ldx         count
@wakeall:
            lda         kids - 1,X
            jsr         WAKE
            dex
            bne         @wakeall
            stz         sum
@waitall:
            WAITFOR     #$FF
            bcs         :+
            inc         sum
            bra         @waitall
:
            lda         sum
            EXPECT_A    13, "each woken, each waited for"

; ---- An orphan: init (this task) inherits it
            CHILD       s_o
            WAITFOR     #$FF
            stx         sum
            WAITFOR     #$FF
            txa
            clc
            adc         sum
            EXPECT_A    <('o' + '9'), "a child's orphan comes to init: both waited for"
            WAITFOR     #$FF
            EXPECT_ERR  E_CHILD, "and no more"

            DONE        "t_task"

; .A = the ticks since t0 (low byte)
elapsed:
            jsr         TICKS
            sec
            sbc         t0
            rts

.rodata
s_child:    .byte       "#m/t_child", 0
s_nope:     .byte       "#m/nope", 0
s_badpath:  .byte       "#x/t_child", 0
s_long:     .byte       "#m/t_child_is_a_name_long_enough_to_make_this_path_over_63_characters", 0
s_e7:       .byte       "e7", 0
s_e1:       .byte       "e1", 0
s_e2:       .byte       "e2", 0
s_e3:       .byte       "e3", 0
s_s28:      .byte       "s28", 0
s_s14:      .byte       "s14", 0
s_k:        .byte       "k", 0
s_p:        .byte       "p", 0
s_o:        .byte       "o", 0
