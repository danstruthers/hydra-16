.debuginfo

; ****************************************************************************
; The sound driver's file server: /dev/snd.  BIOS ROM page 2, included inside `.scope PAGE2` (see
; all.s); the driver itself is sound.s.  The serve routine runs in the sound task (its page 0 gate is in
; sound.s).
;   Write: YM2151 register/value byte pairs (an odd last byte is left over); if the chip doesn't
;          respond, the pairs written so far count, or ERR_IO_DEVICE if none.
;   Read: end of file.  Ctl: SND_CTL_INIT, SND_CTL_TEST.  Stat: all zero.
; Server ZP (in the sound task): ZP_IO_TMP = pair bytes offered.

.segment "IO_P2"

SND_SERVE:
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
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            bra         @ok

@ctl:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_CTL_CODE
            lda         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            cmp         #SND_CTL_INIT
            bne         :+
            jsr         SOUND_INIT
            bra         @ok
:
            cmp         #SND_CTL_TEST
            bne         @bad
            jsr         SOUND_TEST
            bra         @ok

@bad:
            lda         #ERR_IO_BAD_REQ
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
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            bra         @ok
