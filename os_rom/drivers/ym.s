.debuginfo

; ****************************************************************************
; The YM2151's set-up and register writes (BIOS ROM page 2, included inside `.scope PAGE2`, see all.s): the
; sound driver's (sound.s, on page 0, reaches SOUND_INIT through a gate), its server's and test tune's, and
; the bell's (beep.s).

.segment "IO_P2"

; Set up the YM-2151, whatever it powered up with (its reset, /IC, may not clear everything): the timers
; stopped, their IRQs off and their flags reset, then every register $01-$FF zeroed (the test/LFO register
; $01, noise, the timers' periods, and the voices)
SOUND_INIT:
                pha
                phx
                lda         #$30        ; timers stopped, their IRQs off, both flags reset
                ldx         #$14
                jsr         YM_WRITE
                lda         #0
                ldx         #$01

@write_z:
                jsr         YM_WRITE
                bcs         @error
                inx
                bne         @write_z
                ldx         #IRQ_NUMBER_ONBOARD_SOUND
                lda         #<::SOUND_IRQ_HANDLER
                ldy         #>::SOUND_IRQ_HANDLER
                jsr         IRQ_REGISTER            ; Handler runs in this (the sound) task, on page 0

@error:
                plx
                pla
                rts

YM_TIMEOUT = 64 * CPU_CLOCK_MULT    ; Busy-wait loop count (<= 128: the loop ends when .Y goes negative)

; Write value in A to YM-2151 register in X.  OUT: C = 0; or C = 1 (the chip stayed busy).  Keeps the
; caller's I flag (so it also works in an IRQ handler: YM_BEEP)
YM_WRITE:
                php
                sei
                phy
                ldy         #YM_TIMEOUT

@ym_wait1:
                dey
                bmi         @timeout
                bit         YM_DATA
                bmi         @ym_wait1
                stx         YM_REG
                ldy         #YM_TIMEOUT

@ym_wait2:
                dey
                bmi         @timeout
                bit         YM_DATA
                bmi         @ym_wait2
                sta         YM_DATA
                ply
                plp
                clc
                rts

@timeout:
                ply
                plp
                sec
                rts
