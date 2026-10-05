; ****************************************************************************
; sound.s - HyForth's sound library (/lib/forth/sound.fl: lib sound), the old HyForth's sound words with the names and
; order C's snd.h and hylang's snd library have (docs/hyforth.md): the YM2151 through the sound driver.  /dev/snd
; takes register/value pairs (the registers the chip doesn't have are the driver's commands for a channel: SND_R_*);
; /dev/sndctl takes the words that claim and release channels, set the master volume and clear the chip.  Each is
; opened the first time it's wanted, and stays open (a task's claimed channels go back as its last one closes, at
; its end).  A channel's command is written with the channel in one write, so another program's can't come
; between.  A failure is a THROW of the system's error, named by the file (/dev/snd: busy).
;   Songs are play's (the program: play song.zsm at the shell, or s" play song.zsm" sh), not words.

.include "forthlib.inc"

.bss
snd_fd:     .res        1                                   ; /dev/snd's fd ($FF: not open) ...
ctl_fd:     .res        1                                   ;   and /dev/sndctl's
cmd:        .res        4                                   ; A write: SND_R_CH, the channel, the command, its value
ctl_buf:    .res        16                                  ; sndctl's line ...
ctl_len:    .res        1                                   ;   its length so far
.code

; Its start: neither open
lib_init:
            lda         #$FF
            sta         snd_fd
            sta         ctl_fd
            rts

            HEADER      "snd-reset", 0
sndreset:                                                   ; ( -- ): the chip and every setting cleared (old: sndinit)
            LDR         w, s_reset
            lda         #0
            jmp         snd_ctl

            HEADER      "snd-volume", 0
sndvolume:                                                  ; ( v -- ): the master volume, 0-200 (100: as written)
            LDR         w, s_volume
            bra         snd_ctl_n

            HEADER      "snd-claim", 0
sndclaim:                                                   ; ( mask -- ): channels claimed (bit n: channel n), so
            LDR         w, s_claim                          ;   no other program writes them
            bra         snd_ctl_n

            HEADER      "snd-release", 0
sndrelease:                                                 ; ( mask -- ): given back
            LDR         w, s_release
snd_ctl_n:
            lda         #1
            jmp         snd_ctl

            HEADER      "snd-reg", 0
sndreg:                                                     ; ( reg val -- ): a chip register written (old: ywrite)
            lda         dlo + 1,x
            sta         cmd
            lda         dlo,x
            sta         cmd + 1
            inx
            inx
            lda         #2
            jmp         snd_write

            HEADER      "snd-patch", 0
sndpatch:                                                   ; ( ch p -- ): patch p into channel ch (0-127: General
            lda         #SND_R_PATCH                        ;   MIDI's programs; 128-162: drums) (old: patch)
            bra         snd_cmd

            HEADER      "snd-note", 0
sndnote:                                                    ; ( ch n -- ): MIDI note n keyed on (60: middle C) (old:
            lda         #SND_R_NOTE                         ;   note)
            bra         snd_cmd

            HEADER      "snd-off", 0
sndoff:                                                     ; ( ch -- ): keyed off: the note's release (old: noteoff)
            dex
            stz         dlo,x
            lda         #SND_R_OFF
            bra         snd_cmd

            HEADER      "snd-vol", 0
sndvol:                                                     ; ( ch v -- ): the channel's volume, 0-127
            lda         #SND_R_VOL
            bra         snd_cmd

            HEADER      "snd-pan", 0
sndpan:                                                     ; ( ch pan -- ): its speakers: 1 left, 2 right, 3 both
            lda         #SND_R_PAN
            bra         snd_cmd

            HEADER      "snd-bend", 0
sndbend:                                                    ; ( ch n -- ): its bend, 64ths of a semitone (signed)
            lda         #SND_R_BEND
            bra         snd_cmd

            HEADER      "snd-drum", 0
snddrum:                                                    ; ( ch n -- ): General MIDI's drum n (35: a kick) keyed
            lda         #SND_R_DRUM                         ;   on in channel ch
snd_cmd:                                                    ; (( ch val -- ): command .A for the channel)
            sta         cmd + 2
            lda         #SND_R_CH
            sta         cmd
            lda         dlo + 1,x
            sta         cmd + 1
            lda         dlo,x
            sta         cmd + 3
            inx
            inx
            lda         #4
; cmd's first .A bytes written to /dev/snd (opened if it isn't)
snd_write:
            pha
            lda         snd_fd
            bpl         :+
            LDR         w, s_snd
            lda         #O_WRITE
            jsr         snd_open
            sta         snd_fd
:
            pla
            sta         r1
            stz         r1 + 1
            LDR         r0, cmd
            LDR         w, s_snd
            lda         snd_fd
            jmp         snd_put

; sndctl's word w (zero-terminated), with the top after it in decimal if .A <> 0 (dropped), written to /dev/sndctl
; (opened if it isn't)
snd_ctl:
            pha
            ldy         #0                                  ; The word ...
:
            lda         (w),y
            beq         :+
            sta         ctl_buf,y
            iny
            bra         :-
:
            sty         ctl_len
            pla
            beq         @line
            lda         #' '                                ;   a space and the number
            ldy         ctl_len
            sta         ctl_buf,y
            inc         ctl_len
            jsr         u_text                              ; ( u -- c-addr u )
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            inx
            inx
            sta         tmp
            ldy         #0
:
            cpy         tmp
            beq         @line
            lda         (w),y
            phy
            ldy         ctl_len
            sta         ctl_buf,y
            inc         ctl_len
            ply
            iny
            bra         :-
@line:
            lda         ctl_fd
            bpl         :+
            LDR         w, s_sndctl
            lda         #O_WRITE
            jsr         snd_open
            sta         ctl_fd
:
            lda         ctl_len
            sta         r1
            stz         r1 + 1
            LDR         r0, ctl_buf
            LDR         w, s_sndctl
            lda         ctl_fd
; WRITE fd .A, r0, r1 bytes; a failure: THROW, named w
snd_put:
            stx         xsave
            jsr         WRITE
            ldx         xsave
            bcs         snd_fail
            rts

; OPEN w (zero-terminated), mode .A: its fd (.A); a failure: THROW, named w
snd_open:
            pha
            lda         w
            sta         r0
            lda         w + 1
            sta         r0 + 1
            pla
            stx         xsave
            jsr         OPEN
            ldx         xsave
            bcs         snd_fail
            rts

; THROW the system's error .A, named w (zero-terminated: its length counted)
snd_fail:
            pha
            lda         w
            sta         throw_name
            lda         w + 1
            sta         throw_name + 1
            ldy         #0
:
            lda         (w),y
            beq         :+
            iny
            bra         :-
:
            sty         throw_nlen
            lda         #1
            sta         throw_named
            pla
            jmp         throw_os

s_snd:      .byte       "/dev/snd", 0
s_sndctl:   .byte       "/dev/sndctl", 0
s_reset:    .byte       "reset", 0
s_volume:   .byte       "volume", 0
s_claim:    .byte       "claim", 0
s_release:  .byte       "release", 0
