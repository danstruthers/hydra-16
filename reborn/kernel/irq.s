; ****************************************************************************
; irq.s - interrupts: one path for every line (docs/reimplementation-from-scratch.md, §10.3).
;
; A line's vector points at its stub in the COMMON block (common.s), which goes to IRQ_DISPATCH on page 0 with
; .A = the line and the frame begun on the interrupted task's stack (A, X, W; the CPU's P and PC).  The line's
; owner (TA_OWNERS: every task has a copy, so the interrupted task's is read where it is) gets the interrupt in
; its own task: T switched to it (its zero page, its stack below its frame, its banks: its module at $A000), its
; irq entry (TA_IRQVEC) called with .A = the line.  The entry answers .A = 0, or IRQ_RESCHED for a task switch
; (it woke a task; the tick asks every time); the interrupted task is switched out unless it holds preemption
; (then the switch is noted: TK_DUE).
;
; The owner can be the interrupted task itself (the tick, in the idle task): the same steps work, on its stack.
; IRQs stay off throughout, and handlers never wait.  The budget: about 80 cycles from the interrupt to the
; handler's first instruction (sim/test.js measures it: the irq test).

.include "kdefs.inc"

.segment "KCODE"

; IN: .A = the line; IRQs off; on page 0.  The stack: the frame's W, X, A, P, PCL, PCH.  (The CPU cleared D)
IRQ_DISPATCH:
            phy                                             ; (The frame's Y)
            tay                                             ; .Y = the line
            tsx
            stx         TK_SP                               ; The interrupted task's stack pointer, for the way back
            ldx         TA_OWNERS,Y                         ; .X = the line's owner
            bmi         @stray
            lda         T_REGISTER                          ; .A = the interrupted task
            stx         T_REGISTER                          ; ---- The owner: its zero page and stack page (not S yet)
            ldx         TK_SP
            txs                                             ; Its stack: below its frame (or the same, if it's the
            pha                                             ;   interrupted task).  The interrupted task, for later
            tya                                             ; .A = the line
            jsr         IRQ_HANDLER
            ply                                             ; ---- Back to the interrupted task
            sty         T_REGISTER
            ldx         TK_SP
            txs
            and         #IRQ_RESCHED                        ; A task switch, please?
            beq         IRQ_RESTORE
            lda         TK_PREEMPT                          ; Not while it holds the CPU (or is in the scheduler):
            bne         @due                                ;   then it's noted, for PREEMPT_ON or the next YIELD
            lda         U_REGISTER                          ; The frame's last byte, then the switch
            pha
            jmp         K_SCHED_SWITCH

@due:
            lda         #1
            sta         TK_DUE
            bra         IRQ_RESTORE

@stray:                                                     ; Nobody's: counted (a line must be owned before its
            ldx         T_REGISTER                          ;   device interrupts: a held line comes straight back)
            stz         T_REGISTER
            lda         K_IRQ_STRAY,Y
            inc         a
            beq         :+                                  ; (It stops at 255)
            sta         K_IRQ_STRAY,Y
:
            stx         T_REGISTER

; The end of an interrupt with no task switch (and the end of SCHED_RESUME's unwinding, in sched.s)
IRQ_RESTORE:
            ply
            jmp         IRQ_EXIT

IRQ_HANDLER:
            jmp         (TA_IRQVEC)

; ****************************************************************************
; Point every line's vector at its stub, and give the VIA's line (the tick) to the kernel task.  At boot, in task
; 0, IRQs off; every task's owners are $FF (reset.s).  (A vector is written at the entry V selects while no line
; is active: the devices are quiet then.)
IRQ_INIT:
            lda         #<IRQ_STUB_0
            sta         K_PTR
            lda         #>IRQ_STUB_0
            sta         K_PTR + 1
            ldx         #0                                  ; The line
@vector:
            txa
            eor         #7                                  ; (IRQ_INDEX)
            sta         V_REGISTER
            lda         K_PTR
            sta         VECTOR_RAM
            lda         K_PTR + 1
            sta         VECTOR_RAM + 1
            lda         K_PTR                               ; The next stub: 6 bytes on
            clc
            adc         #6
            sta         K_PTR
            bcc         :+
            inc         K_PTR + 1
:
            inx
            cpx         #LINES
            bne         @vector
            lda         #IRQ_INDEX(LINE_NONE)               ; V back on BRK's entry
            sta         V_REGISTER
            ldx         #LINE_VIA                           ; The tick: the kernel task's
            lda         #KERNEL_TASK
            jmp         IRQ_SET_OWNER

; The kernel task's irq entry: the VIA's timer 1, the tick (the only line it owns).  The tick count and the clock;
; a task switch every tick (the scheduler wakes the sleepers whose time has come).  OUT: .A = IRQ_RESCHED or 0
K_KIRQ:
            lda         VIA_IFR
            and         #VIA_IRQ_T1
            beq         @not
            lda         VIA_T1CL                            ; (Clears its flag)
            inc         K0_TICKS
            bne         :+
            inc         K0_TICKS + 1
:
            dec         K0_CLOCKSUB                         ; A second gone?
            bmi         @second
            lda         #IRQ_RESCHED
            rts

@second:
            lda         #TICK_HZ - 1
            sta         K0_CLOCKSUB
            inc         K0_CLOCK
            bne         @resched
            inc         K0_CLOCK + 1
            bne         @resched
            inc         K0_CLOCK + 2
            bne         @resched
            inc         K0_CLOCK + 3
@resched:
            lda         #IRQ_RESCHED
            rts

@not:
            lda         #0
            rts

; Line .X's owner = .A ($FF: none), in every task's copy.  In the kernel task (a KCALL, or the boot); keeps the
; I flag.  Modifies .Y
IRQ_SET_OWNER:
            ldy         #TASKS - 1
@task:
            php
            sei
            sty         T_REGISTER                          ; (A quick look)
            sta         TA_OWNERS,X
            stz         T_REGISTER
            plp
            dey
            bpl         @task
            rts

; IRQ_OWN: own a line.  IN: .A = the line.  OUT: C = 0; or C = 1, .A = E_RANGE, E_INVAL (no irq entry), E_BUSY
K_IRQ_OWN:
            cmp         #LINE_NONE
            bcs         @range
            ldx         TA_IRQVEC + 1
            beq         @inval
            KCALL       K_IRQ_OWN_K
            rts

@range:
            FAIL        E_RANGE

@inval:
            FAIL        E_INVAL

; In the kernel task (KCALL): line .A for task .Y, if it's nobody's (or the task's already)
K_IRQ_OWN_K:
            tax
            lda         TA_OWNERS,X
            bmi         @take
            sty         K0_TMP
            cmp         K0_TMP
            beq         @ours
            FAIL        E_BUSY

@take:
            tya
            jsr         IRQ_SET_OWNER
@ours:
            clc
            rts

; IRQ_RELEASE: give a line back.  IN: .A = the line.  OUT: C = 0; or C = 1, .A = E_RANGE, E_PERM
K_IRQ_RELEASE:
            cmp         #LINE_NONE
            bcs         @range
            KCALL       K_IRQ_RELEASE_K
            rts

@range:
            FAIL        E_RANGE

; In the kernel task (KCALL): line .A, back from task .Y
K_IRQ_RELEASE_K:
            tax
            tya
            cmp         TA_OWNERS,X
            bne         @perm
            lda         #$FF
            jsr         IRQ_SET_OWNER
            clc
            rts

@perm:
            FAIL        E_PERM

; Every line task .Y owns, nobody's (its end).  In the kernel task.  Modifies .A, .X
IRQ_RELEASE_ALL:
            ldx         #LINES - 1
@line:
            tya
            cmp         TA_OWNERS,X
            bne         :+
            phy
            lda         #$FF
            jsr         IRQ_SET_OWNER
            ply
:
            dex
            bpl         @line
            rts

; The NMI (a slot card's): nothing yet.  On page 0, from the COMMON block's NMI_ENTRY
NMI_HANDLER:
            rts
