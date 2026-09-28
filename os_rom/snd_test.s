.debuginfo

; ****************************************************************************
; The sound driver's test tune (SOUND_TEST: "Sweet Child o' Mine", with a guitar patch; SND_CTL_TEST,
; HyForth sndtest) and what only it uses: the patches, YM_LOADPATCH and YM_DELAY_64.  BIOS ROM page 2,
; included inside `.scope PAGE2` (see all.s); moved here from sound.s (page 0) to make room there.  It
; runs in the sound task (its page 0 gate is in sound.s).

.segment "IO_P2"

;C# = 0
;D  = 1
;D# = 2
;E  = 4
;F  = 5
;F# = 6
;G  = 8
;G# = 9
;A  = A
;A# = C
;B  = D
;C  = E

;$20+C, $38+C
;$40+C - $78+C
;$80+C - $B8+C
;$C0+C - $F8+C

M027_Electric_Clean_Guitar:
	.byte $F8,$00
	.byte $29,$31,$21,$31,$2E,$1E,$0F,$00
	.byte $1F,$1F,$1F,$1F,$10,$04,$08,$09
	.byte $00,$00,$00,$00,$F9,$B1,$F4,$FB

M028_Electric_Muted_Guitar:
	.byte $E2,$00
	.byte $54,$51,$01,$01,$20,$21,$32,$00
	.byte $1C,$1F,$1F,$1F,$15,$02,$03,$0A
	.byte $00,$00,$00,$00,$FF,$B1,$F4,$F9

M029_Electric_Overdriven_Guitar:
	.byte $FA,$00
	.byte $33,$11,$32,$33,$10,$16,$21,$00
	.byte $1F,$1F,$1F,$1F,$17,$08,$02,$03
	.byte $00,$00,$00,$00,$FF,$B1,$F4,$FB

M030_Electric_Distorted_Guitar:
	.byte $FA,$00
	.byte $33,$11,$31,$33,$09,$0B,$1A,$00
	.byte $1F,$1F,$1F,$1F,$17,$00,$00,$07
	.byte $00,$00,$00,$00,$FF,$B1,$F4,$FB

GUITAR_PATCH = M029_Electric_Overdriven_Guitar

; Note data for Sweet Child o' Mine (first 4 bars)
SCOM_NOTES_0:   ;            D4,  D4,  E4,  E4,  G4,  G4,  D4,  D4
                ;.byte       $41, $41, $44, $44, $48, $48, $41, $41
                .byte       $40, $40, $42, $42, $46, $46, $40, $40 ; half-note flat, just like Slash

SCOM_NOTES_1:
                ;            D5,  A4,  G4,  G5,  A4, F#5,  A4
                ;.byte       $51, $4A, $48, $58, $4A, $56, $4A
                .byte       $50, $49, $46, $56, $49, $55, $49   ; half-note flat

SOUND_TEST:
                ; Set basic FM patch (simple sine-like sound)
                ldx         #$20
                lda         #$C7
                jsr         YM_WRITE

                ldx         #$80
                lda         #$1F
                jsr         YM_WRITE

                ldx         #$E0
                lda         #$0F
                jsr         YM_WRITE

                ldx         #$21
                lda         #$C7
                jsr         YM_WRITE

                ldx         #$81
                lda         #$1F
                jsr         YM_WRITE

                ldx         #$E1
                lda         #$0F
                jsr         YM_WRITE

                lda         #$00    ; Channel 0
                ldx         #<GUITAR_PATCH
                ldy         #>GUITAR_PATCH
                jsr         YM_LOADPATCH

                lda         #$01    ; Channel 1
                ldx         #<GUITAR_PATCH
                ldy         #>GUITAR_PATCH
                jsr         YM_LOADPATCH

                ldx         #$60    ; Total Level (volume) for operator 1, Channel 0
                lda         #$00    ; Max Volume
                jsr         YM_WRITE

                ldx         #$61    ; Total Level (volume) for operator 1, Channel 1
                lda         #$00    ; Max Volume
                jsr         YM_WRITE

                lda         #<SCOM_NOTES_0
                sta         YMN0L
                lda         #>SCOM_NOTES_0
                sta         YMN0H

                lda         #<SCOM_NOTES_1
                sta         YMN1L
                lda         #>SCOM_NOTES_1
                sta         YMN1H

                ldy         #0          ; counter for channel 0
@play_loop:
                ; 0. set outer counter to 0
                ; 1. key off channel 0
                ; 1a.  key off on channel 1
                ; 2. set note to next note in channel 0 list
                ; 3. key on on channel 0
                ; 4. one delay
                ; 5. notes for rest of sequence on channel 1
                ; 5a.  reset counter to 0
                ; 5b.  key off on channel 1
                ; 5c.  set note on channel 1
                ; 5d.  key on on channel 1
                ; 5e.  one delay
                ; 5f.  increment counter
                ; 5g.  loop until 7 notes have been played on channel 1
                ; 6.  increment outer counter to play next start note on channel 0
                ; 7.  loop back to 1 until outer count reaches 8 (8 sequences have played)
                ; Key Off (silence previous note)

                ldx         #$08    ; Key On/Off register
                lda         #$00    ; CH0 off
                jsr         YM_WRITE

                ldx         #$08    ; Key On/Off register
                lda         #$01    ; CH1 off
                jsr         YM_WRITE

                ; Set frequency (KC)
                ldx         #$28    ; KC register for CH0
                lda         (YMN0L),y
                jsr         YM_WRITE

                ; Key On
                ldx         #$08    ; Key On/Off register
                lda         #$78    ; CH0 on
                jsr         YM_WRITE

                ; Delay 8 64th notes in length
                lda         #8
                jsr         YM_DELAY_64

                phy
                ldy         #0
                bra         @skip_ch1_off   ; already off at the start of the loop

@next_ch1:
                ldx         #$08    ; Key On/Off register
                lda         #$01    ; CH1 off
                jsr         YM_WRITE

@skip_ch1_off:
                ; Set frequency (KC)
                ldx         #$29    ; KC register for CH1
                lda         (YMN1L),y
                jsr         YM_WRITE

                ; Key On
                ldx         #$08    ; Key On/Off register
                lda         #$79    ; CH1 on
                jsr         YM_WRITE

                ; Delay 8 64th notes in length
                lda         #8
                jsr         YM_DELAY_64

                iny
                cpy         #7
                bne         @next_ch1

                ; Next note on CH0
                ply                 ; pull CH0 count
                iny
                cpy         #8      ; 8 bars
                bne         @play_loop

                ; Stop sound
                ldx         #$08
                lda         #$00
                jsr         YM_WRITE

                ldx         #$08
                lda         #$01
                jmp         YM_WRITE

YM_LOADPATCH:
                ; Make re-entrant safe by protecting tmp and pointer variables from interrupt
                php
                sei

                ; and #$07 ; mask channel to range 0..7
                stx AZP0L
                sty AZP0H
                clc
                adc #$20 ; first byte of patch goes to YM:$20+channel
                tax

                lda (AZP0L)
                and #$3F

                ;sta ymtmp1
                ;lda ymshadow,x
                ;and #$C0 ; L+R bits for YM channel
                ;ora ymtmp1 ; Add the patch byte without L+R
                ora #$C0;

                jsr YM_WRITE
                bcs @fail
                ldy #0
                txa         ; YM_WRITE preserves X (YM register)
                ; Now skip over $28 and $30 by adding $10 to the register address.
                ; C guaranteed clear by successful ym_write
                adc #$10
                tax         ; set up for loop
@next:
                txa
                ; C guaranteed clear by successful YM_WRITE
                adc #$08
                bcs @success
                iny
                tax
                lda (AZP0L),y
                phy         ; YM_WRITE clobbers .Y
                jsr YM_WRITE
                ply
                bcc @next
@fail:
                plp         ; restore interrupt flag
                sec
                rts         ; return C set as failed patch write.
@success:
                plp         ; restore interrupt flag
                clc
                rts

; YM_DELAY_64 - Delay subroutine for musical note lengths
; A: number of 64th-note delays to wait (a 64th note @120 BPM = ~ 111,861 clock cycles @ 3.57955 MHz)
; The loop below is tuned for 3.58 MHz; at a faster CPU clock it runs CPU_CLOCK_MULT times.
YM_DELAY_64:
.if ::CPU_CLOCK_MULT > 1
                pha
                jsr         YM_DELAY_64_1
                pla
.endif

YM_DELAY_64_1:
                PUSH_XY
                sta         YMTMP1

@far_outer:
                lda         #1          ; 2 cycles
                ldx         #75
                ldy         #142
                bra         @inner

@outer:
                ldx         #0          ; 2 cycles

@mid:
                ldy         #0          ; 2 cycles

@inner:
                dey                     ; 2 cycles
                bne         @inner      ; 3 cycles when branching, 2 when not
                dex                     ; 2 cycles
                bne         @mid        ; 3 cycles when branching, 2 when not

                ; Fine-tuning at end of inner loop
                dec                     ; 2 cycles
                bne         @outer      ; 3 cycles when branching, 2 when not
                dec         YMTMP1      ; 3 cycles
                bne         @far_outer  ; 3 cycles when branching, 2 when not
                PULL_YX
