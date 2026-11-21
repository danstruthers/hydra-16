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
;
; The candidates (K0_CANDLO/HI, 2026-11-21): the pick looks only at the tasks that may be in a state it runs or ends
; (ST_READY, ST_SLEEP, ST_BLOCKED, ST_EVENT), in the order it always looked at every task, so it picks as it did
; (a task out of the set is one its look would have passed by, changing nothing), for some 650 cycles a tick where
; it took some 1,240 with one program busy, the console waiting for a key and four drivers idle.  The set may hold
; more: a task goes in as it's switched out (it ran, whatever state it left in), as another task makes it ready
; (WAKE, a note, its start: CAND_SET), and leaves it only as a look finds it in none of those states and serving
; no call (a call's end puts back the state the task had before it, a wait the pick ends maybe).
; docs/03-base/kernel.md, "The scheduler".

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
; call's task free, an event: look's, which makes it ST_READY).  With no foreground task it's the round robin
; after this one alone, as it always was.  The round robin's place goes on round: so every task that can run has its
; turn, a foreground task busy or not.
;
; The looks go only to the candidates (K0_CANDLO/HI: every task in a state the pick runs or ends, ST_READY, ST_SLEEP,
; ST_BLOCKED or ST_EVENT, and maybe others), in the order they always went to every task: a task out of the set is
; one whose look would have passed it by and changed nothing.  A task goes in as it's switched out (here: it ran,
; whatever state it left in) and as another makes it ready (WAKE, a note, its start: CAND_SET); out as a look finds
; it in no candidate's state.  Whether any task is in the foreground at all (none: K0_FORES cleared) is the same
; (K_FMASK: TASKFORE puts a task in; fseen takes out one that's left it).
; T is this task's at every JSR, RTS and moment for interrupts (T chooses the stack page too); the kernel task's
; only between.  IN, OUT: IRQs off (a moment on between its looks).  OUT: .A = the task.  Modifies .X, .Y
K_SCHED_PICK:
            ldy         T_REGISTER                          ; .Y = this task, throughout
            stz         T_REGISTER                          ; ---- The kernel task's
            lda         cand_lo,Y                           ; This task a candidate, maybe (it ran)
            tsb         K0_CANDLO
            lda         cand_hi,Y
            tsb         K0_CANDHI
            cpy         K0_FPICK                            ; (The round robin's place: this task, unless the
            beq         :+                                  ;   foreground picked it)
            sty         K0_RR
:
            lda         K0_FORES
            beq         @robin
            lda         K0_TICKS                            ; (Its turns by the ticks: .A = the tick now)
            ldx         K0_FMODE
            beq         @fturn
            cmp         K0_FTICK                            ; The background's turn: the rest of its tick
            beq         @robin
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
@robin:                                                     ; The round robin's pick (T: the kernel task's)
            lda         #$FF
            sta         K0_FPICK
            ldx         K0_RR
            lda         K0_FORES                            ; The round's first time round: the foreground passed
            sty         T_REGISTER                          ; ---- This task
            beq         @all                                ;   by, if there may be any; then with it
            lda         #TF_STOPPED | TF_FORE
            jsr         round
            bcs         @found
            stz         T_REGISTER                          ; ---- The kernel task's
            ldx         K0_RR
            sty         T_REGISTER                          ; ---- This task
@all:
            lda         #TF_STOPPED
            jsr         round
            bcs         @found
            ldx         #KERNEL_TASK                        ; Nobody: the idle task
@found:
            txa
            rts

@fpass:                                                     ; The foreground's looks, after this task
            sty         T_REGISTER                          ; ---- This task
            tya
            tax
            lda         #0
            jsr         round
            bcc         @fnone
            cli                                             ; (A moment for interrupts)
            nop
            sei
            stz         T_REGISTER                          ; ---- The kernel task's: the foreground's pick
            stx         K0_FPICK
            sty         T_REGISTER                          ; ---- This task
            bra         @found

@fnone:
            jsr         fseen                               ; None in the foreground at all: none till TASKFORE
            stz         T_REGISTER                          ; ---- The kernel task's
            bne         :+
            stz         K0_FORES
:
            lda         K0_TICKS                            ; (None of it could run: its turn from this tick)
            sta         K0_FTICK
            bra         @robin

; A round of looks: the candidates after task .X, from .X + 1 to 15, then (the round robin's) the kernel task's place,
; then from 1 to .X; each by the round robin's look (.A: the flags it passes by) or the foreground's (.A = 0), till one
; can run.  The kernel task is in the round robin's only while it has notes queued to post (notes.s: K_NOTE_QUEUED,
; from its idle loop).  IN, OUT: T = this task (.Y), IRQs off (a moment on before each look).  OUT: C = 1, .X = the
; task; C = 0, none.  Keeps .Y
round:
            sta         TK_PICKS                            ; (The look: this task's, in its zero page)
            stz         T_REGISTER                          ; ---- The kernel task's
            stx         K_START                             ; (Bit 7: from 1 on, the second part)
            lda         #$FF                                ; (From .X + 1 to 15)
            sta         K_LIM
            sta         K_LIM + 1
            sty         T_REGISTER                          ; ---- This task
@next:
            cli                                             ; (A moment for interrupts)
            nop
            sei
            jsr         next
            bcs         @wrap
            cli                                             ; (A moment for interrupts, before each look)
            nop
            sei
            jsr         check
            bcc         @next
            rts

@wrap:                                                      ; Past 15 (or, the second time, past the start)
            cli                                             ; (A moment for interrupts)
            nop
            sei
            stz         T_REGISTER                          ; ---- The kernel task's
            ldx         K_START
            bmi         @no
            sty         T_REGISTER                          ; ---- This task
            lda         TK_PICKS                            ; The kernel task's place (the round robin's)
            stz         T_REGISTER                          ; ---- The kernel task's
            beq         :+
            lda         K0_NQ_ANY
            beq         :+
            ldx         #KERNEL_TASK
            sec
            bra         @back

:
            lda         above_lo,X                          ; From 1 to the start: not past it
            eor         #$FF
            sta         K_LIM
            lda         above_hi,X
            eor         #$FF
            sta         K_LIM + 1
            lda         #$80
            tsb         K_START
            ldx         #0
            sty         T_REGISTER                          ; ---- This task
            bra         @next

@no:
            clc
@back:
            sty         T_REGISTER                          ; ---- This task
            rts

; The next candidate after task .X, within K_LIM, as they are now (a WAKE in a moment for interrupts counts).  IN,
; OUT: T = this task (.Y).  OUT: C = 1, none; else C = 0, .X = it.  Modifies .A
next:
            stz         T_REGISTER                          ; ---- The kernel task's
            lda         K0_CANDLO
            and         K_LIM
            and         above_lo,X
            sta         K_M
            lda         K0_CANDHI
            and         K_LIM + 1
            and         above_hi,X
            sta         K_M + 1
            bra         first0

; The first task in the set K_M (bit = task).  IN, OUT: T = this task (.Y).  OUT: C = 1, none; else C = 0, .X =
; it.  Modifies .A
first:
            stz         T_REGISTER                          ; ---- The kernel task's
first0:
            ldx         #0                                  ; (.X: the byte's first task)
            lda         K_M
            bne         :+
            ldx         #8
            lda         K_M + 1
            beq         @none
:
            bit         #$0F                                ; (Its low nibble, else its high)
            bne         :+
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            inx
            inx
            inx
            inx
:
            stx         K_NIB                               ; (A moment)
            and         #$0F
            tax
            lda         lsb4,X
            ora         K_NIB                               ; (0, 4, 8 or 12: no carries)
            tax
            sty         T_REGISTER                          ; ---- This task
            clc
            rts

@none:
            sty         T_REGISTER                          ; ---- This task
            sec
            rts

; Task .X looked at by the round's look: the round robin's (TK_PICKS: the flags it passes by), or the foreground's
; (TK_PICKS = 0).  IN, OUT: T = this task (.Y).  OUT: C = 1, it's picked.  Keeps .X, .Y
check:
            lda         TK_PICKS
            bne         look

; The foreground's look: task .X, if it's in the foreground, or serving a call (SCALL, not BUSY_NOSERVE) for a
; caller in it, or the server a foreground task called last (K0_FSRV, till a foreground task calls another: the
; work its call left it, the console's words painted, done with its caller's); and it can run (look's, but if it's
; stopped).  IN, OUT: T = this task (.Y).  OUT: C = 1, it's picked.  Keeps .X, .Y
fore_ok:
            stx         T_REGISTER                          ; ---- The candidate
            lda         TK_FLAGS
            and         #TF_FORE
            bne         @look
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
@look:
            lda         #TF_STOPPED
            bra         look

@fno:                                                       ; Not in it: the server a foreground task called last?
            stz         T_REGISTER                          ; ---- The kernel task's
            cpx         K0_FSRV
            sty         T_REGISTER                          ; ---- This task
            beq         @look
            clc
            rts

; Can task .X run?  Not if it has any of the flags .A (TF_STOPPED: it's stopped; TF_FORE: the round robin's first
; time round).  If it's ST_READY; or a sleeper whose time has come; or blocked on a call to a task that's free now;
; or waiting for an event count that's changed: then it's made ST_READY.  A task in none of those states (ST_FREE,
; ST_WAIT, ST_CALL, ST_IDLE, ST_NEW) leaves the candidates, unless it's serving a call.  IN: IRQs off.  OUT: C = 1,
; it can; T = this task (.Y).  Keeps .X, .Y
look:
            stx         T_REGISTER                          ; ---- The candidate: a quick look
            and         TK_FLAGS
            bne         @no
            lda         TK_STATE
            stz         T_REGISTER                          ; ---- The kernel task's
            cmp         #ST_READY
            beq         @yes
            cmp         #ST_SLEEP
            bcc         @drop                               ; (ST_FREE, ST_WAIT, ST_CALL, ST_IDLE, ST_NEW)
            beq         @sleeper
            cmp         #ST_BLOCKED
            beq         @blocked
            cmp         #ST_EVENT
            beq         @event
@drop:
            stx         T_REGISTER                          ; (Serving a call, waiting in it: a candidate still,
            lda         TK_BUSY                             ;   as the call's end puts back the state it had
            stz         T_REGISTER                          ;   before, a wait the pick ends maybe)
            beq         :+
            bpl         @no
:
            lda         cand_lo,X                           ; No candidate
            trb         K0_CANDLO
            lda         cand_hi,X
            trb         K0_CANDHI
@no:
            sty         T_REGISTER                          ; ---- This task
            clc
            rts

@sleeper:                                                   ; Has its time come?  (Now - its time >= 0)
            lda         K0_TICKS
            cmp         K_WAKE_LO,X
            lda         K0_TICKS + 1
            sbc         K_WAKE_HI,X
            bmi         @no
@ready:
            stx         T_REGISTER                          ; It can run: ready
            lda         #ST_READY
            sta         TK_STATE
@yes:
            sty         T_REGISTER                          ; ---- This task
            sec
            rts

@blocked:                                                   ; Is the task it's waiting to call free?  (Or gone:
            stx         T_REGISTER                          ;   its call finds out)
            lda         TK_BLOCKEDON
            sta         T_REGISTER
            lda         TK_BUSY
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
            bne         @ready
            bra         @no

; Is any task in the foreground (TF_FORE)?  The tasks in K_FMASK looked at, one that's left it taken out.  IN, OUT:
; T = this task (.Y), IRQs off.  OUT: Z = 0, there is.  Modifies .A, .X
fseen:
            cli                                             ; (A moment for interrupts)
            nop
            sei
            stz         T_REGISTER                          ; ---- The kernel task's
            lda         K_FMASK
            sta         K_M
            lda         K_FMASK + 1
            sta         K_M + 1
            sty         T_REGISTER                          ; ---- This task
            jsr         first
            bcs         @none
            stx         T_REGISTER                          ; ---- The task: a quick look
            lda         TK_FLAGS
            sty         T_REGISTER                          ; ---- This task
            and         #TF_FORE
            bne         @yes
            stz         T_REGISTER                          ; ---- The kernel task's: it's left it
            lda         cand_lo,X
            trb         K_FMASK
            lda         cand_hi,X
            trb         K_FMASK + 1
            sty         T_REGISTER                          ; ---- This task
            bra         fseen

@none:
            lda         #0
@yes:
            rts

.segment "KRODATA"

; Each task's bit in a set of tasks (low byte, high byte: the kernel task's none: CAND_SET's too); the tasks after
; each (above_lo, above_hi); the lowest bit set in each nibble (lsb4)
cand_lo:    .byte       $00, $02, $04, $08, $10, $20, $40, $80, $00, $00, $00, $00, $00, $00, $00, $00
cand_hi:    .byte       $00, $00, $00, $00, $00, $00, $00, $00, $01, $02, $04, $08, $10, $20, $40, $80
above_lo:   .byte       $FE, $FC, $F8, $F0, $E0, $C0, $80, $00, $00, $00, $00, $00, $00, $00, $00, $00
above_hi:   .byte       $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FE, $FC, $F8, $F0, $E0, $C0, $80, $00
lsb4:       .byte       $00, $00, $01, $00, $02, $00, $01, $00, $03, $00, $01, $00, $02, $00, $01, $00

.segment "KCODE"

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
            CAND_SET                                        ; (A candidate)
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

; ****************************************************************************
.segment "KCODE_P1"

; TPAUSE: PAUSE till a time: wait until woken (WAKE), or till the wake time in the kernel task's table (K_WAKE_LO/HI,
; as SLEEP_UNTIL's: the caller's, set already); at once if a wake came since the last.  A sleep the scheduler ends at
; the time, and WAKE too (TF_TIMED: set and cleared by TSB and TRB, a byte others change too).  OUT: C = 1 if it's
; still before the time (woken).  Modifies .A, .Y.  On page 1, with its caller (sem.s: SEM_WAIT's)
K_TPAUSE:
            php
            sei
            lda         TK_WOKEN
            bne         @woken
            lda         #TF_TIMED
            tsb         TK_FLAGS
            lda         #ST_SLEEP
            sta         TK_STATE
            FARCALL     K_YIELD
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
