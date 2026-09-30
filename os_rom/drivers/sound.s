.debuginfo

.segment "SOUND"

; ****************************************************************************
; Sound driver.  Runs in its own Resident task (SOUND_TASK_NUM, started by DRV_START at boot), so its
; ZP is task ZP.  Other tasks use it through its file, /dev/snd (snd_srv.s, on ROM page 2): writes are
; register/value pairs, and IO_CTL clears the chip, and starts and stops the test tune, which plays in
; a player task in the background (snd_test.s).  (The console bell, YM_BEEP, writes the chip directly: page
; 2, beep.s.)

SOUND_DRIVER:
                .word       SOUND_DRV_INIT              ; DriverInfo::init
                .word       SOUND_STOP                  ; DriverInfo::stop
                .word       SOUND_NAME                  ; DriverInfo::name
NamedHString SOUND_NAME, "SOUND"
SND_NAME:       .byte       "snd", 0

; The serve routine (page 2, snd_srv.s)
FAR_GATE_INLINE     SND_SERVE,      PAGE2::SND_SERVE,       2

; Driver init (runs in the sound task): the chip, then the file.  OUT: C = 0, or C = 1 and .A = error
SOUND_DRV_INIT:
                lda         #$FF
                sta         SND_PLAYER                  ; No player (snd_srv.s)
                jsr         SOUND_INIT
                LOAD_ADDR   SND_SERVE, ZP_TC_VEC
                lda         #<SND_NAME
                ldy         #>SND_NAME
                ldx         #SOUND_TASK_NUM
                jmp         DEV_REGISTER

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
                lda         #<SOUND_IRQ_HANDLER
                ldy         #>SOUND_IRQ_HANDLER
                jsr         IRQ_REGISTER            ; Handler runs in this (the sound) task

@error:
                plx
                pla
                rts

SOUND_STOP:
                clc
                rts

; OUT: C = 1 if the interrupt was claimed
SOUND_IRQ_HANDLER:
                ; check which
                clc
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
