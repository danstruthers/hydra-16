; ****************************************************************************
; snd - the sound driver (docs/design/reimplementation-from-scratch.md, §14.4): the YM2151 and the old system's library for
; it (os_rom/sound: snd_lib.s, snd_srv.s, ym.s, beep.s), ported, on srvlib: the device #a (a boot driver: task C);
; and the Vera X's PSG, through the video driver (vid's #v/psg: the VERA is vid's), as channels 8-23.
;   /snd     a write: YM2151 register/value byte pairs, through the library (an odd last byte is dropped); a read:
;            the registers as they were written (the shadow, 256 bytes: the chip's can't be read)
;   /sndctl  claim N [P], release N [P] (masks of channels: N's bit n, channel n (0-7); P's, channel 8 + n), volume
;            N (the master volume, 0-200: 100 as written; more, louder, up to 23 dB), reset (the chips and every
;            setting cleared; the claims stay); and each channel's commands as text (/snd's SND_R_*): patch CH P,
;            note CH N, off CH, level CH V (vol, its old name), pan CH left|right|both (or 0-3), bend CH B (-128 to
;            127), drum CH N, freq CH HZ (0: off), glide CH N (legato), wave CH pulse|saw|triangle|noise (or 0-3)
;            [WIDTH] (the PSG's), sens CH PMS AMS, and reg R V [R V]; the YM2151's own: lfo RATE PMD AMD WAVE, noise
;            N|off.  A channel is 0-7, the YM2151's, or 8-23, the PSG's (E_NODEV with no card).  It reads as the
;            state: "volume 100", "channels 24" (8 with no card), then "claimed" and the channels claimed
;   /psg     a write: the PSG's register/value pairs (0-63: voice n's frequency, low and high, at 4n and 4n + 1, its
;            speakers (bits 7 R, 6 L) and volume (0-63) at 4n + 2, its waveform (bits 7-6) and pulse width at 4n + 3),
;            as ZSM's PSG writes are (an odd last byte, and a register past 63, dropped); a read: them as written (64
;            bytes).  With no card, taken and dropped
;   /bell    a write: the console's bell (cons writes it at a BEL it sends): a short beep on channel 7, unless
;            channel 7 is claimed
; Claims: a channel claimed is its claimer's alone, the other tasks' writes to it dropped, till it releases it or
; closes its last file of #a (keyed off then).  A file is the task's that opened it (a child writing to a file it
; was given writes as its parent).  A claim of a channel another task has: E_BUSY, and nothing claimed.
;   The library: the chip's registers kept as written (the shadow).  A carrier's total level ($60-$7F) goes to the
; chip with its channel's attenuation added (its volume's and the master volume's), so a volume works on anything
; written, a song's register stream too; the shadow keeps the level as written.  Which operators are carriers
; depends on the channel's algorithm ($20-$27: CON), so a new algorithm writes the levels again.  The register
; numbers the chip doesn't have (below $20) are commands (SND_R_*), each for the channel SND_R_CH chose: a patch
; (patches.s: the X16's), a note (MIDI numbers, with a bend), key off, volume, speakers, a drum (General MIDI's), a
; frequency, a glide, a waveform (the PSG's).
; A write to $14, the timers' control, keeps their interrupts off: the driver owns no IRQ line (the old system's
; sound clock, timer B counting a song's ticks, is gone: on the board timer B didn't keep its period, and the song
; player came to time songs by the system's tick).
;   Pitch: the YM2151's key code at its 3.58 MHz clock: octave (3 bits) and note (4 bits: C# D D# _ E F F# _ G G#
; A _ A# B C _), where MIDI note 61 (C#4) is $40 and 69 (A4, 440 Hz) is $4A; the key fraction ($30-$37, bits 7-2)
; is 1/64 of a semitone.
;   The PSG (16 voices: a frequency, a waveform, a volume, speakers; no envelope) takes the same commands on channels
; 8-23: a note sets its voice's frequency and its volume the most (63), key off its volume 0; a patch below 4 is a
; waveform (SND_WAVE_*, a square's width); a drum, nothing.  Its volumes go to the chip with the channel's
; attenuation (its level's and the master volume's, as the YM2151's carriers' levels), 1.5 of the PSG's 0.5 dB steps
; a TL step.  Its registers as written are kept here (/psg), the chip's as vid keeps them; the pairs a request makes
; go to vid in one write at its end (#v/psg, opened the first time it's wanted: with no card, again the next time).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "snd", init, srv_serve, 0, 0, HF_BOOT

            .import     patches, drum_patch, drum_kc, volume_atten

SRV_OPENED      = opened                                    ; (srvlib: a fid made: its task's, counted ...
SRV_CLUNKED     = clunked                                   ;   and forgotten: its task's last, its channels back)
SRV_STAT        = stat                                      ;   (/snd's length: 256)

; The chip's busy time waited at most (ym_write's loops, 128 at most: .Y counts down to < 0)
YM_TIMEOUT      = 64 * CPU_CLOCK_MULT
IOBUF           = 64            ; A write's bytes, a part at a time
PATCH_SIZE      = 26            ; patches.s: a patch's registers
BELL_CH         = 7             ; The bell's channel
ENT_SND         = 1             ; srv_tree's /snd ...
ENT_PSG         = 4             ;   and /psg
PSG_REGS        = 64            ; The PSG's registers (16 voices of 4)
PSG_OUT         = PSG_REGS * 2  ; psg_out: all of them, a pair each
PSG_ON          = $3F           ; A PSG note's volume (as written: its level attenuates it)
PSG_SQUARE      = $3F           ; A PSG voice's waveform after a reset: pulse, a square (width 63)

.zeropage
ch:         .res        1                                   ; The channel the library works on
ptr:        .res        2                                   ; A pointer
t:          .res        2                                   ; Scratch
owner:      .res        1                                   ; The request's task (its fid's: 1-16)
cnt:        .res        1                                   ; A write's part: its bytes ...
done:       .res        2                                   ;   and the bytes taken before it
u:          .res        3                                   ; psg_cook's scratch
kind:       .res        1                                   ; The write's file: 0 /snd, $80 /psg

.bss
shadow:     .res        256                                 ; Every register as last written (the carriers' TL
                                                            ;   before the attenuation)
vol:        .res        SND_CHANNELS                        ; Each channel's volume (0-127) ...
atten:      .res        SND_CHANNELS                        ;   its attenuation (TL steps, signed) ...
bend:       .res        SND_CHANNELS                        ;   its bend ...
note:       .res        SND_CHANNELS                        ;   its note ($FF: none) ...
fine:       .res        SND_CHANNELS                        ;   its 64ths above the note (freq's) ...
chown:      .res        SND_CHANNELS                        ;   and its claimer (a task + 1; 0: none)
mask:       .res        3                                   ; A claim's channels (bit n: channel n)
refs:       .res        16                                  ; Each task's fids on #a
master:     .res        1                                   ; The master volume, a percentage (0-200) ...
master_tl:  .res        1                                   ;   and its TL steps, signed (127: silent; 0: as
                                                            ;   written; -31: louder)
iobuf:      .res        IOBUF
f_note:     .res        1                                   ; cmd_freq: the note of the table's octave (+ 64) ...
f_semi:     .res        1                                   ;   and the semitone in it
psg:        .res        PSG_REGS                            ; The PSG's registers as written (the volumes before
                                                            ;   the attenuation)
psg_out:    .res        PSG_OUT                             ; Pairs for #v/psg (the volumes attenuated) ...
psg_n:      .res        1                                   ;   their bytes ...
psg_fd:     .res        1                                   ;   and #v/psg's fd ($FF: not open)
pw:         .res        2                                   ; psg_pitch's: a semitone's width / 4 ...
pr:         .res        2                                   ;   the frequency word ...
psh:        .res        1                                   ;   and the octaves below the table's

.code

; Its init: the chip and the library reset, the master volume 100 (songs as written), and its device letter
; registered.  (A chip that doesn't answer: its writes fail, E_IO.  #v/psg isn't there yet: vid starts later)
init:
            lda         #$FF
            sta         psg_fd
            stz         psg_n
            lda         #100
            sta         master
            stz         master_tl
            jsr         reset
            lda         #'a'
            jmp         SRV_REGISTER

; ****************************************************************************
; The files

; /snd: a read, a write.  (Its opens, dups and clunks: the hooks count them)
h_snd:
            stz         kind
            cmp         #R_READ
            bne         :+
            jmp         r_snd
:
            cmp         #R_WRITE
            beq         w_snd
            clc
            rts

; /psg: a read, a write (the PSG's registers)
h_psg:
            ldy         #$80
            sty         kind
            cmp         #R_READ
            bne         :+
            jmp         r_psg
:
            cmp         #R_WRITE
            beq         w_snd
            clc
            rts

; /snd, /psg: a write: register/value pairs, through the library, IOBUF bytes at a time (an odd last byte
; dropped); then the PSG's pairs it made, to vid.  The chip not answering: the pairs written so far count, or E_IO
; if none.  IN: .X = the fid
w_snd:
            lda         srv_fid_aux,X
            sta         owner
            stz         done
            stz         done + 1
@part:
            sec                                             ; This part: IOBUF at most, the rest if less
            lda         TASK_INBOX + RQ_COUNT
            sbc         done
            sta         t
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         done + 1
            sta         t + 1
            ora         t
            beq         @end
            lda         #IOBUF
            ldx         t + 1
            bne         :+
            cmp         t
            bcc         :+
            lda         t
:
            sta         cnt
            sta         r2                                  ; Its bytes, into iobuf
            stz         r2 + 1
            LDR         r0, iobuf
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         done
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         done + 1
            sta         r1 + 1
            jsr         CLIENT_READ
            ldy         #0
@pair:
            iny                                             ; (A whole pair left?)
            cpy         cnt
            bcs         @parted
            dey
            ldx         iobuf,Y                             ; .X = the register, .A = the value
            lda         iobuf + 1,Y
            phy
            bit         kind
            bmi         :+
            jsr         pair
            bra         :++
:
            jsr         psg_pair
:
            ply
            bcs         @timeout
            iny
            iny
            bra         @pair

@parted:
            clc
            lda         done
            adc         cnt
            sta         done
            bcc         @part
            inc         done + 1
            bra         @part

@end:
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
            jsr         psg_flush
            clc
            rts

@timeout:
            jsr         psg_flush
            tya                                             ; (The pairs before this one)
            clc
            adc         done
            sta         TASK_INBOX + RQ_DONE
            lda         done + 1
            adc         #0
            sta         TASK_INBOX + RQ_DONE + 1
            ora         TASK_INBOX + RQ_DONE
            beq         :+
            clc
            rts
:
            lda         #E_IO
            sec
            rts

; /snd: a read: the shadow, from the offset (256 bytes in all; past them, nothing)
r_snd:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 1
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            sec                                             ; r2: what's left of it (256 - the offset) ...
            lda         #0
            sbc         TASK_INBOX + RQ_OFFSET
            sta         r2
            lda         #1
            sbc         #0
            sta         r2 + 1
            lda         TASK_INBOX + RQ_COUNT + 1           ;   or the count, if that's less
            cmp         r2 + 1
            bcc         @count
            bne         @give
            lda         TASK_INBOX + RQ_COUNT
            cmp         r2
            bcs         @give
@count:
            MOVR        r2, TASK_INBOX + RQ_COUNT
@give:
            clc                                             ; r0: from the offset
            lda         #<shadow
            adc         TASK_INBOX + RQ_OFFSET
            sta         r0
            lda         #>shadow
            adc         #0
            sta         r0 + 1
            jsr         srv_toclient
@end:
            clc
            rts

; /psg: a read: the PSG's registers as written, from the offset (64 bytes in all; past them, nothing)
r_psg:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 1
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            sec                                             ; r2: what's left after the offset ...
            lda         #PSG_REGS
            sbc         TASK_INBOX + RQ_OFFSET
            bcc         @end
            beq         @end
            sta         r2
            stz         r2 + 1
            lda         TASK_INBOX + RQ_COUNT + 1           ;   or the count, if that's less
            bne         :+
            lda         TASK_INBOX + RQ_COUNT
            cmp         r2
            bcs         :+
            sta         r2
:
            clc
            lda         #<psg
            adc         TASK_INBOX + RQ_OFFSET
            sta         r0
            lda         #>psg
            adc         #0
            sta         r0 + 1
            jsr         srv_toclient
@end:
            clc
            rts

; A /psg pair: register .X = .A through the library, unless its voice's channel is another task's (or the PSG has
; no register .X).  OUT: C = 0
psg_pair:
            cpx         #PSG_REGS
            bcs         @drop
            pha
            phx
            txa
            lsr
            lsr
            clc
            adc         #SND_PSG
            jsr         owned
            plx
            pla
            bcs         @drop
            jsr         psg_set
@drop:
            clc
            rts

; /bell: a write: the beep (channel 7 claimed: none), and all of it taken
h_bell:
            cmp         #R_WRITE
            bne         @done
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
            lda         chown + BELL_CH
            bne         @done
            jsr         beep
@done:
            clc
            rts

; sndctl's claim N [P]: those channels, the fid's task's (all of them, or none if another task has one)
c_claim:
            jsr         ctl_mask
            bcs         @done
            ldx         #0
@check:
            jsr         mask_next
            bcc         @next
            lda         chown,X
            beq         @next
            cmp         owner
            beq         @next
            lda         #E_BUSY
            sec
            rts

@next:
            inx
            cpx         #SND_CHANNELS
            bne         @check
            jsr         ctl_mask                            ; All free: taken (the mask again)
            ldx         #0
@take:
            jsr         mask_next
            bcc         :+
            lda         owner
            sta         chown,X
:
            inx
            cpx         #SND_CHANNELS
            bne         @take
            clc
@done:
            rts

; The mask's next channel's bit (from channel 0): C = 1, in the mask.  Modifies the mask
mask_next:
            lsr         mask + 2
            ror         mask + 1
            ror         mask
            rts

; sndctl's release N [P]: those channels given back, those the fid's task has
c_release:
            jsr         ctl_mask
            bcs         :+
            jmp         release
:
            rts

; The command's numbers, the channels' masks (N: bit n, channel n, 0-7; P, if it's there: bit n, channel 8 + n): mask,
; owner the fid's task.  OUT: C = 0; or C = 1, .A = E_INVAL (none)
ctl_mask:
            lda         z:srv_argn
            beq         @inval
            ldx         z:srv_fid
            lda         srv_fid_aux,X
            sta         owner
            lda         srv_arg
            sta         mask
            stz         mask + 1
            stz         mask + 2
            lda         z:srv_argn
            cmp         #2
            bcc         :+
            lda         srv_arg + 2
            sta         mask + 1
            lda         srv_arg + 3
            sta         mask + 2
:
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; sndctl's volume N: the master volume
c_volume:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1                         ; (Past 255: 200, as past 200)
            beq         :+
            lda         #255
            bra         :++
:
            lda         srv_arg
:
            jsr         master_set
            jsr         psg_flush
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; sndctl's reset: the chips and the library cleared
c_reset:
            jsr         reset
            jmp         ctl_io

; sndctl's channel commands, each its binary command's (/snd's SND_R_*) as text: patch CH P, note CH N, off CH,
; level CH V (vol, its old name), pan CH left|right|both (or 0-3), bend CH B (signed: -128 to 127), drum CH N; and
; reg R V [R V], registers as /snd writes them.  A channel another task has claimed: E_BUSY; a number out of range,
; or one missing: E_INVAL; a PSG channel with no card: E_NODEV; the chip not answering: E_IO.  (ctl_ch and
; ctl_chbyte answer an error themselves)
c_patch:
            ldx         #SND_PATCHES
            jsr         ctl_chbyte
            jsr         cmd_patch
            jmp         ctl_io

c_note:
            ldx         #$80
            jsr         ctl_chbyte
            jsr         cmd_note
            jmp         ctl_io

c_off:
            jsr         ctl_ch
            jsr         cmd_off
            jmp         ctl_io

c_level:
            ldx         #$80
            jsr         ctl_chbyte
            jsr         cmd_vol
            jmp         ctl_io

c_drum:
            ldx         #SND_DRUMS
            jsr         ctl_chbyte
            jsr         cmd_drum
            jmp         ctl_io

c_pan:
            jsr         ctl_ch
            lda         z:srv_argn                          ; (Its speakers: a word, or a number)
            cmp         #2
            bcc         @inval
            lda         srv_argp + 2
            sta         z:srv_p
            lda         srv_argp + 3
            sta         z:srv_p + 1
            ldx         #0
@word:
            lda         pan_words,X
            sta         r3
            lda         pan_words + 1,X
            sta         r3 + 1
            jsr         srv_same                            ; (Keeps .X)
            beq         @named
            inx
            inx
            cpx         #6
            bcc         @word
            lda         (srv_p)                             ; A number, 0-3 (not a word that reads as 0)
            cmp         #'0'
            bcc         @inval
            cmp         #'9' + 1
            bcs         @inval
            lda         srv_arg + 3
            bne         @inval
            lda         srv_arg + 2
            cmp         #4
            bcc         @set
@inval:
            jmp         ctl_inval

@named:
            txa                                             ; (left 1, right 2, both 3)
            lsr
            inc         a
@set:
            ldy         ch
            jsr         cmd_pan
            jmp         ctl_io

c_bend:
            jsr         ctl_ch
            lda         z:srv_argn
            cmp         #2
            bcc         @inval
            lda         srv_arg + 2                         ; -128 to 127: its high byte the low one's sign
            asl
            lda         srv_arg + 3
            adc         #0
            bne         @inval
            lda         srv_arg + 2
            ldy         ch
            jsr         cmd_bend
            jmp         ctl_io

@inval:
            jmp         ctl_inval

c_reg:
            lda         z:srv_argn                          ; Pairs: 2 or 4 numbers, each a byte
            cmp         #2
            beq         :+
            cmp         #4
            bne         @inval
:
            asl
            sta         cnt                                 ; (Their bytes)
            ldx         z:srv_fid
            lda         srv_fid_aux,X
            sta         owner
            ldy         #0
@byte:
            lda         srv_arg + 1,Y
            bne         @inval
            iny
            iny
            cpy         cnt
            bcc         @byte
            ldy         #0
@pair:
            ldx         srv_arg,Y
            lda         srv_arg + 2,Y
            phy
            jsr         pair
            ply
            bcs         @ioerr
            iny
            iny
            iny
            iny
            cpy         cnt
            bcc         @pair
            clc
@ioerr:
            jmp         ctl_io

@inval:
            jmp         ctl_inval

; freq CH HZ: a note at a frequency (0: off); glide CH N: the pitch to note N without a new attack
c_freq:
            jsr         ctl_ch
            lda         z:srv_argn
            cmp         #2
            bcc         @inval
            lda         srv_arg + 2
            sta         shadow + SND_R_FREQ_LO
            lda         srv_arg + 3
            ldy         ch
            jsr         cmd_freq
            jmp         ctl_io

@inval:
            jmp         ctl_inval

c_glide:
            ldx         #$80
            jsr         ctl_chbyte
            jsr         cmd_glide
            jmp         ctl_io

; lfo RATE PMD AMD WAVE: the LFO (the whole chip's): its rate (0-255), its depths of pitch and of amplitude (0-127),
; its waveform (0 saw, 1 square, 2 triangle, 3 noise; CT kept)
c_lfo:
            lda         z:srv_argn
            cmp         #4
            bne         @inval
            ldy         #6                                  ; (Each a byte)
:
            lda         srv_arg + 1,Y
            bne         @inval
            dey
            dey
            bpl         :-
            lda         srv_arg + 2
            bmi         @inval
            lda         srv_arg + 4
            bmi         @inval
            lda         srv_arg + 6
            cmp         #4
            bcs         @inval
            ldx         #$18                                ; The rate ...
            lda         srv_arg
            jsr         set
            bcs         @io
            ldx         #$19                                ;   the pitch's depth (bit 7) ...
            lda         srv_arg + 2
            ora         #$80
            jsr         set
            bcs         @io
            lda         srv_arg + 4                         ;   the amplitude's ...
            jsr         set
            bcs         @io
            ldx         #$1B                                ;   and the waveform
            lda         shadow + $1B
            and         #$C0
            ora         srv_arg + 6
            jsr         set
@io:
            jmp         ctl_io

@inval:
            jmp         ctl_inval

; wave CH W [WIDTH]: a PSG channel's waveform (pulse, saw, triangle, noise: SND_WAVE_*, or its number) and its width
; (0-63; 63, a square, if it isn't given: for the others, 63 their own shape)
c_wave:
            jsr         ctl_ch
            cpy         #SND_PSG
            bcc         @inval                              ; (The YM2151's channels have none)
            lda         z:srv_argn
            cmp         #2
            bcc         @inval
            lda         srv_argp + 2
            sta         z:srv_p
            lda         srv_argp + 3
            sta         z:srv_p + 1
            ldx         #0
@word:
            lda         wave_words,X
            sta         r3
            lda         wave_words + 1,X
            sta         r3 + 1
            jsr         srv_same                            ; (Keeps .X)
            beq         @named
            inx
            inx
            cpx         #8
            bcc         @word
            lda         (srv_p)                             ; A number, 0-3 (not a word that reads as 0)
            cmp         #'0'
            bcc         @inval
            cmp         #'9' + 1
            bcs         @inval
            lda         srv_arg + 3
            bne         @inval
            lda         srv_arg + 2
            cmp         #4
            bcc         @wave
@inval:
            jmp         ctl_inval

@named:
            txa
            lsr
@wave:
            lsr                                             ; (Bits 1-0 to 7-6)
            ror
            ror
            sta         t
            lda         #$3F                                ; Its width
            ldx         z:srv_argn
            cpx         #3
            bcc         :+
            lda         srv_arg + 5
            bne         @inval
            lda         srv_arg + 4
            cmp         #$40
            bcs         @inval
:
            ora         t
            ldy         ch
            jsr         psg_wave
            jmp         ctl_io

; sens CH PMS AMS: the channel's sensitivity to the LFO, of its pitch (vibrato, 0-7) and its amplitude (tremolo, 0-3)
c_sens:
            jsr         ctl_ch
            cpy         #SND_PSG
            bcs         @inval                              ; (The YM2151's alone)
            lda         z:srv_argn
            cmp         #3
            bcc         @inval
            lda         srv_arg + 3
            ora         srv_arg + 5
            bne         @inval
            lda         srv_arg + 2
            cmp         #8
            bcs         @inval
            asl
            asl
            asl
            asl
            sta         t
            lda         srv_arg + 4
            cmp         #4
            bcs         @inval
            ora         t
            pha
            lda         ch
            ora         #$38
            tax
            pla
            jsr         set
            jmp         ctl_io

@inval:
            jmp         ctl_inval

; noise N: channel 7's noise (in place of its last operator) at frequency N (0-31); noise off (or a negative N): none
c_noise:
            lda         z:srv_argn
            cmp         #1
            bne         @inval
            lda         srv_argp
            sta         z:srv_p
            lda         srv_argp + 1
            sta         z:srv_p + 1
            lda         #<s_off
            sta         r3
            lda         #>s_off
            sta         r3 + 1
            jsr         srv_same
            beq         @off
            lda         srv_arg + 1
            bmi         @off
            bne         @inval
            lda         (srv_p)                             ; (A number: not a word that reads as 0)
            cmp         #'0'
            bcc         @inval
            cmp         #'9' + 1
            bcs         @inval
            lda         srv_arg
            cmp         #32
            bcs         @inval
            ora         #$80                                ; (NE: on)
            bra         @set
@off:
            lda         #0
@set:
            ldx         #$0F
            jsr         set
            jmp         ctl_io

@inval:
            jmp         ctl_inval

; The PSG's pairs to vid, and the chip's answer (C = 1: E_IO)
ctl_io:
            php
            jsr         psg_flush
            plp
            bcc         :+
            lda         #E_IO
:
            rts

ctl_inval:
            lda         #E_INVAL
            sec
            rts

; A channel command's channel (its first number) and the value after it, below .X: .A = the value, .Y = ch = the
; channel, owner its task.  An error answers the command (its handler's caller): E_INVAL (none, or out of range),
; E_BUSY (another task's channel)
ctl_chbyte:
            stx         t
            lda         z:srv_argn
            cmp         #2
            bcc         ctl_drop
            lda         srv_arg + 3
            bne         ctl_drop
            lda         srv_arg + 2
            cmp         t
            bcs         ctl_drop
            jsr         ctl_ch1
            bcs         ctl_busy
            lda         srv_arg + 2
            ldy         ch
            rts

; A channel command's channel: ch = .Y = it, owner its task (an error as ctl_chbyte's)
ctl_ch:
            lda         z:srv_argn
            beq         ctl_drop
            jsr         ctl_ch1
            bcs         ctl_busy
            ldy         ch
            rts

ctl_drop:
            pla                                             ; (The command answered: its handler's return
            pla                                             ;   dropped)
            bra         ctl_inval

ctl_busy:
            pla
            pla
            lda         #E_BUSY
            rts

; ch = the first number, owner the fid's task: C = 1 if another task has it.  (Out of range: ctl_drop's, from
; ctl_ch's or ctl_chbyte's caller; the PSG's with no card, E_NODEV, the same way)
ctl_ch1:
            lda         srv_arg + 1
            bne         @range
            lda         srv_arg
            cmp         #SND_CHANNELS
            bcs         @range
            sta         ch
            cmp         #SND_PSG
            bcc         :+
            jsr         psg_here
            bcs         @nodev
:
            ldx         z:srv_fid
            lda         srv_fid_aux,X
            sta         owner
            lda         ch
            jmp         owned

@range:
            pla                                             ; (ctl_ch1's own return dropped too)
            pla
            bra         ctl_drop

@nodev:
            pla                                             ; (Its own return and ctl_ch's)
            pla
            pla
            pla
            lda         #E_NODEV
            sec
            rts

; sndctl's state: "volume 100", "channels 24" (with no card, 8), then "claimed" and the channels claimed
gen_ctl:
            jsr         psg_here                            ; (A card's PSG: its registers sent, if they're new)
            lda         #SND_PSG
            bcs         :+
            jsr         psg_flush
            lda         #SND_CHANNELS
:
            pha
            lda         #<s_volume
            ldx         #>s_volume
            jsr         srv_tputs
            lda         master
            ldx         #0
            jsr         srv_tputdec
            lda         #<s_channels
            ldx         #>s_channels
            jsr         srv_tputs
            pla
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            lda         #<s_claimed
            ldx         #>s_claimed
            jsr         srv_tputs
            ldy         #0
:
            lda         chown,Y
            beq         :+
            lda         #' '
            jsr         srv_tputc
            tya
            ldx         #0
            jsr         srv_tputdec                         ; (Keeps .Y)
:
            iny
            cpy         #SND_CHANNELS
            bne         :--
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; A fid made (srvlib): a new one is its opener's (the client task's, + 1: srv_fid_aux; R_DUP's keeps its old
; fid's), and that task's fids counted.  IN: .X = the fid.  Keeps .X
opened:
            lda         z:srv_rq
            cmp         #R_OPEN
            bne         :+
            lda         TASK_INBOX + RQ_CLIENT
            inc         a
            sta         srv_fid_aux,X
:
            ldy         srv_fid_aux,X
            lda         refs - 1,Y
            inc         a
            sta         refs - 1,Y
            clc
            rts

; A fid forgotten (srvlib): its task's count down; with the task's last, its channels back (keyed off).  IN: .X =
; the fid
clunked:
            ldy         srv_fid_aux,X
            beq         @done
            lda         refs - 1,Y
            beq         @done
            dec         a
            sta         refs - 1,Y
            bne         @done
            sty         owner
            lda         #$FF
            sta         mask
            sta         mask + 1
            sta         mask + 2
            jsr         release
@done:
            clc
            rts

; A stat record made (srvlib): /snd's length, 256; /psg's, 64
stat:
            lda         z:srv_e
            cmp         #ENT_SND
            bne         :+
            lda         #1
            sta         srv_stat + SR_LENGTH + 1
:
            cmp         #ENT_PSG
            bne         :+
            lda         #PSG_REGS
            sta         srv_stat + SR_LENGTH
:
            rts

; ****************************************************************************
; Claims, the volume

; The channels in mask given back, those owner has (keyed off).  OUT: C = 0
release:
            ldx         #0
@channel:
            jsr         mask_next
            bcc         @next
            lda         chown,X
            cmp         owner
            bne         @next
            stz         chown,X
            phx
            txa                                             ; Key off
            tay
            jsr         cmd_off
            plx
@next:
            inx
            cpx         #SND_CHANNELS
            bne         @channel
            jsr         psg_flush
            clc
            rts

; Is channel .A another task's (not owner's)?  OUT: C = 1 yes.  Modifies .A, .X
owned:
            tax
            lda         chown,X
            beq         @free
            cmp         owner
            beq         @free
            sec
            rts

@free:
            clc
            rts

; The master volume: its TL steps (master_tl), every channel's attenuation, and the levels written again.  IN: .A =
; the volume, a percentage: 0-99, quieter (the volume curve at about .A * 1.27); 100, as written; 101-200, louder:
; 0.32 of a TL step (0.75 dB) a point, 31 steps (23 dB) at 200 (more: 200)
master_set:
            cmp         #201
            bcc         :+
            lda         #200
:
            sta         master
            sec
            sbc         #100
            bcc         @quieter
            sta         t                                   ; Louder: -(n / 4 + n / 16), n = .A - 100
            lsr
            lsr
            sta         t + 1
            lsr
            lsr
            clc
            adc         t + 1
            eor         #$FF
            inc         a                                   ; (Negative)
            bra         @set

@quieter:
            lda         master                              ; The curve at .A + .A / 4 + .A / 64 (about * 1.27)
            lsr
            lsr
            sta         t + 1
            lsr
            lsr
            lsr
            lsr
            clc
            adc         t + 1
            adc         master
            tax
            lda         volume_atten,X
@set:
            sta         master_tl
            ldy         #SND_CHANNELS - 1
@channel:
            jsr         atten_set
            tya
            jsr         recook
            dey
            bpl         @channel
            rts

; ****************************************************************************
; The library

; Reset: every channel at full volume, with no bend and no note; the PSG's voices silent, on both speakers, a
; square (to vid); the YM2151 cleared (chip_init), the shadow too, and each channel on both speakers.  The claims
; stay.  OUT: C = 0; or C = 1: the YM2151 didn't answer
reset:
            ldy         #SND_CHANNELS - 1
@channel:
            lda         #$7F
            sta         vol,Y
            lda         #0
            sta         bend,Y
            sta         fine,Y
            lda         #$FF
            sta         note,Y
            jsr         atten_set
            dey
            bpl         @channel
            ldx         #PSG_REGS - 4                       ; The PSG's voices ...
@voice:
            stz         psg,X
            stz         psg + 1,X
            lda         #$C0
            sta         psg + 2,X
            lda         #PSG_SQUARE
            sta         psg + 3,X
            dex
            dex
            dex
            dex
            bpl         @voice
            ldx         #0                                  ;   to the chip
:
            jsr         psg_put
            inx
            cpx         #PSG_REGS
            bcc         :-
            jsr         psg_flush
            jsr         chip_init                           ; The YM2151
            bcs         @done
            ldx         #0
            txa
:
            sta         shadow,X
            inx
            bne         :-
            ldy         #7
@fm:
            tya
            ora         #$20                                ; Both speakers (RL), algorithm 0
            tax
            lda         #$C0
            jsr         set
            bcs         @done
            dey
            bpl         @fm
            clc
@done:
            rts

; Set up the YM2151, whatever it powered up with (its reset, /IC, may not clear everything): the timers stopped,
; their interrupts off and their flags reset, then every register $01-$FF zeroed (the test/LFO register $01, noise,
; the timers' periods, and the voices).  OUT: C = 0; or C = 1: the chip didn't answer
chip_init:
            lda         #$30                                ; (Timers stopped, their interrupts off, their flags
            ldx         #$14                                ;   reset)
            jsr         ym_write
            bcs         @done
            lda         #0
            ldx         #$01
:
            jsr         ym_write
            bcs         @done
            inx
            bne         :-
@done:
            rts

; Register .X = .A on the chip, its busy time (64 of its clocks after a data write) waited out first.  Only this
; task writes the chip, so the select and the write need no IRQs off.  OUT: C = 0; or C = 1: it stayed busy (no
; chip).  Keeps .A, .X, .Y
ym_write:
            phy
            ldy         #YM_TIMEOUT
@wait1:
            dey
            bmi         @timeout
            bit         YM_DATA                             ; (The status: bit 7 busy)
            bmi         @wait1
            stx         YM_REG
            ldy         #YM_TIMEOUT
@wait2:
            dey
            bmi         @timeout
            bit         YM_DATA
            bmi         @wait2
            sta         YM_DATA
            ply
            clc
            rts

@timeout:
            ply
            sec
            rts

; A channel's attenuation: its volume's and the master volume's, 127 at most; negative (-32 at most) when the
; master volume takes away more than the channel's adds.  IN: .Y = the channel.  Keeps .X, .Y
atten_set:
            phx
            ldx         vol,Y
            lda         volume_atten,X                      ; (0-127)
            clc
            bit         master_tl
            bmi         :+
            adc         master_tl                           ; (0-127 more: the sum fits a byte)
            bpl         @set
            lda         #$7F
            bra         @set
:
            adc         master_tl                           ; (Louder: -32 to 127, as it comes out)
@set:
            sta         atten,Y
            plx
            rts

; Register .X = .A through the library: the shadow; a carrier's level with the attenuation; a new algorithm, the
; levels again.  OUT: C = 0; or C = 1: the chip didn't take it.  Keeps .X, .Y
set:
            cpx         #$20
            bcc         @plain
            cpx         #$28
            bcs         @level
            pha                                             ; $20-$27: the algorithm changed?
            eor         shadow,X
            and         #$07
            sta         t
            pla
            sta         shadow,X
            jsr         ym_write
            bcs         @done
            lda         t
            beq         @done                               ; (C = 0)
            txa
            and         #$07
            jmp         recook

@level:
            sta         shadow,X
            cpx         #$60
            bcc         @write
            cpx         #$80
            bcs         @write
            jsr         cook
            bra         @write

@plain:
            sta         shadow,X
@write:
            jmp         ym_write

@done:
            rts

; The level for the chip: a carrier's TL (register .X, $60-$7F; .A = as written) with its channel's attenuation,
; 127 at most; another operator's as it is.  OUT: .A.  Keeps .X, .Y
cook:
            phy
            sta         t
            txa
            and         #$07
            tay                                             ; .Y = the channel
            lda         shadow + $20,Y
            and         #$07
            sta         t + 1                               ; Its algorithm
            txa
            and         #$18                                ; The operator (0, 8, $10, $18: M1, M2, C1, C2) ...
            ora         t + 1
            phx
            tax
            lda         carrier,X                           ;   and the algorithm: a carrier?
            plx
            cmp         #0
            beq         @as_is
            lda         atten,Y
            bmi         @louder
            clc
            adc         t
            bpl         @done                               ; (Each is 127 at most: the sum fits)
            lda         #$7F
            bra         @done

@louder:                                                    ; (Less than as written: down to 0, the loudest)
            clc
            adc         t
            bcs         @done
            lda         #0
            bra         @done

@as_is:
            lda         t
@done:
            ply
            rts

; A channel's four levels written again (a new algorithm or volume; the PSG's, its volume).  IN: .A = the channel.
; OUT: C = 0; or C = 1: the chip didn't take one.  Keeps .X, .Y
recook:
            cmp         #SND_PSG
            bcc         :+
            jmp         psg_recook
:
            phx
            ora         #$60
            tax
@op:
            lda         shadow,X
            jsr         cook
            jsr         ym_write
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

; A channel's pitch: its note and bend as its key code and fraction ($28 and $30), if it has a note.  IN: .Y = the
; channel.  OUT: C = 0; or C = 1: the chip didn't take it.  Keeps .Y
pitch:
            lda         note,Y
            bpl         :+
            clc                                             ; ($FF: no note)
            rts
:
            sta         t + 1                               ; t = the note * 64 ...
            stz         t
            lsr         t + 1
            ror         t
            lsr         t + 1
            ror         t
            lda         t                                   ;   + its 64ths (0-63: the low bits are 0) ...
            ora         fine,Y
            sta         t
            ldx         #0                                  ;   + the bend (signed)
            lda         bend,Y
            bpl         :+
            dex
:
            clc
            adc         t
            sta         t
            txa
            adc         t + 1
            bpl         :+
            lda         #0                                  ; (Below note 0: note 0)
            sta         t
:
            sta         t + 1
            cpy         #SND_PSG                            ; (The PSG's: its frequency)
            bcc         :+
            jmp         psg_pitch
:
            lda         t                                   ; The note: t / 64
            asl
            rol         t + 1
            asl
            rol         t + 1                               ; (t + 1 = the note, 0-129)
            lda         t
            and         #$3F
            asl
            asl
            pha                                             ; The key fraction, for $30
            lda         t + 1
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
            ora         note_code,Y
            ply
            pha
            tya
            ora         #$28
            tax
            pla
            jsr         set                                 ; The key code ...
            pla
            bcs         @done
            pha
            txa
            adc         #$08                                ; ($28 + 8: $30; C = 0)
            tax
            pla
            jmp         set                                 ;   and the fraction

@done:
            rts

; Load patch .A (below SND_PATCHES) into channel ch (its speakers stay the channel's).  OUT: C = 0; or C = 1: the
; chip didn't take it
load_patch:
            sta         t                                   ; ptr = patches + the patch * 26
            stz         t + 1
            asl         t
            rol         t + 1                               ; (* 2)
            lda         t
            sta         ptr
            lda         t + 1
            sta         ptr + 1
            asl         t
            rol         t + 1
            asl         t
            rol         t + 1                               ; (* 8)
            jsr         @add
            asl         t
            rol         t + 1                               ; (* 16)
            jsr         @add
            lda         #<patches
            sta         t
            lda         #>patches
            sta         t + 1
            jsr         @add
            lda         ch                                  ; $20: RL (the channel's), FB, CON
            ora         #$20
            tax
            lda         shadow,X
            and         #$C0
            sta         t
            lda         (ptr)
            and         #$3F
            ora         t
            jsr         set
            bcs         @done
            txa                                             ; $38: PMS, AMS
            adc         #$18                                ; (C = 0)
            tax
            ldy         #1
            lda         (ptr),Y
            jsr         set
            bcs         @done
@next:                                                      ; $40-$F8: the operators
            iny
            txa
            clc
            adc         #$08
            bcs         @loaded
            tax
            lda         (ptr),Y
            jsr         set
            bcc         @next
            rts

@loaded:
            clc
@done:
            rts

@add:                                                       ; ptr += t
            clc
            lda         ptr
            adc         t
            sta         ptr
            lda         ptr + 1
            adc         t + 1
            sta         ptr + 1
            rts

; Channel ch keyed off, then on (all four operators; the PSG's, its volume up).  OUT: C = 0; or C = 1: the chip
; didn't take it
key_on:
            lda         ch
            cmp         #SND_PSG
            bcc         :+
            jmp         psg_on
:
            ldx         #$08
            jsr         set
            bcs         @done
            ora         #$78
            jmp         set

@done:
            rts

; A register/value pair from a client (task owner): a command (SND_R_*), a register written through the library,
; or nothing: a channel another task has claimed, or a register the chip doesn't have.  IN: .X = the register, .A =
; the value.  OUT: C = 0; or C = 1: the chip didn't take it.  Modifies .A, .X, .Y
pair:
            cpx         #$20
            bcs         @channel_reg
            cpx         #$14                                ; The timers' control: their interrupts kept off
            bne         :+
            jmp         timers_set
:
            ldy         low_reg,X                           ; What register .X is
            beq         @keep                               ; (None the chip has: only kept)
            cpy         #LR_CHIP
            beq         @write                              ; One for the whole chip
            cpy         #LR_KEY
            beq         @key
            cpy         #LR_SELECT
            beq         @select
            sta         shadow,X                            ; A command, for the channel SND_R_CH chose
            pha
            lda         shadow + SND_R_CH
            cmp         #SND_CHANNELS
            bcs         @not_mine                           ; (One there isn't: nothing)
            sta         ch
            jsr         owned
            bcs         @not_mine
            lda         cmd_lo - LR_CMD,Y
            sta         ptr
            lda         cmd_hi - LR_CMD,Y
            sta         ptr + 1
            pla
            ldy         ch
            jmp         (ptr)                               ; (.A = the value, .Y = the channel)

@not_mine:
            pla
            clc
            rts

@channel_reg:                                               ; $20-$FF: the channel is the register's
            pha
            phx
            txa
            and         #$07
            jsr         owned
            plx
            pla
            bcc         @write
            clc
            rts

@key:                                                       ; $08 (key on and off): the channel is the value's
            pha
            phx
            and         #$07
            jsr         owned
            plx
            pla
            bcs         @dropped
@write:
            jmp         set

@select:
@keep:
            sta         shadow,X
@dropped:
            clc
            rts

; A client's write to $14, the timers' control: their interrupts stay off (a song's register dump from another
; machine may have them on, and nothing here would clear them); CSM and timer A's load are the client's.  IN: .A =
; the value.  OUT: C = 0; or C = 1: the chip didn't take it
timers_set:
            and         #$F3
            ldx         #$14
            jmp         set

; What each register below $20 is, for pair: none the chip has (0), one for the whole chip, key on and off, the
; channel for the commands, or a command (LR_CMD + its number in cmd_lo)
LR_CHIP         = 1
LR_KEY          = 2
LR_SELECT       = 3
LR_CMD          = 4

; The commands.  IN: .A = the value, .Y = ch = the channel.  OUT: C = 0; or C = 1: the chip didn't take it
cmd_patch:
            cpy         #SND_PSG
            bcs         @psg
            cmp         #SND_PATCHES
            bcs         cmd_none
            jmp         load_patch

@psg:                                                       ; (The PSG's: a waveform, a square's width)
            cmp         #4
            bcs         cmd_none
            lsr
            ror
            ror
            ora         #$3F
            jmp         psg_wave

cmd_none:                                                   ; (Out of range: nothing)
            clc
            rts

cmd_note:
            and         #$7F
            sta         note,Y
            lda         #0
            sta         fine,Y
            jsr         pitch
            bcs         :+
            jmp         key_on
:
            rts

cmd_off:
            lda         #$FF
            sta         note,Y
            cpy         #SND_PSG
            bcc         :+
            jmp         psg_off
:
            ldx         #$08
            tya
            jmp         set

cmd_vol:
            and         #$7F
            sta         vol,Y
            jsr         atten_set
            tya
            jmp         recook

cmd_pan:
            and         #$03                                ; SND_PAN_LEFT: L (bit 6); SND_PAN_RIGHT: R (bit 7)
            asl
            asl
            asl
            asl
            asl
            asl
            sta         t
            cpy         #SND_PSG
            bcs         @psg
            tya
            ora         #$20
            tax
            lda         shadow,X
            and         #$3F
            ora         t
            jmp         set

@psg:                                                       ; (The PSG's: its volume register's bits 7-6, the
            jsr         psg_reg                             ;   same)
            inx
            inx
            lda         psg,X
            and         #$3F
            ora         t
            jsr         psg_set
            clc
            rts

cmd_bend:
            sta         bend,Y
            jmp         pitch

cmd_drum:
            cpy         #SND_PSG                            ; (The PSG: none)
            bcs         cmd_none
            cmp         #SND_DRUMS
            bcs         cmd_none
            tax
            lda         drum_kc,X                           ; Its pitch (after the patch)
            pha
            lda         drum_patch,X
            jsr         load_patch
            bcs         @failed
            ldy         ch
            lda         #$FF                                ; (No note: a bend leaves it alone)
            sta         note,Y
            tya
            ora         #$28
            tax
            pla
            jsr         set                                 ; The key code ...
            bcs         cmd_done
            txa
            adc         #$08                                ; (C = 0)
            tax
            lda         #0
            jsr         set                                 ;   no fraction
            bcs         cmd_done
            jmp         key_on

@failed:
            pla
cmd_done:
            rts

; A frequency (Hz: SND_R_FREQ_LO's byte, and this the high one; 0, off), keyed on: its MIDI note and 64ths, from a
; table of the octave C8-C9 (4,186-8,372 Hz) that the frequency is doubled or halved into (each an octave), the
; 64ths between its semitones taken as a straight line (a cent's error at most).  As high or low as the chip goes
cmd_freq:
            sta         t + 1
            lda         shadow + SND_R_FREQ_LO
            sta         t
            ora         t + 1
            bne         :+
            jmp         cmd_off
:
            phy
            lda         #108 + 64                           ; The table's octave's note (+ 64, as it's moved)
            sta         f_note
@double:
            lda         t + 1                               ; Below C8: doubled, its note an octave down
            cmp         #>FREQ_C8
            bcc         :+
            bne         @halve
            lda         t
            cmp         #<FREQ_C8
            bcs         @halve
:
            asl         t
            rol         t + 1
            lda         f_note
            sec
            sbc         #12
            sta         f_note
            bra         @double
@halve:
            lda         t + 1                               ; C9 or more: halved, an octave up
            cmp         #>FREQ_C9
            bcc         @semitone
            bne         :+
            lda         t
            cmp         #<FREQ_C9
            bcc         @semitone
:
            lsr         t + 1
            ror         t
            lda         f_note
            clc
            adc         #12
            sta         f_note
            bra         @halve
@semitone:
            ldy         #0                                  ; .Y: the semitone (t below the next one's)
@find:
            lda         t + 1
            cmp         freq_hi + 1,Y
            bcc         @found
            bne         :+
            lda         t
            cmp         freq_lo + 1,Y
            bcc         @found
:
            iny
            cpy         #11
            bcc         @find
@found:
            sty         f_semi
            sec                                             ; t: how far above it; ptr: the semitone's width
            lda         t
            sbc         freq_lo,Y
            sta         t
            lda         t + 1
            sbc         freq_hi,Y
            sta         t + 1
            sec
            lda         freq_lo + 1,Y
            sbc         freq_lo,Y
            sta         ptr
            lda         freq_hi + 1,Y
            sbc         freq_hi,Y
            sta         ptr + 1
            ldy         #6                                  ; The 64ths: t * 64 / the width (t * 64 < 32768)
:
            asl         t
            rol         t + 1
            dey
            bne         :-
            ldx         #0
@divide:
            sec
            lda         t
            sbc         ptr
            pha
            lda         t + 1
            sbc         ptr + 1
            bcc         @divided
            sta         t + 1
            pla
            sta         t
            inx
            bra         @divide
@divided:
            pla
            ply
            lda         f_note                              ; Its note: 0-127 (past them: the end's)
            clc
            adc         f_semi
            sec
            sbc         #64
            bcs         :+
            lda         #0
            tax
:
            bpl         :+
            lda         #127
            ldx         #63
:
            sta         note,Y
            txa
            sta         fine,Y
            jsr         pitch
            bcs         :+
            jmp         key_on
:
            rts

; Pitch to note .A without a new attack (legato: MML's &, the X16's negative notes); a channel with no note
; takes it for its next (a bend moves it too)
cmd_glide:
            and         #$7F
            sta         note,Y
            lda         #0
            sta         fine,Y
            jmp         pitch

; A PSG channel's waveform and width (the voice's register 3: SND_WAVE_* << 6 | the width); the YM2151's, nothing
cmd_wave:
            cpy         #SND_PSG
            bcs         :+
            clc
            rts
:
            jmp         psg_wave

; ****************************************************************************
; The PSG (the Vera X's, through vid: #v/psg): channels 8-23, its voices 0-15

; Channel .Y's voice's first register (4 a voice): .X.  Modifies .A.  Keeps .Y
psg_reg:
            tya
            sec
            sbc         #SND_PSG
            asl
            asl
            tax
            rts

; PSG register .X = .A as written: kept, and queued for the chip (a volume with its channel's attenuation).  Keeps
; .X, .Y
psg_set:
            sta         psg,X
; PSG register .X queued for the chip as kept (a volume attenuated; the queue full, sent first).  Keeps .X, .Y
psg_put:
            phy
            lda         psg_n
            cmp         #PSG_OUT
            bcc         :+
            phx
            jsr         psg_flush
            plx
:
            txa
            and         #$03
            cmp         #2
            bne         @plain
            txa                                             ; A volume: its channel's
            lsr
            lsr
            clc
            adc         #SND_PSG
            tay
            lda         psg,X
            jsr         psg_cook
            bra         @queue

@plain:
            lda         psg,X
@queue:
            ldy         psg_n
            sta         psg_out + 1,Y
            txa
            sta         psg_out,Y
            iny
            iny
            sty         psg_n
            ply
            rts

; A PSG volume register for the chip: .A as written (speakers, volume) with channel .Y's attenuation (TL's 0.75 dB
; steps, 1.5 of the PSG's 0.5 dB each), 0-63; a volume 0 stays 0.  OUT: .A.  Modifies u.  Keeps .X, .Y
psg_cook:
            sta         u                                   ; Its speakers ...
            and         #$3F
            beq         @done                               ;   and its volume
            sta         u + 1
            lda         atten,Y
            bmi         @louder
            lsr                                             ; Quieter: less the attenuation * 1.5, to 0
            clc
            adc         atten,Y
            sta         u + 2
            lda         u + 1
            sec
            sbc         u + 2
            bcs         @done
            lda         #0
            bra         @done

@louder:
            eor         #$FF                                ; Louder (the master volume past 100): more by
            inc         a                                   ;   -the attenuation * 1.5, to 63
            sta         u + 2
            lsr
            clc
            adc         u + 2
            adc         u + 1                               ; (C = 0: 48 + 63 at most)
            cmp         #$40
            bcc         @done
            lda         #$3F
@done:
            sta         u + 2
            lda         u
            and         #$C0
            ora         u + 2
            rts

; The PSG's queued pairs sent to vid (#v/psg: opened the first time, every register sent then, as the chip has
; none of them yet).  No card: dropped.  Modifies .A, .X, .Y, r0, r1
psg_flush:
            lda         psg_n
            beq         @done
            lda         psg_fd
            bpl         @send
            jsr         psg_open
            bcs         @drop
@send:
            LDR         r0, psg_out
            lda         psg_n
            sta         r1
            stz         r1 + 1
            lda         psg_fd
            jsr         WRITE
@drop:
            stz         psg_n
@done:
            rts

; Is there a PSG (a Vera X: #v/psg open, or opened now)?  OUT: C = 0 yes; C = 1 no.  Modifies .A, .X, .Y, r0, r1
psg_here:
            lda         psg_fd
            bmi         psg_open
            clc
            rts

; #v/psg opened: psg_fd, and every register queued (the queue's pairs among them).  OUT: C = 0; or C = 1: no card.
; Modifies .A, .X, .Y, r0, r1
psg_open:
            LDR         r0, s_vpsg
            lda         #O_WRITE
            jsr         OPEN
            bcs         @done
            sta         psg_fd
            stz         psg_n
            ldx         #0
:
            jsr         psg_put
            inx
            cpx         #PSG_REGS
            bcc         :-
            clc
@done:
            rts

; A PSG channel's note on: its volume as written the most (its level attenuating it).  IN: ch.  OUT: C = 0
psg_on:
            ldy         ch
            jsr         psg_reg
            inx
            inx
            lda         psg,X
            ora         #PSG_ON
            jsr         psg_set
            clc
            rts

; A PSG channel's note off: its volume 0 (speakers kept).  IN: .Y = the channel.  OUT: C = 0.  Keeps .Y
psg_off:
            jsr         psg_reg
            inx
            inx
            lda         psg,X
            and         #$C0
            jsr         psg_set
            clc
            rts

; A PSG channel's waveform and width (.A: its register 3).  IN: .Y = the channel.  OUT: C = 0.  Keeps .Y
psg_wave:
            pha
            jsr         psg_reg
            pla
            inx
            inx
            inx
            jsr         psg_set
            clc
            rts

; A PSG channel's volume to the chip again (a new attenuation).  IN: .A = the channel.  OUT: C = 0.  Keeps .X, .Y
psg_recook:
            phx
            phy
            tay
            jsr         psg_reg
            inx
            inx
            jsr         psg_put
            ply
            plx
            clc
            rts

; A PSG channel's pitch: t (in 64ths of a semitone over MIDI note 0, up to 129) as its frequency word (Hz * 2^17 /
; 48,828.125: the VERA's 25 MHz / 512), from the table of the octave C9-C10 (MIDI 120-132), the 64ths between its
; semitones a straight line (under a cent off), then halved to the note's octave, rounded.  IN: .Y = the channel.
; OUT: C = 0.  Keeps .Y
psg_pitch:
            lda         t                                   ; The note (t / 64) ...
            asl
            rol         t + 1
            asl
            rol         t + 1
            lda         t
            and         #$3F
            sta         t                                   ;   and its 64ths
            lda         t + 1
            ldx         #0                                  ; The octaves below the table's
@octave:
            cmp         #120
            bcs         :+
            adc         #12                                 ; (C = 0)
            inx
            bra         @octave
:
            stx         psh
            sbc         #120                                ; (C = 1) The semitone in it, 0-11
            asl
            tax
            sec                                             ; Its width to the next / 4 (under 1024)
            lda         psg_freq + 2,X
            sbc         psg_freq,X
            sta         pw
            lda         psg_freq + 3,X
            sbc         psg_freq + 1,X
            lsr
            ror         pw
            lsr
            ror         pw
            sta         pw + 1
            stz         pr                                  ; * the 64ths (0-63) / 16: the 64ths' part
            stz         pr + 1
            phy
            ldy         #6
@multiply:
            lsr         t
            bcc         :+
            clc
            lda         pr
            adc         pw
            sta         pr
            lda         pr + 1
            adc         pw + 1
            sta         pr + 1
:
            asl         pw
            rol         pw + 1
            dey
            bne         @multiply
            ply
            ldy         #4
:
            lsr         pr + 1
            ror         pr
            dey
            bne         :-
            clc                                             ; + the semitone's word: the table's octave's
            lda         pr
            adc         psg_freq,X
            sta         pr
            lda         pr + 1
            adc         psg_freq + 1,X
            sta         pr + 1
            ldx         psh                                 ; Halved to the note's octave, rounded (the last bit
            beq         @word                               ;   out added)
:
            lsr         pr + 1
            ror         pr
            dex
            bne         :-
            lda         pr
            adc         #0
            sta         pr
            lda         pr + 1
            adc         #0
            sta         pr + 1
@word:
            ldy         ch
            jsr         psg_reg
            lda         pr
            jsr         psg_set
            inx
            lda         pr + 1
            jsr         psg_set
            clc
            rts

; The console's bell: a short beep on channel 7, a sine (four operators in step) that fades by itself, so nothing
; has to turn it off.  (Through the library: the master volume's, and the shadow keeps it)
beep:
            ldy         #0
@reg:
            ldx         beep_regs,Y                         ; Register, value, ...; 0 ends it
            beq         @done
            lda         beep_regs + 1,Y
            jsr         set
            bcs         @done
            iny
            iny
            bra         @reg

@done:
            rts

BEEP_NOTE       = $5A           ; Key code: octave 5, A (about 880 Hz)
BEEP_LEVEL      = $00           ; Each operator's total level (attenuation: 0 = loudest)
BEEP_DECAY      = $0C           ; Their decay rates (D1R, D2R; 31 = fastest)

; All four operators are carriers (algorithm 7) on the same pitch, so their outputs add up: about 12 dB louder than
; one alone
.macro BEEP_OP slot                                         ; An operator (slot: 0 M1, 8 M2, $10 C1, $18 C2)
            .byte       $40 + slot + BELL_CH, $01           ; No detune, multiplier 1
            .byte       $60 + slot + BELL_CH, BEEP_LEVEL
            .byte       $80 + slot + BELL_CH, $1F           ; Attack at once
            .byte       $A0 + slot + BELL_CH, BEEP_DECAY
            .byte       $C0 + slot + BELL_CH, BEEP_DECAY
            .byte       $E0 + slot + BELL_CH, $FF           ; Decay all the way to silence, release fast
.endmacro

.rodata
beep_regs:
            .byte       $20 + BELL_CH, $C7                  ; Both speakers, no feedback, algorithm 7 (all carriers)
            .byte       $28 + BELL_CH, BEEP_NOTE
            .byte       $30 + BELL_CH, $00                  ; Key fraction
            .byte       $38 + BELL_CH, $00                  ; No vibrato or tremolo
            BEEP_OP     $00                                 ; M1
            BEEP_OP     $08                                 ; M2
            BEEP_OP     $10                                 ; C1
            BEEP_OP     $18                                 ; C2
            .byte       $08, BELL_CH                        ; Key off (so it starts again) ...
            .byte       $08, $78 | BELL_CH                  ;   and on: all four operators
            .byte       0
.assert     * - beep_regs < 256, error, "beep: its table in reach of .Y"

; Which operators are carriers, by operator (M1, M2, C1, C2: 8 each) and algorithm (0-7)
carrier:
            .byte       0, 0, 0, 0, 0, 0, 0, 1              ; M1: algorithm 7
            .byte       0, 0, 0, 0, 0, 1, 1, 1              ; M2: 5-7
            .byte       0, 0, 0, 0, 1, 1, 1, 1              ; C1: 4-7
            .byte       1, 1, 1, 1, 1, 1, 1, 1              ; C2: all

; The YM2151's note codes for C#, D, D#, E, F, F#, G, G#, A, A#, B, C
note_code:
            .byte       0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14

low_reg:
            .byte       0, LR_CHIP, LR_SELECT, LR_CMD, LR_CMD + 1, LR_CMD + 2, LR_CMD + 3, LR_CMD + 4
                                                            ; $00-$07: -, test and LFO reset, SND_R_CH, SND_R_PATCH,
                                                            ;   SND_R_NOTE, SND_R_OFF, SND_R_VOL, SND_R_PAN
            .byte       LR_KEY, LR_CMD + 5, LR_CMD + 6, 0, LR_CMD + 7, LR_CMD + 8, LR_CMD + 9, LR_CHIP
                                                            ; $08-$0F: key on, SND_R_BEND, SND_R_DRUM,
                                                            ;   SND_R_FREQ_LO (kept), SND_R_FREQ, SND_R_GLIDE,
                                                            ;   SND_R_WAVE, noise
            .byte       LR_CHIP, LR_CHIP, LR_CHIP, 0, LR_CHIP, 0, 0, 0
                                                            ; $10-$17: timer A, timer B, -, the timers' control
            .byte       LR_CHIP, LR_CHIP, 0, LR_CHIP, 0, 0, 0, 0
                                                            ; $18-$1F: LFO rate, depths, -, CT and the waveform
.assert     SND_R_CH = 2 .and SND_R_PATCH = 3 .and SND_R_NOTE = 4 .and SND_R_OFF = 5, error, "low_reg: the commands"
.assert     SND_R_VOL = 6 .and SND_R_PAN = 7 .and SND_R_BEND = 9 .and SND_R_DRUM = $0A, error, "low_reg: the commands"
.assert     SND_R_FREQ_LO = $0B .and SND_R_FREQ = $0C .and SND_R_GLIDE = $0D, error, "low_reg: the commands"
.assert     SND_R_WAVE = $0E, error, "low_reg: the commands"

cmd_lo:
            .lobytes    cmd_patch, cmd_note, cmd_off, cmd_vol, cmd_pan, cmd_bend, cmd_drum, cmd_freq, cmd_glide
            .lobytes    cmd_wave
cmd_hi:
            .hibytes    cmd_patch, cmd_note, cmd_off, cmd_vol, cmd_pan, cmd_bend, cmd_drum, cmd_freq, cmd_glide
            .hibytes    cmd_wave

; The frequencies of C8 to C9 (MIDI 108-120), in Hz (A4 440): cmd_freq's octave
FREQ_C8         = 4186
FREQ_C9         = 8372
freq_lo:
            .lobytes    FREQ_C8, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902, FREQ_C9
freq_hi:
            .hibytes    FREQ_C8, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902, FREQ_C9

; The PSG's frequency words (Hz * 2^17 / 48,828.125) of C9 to C10 (MIDI 120-132, A4 440): psg_pitch's octave
psg_freq:
            .word       22473, 23810, 25226, 26726, 28315, 29998, 31782, 33672, 35674, 37796, 40043, 42424, 44947

; The device
srv_tree:
            SRV_ENTRY   s_root,    $FF, SK_DIR,  0,          SM_READ,            0      ; 0
            SRV_ENTRY   s_snd,     0,   SK_DATA, h_snd,      SM_READ | SM_WRITE, 0      ; 1 (ENT_SND)
            SRV_ENTRY   s_sndctl,  0,   SK_CTL,  ctl_cmds,   SM_READ | SM_WRITE, 5      ; 2 (reads as 5)
            SRV_ENTRY   s_bell,    0,   SK_DATA, h_bell,     SM_WRITE,           0      ; 3
            SRV_ENTRY   s_psg,     0,   SK_DATA, h_psg,      SM_READ | SM_WRITE, 0      ; 4 (ENT_PSG)
            SRV_ENTRY   s_sndctl,  $FE, SK_TEXT, gen_ctl,    SM_READ,            0      ; 5 (its state: no directory's)
            .word       0
ctl_cmds:
            .word       s_claim, c_claim
            .word       s_release, c_release
            .word       s_volume_w, c_volume
            .word       s_reset, c_reset
            .word       s_patch, c_patch
            .word       s_note, c_note
            .word       s_off, c_off
            .word       s_level, c_level
            .word       s_vol, c_level
            .word       s_pan, c_pan
            .word       s_bend, c_bend
            .word       s_drum, c_drum
            .word       s_reg, c_reg
            .word       s_freq, c_freq
            .word       s_glide, c_glide
            .word       s_wave, c_wave
            .word       s_lfo, c_lfo
            .word       s_sens, c_sens
            .word       s_noise, c_noise
            .word       0
pan_words:
            .word       s_left, s_right, s_both
wave_words:
            .word       s_pulse, s_saw, s_triangle, s_noise
s_root:     .byte       "/", 0
s_snd:      .byte       "snd", 0
s_sndctl:   .byte       "sndctl", 0
s_bell:     .byte       "bell", 0
s_psg:      .byte       "psg", 0
s_vpsg:     .byte       "#v/psg", 0
s_claim:    .byte       "claim", 0
s_release:  .byte       "release", 0
s_volume_w: .byte       "volume", 0
s_reset:    .byte       "reset", 0
s_patch:    .byte       "patch", 0
s_note:     .byte       "note", 0
s_off:      .byte       "off", 0
s_level:    .byte       "level", 0
s_vol:      .byte       "vol", 0
s_pan:      .byte       "pan", 0
s_bend:     .byte       "bend", 0
s_drum:     .byte       "drum", 0
s_reg:      .byte       "reg", 0
s_freq:     .byte       "freq", 0
s_glide:    .byte       "glide", 0
s_wave:     .byte       "wave", 0
s_lfo:      .byte       "lfo", 0
s_sens:     .byte       "sens", 0
s_noise:    .byte       "noise", 0
s_left:     .byte       "left", 0
s_right:    .byte       "right", 0
s_both:     .byte       "both", 0
s_pulse:    .byte       "pulse", 0
s_saw:      .byte       "saw", 0
s_triangle: .byte       "triangle", 0
s_channels: .byte       LF, "channels ", 0
s_volume:   .byte       "volume ", 0
s_claimed:  .byte       "claimed", 0

.include "srvlib.s"
