.debuginfo

.segment "SOUND"

; ****************************************************************************
; Sound driver.  Runs in its own Resident task (SOUND_TASK_NUM, started by DRV_START at boot), so its
; ZP is task ZP.  Other tasks use it through its file, /dev/snd (snd_srv.s, on ROM page 2): writes are
; register/value pairs, and IO_CTL clears the chip, and starts and stops the test tune, which plays in
; a player task in the background (snd_test.s).  (The console bell, YM_BEEP, writes the chip directly.)

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

; The console bell on the YM2151: a short beep on channel 7, a sine (operator M1 alone) that fades by
; itself, so nothing has to turn it off.  The serial driver calls it as it sends a BEL (Ctrl-G) to the
; terminal (_M_SER_TX_BYTE): the terminal's bell, echoed Ctrl-G, a console command that failed.  From any
; task, and in IRQ handlers: IRQs are off while it writes.  Skipped while the sound driver is busy (a
; guest in the sound task: a tune, /dev/snd, ywrite), so it never cuts into the driver's writes.
; Preserves .A, .X, .Y and C
YM_BEEP:
                php
                sei
                PUSH_AXY
                ldy         T_REGISTER
                ldx         #SOUND_TASK_NUM
                stx         T_REGISTER                  ; Quick look (no stack use!)
                lda         ZP_TC_GUEST
                sty         T_REGISTER
                cmp         #0
                bne         @done                       ; The sound driver is busy
                ldx         #0

@reg:
                lda         YM_BEEP_REGS,X              ; Register, value, ...; 0 ends it
                beq         @done
                phx
                pha
                lda         YM_BEEP_REGS + 1,X
                plx                                     ; .X = the register
                jsr         YM_WRITE
                plx
                inx
                inx
                bra         @reg

@done:
                PULL_YXA
                plp
                rts

YM_BEEP_CH      = 7                                     ; The channel
YM_BEEP_NOTE    = $5A                                   ; Key code: octave 5, A (about 880 Hz)
YM_BEEP_LEVEL   = $10                                   ; M1's total level (attenuation: 0 = loudest)
YM_BEEP_DECAY   = $0C                                   ; M1's decay rates (D1R, D2R; 31 = fastest)

YM_BEEP_REGS:
                .byte       $20 + YM_BEEP_CH, $C7       ; Both speakers, no feedback, algorithm 7 (all carriers)
                .byte       $28 + YM_BEEP_CH, YM_BEEP_NOTE
                .byte       $30 + YM_BEEP_CH, $00       ; Key fraction
                .byte       $38 + YM_BEEP_CH, $00       ; No vibrato or tremolo
                .byte       $40 + YM_BEEP_CH, $01       ; M1: no detune, multiplier 1
                .byte       $60 + YM_BEEP_CH, YM_BEEP_LEVEL
                .byte       $80 + YM_BEEP_CH, $1F       ;   attack at once
                .byte       $A0 + YM_BEEP_CH, YM_BEEP_DECAY
                .byte       $C0 + YM_BEEP_CH, YM_BEEP_DECAY
                .byte       $E0 + YM_BEEP_CH, $FF       ;   decay all the way to silence, release fast
                .byte       $68 + YM_BEEP_CH, $7F       ; M2, C1, C2: silent
                .byte       $70 + YM_BEEP_CH, $7F
                .byte       $78 + YM_BEEP_CH, $7F
                .byte       $08, YM_BEEP_CH             ; Key off (so it starts again)...
                .byte       $08, $08 | YM_BEEP_CH       ; ...and on: M1
                .byte       0

