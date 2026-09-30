.debuginfo

; ****************************************************************************
; Semaphores (BIOS ROM page 5, included inside `.scope PAGE5`, see all.s): counting semaphores and mutexes,
; for tasks that share something (memory, a device) or wait for each other: SEM_NEW, SEM_ACQUIRE, SEM_TRY,
; SEM_RELEASE and SEM_FREE (thunks $F8D8-$F8E4).  A semaphore is a count and the tasks waiting for it:
; SEM_ACQUIRE takes one, or waits (using no CPU) until SEM_RELEASE gives one back.  A mutex (SEM_MUTEX) is
; a semaphore of 1 that has a holder: only the task that took it can release it.
;   They're in the system's shared bank (SEM_TABLE: kernel.inc), so every task sees the same ones, by
; number (1-SEM_MAX).  The calls work on the table with IRQs off (a few hundred cycles at most), and a
; wait is the kernel's usual one: the task's bit in the semaphore's wait mask, TASK_WAITING_FLAG, YIELD,
; then look again.  A release wakes every waiter (TASK_WAKE_MASK): the first to run takes it, and the
; others wait again.  A break or kill ends a wait, as it ends any other.
;   When a task ends (MM_TASK_RESET: SEM_RESET_TASK), the semaphores it made are freed (the tasks waiting
; for them get ERR_SEM_BAD) and the mutexes it holds are released.
; All calls: C = 0; or C = 1 with the error in .A.  They preserve .X and .Y (and the caller's I flag).

.segment "FP_P5"

SEM_Z           = ZP_SLEEP_SCAN                             ; (2) Scratch: a wait mask to wake, or whether
                                                            ;   to wait.  Only with IRQs off and no YIELD
                                                            ;   in between (so as in SLEEP_CHECK, the system
                                                            ;   task's): what a call keeps across a wait
                                                            ;   is on its stack

.assert     SEM_TABLE >= ENV_VALBUF + 256 .and SEM_TABLE + SEM_MAX * .sizeof(Sem) <= $8400, error, "SEM_TABLE: after ENV_VALBUF, below SH_HANDLES"

; ****************************************************************************
; At boot (os_main, after MMU_INIT: the system's shared bank is there): every semaphore free, and every exit
; record empty (exits.s).  Modifies: .A, .X, .Y
SEM_INIT:
            php
            sei
            _M_SYS_ENTER
            ldx         #SEM_MAX * .sizeof(Sem) - 1
            lda         #SEM_NONE
:
            sta         SEM_TABLE,X
            dex
            bpl         :-
            jsr         EXIT_INIT
            _M_SYS_LEAVE
            plp
            rts

; Make a semaphore.  IN: .A = its count: how many can take it before a task has to wait (0-255); .Y = 0,
; or SEM_MUTEX (a mutex: its count is 1).  OUT: .A = the semaphore (1-SEM_MAX); or ERR_SEM_NONE
SEM_NEW:
            php
            sei
            phx
            phy
            sta         SEM_Z                               ; The count
            sty         SEM_Z + 1                           ;   and the flags
            _M_SYS_ENTER
            ldy         #0                                  ; .Y = the semaphore - 1, .X = its entry

@find:
            ldx         SEM_OFS,Y
            lda         SEM_TABLE + Sem::maker,X
            cmp         #SEM_NONE
            beq         @found
            iny
            cpy         #SEM_MAX
            bne         @find
            lda         #ERR_SEM_NONE
            sec
            bra         @leave

@found:
            lda         T_REGISTER
            and         #$0F
            sta         SEM_TABLE + Sem::maker,X
            lda         #SEM_NONE
            sta         SEM_TABLE + Sem::holder,X
            stz         SEM_TABLE + Sem::waiters,X
            stz         SEM_TABLE + Sem::waiters + 1,X
            lda         SEM_Z + 1
            and         #SEM_MUTEX
            sta         SEM_TABLE + Sem::flags,X
            beq         :+
            lda         #1                                  ; (A mutex: 1)
            sta         SEM_Z
:
            lda         SEM_Z
            sta         SEM_TABLE + Sem::count,X
            iny
            tya                                             ; The semaphore
            clc

@leave:
            _M_SYS_LEAVE                                    ; (Keeps .A and C)
            jmp         SEM_RETURN

; Take one of semaphore .A: if there's none, wait until a release gives one back (SEM_ACQUIRE), or return
; ERR_SEM_BUSY (SEM_TRY).  A mutex's holder is then this task.  OUT: C = 0; or ERR_SEM_BAD (it isn't a
; semaphore, or it was freed while this task waited), ERR_SEM_BUSY
SEM_ACQUIRE:
            php
            sei
            phx
            phy
            pha                                             ; (The semaphore: kept across the waits)

@look:
            pla
            pha
            ldy         #1                                  ; (None: join its waiters)
            jsr         SEM_GRAB
            bcc         @done
            cmp         #ERR_SEM_BUSY
            beq         @wait
            sec                                             ; (ERR_SEM_BAD: the cmp changed C)
            bra         @done

@wait:
            smb2        TASK_STATUS_REG                     ; Wait (TASK_WAITING_FLAG) until a release
            jsr         YIELD                               ;   wakes us, then look again
            bra         @look

@done:
            plx                                             ; (Its copy: .X comes back in SEM_RETURN)
            jmp         SEM_RETURN

SEM_TRY:
            php
            sei
            phx
            phy
            ldy         #0                                  ; (None: say so)
            jsr         SEM_GRAB
            jmp         SEM_RETURN

; Take one of semaphore .A, if there is one; if there isn't and .Y <> 0, this task joins its waiters (it
; still has to wait: TASK_WAITING_FLAG, YIELD).  IRQs off.  OUT: C = 0; or C = 1, .A = ERR_SEM_BAD, or
; ERR_SEM_BUSY (none).  Modifies: .A, .X, .Y
SEM_GRAB:
            sty         SEM_Z                               ; (Join?)
            _M_SYS_ENTER
            jsr         SEM_ENTRY                           ; .X = its entry
            bcs         @leave
            lda         SEM_TABLE + Sem::count,X
            beq         @none
            dec         SEM_TABLE + Sem::count,X
            lda         T_REGISTER                          ; (Its holder, if it's a mutex: this task)
            and         #$0F
            sta         SEM_TABLE + Sem::holder,X
            clc
            bra         @leave

@none:
            lda         SEM_Z
            beq         @busy
            jsr         SEM_MY_BIT                          ; This task's bit in its wait mask
            bcs         @high
            ora         SEM_TABLE + Sem::waiters,X
            sta         SEM_TABLE + Sem::waiters,X
            bra         @busy

@high:
            ora         SEM_TABLE + Sem::waiters + 1,X
            sta         SEM_TABLE + Sem::waiters + 1,X

@busy:
            lda         #ERR_SEM_BUSY
            sec

@leave:
            _M_SYS_LEAVE
            rts

; Give one back to semaphore .A, and wake the tasks waiting for it.  A mutex: only its holder can.
; OUT: C = 0; or ERR_SEM_BAD, ERR_SEM_NOT_HELD (a mutex this task doesn't hold), ERR_SEM_FULL (its count
; is 255)
SEM_RELEASE:
            php
            sei
            phx
            phy
            _M_SYS_ENTER
            jsr         SEM_ENTRY
            bcs         @leave
            lda         SEM_TABLE + Sem::flags,X
            bpl         @count                              ; (Not a mutex)
            lda         T_REGISTER
            and         #$0F
            cmp         SEM_TABLE + Sem::holder,X
            beq         :+
            lda         #ERR_SEM_NOT_HELD
            sec
            bra         @leave
:
            lda         #SEM_NONE                           ; (Nobody holds it now)
            sta         SEM_TABLE + Sem::holder,X

@count:
            lda         SEM_TABLE + Sem::count,X
            cmp         #$FF
            beq         @full
            inc         SEM_TABLE + Sem::count,X
            jsr         SEM_TAKE_WAITERS
            clc
            bra         @leave

@full:
            lda         #ERR_SEM_FULL
            sec

@leave:
            _M_SYS_LEAVE
            bcs         @done
            jsr         SEM_WAKE
            clc

@done:
            jmp         SEM_RETURN

; Free semaphore .A (any task can): its number can be made again.  The tasks waiting for it wake, and get
; ERR_SEM_BAD.  OUT: C = 0; or ERR_SEM_BAD
SEM_FREE:
            php
            sei
            phx
            phy
            _M_SYS_ENTER
            jsr         SEM_ENTRY
            bcs         @leave
            lda         #SEM_NONE
            sta         SEM_TABLE + Sem::maker,X
            jsr         SEM_TAKE_WAITERS
            clc

@leave:
            _M_SYS_LEAVE
            bcs         @done
            jsr         SEM_WAKE
            clc

@done:
            jmp         SEM_RETURN

; A task ends (MM_TASK_RESET): the semaphores it made are freed, and the mutexes it holds released; their
; waiters wake.  (A counting semaphore it took one of stays one down: nothing says who took what.)
; IN: .A = the task.  Preserves .A, .X, .Y
SEM_RESET_TASK:
            php
            sei
            pha
            phx
            phy
            and         #$0F
            sta         ZP_TEMP                             ; (The task)
            stz         SEM_Z                               ; The tasks to wake
            stz         SEM_Z + 1
            _M_SYS_ENTER
            ldy         #0

@each:
            ldx         SEM_OFS,Y
            lda         SEM_TABLE + Sem::maker,X
            cmp         #SEM_NONE
            beq         @next
            cmp         ZP_TEMP
            beq         @free
            lda         SEM_TABLE + Sem::flags,X
            bpl         @next                               ; (Not a mutex)
            lda         SEM_TABLE + Sem::holder,X
            cmp         ZP_TEMP
            bne         @next
            lda         #SEM_NONE                           ; A mutex it holds: released
            sta         SEM_TABLE + Sem::holder,X
            lda         #1
            sta         SEM_TABLE + Sem::count,X
            bra         @wake

@free:
            lda         #SEM_NONE                           ; One it made: freed
            sta         SEM_TABLE + Sem::maker,X

@wake:
            lda         SEM_TABLE + Sem::waiters,X
            tsb         SEM_Z
            lda         SEM_TABLE + Sem::waiters + 1,X
            tsb         SEM_Z + 1
            stz         SEM_TABLE + Sem::waiters,X
            stz         SEM_TABLE + Sem::waiters + 1,X

@next:
            iny
            cpy         #SEM_MAX
            bne         @each
            _M_SYS_LEAVE
            jsr         SEM_WAKE
            ply
            plx
            pla
            plp
            rts

; ****************************************************************************
; The calls' end: .Y and .X back (pushed after the call's php), then the caller's I flag, keeping .A and C
SEM_RETURN:
            ply
            plx
            bcs         @error
            plp
            clc
            rts

@error:
            plp
            sec
            rts

; .X = semaphore .A's entry, if it's one in use.  In the system's shared bank.  OUT: C = 0; or C = 1,
; .A = ERR_SEM_BAD.  Modifies: .A, .X
SEM_ENTRY:
            dec
            cmp         #SEM_MAX
            bcs         @bad                                ; (0 too: $FF)
            tax
            lda         SEM_OFS,X
            tax
            lda         SEM_TABLE + Sem::maker,X
            cmp         #SEM_NONE
            beq         @bad
            clc
            rts

@bad:
            lda         #ERR_SEM_BAD
            sec
            rts

; SEM_Z = the entry .X's waiters, and it has none now.  Modifies: .A
SEM_TAKE_WAITERS:
            lda         SEM_TABLE + Sem::waiters,X
            sta         SEM_Z
            lda         SEM_TABLE + Sem::waiters + 1,X
            sta         SEM_Z + 1
            stz         SEM_TABLE + Sem::waiters,X
            stz         SEM_TABLE + Sem::waiters + 1,X
            rts

; Wake the tasks in SEM_Z.  Modifies: .A, .X, .Y
SEM_WAKE:
            ldx         #SEM_Z
            jmp         TASK_WAKE_MASK

; .A = this task's bit in a wait mask, C = 1: in its high byte (tasks 8-15).  Modifies: .Y
SEM_MY_BIT:
            lda         T_REGISTER
            and         #$0F
            tay
            lda         SEM_BITS,Y
            cpy         #8
            rts

SEM_OFS:        .byte   0, 6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90
.assert     .sizeof(Sem) = 6 .and SEM_MAX = 16, error, "SEM_OFS: an entry's offset for each semaphore"
SEM_BITS:       .byte   $01, $02, $04, $08, $10, $20, $40, $80, $01, $02, $04, $08, $10, $20, $40, $80
