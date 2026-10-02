; ****************************************************************************
; sched.s - the scheduler, waiting and waking, sleeping, the tick (docs/reimplementation-from-scratch.md, §10.8).
;
; A task that isn't running keeps one frame on its own stack, whatever stopped it (an interrupt, YIELD, a wait):
;   U, Y, W, X, A, P, PCL, PCH          (top first; TK_SP is just below it)
; A switch saves the stack pointer, picks the next task, writes T, loads that task's stack pointer and unwinds
; its frame (K_SCHED_RESUME).  Round robin over tasks 1-15; the kernel task (0) runs only when nothing else can,
; and sleeps the CPU (its idle loop: reset.s).  Only ST_READY runs (TK_STATE: the state of the context on top of
; the task's stack, so a driver serving a call is ST_READY while the call runs, and can be switched out).
;
; Waiting is always the same: set ST_WAIT, YIELD, and when woken (WAKE: ST_READY), look again.  A wake for
; nothing costs a look.  Two waits the scheduler ends itself, as it looks for the next task: ST_SLEEP, once the
; time in the kernel task's table has come (so the tick's interrupt does no more than count), and ST_BLOCKED, once
; the task it's waiting to call is free (so a call's end wakes nobody).

.include "kdefs.inc"

.segment "KCODE"

; ****************************************************************************
; YIELD: let the other tasks run.  Keeps every register and flag (the frame has them)
K_YIELD:
            php
            sei
            pha
            phx
            tsx
            inc         $0104,X                             ; The return address + 1: RTI's way (S + 4 = PCL)
            bne         :+
            inc         $0105,X
:
            lda         W_REGISTER                          ; The rest of the frame: W, Y, U
            pha
            phy
            lda         U_REGISTER
            pha
            stz         TK_DUE

; Switch: IRQs off, this task's whole frame on its stack
K_SCHED_SWITCH:
            inc         TK_PREEMPT                          ; (In the scheduler: an interrupt in its windows
            cli                                             ;   mustn't start a switch of its own.  A moment for
            nop                                             ;   interrupts: the frame is whole)
            sei
            jsr         K_SCHED_PICK                        ; .A = the next task
            dec         TK_PREEMPT
            stz         TK_DUE                              ; (A switch that came due meanwhile: this one)
            tay
            tsx                                             ; (Saved only now: an interrupt in the scheduler's
            stx         TK_SP                               ;   windows uses TK_SP as its own)
            sty         T_REGISTER                          ; ---- The next task
            ldx         TK_SP
            txs
            lda         TK_NOTED                            ; A note for it?  (notes.s)
            bne         K_SCHED_NOTED

; Unwind a frame: U and Y here, then W, X, A and RTI in the COMMON block
K_SCHED_RESUME:
            pla
            sta         U_REGISTER
            ply
            jmp         IRQ_EXIT

; A note pending: taken now (the trampoline, in the task) if the task is in its own code, W = 0 and its PC below
; $E000, and its handler isn't running (but for a kill); else when it is
K_SCHED_NOTED:
            lda         TK_INNOTE
            beq         :+
            lda         #1 << NOTE_KILL
            and         TK_NOTES
            beq         K_SCHED_RESUME
:
            tsx
            lda         $0100 + FR_W,X
            bne         K_SCHED_RESUME
            lda         $0100 + FR_PCH,X
            cmp         #>BIOS_BASE
            bcs         K_SCHED_RESUME
            jmp         K_NOTE_TRAMP

; The next task to run: the next ST_READY task after this one (1-15, round robin, this one last), a sleeper whose
; time has come counting as ready (it's made ST_READY); else the kernel task (to idle).  IN, OUT: IRQs off (a
; moment on between its looks at each task).  OUT: .A = the task.  Modifies .X, .Y
K_SCHED_PICK:
            ldy         T_REGISTER                          ; .Y = this task, throughout
            tya
            tax                                             ; .X = the candidate
            lda         #TASKS
            sta         TK_PICKS
@next:
            inx
            txa
            and         #TASKS - 1
            tax
            beq         @skip                               ; (The kernel task isn't in the round)
            stx         T_REGISTER                          ; A quick look at it
            lda         TK_STATE
            sty         T_REGISTER
            cmp         #ST_READY
            beq         @found
            cmp         #ST_SLEEP
            beq         @sleeper
            cmp         #ST_BLOCKED
            beq         @blocked
@skip:
            cli                                             ; (A moment for interrupts)
            nop
            sei
            dec         TK_PICKS
            bne         @next
            lda         #KERNEL_TASK                        ; Nobody: the idle task
            rts

@sleeper:                                                   ; Has its time come?  (Now - its time >= 0)
            stz         T_REGISTER                          ; (The kernel task's: the ticks, the wake times)
            lda         K0_TICKS
            cmp         K_WAKE_LO,X
            lda         K0_TICKS + 1
            sbc         K_WAKE_HI,X
            sty         T_REGISTER
            bmi         @skip
@ready:
            stx         T_REGISTER                          ; It can run: ready
            lda         #ST_READY
            sta         TK_STATE
            sty         T_REGISTER
@found:
            txa
            rts

@blocked:                                                   ; Is the task it's waiting to call free?  (Or gone:
            stx         T_REGISTER                          ;   its call finds out)
            lda         TK_BLOCKEDON
            sta         T_REGISTER
            lda         TK_BUSY
            sty         T_REGISTER                          ; (Z: from TK_BUSY)
            beq         @ready
            bra         @skip

; ****************************************************************************
; PREEMPT_OFF: hold the CPU (no task switch: interrupts go on).  They nest.  Keeps everything
K_PREEMPT_OFF:
            inc         TK_PREEMPT
            rts

; PREEMPT_ON: undo PREEMPT_OFF; a switch that came due meanwhile happens now.  Keeps everything
K_PREEMPT_ON:
            php
            sei
            pha
            lda         TK_PREEMPT
            beq         @done                               ; (Not holding it)
            dec         TK_PREEMPT
            bne         @done                               ; (Still nested)
            lda         TK_DUE
            beq         @done
            pla
            plp
            jmp         K_YIELD                             ; (It clears TK_DUE)

@done:
            pla
            plp
            rts

; ****************************************************************************
; PAUSE: wait until woken (WAKE).  Keeps everything
K_PAUSE:
            php
            sei
            pha
            lda         #ST_WAIT
            sta         TK_STATE
            pla
            jsr         K_YIELD
            plp
            rts

; WAKE: task .A, if it's waiting, can run.  From any task, or an irq entry.  Keeps everything
K_WAKE:
            php
            sei
            phx
            phy
            pha
            and         #TASKS - 1
            tax
            ldy         T_REGISTER
            stx         T_REGISTER                          ; A quick look
            lda         TK_STATE
            cmp         #ST_WAIT
            bne         :+
            lda         #ST_READY
            sta         TK_STATE
:
            sty         T_REGISTER
            pla
            ply
            plx
            plp
            clc
            rts

; ****************************************************************************
; Time.  The tick count is the kernel task's (K0_TICKS: 32 bits), counted by its irq entry
; (irq.s: K_KIRQ)

; TICKS: .A/.X = the tick count.  Modifies .Y
K_TICKS:
            php
            sei
            ldy         T_REGISTER
            stz         T_REGISTER
            lda         K0_TICKS
            ldx         K0_TICKS + 1
            sty         T_REGISTER
            plp
            clc
            rts

; SLEEP: .A/.X = ticks (0-32767).  OUT: C = 0; or C = 1, .A = E_INTR (a note woke it early: taken on the way out)
K_SLEEP:
            sta         K_PTR
            stx         K_PTR + 1
            jsr         K_TICKS
            clc
            adc         K_PTR
            pha
            txa
            adc         K_PTR + 1
            tax
            pla

; SLEEP_UNTIL: .A/.X = the tick count to wake at (at most 32767 ahead; a time that's passed: at once).  Its wake
; time goes in the kernel task's table, and the scheduler makes it ready once the time has come
K_SLEEP_UNTIL:
            php
            sei
            ldy         T_REGISTER
            stz         T_REGISTER                          ; ---- The kernel task's: the wake time, and now
            sta         K_WAKE_LO,Y
            txa
            sta         K_WAKE_HI,Y
            lda         K0_TICKS                            ; Come already?  (Now - the time >= 0)
            cmp         K_WAKE_LO,Y
            lda         K0_TICKS + 1
            sbc         K_WAKE_HI,Y
            sty         T_REGISTER                          ; ---- Back
            bpl         @done
            lda         #ST_SLEEP
            sta         TK_STATE
            jsr         K_YIELD
            lda         TK_NOTED                            ; Woken early by a note: E_INTR, and the note
            bne         @intr
@done:
            plp
            clc
            jmp         K_NOTE_CHECK

@intr:
            plp
            lda         #E_INTR
            sec
            jmp         K_NOTE_RETURN
