; ****************************************************************************
; irq.s - interrupts: one path for every line (docs/design/reimplementation-from-scratch.md, §10.3).
;
; A line's vector points at its stub in the COMMON block (common.s), which goes on, on page 0, to the dispatcher
; (its main path is in the COMMON block too: it saves two jumps) with .A = the line and the frame begun on the
; interrupted task's stack (A, X, W; the CPU's P and PC).  The line's owner (TA_OWNERS: every task has a copy, so
; the interrupted task's is read where it is) gets the interrupt in its own task: T switched to it (its zero
; page, its stack below its frame, its banks: its module at $A000), its irq entry (TA_IRQVEC) called with .A =
; the line, .X = the task interrupted.  The entry answers .A = 0, or IRQ_RESCHED for a task switch (the tick
; asks every time); the interrupted task is switched out unless it holds preemption (then the switch is noted:
; TK_DUE).  Here: what's off that path, a line nobody owns (IRQ_STRAY) and the switch (IRQ_SWITCH).
;
; The owner can be the interrupted task itself (the tick, in the idle task): the same steps work, on its stack.
; IRQs stay off throughout, and handlers never wait.  The budget: about 75 cycles from the interrupt to the
; handler's first instruction, and 115 for the whole path less the handler (sim/test.js measures them: the
; IRQs-off stretches), so a handler has about 85 of the 200 cycles any IRQs-off stretch may take.
;
; VIA timer 2 is a line of its own, LINE_VIA_T2 (16), so its owner (the console: its paced sending) gets it in one
; step: the VIA's stub sends its interrupt there (common.s: IRQ_VIA).  Owning the line is owning the timer: IRQ_OWN
; sets it one-shot, its interrupt on; the owner starts it (T2CL, then T2CH) and its interrupt clears its flag
; (reading T2CL, or starting it again).  CA1 is one too, LINE_VIA_CA1 (17): owning it turns CA1's interrupt on (off
; again as it's given back), and the owner (the GPIO driver) clears its flag (IFR).

.assert     LINE_VIA = 0, error, "IRQ_VIA's .A = 0 is the VIA's line"

.include "kdefs.inc"

.segment "KCODE"

; The owner's irq entry (the dispatcher's jsr)
IRQ_HANDLER:
            jmp         (TA_IRQVEC)

; An interrupt's task switch, from the dispatcher: back in the interrupted task (it doesn't hold the CPU), its
; frame's Y on its stack.  It's switched out
IRQ_SWITCH:
            lda         U_REGISTER                          ; The frame's last byte, then the switch
            pha
            jmp         K_SCHED_SWITCH

; A line nobody owns, from the dispatcher: .Y = the line; in the interrupted task, its frame's Y on its stack
IRQ_STRAY:
            cpy         #LINE_NONE                          ; A BRK?  (Line 15's entry, and B set in the P it pushed:
            bne         irq_stray_n                         ;   the frame is Y, W, X, A, P ...)
            ldx         TK_SP
            lda         $0105,X
            and         #$10
            beq         irq_stray_n
            bit         TK_FLAGS                            ; The debugger's (TF_TRAP: a step out, or breakpoints)?
            bvc         IRQ_BRK_NOTE
            jmp         K_TRAP                              ; (debug.s: .X = TK_SP)

; A BRK that's the program's own (from IRQ_STRAY and K_TRAP): its frame's Y on its stack
IRQ_BRK_NOTE:
            lda         #1 << NOTE_BRK                      ; The note sys: brk, taken (notes.s) when the switch
            tsb         TK_NOTES                            ;   back to it finds it in its own code
            sta         TK_NOTED
            lda         TK_PREEMPT                          ; A switch, if it doesn't hold the CPU; else noted
            beq         IRQ_SWITCH
            sta         TK_DUE
            jmp         IRQ_RESTORE

irq_stray_n:                                                ; Nobody's: counted (a line must be owned before its
            cpy         #LINE_VIA_T2                        ;   device interrupts: a held line comes straight back)
            beq         @t2
            cpy         #LINE_VIA_CA1
            beq         @ca1
            ldx         T_REGISTER
            stz         T_REGISTER
            lda         K_IRQ_STRAY,Y
            inc         a
            beq         :+                                  ; (It stops at 255)
            sta         K_IRQ_STRAY,Y
:
            stx         T_REGISTER
            jmp         IRQ_RESTORE

@t2:                                                        ; (Timer 2 with no owner: its interrupt off, its flag
            jsr         T2_OFF                              ;   cleared)
            jmp         IRQ_RESTORE

@ca1:                                                       ; (CA1 the same)
            jsr         CA1_OFF
            jmp         IRQ_RESTORE

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
            lda         #IRQ_INDEX(LINE_VIA)                ; The VIA's: IRQ_VIA itself (it looks at the ACIA's
            sta         V_REGISTER                          ;   first: common.s)
            lda         #<IRQ_VIA
            sta         VECTOR_RAM
            lda         #>IRQ_VIA
            sta         VECTOR_RAM + 1
            lda         #IRQ_INDEX(LINE_NONE)               ; V back on BRK's entry
            sta         V_REGISTER
            jsr         T2_OFF                              ; Timer 2: nobody's
            ldx         #LINE_VIA                           ; The tick: the kernel task's
            lda         #KERNEL_TASK
            jmp         IRQ_SET_OWNER

; The kernel task's irq entry: the VIA's timer 1, the tick (the only line it owns).  The tick count (32 bits: the
; clock will be the boot's time and the ticks since), and the tick charged to the task it interrupted (its CPU
; time); a task switch every tick (the scheduler wakes the sleepers whose time has come).  It's in every tick's
; IRQs-off time: keep it short.  IN: .X = the task interrupted.  OUT: .A = IRQ_RESCHED or 0
K_KIRQ:
            bit         VIA_IFR                             ; (V: timer 1's flag)
            bvc         @not
            lda         VIA_T1CL                            ; (Clears its flag)
            inc         K_CPU_LO,X
            bne         :+
            inc         K_CPU_MID,X
            bne         :+
            inc         K_CPU_HI,X
:
            inc         K0_TICKS
            bne         @resched
            inc         K0_TICKS + 1
            bne         @resched
            inc         K0_TICKS + 2
            bne         @resched
            inc         K0_TICKS + 3
@resched:
            lda         #IRQ_RESCHED
            rts

@not:
            lda         #0
            rts

; Timer 2 its owner's: one-shot, its flag cleared, its interrupt on (T2_ON); or nobody's: its interrupt off, its
; flag cleared (T2_OFF).  IRQs off.  Modifies .A
T2_ON:
            lda         VIA_ACR                             ; (ACR bit 5 = 0: one-shot)
            and         #<~VIA_IRQ_T2
            sta         VIA_ACR
            lda         VIA_T2CL
            lda         #VIA_IER_SET | VIA_IRQ_T2
            sta         VIA_IER
            rts

T2_OFF:
            lda         #VIA_IRQ_T2
            sta         VIA_IER
            lda         VIA_T2CL
            rts

.assert     VIA_IRQ_T2 = $20, error, "T2_ON's ACR bit 5 is VIA_IRQ_T2's"

; CA1's interrupt on (its flag cleared first: an edge from before isn't counted), or off (its flag cleared)
CA1_ON:
            lda         #VIA_IRQ_CA1
            sta         VIA_IFR
            lda         #VIA_IER_SET | VIA_IRQ_CA1
            sta         VIA_IER
            rts

CA1_OFF:
            lda         #VIA_IRQ_CA1
            sta         VIA_IER
            sta         VIA_IFR
            rts

; Line .X's owner = .A ($FF: none), in every task's copy (and timer 2 its owner's, or nobody's: T2_ON, T2_OFF; CA1's
; interrupt on or off: CA1_ON, CA1_OFF).
; In the kernel task (a KCALL, or the boot); keeps the I flag.  Modifies .Y
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
            cpx         #LINE_VIA_CA1                       ; CA1: its interrupt on for its owner, or off
            beq         @ca1
            cpx         #LINE_VIA_T2                        ; Timer 2: on for its owner, or off
            bne         @done
            php
            sei
            pha
            cmp         #$FF
            beq         :+
            jsr         T2_ON
            bra         :++
:
            jsr         T2_OFF
:
            pla
            plp
@done:
            rts

@ca1:
            php
            sei
            pha
            cmp         #$FF
            beq         :+
            jsr         CA1_ON
            bra         :++
:
            jsr         CA1_OFF
:
            pla
            plp
            rts

; IRQ_OWN: own a line.  IN: .A = the line.  OUT: C = 0; or C = 1, .A = E_RANGE, E_INVAL (no irq entry), E_BUSY
K_IRQ_OWN:
            cmp         #LINE_NONE                          ; (0-14, and LINE_VIA_T2)
            beq         @range
            cmp         #IRQ_LINES
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
            sty         K0_TMP
            tax
            lda         TA_OWNERS,X
            bmi         @take
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
            beq         @range
            cmp         #IRQ_LINES
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
            ldx         #IRQ_LINES - 1
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
