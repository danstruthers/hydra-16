; ****************************************************************************
; play [-l] song [n] - a song (a ZSM file: the Commander X16's format, which Furnace exports) on the YM2151, through
; /dev/snd: to its end once; with n, its loop n more times; -l, its loop till it's stopped (Ctrl-C).  The old
; system's player (os_rom/sound/player.s), a program now.  A song whose name ends in .mml is a score (hysong.js's
; language: mml.inc), compiled as it plays into the stream hysong.js would make of it; play -o score.mml song.zsm
; writes that stream to a ZSM file instead.
;   The header (16 bytes): "zm", a version, the loop point (3 bytes: an offset in the file; 0: none, and a loop is
; the whole song), the PCM table's (ignored), the FM channels it uses (claimed: /dev/sndctl's claim), the PSG's
; (ignored), the tick rate (Hz; 0: 60), 2 reserved.  Then the stream: $00-$3F a PSG write (skipped: the Hydra has
; no PSG), $40 an extension (skipped), $41-$7F n register/value pairs, $80 the end (or the loop), $81-$FF a delay
; of n ticks.
;   Each tick's pairs go to /dev/snd in one write (another program's can't come between them); then it sleeps to
; the next tick by the system's tick (TICK_HZ a second: SLEEP_UNTIL), keeping a fraction, so the tempo is exact on
; average, if not each tick.  It holds the CPU for the song (PREEMPT_OFF): a task switch comes only as it sleeps or
; waits, so a tick's work isn't cut in two by another task's slice.  It reads the file ahead as it waits, 256
; bytes at a time.  Its end, or Ctrl-C, closes /dev/snd, which gives its channels back, keyed off.
;   Its status: none; "usage"; the song's error ("play: song: why"); "not a song", "no sound", "channels busy"; a
; score's error ("channel 2: a note before an instrument" ...).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "play", main

            .import     patches, drum_patch, drum_kc, volume_atten

HDR_SIZE        = 16            ; The header ...
H_LOOP          = 3             ;   its loop point (3) ...
H_FM            = 9             ;   the FM channels ...
H_RATE          = 12            ;   the tick rate (2)
FRAME_MAX       = 254           ; A tick's pairs, a write's at most (127 of them)
FOREVER         = $FF           ; loops: the loop till it's stopped
F_LOOP          = $01           ; -l
F_OUT           = $02           ; -o

.zeropage
next:       .res        4                                   ; The next tick's time: a fraction (2), then the tick
period:     .res        3                                   ; A song tick in system ticks (8.16 fixed point)
in_left:    .res        2                                   ; inbuf: the bytes left ...
in_pos:     .res        1                                   ;   and the next one's place
staged:     .res        2                                   ; stage: the bytes read ahead (0: none yet)
t:          .res        2                                   ; Scratch

.bss
song:       .res        2                                   ; The song's name (its argument)
why:        .res        2                                   ; What's wrong (fail's)
fd:         .res        1                                   ; The song's fd ...
snd:        .res        1                                   ;   /dev/snd's ...
ctl:        .res        1                                   ;   and /dev/sndctl's
loops:      .res        1                                   ; The loop's times to come (FOREVER: forever) ...
rloops:     .res        1                                   ;   and the read ahead's count of them
eof:        .res        1                                   ; <> 0: the file's end (or an error), read ahead
out:        .res        1                                   ; frame's bytes
loop:       .res        3                                   ; The loop point
hdr:        .res        HDR_SIZE
words:      .res        12                                  ; sndctl's claim
inbuf:      .res        256                                 ; The file, played from here ...
stage:      .res        256                                 ;   and read ahead into here
frame:      .res        FRAME_MAX                           ; A tick's register pairs

.code
main:
            jsr         tl_start
            lda         (tl_arg)                            ; The song
            bne         :+
            jmp         tl_badusage
:
            MOVR        song, tl_arg
            jsr         mml_name
            stz         loops
            lda         tl_flags                            ; -o score song: the score compiled into a file
            and         #F_OUT
            beq         @times
            lda         mml
            beq         @usage
            jsr         tl_next
            beq         @usage
            jsr         open_song
            jsr         mml_load
            jmp         mml_out
@usage:
            jmp         tl_badusage
@times:
            jsr         tl_next                             ; n: the loop's times more
            beq         @args
            MOVR        r0, tl_arg
            jsr         tl_atoi
            bcc         :+
            jmp         tl_badusage
:
            lda         tl_num + 1
            ora         tl_num + 2
            ora         tl_num + 3
            bne         :+
            lda         tl_num
            cmp         #FOREVER
            bcc         :++
:
            lda         #FOREVER - 1                        ; (254 at most: 255 is forever)
:
            sta         loops
            jsr         tl_next
            beq         @args
            jmp         tl_badusage

@args:
            lda         tl_flags
            and         #F_LOOP
            beq         :+
            lda         #FOREVER
            sta         loops
:
            jsr         open_song                           ; The song, and its header (a score: read whole)
            lda         mml
            beq         :+
            jsr         mml_load
            lda         m_mask
            sta         hdr + H_FM
            stz         loop + 1
            stz         loop + 2
            lda         #HDR_SIZE
            sta         loop
            bra         @sound
:
            LDR         r0, hdr
            LDR         r1, HDR_SIZE
            lda         fd
            jsr         READ
            bcs         not_song
            cmp         #HDR_SIZE
            bne         not_song
            lda         hdr
            cmp         #'z'
            bne         not_song
            lda         hdr + 1
            cmp         #'m'
            bne         not_song
            ldx         #2                                  ; The loop point (none: the stream's start)
:
            lda         hdr + H_LOOP,X
            sta         loop,X
            dex
            bpl         :-
            lda         loop
            ora         loop + 1
            ora         loop + 2
            bne         :+
            lda         #HDR_SIZE
            sta         loop
:
@sound:
            LDR         r0, s_snd                           ; The sound driver's files
            lda         #O_WRITE
            jsr         OPEN
            bcs         no_sound
            sta         snd
            LDR         r0, s_sndctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         no_sound
            sta         ctl
            jsr         claim                               ; Its channels, its alone
            bcs         busy
            stz         out
            jsr         start                               ; (The song's start read before its time starts)
            jsr         rate                                ; A song tick: period
            jsr         PREEMPT_OFF                         ; (The CPU held: a switch only as it sleeps)
            jsr         TICKS                               ; The first tick: the system's next but one, so
            clc                                             ;   every song tick is timed from the start of one
            adc         #2
            bcc         :+
            inx
:
            sta         next + 2
            stx         next + 3
            jsr         SLEEP_UNTIL
            stz         next                                ; (The fraction from a half: each time rounds to the
            lda         #$80                                ;   nearest system tick, not down)
            sta         next + 1
            jmp         stream

not_song:
            LDR         r0, s_notsong
            bra         fail

no_sound:
            LDR         r0, s_nosound
            bra         fail

busy:
            LDR         r0, s_busy
; "play: song: why" on fd 2 (why at r0), and the end, why its status
fail:
            MOVR        why, r0
            jsr         tl_flush
            LDR         r0, s_name
            jsr         puts2
            MOVR        r0, song
            jsr         puts2
            LDR         r0, s_colon
            jsr         puts2
            MOVR        r0, why
            jsr         puts2
            LDR         r0, s_nl
            jsr         puts2
            MOVR        r0, why
            lda         #1
            jmp         EXITS

; The string at r0 on fd 2
puts2:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            iny
            bne         :-
:
            sty         r1
            stz         r1 + 1
            lda         #2
            jmp         WRITE

; The song (its name at song) opened: fd.  Not: its error, and the end
open_song:
            MOVR        r0, song
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            pha                                             ; (MOVR changes .A)
            MOVR        r0, song
            pla
            jsr         tl_err
            jmp         tl_end
:
            sta         fd
            rts

; sndctl's "claim $NN": the song's FM channels.  OUT: C = 0; or C = 1: another program has one
claim:
            ldx         #0
:
            lda         s_claim,X
            sta         words,X
            beq         :+
            inx
            bra         :-
:
            lda         hdr + H_FM
            lsr
            lsr
            lsr
            lsr
            jsr         @hex
            lda         hdr + H_FM
            jsr         @hex
            LDR         r0, words
            stx         r1
            stz         r1 + 1
            lda         ctl
            jmp         WRITE

@hex:
            and         #$0F
            ora         #'0'
            cmp         #'9' + 1
            bcc         :+
            adc         #'a' - '9' - 2                      ; (C = 1)
:
            sta         words,X
            inx
            rts

; ****************************************************************************
; The stream

; The stream, to its end (and its loop, as many times as loops says): then the end (its fds closed, so its channels
; keyed off)
stream:
            jsr         byte
            bcs         @end                                ; (The file's end)
            cmp         #$40
            bcc         @psg
            beq         @ext
            cmp         #$80
            bcc         @fm
            beq         @eof
            and         #$7F                                ; A delay: n ticks
            jsr         delay
            bra         stream

@psg:                                                       ; A PSG write: its value, skipped
            jsr         byte
            bra         stream

@ext:                                                       ; An extension: its bytes, skipped
            jsr         byte
            bcs         @end
            and         #$3F
            tax
            beq         stream
:
            jsr         byte
            bcs         @end
            dex
            bne         :-
            bra         stream

@fm:                                                        ; n register/value pairs: gathered for the tick
            and         #$3F
            tax
@pair:
            lda         out
            cmp         #FRAME_MAX
            bcc         :+
            jsr         flush
:
            jsr         byte
            bcs         @end
            ldy         out
            sta         frame,Y
            jsr         byte
            bcs         @end
            ldy         out
            sta         frame + 1,Y
            iny
            iny
            sty         out
            dex
            bne         @pair
            bra         stream

@eof:                                                       ; The end: the loop again?
            lda         loops
            beq         @end
            cmp         #FOREVER
            beq         stream
            dec         loops
            bra         stream                              ; (The read ahead went on into the loop already)

@end:
            jsr         flush
            jmp         tl_end

; The song's next byte (inbuf filled from what was read ahead as it runs out, or from the file, waiting for it, if
; nothing was).  OUT: C = 0, .A = the byte; or C = 1: the file's end (or an error).  Keeps .X, .Y
byte:
            lda         in_left
            ora         in_left + 1
            bne         @have
            phx
            phy
            lda         staged
            ora         staged + 1
            bne         :+
            jsr         stage_read                          ; (Nothing read ahead: read it now)
            lda         staged
            ora         staged + 1
            beq         @none
:
            jsr         unstage
            ply
            plx
@have:
            lda         in_left
            bne         :+
            dec         in_left + 1
:
            dec         in_left
            phy
            ldy         in_pos
            lda         inbuf,Y
            inc         in_pos
            ply
            clc
            rts

@none:
            ply
            plx
            sec
            rts

; The song from the file's offset as it is (just past the header): nothing played yet; its first 512 bytes read
; before its time starts (a song's first tick sets its voices up)
start:
            stz         in_left
            stz         in_left + 1
            stz         staged
            stz         staged + 1
            stz         eof
            lda         loops
            sta         rloops
            lda         mml                                 ; (A score: compiled ahead, the ring full)
            beq         :++
            jsr         mml_start
:
            jsr         m_step
            bcc         :-
:
            jsr         stage_read
            jsr         unstage
            jmp         stage_read

; What was read ahead (stage) into inbuf, which is empty
unstage:
            ldx         #0
:
            lda         stage,X
            sta         inbuf,X
            inx
            bne         :-
            MOVR        in_left, staged
            stz         in_pos
            stz         staged
            stz         staged + 1
            rts

; Read ahead into stage, 256 bytes, if nothing's there yet; at the file's end, on from the loop point if the loop
; is to be played again (rloops: so the stream finds it there after its $80, with no wait for a seek and a read).
; OUT: staged (0: the end); eof at the file's end, or an error
stage_read:
            lda         mml
            beq         :+
            jmp         mml_stage
:
            lda         staged
            ora         staged + 1
            ora         eof
            bne         @done
            LDR         r0, stage
            LDR         r1, 256
            lda         fd
            jsr         READ
            bcs         @end
            sta         staged
            stx         staged + 1
            ora         staged + 1
            bne         @done
            lda         rloops                              ; The file's end: on at the loop point, if it's
            beq         @end                                ;   played again
            cmp         #FOREVER
            beq         :+
            dec         rloops
:
            lda         loop                                ; (SEEK from the start: r0, r1 the offset)
            sta         r0
            lda         loop + 1
            sta         r0 + 1
            lda         loop + 2
            sta         r1
            stz         r1 + 1
            ldx         #0
            lda         fd
            jsr         SEEK
            bcc         stage_read
@end:
            lda         #1
            sta         eof
@done:
            rts

; The tick's register pairs to /dev/snd (one write), if there are any.  Keeps .X
flush:
            lda         out
            beq         @done
            phx
            sta         r1
            stz         r1 + 1
            LDR         r0, frame
            lda         snd
            jsr         WRITE
            stz         out
            plx
@done:
            rts

; A delay of .A song ticks (1-127): the tick's writes out, the file read ahead, then a sleep till the time n song
; ticks after the last one (next += period, n times)
delay:
            tax
            jsr         flush
:
            clc
            lda         next
            adc         period
            sta         next
            lda         next + 1
            adc         period + 1
            sta         next + 1
            lda         next + 2
            adc         period + 2
            sta         next + 2
            lda         next + 3
            adc         #0
            sta         next + 3
            dex
            bne         :-
            jsr         stage_read                          ; (Read ahead: this is the time for it)
            lda         mml                                 ; (A score: compiled ahead till the tick before)
            beq         :+
            sec
            lda         next + 2
            sbc         #1
            sta         m_tick
            lda         next + 3
            sbc         #0
            sta         m_tick + 1
            jsr         mml_idle
:
            lda         next + 2
            ldx         next + 3
            jmp         SLEEP_UNTIL                         ; (A time that's passed: at once)

; A song tick in system ticks, 8.16 fixed point: period = TICK_HZ * 65536 / the rate (0: 60 Hz)
rate:
            lda         hdr + H_RATE
            sta         t
            lda         hdr + H_RATE + 1
            sta         t + 1
            ora         t
            bne         :+
            lda         #60
            sta         t
:
            stz         period                              ; The dividend, shifted out as the quotient's shifted
            stz         period + 1                          ;   in ...
            lda         #TICK_HZ
            sta         period + 2
            stz         next                                ;   and the remainder (17 bits: next isn't set yet)
            stz         next + 1
            stz         next + 2
            ldx         #24
@bit:
            asl         period
            rol         period + 1
            rol         period + 2
            rol         next
            rol         next + 1
            rol         next + 2
            lda         next
            sec
            sbc         t
            tay
            lda         next + 1
            sbc         t + 1
            pha
            lda         next + 2
            sbc         #0
            bcc         @less
            sta         next + 2
            pla
            sta         next + 1
            sty         next
            inc         period
            bra         @next

@less:
            pla
@next:
            dex
            bne         @bit
            rts

.rodata
s_snd:      .byte       "/dev/snd", 0
s_sndctl:   .byte       "/dev/sndctl", 0
s_claim:    .byte       "claim $", 0
s_notsong:  .byte       "not a song", 0
s_nosound:  .byte       "no sound", 0
s_busy:     .byte       "channels busy", 0
s_name:     .byte       "play: ", 0
s_colon:    .byte       ": ", 0
s_nl:       .byte       LF, 0
tl_name:    .byte       "play", 0
tl_flagset: .byte       "lo", 0
tl_usage:   .byte       "play [-l] song [n]; play -o score.mml song.zsm", 0

.include "mml.inc"
.include "toollib.s"
