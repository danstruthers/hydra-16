.debuginfo

; ****************************************************************************
; The sound driver's file server: /dev/snd.  BIOS ROM page B, included inside `.scope PAGEB` (see
; all.s); the driver itself is sound.s.  The serve routine runs in the sound task (its page 0 gate is in
; sound.s), with the library (snd_lib.s) and its RAM.
;   Open: a fid of its own (1-SND_FIDS), counted (H9_DUP), and freed at the last clunk, which gives back the
;         channels it claimed (keyed off).
;   Write: YM2151 register/value byte pairs, through the library (SND_PAIR: the library's commands at the
;          registers the chip doesn't have; a channel another fid has claimed is left alone).  An odd last
;          byte is left over; if the chip doesn't respond, the pairs written so far count, or ERR_IO_DEVICE
;          if none.
;   Read: the registers as they were written (SND_SHADOW), a 256-byte file.  Stat: its size, 256.
;   /dev/snd/volume (SND_FID_VOLUME): the master volume as text, a percentage: read "100" (and CR LF); write a number,
;         0-200 (100: songs as written; more: louder, up to about 24 dB; any words before it are skipped: "volume 150").
;   Ctl: SND_CTL_INIT (stop, and clear the chip and the library's settings), SND_CTL_TEST (play the test song in
;        the background: the song player, page C, in a task of its own, SND_PLAYER), SND_CTL_STOP (stop it),
;        SND_CTL_CLAIM and SND_CTL_RELEASE (.Y: a mask of channels), SND_CTL_VOLUME (the master volume),
;        SND_CTL_CLOCK (the sound clock: timer B, for a song player; snd_lib.s).
; Server ZP (in the sound task): ZP_IO_TMP = pair bytes offered; SND_FID = the request's fid.

.segment "SOUND_PB"

SND_SERVE:
            sty         SND_FID
            cpy         #SND_FID_VOLUME                     ; /dev/snd/volume's (H9_OPEN: no fid yet)?
            bne         :+
            cmp         #H9_OPEN
            beq         :+
            jmp         SND_VOLUME_REQ
:
            cmp         #H9_CREATE
            bcc         :+
            jmp         @bad                                ; (The filesystem's requests)
:
            cmp         #H9_WRITE
            bne         :+
            jmp         @write
:
            cmp         #H9_READ
            bne         :+
            jmp         SND_READ
:
            cmp         #H9_CTL
            bne         :+
            jmp         @ctl
:
            cmp         #H9_STAT
            bne         :+
            jmp         SND_STAT
:
            cmp         #H9_OPEN
            beq         @open
            cmp         #H9_DUP
            beq         @dup
            cmp         #H9_CLUNK
            beq         @clunk

@ok:
            lda         #0
            clc
            rts

@open:                                                      ; "/volume": /dev/snd/volume
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; (The name, in the data area)
            ldy         #0
            lda         (ZP_IO_REQ),Y
            beq         @slot                               ; "": /dev/snd
:
            lda         (ZP_IO_REQ),Y
            cmp         SND_S_VOLUME,Y
            bne         @no_name
            iny
            cmp         #0
            bne         :-
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #SND_FID_VOLUME
            clc
            rts

@no_name:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

@slot:                                                      ; A free fid
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            ldx         #0
:
            lda         SND_REFS,X
            beq         :+
            inx
            cpx         #SND_FIDS
            bne         :-
            lda         #ERR_IO_BUSY                        ; (SND_FIDS open already)
            sec
            rts
:
            inc         SND_REFS,X
            inx
            txa                                             ; (The fid: 1-SND_FIDS)
            clc
            rts

@dup:
            ldx         SND_FID
            inc         SND_REFS - 1,X
            bra         @ok

@clunk:
            ldx         SND_FID
            dec         SND_REFS - 1,X
            bne         @ok
            lda         SND_CLK_OWNER                       ; The last: its sound clock stopped ...
            cmp         SND_FID
            bne         :+
            jsr         SND_CLOCK_STOP
:
            lda         #$FF                                ;   and its channels back
            jsr         SND_RELEASE
            bra         @ok

@ctl:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_CTL_ARG
            lda         (ZP_IO_REQ),Y
            sta         SND_T + 1
            ldy         #IO_BLK_CTL_CODE
            lda         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            cmp         #SND_CTL_INIT
            bne         :+
            jsr         SND_STOP                            ; (The player can't write over it)
            jsr         SND_RESET
            bra         @ctl_ok
:
            cmp         #SND_CTL_STOP
            bne         :+
            jsr         SND_STOP
            bra         @ctl_ok
:
            cmp         #SND_CTL_CLAIM
            bne         :+
            lda         SND_T + 1
            jsr         SND_CLAIM
            bcs         @error
            bra         @ctl_ok
:
            cmp         #SND_CTL_RELEASE
            bne         :+
            lda         SND_T + 1
            jsr         SND_RELEASE
            bra         @ctl_ok
:
            cmp         #SND_CTL_VOLUME
            bne         :+
            lda         SND_T + 1
            jsr         SND_MASTER_SET
            bra         @ctl_ok
:
            cmp         #SND_CTL_CLOCK
            bne         :+
            lda         SND_T + 1
            jsr         SND_CLOCK_SET
            bcs         @clock_err
            lda         #0

@clock_err:
            sta         SND_CLK_ERR                         ; (Its result, for /dev/snd's numbers)
            bcs         @error
            bra         @ctl_ok
:
            cmp         #SND_CTL_TEST
            bne         @bad
            jsr         SND_PLAYING
            bcs         @busy
            lda         #<::ZSM_PLAY_TEST_PC                ; The song player on the test song, in a task of
            ldy         #>::ZSM_PLAY_TEST_PC                ;   its own (the sound task's: it isn't the caller's,
            ldx         #$C                                 ;   so the console keys leave it alone).  It claims
            jsr         TASK_RUN                            ;   its channels itself
            bcs         @error
            sta         SND_PLAYER

@ctl_ok:
            jmp         @ok

@busy:
            lda         #ERR_TASK_BUSY
            bra         @error

@bad:
            lda         #ERR_IO_BAD_REQ

@error:
            sec
            rts

@write:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y                       ; Bytes offered (1-256: 256 = low 0, high 1)
            and         #$FE                                ; The count taken: whole pairs (an odd
            sta         (ZP_IO_REQ),Y                       ;   last byte is left over)
            sta         ZP_IO_TMP
            iny
            ora         (ZP_IO_REQ),Y
            beq         @done                               ; Just 1 byte: nothing taken
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0

@pair:
            lda         (ZP_IO_REQ),Y
            tax                                             ; .X = register
            iny
            lda         (ZP_IO_REQ),Y                       ; .A = value
            iny
            phy
            jsr         SND_PAIR
            ply
            bcs         @timeout
            cpy         ZP_IO_TMP
            bne         @pair
            dec         ZP_IO_REQ + 1

@done:
            jsr         IO_SRV_UNMAP
            jmp         @ok

@timeout:
            dec         ZP_IO_REQ + 1
            dey                                             ; This pair didn't go
            dey
            bne         @short
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_DEVICE
            sec
            rts

@short:
            tya
            jsr         IO_SRV_COUNT
            jmp         @ok

; The sound clock's numbers, in the shadow's spare bytes (no register of the chip's), for a read of /dev/snd:
;   $15-$16 its ticks (SND_CLK), $1C-$1D its interrupts taken (SND_IRQS), $1E-$1F the system's ticks since it
;   started (200 a second: as many as the clock's at a song's 200 Hz); $00 SND_CLOCK_SET's last result (0: started), $0B and $0E the
;   time the player last waited for (its ZSM_AT: 0 if it never waited on the clock), $13 its wake-ups (low byte).
;   Modifies .A, .X, .Y
SND_NUMBERS:
            lda         SND_CLK
            sta         SND_SHADOW + $15
            lda         SND_CLK + 1
            sta         SND_SHADOW + $16
            lda         SND_IRQS
            sta         SND_SHADOW + $1C
            lda         SND_IRQS + 1
            sta         SND_SHADOW + $1D
            lda         SND_CLK_ERR
            sta         SND_SHADOW + $00
            lda         SND_LAST_AT
            sta         SND_SHADOW + $0B
            lda         SND_LAST_AT + 1
            sta         SND_SHADOW + $0E
            lda         SND_WAKES
            sta         SND_SHADOW + $13
            jsr         TICKS_GET
            sec
            sbc         SND_CLK_T0
            sta         SND_SHADOW + $1E
            tya
            sbc         SND_CLK_T0 + 1
            sta         SND_SHADOW + $1F
            rts

; A read: the shadow's bytes from the fd's offset (256 bytes in all; past them, the end).  IN: .X = the client
SND_READ:
            jsr         SND_NUMBERS
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_OFS + 3                     ; An offset past 255: the end
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @end
            dey
            lda         (ZP_IO_REQ),Y
            tax                                             ; .X = the offset
            ldy         #IO_BLK_COUNT + 1
            lda         (ZP_IO_REQ),Y
            beq         :+
            lda         #0                                  ; (256 or more: as many as there are)
            bra         @count
:
            dey
            lda         (ZP_IO_REQ),Y                       ; (1-255)
@count:
            sta         SND_T                               ; Bytes wanted (0: 256)
            inc         ZP_IO_REQ + 1
            ldy         #0

@copy:
            lda         SND_SHADOW,X
            sta         (ZP_IO_REQ),Y
            iny
            inx
            beq         @copied                             ; (The shadow's end)
            cpy         SND_T
            bne         @copy

@copied:
            dec         ZP_IO_REQ + 1
            tya
            bne         @given
            ldy         #IO_BLK_COUNT                       ; All 256
            sta         (ZP_IO_REQ),Y
            iny
            lda         #1
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            bra         @ok

@end:
            lda         #0

@given:
            jsr         IO_SRV_COUNT

@ok:
            lda         #0
            clc
            rts

; A stat: all zeros but the size, 256.  IN: .X = the client
SND_STAT:
            jsr         STAT_ZERO                           ; (It unmaps)
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1
            ldy         #IO_ST_SIZE + 1
            lda         #1
            sta         (ZP_IO_REQ),Y
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #0
            clc
            rts

; Claim channels for fid SND_FID: all of them, or none if another fid has one.  IN: .A = the mask.  OUT: C = 0;
; or C = 1, .A = ERR_IO_BUSY.  Modifies .X
SND_CLAIM:
            sta         SND_T + 1                           ; (The mask, for both passes)
            sta         SND_T
            ldx         #0

@check:
            lsr         SND_T
            bcc         @next_check
            lda         SND_OWNER,X
            beq         @next_check
            cmp         SND_FID
            beq         @next_check
            lda         #ERR_IO_BUSY
            sec
            rts

@next_check:
            inx
            cpx         #8
            bne         @check
            lda         SND_T + 1                           ; All free: take them
            sta         SND_T
            ldx         #0

@take:
            lsr         SND_T
            bcc         :+
            lda         SND_FID
            sta         SND_OWNER,X
:
            inx
            cpx         #8
            bne         @take
            jmp         SND_CLAIMED_SET

; Give back channels fid SND_FID has (keyed off).  IN: .A = the mask.  OUT: C = 0.  Modifies .X, .Y
SND_RELEASE:
            sta         SND_T + 1
            ldx         #0

@channel:
            lsr         SND_T + 1
            bcc         @next
            lda         SND_OWNER,X
            cmp         SND_FID
            bne         @next
            stz         SND_OWNER,X
            phx
            txa                                             ; Key off
            ldx         #$08
            jsr         SND_SET
            plx

@next:
            inx
            cpx         #8
            bne         @channel
            jmp         SND_CLAIMED_SET

; SND_CLAIMED: a bit for each channel claimed.  OUT: C = 0.  Modifies .A, .X
SND_CLAIMED_SET:
            stz         SND_CLAIMED
            ldx         #7

@look:
            lda         SND_OWNER,X
            cmp         #1                                  ; (C = 1: claimed)
            rol         SND_CLAIMED
            dex
            bpl         @look
            clc
            rts

; The master volume: its TL steps (SND_MASTER_TL), every channel's attenuation, and the levels written again.
; IN: .A = the volume, a percentage: 0-99, quieter (the volume curve, SND_VOLUME_ATTEN, at about .A * 1.27); 100,
; songs as written; 101-200, louder: 0.32 of a TL step (0.75 dB) a point, 31 steps (23 dB) at 200 (more: 200)
SND_MASTER_SET:
            cmp         #201
            bcc         :+
            lda         #200
:
            sta         SND_MASTER
            sec
            sbc         #100
            bcc         @quieter
            sta         SND_T                               ; Louder: -(n / 4 + n / 16), n = .A - 100
            lsr
            lsr
            sta         SND_T + 1
            lsr
            lsr
            clc
            adc         SND_T + 1
            eor         #$FF
            inc                                             ; (Negative)
            bra         @set

@quieter:
            lda         SND_MASTER                          ; The curve at .A + .A / 4 + .A / 64 (about * 1.27)
            lsr
            lsr
            sta         SND_T + 1
            lsr
            lsr
            lsr
            lsr
            clc
            adc         SND_T + 1
            adc         SND_MASTER
            tax
            lda         SND_VOLUME_ATTEN,X

@set:
            sta         SND_MASTER_TL
            ldy         #7

@channel:
            jsr         SND_ATTEN_SET
            tya
            jsr         SND_RECOOK
            dey
            bpl         @channel
            rts

SND_S_VOLUME:   .byte   "/volume", 0

; /dev/snd/volume's request.  IN: .A = request
SND_VOLUME_REQ:
            cmp         #H9_READ
            beq         @read
            cmp         #H9_WRITE
            beq         @write
            cmp         #H9_STAT
            bne         :+
            jsr         STAT_ZERO                           ; (It unmaps)
            bra         @ok
:
            cmp         #H9_CLUNK
            beq         @ok
            cmp         #H9_DUP
            beq         @ok
            lda         #ERR_IO_BAD_REQ
            sec
            rts

@ok:
            lda         #0
            clc
            rts

@write:                                                     ; A number: the first digits, up to 255 (more: 255)
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_IO_TMP                           ; (The count: 1-256, 256 = 0)
            inc         ZP_IO_REQ + 1
            ldy         #0
            stz         SND_T                               ; The number
            stz         SND_T + 1                           ; (<> 0: a digit seen)

@char:
            lda         (ZP_IO_REQ),Y
            sec
            sbc         #'0'
            cmp         #10
            bcc         @digit
            lda         SND_T + 1
            bne         @number                             ; (After the digits: the end)
            bra         @next

@digit:
            sta         ZP_IO_BYTE
            sta         SND_T + 1                           ; (Seen: <> 0 below if it's 0, by the inc)
            inc         SND_T + 1
            lda         SND_T                               ; * 10 + the digit, 255 at most
            cmp         #26
            bcs         @big
            asl
            asl
            adc         SND_T
            asl
            adc         ZP_IO_BYTE
            bcc         :+

@big:
            lda         #255
:
            sta         SND_T

@next:
            iny
            cpy         ZP_IO_TMP
            bne         @char
            lda         SND_T + 1
            beq         @bad                                ; (No number)

@number:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         SND_T
            jsr         SND_MASTER_SET
            bra         @ok

@bad:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_BAD_REQ
            sec
            rts

@read:                                                      ; "N", CR LF, from the offset
            phx                                             ; (The client)
            ldx         #0                                  ; SND_TEXT: the number, in decimal
            lda         SND_MASTER
            ldy         #0                                  ; (Hundreds)
:
            cmp         #100
            bcc         :+
            sbc         #100
            iny
            bra         :-
:
            pha
            tya
            beq         :+                                  ; (No leading 0)
            ora         #'0'
            sta         SND_TEXT,X
            inx
:
            pla
            ldy         #0                                  ; (Tens)
:
            cmp         #10
            bcc         :+
            sbc         #10
            iny
            bra         :-
:
            pha
            tya
            bne         :+
            cpx         #0
            beq         :++                                 ; (No leading 0)
:
            ora         #'0'
            sta         SND_TEXT,X
            inx
:
            pla
            ora         #'0'
            sta         SND_TEXT,X
            lda         #ASCII_CR
            sta         SND_TEXT + 1,X
            lda         #ASCII_LF
            sta         SND_TEXT + 2,X
            inx
            inx
            inx
            stx         SND_T + 1                           ; (Its length)
            plx
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_OFS + 3                     ; From the offset: past the text, nothing
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @none
            dey
            lda         (ZP_IO_REQ),Y
            tax                                             ; .X = the offset
            ldy         #0                                  ; .Y = bytes given
            inc         ZP_IO_REQ + 1

@give:
            cpx         SND_T + 1
            bcs         @given
            lda         SND_TEXT,X
            sta         (ZP_IO_REQ),Y
            inx
            iny
            bra         @give                               ; (5 at most: a read's count is more)

@given:
            dec         ZP_IO_REQ + 1
            tya
            bra         :+

@none:
            lda         #0
:
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            jmp         @ok

; Is the player playing: SND_PLAYER busy, and still the sound task's (not a task that came after it)?
; OUT: C = 1 yes (.X = it).  Modifies: .A, .Y, ZP_IO_TMP
SND_PLAYING:
            ldx         SND_PLAYER
            cpx         #MAX_TASK_NUMBER + 1
            bcs         @no                                 ; ($FF: none)
            php
            sei
            ldy         T_REGISTER
            stx         T_REGISTER                          ; Quick look (no stack use!)
            lda         TASK_STATUS_REG
            sty         T_REGISTER
            sta         ZP_IO_TMP
            stx         T_REGISTER                          ; Quick look (no stack use!)
            lda         ZP_TASK_OWNER
            sty         T_REGISTER
            plp
            cmp         #SOUND_TASK_NUM
            bne         @no
            lda         ZP_IO_TMP
            and         #TASK_BUSY_FLAG
            beq         @no
            sec
            rts

@no:
            clc
            rts

; Stop the player, if it's playing (it ends without writing to the chip again: a killed task only runs
; again at BREAK_ENTRY; its end closes its /dev/snd, which gives its channels back), and key off all the
; channels.  Modifies: .A, .X, .Y
SND_STOP:
            jsr         SND_PLAYING
            bcc         :+
            lda         #TASK_KILL_FLAG
            jsr         TASK_SIGNAL
:
            lda         #$FF
            sta         SND_PLAYER
            lda         #7                                  ; Channels 7-0: key off

@channel:
            ldx         #$08
            pha
            jsr         YM_WRITE
            pla
            dec
            bpl         @channel
            rts
