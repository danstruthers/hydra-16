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
;   4: 0 = , 1 =
;   5: 0 = , 1 =
;   6: 0 = , 1 =
;   7: 0 = , 1 =

.macro SELECT_TASK      task
            lda     T_REGISTER
            and     #$F0
            ora     #(task & $0F)
            sta     T_REGISTER
.endmacro

.macro SELECT_SHARED_BANK bank
            lda     T_REGISTER
            and     #$0F
            ora     #(bank << 4)
            sta     T_REGISTER
.endmacro

; Initialize the tasks, their stacks, etc.
TASKS_INIT:
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
            stz     ZP_TC_GUEST
            stz     ZP_IRQ_RESCHED
            stz     ZP_BREAK_VEC + 1                ; No break handler
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
            cli                                     ; Turn interrupts back on
            rts

; ****************************************************************************
; Scheduler (see IO_PLAN.md, Phase 1)
;
;   Every task that isn't running keeps the same frame on its own stack: the one IRQ_DISPATCH builds
;   (interrupt frame, A, X, W, Y, the TASK_CALL scratch bytes), plus U.  Top of stack first:
;       U, ZP_TC_TASK, ZP_TC_VEC + 1, ZP_TC_VEC, Y, W, X, A, P, PCL, PCH
;   and STACK_SAVE_REG is the SP below it.  So a task switch is always the same: save SP, pick a task,
;   load its SP, unwind its frame (SCHED_RESUME), whether the task stopped for the timer tick (IRQ),
;   YIELD, or to wait for IO.  W and U are in the frame because they're global pseudo-registers.
;
;   Runnable: busy, and not paused, waiting or resident.  Round-robin over tasks 1-15; task 0 is the
;   idle task and only runs when nothing else can.

TASK_WAITING_FLAG       = 4                         ; Bit 2: awaiting IO (TASK_WAIT / IO_WAKE)
TASK_BREAK_FLAG         = $10                       ; Bit 4: a break is due (console: SER_BREAK)
TASK_KILL_FLAG          = $20                       ; Bit 5: a kill is due (console: SER_BREAK)
TASK_RUN_MASK           = TASK_BUSY_FLAG | TASK_PAUSED_FLAG | TASK_WAITING_FLAG | TASK_RESIDENT_FLAG
TASK_FRAME_SP           = $F4                       ; STACK_SAVE_REG of a new task (11-byte frame at $01F5)
SCHED_RESCHED_A         = $A5                       ; An IRQ handler returns C = 1, .A and .Y = these
SCHED_RESCHED_Y         = $5A                       ;   to ask for a task switch (the timer tick)

; Switch tasks.  IRQs off, and the current task's full frame (including U) on its stack.
SCHED_SWITCH:
            tsx
            stx     STACK_SAVE_REG                  ; The current task's SP
            jsr     SCHED_PICK                      ; .A = next task (maybe the same one)
            sta     T_REGISTER                      ; Its ZP and stack
            ldx     STACK_SAVE_REG
            txs

; Unwind a task's frame and continue it; or, if a break or kill is due, continue it at BREAK_ENTRY
; instead (on ROM page 0, IRQs on, U = 0)
SCHED_RESUME:
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
; the current one if it's runnable; else task 0 (idle).  No stack use while looking at other tasks.
; OUT: .A = task
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
            stx     T_REGISTER                      ; Quick look at the candidate (no stack use!)
            lda     TASK_STATUS_REG
            sty     T_REGISTER
            and     #TASK_RUN_MASK
            cmp     #TASK_BUSY_FLAG
            beq     @found

@skip:
            dec     ZP_SCHED_CNT
            bne     @next
            lda     TASK_STATUS_REG                 ; Nobody else: keep going if we can
            and     #TASK_RUN_MASK
            cmp     #TASK_BUSY_FLAG
            beq     @stay
            lda     #SYSTEM_TASK_NUM                ; Idle
            rts

@stay:
            tya
            rts

@found:
            txa
            rts

; Can the interrupted (current) task be preempted?  Not if it's running a TASK_CALL routine for another
; task (a guest), isn't runnable (e.g. a resident driver task), or holds NO_PREEMPT (then the switch is
; noted, for PREEMPT).
; OUT: C = 1 switch, C = 0 don't
SCHED_CAN_PREEMPT:
            lda     ZP_TC_GUEST
            bne     @no
            lda     TASK_STATUS_REG
            and     #TASK_RUN_MASK
            cmp     #TASK_BUSY_FLAG
            bne     @no
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
            stz     ZP_TC_GUEST
            stz     ZP_IRQ_RESCHED
            stz     ZP_BREAK_VEC + 1                ; No break handler
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
            sei
            lda     T_REGISTER
            jsr     MM_TASK_RESET
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
            php
            sei
            PUSH_XY
            sta     ZP_TEMP_VEC
            sty     ZP_TEMP_VEC + 1
            stx     ZP_TEMP
            jsr     RESERVE_TASK                    ; C = 1: .A = task (busy + paused)
            bcc     @none
            tax
            jsr     TASK_BUILD_FRAME
            ldy     T_REGISTER
            stx     T_REGISTER                      ; Quick switch to the new task (no stack use!)
            rmb1    TASK_STATUS_REG                 ; Runnable
            sty     T_REGISTER
            txa
            clc
            bra     @done

@none:
            lda     #ERR_NO_TASKS_AVAILABLE
            sec

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
; Return task # in A and C == 1
;   OR error in A and C == 0 (if no task available)
TASK_START:
            php
            sei
            stz     ZP_TEMP                         ; ROM page 0
            jsr     RESERVE_TASK                    ; C = 1: .A = task (busy + paused)
            bcs     @start_task
            plp
            lda     #ERR_NO_TASKS_AVAILABLE
            clc
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
            sec
            rts


; Find an available task
; Modifies: A, CNZ Flags
; Returns C = 1 AND A = TaskNumber (when found)
; Returns C = 0 AND A = $FF        (when not found)
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
            clc                                     ; Not found
            dex                                     ; .X == $FF
            bra     @cleanup

@task_found:
            smb0    TASK_STATUS_REG
            smb1    TASK_STATUS_REG
            sec                                     ; Found

@cleanup:
            txa                                     ; Return the task number in A (OR $FF if not found)
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
; current task (then this is a plain call).
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

; !! NO STACK MANIPULATIONS UNTIL THE TARGET TASK'S STACK IS SELECTED !!
            sty     T_REGISTER                      ; Switch to the target task
            ldx     STACK_SAVE_REG                  ; ...and its stack
            txs
            inc     ZP_TC_GUEST                     ; Running for another task: the scheduler mustn't switch
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
            pla
            sta     ZP_TC_P                         ; Routine's result flags
            pla
            tax                                     ; .X = calling task
            ldy     T_REGISTER                      ; .Y = target task
            _M_TC_COPY_BACK ZP_TC_A
            _M_TC_COPY_BACK ZP_TC_X
            _M_TC_COPY_BACK ZP_TC_Y
            _M_TC_COPY_BACK ZP_TC_P

; !! NO STACK MANIPULATIONS UNTIL THE CALLING TASK'S STACK IS SELECTED !!
            stx     T_REGISTER                      ; Back to the calling task
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
; OUT (failure): .A = ERROR, C = 1 (the task is left free)
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
