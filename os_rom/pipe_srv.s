.debuginfo

; ****************************************************************************
; The pipe server: /dev/pipe.  BIOS ROM page 2, included inside `.scope PAGE2` (see all.s); it runs in
; its own Resident task (PIPE_TASK_NUM; PIPE_INIT and the gate are in io_p0.s), which holds the pipe
; table and the rings (PIPE_TABLE, PIPE_BUFS).  Requests run one at a time (TASK_CALL: no task switch
; while the server runs), and no IRQ handler touches the pipes, so no locking is needed.
;
;   Opening /dev/pipe makes a new pipe; the fid is its index.  IO_PIPE opens the read end and adds a
;   write end.  Each request carries the fd's mode (IO_BLK_MODE), so the server counts the fds on each
;   end: H9_OPEN and H9_DUP add one, H9_CLUNK takes one away; the pipe is free when both counts are 0.
;   Read: what's in the ring, up to the count; if it's empty, the reader waits, or gets end of file if
;         there are no writers left.
;   Write: as much as fits; if nothing fits, the writer waits; ERR_IO_BROKEN if there are no readers.
; Server ZP (in the pipe task): ZP_IO_CHUNK = client, ZP_IO_CHUNK + 1 = the pipe's table offset,
; ZP_IO_OFS = its ring, ZP_IO_TMP = bytes asked for, ZP_IO_BYTE = bytes done, ZP_IO_LEFT = a wait mask.

.segment "IO_P2"

; IN: .A = request, .X = client, .Y = fid
PIPE_SERVE:
            stx         ZP_IO_CHUNK
            cmp         #H9_OPEN
            beq         PIPE_OPEN
            pha
            tya
            and         #PIPE_MAX - 1
            sta         ZP_IO_OFS + 1                       ; (Its ring: set up below)
            asl
            asl
            asl
            sta         ZP_IO_CHUNK + 1                     ; The pipe's table offset
            .assert     PIPE_ENTRY_SIZE = 8, error, "PIPE_SERVE: entry offset = fid * 8"
            lda         ZP_IO_OFS + 1
            clc
            adc         #>PIPE_BUFS
            sta         ZP_IO_OFS + 1
            stz         ZP_IO_OFS
            pla
            cmp         #H9_READ
            bne         :+
            jmp         PIPE_READ
:
            cmp         #H9_WRITE
            bne         :+
            jmp         PIPE_WRITE
:
            cmp         #H9_DUP
            beq         PIPE_DUP
            cmp         #H9_CLUNK
            beq         PIPE_CLUNK
            cmp         #H9_STAT
            bne         PIPE_BAD
            jsr         STAT_ZERO

PIPE_OK:
            lda         #0
            clc
            rts

PIPE_BAD:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

PIPE_OPEN:
            ldx         #0                                  ; A free pipe

@find:
            lda         PIPE_READERS,X
            ora         PIPE_WRITERS,X
            beq         @free
            txa
            clc
            adc         #PIPE_ENTRY_SIZE
            tax
            cpx         #PIPE_MAX * PIPE_ENTRY_SIZE
            bne         @find
            lda         #ERR_IO_NO_PIPES
            sec
            rts

@free:
            stx         ZP_IO_CHUNK + 1
            ldy         #PIPE_ENTRY_SIZE

@clear:
            stz         PIPE_TABLE,X                        ; Empty, nobody waiting
            inx
            dey
            bne         @clear
            jsr         PIPE_ADD_FD                         ; The opener's end(s)
            lda         ZP_IO_CHUNK + 1                     ; fid = the pipe's index
            lsr
            lsr
            lsr
            clc
            rts

PIPE_DUP:
            jsr         PIPE_ADD_FD
            bra         PIPE_OK

PIPE_CLUNK:
            jsr         PIPE_MODE
            lsr                                             ; C = read end
            bcc         @write_end
            pha
            dec         PIPE_READERS,X
            bne         :+
            ldy         #PIPE_WR_WAIT                       ; No readers left: the writers find out
            jsr         PIPE_WAKE
:
            pla

@write_end:
            lsr                                             ; C = write end
            bcc         PIPE_OK
            dec         PIPE_WRITERS,X
            bne         PIPE_OK
            ldy         #PIPE_RD_WAIT                       ; No writers left: the readers get end of file
            jsr         PIPE_WAKE
            bra         PIPE_OK

PIPE_READ:
            ldx         ZP_IO_CHUNK + 1
            lda         PIPE_HEAD,X
            cmp         PIPE_TAIL,X
            bne         @data
            lda         PIPE_WRITERS,X
            beq         @eof
            ldy         #PIPE_RD_WAIT                       ; Empty: wait for a writer
            jmp         PIPE_WAIT

@eof:
            jsr         IO_SRV_MAP_C
            lda         #0
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            bra         PIPE_OK

@data:
            jsr         IO_SRV_MAP_C
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_IO_TMP                           ; Bytes wanted (1-256; 256 = 0)
            inc         ZP_IO_REQ + 1                       ; The data area
            stz         ZP_IO_BYTE

@next:
            ldx         ZP_IO_CHUNK + 1
            ldy         PIPE_TAIL,X
            tya
            cmp         PIPE_HEAD,X
            beq         @end
            lda         (ZP_IO_OFS),Y
            inc         PIPE_TAIL,X
            ldy         ZP_IO_BYTE
            sta         (ZP_IO_REQ),Y
            iny
            sty         ZP_IO_BYTE
            cpy         ZP_IO_TMP
            bne         @next

@end:
            dec         ZP_IO_REQ + 1
            lda         ZP_IO_BYTE                          ; The count read (the ring holds 255 at most)
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            ldy         #PIPE_WR_WAIT                       ; There's room now
            jsr         PIPE_WAKE
            jmp         PIPE_OK

PIPE_WRITE:
            ldx         ZP_IO_CHUNK + 1
            lda         PIPE_READERS,X
            bne         :+
            lda         #ERR_IO_BROKEN                      ; Nobody will read it
            sec
            rts
:
            jsr         IO_SRV_MAP_C
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_IO_TMP                           ; Bytes offered (1-256; 256 = 0)
            inc         ZP_IO_REQ + 1                       ; The data area
            stz         ZP_IO_BYTE

@next:
            ldy         ZP_IO_BYTE
            lda         (ZP_IO_REQ),Y
            ldx         ZP_IO_CHUNK + 1
            ldy         PIPE_HEAD,X
            sta         (ZP_IO_OFS),Y                       ; (The slot at head is always free)
            iny
            tya
            cmp         PIPE_TAIL,X
            beq         @full
            sta         PIPE_HEAD,X
            inc         ZP_IO_BYTE
            lda         ZP_IO_BYTE
            cmp         ZP_IO_TMP
            bne         @next
            dec         ZP_IO_REQ + 1                       ; All of it: the count stays
            bra         @taken

@full:
            dec         ZP_IO_REQ + 1
            lda         ZP_IO_BYTE
            bne         @short
            jsr         IO_SRV_UNMAP                        ; Nothing fits: wait for a reader
            ldy         #PIPE_WR_WAIT
            bra         PIPE_WAIT

@short:
            ldy         #IO_BLK_COUNT                       ; The count taken
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y

@taken:
            jsr         IO_SRV_UNMAP
            ldy         #PIPE_RD_WAIT                       ; There's data now
            jsr         PIPE_WAKE
            jmp         PIPE_OK

; The client waits: add it to a wait mask of the pipe, and tell the IO layer.  IN: .Y = PIPE_RD_WAIT or
; PIPE_WR_WAIT
PIPE_WAIT:
            tya
            clc
            adc         ZP_IO_CHUNK + 1
            tax                                             ; PIPE_TABLE,X = the mask
            lda         ZP_IO_CHUNK
            and         #7
            tay
            lda         P2_BIT_MASKS,Y
            ldy         ZP_IO_CHUNK
            cpy         #8
            bcc         :+
            inx                                             ; Tasks 8-15: the mask's high byte
:
            ora         PIPE_TABLE,X
            sta         PIPE_TABLE,X
            lda         #ERR_IO_WOULD_BLOCK
            sec
            rts

; Wake the tasks in a wait mask of the pipe, and clear it.  IN: .Y = PIPE_RD_WAIT or PIPE_WR_WAIT
; OUT: .X = the pipe's table offset.  Modifies: .A, .Y
PIPE_WAKE:
            tya
            clc
            adc         ZP_IO_CHUNK + 1
            tax
            lda         PIPE_TABLE,X
            sta         ZP_IO_LEFT
            lda         PIPE_TABLE + 1,X
            sta         ZP_IO_LEFT + 1
            stz         PIPE_TABLE,X
            stz         PIPE_TABLE + 1,X
            ldy         #0

@loop:
            lsr         ZP_IO_LEFT + 1
            ror         ZP_IO_LEFT
            bcc         :+
            tya
            jsr         IO_WAKE
:
            iny
            cpy         #16
            bne         @loop
            ldx         ZP_IO_CHUNK + 1
            rts

; Count a new fd on the pipe's end(s), from the request's mode.  OUT: .X = the pipe's table offset
PIPE_ADD_FD:
            jsr         PIPE_MODE
            lsr                                             ; C = read end
            bcc         :+
            inc         PIPE_READERS,X
:
            lsr                                             ; C = write end
            bcc         :+
            inc         PIPE_WRITERS,X
:
            rts

; The request's mode (the fd's).  OUT: .A = mode, .X = the pipe's table offset.  Modifies: .Y
PIPE_MODE:
            jsr         IO_SRV_MAP_C
            ldy         #IO_BLK_MODE
            lda         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            ldx         ZP_IO_CHUNK + 1
            rts

; IO_SRV_MAP for the client in ZP_IO_CHUNK.  Modifies: .X
IO_SRV_MAP_C:
            ldx         ZP_IO_CHUNK
            jmp         IO_SRV_MAP
