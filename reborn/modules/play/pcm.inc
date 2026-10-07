; ****************************************************************************
; pcm.inc - play's second bank (CODE2, RODATA2): PCM, through the Vera X's /dev/vid/pcm and pcmctl (vid's).
;   wav_play: a WAV file (RIFF: its fmt chunk, PCM of 8 or 16 bits, mono or stereo, at a rate up to 48,828 Hz, the
; VERA's nearest; its other chunks skipped), its data written to /dev/vid/pcm (8 bits made signed, as the VERA takes
; them; a write waits as the FIFO fills), as many times as loops says; then pcmctl's drain.
;   zpcm_open, zpcm_cmds, zpcm_feed: a ZSM's PCM instruments (the X16's reference, appendix G): its PCM table, at the
; header's offset ("PCM", the last instrument's index, then 16 bytes each: its index, its format (AUDIO_CTRL's bits
; 5-4: 16 bits, stereo), its offset in the PCM data after the table, its length (3 bytes each), its features (bit 7:
; it loops), its loop point (3)); the stream's extension commands on channel 0, in pairs (0 AUDIO_CTRL: bits 3-0 the
; volume, bit 7 the FIFO emptied; 1 AUDIO_RATE, as Hz to pcmctl; 2 an instrument started); and the instrument's
; bytes written to /dev/vid/pcm (non-blocking) each song tick (delay's): from RAM, as much as the FIFO takes, when the
; PCM data fits the RAM past the BSS (BREAK: some 20K; read before the song's time starts, as the card's too slow to
; read as it plays); or else from the song's file on an fd of its own, a part of PBUF a tick at most.  System calls alone here: the first bank's
; routines would be FAR1's.

.macpack longbranch

PBUF            = 1024          ; pbuf: samples from the file, to /dev/vid/pcm
ITAB_MAX        = 128           ; A ZSM's PCM instruments read (the first so many)

.pushseg
.bss
pfd:        .res        1                                   ; /dev/vid/pcm's fd ...
pctl:       .res        1                                   ;   pcmctl's ...
pfd2:       .res        1                                   ;   the song's again (its instruments' samples) ...
pmode:      .res        1                                   ;   and pcm's mode (O_NONBLOCK: a ZSM's)
zok:        .res        1                                   ; <> 0: a ZSM's PCM instruments playable
zon:        .res        1                                   ; <> 0: one playing ...
zloops:     .res        1                                   ;   <> 0: it loops
sleft:      .res        4                                   ; The bytes left (a WAV file's data's, an instrument's)
sloop:      .res        4                                   ; Where its loop starts in the file (a WAV file's data's
                                                            ;   start; an instrument's loop point) ...
slen:       .res        4                                   ;   and its bytes from there
wpos:       .res        4                                   ; A WAV file's chunk's place ...
wchunk:     .res        8                                   ;   its id and size ...
wfmt:       .res        16                                  ;   and fmt's: tag, channels, rate (4), bytes a second
                                                            ;   (4), a sample's bytes, its bits
zbase:      .res        4                                   ; A ZSM's PCM data's place ...
zlast:      .res        1                                   ;   its last instrument in itab ...
zrec:       .res        16                                  ;   the one starting ...
zram:       .res        1                                   ;   <> 0: its data in RAM, at zbuf ...
zbuf:       .res        2
zext:       .res        3                                   ;   its size (the instruments' furthest end) ...
zat:        .res        2                                   ;   from RAM: the next byte ...
zleft:      .res        2                                   ;     the bytes left ...
zloopat:    .res        2                                   ;     where its loop starts ...
zlooplen:   .res        2                                   ;     and its bytes from there
pgeo:       .res        1                                   ; pcm_format's format
pnflag:     .res        1                                   ; psay's: a number after the word ...
plen:       .res        1                                   ;   the line's length so far ...
pnum:       .res        2                                   ;   the number
pv:         .res        1                                   ; Scratch
pbn:        .res        2                                   ; pbuf's bytes ...
pbat:       .res        2                                   ;   and those written
pline:      .res        24                                  ; A pcmctl line
pbuf:       .res        PBUF
itab:       .res        ITAB_MAX * 16                       ; A ZSM's PCM instruments
.popseg

; ****************************************************************************
; WAV files

; A WAV file played (its first 16 bytes in hdr, the file just past them): its chunks looked through for fmt and
; data, the PCM's format and rate set, its data written to /dev/vid/pcm as many times as loops says, then drained.
; OUT: C = 0 (played); or C = 1, .A = W_NOTSONG (not a WAV file of PCM it plays), W_NOSOUND (no Vera X), W_BUSY
; (another program has the PCM)
wav_play:
            ldx         #2                                  ; "RIFF" (the first bank saw the R), "WAVE"
:
            lda         hdr + 1,X
            cmp         ps_iff,X
            jne         @not
            dex
            bpl         :-
            ldx         #3
:
            lda         hdr + 8,X
            cmp         ps_wave,X
            jne         @not
            dex
            bpl         :-
            stz         wfmt                                ; (No fmt chunk yet: its tag 0)
            stz         wfmt + 1
            lda         #12                                 ; The first chunk: at 12, its id read already ...
            sta         wpos
            stz         wpos + 1
            stz         wpos + 2
            stz         wpos + 3
            ldx         #3
:
            lda         hdr + 12,X
            sta         wchunk,X
            dex
            bpl         :-
            LDR         r0, wchunk + 4                      ;   its size
            LDR         r1, 4
            lda         fd
            jsr         READ
            jcs         @not
            cmp         #4
            beq         @chunk
@not:
            lda         #W_NOTSONG
            sec
            rts

@chunk:
            ldx         #3                                  ; fmt: its first 16 bytes
:
            lda         wchunk,X
            cmp         ps_fmt,X
            bne         @data
            dex
            bpl         :-
            LDR         r0, wfmt
            LDR         r1, 16
            lda         fd
            jsr         READ
            jcs         @not
            cmp         #16
            jne         @not
            bra         @next

@data:
            ldx         #3
:
            lda         wchunk,X
            cmp         ps_data,X
            bne         @next
            dex
            bpl         :-
            jmp         wav_data

@next:
            clc                                             ; The next chunk: its size, 8 and a byte if the size
            lda         wpos                                ;   is odd on
            adc         wchunk + 4
            sta         wpos
            lda         wpos + 1
            adc         wchunk + 5
            sta         wpos + 1
            lda         wpos + 2
            adc         wchunk + 6
            sta         wpos + 2
            lda         wpos + 3
            adc         wchunk + 7
            sta         wpos + 3
            lda         wchunk + 4
            and         #1
            ora         #8
            jsr         wpos_add
            lda         wpos                                ; (SEEK from the start: r0, r1 the offset)
            sta         r0
            lda         wpos + 1
            sta         r0 + 1
            lda         wpos + 2
            sta         r1
            lda         wpos + 3
            sta         r1 + 1
            ldx         #0
            lda         fd
            jsr         SEEK
            jcs         @not
            LDR         r0, wchunk
            LDR         r1, 8
            lda         fd
            jsr         READ
            jcs         @not
            cmp         #8
            jne         @not
            jmp         @chunk

; wav_play's data chunk found (wpos its place, wchunk its header): its fmt one we play?  Then played
wav_data:
            lda         wfmt                                ; PCM (1), 1 or 2 channels, a rate under 65,536, 8 or
            cmp         #1                                  ;   16 bits
            jne         @not
            lda         wfmt + 1
            ora         wfmt + 3
            ora         wfmt + 6
            ora         wfmt + 7
            ora         wfmt + 15
            jne         @not
            lda         wfmt + 2
            jeq         @not
            cmp         #3
            jcs         @not
            lda         wfmt + 14
            cmp         #8
            beq         :+
            cmp         #16
            jne         @not
:
            lda         #8                                  ; The data: at wpos + 8, its size's bytes
            jsr         wpos_add
            ldx         #3
:
            lda         wpos,X
            sta         sloop,X
            lda         wchunk + 4,X
            sta         slen,X
            dex
            bpl         :-
            lda         #O_WRITE
            sta         pmode
            jsr         pcm_open
            jcs         @fail
            lda         #0                                  ; Its format, then its rate
            ldx         wfmt + 14
            cpx         #16
            bne         :+
            lda         #$20
:
            ldx         wfmt + 2
            cpx         #2
            bne         :+
            ora         #$10
:
            jsr         pcm_format
            lda         wfmt + 4
            sta         pnum
            lda         wfmt + 5
            sta         pnum + 1
            lda         #<ps_rate
            ldx         #>ps_rate
            ldy         #1
            jsr         psay
@again:
            lda         fd                                  ; The data from its start
            jsr         sseek
            ldx         #3
:
            lda         slen,X
            sta         sleft,X
            dex
            bpl         :-
@feed:
            lda         fd
            jsr         sread
            bcs         @end
            lda         wfmt + 14                           ; (8 bits: made signed)
            cmp         #8
            bne         :+
            jsr         signed
:
            LDR         r0, pbuf
            lda         pbn
            sta         r1
            lda         pbn + 1
            sta         r1 + 1
            lda         pfd
            jsr         WRITE
            bcc         @feed
            bra         @drain                              ; (An error: no more)

@end:
            lda         loops                               ; Its end: again, as many times as loops says
            beq         @drain
            cmp         #FOREVER
            beq         @again
            dec         loops
            bra         @again

@drain:
            lda         #<ps_drain                          ; Played: when the FIFO's empty
            ldx         #>ps_drain
            ldy         #0
            jsr         psay
            clc
            rts

@not:
            lda         #W_NOTSONG
@fail:
            sec
            rts

; wpos += .A
wpos_add:
            clc
            adc         wpos
            sta         wpos
            bcc         :+
            inc         wpos + 1
            bne         :+
            inc         wpos + 2
            bne         :+
            inc         wpos + 3
:
            rts

; pbuf's samples, 8-bit unsigned (a WAV file's), made signed (the VERA's)
signed:
            ldy         #0
:
            lda         pbuf,Y
            eor         #$80
            sta         pbuf,Y
            lda         pbuf + 256,Y
            eor         #$80
            sta         pbuf + 256,Y
            lda         pbuf + 512,Y
            eor         #$80
            sta         pbuf + 512,Y
            lda         pbuf + 768,Y
            eor         #$80
            sta         pbuf + 768,Y
            iny
            bne         :-
            rts
.assert     PBUF = 1024, error, "signed: pbuf's 4 pages"

; ****************************************************************************
; A ZSM's PCM instruments

; Made playable (zok): the song's file opened again (pfd2), its PCM table read (itab: the first ITAB_MAX
; instruments), /dev/vid/pcm (non-blocking) and pcmctl opened, the FIFO emptied.  No Vera X, or a table that isn't
; one: the song's PCM dropped (zok 0).  OUT: C = 0; or C = 1: another program has the PCM
zpcm_open:
            stz         zok
            stz         zon
            MOVR        r0, song
            lda         #O_READ
            jsr         OPEN
            jcs         @none
            sta         pfd2
            lda         hdr + H_PCM                         ; The table: "PCM", the last instrument's index ...
            sta         r0
            lda         hdr + H_PCM + 1
            sta         r0 + 1
            lda         hdr + H_PCM + 2
            sta         r1
            stz         r1 + 1
            ldx         #0
            lda         pfd2
            jsr         SEEK
            jcs         @none
            LDR         r0, wchunk
            LDR         r1, 4
            lda         pfd2
            jsr         READ
            jcs         @none
            cmp         #4
            jne         @none
            ldx         #2
:
            lda         wchunk,X
            cmp         ps_pcm,X
            jne         @none
            dex
            bpl         :-
            lda         wchunk + 3                          ; The PCM data's place: after the table, (the last + 1)
            clc                                             ;   * 16 + 4 on from it
            adc         #1
            sta         pnum
            lda         #0
            adc         #0
            sta         pnum + 1
            ldx         #4
:
            asl         pnum
            rol         pnum + 1
            dex
            bne         :-
            clc
            lda         hdr + H_PCM
            adc         #4
            sta         zbase
            lda         hdr + H_PCM + 1
            adc         #0
            sta         zbase + 1
            lda         hdr + H_PCM + 2
            adc         #0
            sta         zbase + 2
            clc
            lda         zbase
            adc         pnum
            sta         zbase
            lda         zbase + 1
            adc         pnum + 1
            sta         zbase + 1
            lda         zbase + 2
            adc         #0
            sta         zbase + 2
            stz         zbase + 3
            lda         wchunk + 3                          ;   and the instruments (the first ITAB_MAX)
            cmp         #ITAB_MAX
            bcc         :+
            lda         #ITAB_MAX - 1
:
            sta         zlast
            inc         a
            sta         r1
            stz         r1 + 1
            ldx         #4
:
            asl         r1
            rol         r1 + 1
            dex
            bne         :-
            LDR         r0, itab
            lda         pfd2
            jsr         READ
            jcs         @none
            jsr         zpcm_load                           ; (Its PCM data into RAM, if it fits)
            lda         #O_WRITE | O_NONBLOCK               ; The PCM's files
            sta         pmode
            jsr         pcm_open
            bcc         :+
            cmp         #W_BUSY
            beq         @busy
            bra         @none
:
            lda         #<ps_reset
            ldx         #>ps_reset
            ldy         #0
            jsr         psay
            lda         #1
            sta         zok
@none:
            clc
            rts

@busy:
            sec
            rts

; A ZSM's PCM data (up to the instruments' furthest end) read into the RAM past the BSS, if it fits: zbuf, the break
; moved past it (BREAK); zram.  OUT: zram
zpcm_load:
            stz         zram
            stz         zext
            stz         zext + 1
            stz         zext + 2
            LDR         r2, itab
            ldx         zlast
            inx
@inst:
            clc                                             ; Its end: its offset + its length (zrec, a moment)
            ldy         #2
            lda         (r2),Y
            ldy         #5
            adc         (r2),Y
            sta         zrec
            ldy         #3
            lda         (r2),Y
            ldy         #6
            adc         (r2),Y
            sta         zrec + 1
            ldy         #4
            lda         (r2),Y
            ldy         #7
            adc         (r2),Y
            sta         zrec + 2
            sec                                             ; The furthest so far?
            lda         zext
            sbc         zrec
            lda         zext + 1
            sbc         zrec + 1
            lda         zext + 2
            sbc         zrec + 2
            bcs         :+
            lda         zrec
            sta         zext
            lda         zrec + 1
            sta         zext + 1
            lda         zrec + 2
            sta         zext + 2
:
            clc
            lda         r2
            adc         #16
            sta         r2
            bcc         :+
            inc         r2 + 1
:
            dex
            bne         @inst
            lda         zext + 2                            ; Room past the BSS?  (BREAK says)
            bne         @file
            stz         r0
            stz         r0 + 1
            jsr         BREAK
            bcs         @file
            lda         r0
            sta         zbuf
            clc
            adc         zext
            sta         r0
            lda         r0 + 1
            sta         zbuf + 1
            adc         zext + 1
            sta         r0 + 1
            bcs         @file
            jsr         BREAK
            bcs         @file
            lda         zbase                               ; Then read, from the data's start
            sta         r0
            lda         zbase + 1
            sta         r0 + 1
            lda         zbase + 2
            sta         r1
            lda         zbase + 3
            sta         r1 + 1
            ldx         #0
            lda         pfd2
            jsr         SEEK
            bcs         @file
            MOVR        r0, zbuf
            lda         zext
            sta         r1
            lda         zext + 1
            sta         r1 + 1
            lda         pfd2
            jsr         READ
            bcs         @file
            lda         #1
            sta         zram
@file:
            rts

; A PCM extension's commands (xbuf: xlen bytes, in pairs; an odd last byte dropped): 0 AUDIO_CTRL (bit 7: the FIFO
; emptied, the instrument stopped; bits 3-0 the volume), 1 AUDIO_RATE (pcmctl's rate in Hz: V * 381 + V / 2 - V / 32,
; as 48,828.125 / 128 has it), 2 an instrument started
zpcm_cmds:
            ldx         #0
@pair:
            inx                                             ; (A whole pair left: its value's at .X)
            cpx         xlen
            bcs         @done
            phx
            lda         xbuf - 1,X
            ldy         xbuf,X
            jsr         zpcm_cmd
            plx
            inx
            bra         @pair

@done:
            rts

; Command .A, its value .Y
zpcm_cmd:
            cmp         #1
            bcc         @ctrl
            beq         @rate
            cmp         #2
            bne         @done
            jmp         zpcm_start

@ctrl:
            tya                                             ; AUDIO_CTRL: bit 7, the FIFO emptied (the instrument
            pha                                             ;   stopped) ...
            bpl         :+
            stz         zon
            lda         #<ps_reset
            ldx         #>ps_reset
            ldy         #0
            jsr         psay
:
            pla                                             ;   bits 3-0, the volume
            and         #$0F
            sta         pnum
            stz         pnum + 1
            lda         #<ps_volume
            ldx         #>ps_volume
            ldy         #1
            jmp         psay

@rate:
            tya                                             ; AUDIO_RATE (128 at most) in Hz
            cmp         #129
            bcc         :+
            lda         #128
:
            sta         pv
            stz         pnum
            stz         pnum + 1
            tax
            beq         @hz
:
            clc
            lda         pnum
            adc         #<381
            sta         pnum
            lda         pnum + 1
            adc         #>381
            sta         pnum + 1
            dex
            bne         :-
            lda         pv
            lsr
            clc
            adc         pnum
            sta         pnum
            bcc         :+
            inc         pnum + 1
:
            lda         pv
            lsr
            lsr
            lsr
            lsr
            lsr
            sta         pv
            sec
            lda         pnum
            sbc         pv
            sta         pnum
            bcs         @hz
            dec         pnum + 1
@hz:
            lda         #<ps_rate
            ldx         #>ps_rate
            ldy         #1
            jmp         psay

@done:
            rts

; Instrument .Y started (none in itab: nothing): its format (the FIFO emptied), its bytes from its start, its loop
; (from its loop point to its end, again and again) if it loops
zpcm_start:
            sty         pv
            LDR         r2, itab
            ldx         zlast
            inx
@find:
            lda         (r2)
            cmp         pv
            beq         @found
            clc
            lda         r2
            adc         #16
            sta         r2
            bcc         :+
            inc         r2 + 1
:
            dex
            bne         @find
            rts

@found:
            ldy         #15
:
            lda         (r2),Y
            sta         zrec,Y
            dey
            bpl         :-
            lda         zrec + 1                            ; Its format, the FIFO emptied
            and         #$30
            jsr         pcm_format
            clc                                             ; Its start (in sloop a moment) and length
            lda         zbase
            adc         zrec + 2
            sta         sloop
            lda         zbase + 1
            adc         zrec + 3
            sta         sloop + 1
            lda         zbase + 2
            adc         zrec + 4
            sta         sloop + 2
            lda         zbase + 3
            adc         #0
            sta         sloop + 3
            lda         zrec + 5
            sta         sleft
            lda         zrec + 6
            sta         sleft + 1
            lda         zrec + 7
            sta         sleft + 2
            stz         sleft + 3
            lda         pfd2
            jsr         sseek
            stz         zloops                              ; Its loop: from its loop point (under its length) to
            lda         zrec + 8                            ;   its end
            bpl         @ram
            sec
            lda         zrec + 5
            sbc         zrec + 9
            sta         slen
            lda         zrec + 6
            sbc         zrec + 10
            sta         slen + 1
            lda         zrec + 7
            sbc         zrec + 11
            sta         slen + 2
            stz         slen + 3
            bcc         @ram
            ora         slen
            ora         slen + 1
            beq         @ram
            clc
            lda         sloop
            adc         zrec + 9
            sta         sloop
            lda         sloop + 1
            adc         zrec + 10
            sta         sloop + 1
            lda         sloop + 2
            adc         zrec + 11
            sta         sloop + 2
            lda         sloop + 3
            adc         #0
            sta         sloop + 3
            lda         #1
            sta         zloops
@ram:
            lda         zram                                ; From RAM: zat, zleft, zloopat, zlooplen (its data
            beq         @go                                 ;   under 64K: 16 bits)
            clc
            lda         zbuf
            adc         zrec + 2
            sta         zat
            lda         zbuf + 1
            adc         zrec + 3
            sta         zat + 1
            lda         zrec + 5
            sta         zleft
            lda         zrec + 6
            sta         zleft + 1
            clc
            lda         zat
            adc         zrec + 9
            sta         zloopat
            lda         zat + 1
            adc         zrec + 10
            sta         zloopat + 1
            lda         slen
            sta         zlooplen
            lda         slen + 1
            sta         zlooplen + 1
@go:
            stz         pbn
            stz         pbn + 1
            stz         pbat
            stz         pbat + 1
            lda         #1
            sta         zon
            rts

; The instrument playing fed to /dev/vid/pcm (non-blocking).  From RAM (zram): as much as the FIFO takes, four
; writes at most, its loop again at its end if it loops.  From the file: pbuf's rest, then, if that all went, its
; next part read and written (a read a song tick at most: the card's slow)
zpcm_feed:
            lda         zon
            beq         @done
            lda         zram
            bne         @ram
            jsr         zfile_write
            bcs         @done
            jsr         zfile_read
            bcs         @end
            jsr         zfile_write
@done:
            rts

@end:
            stz         zon
            rts

@ram:
            lda         #4
            sta         pv
@part:
            lda         zleft
            ora         zleft + 1
            bne         @some
            lda         zloops                              ; Its end: its loop again, or nothing more
            beq         @end
            lda         zloopat
            sta         zat
            lda         zloopat + 1
            sta         zat + 1
            lda         zlooplen
            sta         zleft
            lda         zlooplen + 1
            sta         zleft + 1
@some:
            lda         zat
            sta         r0
            lda         zat + 1
            sta         r0 + 1
            lda         zleft
            sta         r1
            lda         zleft + 1
            sta         r1 + 1
            lda         pfd
            jsr         WRITE
            bcs         @done                               ; (E_AGAIN: the FIFO's full)
            sta         r2                                  ; (The count written)
            stx         r2 + 1
            clc
            lda         zat
            adc         r2
            sta         zat
            lda         zat + 1
            adc         r2 + 1
            sta         zat + 1
            sec
            lda         zleft
            sbc         r2
            sta         zleft
            lda         zleft + 1
            sbc         r2 + 1
            sta         zleft + 1
            ora         zleft
            bne         @done                               ; (Some left: the FIFO's full)
            dec         pv
            bne         @part
            rts

; pbuf's rest written (non-blocking).  OUT: C = 0, all of it (or none to go); C = 1, the FIFO's full (or an error)
zfile_write:
            sec
            lda         pbn
            sbc         pbat
            sta         r1
            lda         pbn + 1
            sbc         pbat + 1
            sta         r1 + 1
            ora         r1
            beq         @all
            clc
            lda         #<pbuf
            adc         pbat
            sta         r0
            lda         #>pbuf
            adc         pbat + 1
            sta         r0 + 1
            lda         pfd
            jsr         WRITE
            bcs         @done
            clc
            adc         pbat
            sta         pbat
            txa
            adc         pbat + 1
            sta         pbat + 1
            cmp         pbn + 1
            bne         @full
            lda         pbat
            cmp         pbn
            bne         @full
@all:
            clc
            rts

@full:
            sec
@done:
            rts

; The instrument's next part from the file into pbuf (its loop again at its end, if it loops).  OUT: C = 1, its end
zfile_read:
            lda         sleft
            ora         sleft + 1
            ora         sleft + 2
            ora         sleft + 3
            bne         @read
            lda         zloops
            beq         @end
            lda         pfd2
            jsr         sseek
            ldx         #3
:
            lda         slen,X
            sta         sleft,X
            dex
            bpl         :-
@read:
            lda         pfd2
            jmp         sread

@end:
            sec
            rts

; ****************************************************************************
; The PCM's files, and the song's

; /dev/vid/pcm (in pmode) and pcmctl opened: pfd, pctl.  OUT: C = 0; or C = 1, .A = W_BUSY (another program has
; the PCM) or W_NOSOUND (no Vera X)
pcm_open:
            LDR         r0, ps_vpcm
            lda         pmode
            jsr         OPEN
            jcs         @fail
            sta         pfd
            LDR         r0, ps_vpcmctl
            lda         #O_WRITE
            jsr         OPEN
            jcs         @none
            sta         pctl
            clc
            rts

@fail:
            cmp         #E_BUSY
            jne         @none
            lda         #W_BUSY
            sec
            rts

@none:
            lda         #W_NOSOUND
            sec
            rts

; pcmctl: the FIFO emptied, then the format: .A, AUDIO_CTRL's bits 5-4 (16 bits, stereo)
pcm_format:
            sta         pgeo
            lda         #<ps_reset
            ldx         #>ps_reset
            ldy         #0
            jsr         psay
            ldx         #8
            lda         pgeo
            and         #$20
            beq         :+
            ldx         #16
:
            stx         pnum
            stz         pnum + 1
            lda         #<ps_bits
            ldx         #>ps_bits
            ldy         #1
            jsr         psay
            lda         pgeo
            and         #$10
            bne         :+
            lda         #<ps_mono
            ldx         #>ps_mono
            bra         :++
:
            lda         #<ps_stereo
            ldx         #>ps_stereo
:
            ldy         #0
            jmp         psay

; pcmctl's command: the word at .A/.X (zero-terminated), then pnum in decimal if .Y <> 0
psay:
            sta         r0
            stx         r0 + 1
            sty         pnflag
            ldy         #0
@word:
            lda         (r0),Y
            beq         @worded
            sta         pline,Y
            iny
            bra         @word

@worded:
            lda         pnflag
            beq         @send
            lda         #' '
            sta         pline,Y
            iny
            sty         plen
            lda         #0                                  ; (A 0 on the stack: the digits' end)
            pha
@digit:
            ldx         #16                                 ; pnum / 10: the remainder in .A
            lda         #0
@bit:
            asl         pnum
            rol         pnum + 1
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         pnum
:
            dex
            bne         @bit
            ora         #'0'
            pha
            lda         pnum
            ora         pnum + 1
            bne         @digit
            ldy         plen
@put:
            pla
            beq         @send
            sta         pline,Y
            iny
            bra         @put

@send:
            sty         r1
            stz         r1 + 1
            LDR         r0, pline
            lda         pctl
            jmp         WRITE

; fd .A to sloop (SEEK from the start: r0, r1 the offset)
sseek:
            pha
            lda         sloop
            sta         r0
            lda         sloop + 1
            sta         r0 + 1
            lda         sloop + 2
            sta         r1
            lda         sloop + 3
            sta         r1 + 1
            ldx         #0
            pla
            jmp         SEEK

; pbuf from fd .A: the next min(PBUF, sleft) bytes (pbn their count, pbat 0; sleft less them).  OUT: C = 0; or
; C = 1: none (sleft 0, the file's end, or an error)
sread:
            pha
            lda         sleft + 2
            ora         sleft + 3
            bne         @full
            lda         sleft + 1
            cmp         #>PBUF
            bcs         @full
            lda         sleft
            sta         r1
            lda         sleft + 1
            sta         r1 + 1
            ora         r1
            bne         @read
            pla
            sec
            rts

@full:
            LDR         r1, PBUF
@read:
            LDR         r0, pbuf
            pla
            jsr         READ
            jcs         @none
            sta         pbn
            stx         pbn + 1
            ora         pbn + 1
            jeq         @none
            stz         pbat
            stz         pbat + 1
            sec
            lda         sleft
            sbc         pbn
            sta         sleft
            lda         sleft + 1
            sbc         pbn + 1
            sta         sleft + 1
            lda         sleft + 2
            sbc         #0
            sta         sleft + 2
            lda         sleft + 3
            sbc         #0
            sta         sleft + 3
            clc
            rts

@none:
            sec
            rts

.pushseg
.segment "RODATA2"
ps_iff:     .byte       "IFF"
ps_wave:    .byte       "WAVE"
ps_fmt:     .byte       "fmt "
ps_data:    .byte       "data"
ps_pcm:     .byte       "PCM"
ps_vpcm:    .byte       "/dev/vid/pcm", 0
ps_vpcmctl: .byte       "/dev/vid/pcmctl", 0
ps_reset:   .byte       "reset", 0
ps_bits:    .byte       "bits", 0
ps_mono:    .byte       "mono", 0
ps_stereo:  .byte       "stereo", 0
ps_rate:    .byte       "rate", 0
ps_volume:  .byte       "volume", 0
ps_drain:   .byte       "drain", 0
.popseg
