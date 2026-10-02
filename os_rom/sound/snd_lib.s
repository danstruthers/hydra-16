.debuginfo

; ****************************************************************************
; The YM2151's library (BIOS ROM page B, included inside `.scope PAGEB`, see all.s): what /dev/snd's writes go
; through (snd_srv.s).  It runs in the sound task, whose RAM keeps the chip's registers as they were written
; (SND_SHADOW: the chip's can't be read) and each channel's settings (io.inc).
;   Registers: SND_SET writes one, keeping it in the shadow.  A carrier's total level (TL, $60-$7F) goes to the
; chip with the channel's attenuation added (its volume's and the master volume's, SND_ATTEN), so a volume works
; on anything written, a song's register stream too; the shadow keeps the level as written.  Which operators are
; carriers depends on the channel's algorithm ($20-$27: CON), so a new algorithm writes the levels again.
;   Commands (SND_PAIR): the register numbers the chip doesn't have (SND_R_*), each for the channel SND_R_CH
; chose: a patch, a note (MIDI numbers, with a bend), key off, volume, speakers, a drum.
;   Claims: a channel claimed by an fid (SND_OWNER) takes writes and commands from that fid only.
;   Pitch: the YM2151's key code at its 3.58 MHz clock: octave (3 bits) and note (4 bits: C# D D# _ E F F# _ G G#
; A _ A# B C _), where MIDI note 61 (C#4) is $40 and 69 (A4, 440 Hz) is $4A; the key fraction ($30-$37, bits
; 7-2) is 1/64 of a semitone.
; ZP (the sound task's): SND_FID, SND_CH, SND_P, SND_T.

.segment "SOUND_PB"

; Reset the library: the chip cleared (SOUND_INIT), the shadow too, and each channel on both speakers, at full
; volume, with no bend and no note.  The claims stay.  OUT: C = 0; or C = 1: the chip didn't answer
SND_RESET:
            jsr         SOUND_INIT
            bcs         @done
            ldx         #0
            txa
@shadow:
            sta         SND_SHADOW,X
            inx
            bne         @shadow
            ldy         #7

@channel:
            lda         #$7F
            sta         SND_VOL,Y
            lda         #0
            sta         SND_BEND,Y
            lda         #$FF
            sta         SND_NOTE,Y
            jsr         SND_ATTEN_SET
            tya
            ora         #$20                                ; Both speakers (RL), algorithm 0
            tax
            lda         #$C0
            jsr         SND_SET
            bcs         @done
            dey
            bpl         @channel
            stz         SND_R14                             ; (SOUND_INIT stopped the timers: the sound clock
            lda         SND_CLK_OWNER                       ;   runs on, if it was)
            beq         @reset
            jmp         SND_CLOCK_GO

@reset:
            clc

@done:
            rts

; A channel's attenuation: its volume's and the master volume's (SND_VOLUME_ATTEN), 127 at most.  IN: .Y = the
; channel.  Preserves .X, .Y
SND_ATTEN_SET:
            phx
            ldx         SND_VOL,Y
            lda         SND_VOLUME_ATTEN,X
            ldx         SND_MASTER
            clc
            adc         SND_VOLUME_ATTEN,X
            bpl         :+                                  ; (Each is 127 at most: the sum fits)
            lda         #$7F
:
            sta         SND_ATTEN,Y
            plx
            rts

; Write register .X = .A through the library (the shadow; a carrier's level with the attenuation; a new
; algorithm: the levels again).  OUT: C = 0; or C = 1: the chip didn't take it.  Preserves .X, .Y
SND_SET:
            cpx         #$20
            bcc         @plain
            cpx         #$28
            bcs         @level
            pha                                             ; $20-$27: the algorithm changed?
            eor         SND_SHADOW,X
            and         #$07
            sta         SND_T
            pla
            sta         SND_SHADOW,X
            jsr         YM_WRITE
            bcs         @done
            lda         SND_T
            beq         @done                               ; (C = 0)
            txa
            and         #$07
            jmp         SND_RECOOK

@level:
            sta         SND_SHADOW,X
            cpx         #$60
            bcc         @write
            cpx         #$80
            bcs         @write
            jsr         SND_COOK
            bra         @write

@plain:
            sta         SND_SHADOW,X

@write:
            jmp         YM_WRITE

@done:
            rts

; The level for the chip: a carrier's TL (register .X, $60-$7F; .A = as written) with its channel's
; attenuation, 127 at most; another operator's as it is.  OUT: .A.  Preserves .X, .Y
SND_COOK:
            phy
            sta         SND_T
            txa
            and         #$07
            tay                                             ; .Y = the channel
            lda         SND_SHADOW + $20,Y
            and         #$07
            sta         SND_T + 1                           ; Its algorithm
            txa
            and         #$18                                ; The operator (0, 8, $10, $18: M1, M2, C1, C2) ...
            ora         SND_T + 1
            phx
            tax
            lda         SND_CARRIER,X                       ;   and the algorithm: a carrier?
            plx
            cmp         #0
            beq         @as_is
            lda         SND_T
            clc
            adc         SND_ATTEN,Y
            bpl         @done                               ; (Each is 127 at most: the sum fits)
            lda         #$7F
            bra         @done

@as_is:
            lda         SND_T

@done:
            ply
            rts

; Write a channel's four levels again (a new algorithm or volume).  IN: .A = the channel.  OUT: C = 0; or
; C = 1: the chip didn't take one.  Preserves .X, .Y
SND_RECOOK:
            phx
            ora         #$60
            tax

@op:
            lda         SND_SHADOW,X
            jsr         SND_COOK
            jsr         YM_WRITE
            bcs         @done
            txa
            adc         #$08                                ; (C = 0)
            tax
            cpx         #$80
            bcc         @op
            clc

@done:
            plx
            rts

; Which operators are carriers, by operator (M1, M2, C1, C2: 8 each) and algorithm (0-7)
SND_CARRIER:
            .byte       0, 0, 0, 0, 0, 0, 0, 1                  ; M1: algorithm 7
            .byte       0, 0, 0, 0, 0, 1, 1, 1                  ; M2: 5-7
            .byte       0, 0, 0, 0, 1, 1, 1, 1                  ; C1: 4-7
            .byte       1, 1, 1, 1, 1, 1, 1, 1                  ; C2: all

; A channel's pitch: its note and bend as its key code and fraction ($28 and $30), if a note's set.  IN: .Y =
; the channel.  OUT: C = 0; or C = 1: the chip didn't take it.  Preserves .Y
SND_PITCH:
            lda         SND_NOTE,Y
            bpl         :+
            clc                                             ; ($FF: no note)
            rts
:
            sta         SND_T + 1                           ; SND_T = the note * 64 ...
            stz         SND_T
            lsr         SND_T + 1
            ror         SND_T
            lsr         SND_T + 1
            ror         SND_T
            ldx         #0                                  ;   + the bend (signed)
            lda         SND_BEND,Y
            bpl         :+
            dex
:
            clc
            adc         SND_T
            sta         SND_T
            txa
            adc         SND_T + 1
            bmi         @lowest                             ; (Below note 0)
            sta         SND_T + 1
            lda         SND_T                               ; The note: SND_T / 64
            asl
            rol         SND_T + 1
            asl
            rol         SND_T + 1                           ; (SND_T + 1 = the note, 0-129)
            lda         SND_T
            and         #$3F
            asl
            asl
            pha                                             ; The key fraction, for $30
            lda         SND_T + 1
            sec
            sbc         #13                                 ; C#0 (MIDI 13) is key code 0
            bcc         @low
            cmp         #8 * 12
            bcs         @high
            ldx         #0                                  ; .X = the octave, .A = the note in it

@octave:
            cmp         #12
            bcc         @code
            sbc         #12
            inx
            bra         @octave

@lowest:
            lda         #0
            pha
@low:
            pla                                             ; Below C#0: C#0
            lda         #0
            pha
            tax
            bra         @code

@high:
            pla                                             ; Above C8: C8, as high as it goes
            lda         #$FC
            pha
            ldx         #7
            lda         #11

@code:
            phy
            tay
            txa
            asl
            asl
            asl
            asl
            ora         SND_NOTE_CODE,Y
            ply
            pha
            tya
            ora         #$28
            tax
            pla
            jsr         SND_SET                             ; The key code ...
            pla
            bcs         @done
            inx
            inx
            inx
            inx
            inx
            inx
            inx
            inx                                             ; ($28 + 8: $30)
            jmp         SND_SET                             ;   and the fraction

@done:
            rts

; The YM2151's note codes for C#, D, D#, E, F, F#, G, G#, A, A#, B, C
SND_NOTE_CODE:
            .byte       0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14

; Load a patch into a channel (its speakers stay the channel's).  IN: .A = the patch (below SND_PATCH_COUNT),
; SND_CH = the channel.  OUT: C = 0; or C = 1: the chip didn't take it
SND_LOAD_PATCH:
            sta         SND_T                               ; SND_P = SND_PATCHES + the patch * 26
            stz         SND_T + 1
            asl         SND_T
            rol         SND_T + 1                           ; (* 2)
            lda         SND_T
            sta         SND_P
            lda         SND_T + 1
            sta         SND_P + 1
            asl         SND_T
            rol         SND_T + 1
            asl         SND_T
            rol         SND_T + 1                           ; (* 8)
            jsr         @add
            asl         SND_T
            rol         SND_T + 1                           ; (* 16)
            jsr         @add
            lda         #<SND_PATCHES
            sta         SND_T
            lda         #>SND_PATCHES
            sta         SND_T + 1
            jsr         @add
            lda         SND_CH                              ; $20: RL (the channel's), FB, CON
            ora         #$20
            tax
            lda         SND_SHADOW,X
            and         #$C0
            sta         SND_T
            lda         (SND_P)
            and         #$3F
            ora         SND_T
            jsr         SND_SET
            bcs         @done
            txa                                             ; $38: PMS, AMS
            adc         #$18                                ; (C = 0)
            tax
            ldy         #1
            lda         (SND_P),Y
            jsr         SND_SET
            bcs         @done

@next:                                                      ; $40-$F8: the operators
            iny
            txa
            clc
            adc         #$08
            bcs         @loaded
            tax
            lda         (SND_P),Y
            jsr         SND_SET
            bcc         @next
            rts

@loaded:
            clc

@done:
            rts

@add:                                                       ; SND_P += SND_T
            clc
            lda         SND_P
            adc         SND_T
            sta         SND_P
            lda         SND_P + 1
            adc         SND_T + 1
            sta         SND_P + 1
            rts

; Key the channel off, then on (all four operators).  IN: SND_CH.  OUT: C = 0; or C = 1: the chip didn't take it
SND_KEY_ON:
            ldx         #$08
            lda         SND_CH
            jsr         SND_SET
            bcs         @done
            ora         #$78
            jmp         SND_SET

@done:
            rts

; A register/value pair from a client (fid SND_FID): a command (SND_R_*), a register written through the
; library, or nothing: a channel another fid has claimed, or a register the chip doesn't have.  IN: .X = the
; register, .A = the value.  OUT: C = 0; or C = 1: the chip didn't take it.  Modifies .A, .X, .Y
SND_PAIR:
            cpx         #$20
            bcs         @channel_reg
            cpx         #$14                                ; The timers' control: the driver's part of it kept
            bne         :+
            jmp         SND_R14_SET
:
            cpx         #$12                                ; Timer B's period: the sound clock's while it runs
            bne         :+
            ldy         SND_CLK_OWNER
            bne         @dropped
:
            ldy         SND_LOW_REG,X                       ; What register .X is
            beq         @keep                               ; (None the chip has: only kept)
            cpy         #SND_LR_CHIP
            beq         @write                              ; One for the whole chip
            cpy         #SND_LR_KEY
            beq         @key
            cpy         #SND_LR_SELECT
            beq         @select
            sta         SND_SHADOW,X                        ; A command, for the channel SND_R_CH chose
            pha
            lda         SND_SHADOW + SND_R_CH
            sta         SND_CH
            jsr         SND_OWNED
            bcs         @not_mine
            lda         SND_CMD_LO - SND_LR_CMD,Y
            sta         SND_P
            lda         SND_CMD_HI - SND_LR_CMD,Y
            sta         SND_P + 1
            pla
            ldy         SND_CH
            jmp         (SND_P)                             ; (.A = the value, .Y = the channel)

@not_mine:
            pla
            clc
            rts

@channel_reg:                                               ; $20-$FF: the channel is the register's
            pha
            phx
            txa
            and         #$07
            jsr         SND_OWNED
            plx
            pla
            bcc         @write
            clc
            rts

@key:                                                       ; $08 (key on and off): the channel is the value's
            pha
            phx
            and         #$07
            jsr         SND_OWNED
            plx
            pla
            bcs         @dropped

@write:
            jmp         SND_SET

@select:
            and         #$07

@keep:
            sta         SND_SHADOW,X

@dropped:
            clc
            rts

; A client's write to $14, the timers' control (a song's register dump from another machine may have their
; interrupts on, and nothing would clear them: they stay off; CSM and timer A's load are the client's, kept in
; SND_R14 for the clock's interrupt).  While the sound clock runs, timer B's bits are its own.  IN: .A = the value.
; OUT: C = 0; or C = 1: the chip didn't take it
SND_R14_SET:
            pha
            and         #$81
            sta         SND_R14
            pla
            ldx         SND_CLK_OWNER
            beq         @no_clock
            and         #$91                                ; (CSM, timer A's flag reset and load)
            ora         #$0A                                ; (Timer B: on, its interrupt on)
            bra         @write

@no_clock:
            and         #$F3                                ; (Everything but the interrupts)

@write:
            ldx         #$14
            jmp         SND_SET

; The sound clock (SND_CTL_CLOCK): timer B, a period of K units (1024 of the chip's clocks) or K + 1, as the
; fraction at SND_R_CLOCK_F says; its interrupt (ymfast.s, page 2) counts SND_CLK from 0.  The fid's (SND_FID)
; till it stops it or its last close.  IN: .A = K (1-255; 0: stop).  OUT: C = 0; or C = 1, .A = ERR_IO_BUSY
; (another fid's), or the chip didn't take it.  Modifies .A, .X, .Y
SND_CLOCK_SET:
            ldx         SND_CLK_OWNER
            beq         @ours
            cpx         SND_FID
            beq         @ours
            lda         #ERR_IO_BUSY
            sec
            rts

@ours:
            cmp         #0
            beq         SND_CLOCK_STOP
            sta         SND_T                               ; K and its fraction (SND_P)
            lda         SND_SHADOW + SND_R_CLOCK_F
            sta         SND_P
            lda         SND_SHADOW + SND_R_CLOCK_F + 1
            sta         SND_P + 1
            jsr         TICKS_GET                           ; (The system's tick now: for /dev/snd's numbers)
            sta         SND_CLK_T0
            sty         SND_CLK_T0 + 1
            stz         SND_IRQS
            stz         SND_IRQS + 1
            stz         SND_WAKES
            stz         SND_LAST_AT
            stz         SND_LAST_AT + 1
            lda         SND_T
            eor         #$FF                                ; A long period: K + 1 units ($12 = 256 - K - 1)
            sta         SND_CLK_NB1
            inc
            sta         SND_CLK_LAST                        ; (The first: a short one)
            lda         SND_R14                             ; Timer B stopped, while it's set up
            ora         #$20
            ldx         #$14
            jsr         YM_WRITE
            bcs         @done
            php
            sei                                             ; (Its interrupt can't come in halfway)
            lda         SND_FID
            sta         SND_CLK_OWNER
            lda         SND_P                               ; (Its fraction)
            sta         SND_CLK_F
            lda         SND_P + 1
            sta         SND_CLK_F + 1
            stz         SND_CLK_ACC
            stz         SND_CLK_ACC + 1
            stz         SND_CLK
            stz         SND_CLK + 1
            lda         #$FF
            sta         SND_CLK_WAIT
            plp
            jmp         SND_CLOCK_GO

@done:
            rts

; Timer B started with the clock's period (SND_CLK_LAST), its interrupt on.  OUT: C = 0; or C = 1: the chip
; didn't take it
SND_CLOCK_GO:
            lda         SND_CLK_LAST
            ldx         #$12
            jsr         YM_WRITE
            bcs         @done
            lda         SND_R14
            ora         #$2A                                ; (Its flag reset; its interrupt on; loaded: running)
            ldx         #$14
            jmp         YM_WRITE

@done:
            rts

; The sound clock stopped (timer B off, its flag reset); the waiting player (if any) isn't any more.  OUT: C = 0;
; or C = 1: the chip didn't take it.  Modifies .A, .X
SND_CLOCK_STOP:
            stz         SND_CLK_OWNER
            lda         #$FF
            sta         SND_CLK_WAIT
            lda         SND_R14
            ora         #$20
            ldx         #$14
            jmp         YM_WRITE

; Is channel .A another fid's (not SND_FID's)?  OUT: C = 1 yes.  Modifies .A, .X
SND_OWNED:
            tax
            lda         SND_OWNER,X
            beq         @free
            cmp         SND_FID
            beq         @free
            sec
            rts

@free:
            clc
            rts

; What each register below $20 is, for SND_PAIR: none the chip has (0), one for the whole chip, key on and off, the
; channel for the commands, or a command (SND_LR_CMD + its number in SND_CMD_LO)
SND_LR_CHIP     = 1
SND_LR_KEY      = 2
SND_LR_SELECT   = 3
SND_LR_CMD      = 4
SND_LOW_REG:
            .byte       0, SND_LR_CHIP, SND_LR_SELECT, SND_LR_CMD, SND_LR_CMD + 1, SND_LR_CMD + 2, SND_LR_CMD + 3
            .byte       SND_LR_CMD + 4                      ; $00-$07: -, test and LFO reset, SND_R_CH, SND_R_PATCH,
                                                            ;   SND_R_NOTE, SND_R_OFF, SND_R_VOL, SND_R_PAN
            .byte       SND_LR_KEY, SND_LR_CMD + 5, SND_LR_CMD + 6, 0, 0, 0, 0, SND_LR_CHIP
                                                            ; $08-$0F: key on, SND_R_BEND, SND_R_DRUM, -, noise
            .byte       SND_LR_CHIP, SND_LR_CHIP, SND_LR_CHIP, 0, SND_LR_CHIP, 0, 0, 0
                                                            ; $10-$17: timer A, timer B, -, the timers' control
            .byte       SND_LR_CHIP, SND_LR_CHIP, 0, SND_LR_CHIP, 0, 0, 0, 0
                                                            ; $18-$1F: LFO rate, depths, -, CT and the waveform
.assert     SND_R_CH = 2 && SND_R_PATCH = 3 && SND_R_NOTE = 4 && SND_R_OFF = 5 && SND_R_VOL = 6 && SND_R_PAN = 7 && SND_R_BEND = 9 && SND_R_DRUM = $0A, error, "SND_LOW_REG: the commands' registers"

SND_CMD_LO:
            .lobytes    SND_CMD_PATCH, SND_CMD_NOTE, SND_CMD_OFF, SND_CMD_VOL, SND_CMD_PAN, SND_CMD_BEND, SND_CMD_DRUM
SND_CMD_HI:
            .hibytes    SND_CMD_PATCH, SND_CMD_NOTE, SND_CMD_OFF, SND_CMD_VOL, SND_CMD_PAN, SND_CMD_BEND, SND_CMD_DRUM

; The commands.  IN: .A = the value, .Y = SND_CH = the channel.  OUT: C = 0; or C = 1: the chip didn't take it

SND_CMD_PATCH:
            cmp         #SND_PATCH_COUNT
            bcs         SND_CMD_NONE
            jmp         SND_LOAD_PATCH

SND_CMD_NONE:                                               ; (Out of range: nothing)
            clc
            rts

SND_CMD_NOTE:
            and         #$7F
            sta         SND_NOTE,Y
            jsr         SND_PITCH
            bcs         SND_CMD_DONE
            jmp         SND_KEY_ON

SND_CMD_OFF:
            lda         #$FF
            sta         SND_NOTE,Y
            ldx         #$08
            tya
            jmp         SND_SET

SND_CMD_VOL:
            and         #$7F
            sta         SND_VOL,Y
            jsr         SND_ATTEN_SET
            tya
            jmp         SND_RECOOK

SND_CMD_PAN:
            and         #$03                                ; SND_PAN_LEFT: L (bit 6); SND_PAN_RIGHT: R (bit 7)
            asl
            asl
            asl
            asl
            asl
            asl
            sta         SND_T
            tya
            ora         #$20
            tax
            lda         SND_SHADOW,X
            and         #$3F
            ora         SND_T
            jmp         SND_SET

SND_CMD_BEND:
            sta         SND_BEND,Y
            jmp         SND_PITCH

SND_CMD_DRUM:
            cmp         #SND_DRUM_COUNT
            bcs         SND_CMD_NONE
            tax
            lda         SND_DRUM_KC,X                       ; Its pitch (after the patch)
            pha
            lda         SND_DRUM_PATCH,X
            jsr         SND_LOAD_PATCH
            bcs         @failed
            ldy         SND_CH
            lda         #$FF                                ; (No note: a bend leaves it alone)
            sta         SND_NOTE,Y
            tya
            ora         #$28
            tax
            pla
            jsr         SND_SET                             ; The key code ...
            bcs         SND_CMD_DONE
            txa
            adc         #$08                                ; (C = 0)
            tax
            lda         #0
            jsr         SND_SET                             ;   no fraction
            bcs         SND_CMD_DONE
            jmp         SND_KEY_ON

@failed:
            pla

SND_CMD_DONE:
            rts
