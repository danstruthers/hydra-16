; ****************************************************************************
; sound.s - HyForth's sound library (/lib/forth/sound.fl: lib sound), the old HyForth's sound words with the names and
; order C's snd.h and hylang's snd library have (docs/hyforth.md): the YM2151 through the sound driver.  /dev/snd
; takes register/value pairs (the registers the chip doesn't have are the driver's commands for a channel: SND_R_*);
; /dev/sndctl takes the words that claim and release channels, set the master volume and clear the chip.  Each is
; opened the first time it's wanted, and stays open (a task's claimed channels go back as its last one closes, at
; its end).  A channel's command is written with the channel in one write, so another program's can't come
; between.  A failure is a THROW of the system's error, named by the file (/dev/snd: busy).  And hylang's note-of (a
; note's MIDI number, by its name) and tune (notes and their beats played).  The two volumes: snd-volume the
; master's, snd-level a channel's (snd-vol, its old name).  snd-regs reads the registers back.  snd-freq, snd-glide
; (the driver's commands), snd-lfo, snd-sens and snd-noise (the chip's own registers) as C's and hylang's.
;   Songs are play's (the program, play song.zsm at the shell): snd-play runs it; and lines of MML (the score
; language's, play -m and -c): snd-mml, snd-chord.

.include "forthlib.inc"

.bss
snd_fd:     .res        1                                   ; /dev/snd's fd ($FF: not open) ...
ctl_fd:     .res        1                                   ;   and /dev/sndctl's
cmd:        .res        8                                   ; A write: SND_R_CH, the channel, the command, its value
ctl_buf:    .res        16                                  ; sndctl's line ...
ctl_len:    .res        1                                   ;   its length so far
tune_tpb:   .res        2                                   ; TUNE: a beat's ticks ...
tune_ch:    .res        1                                   ;   the channel ...
tune_left:  .res        1                                   ;   the tune's bytes left (from p1) ...
tune_note:  .res        1                                   ;   the note ...
tune_on:    .res        1                                   ;   <> 0: a note (not a rest) ...
tune_beats: .res        1                                   ;   and its beats
regs_fd:    .res        1                                   ; SND-REGS: its fd ...
regs_at:    .res        2                                   ;   the buffer ...
regs_n:     .res        2                                   ;   and its bytes so far
play_buf:   .res        224                                 ; SND-PLAY: play's command line ...
play_len:   .res        1                                   ;   its length ...
play_times: .res        2                                   ;   and the times asked for
line_flag:  .res        2                                   ; SND-MML, SND-CHORD: play's flag (-m, -c) ...
line_text:  .res        2                                   ;   the MML ...
line_len:   .res        1                                   ;   its length
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

            HEADER      "snd-level", 0
sndlevel:                                                   ; ( ch v -- ): the channel's level (its volume), 0-127;
            lda         #SND_R_VOL                          ;   snd-volume is the master's
            bra         snd_cmd

            HEADER      "snd-vol", 0
sndvol:                                                     ; ( ch v -- ): snd-level's old name
            bra         sndlevel

            HEADER      "snd-pan", 0
sndpan:                                                     ; ( ch pan -- ): its speakers: 1 left, 2 right, 3 both
            lda         #SND_R_PAN
            bra         snd_cmd

            HEADER      "snd-bend", 0
sndbend:                                                    ; ( ch n -- ): its bend, 64ths of a semitone (signed)
            lda         #SND_R_BEND
            bra         snd_cmd

            HEADER      "snd-glide", 0
sndglide:                                                   ; ( ch n -- ): its pitch to note n without a new attack
            lda         #SND_R_GLIDE                        ;   (legato)
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
            jmp         snd_write

            HEADER      "snd-freq", 0
sndfreq:                                                    ; ( ch hz -- ): a note at a frequency, keyed on (0: off)
            lda         #SND_R_CH
            sta         cmd
            lda         dlo + 1,x
            sta         cmd + 1
            lda         #SND_R_FREQ_LO
            sta         cmd + 2
            lda         dlo,x
            sta         cmd + 3
            lda         #SND_R_FREQ
            sta         cmd + 4
            lda         dhi,x
            sta         cmd + 5
            inx
            inx
            lda         #6
            jmp         snd_write

            HEADER      "snd-sens", 0
sndsens:                                                    ; ( ch pms ams -- ): its sensitivity to the LFO: vibrato
            lda         dlo + 2,x                           ;   0-7, tremolo 0-3 (register $38 + ch)
            ora         #$38
            sta         cmd
            lda         dlo + 1,x
            and         #7
            asl
            asl
            asl
            asl
            sta         cmd + 1
            lda         dlo,x
            and         #3
            ora         cmd + 1
            sta         cmd + 1
            inx
            inx
            inx
            lda         #2
            jmp         snd_write

            HEADER      "snd-noise", 0
sndnoise:                                                   ; ( n -- ): channel 7's noise at frequency n (0-31);
            lda         #$0F                                ;   negative: off (register $0F)
            sta         cmd
            lda         dhi,x
            bmi         :+
            lda         dlo,x
            and         #31
            ora         #$80
            bra         :++
:
            lda         #0
:
            sta         cmd + 1
            inx
            lda         #2
            jmp         snd_write

            HEADER      "snd-lfo", 0
sndlfo:                                                     ; ( rate pmd amd wave -- ): the LFO (the whole chip's): its
            lda         #$18                                ;   rate, its depths of pitch and amplitude (0-127), its
            sta         cmd                                 ;   wave (0 saw, 1 square, 2 triangle, 3 noise)
            lda         dlo + 3,x
            sta         cmd + 1
            lda         #$19
            sta         cmd + 2
            sta         cmd + 4
            lda         dlo + 2,x
            ora         #$80
            sta         cmd + 3
            lda         dlo + 1,x
            and         #$7F
            sta         cmd + 5
            lda         #$1B
            sta         cmd + 6
            lda         dlo,x
            and         #3
            sta         cmd + 7
            inx
            inx
            inx
            inx
            lda         #8
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

            HEADER      "snd-regs", 0
sndregs:                                                    ; ( c-addr -- ): the chip's 256 registers as written (the
            LDR         w, s_snd                            ;   driver's shadow: the chip's can't be read) into c-addr
            lda         #O_READ                             ;   (/dev/snd opened for it, from its start, and closed)
            jsr         snd_open
            sta         regs_fd
            lda         dlo,x
            sta         regs_at
            lda         dhi,x
            sta         regs_at + 1
            inx
            stz         regs_n
            stz         regs_n + 1
@read:
            clc                                             ; r0: where the next bytes go; r1: how many are left
            lda         regs_at
            adc         regs_n
            sta         r0
            lda         regs_at + 1
            adc         regs_n + 1
            sta         r0 + 1
            sec
            lda         #<256
            sbc         regs_n
            sta         r1
            lda         #>256
            sbc         regs_n + 1
            sta         r1 + 1
            lda         regs_fd
            stx         xsave
            jsr         READ
            bcs         @fail
            sta         tmp
            txa
            ldx         xsave
            ora         tmp                                 ; (The end: as much as there was)
            beq         @done
            clc
            lda         regs_n
            adc         tmp
            sta         regs_n
            bcc         :+
            inc         regs_n + 1
:
            lda         regs_n + 1                          ; (All 256: done)
            beq         @read
@done:
            lda         regs_fd
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            rts

@fail:
            ldx         xsave
            pha
            lda         regs_fd
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            pla
            LDR         w, s_snd
            jmp         snd_fail

            HEADER      "snd-play", 0
sndplay:                                                    ; ( c-addr u times -- status ): a song (a ZSM file) played
            lda         dlo,x                               ;   times times (0: its loop till Ctrl-C) by play, the
            sta         play_times                          ;   program, waited for: its exit code (hylang's play)
            lda         dhi,x
            sta         play_times + 1
            inx
            jsr         str_wt                              ; The path: w, tmp characters
            lda         tmp
            cmp         #200
            bcc         :+
            lda         #E_NAMETOOLONG
            jmp         throw_os
:
            ldy         #0                                  ; play, and -l for 0 times
:
            lda         s_play,y
            beq         :+
            sta         play_buf,y
            iny
            bra         :-
:
            sty         play_len
            lda         play_times
            ora         play_times + 1
            bne         :+
            lda         #<s_loop
            ldy         #>s_loop
            jsr         play_add
:
            ldy         #0                                  ; The path
:
            cpy         tmp
            beq         :+
            lda         (w),y
            jsr         play_char
            iny
            bra         :-
:
            lda         play_times + 1                      ; Its loop times - 1 more times (2 or more)
            bne         :+
            lda         play_times
            cmp         #2
            bcc         play_run
:
            lda         #' '
            jsr         play_char
            lda         play_times
            sec
            sbc         #1
            pha
            lda         play_times + 1
            sbc         #0
            tay
            pla
            PUSHAY
            jsr         u_text                              ; ( u -- c-addr u )
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            inx
            inx
            ldy         #0
:
            cpy         tmp
            beq         play_run
            lda         (w),y
            jsr         play_char
            iny
            bra         :-
play_run:
            lda         #<play_buf                          ; Run, as run does
            ldy         #>play_buf
            PUSHAY
            lda         play_len
            ldy         #0
            PUSHAY
            jsr         prog_args
            lda         #0
            jsr         prog_spawn
            bcc         :+
            jmp         throw_os
:
            jsr         prog_wait
            lda         tmp2
            ldy         tmp2 + 1
            PUSHAY
            rts

            HEADER      "snd-mml", 0
sndmml:                                                     ; ( ch c-addr u -- status ): a line of MML (the score
            lda         #<s_m                               ;   language's) on channel ch, its own instrument if it
            ldy         #>s_m                               ;   names none, played to its end (play -m): play's
            bra         snd_line                            ;   exit code

            HEADER      "snd-chord", 0
sndchord:                                                   ; ( ch c-addr u -- status ): its notes at once, a channel
            lda         #<s_c                               ;   each from ch, with the commands before each (play -c)
            ldy         #>s_c
; play, the flag at .A/.Y, the channel, the MML: run
snd_line:
            sta         line_flag
            sty         line_flag + 1
            jsr         str_wt                              ; ( ch c-addr u -- ch ): the MML
            lda         tmp
            cmp         #200
            bcc         :+
            lda         #E_NAMETOOLONG
            jmp         throw_os
:
            sta         line_len
            lda         w
            sta         line_text
            lda         w + 1
            sta         line_text + 1
            stz         play_len
            lda         #<s_play
            ldy         #>s_play
            jsr         play_add
            lda         line_flag
            ldy         line_flag + 1
            jsr         play_add
            jsr         u_text                              ; ( ch -- c-addr u ): its channel
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            inx
            inx
            ldy         #0
:
            cpy         tmp
            beq         :+
            lda         (w),y
            jsr         play_char
            iny
            bra         :-
:
            lda         #' '
            jsr         play_char
            lda         line_text                           ; The MML
            sta         w
            lda         line_text + 1
            sta         w + 1
            ldy         #0
:
            cpy         line_len
            beq         :+
            lda         (w),y
            jsr         play_char
            iny
            bra         :-
:
            jmp         play_run

; play_buf: string .A/.Y (zero-terminated) added; .A, a character added.  Keep .Y
play_add:
            sta         p1
            sty         p1 + 1
            phy
            ldy         #0
:
            lda         (p1),y
            beq         :+
            jsr         play_char
            iny
            bra         :-
:
            ply
            rts

play_char:
            phy
            ldy         play_len
            sta         play_buf,y
            inc         play_len
            ply
            rts

s_play:     .byte       "play ", 0
s_m:        .byte       "-m ", 0
s_c:        .byte       "-c ", 0
s_loop:     .byte       "-l ", 0
s_snd:      .byte       "/dev/snd", 0
s_sndctl:   .byte       "/dev/sndctl", 0
s_reset:    .byte       "reset", 0
s_volume:   .byte       "volume", 0
s_claim:    .byte       "claim", 0
s_release:  .byte       "release", 0

; ---- Notes by name, and tunes (hylang's note-of and tune)

            HEADER      "note-of", 0
noteof:                                                     ; ( c-addr u -- n ): a note's MIDI number, by its name: a
            jsr         str_wt                              ;   letter, # or b, and an octave (-1 to 9): C4 60 (middle
            jsr         note_name                           ;   C), C#4 and Db4 61, A4 69 (440 Hz); not a note: THROW
            bcs         bad_tune                            ;   -24
            ldy         #0
            PUSHAY
            rts

; Not a note, or a tune that isn't one: THROW -24 (invalid numeric argument)
bad_tune:
            lda         #<-24
            jmp         throw_a

            HEADER      "tune", 0
tune:                                                       ; ( c-addr u ch tempo -- ): the tune played on channel ch,
            lda         #<12000                             ;   tempo beats a minute: notes and their beats, blanks
            ldy         #>12000                             ;   between (C4 1 E4 1 G4 2: a note's name as note-of's,
            PUSHAY                                          ;   or - a rest; beats 1-255), each keyed on, then off
            jsr         swap                                ;   as its time ends; Ctrl-C ends it (the note off)
            dex
            stz         dlo,x
            stz         dhi,x
            jsr         swap                                ; ( c-addr u ch 12000 0 tempo -- ... ticks a beat )
            jsr         ummod
            lda         dlo,x
            sta         tune_tpb
            lda         dhi,x
            sta         tune_tpb + 1
            inx
            inx
            lda         dlo,x
            sta         tune_ch
            inx
            jsr         str_wt                              ; The tune: p1, tune_left bytes
            lda         w
            sta         p1
            lda         w + 1
            sta         p1 + 1
            lda         tmp
            sta         tune_left
@pair:
            stz         tune_on                             ; Its note, or a rest
            jsr         tune_word
            bcc         :+
            rts
@bad:
            jmp         bad_tune
:
            lda         tmp
            cmp         #1
            bne         :+
            lda         (w)
            cmp         #'-'
            beq         @beats
:
            jsr         note_name
            bcs         @bad
            sta         tune_note
            inc         tune_on
@beats:
            jsr         tune_word                           ; Its beats (decimal, 1-255)
            bcs         @bad
            ldy         #0
            sty         tune_beats
:
            lda         (w),y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @bad
            pha
            lda         tune_beats
            asl
            asl
            clc
            adc         tune_beats
            asl
            sta         tune_beats
            pla
            clc
            adc         tune_beats
            sta         tune_beats
            iny
            cpy         tmp
            bne         :-
            lda         tune_on                             ; Keyed on ...
            beq         :+
            jsr         tune_push
            lda         tune_note
            ldy         #0
            PUSHAY
            jsr         sndnote
:
            lda         tune_beats                          ;   its time (its beats' ticks, 32767 at most) ...
            ldy         #0
            PUSHAY
            lda         tune_tpb
            ldy         tune_tpb + 1
            PUSHAY
            jsr         umstar
            inx
            lda         dhi,x
            and         #$7F
            sta         tmp + 1
            lda         dlo,x
            sta         tmp
            inx
            stx         xsave
            lda         tmp
            ldx         tmp + 1
            jsr         SLEEP
            ldx         xsave
            lda         tune_on                             ;   and off (a note ending the wait too)
            beq         :+
            jsr         tune_push
            jsr         sndoff
:
            bit         intr                                ; (Ctrl-C: the end, THROW -28)
            bvs         :+
            jmp         @pair
:
            jmp         intr_throw

; ( -- ch ): the tune's channel pushed
tune_push:
            lda         tune_ch
            ldy         #0
            PUSHAY
            rts

; w and tmp: the next word of the tune (p1, tune_left bytes left).  OUT: C = 1, none left
tune_word:
@skip:
            lda         tune_left
            beq         @none
            lda         (p1)
            cmp         #' ' + 1
            bcs         @word
            jsr         @next
            bra         @skip
@word:
            lda         p1
            sta         w
            lda         p1 + 1
            sta         w + 1
            stz         tmp
:
            lda         tune_left
            beq         :+
            lda         (p1)
            cmp         #' ' + 1
            bcc         :+
            inc         tmp
            jsr         @next
            bra         :-
:
            clc
            rts
@none:
            sec
            rts
@next:
            inc         p1
            bne         :+
            inc         p1 + 1
:
            dec         tune_left
            rts

; ( c-addr u -- ): w and tmp the string (255 chars at most)
str_wt:
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dhi,x
            beq         :+
            lda         #255
            bra         :++
:
            lda         dlo,x
:
            sta         tmp
            inx
            inx
            rts

; The note named at w (tmp chars): .A its MIDI number (0-127).  OUT: C = 1, not a note
note_name:
            ldy         #0
            cpy         tmp
            beq         @bad
            lda         (w),y                               ; Its letter: its semitone in the octave
            and         #$DF
            sec
            sbc         #'A'
            cmp         #7
            bcs         @bad
            phy
            tay
            lda         semitones,y
            ply
            sta         tmp2
            iny
            cpy         tmp
            beq         @bad
            lda         (w),y                               ; # or b
            cmp         #'#'
            bne         :+
            inc         tmp2
            iny
            bra         @octave
:
            cmp         #'b'
            bne         @octave
            dec         tmp2
            iny
@octave:
            stz         tmp2 + 1                            ; Its octave, -1 to 9
            cpy         tmp
            beq         @bad
            lda         (w),y
            cmp         #'-'
            bne         :+
            inc         tmp2 + 1
            iny
            cpy         tmp
            beq         @bad
            lda         (w),y
:
            sec
            sbc         #'0'
            cmp         #10
            bcs         @bad
            iny
            cpy         tmp
            bne         @bad
            ldy         tmp2 + 1
            beq         :+
            cmp         #1                                  ; (-1: octave 0's start, 0)
            bne         @bad
            lda         #0
            bra         @sum
:
            inc                                             ; ((octave + 1) * 12)
            asl
            asl
            sta         tmp3
            asl
            clc
            adc         tmp3
@sum:
            clc
            adc         tmp2                                ; (A flat C: one less; past 127, or less than 0: none)
            cmp         #128
            bcs         @bad
            clc
            rts
@bad:
            sec
            rts

semitones:  .byte       9, 11, 0, 2, 4, 5, 7                ; A B C D E F G
