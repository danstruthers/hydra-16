.debuginfo

; ****************************************************************************
; The console bell (BIOS ROM page 2, included inside `.scope PAGE2`, see all.s; page 0's serial driver
; reaches it through a gate).  YM_WRITE is page 0's (the sound driver's: sound.s).

.segment "IO_P2"

; The console bell on the YM2151: a short beep on channel 7, a sine (four operators in step) that fades by
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
YM_BEEP_LEVEL   = $00                                   ; Each operator's total level (attenuation: 0 = loudest)
YM_BEEP_DECAY   = $0C                                   ; Their decay rates (D1R, D2R; 31 = fastest)

; All four operators are carriers (algorithm 7) on the same pitch, so their outputs add up: about 12 dB
; louder than one alone
.macro YM_BEEP_OP  slot                                 ; An operator (slot: 0 M1, 8 M2, $10 C1, $18 C2)
                .byte       $40 + slot + YM_BEEP_CH, $01    ; No detune, multiplier 1
                .byte       $60 + slot + YM_BEEP_CH, YM_BEEP_LEVEL
                .byte       $80 + slot + YM_BEEP_CH, $1F    ; Attack at once
                .byte       $A0 + slot + YM_BEEP_CH, YM_BEEP_DECAY
                .byte       $C0 + slot + YM_BEEP_CH, YM_BEEP_DECAY
                .byte       $E0 + slot + YM_BEEP_CH, $FF    ; Decay all the way to silence, release fast
.endmacro

YM_BEEP_REGS:
                .byte       $20 + YM_BEEP_CH, $C7       ; Both speakers, no feedback, algorithm 7 (all carriers)
                .byte       $28 + YM_BEEP_CH, YM_BEEP_NOTE
                .byte       $30 + YM_BEEP_CH, $00       ; Key fraction
                .byte       $38 + YM_BEEP_CH, $00       ; No vibrato or tremolo
                YM_BEEP_OP  $00                        ; M1
                YM_BEEP_OP  $08                        ; M2
                YM_BEEP_OP  $10                        ; C1
                YM_BEEP_OP  $18                        ; C2
                .byte       $08, YM_BEEP_CH             ; Key off (so it starts again)...
                .byte       $08, $78 | YM_BEEP_CH       ; ...and on: all four operators
                .byte       0

