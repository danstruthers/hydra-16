; ****************************************************************************
; sched.s - the scheduler, waiting and waking, sleeping, the tick (docs/design/reimplementation-from-scratch.md, §10.8).
;
; A task that isn't running keeps one frame on its own stack, whatever stopped it (an interrupt, YIELD, a wait):
;   U, Y, W, X, A, P, PCL, PCH          (top first; TK_SP is just below it)
; A switch saves the stack pointer, picks the next task, writes T, loads that task's stack pointer and unwinds
; its frame (K_SCHED_RESUME).  Round robin over tasks 1-15; the kernel task (0) runs only when nothing else can,
; and sleeps the CPU (its idle loop: reset.s), or when irq entries have queued notes to groups for it to post (then
; it takes its turn in the round).  Only ST_READY runs (TK_STATE: the state of the context on top of
; the task's stack, so a driver serving a call is ST_READY while the call runs, and can be switched out).
;
; Waiting is always the same: set ST_WAIT, YIELD, and when woken (WAKE: ST_READY), look again.  A wake for
; nothing costs a look.  Three waits the scheduler ends itself, as it looks for the next task: ST_SLEEP, once the
; time in the kernel task's table has come (so the tick's interrupt does no more than count); ST_BLOCKED, once
; the task it's waiting to call is free (so a call's end wakes nobody); and ST_EVENT, once the event count of the
; server it's waiting on has changed (so an irq entry wakes its clients by adding 1 to a byte: file.s).  A task
; that's stopped (TF_STOPPED: /proc's ctl, TASKSTOP) is passed by whatever its state, till it's started again.
;
; The foreground (TF_FORE: TASKFORE, a program's for itself or the tasks it started): a task in it is picked
; before the round robin's whenever it can run, at the next switch (a tick at most), and a task serving its call
; with it.  Turns by the ticks: the foreground's FORE_RUN ticks at most, then the background's, the rest of a
; tick (its tasks by the round robin), so the others always have theirs.
; For a program whose answer runs through several tasks (a game's: the UI, its parser, its world) while others
; work in the background.  With none in it, the scheduler is the round robin alone, as it was.

.include "kdefs.inc"

.segment "KCODE"

.assert     TF_STOPPED = $80, error, "The scheduler tests TF_STOPPED with bit"

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

; The next task to run.  The foreground first (TF_FORE: TASKFORE's), while there may be some (K0_FORES), in its turn
; (K0_FMODE 0: FORE_RUN ticks at most, from the tick it began, K0_FTICK; a pick with none of it able to run begins
; it again): the next foreground task after this one that can run (this one last), a task serving a call (SCALL) for
; a foreground caller counting as one.  The background's turn (K0_FMODE 1) is the rest of the tick it began in.
; Else the round robin: the next task that can run after its place (1-15, its place last), the foreground's passed
; by unless nobody else can run, that place this task unless the foreground's last pick was this one; else the
; kernel task (to idle).  A task can run when it's ST_READY, or it's waiting for what's come (a sleeper's time, a
; call's task free, an event: pick_ok's, which makes it ST_READY).  With no foreground task it's the round robin
; after this one alone, as it always was.  The round robin's place goes on round: so every task that can run has its
; turn, a foreground task busy or not.  IN, OUT: IRQs off (a moment on between its looks at each task).  OUT: .A =
; the task.  Modifies .X, .Y
K_SCHED_PICK:
            ldy         T_REGISTER                          ; .Y = this task, throughout
            stz         T_REGISTER                          ; ---- The kernel task's
            cpy         K0_FPICK                            ; (The round robin's place: this task, unless the
            beq         :+                                  ;   foreground picked it)
            sty         K0_RR
:
            lda         K0_FORES
            beq         @tor
            lda         K0_TICKS                            ; (Its turns by the ticks: .A = the tick now)
            ldx         K0_FMODE
            beq         @fturn
            cmp         K0_FTICK                            ; The background's turn: the rest of its tick
            beq         @tor
            stz         K0_FMODE                            ; (Over: the foreground's turn, from this tick)
            sta         K0_FTICK
            bra         @fpass

@fturn:                                                     ; The foreground's turn: FORE_RUN ticks at most
            sec
            sbc         K0_FTICK
            cmp         #FORE_RUN
            bcc         @fpass
            inc         K0_FMODE                            ; (Over: the background's turn, this tick)
            lda         K0_TICKS
            sta         K0_FTICK
@tor:
            jmp         @robin

@fpass:
            stz         K0_FSEEN
            sty         T_REGISTER                          ; ---- This task
            tya
            tax                                             ; .X = the candidate
            lda         #TASKS
            sta         TK_PICKS
@fnext:
            cli                                             ; (A moment for interrupts, before each look)
            nop
            sei
            inx
            txa
            and         #TASKS - 1
            tax
            beq         @fskip                              ; (The kernel task: never the foreground's)
            stx         T_REGISTER                          ; ---- The candidate
            lda         TK_FLAGS
            and         #TF_FORE
            bne         @fore
            lda         TK_BUSY                             ; Serving a call (not BUSY_NOSERVE) ...
            beq         @fno
            bmi         @fno
            lda         TK_INCALLER
            sta         T_REGISTER                          ; ---- Its caller
            lda         TK_STATE
            cmp         #ST_CALL                            ;   ... for a caller in the foreground?
            bne         @fno
            lda         TK_FLAGS
            and         #TF_FORE
            beq         @fno
            sty         T_REGISTER                          ; ---- This task
            lda         #TF_STOPPED
            jsr         pick_ok
            bcs         @fpick
            bra         @fskip

@fore:
            sty         T_REGISTER                          ; ---- This task
            lda         #TF_STOPPED
            jsr         pick_ok
            bcs         @fpick
            stz         T_REGISTER                          ; ---- The kernel task's: the foreground's not empty
            lda         #TF_FORE
            sta         K0_FSEEN
            bra         @fself
@fno:                                                       ; Not in it: the server a foreground caller's call
            stz         T_REGISTER                          ; ---- The kernel task's (left last, K0_FSRV, FORE_SRV
            cpx         K0_FSRV                             ;   ticks at most since: the work its call left it, the
            bne         @fself                              ;   console's words, done with its caller's)
            lda         K0_TICKS
            sec
            sbc         K0_FSRVT
            cmp         #FORE_SRV
            bcs         @fself
            sty         T_REGISTER                          ; ---- This task
            lda         #TF_STOPPED
            jsr         pick_ok
            bcs         @fpick
            bra         @fskip
@fself:
            sty         T_REGISTER                          ; ---- This task
@fskip:
            dec         TK_PICKS
            bne         @fnext
            stz         T_REGISTER                          ; ---- The kernel task's
            lda         K0_FSEEN                            ; None in the foreground at all: none till TASKFORE
            bne         :+
            stz         K0_FORES
:
            lda         K0_TICKS                            ; (None of it could run: its turn from this tick)
            sta         K0_FTICK
            bra         @robin

@fpick:
            cli                                             ; (A moment for interrupts)
            nop
            sei
            stz         T_REGISTER                          ; ---- The kernel task's: the foreground's pick
            stx         K0_FPICK
            sty         T_REGISTER                          ; ---- This task
            txa
            rts

@robin:                                                     ; (T: the kernel task's)
            lda         #$FF                                ; The round robin's pick
            sta         K0_FPICK
            ldx         K0_RR                               ; .X = the candidate: after the place
            lda         K0_FORES                            ; The round's first time round: the foreground passed
            sty         T_REGISTER                          ;   by, if there may be any; then with it
            beq         @all
            lda         #TASKS
            sta         TK_PICKS
@next:
            cli                                             ; (A moment for interrupts, before each look)
            nop
            sei
            inx
            txa
            and         #TASKS - 1
            tax
            beq         @kernel
            lda         #TF_STOPPED | TF_FORE
            jsr         pick_ok
            bcs         @found
@skip:
            dec         TK_PICKS
            bne         @next
            stz         T_REGISTER                          ; ---- The kernel task's
            ldx         K0_RR
            sty         T_REGISTER                          ; ---- This task
@all:
            lda         #TASKS
            sta         TK_PICKS
@next2:
            cli                                             ; (A moment for interrupts, before each look)
            nop
            sei
            inx
            txa
            and         #TASKS - 1
            tax
            beq         @kernel2
            lda         #TF_STOPPED
            jsr         pick_ok
            bcs         @found
@skip2:
            dec         TK_PICKS
            bne         @next2
            lda         #KERNEL_TASK                        ; Nobody: the idle task
            rts

@kernel:                                                    ; The kernel task is in the round only while it has
            jsr         kernel_ok                           ;   notes queued to post (notes.s: K_NOTE_QUEUED, from
            beq         @skip                               ;   its idle loop)
@found:
            txa
            rts

@kernel2:
            jsr         kernel_ok
            beq         @skip2
            txa                                             ; (KERNEL_TASK)
            rts

; Has the kernel task notes queued to post?  IN: IRQs off, .Y and T = this task.  OUT: Z = 0, it has
kernel_ok:
            stz         T_REGISTER
            lda         K0_NQ_ANY
            sty         T_REGISTER
            rts

; Can task .X run?  Not if it has any of the flags .A (TF_STOPPED: it's stopped; TF_FORE: the round robin's first
; time round).  If it's ST_READY; or a sleeper whose time has come; or blocked on a call to a task that's free now;
; or waiting for an event count that's changed: then it's made ST_READY.  IN: IRQs off, .Y and T = this task.
; OUT: C = 1, it can.  Keeps .X, .Y
pick_ok:
            stx         T_REGISTER                          ; A quick look at it
            and         TK_FLAGS
            bne         @not
            lda         TK_STATE
            sty         T_REGISTER
            cmp         #ST_READY
            beq         @yes
            cmp         #ST_SLEEP
            beq         @sleeper
            cmp         #ST_BLOCKED
            beq         @blocked
            cmp         #ST_EVENT
            beq         @event
@no:
            clc
            rts

@not:
            sty         T_REGISTER
            clc
            rts

@sleeper:                                                   ; Has its time come?  (Now - its time >= 0)
            stz         T_REGISTER                          ; (The kernel task's: the ticks, the wake times)
            lda         K0_TICKS
            cmp         K_WAKE_LO,X
            lda         K0_TICKS + 1
            sbc         K_WAKE_HI,X
            sty         T_REGISTER
            bmi         @no
@ready:
            stx         T_REGISTER                          ; It can run: ready
            lda         #ST_READY
            sta         TK_STATE
            sty         T_REGISTER
@yes:
            sec
            rts

@blocked:                                                   ; Is the task it's waiting to call free?  (Or gone:
            stx         T_REGISTER                          ;   its call finds out)
            lda         TK_BLOCKEDON
            sta         T_REGISTER
            lda         TK_BUSY
            sty         T_REGISTER                          ; (Z: from TK_BUSY)
            beq         @ready
            bra         @no

@event:                                                     ; Has its server's event count changed?  (Or the server
            stx         T_REGISTER                          ;   gone: it asks again, and finds out)
            lda         TK_EVTASK
            sta         T_REGISTER                          ; ---- The server
            lda         TK_STATE
            beq         @ready                              ; (ST_FREE)
            lda         TK_EVENT
            stx         T_REGISTER                          ; ---- The waiter
            cmp         TK_EVV
            sty         T_REGISTER                          ; ---- Back
            bne         @ready
            bra         @no

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
; PAUSE: wait until woken (WAKE); at once if a wake came since the last (TK_WOKEN: so a wake between asking for
; something and waiting for it isn't lost).  Keeps everything
K_PAUSE:
            php
            sei
            pha
            lda         TK_WOKEN
            bne         @woken
            lda         #ST_WAIT
            sta         TK_STATE
            pla
            jsr         K_YIELD
            pha
@woken:
            stz         TK_WOKEN
            pla
            plp
            rts

; TPAUSE: PAUSE till a time: wait until woken (WAKE), or till the wake time in the kernel task's table (K_WAKE_LO/HI,
; as SLEEP_UNTIL's: the caller's, set already); at once if a wake came since the last.  A sleep the scheduler ends at
; the time, and WAKE too (TF_TIMED: set and cleared by TSB and TRB, a byte others change too).  OUT: C = 1 if it's
; still before the time (woken).  Modifies .A, .Y
K_TPAUSE:
            php
            sei
            lda         TK_WOKEN
            bne         @woken
            lda         #TF_TIMED
            tsb         TK_FLAGS
            lda         #ST_SLEEP
            sta         TK_STATE
            jsr         K_YIELD
@woken:
            stz         TK_WOKEN
            plp
            lda         #TF_TIMED
            trb         TK_FLAGS
            php
            sei
            ldy         T_REGISTER
            stz         T_REGISTER                          ; ---- The kernel task's: the wake time, and now
            lda         K0_TICKS
            cmp         K_WAKE_LO,Y
            lda         K0_TICKS + 1
            sbc         K_WAKE_HI,Y
            sty         T_REGISTER                          ; ---- Back
            plp
            cmp         #$80                                ; (Now - the time below 0: C = 1)
            rts

; WAKE: task .A, if it's waiting, can run; if it isn't, its next PAUSE won't wait (TK_WOKEN).  A task asleep in a
; timed wait (TF_TIMED: TPAUSE's) is waiting too.  From any task, or an irq entry.  Keeps everything
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
            beq         @ready
            cmp         #ST_EVENT
            beq         @ready
            cmp         #ST_SLEEP
            bne         @not
            lda         TK_FLAGS                            ; (A timed wait's sleep)
            and         #TF_TIMED
            beq         @not
@ready:
            lda         #ST_READY
            sta         TK_STATE
            bra         @back
@not:
            lda         #1
            sta         TK_WOKEN
@back:
            sty         T_REGISTER
            pla
            ply
            plx
            plp
            clc
            rts

; Wait till task .A's event count (TK_EVENT) isn't .X: a server's, as it took a request that answered E_AGAIN
; (file.s); or a WAKE, or a note.  At once if a wake or a note came since.  Modifies .A, .X, .Y
K_EVWAIT:
            php
            sei
            ldy         TK_WOKEN
            bne         @woken
            ldy         TK_NOTED
            bne         @woken
            sta         TK_EVTASK
            stx         TK_EVV
            lda         #ST_EVENT
            sta         TK_STATE
            jsr         K_YIELD                             ; (The scheduler looks at the count: K_SCHED_PICK)
@woken:
            stz         TK_WOKEN
            plp
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
