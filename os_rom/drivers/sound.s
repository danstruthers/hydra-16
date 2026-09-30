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

; The serve routine (page 2, snd_srv.s), and the chip's set-up (page 2, ym.s)
FAR_GATE_INLINE     SND_SERVE,      PAGE2::SND_SERVE,       2
FAR_GATE_INLINE     SOUND_INIT,     PAGE2::SOUND_INIT,      2

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

SOUND_STOP:
                clc
                rts

; OUT: C = 1 if the interrupt was claimed
SOUND_IRQ_HANDLER:
                ; check which
                clc
                rts
