.debuginfo

; ****************************************************************************
; VIA (the scheduler's tick is its timer 1; see SCHED_START; SPI uses port B, spi.s)

.segment "BIOS"

; Must be called from the system task (the VIA handler runs there)
VIA_INIT:
            PUSH_AXY
            stz     ZP_TICKS                ; (The system task's: the tick count)
            stz     ZP_TICKS + 1
            ldx     #IRQ_NUMBER_ONBOARD_VIA
            lda     #<VIA_IRQ_HANDLER
            ldy     #>VIA_IRQ_HANDLER
            jsr     IRQ_REGISTER
            PULL_YXA
            rts

; VIA IRQ handler (registered by VIA_INIT; runs in the system task).
; OUT: C = 1 if T1 was interrupting
VIA_IRQ_HANDLER:
; check which sub-device is triggering the IRQ
            lda     #VIA_T1_INT_BIT
            and     VIA_R_INT_FLAGS
            beq     :+
            lda     VIA_R_T1C_L             ; clear the interrupt
            inc     ZP_TICKS                ; Count it (TICKS_GET)
            bne     @counted
            inc     ZP_TICKS + 1

@counted:
            jsr     SLEEP_CHECK             ; Wake the sleepers whose time has come (TASK_SLEEP)
            lda     #SCHED_RESCHED_A        ; T1 is the scheduler's tick: ask the dispatcher for a task switch
            ldy     #SCHED_RESCHED_Y
            sec
            rts

:
            clc
            rts

; The tick count: SCHED_TICK_HZ (200) a second, the scheduler's tick (VIA timer 1), counted in the system
; task's ZP_TICKS; it wraps round after about 5.5 minutes.  From any task.  OUT: .A.Y = the count
; Preserves .X
TICKS_GET:
            php
            sei
            phx
            ldx     T_REGISTER
            stz     T_REGISTER              ; Quick look at the system task (no stack use!)
            lda     ZP_TICKS
            ldy     ZP_TICKS + 1
            stx     T_REGISTER
            plx
            plp
            rts
