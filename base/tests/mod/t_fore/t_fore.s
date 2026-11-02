; ****************************************************************************
; t_fore - the foreground (TASKFORE, TF_FORE: sched.s), run as init: who may put a task in it (itself, a task it
; started; not the kernel task, a driver, a free task, its parent); TASKINFO's flag; a foreground task busy for
; good (never yielding, never waiting) beside one in the background, which still has its turns (the background's turn
; one pick in every FORE_RUN + 1), and ends; the flag gone as the busy one's killed.  Its children are itself,
; started with an argument, a letter:
;   "b"         busy for good: a loop, no call
;   "c"         20 ticks asleep, then count down 65536 three times: end with the time that took over its own CPU
;               time meanwhile (TASKINFO's, in ticks)
;   "p"         TASKFORE its parent: end with 0, or $E0 + the error
;   "w"         a server called for good: a space written to the console (ser's) again and again

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_fore", main

.zeropage
argp:       .res        2
self:       .res        1                                   ; This task
busy:       .res        1                                   ; The busy child
kid:        .res        1
n:          .res        1
cpu:        .res        1                                   ; The counter's CPU time (ticks)
t0:         .res        2                                   ; A tick count
el:         .res        2                                   ; The ticks since

.bss
args:       .res        3                                   ; A child's argument: the letter, its 0, the empty one
info:       .res        TI_SIZE

.code
main:
            MOVR        argp, r0
            lda         (argp)
            beq         parent
            jmp         child

parent:
            stz         T_FAILS
            jsr         GETPID
            sta         self

; ---- Who may
            lda         #0
            ldx         #1
            jsr         TASKFORE
            EXPECT_ERR  E_PERM, "TASKFORE of the kernel task: E_PERM"
            lda         #$0F
            ldx         #1
            jsr         TASKFORE
            EXPECT_ERR  E_PERM, "TASKFORE of a driver (ser, task F): E_PERM"
            lda         #$0E                                ; (A free task: the highest)
            sta         kid
:           lda         kid
            jsr         state
            beq         :+
            dec         kid
            bne         :-
:           lda         kid
            ldx         #1
            jsr         TASKFORE
            EXPECT_ERR  E_SRCH, "TASKFORE of a free task: E_SRCH"
            lda         #16
            ldx         #1
            jsr         TASKFORE
            EXPECT_ERR  E_SRCH, "TASKFORE of task 16: E_SRCH"
            lda         #'p'
            jsr         spawn
            jsr         wait_kid
            EXPECT_A    $E0 + E_PERM, "a child's TASKFORE of its parent: E_PERM"

; ---- Itself
            lda         self
            ldx         #1
            jsr         TASKFORE
            EXPECT_OK   "TASKFORE of itself, in"
            lda         self
            jsr         flags
            and         #TF_FORE
            EXPECT_A    TF_FORE, "TASKINFO's TI_FLAGS: TF_FORE"
            lda         self
            ldx         #0
            jsr         TASKFORE
            EXPECT_OK   "TASKFORE of itself, out"
            lda         self
            jsr         flags
            and         #TF_FORE
            EXPECT_A    0, "TF_FORE gone"

; ---- A foreground task busy for good, and one in the background
            lda         #'b'
            jsr         spawn
            sta         busy
            lda         #'c'
            jsr         spawn                               ; (The counter: it waits 20 ticks, then counts)
            lda         busy
            ldx         #1
            jsr         TASKFORE
            EXPECT_OK   "TASKFORE of a child, busy for good"
            lda         busy
            jsr         flags
            and         #TF_FORE
            EXPECT_A    TF_FORE, "the child's TF_FORE"
            jsr         wait_kid                            ; (It ends: it isn't starved)
            sta         n
            OK          "the background task beside it ran, and ended"
            lda         n                                   ; Its time taken over its CPU time: the foreground
            cmp         #5                                  ;   first, five at least (the round robin alone: 2);
            bcs         :+                                  ;   and the background's turn after FORE_RUN ticks
            NOTOK       "the foreground first: the time taken 5 times the counter's CPU at least"
            bra         @most                               ;   of the foreground, FORE_RUN + 1 or so at most
:           OK          "the foreground first: the time taken 5 times the counter's CPU at least"
@most:
            lda         n
            cmp         #FORE_RUN + 4
            bcc         :+
            NOTOK       "the background's turns: the time taken FORE_RUN + 3 times its CPU at most"
            bra         @kill
:           OK          "the background's turns: the time taken FORE_RUN + 3 times its CPU at most"
@kill:
            lda         busy
            ldx         #NOTE_KILL
            jsr         NOTE
            EXPECT_OK   "the busy child killed"
            jsr         wait_kid
            OK          "and ended"
            lda         busy
            jsr         flags                               ; (Free: TASKINFO's answer has no TF_FORE)
            and         #TF_FORE
            EXPECT_A    0, "its TF_FORE gone with it"
            lda         #'p'                                ; A new child, in its task: not in the foreground
            jsr         spawn
            sta         kid
            jsr         flags
            and         #TF_FORE
            EXPECT_A    0, "a new child: not in the foreground"
            jsr         wait_kid

; ---- A foreground task calling a server for good (the console's writes), the server in the foreground a moment
; after each call (K0_FSRV, FORE_SRV): the background still has its turns
            lda         #'w'
            jsr         spawn
            sta         busy
            lda         #'c'
            jsr         spawn
            lda         busy
            ldx         #1
            jsr         TASKFORE
            EXPECT_OK   "TASKFORE of a child calling a server for good"
            jsr         wait_kid                            ; (It ends: neither starves it)
            sta         n
            OK          "the background task beside it and its server ran, and ended"
            lda         n
            cmp         #FORE_RUN + 4
            bcc         :+
            NOTOK       "beside a server's caller: the time taken FORE_RUN + 3 times its CPU at most"
            bra         @kill2
:           OK          "beside a server's caller: the time taken FORE_RUN + 3 times its CPU at most"
@kill2:
            lda         busy
            ldx         #NOTE_KILL
            jsr         NOTE
            EXPECT_OK   "the server's caller killed"
            jsr         wait_kid
            OK          "and ended"

            DONE        "t_fore"

; ---- The parent's helpers

; Start this module as a child, with the argument .A (a letter).  OUT: .A = the task
spawn:
            sta         args
            stz         args + 1
            stz         args + 2
            LDR         r0, s_self
            LDR         r1, args
            lda         #0
            jmp         SPAWN

; Wait for a child to end.  OUT: .A = its exit code
wait_kid:
            stz         r0
            stz         r0 + 1
            lda         #$FF
            jsr         WAIT
            txa
            rts

; .A = task .A's state (TASKINFO's TI_STATE: 0, free); Z from it
state:
            pha
            LDR         r0, info
            pla
            jsr         TASKINFO
            lda         info + TI_STATE
            rts

; .A = task .A's flags (TASKINFO's TI_FLAGS; 0 if it isn't a task in use)
flags:
            pha
            LDR         r0, info
            pla
            jsr         TASKINFO
            lda         #0
            bcs         :+
            lda         info + TI_FLAGS
:           rts

; r0 = .A times .X (16 bits)
times:
            sta         n
            stz         r0
            stz         r0 + 1
:           lda         r0
            clc
            adc         n
            sta         r0
            bcc         :+
            inc         r0 + 1
:           dex
            bne         :--
            rts

; ---- A child
child:
            lda         (argp)
            cmp         #'b'
            beq         c_busy
            cmp         #'c'
            beq         c_count
            cmp         #'p'
            beq         c_parent
            cmp         #'w'
            bne         :+
            jmp         c_write
:
            lda         #$EE
            jmp         end

c_busy:
            bra         c_busy                              ; (Busy for good: no call, no wait)

c_count:
            lda         #20                                 ; (The parent's TASKFORE first)
            ldx         #0
            jsr         SLEEP
            jsr         cpu_now
            jsr         TICKS
            sta         t0
            stx         t0 + 1
            lda         #3
            sta         n
            ldx         #0
            ldy         #0
:           dey
            bne         :-
            dex
            bne         :-
            dec         n
            bne         :-
            jsr         TICKS                               ; el = the ticks it took
            sec
            sbc         t0
            sta         el
            txa
            sbc         t0 + 1
            sta         el + 1
            lda         cpu                                 ; cpu = its CPU time meanwhile
            pha
            jsr         cpu_now
            pla
            sta         n
            lda         cpu
            sec
            sbc         n
            beq         @over                               ; (None: a ratio past any)
            sta         cpu
            ldx         #0                                  ; .X = el / cpu
:           lda         el
            sec
            sbc         cpu
            sta         el
            lda         el + 1
            sbc         #0
            sta         el + 1
            bcc         :+
            inx
            bne         :-
@over:
            ldx         #$FF
:           txa
            bra         end

; cpu = this task's CPU time in ticks (TASKINFO's, its low byte)
cpu_now:
            jsr         GETPID
            pha
            LDR         r0, info
            pla
            jsr         TASKINFO
            lda         info + TI_CPU
            sta         cpu
            rts

c_parent:
            jsr         GETPPID
            ldx         #1
            jsr         TASKFORE
            bcs         @err
            lda         #0
            bra         end

@err:
            ora         #$E0
end:
            stz         r0
            stz         r0 + 1
            jmp         EXITS

c_write:
            LDR         r0, s_space                         ; (A server called for good: ser's writes)
            LDR         r1, 1
            lda         #1
            jsr         WRITE
            bra         c_write

.rodata
s_self:     .byte       "#m/t_fore", 0
s_space:    .byte       " "
