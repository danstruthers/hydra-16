.debuginfo
.segment "TASKS"

TASK_0_VECTOR           = $E000

TASK_BUSY_FLAG          = 1
TASK_PAUSED_FLAG        = 2
TASK_RESIDENT_FLAG      = 8

; TASK STATUS REGISTER BITS
;   0: 0 = Available, 1 = In Use
;   1: 0 = Active, 1 = Paused
;   2: 0 = Not Waiting, 1 = Awaiting I/O
;   3: 0 = , 1 = Resident (driver task: runs only from IRQs / TASK_CALLs, never scheduled)
;   4: 0 = , 1 = A break is due (TASK_BREAK_FLAG)
;   5: 0 = , 1 = A kill is due (TASK_KILL_FLAG)
;   6: 0 = , 1 = In a TASK_CALL: its call runs in another task, and it goes on when that returns
;   7: 0 = , 1 = Switched out in the middle of another task's call (TASK_GUEST_OUT_FLAG): runnable,
;                even resident or paused, to finish it (unless it's waiting)

; Initialize the tasks, their stacks, etc.  Keeps the caller's I flag.
TASKS_INIT:
            php
            sei                                     ; Turn off interrupts
            lda     T_REGISTER
            bne     @cleanup                        ; Only support tasks init when on task 0
            ldx     #MAX_TASK_NUMBER

@loop:
            stx     T_REGISTER                      ; Quick switch to task X
            stz     RAM_BANK_REG
            stz     ROM_BANK_REG
            stz     TASK_STATUS_REG
            stz     ZP_D_PAGE                       ; Disassembler reads the BIOS page by default
            stz     ZP_NO_PREEMPT                   ; Scheduler
            stz     ZP_PREEMPT_DUE
            stz     ZP_IN_SCHED
            stz     ZP_TC_GUEST
            stz     ZP_TC_WAITERS
            stz     ZP_TC_WAITERS + 1
            stz     ZP_SLEEPERS                     ; (The system task's: nobody sleeping)
            stz     ZP_SLEEPERS + 1
            stz     ZP_IRQ_RESCHED
            stz     ZP_BREAK_VEC + 1                ; No break handler
            stz     ZP_OUT_CNT                      ; No stdio buffering (WRITE_CHAR's fast path
            stz     ZP_IN_CNT                       ;   looks at these even without fds)
            stz     ZP_IN_POS
            stz     ZP_OUT_LINE
            lda     #$FF
            sta     TASK_PARENT                     ; No parent
            sta     ZP_TASK_OWNER
            sta     STACK_SAVE_REG
            ldy     #IO_MAX_FDS * IO_FD_SIZE        ; All fds closed

@fds:
            dey
            sta     IO_FD_TABLE,Y
            bne     @fds
            dex
            bpl     @loop                           ; Loop back as long as X >= 0
            lda     #TASK_BUSY_FLAG
            sta     TASK_STATUS_REG                 ; Mark task zero as "busy"

            ; setup interrupt handler and interrupt timer

            ; Will fall through when X = $FF, leaving us in Task 0, as required

@cleanup:
            plp                                     ; The caller's I flag
            rts

; ****************************************************************************
; Scheduler (see docs/plans/IO_PLAN.md, Phase 1)
;
;   Every task that isn't running keeps the same frame on its own stack: the one IRQ_DISPATCH builds
;   (interrupt frame, A, X, W, Y, the TASK_CALL scratch bytes), plus U.  Top of stack first:
;       U, ZP_TC_TASK, ZP_TC_VEC + 1, ZP_TC_VEC, Y, W, X, A, P, PCL, PCH
;   and STACK_SAVE_REG is the SP below it.  So a task switch is always the same: save SP, pick a task,
;   load its SP, unwind its frame (SCHED_RESUME), whether the task stopped for the timer tick (IRQ),
;   YIELD, or to wait for IO.  W and U are in the frame because they're global pseudo-registers.
;
;   Runnable: busy, and not paused, waiting, resident or in a TASK_CALL; or switched out in the middle of
;   another task's call (a server), and not waiting.  Round-robin over tasks 1-15; task 0 is the idle
;   task and only runs when nothing else can.
;
;   Servers can be preempted: a TASK_CALL routine (e.g. a file server's serve routine, for a client's
;   request) runs in the server's task like any other code, so a long request doesn't hold up the other
;   tasks.  The client is marked in a call (TASK_CALLING_FLAG) and isn't run until the call returns; the
;   server, switched out mid-call, gets TASK_GUEST_OUT_FLAG, which makes it runnable until it's resumed.
;   A server serves one call at a time: a task that calls it while it's busy with another's waits
;   (TC_WAIT_FREE), except the IRQ dispatcher's calls (TASK_CALL_IRQ), which run at once, on its stack
;   below the switched-out call, as they could always interrupt a call.

TASK_WAITING_FLAG       = 4                         ; Bit 2: awaiting IO (TASK_WAIT / IO_WAKE)
TASK_BREAK_FLAG         = $10                       ; Bit 4: a break is due (console: SER_BREAK)
TASK_KILL_FLAG          = $20                       ; Bit 5: a kill is due (console: SER_BREAK)
TASK_CALLING_FLAG       = $40                       ; Bit 6: in a TASK_CALL (its call runs in another task)
TASK_GUEST_OUT_FLAG     = $80                       ; Bit 7: switched out in the middle of another task's call
TASK_RUN_MASK           = TASK_BUSY_FLAG | TASK_PAUSED_FLAG | TASK_WAITING_FLAG | TASK_RESIDENT_FLAG | TASK_CALLING_FLAG
TASK_FRAME_SP           = $F4                       ; STACK_SAVE_REG of a new task (11-byte frame at $01F5)
SCHED_RESCHED_A         = $A5                       ; An IRQ handler returns C = 1, .A and .Y = these
SCHED_RESCHED_Y         = $5A                       ;   to ask for a task switch (the timer tick)

; Switch tasks.  IRQs off, and the current task's full frame (including U) on its stack.
SCHED_SWITCH:
            lda     ZP_TC_GUEST
            beq     :+
            smb7    TASK_STATUS_REG                 ; In another task's call: run it again to finish it
:                                                   ;   (TASK_GUEST_OUT_FLAG)
            tsx
            stx     STACK_SAVE_REG                  ; The current task's SP
            inc     ZP_IN_SCHED                     ; (IRQs come in during SCHED_PICK: no switch in them)
            jsr     SCHED_PICK                      ; .A = next task (maybe the same one); IRQs off again
            stz     ZP_IN_SCHED
            sta     T_REGISTER                      ; Its ZP and stack
            ldx     STACK_SAVE_REG
            txs

; Unwind a task's frame and continue it; or, if a break or kill is due, continue it at BREAK_ENTRY
; instead (on ROM page 0, IRQs on, U = 0), unless it's in the middle of another task's call (then the
; break waits until it's back in its own code)
SCHED_RESUME:
            rmb7    TASK_STATUS_REG                 ; (TASK_GUEST_OUT_FLAG)
            lda     ZP_TC_GUEST
            bne     @resume
            lda     TASK_STATUS_REG
            and     #TASK_BREAK_FLAG | TASK_KILL_FLAG
            beq     @resume
            tsx                                     ; The frame: $0101,X = U ... $010B,X = PCH
            stz     $0101,X                         ; U
            stz     $0106,X                         ; W
            stz     $0109,X                         ; P
            lda     #<BREAK_ENTRY
            sta     $010A,X
            lda     #>BREAK_ENTRY
            sta     $010B,X

@resume:
            pla
            sta     U_REGISTER
            jmp     IRQ_RESTORE                     ; The rest is the IRQ frame (irq.s)

; Pick the next task to run: the next runnable task after the current one (1-15, round-robin); else
; the current one if it's runnable; else task 0 (idle).  No stack use while looking at other tasks.  IRQs
; are on between the looks, so a serial byte isn't kept waiting by a scan of 15 tasks (SCHED_SWITCH sets
; ZP_IN_SCHED, so they don't start a switch of their own).  IN: IRQs off.  OUT: .A = task, IRQs off
; Modifies: .A, .X, .Y
SCHED_PICK:
            lda     T_REGISTER
            and     #$0F
            tay                                     ; .Y = current task
            tax                                     ; .X = candidate
            lda     #MAX_TASK_NUMBER
            sta     ZP_SCHED_CNT

@next:
            inx
            txa
            and     #$0F
            tax
            beq     @skip                           ; Task 0 isn't in the rotation
            sei
            stx     T_REGISTER                      ; Quick look at the candidate (no stack use!)
            lda     TASK_STATUS_REG
            sty     T_REGISTER
            cli                                     ; (IRQs between the looks: ZP_IN_SCHED stops a switch)
            bmi     @guest                          ; (SCHED_RUNNABLE, inline: this runs every tick)
            and     #TASK_RUN_MASK
            cmp     #TASK_BUSY_FLAG
            beq     @found
            bra     @skip

@guest:
            and     #TASK_WAITING_FLAG | TASK_CALLING_FLAG
            beq     @found

@skip:
            dec     ZP_SCHED_CNT
            bne     @next
            sei
            lda     TASK_STATUS_REG                 ; Nobody else: keep going if we can
            jsr     SCHED_RUNNABLE
            beq     @stay
            lda     #SYSTEM_TASK_NUM                ; Idle
            rts

@stay:
            tya
            rts

@found:
            sei
            txa
            rts

; Can a task with status .A run?  OUT: Z = 1 yes.  Modifies: .A
SCHED_RUNNABLE:
            bmi     @guest                          ; (TASK_GUEST_OUT_FLAG)
            and     #TASK_RUN_MASK
            cmp     #TASK_BUSY_FLAG
            rts

@guest:
            and     #TASK_WAITING_FLAG | TASK_CALLING_FLAG
            rts

.assert     TASK_GUEST_OUT_FLAG = $80, error, "SCHED_RUNNABLE tests TASK_GUEST_OUT_FLAG with bmi"

; Can the interrupted (current) task be preempted?  Not if it isn't runnable (e.g. a resident driver
; task), unless it's running a TASK_CALL routine for another task (a server serving a request: that can
; be switched out too), or if it holds NO_PREEMPT (then the switch is noted, for PREEMPT).
; OUT: C = 1 switch, C = 0 don't
SCHED_CAN_PREEMPT:
            lda     ZP_IN_SCHED                     ; An IRQ during SCHED_PICK: already switching
            bne     @no
            lda     ZP_TC_GUEST
            bne     @guest
            lda     TASK_STATUS_REG
            and     #TASK_RUN_MASK
            cmp     #TASK_BUSY_FLAG
            bne     @no

@guest:
            lda     ZP_NO_PREEMPT
            beq     @yes
            lda     #1
            sta     ZP_PREEMPT_DUE                  ; Switch at PREEMPT

@no:
            clc
            rts

@yes:
            sec
            rts

; Give up the CPU: switch to the next runnable task.  Returns when this task is picked again (at once
; if nothing else can run).
; Preserves .A, .X, .Y and the flags
YIELD:
            php
            sei
            pha                                     ; Build the frame an IRQ would: P is there already,
            phx                                     ;   under it the return address (made RTI-style below)
            tsx
            inc     $0104,X                         ; Return address (S+4 = PCL, S+5 = PCH) + 1
            bne     :+
            inc     $0105,X
:
            lda     W_REGISTER
            pha
            phy
            lda     ZP_TC_VEC
            pha
            lda     ZP_TC_VEC + 1
            pha
            lda     ZP_TC_TASK
            pha
            lda     U_REGISTER
            pha
            stz     ZP_PREEMPT_DUE
            jmp     SCHED_SWITCH

; Hold the CPU: no task switches until the matching PREEMPT (nestable).  IRQs and their handlers keep
; running.  For short sections, sei / cli also works (but holds off all IRQs).
; Preserves .A, .X, .Y
NO_PREEMPT:
            inc     ZP_NO_PREEMPT
            rts

; Undo NO_PREEMPT; if a task switch came due meanwhile (and this was the outermost), switch now.
; Preserves .A, .X, .Y
PREEMPT:
            php
            sei
            pha
            lda     ZP_NO_PREEMPT
            beq     @done                           ; Not holding it
            dec     ZP_NO_PREEMPT
            bne     @done                           ; Still nested
            lda     ZP_PREEMPT_DUE
            beq     @done
            pla
            plp
            jmp     YIELD                           ; (YIELD clears ZP_PREEMPT_DUE)

@done:
            pla
            plp
            rts

; Wait for IO: the task isn't run again until IO_WAKE.
; Preserves .A, .X, .Y
TASK_WAIT:
            php
            sei
            smb2    TASK_STATUS_REG                 ; TASK_WAITING_FLAG
            jsr     YIELD
            plp
            rts

; Let a task waiting for IO (TASK_WAIT) run again.  Can be called from IRQ handlers.
; IN: .A = task
; Preserves .A, .X, .Y
IO_WAKE:
            php
            sei
            phy
            pha
            ldy     T_REGISTER
            and     #$0F
            sta     T_REGISTER                      ; Quick switch to the task (no stack use!)
            rmb2    TASK_STATUS_REG                 ; TASK_WAITING_FLAG
            sty     T_REGISTER
            pla
            ply
            plp
            rts

; A task's status (TASK_STATUS_REG).
; IN: .A = task.  OUT: .A = status
; Preserves .X, .Y
TASK_STATUS:
            php
            sei
            phy
            ldy     T_REGISTER
            and     #$0F
            sta     T_REGISTER                      ; Quick switch to the task (no stack use!)
            lda     TASK_STATUS_REG
            sty     T_REGISTER
            ply
            plp
            rts

; This task's bit in a 16-bit task mask (e.g. ZP_TC_WAITERS, ZP_SLEEPERS).
; OUT: .A = the bit, C = 1 if it's in the high byte (tasks 8-15), .X = this task.  Modifies: .Y
TASK_MY_BIT:
            lda     T_REGISTER
            and     #$0F
            tax
            and     #7
            tay
            lda     MMU_BIT_MASKS,Y
            cpx     #8
            rts

; Sleep for .A.Y ticks (the scheduler's tick: SCHED_TICK_HZ a second; up to 32767): the other tasks run
; meanwhile, or the system idles.  Modifies: .A, .Y.  Preserves .X
TASK_SLEEP:
            sta     ZP_SLEEP_UNTIL
            sty     ZP_SLEEP_UNTIL + 1
            jsr     TICKS_GET
            clc
            adc     ZP_SLEEP_UNTIL
            pha
            tya
            adc     ZP_SLEEP_UNTIL + 1
            tay
            pla

; Sleep until the tick count (TICKS_GET) reaches .A.Y (at most 32767 ticks ahead; a time that's come
; already returns at once).  The system task's tick handler wakes the task (SLEEP_CHECK); a break or kill
; ends the sleep too.  Modifies: .A, .Y.  Preserves .X
TASK_SLEEP_UNTIL:
            jsr     IO_FLUSH                        ; (Our output first: e.g. a line with no LF yet)
            php
            sei
            phx
            sta     ZP_SLEEP_UNTIL
            sty     ZP_SLEEP_UNTIL + 1

@check:
            lda     ZP_SLEEP_UNTIL
            ldy     ZP_SLEEP_UNTIL + 1
            ldx     T_REGISTER
            stz     T_REGISTER                      ; Quick look at the system task (no stack use!)
            clc
            sbc     ZP_TICKS                        ; The time - now - 1: negative once it's come
            tya
            sbc     ZP_TICKS + 1
            stx     T_REGISTER
            bmi     @done
            jsr     TASK_MY_BIT
            stz     T_REGISTER                      ; Quick switch to the system task (no stack use!)
            bcs     @high
            tsb     ZP_SLEEPERS                     ; One of its sleepers
            bra     @joined

@high:
            tsb     ZP_SLEEPERS + 1

@joined:
            stx     T_REGISTER
            smb2    TASK_STATUS_REG                 ; Wait (TASK_WAITING_FLAG), then look again (a wake
            jsr     YIELD                           ;   can come early: IO_WAKE)
            bra     @check

@done:
            plx
            plp
            rts

; The tick handler (VIA_IRQ_HANDLER; in the system task, IRQs off): wake the sleepers whose time has come
; (TASK_SLEEP_UNTIL).  Modifies: .A, .X, .Y
SLEEP_CHECK:
            lda     ZP_SLEEPERS
            sta     ZP_SLEEP_SCAN
            lda     ZP_SLEEPERS + 1
            sta     ZP_SLEEP_SCAN + 1
            ldx     #0                              ; .X = task

@loop:
            lda     ZP_SLEEP_SCAN                   ; Nobody (else) sleeping: done (usually at once)
            ora     ZP_SLEEP_SCAN + 1
            beq     @done
            lsr     ZP_SLEEP_SCAN + 1
            ror     ZP_SLEEP_SCAN
            bcc     @next
            stx     T_REGISTER                      ; Quick look at the sleeper (no stack use!)
            lda     ZP_SLEEP_UNTIL
            ldy     ZP_SLEEP_UNTIL + 1
            stz     T_REGISTER                      ; (Back in the system task)
            clc
            sbc     ZP_TICKS                        ; The time - now - 1: negative once it's come
            tya
            sbc     ZP_TICKS + 1
            bpl     @next
            txa
            and     #7
            tay
            lda     MMU_BIT_MASKS,Y
            cpx     #8
            bcs     @high
            trb     ZP_SLEEPERS                     ; Not sleeping any more
            bra     @wake

@high:
            trb     ZP_SLEEPERS + 1

@wake:
            txa
            jsr     IO_WAKE

@next:
            inx
            bra     @loop

@done:
            rts

; Start the scheduler's tick: VIA T1 free-running, one IRQ every TIMER_TASK_INT_H/L cycles (~5 ms).
; VIA_IRQ_HANDLER turns each tick into a task switch.
SCHED_START:
            php
            sei
            lda     VIA_R_AUX_CTRL
            and     #$3F
            ora     #$40                            ; T1 continuous, no PB7 output
            sta     VIA_R_AUX_CTRL
            lda     #TIMER_TASK_INT_L
            sta     VIA_R_T1C_L                     ; Latch low
            lda     #TIMER_TASK_INT_H
            sta     VIA_R_T1C_H                     ; Latch high, load and start
            lda     #VIA_INT_ENABLE | VIA_T1_INT_BIT
            sta     VIA_R_INT_ENABLE
            plp
            rts

; ****************************************************************************
; Starting tasks

; Build a new task's starting frame: it starts in TASK_TRAMPOLINE, which calls its entry point (on ROM
; page ZP_TEMP) and ends the task when that returns.  It gets copies of the current task's open fds.
; IRQs must be off; .X must not be the current task.
; IN: ZP_TEMP_VEC = entry point, ZP_TEMP = its ROM page, .X = task
; Modifies: .A, .Y
TASK_BUILD_FRAME:
            ldy     T_REGISTER                      ; .Y = current task, .X = new task

; !! NO STACK MANIPULATIONS WHILE IN THE NEW TASK !!
            lda     ZP_TEMP_VEC
            stx     T_REGISTER
            sta     ZP_TASK_ENTRY
            sty     T_REGISTER
            lda     ZP_TEMP_VEC + 1
            stx     T_REGISTER
            sta     ZP_TASK_ENTRY + 1
            sty     T_REGISTER
            lda     ZP_TEMP
            stx     T_REGISTER
            sta     ZP_TASK_PAGE
            lda     #>TASK_TRAMPOLINE               ; RTI frame: PCH, PCL, P (IRQs on)
            sta     $01FF
            lda     #<TASK_TRAMPOLINE
            sta     $01FE
            lda     #0
            sta     $01FD                           ; P
            sta     $01FC                           ; A
            sta     $01FB                           ; X
            sta     $01FA                           ; W
            sta     $01F9                           ; Y
            sta     $01F8                           ; ZP_TC_VEC
            sta     $01F7                           ; ZP_TC_VEC + 1
            sta     $01F6                           ; ZP_TC_TASK
            sta     $01F5                           ; U
            lda     #TASK_FRAME_SP
            sta     STACK_SAVE_REG
            stz     ZP_NO_PREEMPT
            stz     ZP_PREEMPT_DUE
            stz     ZP_IN_SCHED                     ; (RAM powers up random: a stray one stops preemption)
            stz     ZP_TC_GUEST
            stz     ZP_TC_WAITERS                   ; (Left by the task that had the number before)
            stz     ZP_TC_WAITERS + 1
            stz     ZP_IRQ_RESCHED
            stz     ZP_BREAK_VEC + 1                ; No break handler
            stz     ZP_OUT_CNT                      ; Nothing buffered for stdout or read ahead
            stz     ZP_IN_CNT                       ;   from stdin (STDOUT_PUT, STDIN_GET)
            stz     ZP_IN_POS
            sty     ZP_TASK_OWNER                   ; Started by the current task
            lda     #$FF
            sta     TASK_PARENT                     ; No parent to wake (TASK_START sets one)
            sty     T_REGISTER                      ; Back to the current task
            txa
            jmp     IO_INHERIT                      ; The fds (preserves .X)

; Every task started with TASK_BUILD_FRAME begins here (ROM page 0, IRQs on)
TASK_TRAMPOLINE:
            lda     ZP_TASK_ENTRY
            sta     ZP_FAR_VEC
            lda     ZP_TASK_ENTRY + 1
            sta     ZP_FAR_VEC + 1
            lda     ZP_TASK_PAGE
            sta     ZP_FAR_PAGE
            jsr     FAR_CALL_A                      ; Run the task

; The task's entry point returned: free everything it had, wake its parent, and never run again
TASK_EXIT:
            jsr     CONS_RELEASE                    ; (In the foreground: the console goes back)
            lda     T_REGISTER
            jsr     MM_TASK_RESET                   ; (IRQs on: it holds NO_PREEMPT)
            sei
            lda     TASK_PARENT
            cmp     #MAX_TASK_NUMBER + 1
            bcs     @no_parent
            ldy     T_REGISTER
            sta     T_REGISTER                      ; Quick switch to the parent (no stack use!)
            rmb1    TASK_STATUS_REG                 ; TASK_PAUSED_FLAG: it's waiting for us (TASK_START)
            sty     T_REGISTER

@no_parent:
            stz     TASK_STATUS_REG                 ; Free: never picked again
            jsr     YIELD

@halt:
            bra     @halt                           ; (not reached)

; A break or kill from the console (SER_BREAK flags the task; SCHED_RESUME continues it here, on ROM page
; 0 with IRQs on, instead of where it was).  A break goes to the task's break handler (TASK_SET_BREAK),
; with the stack pointer it had then; a kill, or a break without a handler, ends the task, except the
; shell, which starts again from scratch.
BREAK_ENTRY:
            sei
            lda     TASK_STATUS_REG
            tax
            and     #<~(TASK_BREAK_FLAG | TASK_KILL_FLAG | TASK_WAITING_FLAG)
            sta     TASK_STATUS_REG                 ; (It may have been waiting for IO)
            stz     ZP_NO_PREEMPT                   ; (Or holding the CPU, e.g. in IO_SERVE)
            stz     ZP_PREEMPT_DUE
            stz     RAM_BANK_REG                    ; (Or in the middle of a bank switch)
            cli
            txa
            and     #TASK_KILL_FLAG
            bne     @kill
            lda     ZP_BREAK_VEC + 1
            beq     @kill                           ; No handler
            ldx     ZP_BREAK_SP
            txs
            lda     ZP_BREAK_VEC
            sta     ZP_FAR_VEC
            lda     ZP_BREAK_VEC + 1
            sta     ZP_FAR_VEC + 1
            lda     ZP_BREAK_PAGE
            sta     ZP_FAR_PAGE
            jmp     FAR_JUMP

@kill:
            lda     T_REGISTER
            and     #$0F
            cmp     #SHELL_TASK_NUM
            beq     @shell
            jmp     TASK_EXIT

@shell:                                     ; The shell: free everything, and start it again
            ldx     #$FF
            txs
            jsr     MM_TASK_RESET                   ; (Its fds and namespace too)
            stz     ZP_BREAK_VEC + 1
            jmp     TASK_TRAMPOLINE                 ; (Its entry point is still SHELL_MAIN)

; Signal a task, like a Plan 9 note: a break or a kill (it's acted on when the task next runs: see
; BREAK_ENTRY), and a kill for the tasks it started (and theirs, 4 levels).  They stop waiting, so they
; run and see it.  Task 0 and the drivers (resident tasks) aren't signalled.  From the console keys
; (SER_BREAK), /dev/proc's ctl files and the shell's kill.
; IN: .A = TASK_BREAK_FLAG or TASK_KILL_FLAG, .X = task
; OUT: C = 0; or C = 1, .A = ERR_BAD_TASK (not a task that can be signalled)
; Modifies: .A, .X, .Y
TASK_SIGNAL:
            php
            sei
            ldy     T_REGISTER                      ; .Y = this task, all the way through
            cpx     #MAX_TASK_NUMBER + 1
            bcs     @bad
            cpx     #SYSTEM_TASK_NUM
            beq     @bad
            stx     T_REGISTER                      ; Quick look (no stack use!)
            bbr0    TASK_STATUS_REG, @bad_look      ; Free (TASK_BUSY_FLAG)
            bbs3    TASK_STATUS_REG, @bad_look      ; A driver (TASK_RESIDENT_FLAG)
            sty     T_REGISTER
            stx     ZP_SIG_TARGET
            jsr     TASK_FLAG
            ldx     #MAX_TASK_NUMBER                ; Tasks 15-1: started by it, or by one of those, ...?

@task:
            cpx     ZP_SIG_TARGET
            beq     @next
            stx     ZP_SIG_TASK
            lda     #4
            sta     ZP_SIG_CNT

@owner:                                             ; .X = the task, then its owner, ...
            stx     T_REGISTER                      ; Quick look (no stack use!)
            ldx     ZP_TASK_OWNER
            sty     T_REGISTER
            cpx     ZP_SIG_TARGET
            beq     @child
            cpx     #MAX_TASK_NUMBER + 1
            bcs     @not_child                      ; ($FF: nobody)
            dec     ZP_SIG_CNT
            bne     @owner
            bra     @not_child

@child:
            ldx     ZP_SIG_TASK
            lda     #TASK_KILL_FLAG
            jsr     TASK_FLAG

@not_child:
            ldx     ZP_SIG_TASK

@next:
            dex
            bne     @task
            plp
            clc
            rts

@bad_look:
            sty     T_REGISTER

@bad:
            plp
            lda     #ERR_BAD_TASK
            sec
            rts

.assert     TASK_BUSY_FLAG = 1 .and TASK_RESIDENT_FLAG = 8, error, "TASK_SIGNAL and TASK_FLAG test bits 0 and 3"

; Task .X gets flag .A (if it's busy and not a driver), and stops waiting.  IRQs off; .Y = this task.
; Preserves .A, .X, .Y
TASK_FLAG:
            stx     T_REGISTER                      ; Quick switch (no stack use!)
            bbr0    TASK_STATUS_REG, @done          ; Free
            bbs3    TASK_STATUS_REG, @done          ; A driver
            tsb     TASK_STATUS_REG
            rmb2    TASK_STATUS_REG                 ; Not waiting any more (TASK_WAITING_FLAG)

@done:
            sty     T_REGISTER
            rts

; Set the current task's break handler: where a break from the console (SER_KEY_BREAK) sends it, with
; the stack pointer it has now (the handler never returns).  .A.Y = 0: no handler (a break ends the task).
; IN: .A.Y = handler, .X = its ROM page.  Preserves .A, .X, .Y
TASK_SET_BREAK:
            php
            sei
            sta     ZP_BREAK_VEC
            sty     ZP_BREAK_VEC + 1
            stx     ZP_BREAK_PAGE
            phx
            tsx
            inx                                     ; (The phx, the php and the return address)
            inx
            inx
            inx
            stx     ZP_BREAK_SP
            plx
            plp
            rts

; Start a task in the background: it runs alongside the caller, and ends when its entry point returns.
; IN: .A.Y = entry point, .X = its ROM page (0 for RAM or page 0 code)
; OUT (success): .A = task, C = 0
; OUT (failure): .A = ERR_NO_TASKS_AVAILABLE, C = 1
; Preserves .X, .Y
TASK_RUN:
            jsr     IO_FLUSH                        ; (Our output first: the new task shares fd 1)
            php
            sei
            PUSH_XY
            sta     ZP_TEMP_VEC
            sty     ZP_TEMP_VEC + 1
            stx     ZP_TEMP
            jsr     RESERVE_TASK                    ; C = 0: .A = task (busy + paused)
            bcs     @done                           ; (.A = ERR_NO_TASKS_AVAILABLE)
            tax
            jsr     TASK_BUILD_FRAME
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the new task (no stack use!)
            rmb1    TASK_STATUS_REG                 ; Runnable
            sty     T_REGISTER
            txa
            clc

@done:
            PULL_YX
            jmp     MM_RETURN

; Let a task that RESERVE_TASK left paused run (TASK_CLONE sets it up first).  IN: .A = task
; Preserves .A, .X, .Y
TASK_GO:
            php
            sei
            phy
            ldy     T_REGISTER
            sta     T_REGISTER                      ; Quick switch to the task (no stack use!)
            rmb1    TASK_STATUS_REG                 ; Runnable
            sty     T_REGISTER
            ply
            plp
            rts

; .A.Y: Address of task entrypoint
SPAWN_TASK:
            sta     ZP_TEMP_VEC
            sty     ZP_TEMP_VEC + 1

; Start a task at the address in ZP_TEMP_VEC (RAM or ROM page 0), and wait for it to finish.
; OUT: .A = the task #, C = 0; or .A = ERR_NO_TASKS_AVAILABLE, C = 1
TASK_START:
            php
            sei
            stz     ZP_TEMP                         ; ROM page 0
            jsr     RESERVE_TASK                    ; C = 0: .A = task (busy + paused)
            bcc     @start_task
            plp
            sec
            rts

@start_task:
            sta     TASK_SAVE_REG                   ; Child task #, returned when it's done
            tax
            jsr     TASK_BUILD_FRAME
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the child (no stack use!)
            sty     TASK_PARENT                     ; It wakes us when it's done (TASK_EXIT)
            rmb1    TASK_STATUS_REG                 ; Runnable
            sty     T_REGISTER
            smb1    TASK_STATUS_REG                 ; We're paused until then
            jsr     YIELD
            plp
            lda     TASK_SAVE_REG
            clc
            rts


; Find an available task, and mark it busy and paused
; OUT: .A = the task, C = 0; or .A = ERR_NO_TASKS_AVAILABLE, C = 1
; Preserves .X, .Y
RESERVE_TASK:
            php                                     ; Save caller's I flag
            sei                                     ; Disable interrupts
            PUSH_XY

; !! NO STACK MANIPULATIONS UNTIL SWITCHING BACK TO ORIGINAL TASK !!
            ldy     T_REGISTER
            ldx     #$F                             ; Start search with Task $F

@task_busy:
            stx     T_REGISTER                      ; Quick task switch to task in .X
            bbr0    TASK_STATUS_REG, @task_found    ; Is Bit 0 (TASK_BUSY_FLAG) reset/clear?
            dex                                     ; Not found, so check next
            bne     @task_busy                      ; Until we reach the system task (0), loop
            ldx     #ERR_NO_TASKS_AVAILABLE         ; Not found
            sec
            bra     @cleanup

@task_found:
            smb0    TASK_STATUS_REG
            smb1    TASK_STATUS_REG
            clc                                     ; Found

@cleanup:
            txa                                     ; The task number (or the error)
            sty     T_REGISTER                      ; Switch back to the original task

; Back on the original task, so restore the registers
            PULL_YX
            jmp     MM_RETURN                       ; Restore caller's I flag, keep C

; ****************************************************************************
; Run a routine in another task's context: its ZP, stack (below its saved SP), RAM bank and MMU area.
; Used by the IRQ dispatcher and DRV_START, and by gates into driver tasks.
;
; IN:  ZP_TC_VEC = routine, ZP_TC_TASK = task to run it in, .A/.X/.Y/C = routine's inputs
; OUT: .A/.X/.Y/flags as returned by the routine
; The routine runs with the caller's I flag.  The target task must not be running, or it's the
; current task (then this is a plain call).  If the target is busy with another task's call (switched
; out in the middle of it), the caller waits until it's done (see the scheduler's notes above).
; While the routine runs, the caller is in a call (TASK_CALLING_FLAG): the scheduler leaves it alone.
; The calling task # is kept on the target task's stack, so IRQs during the routine are safe.
.macro _M_TC_COPY_TO    zp                  ; ZP byte: calling task (.X) -> target task (.Y); ends in calling task
            lda     zp
            sty     T_REGISTER
            sta     zp
            stx     T_REGISTER
.endmacro

.macro _M_TC_COPY_BACK  zp                  ; ZP byte: target task (.Y) -> calling task (.X); ends in target task
            lda     zp
            stx     T_REGISTER
            sta     zp
            sty     T_REGISTER
.endmacro

TASK_CALL:
            php
            sei
            jsr     TC_WAIT_FREE                    ; (Preserves .A, .X, .Y)
            bra     TC_GO

; TASK_CALL for the IRQ dispatcher (IRQs off): runs the routine at once, even in a task that's busy with
; another task's call
TASK_CALL_IRQ:
            php

TC_GO:
            sta     ZP_TC_A
            pla
            sta     ZP_TC_P                         ; Caller's flags (C in, I state)
            stx     ZP_TC_X
            sty     ZP_TC_Y
            lda     ZP_TC_TASK
            cmp     T_REGISTER
            bne     @switch
            lda     ZP_TC_P                         ; Same task: restore flags and tail-call
            pha
            lda     ZP_TC_A
            plp
            jmp     (ZP_TC_VEC)

@switch:
            lda     STACK_SAVE_REG                  ; Preserve the calling task's saved SP
            pha
            tsx
            stx     STACK_SAVE_REG
            lda     T_REGISTER
            sta     ZP_TC_FROM
            tax                                     ; .X = calling task
            ldy     ZP_TC_TASK                      ; .Y = target task
            _M_TC_COPY_TO   ZP_TC_VEC
            _M_TC_COPY_TO   ZP_TC_VEC + 1
            _M_TC_COPY_TO   ZP_TC_A
            _M_TC_COPY_TO   ZP_TC_X
            _M_TC_COPY_TO   ZP_TC_Y
            _M_TC_COPY_TO   ZP_TC_P
            _M_TC_COPY_TO   ZP_TC_FROM
            lda     ZP_NO_PREEMPT                   ; The caller holds the CPU (NO_PREEMPT): the call
            sty     T_REGISTER                      ;   does too, though it runs in the target task
            sta     ZP_TC_HOLD
            stx     T_REGISTER
            smb6    TASK_STATUS_REG                 ; In a call (TASK_CALLING_FLAG): not run until it returns

; !! NO STACK MANIPULATIONS UNTIL THE TARGET TASK'S STACK IS SELECTED !!
            sty     T_REGISTER                      ; Switch to the target task
            ldx     STACK_SAVE_REG                  ; ...and its stack
            txs
            inc     ZP_TC_GUEST                     ; Running for another task: the scheduler mustn't switch
            lda     ZP_TC_HOLD                      ; (Kept on the target's stack: a nested call can
            pha                                     ;   change ZP_TC_HOLD)
            beq     :+
            inc     ZP_NO_PREEMPT
:
            lda     ZP_TC_FROM
            pha                                     ; Keep the calling task # on the target's stack
            lda     ZP_TC_P
            pha
            ldx     ZP_TC_X
            ldy     ZP_TC_Y
            lda     ZP_TC_A
            plp                                     ; Caller's flags
            jsr     @call
            php
            sei
            dec     ZP_TC_GUEST
            sta     ZP_TC_A
            stx     ZP_TC_X
            sty     ZP_TC_Y
            bne     :+                              ; (Z from the dec) Still in a call
            ldx     #ZP_TC_WAITERS
            jsr     TASK_WAKE_MASK                  ; Free: the tasks waiting to call us
:
            pla
            sta     ZP_TC_P                         ; Routine's result flags
            tsx
            inx
            inx
            stx     STACK_SAVE_REG                  ; Our SP as it was (a task switch during the call moved it)
            pla
            tax                                     ; .X = calling task
            pla                                     ; (ZP_TC_HOLD)
            beq     :+
            dec     ZP_NO_PREEMPT
:
            ldy     T_REGISTER                      ; .Y = target task
            _M_TC_COPY_BACK ZP_TC_A
            _M_TC_COPY_BACK ZP_TC_X
            _M_TC_COPY_BACK ZP_TC_Y
            _M_TC_COPY_BACK ZP_TC_P

; !! NO STACK MANIPULATIONS UNTIL THE CALLING TASK'S STACK IS SELECTED !!
            stx     T_REGISTER                      ; Back to the calling task
            rmb6    TASK_STATUS_REG                 ; (TASK_CALLING_FLAG)
            ldx     STACK_SAVE_REG                  ; ...and its stack
            txs
            pla
            sta     STACK_SAVE_REG                  ; Restore the calling task's saved SP
            ldx     ZP_TC_X
            ldy     ZP_TC_Y
            lda     ZP_TC_P
            pha
            lda     ZP_TC_A
            plp                                     ; Routine's result flags
            rts

@call:
            jmp     (ZP_TC_VEC)

.assert     TASK_CALLING_FLAG = $40 .and TASK_GUEST_OUT_FLAG = $80, error, "TASK_CALL and SCHED_SWITCH use smb6/rmb6 and smb7/rmb7"

; TASK_CALL: wait while the target task (ZP_TC_TASK) is busy with another task's call, as one of its
; waiters (TASK_WAKE_MASK wakes them when the call returns).  IRQs off.
; Preserves .A, .X, .Y
TC_WAIT_FREE:
            PUSH_AXY

@check:
            ldy     ZP_TC_TASK
            ldx     T_REGISTER
            sty     T_REGISTER                      ; Quick look at the target (no stack use!)
            lda     ZP_TC_GUEST
            stx     T_REGISTER
            beq     @free
            tya
            eor     T_REGISTER
            and     #$0F
            beq     @free                           ; (Ourselves: a plain call)
            jsr     TASK_MY_BIT                     ; Our bit in its ZP_TC_WAITERS
            ldy     ZP_TC_TASK
            sty     T_REGISTER                      ; Quick switch to the target (no stack use!)
            bcs     @high
            tsb     ZP_TC_WAITERS
            bra     @joined

@high:
            tsb     ZP_TC_WAITERS + 1

@joined:
            stx     T_REGISTER
            smb2    TASK_STATUS_REG                 ; Wait (TASK_WAITING_FLAG), then look again
            jsr     YIELD
            bra     @check

@free:
            PULL_YXA
            rts

; Wake every task in a 16-bit wait mask in the current task's ZP (bit = task), and clear the mask.
; IN: .X = the mask's ZP address.  Modifies: .A, .Y
TASK_WAKE_MASK:
            php
            sei
            lda     0,X
            ora     1,X
            beq     @done
            ldy     #0

@loop:
            lsr     1,X
            ror     0,X
            bcc     :+
            tya
            jsr     IO_WAKE
:
            iny
            cpy     #16
            bne     @loop

@done:
            plp
            rts

; ****************************************************************************
; Make a free task (a specific one) ready to run from an entry point on ROM page 0: marks it busy and
; runnable, so the scheduler starts it.  (The shell task at boot.)
; IN: .A.Y = entry point, .X = task#
; OUT (success): C = 0
; OUT (failure): .A = ERR_BAD_TASK or ERR_TASK_BUSY, C = 1
; Modifies: .A, .Y
TASK_PREPARE:
            php                                     ; Save caller's I flag
            sei
            cpx     #MAX_TASK_NUMBER + 1
            bcs     @bad_task
            cpx     T_REGISTER
            beq     @bad_task
            sta     ZP_TEMP_VEC
            sty     ZP_TEMP_VEC + 1
            ldy     T_REGISTER                      ; .Y = calling task, .X = new task
            stx     T_REGISTER                      ; Quick look at the new task (no stack use!)
            lda     TASK_STATUS_REG
            sty     T_REGISTER
            bne     @busy
            stz     ZP_TEMP                         ; ROM page 0
            jsr     TASK_BUILD_FRAME
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the new task (no stack use!)
            lda     #TASK_BUSY_FLAG                 ; Busy and runnable
            sta     TASK_STATUS_REG
            sty     T_REGISTER                      ; Back to the calling task
            clc
            jmp     MM_RETURN

@busy:
            lda     #ERR_TASK_BUSY
            sec
            jmp     MM_RETURN

@bad_task:
            lda     #ERR_BAD_TASK
            sec
            jmp     MM_RETURN

; ****************************************************************************
; Drivers

.struct     DriverInfo
            init        .word                       ; Runs in the driver's task.  OUT: C = 0 OK, or C = 1 and .A = error
            stop        .word                       ; (future) Runs in the driver's task before the task is reset
            name        .word                       ; HString
.endstruct

; Start a driver in a (free) task, as a Resident task: its init runs in that task, so the driver's
; state lives in that task's ZP/RAM, and IRQ handlers it registers run in that task.
; IN: .A.Y = DriverInfo, .X = task#
; OUT (success): .A = task#, C = 0
; OUT (failure): .A = ERROR, C = 1 (the task is left free, with its IRQ handlers and devices removed)
; Modifies: .A, .X, .Y
DRV_START:
            php                                     ; Save caller's I flag
            sei
            sta     ZP_DRV_PTR
            sty     ZP_DRV_PTR + 1
            cpx     #MAX_TASK_NUMBER + 1
            bcs     @bad_task
            cpx     T_REGISTER
            beq     @bad_task
            stx     ZP_TC_TASK
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the driver task (no stack use!)
            lda     TASK_STATUS_REG
            bne     @busy
            lda     #TASK_BUSY_FLAG | TASK_RESIDENT_FLAG
            sta     TASK_STATUS_REG
            sty     T_REGISTER                      ; Back to the calling task
            ldy     #DriverInfo::init
            lda     (ZP_DRV_PTR),Y
            sta     ZP_TC_VEC
            iny
            lda     (ZP_DRV_PTR),Y
            sta     ZP_TC_VEC + 1
            lda     ZP_TC_TASK                      ; init gets its task# in .A
            jsr     TASK_CALL
            bcs     @init_failed
            lda     ZP_TC_TASK
            clc
            jmp     MM_RETURN

@init_failed:
            pha                                     ; (The error)
            lda     ZP_TC_TASK                      ; Whatever it registered before failing goes too:
            jsr     IRQ_UNREGISTER_TASK             ;   nothing may call into a free task
            jsr     DEV_UNREGISTER_TASK
            pla
            ldx     ZP_TC_TASK
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the driver task (no stack use!)
            stz     TASK_STATUS_REG                 ; Leave it free
            sty     T_REGISTER
            sec                                     ; .A = error from init
            jmp     MM_RETURN

@busy:
            sty     T_REGISTER                      ; Back to the calling task
            lda     #ERR_TASK_BUSY
            sec
            jmp     MM_RETURN

@bad_task:
            lda     #ERR_BAD_TASK
            sec
            jmp     MM_RETURN

; Non-maskable interrupt handler, called from NMI_ENTRY (COMMON block) on ROM page 0
NMI_HANDLER:
            rts
