; ****************************************************************************
; Sound driver.  Runs in its own Resident task (SOUND_TASK_NUM, started by DRV_START at boot), so its
; ZP is task ZP.  Other tasks call it through the SND_CALL_* gates, which run the routine in the
; sound task with TASK_CALL, or through its file, /dev/snd (snd_srv.s, on ROM page 2).

SOUND_DRIVER:
                .word       SOUND_DRV_INIT              ; DriverInfo::init
                .word       SOUND_STOP                  ; DriverInfo::stop
                .word       SOUND_NAME                  ; DriverInfo::name
NamedHString SOUND_NAME, "SOUND"
SND_NAME:       .byte       "snd", 0

; Gates into the sound task: .A/.X/.Y/C pass through to the routine and back
TASK_GATE           SND_CALL_INIT, SOUND_INIT, SOUND_TASK_NUM
TASK_GATE           SND_CALL_TEST, SOUND_TEST, SOUND_TASK_NUM
TASK_GATE           SND_CALL_YM_WRITE, YM_WRITE, SOUND_TASK_NUM

; The serve routine (page 2, snd_srv.s), and the test tune (page 2, snd_test.s; its ZP is in zero.s)
FAR_GATE_INLINE     SND_SERVE,      PAGE2::SND_SERVE,       2
FAR_GATE_INLINE     SOUND_TEST,     PAGE2::SOUND_TEST,      2

; Driver init (runs in the sound task): the chip, then the file.  OUT: C = 0, or C = 1 and .A = error
SOUND_DRV_INIT:
                jsr         SOUND_INIT
                LOAD_ADDR   SND_SERVE, ZP_TC_VEC
                lda         #<SND_NAME
                ldy         #>SND_NAME
                ldx         #SOUND_TASK_NUM
                jmp         DEV_REGISTER

; zero out all YM-2151 registers $28-$FF
SOUND_INIT:
                pha
                phx
                lda         #0
                ldx         #$14        ; turn off the clocks
                jsr         YM_WRITE
                ldx         #$28

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

; Write value in A to YM-2151 register in X
YM_WRITE:
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
                clc
                bra         @cleanup

@timeout:
                sec

@cleanup:
                ply
                cli
                rts

