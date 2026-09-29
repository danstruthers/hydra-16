.debuginfo

; ****************************************************************************
; The sound driver's file server: /dev/snd.  BIOS ROM page 2, included inside `.scope PAGE2` (see
; all.s); the driver itself is sound.s.  The serve routine runs in the sound task (its page 0 gate is in
; sound.s).
;   Write: YM2151 register/value byte pairs (an odd last byte is left over); if the chip doesn't
;          respond, the pairs written so far count, or ERR_IO_DEVICE if none.
;   Read: end of file.  Stat: all zero.
;   Ctl: SND_CTL_INIT (stop, and clear the chip), SND_CTL_TEST (play the test tune in the background: the
;        player task, SND_PLAYER), SND_CTL_STOP (stop the player).
; Server ZP (in the sound task): ZP_IO_TMP = pair bytes offered.

.segment "IO_P2"

SND_SERVE:
            cmp         #H9_CREATE
            bcs         @bad                                ; (The filesystem's requests)
            cmp         #H9_WRITE
            beq         @write
            cmp         #H9_READ
            beq         @read
            cmp         #H9_CTL
            beq         @ctl
            cmp         #H9_STAT
            bne         @ok                                 ; H9_OPEN (fid 0), H9_CLUNK, H9_DUP
            jsr         STAT_ZERO

@ok:
            lda         #0
            clc
            rts

@read:                                                      ; Nothing to read: end of file
            jsr         IO_SRV_MAP
            lda         #0
            jsr         IO_SRV_COUNT                        ; (.A = 0)
            bra         @ok

@ctl:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_CTL_CODE
            lda         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            cmp         #SND_CTL_INIT
            bne         :+
            jsr         SND_STOP                            ; (The player can't write over it)
            jsr         SOUND_INIT
            bra         @ok
:
            cmp         #SND_CTL_STOP
            bne         :+
            jsr         SND_STOP
            bra         @ok
:
            cmp         #SND_CTL_TEST
            bne         @bad
            jsr         SND_PLAYING
            bcs         @busy
            lda         #<SOUND_TEST                        ; The tune, in a task of its own (the
            ldy         #>SOUND_TEST                        ;   sound task's: it isn't the caller's, so
            ldx         #2                                  ;   the console keys leave it alone)
            jsr         TASK_RUN
            bcs         @error
            sta         SND_PLAYER
            bra         @ok

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
            jsr         YM_WRITE
            bcs         @timeout
            cpy         ZP_IO_TMP
            bne         @pair
            dec         ZP_IO_REQ + 1

@done:
            jsr         IO_SRV_UNMAP
            bra         @ok

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
; again at BREAK_ENTRY), and key off all the channels.  Modifies: .A, .X, .Y
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
