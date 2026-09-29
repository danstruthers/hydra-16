.debuginfo

; ****************************************************************************
; The serial driver's file server: /dev/cons (fid SER_FID_CONS), /dev/ser (fid SER_FID_SER) and /dev/ser/ctl
; (fid SER_FID_CTL: the port's settings, below and in serctl.s).  BIOS ROM
; page 2, included inside `.scope PAGE2` (see all.s); the driver itself (init, IRQ handler, rings, and
; the page 0 gates to these serve routines) is in drivers/serial.s.  The serve routines run in the serial
; task, with a client's request (IO_SRV_MAP); the IRQ handler can come in at any time, and fills the RX
; ring and empties the TX ring.
;   Read: whatever is in the RX ring, up to the count (at least 1 byte); if it's empty, or the client
;         isn't in the foreground (/dev/cons only), the client waits (ERR_IO_WOULD_BLOCK) until a byte
;         arrives or the foreground changes.  /dev/cons echoes what it reads, and returns end of file
;         for an end-of-input key (SER_KEY_EOF, SER_KEY_EOF2), which isn't echoed.  It turns DEL (many
;         terminals' Backspace key) into BS, and echoes a BS as BS, space, BS (erasing the character).
;   Write: as much as fits in the TX ring; if nothing fits, the client waits until there's room.  On
;         /dev/cons, only the foreground task and the tasks it started write: others wait until they're
;         in front (like Unix job control).
;   Ctl: SER_CTL_FOREGROUND (.Y = task), SER_CTL_RATE (.Y = SER_RATE_*), SER_CTL_FORMAT (.Y = SER_FMT_*); on
;         any of the three files.  Stat: all zero.
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

; /dev/ser, and /dev/ser/ctl: the rest of the name is "" or "/ctl"
SER_SERVE:
            cmp         #H9_OPEN
            bne         SER_REQUEST
            jsr         IO_SRV_MAP                          ; (.X = the client)
            inc         ZP_IO_REQ + 1                       ; The data area: the name
            ldy         #0
            lda         (ZP_IO_REQ),Y
            beq         @ser

@ctl:
            lda         SER_S_CTL,Y
            cmp         (ZP_IO_REQ),Y
            bne         @not_found
            iny
            ora         #0
            bne         @ctl                                ; (Both ended: a match)
            lda         #SER_FID_CTL
            bra         @open

@ser:
            lda         #SER_FID_SER

@open:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (Keeps .A)
            clc
            rts

@not_found:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

SER_S_CTL:  .byte   "/ctl", 0

; A request on an open fid.  IN: .A = request, .X = client, .Y = fid
SER_REQUEST:
            stx         ZP_IO_CHUNK
            cpy         #SER_FID_CTL
            bne         @data
            cmp         #H9_READ                            ; /dev/ser/ctl: its text
            bne         :+
            jmp         SER_CTL_READ
:
            cmp         #H9_WRITE
            bne         @other
            jmp         SER_CTL_WRITE

@data:
            cmp         #H9_READ
            beq         SER_READ
            cmp         #H9_WRITE
            bne         @other
            jmp         SER_WRITE

@other:
            cmp         #H9_CREATE
            bcs         SER_REFUSE                          ; (The filesystem's requests)
            cmp         #H9_CTL
            beq         SER_CTL
            cmp         #H9_STAT
            bne         SER_OK                              ; H9_CLUNK: nothing to do
            jsr         STAT_ZERO

SER_OK:
            lda         #0
            clc
            rts

SER_REFUSE:
            lda         #ERR_IO_BAD_REQ
            sec
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
            cmp         #SER_CTL_RATE
            bne         :+
            tya                                             ; The rate, with the format as it is
            ldy         SER_FORMAT
            jmp         SER_SET
:
            cmp         #SER_CTL_FORMAT
            bne         :+
            lda         SER_RATE                            ; The format, with the rate as it is
            jmp         SER_SET
:
            cmp         #SER_CTL_FOREGROUND
            bne         @bad
            tya
            tax
            jsr         CONS_FG_CHECK
            bcs         @error
            txa
            jsr         SERIAL_SET_CAPTURE
            bra         SER_OK

@bad:
            lda         #ERR_IO_BAD_REQ

@error:
            sec
            rts

SER_READ:
            sty         ZP_IO_BYTE                          ; (The fid: /dev/cons echoes what it reads)
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
            ldy         ZP_IO_BYTE                          ; /dev/cons: an end-of-input key?
            bne         @take
            cmp         #SER_KEY_EOF
            beq         @eof
            cmp         #SER_KEY_EOF2
            beq         @eof
            cmp         #ASCII_DEL                          ; DEL (many terminals' Backspace key) is a BS
            bne         @take
            lda         #ASCII_BACKSPACE

@take:
            inc         SER_RX_TAIL
            pha
            txa
            tay
            pla
            sta         (ZP_IO_REQ),Y
            ldy         ZP_IO_BYTE
            .assert     SER_FID_CONS = 0, error, "SER_READ: echo when the fid is 0"
            bne         :+
            cmp         #ASCII_BACKSPACE                    ; A backspace: BS, space, BS erases the character
            bne         @echo
            jsr         SER_ECHO
            lda         #ASCII_SPACE
            jsr         SER_ECHO
            lda         #ASCII_BACKSPACE

@echo:
            jsr         SER_ECHO
:
            inx
            cpx         ZP_IO_TMP
            bne         @next

@eof:                                                       ; End of input: after the bytes read so far,
            txa                                             ;   or (none) this read is the end of file
            bne         @done
            inc         SER_RX_TAIL                         ; (Taken: it's not seen again)
            bra         @done                               ; (.A = 0 bytes)

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
            jsr         IO_SRV_COUNT                        ; The count read
            jmp         SER_OK

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
            cpy         #SER_FID_CONS
            bne         @write                              ; /dev/ser: any task
            php                                             ; /dev/cons: the foreground task and the
            sei                                             ;   tasks it started (IRQs off: a console
            jsr         SER_IN_FRONT                        ;   key can change the foreground)
            bcs         :+
            ldx         ZP_IO_CHUNK                         ; In the background: wait until it's in
            ldy         #SER_WR_WAIT                        ;   front (SERIAL_SET_CAPTURE wakes it)
            jsr         SER_ADD_WAIT
            plp
            bra         SER_WOULD_BLOCK
:
            plp
            ldx         ZP_IO_CHUNK                         ; (The client, for IO_SRV_MAP)

@write:
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
            _M_SER_TX_BYTE                                  ; Idle (so the ring is empty): send it now
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
            jsr         IO_SRV_COUNT                        ; The count taken
            jmp         SER_OK

; Echo a byte read from /dev/cons (like a terminal: programs reading from a pipe or a file don't echo):
; into the TX ring.  If it's full, wait for the TX IRQ to make room (about a character's time), or drop
; the byte if IRQs are off.  Preserves .A, .X, .Y
SER_ECHO:
            phy
            php

@try:
            sei
            ldy         ZP_SER_SEND_STATUS
            bne         @queue                              ; Busy: the TX IRQ sends it
            _M_SER_TX_BYTE                                  ; Idle (so the ring is empty): send it now
            inc         ZP_SER_SEND_STATUS                  ; SER_SEND_STATUS_BUSY
            bra         @done

@queue:
            ldy         SER_TX_HEAD
            sta         SER_TX_BUF,Y
            iny
            cpy         SER_TX_TAIL
            beq         @full
            sty         SER_TX_HEAD

@done:
            plp
            ply
            rts

@full:
            sta         ZP_IO_LEFT                          ; (The byte)
            pla                                             ; The caller's flags: IRQs on?
            pha
            and         #$04                                ; (I)
            beq         :+
            lda         ZP_IO_LEFT                          ; No: drop it
            bra         @done
:
            cli
            wai                                             ; Until the TX IRQ (or another)
            lda         ZP_IO_LEFT
            bra         @try

; Is the client in the foreground: the foreground task, or a task it started (or one of those did, ...: 4
; levels), like a Unix process group?  IRQs off.  IN: ZP_IO_CHUNK = client.  OUT: C = 1 yes.
; Modifies: .A, .X, .Y, ZP_IO_TMP
SER_IN_FRONT:
            ldx         ZP_IO_CHUNK
            lda         #5
            sta         ZP_IO_TMP

@task:
            cpx         ZP_SER_CAPTURE
            beq         @yes
            cpx         #MAX_TASK_NUMBER + 1
            bcs         @no                                 ; ($FF: nobody)
            dec         ZP_IO_TMP
            beq         @no
            ldy         T_REGISTER
            stx         T_REGISTER                          ; Quick look (no stack use!)
            ldx         ZP_TASK_OWNER                       ; Its owner
            sty         T_REGISTER
            bra         @task

@yes:
            sec
            rts

@no:
            clc
            rts

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
