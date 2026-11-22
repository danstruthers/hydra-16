; ****************************************************************************
; t_note - notes (phase 1.9), run as init with t_child: NOTE's errors; the defaults (Ctrl-C, kill, a BRK); a
; handler that goes on and one that asks for the default; a note to this task taken before NOTE returns; WAIT
; ended by a note (E_INTR); a note group, and SPAWN_NEWGROUP.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_note", main

.zeropage
child:      .res        1
child2:     .res        1
got:        .res        1                                   ; The note this task's handler got
lat:        .res        1                                   ; A kill's ticks, from NOTE to WAIT's end
was:        .res        1                                   ;   (and the tick as it was sent)
round:      .res        1                                   ; The TICKS spinners' round ...
worst:      .res        1                                   ;   their most ticks past the one in its own code ...
codes:      .res        1                                   ;   and their codes but 137 (or 0)

.bss
msg:        .res        32
info:       .res        TI_SIZE

.code

; SPAWN t_child with the arguments at label (flags .A).  OUT: as SPAWN's
.macro CHILD    label
            pha
            LDR         r0, s_child
            LDR         r1, label
            pla
            jsr         SPAWN
.endmacro

; WAIT for child what (#$FF: any), its message into msg.  OUT: as WAIT's
.macro WAITMSG  what
            LDR         r0, msg
            lda         what
            jsr         WAIT
.endmacro

main:
            stz         T_FAILS

; ---- NOTE's errors
            lda         #2
            ldx         #0
            jsr         NOTE
            EXPECT_ERR  E_INVAL, "note 0: E_INVAL"
            lda         #2
            ldx         #32
            jsr         NOTE
            EXPECT_ERR  E_INVAL, "note 32: E_INVAL"
            lda         #0
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            EXPECT_ERR  E_PERM, "a note to the kernel task: E_PERM"
            lda         #16
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            EXPECT_ERR  E_SRCH, "a note to task 16: E_SRCH"
            lda         #9
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            EXPECT_ERR  E_SRCH, "a note to a free task: E_SRCH"
            lda         #NOTE_GROUP | 9
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            EXPECT_ERR  E_SRCH, "a note to an empty group: E_SRCH"

; ---- The defaults
            lda         #0
            CHILD       s_p
            sta         child
            jsr         nap
            lda         child
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            EXPECT_OK   "NOTE: interrupt, to a child that waits"
            WAITMSG     child
            txa
            EXPECT_A    130, "with no handler it ends: 130"
            lda         msg
            EXPECT_A    'i', "and the message interrupt"

            lda         #0
            CHILD       s_s28                               ; (Spinning, in its own code: it never waits)
            sta         child
            jsr         nap
            jsr         kill
            EXPECT_A    137, "a kill, to a child that spins: 137"
            lda         msg
            EXPECT_A    'k', "and the message killed"
            lda         was
            sta         lat

            stz         worst                               ; Spinning on TICKS alone, in the kernel the most of its
            stz         codes                               ;   time: a note taken as the call returns (sched.s's
            lda         #8                                  ;   K_SCHED_NOTED, 2026-11-21), 8 times, each a tick
            sta         round                               ;   later than the last
@spin:
            lda         #0
            CHILD       s_sf0
            sta         child
            lda         round
            ldx         #0
            jsr         SLEEP
            jsr         kill
            cmp         #137
            beq         :+
            sta         codes
:
            lda         was
            sec
            sbc         lat
            bcc         :+
            cmp         worst
            bcc         :+
            sta         worst
:
            dec         round
            bne         @spin
            lda         codes
            EXPECT_A    0, "a kill, to a child that spins on TICKS: 137, 8 times"
            lda         worst
            cmp         #2
            lda         #0
            rol
            EXPECT_A    0, "and as soon as the one in its own code (a tick more at most)"

            lda         #0
            CHILD       s_b
            WAITMSG     #$FF
            txa
            EXPECT_A    133, "a BRK: the note sys: brk, 133"
            lda         msg + 5
            EXPECT_A    'b', "and the message sys: brk"

; ---- Handlers
            lda         #0
            CHILD       s_n
            sta         child
            jsr         nap
            lda         child
            ldx         #17
            jsr         NOTE
            WAITMSG     child
            txa
            EXPECT_A    17, "a handler that goes on: it got note 17, and went on"

            lda         #0
            CHILD       s_d
            sta         child
            jsr         nap
            lda         child
            ldx         #18
            jsr         NOTE
            WAITMSG     child
            txa
            EXPECT_A    128 + 18, "a handler that asks for the default: 128 + 18"
            lda         msg
            EXPECT_A    'n', "and the message note"
            lda         #0
            CHILD       s_d
            sta         child
            jsr         nap
            lda         child
            ldx         #NOTE_KILL
            jsr         NOTE
            WAITMSG     child
            txa
            EXPECT_A    137, "a kill isn't the handler's: 137"

; ---- This task's own handler
            LDR         r0, handler
            jsr         NOTIFY
            stz         got
            lda         #1
            ldx         #19
            jsr         NOTE
            php
            ldx         got
            stx         child2
            plp
            EXPECT_OK   "NOTE to this task"
            lda         child2
            EXPECT_A    19, "its handler ran before NOTE came back"

            stz         got                                 ; WAIT, ended by a note
            lda         #0
            CHILD       s_t                                 ; (It notes init with 20, then waits)
            sta         child
            stz         r0
            stz         r0 + 1
            lda         child
            jsr         WAIT
            EXPECT_ERR  E_INTR, "WAIT ended by a note: E_INTR"
            lda         got
            EXPECT_A    20, "and the handler got it"
            lda         child
            ldx         #NOTE_KILL
            jsr         NOTE
            WAITMSG     child
            txa
            EXPECT_A    137, "(and the child killed)"

; ---- Note groups
            stz         got
            lda         #0
            CHILD       s_p
            sta         child
            lda         #0
            CHILD       s_p
            sta         child2
            jsr         nap
            lda         #NOTE_GROUP | 1                     ; (Init's group: this task and its children)
            ldx         #21
            jsr         NOTE
            EXPECT_OK   "a note to this task's group"
            lda         got
            EXPECT_A    21, "this task's handler got it"
            WAITMSG     child
            txa
            EXPECT_A    128 + 21, "a child in the group got it"
            WAITMSG     child2
            txa
            EXPECT_A    128 + 21, "and the other"

            stz         got
            lda         #SPAWN_NEWGROUP
            CHILD       s_p
            sta         child
            LDR         r0, info
            lda         child
            jsr         TASKINFO
            lda         info + TI_GROUP
            cmp         child
            beq         :+
            NOTOK       "SPAWN_NEWGROUP: a group of its own"
            bra         @own
:
            OK          "SPAWN_NEWGROUP: a group of its own"
@own:
            jsr         nap
            lda         child
            ora         #NOTE_GROUP
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            WAITMSG     child
            txa
            EXPECT_A    130, "a note to its group ends it"
            lda         got
            EXPECT_A    0, "and not this task"

            DONE        "t_note"

; This task's note handler: keep the note, go on
handler:
            sta         got
            clc
            rts

; A moment for a child to start (and pause)
; A kill to child, and its WAIT: .A its code; was the ticks from NOTE to WAIT's end
kill:
            jsr         TICKS
            sta         was
            lda         child
            ldx         #NOTE_KILL
            jsr         NOTE
            WAITMSG     child
            phx
            jsr         TICKS
            sec
            sbc         was
            sta         was
            pla
            rts

nap:
            lda         #3
            ldx         #0
            jmp         SLEEP

.rodata
s_child:    .byte       "#m/t_child", 0
s_p:        .byte       "p", 0, 0
s_s28:      .byte       "s28", 0, 0
s_sf0:      .byte       "Sf0", 0, 0
s_b:        .byte       "b", 0, 0
s_n:        .byte       "n", 0, 0
s_d:        .byte       "d", 0, 0
s_t:        .byte       "t", 0, 0
