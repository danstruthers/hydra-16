.debuginfo

; ****************************************************************************
; VIA (the scheduler's tick is its timer 1; see SCHED_START; SPI uses port B, spi.s)

.segment "IRQ"

; Must be called from the system task (the VIA handler runs there)
VIA_INIT:
            PUSH_AXY
            stz     ZP_TICKS                ; (The system task's: the tick count)
            stz     ZP_TICKS + 1
            stz     ZP_CLOCK                ;   and the clock (VIA_IRQ_FAST counts it; this handler's
            stz     ZP_CLOCK + 1            ;   tick is only a fallback, and doesn't)
            stz     ZP_CLOCK + 2
            stz     ZP_CLOCK + 3
            jsr     VIA_CA1_INIT            ; CA1's edges (/dev/gpio/ca1): none, and none waiting
            ldx     #IRQ_NUMBER_ONBOARD_VIA
            lda     #<VIA_IRQ_HANDLER
            ldy     #>VIA_IRQ_HANDLER
            jsr     IRQ_REGISTER
            PULL_YXA
            rts

; VIA IRQ handler (registered by VIA_INIT; runs in the system task): T1 (VIA_IRQ_FAST does it, so this is a
; fallback), and CA1's active edge (/dev/gpio/ca1: counted, and the tasks waiting for it woken).
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
            jmp     VIA_CA1                 ; (After the thunks, where page 0 has room)

; CA1's (/dev/gpio/ca1): an active edge counted, and the tasks waiting for it woken.  In the system task, as
; VIA_IRQ_HANDLER is.  OUT: C = 0
.pushseg
.segment "BIOS"
VIA_CA1:
            lda     #VIA_CA1_INT_BIT
            and     VIA_R_INT_FLAGS
            beq     @none
            sta     VIA_R_INT_FLAGS         ; Clears it
            inc     GPIO_CA1N               ; Count it
            bne     :+
            inc     GPIO_CA1N + 1
:
            ldx     #0                      ; Wake the tasks waiting for it (.X = task)

@wake:
            lsr     GPIO_CA1W + 1
            ror     GPIO_CA1W
            bcc     @next
            txa
            jsr     IO_WAKE                 ; (Keeps .X)

@next:
            inx
            cpx     #MAX_TASK_NUMBER + 1
            bne     @wake

@none:
            clc
            rts

; At boot (VIA_INIT, in the system task): no edges yet, no task waiting, no fd open; and no task to run next
; (SCHED_URGENT_T: the scheduler's, here for room)
VIA_CA1_INIT:
            lda     #$FF
            sta     SCHED_URGENT_T
            stz     GPIO_CA1N
            stz     GPIO_CA1N + 1
            stz     GPIO_CA1W
            stz     GPIO_CA1W + 1
            stz     GPIO_CA1REFS
            rts
.popseg

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
