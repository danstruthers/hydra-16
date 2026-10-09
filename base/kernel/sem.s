; ****************************************************************************
; sem.s - semaphores (docs/design/reimplementation-from-scratch.md, §21: kept from the old system, in the kernel task).
;
; A semaphore is a count and the tasks waiting for it, in the kernel task's tables (K_SEM_*: layout.inc), so every
; task sees the same ones, by number (0 to SEM_MAX - 1).  SEM_ACQUIRE takes one of the count, or waits (using no
; CPU) till SEM_RELEASE gives one back; SEM_TRY takes one or answers E_AGAIN.  A mutex (SEM_MUTEX) is a semaphore
; of 1 with a holder: only the task that took it gives it back, and it can't take it twice (E_BUSY).
;   Each call is a KCALL, so the tables need no locks.  A wait is the kernel's usual one: the task's bit in the
; semaphore's waiters, PAUSE, then a look again; a release wakes every waiter (WAKE), the first to run takes it, and
; the others wait again.  A note ends a wait with E_INTR (the task no longer a waiter).  When a task ends
; (K_SEM_EXIT, from K_EXIT_K), the semaphores it made are freed and the mutexes it holds given back; their waiters
; wake (those of one freed to E_INVAL).  A counting semaphore it took one of stays one down: nothing says who took
; what.

.include "kdefs.inc"

.segment "KCODE_P1"

.assert     SEM_MAX = TASKS, error, "The boot sets the semaphores up in its loop over the tasks"

; ****************************************************************************
; The calls (in the calling task), each a KCALL

; SEM_NEW: a semaphore.  IN: .A = its count (0-255); .X = 0, or SEM_MUTEX.  OUT: .A = the semaphore; or C = 1,
; .A = E_NOMEM (none free)
K_SEM_NEW:
            KCALL_FAR   K_SEM_NEW_K
            rts

; SEM_ACQUIRE: take one of semaphore .A, waiting till there's one.  OUT: C = 0; or C = 1, .A = E_INVAL (not a
; semaphore, or freed meanwhile), E_BUSY (a mutex this task holds), E_INTR (a note came: the far call's stub takes
; it on the way out)
K_SEM_ACQUIRE:
            pha                                             ; The semaphore, on the stack throughout
@take:
            stz         TK_WOKEN                            ; (A release from here on: the wait won't wait)
            pla
            pha
            ldx         #1                                  ; (None to take: this task a waiter)
            KCALL_FAR   K_SEM_TAKE_K
            bcc         @done
            cmp         #E_AGAIN
            sec
            bne         @done                               ; (E_INVAL, E_BUSY)
            lda         TK_NOTED
            bne         @intr
            FARCALL     K_PAUSE                             ; Till a release wakes it (or a note)
            lda         TK_NOTED
            beq         @take
@intr:
            pla                                             ; A note: no longer a waiter
            KCALL_FAR   K_SEM_UNWAIT_K
            lda         #E_INTR
            sec
            rts

@done:
            plx                                             ; (Keeps .A and C)
            rts

; SEM_TRY: take one of semaphore .A, if there's one.  OUT: C = 0; or C = 1, .A = E_INVAL, E_BUSY, E_AGAIN (none)
K_SEM_TRY:
            ldx         #0
            KCALL_FAR   K_SEM_TAKE_K
            rts

; SEM_RELEASE: give one back to semaphore .A, and wake its waiters.  OUT: C = 0; or C = 1, .A = E_INVAL, E_PERM
; (a mutex this task doesn't hold), E_RANGE (its count is 255)
K_SEM_RELEASE:
            KCALL_FAR   K_SEM_RELEASE_K
            rts

; SEM_FREE: free semaphore .A; its waiters wake (to E_INVAL).  OUT: C = 0; or C = 1, .A = E_INVAL
K_SEM_FREE:
            KCALL_FAR   K_SEM_FREE_K
            rts

; ---- In the kernel task (KCALLs): .Y = the calling task

K_SEM_NEW_K:
            sta         K0_TMP                              ; Its count
            txa
            and         #SEM_MUTEX
            sta         K0_TMP3                             ; Its flags
            beq         :+
            lda         #1                                  ; (A mutex: 1)
            sta         K0_TMP
:
            ldx         #0                                  ; A free one, from the lowest
@find:
            lda         K_SEM_MAKER,X
            bmi         @found                              ; ($FF: free)
            inx
            cpx         #SEM_MAX
            bne         @find
            FAIL        E_NOMEM

@found:
            tya
            sta         K_SEM_MAKER,X
            lda         K0_TMP
            sta         K_SEM_COUNT,X
            lda         K0_TMP3
            sta         K_SEM_FLAGS,X
            lda         #$FF
            sta         K_SEM_HOLDER,X
            stz         K_SEM_WAITLO,X
            stz         K_SEM_WAITHI,X
            txa
            clc
            rts

; Take one of semaphore .A; if there's none and .X <> 0, the task joins its waiters (it waits itself: SEM_ACQUIRE).
; OUT: C = 0; or C = 1, .A = E_INVAL, E_BUSY, E_AGAIN (none)
K_SEM_TAKE_K:
            sty         K0_TMP2                             ; The task
            stx         K0_TMP3
            jsr         s_sem
            bcs         @inval
            tya                                             ; (A mutex this task holds already?)
            cmp         K_SEM_HOLDER,X
            beq         @busy
            lda         K_SEM_COUNT,X
            beq         @none
            dec         K_SEM_COUNT,X
            lda         K_SEM_FLAGS,X
            and         #SEM_MUTEX
            beq         :+
            tya
            sta         K_SEM_HOLDER,X                      ; (A mutex: its holder)
:
            clc
            rts

@none:
            lda         K0_TMP3
            beq         @again
            jsr         s_join
@again:
            FAIL        E_AGAIN

@inval:
            FAIL        E_INVAL

@busy:
            FAIL        E_BUSY

; The task no longer waiting for semaphore .A (a note ended its wait).  OUT: C = 0
K_SEM_UNWAIT_K:
            sty         K0_TMP2
            jsr         s_sem
            bcs         :+                                  ; (Freed meanwhile: no waiters)
            jsr         s_leave
:
            clc
            rts

K_SEM_RELEASE_K:
            sty         K0_TMP2
            jsr         s_sem
            bcs         @inval
            lda         K_SEM_FLAGS,X
            and         #SEM_MUTEX
            beq         @count
            tya                                             ; A mutex: this task its holder?
            cmp         K_SEM_HOLDER,X
            bne         @perm
            lda         #$FF
            sta         K_SEM_HOLDER,X
@count:
            lda         K_SEM_COUNT,X
            inc         a
            beq         @range
            sta         K_SEM_COUNT,X
            jsr         s_wake
            clc
            rts

@inval:
            FAIL        E_INVAL

@perm:
            FAIL        E_PERM

@range:
            FAIL        E_RANGE

K_SEM_FREE_K:
            jsr         s_sem
            bcs         @inval
            lda         #$FF                                ; Free, and its waiters woken: they find it gone
            sta         K_SEM_MAKER,X
            jsr         s_wake
            clc
            rts

@inval:
            FAIL        E_INVAL

; A task's end (K_EXIT_K, FARCALL): task .Y's semaphores freed, the mutexes it holds given back, and it no longer
; a waiter.  Keeps K0_TMP (K_EXIT_K's)
K_SEM_EXIT:
            sty         K0_TMP2
            ldx         #SEM_MAX - 1
@sem:
            lda         K_SEM_MAKER,X
            bmi         @next                               ; (Free)
            jsr         s_leave
            lda         K_SEM_MAKER,X
            cmp         K0_TMP2
            bne         @held
            lda         #$FF                                ; Its own: freed
            sta         K_SEM_MAKER,X
            bra         @wake

@held:
            lda         K_SEM_HOLDER,X
            cmp         K0_TMP2
            bne         @next
            lda         #$FF                                ; A mutex it holds: given back
            sta         K_SEM_HOLDER,X
            lda         #1
            sta         K_SEM_COUNT,X
@wake:
            jsr         s_wake
@next:
            dex
            bpl         @sem
            rts

; ---- The helpers (in the kernel task)

; Semaphore .A in use?  OUT: C = 0, .X = it; or C = 1.  Keeps .Y
s_sem:
            cmp         #SEM_MAX
            bcs         @no
            tax
            lda         K_SEM_MAKER,X
            bmi         @no
            clc
            rts

@no:
            sec
            rts

; Task K0_TMP2 one of semaphore .X's waiters.  Keeps .X.  Modifies .A, .Y
s_join:
            lda         K0_TMP2
            and         #7
            tay
            lda         S_BIT8,Y
            ldy         K0_TMP2
            cpy         #8
            bcs         :+
            ora         K_SEM_WAITLO,X
            sta         K_SEM_WAITLO,X
            rts
:
            ora         K_SEM_WAITHI,X
            sta         K_SEM_WAITHI,X
            rts

; Task K0_TMP2 no longer one of semaphore .X's waiters.  Keeps .X.  Modifies .A, .Y
s_leave:
            lda         K0_TMP2
            and         #7
            tay
            lda         S_BIT8,Y
            eor         #$FF
            ldy         K0_TMP2
            cpy         #8
            bcs         :+
            and         K_SEM_WAITLO,X
            sta         K_SEM_WAITLO,X
            rts
:
            and         K_SEM_WAITHI,X
            sta         K_SEM_WAITHI,X
            rts

; Semaphore .X's waiters woken (WAKE: each looks again), and none now.  Keeps .X.  Modifies .A, .Y, K_PTR
s_wake:
            lda         K_SEM_WAITLO,X
            sta         K_PTR
            lda         K_SEM_WAITHI,X
            sta         K_PTR + 1
            stz         K_SEM_WAITLO,X
            stz         K_SEM_WAITHI,X
            ldy         #0                                  ; .Y = the task
@task:
            lda         K_PTR                               ; (None left?)
            ora         K_PTR + 1
            beq         @done
            lsr         K_PTR + 1
            ror         K_PTR
            bcc         @next
            tya
            FARCALL     K_WAKE                              ; (Keeps .A, .X, .Y)
@next:
            iny
            bra         @task

@done:
            rts

.segment "KRODATA_P1"

S_BIT8:     .byte       $01, $02, $04, $08, $10, $20, $40, $80
