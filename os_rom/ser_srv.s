.debuginfo

; ****************************************************************************
; The serial driver's file server: /dev/cons (fid SER_FID_CONS) and /dev/ser (fid SER_FID_SER).  BIOS ROM
; page 2, included inside `.scope PAGE2` (see all.s); the driver itself (init, IRQ handler, rings) is in
; bios.s.  The serve routines run in the serial task (their page 0 gates are in bios.s), with a client's
; request (IO_SRV_MAP); the IRQ handler can come in at any time, and fills the RX ring and empties the
; TX ring.
;   Read: whatever is in the RX ring, up to the count (at least 1 byte); if it's empty, or the client
;         isn't in the foreground (/dev/cons only), the client waits (ERR_IO_WOULD_BLOCK) until a byte
;         arrives or the foreground changes.
;   Write: as much as fits in the TX ring; if nothing fits, the client waits until there's room.
;   Ctl: SER_CTL_FOREGROUND (.Y = task).  Stat: all zero.
; Server ZP (in the serial task): ZP_IO_TMP = count, ZP_IO_CHUNK = client.

.segment "IO_P2"

P2_BIT_MASKS:   .byte   $01, $02, $04, $08, $10, $20, $40, $80

; /dev/cons
CONS_SERVE:
            cmp         #H9_OPEN
            bne         SER_REQUEST
            lda         #SER_FID_CONS
            clc
            rts

; /dev/ser
SER_SERVE:
            cmp         #H9_OPEN
            bne         SER_REQUEST
            lda         #SER_FID_SER
            clc
            rts

; A request on an open fid.  IN: .A = request, .X = client, .Y = fid
SER_REQUEST:
            stx         ZP_IO_CHUNK
            cmp         #H9_READ
            beq         SER_READ
            cmp         #H9_WRITE
            bne         :+
            jmp         SER_WRITE
:
            cmp         #H9_CTL
            beq         SER_CTL
            cmp         #H9_STAT
            bne         SER_OK                              ; H9_CLUNK: nothing to do
            jsr         STAT_ZERO

SER_OK:
            lda         #0
            clc
            rts

SER_CTL:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_CTL_CODE
            lda         (ZP_IO_REQ),Y
            pha
            iny
            lda         (ZP_IO_REQ),Y
            tay                                             ; .Y = argument
            jsr         IO_SRV_UNMAP
            pla
            cmp         #SER_CTL_FOREGROUND
            bne         @bad
            tya
            and         #$0F
            jsr         SERIAL_SET_CAPTURE
            bra         SER_OK

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

SER_READ:
            cpy         #SER_FID_CONS
            bne         :+                                  ; /dev/ser: any task
            cpx         ZP_SER_CAPTURE
            bne         @wait                               ; /dev/cons: only the foreground task
:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_IO_TMP                           ; Bytes wanted (1-256; 256 = 0)
            inc         ZP_IO_REQ + 1                       ; The data area
            ldx         #0                                  ; Bytes done (the ring holds 255 at most)

@next:
            ldy         SER_RX_TAIL
            cpy         SER_RX_HEAD
            beq         @empty
            lda         SER_RX_BUF,Y
            inc         SER_RX_TAIL
            pha
            txa
            tay
            pla
            sta         (ZP_IO_REQ),Y
            inx
            cpx         ZP_IO_TMP
            bne         @next

@empty:
            txa
            bne         @done
            php                                             ; Nothing: check again with IRQs off, so a
            sei                                             ;   byte can't slip in before we're waiting
            lda         SER_RX_HEAD
            cmp         SER_RX_TAIL
            beq         :+
            plp                                             ; It just came in
            bra         @next
:
            ldx         ZP_IO_CHUNK
            ldy         #SER_RD_WAIT
            jsr         SER_ADD_WAIT
            plp
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            bra         SER_WOULD_BLOCK

@done:
            dec         ZP_IO_REQ + 1
            ldy         #IO_BLK_COUNT                       ; The count read
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            bra         SER_OK

@wait:                                                      ; Not in the foreground: wait until it is
            php
            sei
            ldy         #SER_RD_WAIT
            jsr         SER_ADD_WAIT
            plp

SER_WOULD_BLOCK:
            lda         #ERR_IO_WOULD_BLOCK
            sec
            rts

SER_WRITE:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_IO_TMP                           ; Bytes offered (1-256; 256 = 0)
            inc         ZP_IO_REQ + 1                       ; The data area
            ldx         #0                                  ; Bytes taken

@next:
            txa
            tay
            lda         (ZP_IO_REQ),Y
            php
            sei
            ldy         ZP_SER_SEND_STATUS
            bne         @queue                              ; Busy: the TX IRQ sends it
            IO_PORT_WRITE   ACIA_R_DATA                     ; Idle (so the ring is empty): send it now
            inc         ZP_SER_SEND_STATUS                  ; SER_SEND_STATUS_BUSY
            bra         @taken

@queue:
            ldy         SER_TX_HEAD
            sta         SER_TX_BUF,Y
            iny
            cpy         SER_TX_TAIL
            beq         @full                               ; (The byte stored isn't counted: head stays)
            sty         SER_TX_HEAD

@taken:
            plp
            inx
            cpx         ZP_IO_TMP
            bne         @next
            dec         ZP_IO_REQ + 1                       ; All of it: the count stays
            jsr         IO_SRV_UNMAP
            jmp         SER_OK

@full:
            txa
            bne         @short
            ldx         ZP_IO_CHUNK                         ; Nothing fits: wait for room (IRQs are
            ldy         #SER_WR_WAIT                        ;   still off, so the TX IRQ can't slip in)
            jsr         SER_ADD_WAIT
            plp
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            bra         SER_WOULD_BLOCK

@short:
            plp
            dec         ZP_IO_REQ + 1
            ldy         #IO_BLK_COUNT                       ; The count taken
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            jmp         SER_OK

; Add a task to a wait mask (SER_RD_WAIT or SER_WR_WAIT).  IRQs must be off.
; IN: .X = task, .Y = the mask's ZP address.  Modifies: .A, .Y
SER_ADD_WAIT:
            phx
            txa
            and         #7
            tax
            lda         P2_BIT_MASKS,X
            plx
            cpx         #8
            bcc         :+
            iny                                             ; Tasks 8-15: the mask's high byte
:
            ora         a:$0000,Y
            sta         a:$0000,Y
            rts
