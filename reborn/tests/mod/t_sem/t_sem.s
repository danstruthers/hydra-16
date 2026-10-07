; ****************************************************************************
; t_sem - semaphores and GETPPID, run as init: counting semaphores and mutexes taken, tried, released and freed;
; tasks waiting for one (using no CPU), woken by a release one at a time, by a free (E_INVAL), by a note (E_INTR);
; a task's end freeing what it made and giving back the mutexes it held; the limits.  Its children are itself,
; started with an argument: a letter, then a hex byte (hh), as t_child's:
;   "a" hh      SEM_ACQUIRE hh: end with "a" ($E0 + the error, if one)
;   "i" hh      the same, with a note handler that keeps the note and goes on
;   "r" hh      SEM_RELEASE hh: end with 0 ($E0 + the error, if one)
;   "m"         SEM_NEW 1: end with the semaphore (it's freed as this task ends)
;   "h" hh      SEM_ACQUIRE mutex hh and end holding it (its end gives it back)
;   "p"         end with GETPPID's answer

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_sem", main

ST_FREE         = 0                                         ; TASKINFO's states
ST_WAIT         = 2

.zeropage
argp:       .res        2
param:      .res        1
sem:        .res        1                                   ; A counting semaphore (count 0 at first)
mutex:      .res        1
kid:        .res        1                                   ; A child
kid2:       .res        1
n:          .res        1

.bss
args:       .res        5                                   ; A child's argument: "xhh", its 0, the empty one
info:       .res        TI_SIZE

.code
main:
            MOVR        argp, r0
            lda         (argp)
            beq         parent
            jmp         child

parent:
            stz         T_FAILS

; ---- GETPPID
            jsr         GETPPID
            EXPECT_A    $FF, "GETPPID: init has no parent"
            lda         #'p'
            jsr         spawn
            jsr         wait_kid
            EXPECT_A    1, "GETPPID in a child: init's task"

; ---- A counting semaphore
            lda         #0
            ldx         #0
            jsr         SEM_NEW
            sta         sem
            EXPECT_OK   "SEM_NEW, a count of 0"
            lda         sem
            jsr         SEM_TRY
            EXPECT_ERR  E_AGAIN, "SEM_TRY with none to take: E_AGAIN"
            lda         sem
            jsr         SEM_RELEASE
            EXPECT_OK   "SEM_RELEASE"
            lda         sem
            jsr         SEM_TRY
            EXPECT_OK   "SEM_TRY takes the one given back"
            lda         sem
            jsr         SEM_TRY
            EXPECT_ERR  E_AGAIN, "and there's none again"

; ---- A mutex
            lda         #5                                  ; (Its count is 1 whatever .A is)
            ldx         #SEM_MUTEX
            jsr         SEM_NEW
            sta         mutex
            EXPECT_OK   "SEM_NEW, a mutex"
            lda         mutex
            jsr         SEM_ACQUIRE
            EXPECT_OK   "SEM_ACQUIRE of the mutex"
            lda         mutex
            jsr         SEM_ACQUIRE
            EXPECT_ERR  E_BUSY, "SEM_ACQUIRE again by its holder: E_BUSY (not a wait for ever)"
            lda         mutex
            jsr         SEM_TRY
            EXPECT_ERR  E_BUSY, "SEM_TRY by its holder: E_BUSY"
            lda         #'r'
            ldx         mutex
            jsr         spawn
            jsr         wait_kid
            EXPECT_A    $E0 + E_PERM, "SEM_RELEASE of a mutex by a task that doesn't hold it: E_PERM"

; ---- A task waits for the mutex, using no CPU, till its holder gives it back
            lda         #'h'
            ldx         mutex
            jsr         spawn
            sta         kid
            jsr         settle
            jsr         kid_state
            EXPECT_A    ST_WAIT, "a child's SEM_ACQUIRE of a held mutex waits"
            lda         mutex
            jsr         SEM_RELEASE
            EXPECT_OK   "SEM_RELEASE by the holder"
            jsr         wait_kid
            EXPECT_A    'a', "and the child takes it, and ends"
            lda         mutex
            jsr         SEM_TRY
            EXPECT_OK   "its end gave the mutex back: SEM_TRY takes it"
            lda         mutex
            jsr         SEM_RELEASE
            EXPECT_OK   "and gives it back"

; ---- Two tasks wait for a count of 0: a release wakes one, the next the other
            lda         #'a'
            ldx         sem
            jsr         spawn
            sta         kid
            lda         #'a'
            ldx         sem
            jsr         spawn
            sta         kid2
            jsr         settle
            jsr         kid_state
            EXPECT_A    ST_WAIT, "two children wait for the count of 0: the first"
            lda         kid2
            jsr         task_state
            EXPECT_A    ST_WAIT, "and the second"
            lda         sem
            jsr         SEM_RELEASE
            jsr         settle
            stz         n                                   ; How many have ended?
            jsr         kid_state
            bne         :+
            inc         n
:
            lda         kid2
            jsr         task_state
            bne         :+
            inc         n
:
            lda         n
            EXPECT_A    1, "one release: one of them takes it and ends, the other waits on"
            lda         sem
            jsr         SEM_RELEASE
            jsr         wait_kid
            EXPECT_A    'a', "the next release: the other"
            jsr         wait_kid
            EXPECT_A    'a', "both ended with it"
            lda         sem
            jsr         SEM_TRY
            EXPECT_ERR  E_AGAIN, "and the count is 0"

; ---- A note ends a wait: E_INTR (the handler goes on), and the task is no longer a waiter
            lda         #'i'
            ldx         sem
            jsr         spawn
            sta         kid
            jsr         settle
            lda         kid
            ldx         #NOTE_INTERRUPT
            jsr         NOTE
            jsr         wait_kid
            EXPECT_A    $E0 + E_INTR, "a note to a task waiting for a semaphore: E_INTR"
            lda         sem
            jsr         SEM_RELEASE
            lda         sem
            jsr         SEM_TRY
            EXPECT_OK   "and a release after it isn't taken by the task that left"

; ---- SEM_FREE wakes the waiters, to E_INVAL
            lda         #'a'
            ldx         sem
            jsr         spawn
            sta         kid
            jsr         settle
            lda         sem
            jsr         SEM_FREE
            EXPECT_OK   "SEM_FREE with a task waiting"
            jsr         wait_kid
            EXPECT_A    $E0 + E_INVAL, "the waiter wakes: E_INVAL"
            lda         sem
            jsr         SEM_TRY
            EXPECT_ERR  E_INVAL, "SEM_TRY of a freed semaphore: E_INVAL"
            lda         sem
            jsr         SEM_FREE
            EXPECT_ERR  E_INVAL, "SEM_FREE again: E_INVAL"

; ---- A task's end frees the semaphores it made
            lda         #'m'
            jsr         spawn
            jsr         wait_kid
            sta         n
            jsr         SEM_TRY
            EXPECT_ERR  E_INVAL, "a child's semaphore is freed as it ends"

; ---- The limits
            lda         #255
            ldx         #0
            jsr         SEM_NEW
            sta         sem
            jsr         SEM_RELEASE
            EXPECT_ERR  E_RANGE, "SEM_RELEASE of a count of 255: E_RANGE"
            lda         sem
            jsr         SEM_FREE
            lda         #SEM_MAX
            jsr         SEM_TRY
            EXPECT_ERR  E_INVAL, "SEM_TRY of SEM_MAX: E_INVAL"
            lda         #$FF
            jsr         SEM_RELEASE
            EXPECT_ERR  E_INVAL, "SEM_RELEASE of $FF: E_INVAL"
            stz         n                                   ; As many as there can be
:
            lda         #1
            ldx         #0
            jsr         SEM_NEW
            bcs         :+
            inc         n
            bra         :-
:
            EXPECT_ERR  E_NOMEM, "SEM_NEW past the last: E_NOMEM"
            lda         n
            EXPECT_A    SEM_MAX - 1, "SEM_MAX in all (the mutex one of them)"
            ldx         #SEM_MAX - 1                        ; All freed again
:
            phx
            txa
            jsr         SEM_FREE
            plx
            dex
            bpl         :-
            lda         #0
            ldx         #0
            jsr         SEM_NEW
            EXPECT_A    0, "freed, the lowest is made again first"

            DONE        "t_sem"

; ---- The parent's helpers

; Start this module as a child, with the argument .A (a letter) and .X (its hex byte).  OUT: .A = the task
spawn:
            sta         args
            txa
            jsr         hexbyte
            stz         args + 3
            stz         args + 4
            LDR         r0, s_self
            LDR         r1, args
            lda         #0
            jsr         SPAWN
            rts

; Wait for a child to end.  OUT: .A = its exit code
wait_kid:
            stz         r0
            stz         r0 + 1
            lda         #$FF
            jsr         WAIT
            txa
            rts

; Sleep a few ticks: the children run meanwhile
settle:
            lda         #4
            ldx         #0
            jmp         SLEEP

; .A = the child kid's state (TASKINFO: ST_FREE once it's ended); Z from it
kid_state:
            lda         kid
task_state:
            pha
            LDR         r0, info
            pla
            jsr         TASKINFO
            lda         info + TI_STATE
            rts

; .A as two hex digits at args + 1
hexbyte:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         @digit
            sta         args + 1
            pla
            jsr         @digit
            sta         args + 2
            rts

@digit:
            and         #$0F
            cmp         #10
            bcc         :+
            adc         #'a' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            rts

; ---- A child: its letter, then its hex byte
child:
            ldy         #1
            jsr         hex
            sta         param
            lda         (argp)
            cmp         #'a'
            beq         c_acquire
            cmp         #'h'
            beq         c_acquire
            cmp         #'i'
            beq         c_note
            cmp         #'r'
            beq         c_release
            cmp         #'m'
            beq         c_make
            cmp         #'p'
            beq         c_parent
            lda         #$EE
            bra         end

c_note:
            LDR         r0, keep
            jsr         NOTIFY
c_acquire:
            lda         param
            jsr         SEM_ACQUIRE
            bcs         error
            lda         #'a'
            bra         end

c_release:
            lda         param
            jsr         SEM_RELEASE
            bcs         error
            lda         #0
            bra         end

c_make:
            lda         #1
            ldx         #0
            jsr         SEM_NEW
            bra         end

c_parent:
            jsr         GETPPID
            bra         end

error:
            ora         #$E0
end:
            stz         r0
            stz         r0 + 1
            jmp         EXITS

keep:                                                       ; The note handler: go on
            clc
            rts

; .A = the hex byte at (argp),Y (0: none)
hex:
            jsr         @digit
            bcs         @none
            asl
            asl
            asl
            asl
            sta         param
            iny
            jsr         @digit
            bcs         @none
            ora         param
            rts

@none:
            lda         #0
            rts

@digit:                                                     ; C = 0 and .A = a digit's value, or C = 1
            lda         (argp),Y
            sec
            sbc         #'0'
            cmp         #10
            bcc         @ok
            sbc         #'a' - '0' - 10                     ; (Lower case, C = 1)
            cmp         #10
            bcc         @bad
            cmp         #16
            bcs         @bad
@ok:
            clc
            rts

@bad:
            sec
            rts

.rodata
s_self:     .byte       "#m/t_sem", 0
