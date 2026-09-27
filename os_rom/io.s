.debuginfo

; ****************************************************************************
; The IO layer (see IO_PLAN.md): all IO goes through file descriptors.  BIOS ROM page 2, included inside
; `.scope PAGE2` (see all.s); page 0 and page 1 reach it through gates.
;
;   IO_OPEN "/dev/<name>[/<rest>]" finds <name> in the device table (DEV_REGISTER), and the device's
;   serve routine (a file server, usually a driver) opens <rest>.  Reads and writes go to the server as
;   H9P requests: the request block and up to IO_UNIT bytes of data travel through the task's IO transfer
;   area in shared RAM (bank ID $09), and the serve routine runs in the server's task (TASK_CALL).
;
;   All calls: C = 0 on success, C = 1 with the error in .A; they preserve .X and .Y (except where they
;   return something in them).  Names and buffers must be in task RAM ($0000-$7FFF); a name can also be on
;   ROM page 2.
;
;   Blocking: a server that has no data yet returns ERR_IO_WOULD_BLOCK, and later wakes the task
;   (IO_WAKE).  The task marks itself waiting *before* calling the server, so a wake that comes early
;   isn't lost; if the fd is IO_MODE_NONBLOCK, the error is returned instead.

.segment "IO_P2"

; Save RAM_BANK_REG / U on the stack and map the IO transfer bank (uses .Y)
.macro _M_IO_MAP_XFER
            ldy         RAM_BANK_REG
            phy
            ldy         U_REGISTER
            phy
            stz         U_REGISTER
            ldy         #IO_XFER_BANK
            sty         RAM_BANK_REG
.endmacro

; Restore U / RAM_BANK_REG (uses .Y; preserves .A and C)
.macro _M_IO_UNMAP
            ply
            sty         U_REGISTER
            ply
            sty         RAM_BANK_REG
.endmacro

S_DEV_PREFIX:   .byte "/dev/"
DEV_PREFIX_LEN  = 5

; ---- helpers

; ZP_IO_XFER / ZP_IO_DATA = the current task's transfer area: $8000 + task * $200.  Modifies: .A
IO_XFER_SETUP:
            lda         T_REGISTER
            and         #$0F
            asl
            ora         #>PAGED_RAM_BASE
            sta         ZP_IO_XFER + 1
            inc
            sta         ZP_IO_DATA + 1
            stz         ZP_IO_XFER
            stz         ZP_IO_DATA
            rts

; Check an fd is open.  IN: .A = fd.  OUT: .X = fd * IO_FD_SIZE, ZP_IO_FD = fd, C = 0; or .A = ERR_IO_BAD_FD, C = 1
IO_FD_CHECK:
            cmp         #IO_MAX_FDS
            bcs         @bad
            sta         ZP_IO_FD
            asl
            asl
            asl
            tax
            lda         IO_FD_SERVER,X
            cmp         #IO_FD_CLOSED
            beq         @bad
            clc
            rts

@bad:
            lda         #ERR_IO_BAD_FD
            sec
            rts

; Send the request in the transfer area to fd ZP_IO_FD's server (transfer bank mapped).
; IN: .A = H9_* request.  OUT: C = 0 and .A from the server; or .A = error, C = 1
; Modifies: .A, .X, .Y
IO_SERVE:
            ldy         #IO_BLK_TYPE
            sta         (ZP_IO_XFER),Y
            lda         ZP_IO_FD
            asl
            asl
            asl
            tax
            lda         IO_FD_FID,X
            ldy         #IO_BLK_FID
            sta         (ZP_IO_XFER),Y
            lda         IO_FD_MODE,X                ; (A pipe's ends differ only in their modes)
            ldy         #IO_BLK_MODE
            sta         (ZP_IO_XFER),Y
            lda         T_REGISTER
            and         #$0F
            ldy         #IO_BLK_CLIENT
            sta         (ZP_IO_XFER),Y
            lda         IO_FD_SERVER,X              ; The device: its task and serve routine
            asl
            asl
            asl
            asl
            tax
            ldy         #MSG_PTR_BANK               ; Device table (shared bank ID $00)
            sty         RAM_BANK_REG
            lda         IO_DEV_TABLE + IO_DEV_TASK,X
            cmp         #IO_DEV_CALLER_TASK
            bne         :+
            lda         T_REGISTER                  ; Serve in the calling task
:
            sta         ZP_TC_TASK
            lda         IO_DEV_TABLE + IO_DEV_SERVE,X
            sta         ZP_TC_VEC
            lda         IO_DEV_TABLE + IO_DEV_SERVE + 1,X
            sta         ZP_TC_VEC + 1
            ldy         #IO_XFER_BANK
            sty         RAM_BANK_REG

@request:
            inc         ZP_NO_PREEMPT               ; No task switch while we're marked waiting but not yet in
            php                                     ;   the server's wait list: nobody would wake us (NO_PREEMPT)
            sei
            smb2        TASK_STATUS_REG             ; Waiting, until the server says otherwise (TASK_WAITING_FLAG)
            plp
            ldy         #IO_BLK_TYPE
            lda         (ZP_IO_XFER),Y
            pha                                     ; .A = request
            ldy         #IO_BLK_FID
            lda         (ZP_IO_XFER),Y
            tay                                     ; .Y = fid
            lda         T_REGISTER
            and         #$0F
            tax                                     ; .X = client
            pla
            jsr         TASK_CALL                   ; The serve routine, in the server's task
            bcc         @done
            cmp         #ERR_IO_WOULD_BLOCK
            bne         @done
            lda         ZP_IO_FD                    ; No data yet: wait?
            asl
            asl
            asl
            tax
            lda         IO_FD_MODE,X
            bmi         @no_wait                    ; IO_MODE_NONBLOCK
            jsr         YIELD                       ; Sleep until the server wakes us (at once if it did already)
            dec         ZP_NO_PREEMPT
            bra         @request

@no_wait:
            lda         #ERR_IO_WOULD_BLOCK
            sec

@done:
            rmb2        TASK_STATUS_REG             ; Not waiting (rmb doesn't change the flags)
            jmp         PREEMPT                     ; (Preserves .A and the flags; switches if a switch came due)

; ZP_IO_CHUNK = min(ZP_IO_LEFT, IO_UNIT); also the request's count.  Modifies: .A, .Y
IO_SET_CHUNK:
            lda         ZP_IO_LEFT + 1
            beq         :+
            stz         ZP_IO_CHUNK                 ; 256 or more left: a full IO_UNIT
            lda         #1
            sta         ZP_IO_CHUNK + 1
            bra         @count
:
            lda         ZP_IO_LEFT
            sta         ZP_IO_CHUNK
            stz         ZP_IO_CHUNK + 1

@count:
            ldy         #IO_BLK_COUNT
            lda         ZP_IO_CHUNK
            sta         (ZP_IO_XFER),Y
            iny
            lda         ZP_IO_CHUNK + 1
            sta         (ZP_IO_XFER),Y
            rts

; Copy fd ZP_IO_FD's offset into the request.  Modifies: .A, .X, .Y
IO_SET_OFS:
            lda         ZP_IO_FD
            asl
            asl
            asl
            tax
            ldy         #IO_BLK_OFS

@copy:
            lda         IO_FD_OFS,X
            sta         (ZP_IO_XFER),Y
            inx
            iny
            cpy         #IO_BLK_OFS + 4
            bne         @copy
            rts

; The request's count (bytes the server did): into ZP_IO_CHUNK.  Z = 1 if none.  Modifies: .A, .Y
IO_GET_DONE:
            ldy         #IO_BLK_COUNT + 1
            lda         (ZP_IO_XFER),Y
            sta         ZP_IO_CHUNK + 1
            dey
            lda         (ZP_IO_XFER),Y
            sta         ZP_IO_CHUNK
            ora         ZP_IO_CHUNK + 1
            rts

; Advance by ZP_IO_CHUNK bytes: the caller's buffer, the fd's offset, the count done (ZP_IO_CNT) and the
; count left.  Modifies: .A, .X
IO_ADVANCE:
            lda         ZP_IO_BUF
            clc
            adc         ZP_IO_CHUNK
            sta         ZP_IO_BUF
            lda         ZP_IO_BUF + 1
            adc         ZP_IO_CHUNK + 1
            sta         ZP_IO_BUF + 1
            lda         ZP_IO_CNT
            clc
            adc         ZP_IO_CHUNK
            sta         ZP_IO_CNT
            lda         ZP_IO_CNT + 1
            adc         ZP_IO_CHUNK + 1
            sta         ZP_IO_CNT + 1
            lda         ZP_IO_LEFT
            sec
            sbc         ZP_IO_CHUNK
            sta         ZP_IO_LEFT
            lda         ZP_IO_LEFT + 1
            sbc         ZP_IO_CHUNK + 1
            sta         ZP_IO_LEFT + 1
            lda         ZP_IO_FD
            asl
            asl
            asl
            tax
            lda         IO_FD_OFS,X
            clc
            adc         ZP_IO_CHUNK
            sta         IO_FD_OFS,X
            lda         IO_FD_OFS + 1,X
            adc         ZP_IO_CHUNK + 1
            sta         IO_FD_OFS + 1,X
            bcc         @done
            inc         IO_FD_OFS + 2,X
            bne         @done
            inc         IO_FD_OFS + 3,X

@done:
            rts

; Copy ZP_IO_CHUNK (1-256) bytes: transfer data -> caller's buffer (IO_COPY_OUT) or back (IO_COPY_IN).
; Modifies: .A, .Y
IO_COPY_OUT:
            ldy         #0
:
            lda         (ZP_IO_DATA),Y
            sta         (ZP_IO_BUF),Y
            iny
            cpy         ZP_IO_CHUNK                 ; (256: ZP_IO_CHUNK = 0, so Y wraps round to it)
            bne         :-
            rts

IO_COPY_IN:
            ldy         #0
:
            lda         (ZP_IO_BUF),Y
            sta         (ZP_IO_DATA),Y
            iny
            cpy         ZP_IO_CHUNK
            bne         :-
            rts

; ****************************************************************************
; The calls

; Open a file.
; IN: .A.Y = name ("/dev/<device>" or "/dev/<device>/<rest>", zero-terminated), .X = IO_MODE_* bits
; OUT (success): .A = fd, C = 0
; OUT (failure): .A = ERR_IO_NOT_FOUND, ERR_IO_NO_FDS or the server's error, C = 1
IO_OPEN:
            PUSH_XY
            sta         ZP_IO_BUF
            sty         ZP_IO_BUF + 1
            stx         ZP_IO_MODE
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            ldy         #DEV_PREFIX_LEN - 1         ; "/dev/"?

@prefix:
            lda         (ZP_IO_BUF),Y
            cmp         S_DEV_PREFIX,Y
            bne         @not_found
            dey
            bpl         @prefix
            lda         ZP_IO_BUF                   ; ZP_IO_LEFT = the device name
            clc
            adc         #DEV_PREFIX_LEN
            sta         ZP_IO_LEFT
            lda         ZP_IO_BUF + 1
            adc         #0
            sta         ZP_IO_LEFT + 1
            ldy         #MSG_PTR_BANK               ; Device table (shared bank ID $00)
            sty         RAM_BANK_REG
            lda         #<IO_DEV_TABLE
            sta         ZP_IO_CHUNK                 ; ZP_IO_CHUNK = the entry
            lda         #>IO_DEV_TABLE
            sta         ZP_IO_CHUNK + 1

@entry:
            lda         (ZP_IO_CHUNK)
            beq         @next_entry                 ; Free entry
            ldy         #0

@compare:
            lda         (ZP_IO_LEFT),Y              ; The name's character; '/' or 0 ends it
            cmp         #'/'
            bne         :+
            lda         #0
:
            sta         ZP_IO_TMP
            cpy         #IO_DEV_NAME_LEN
            beq         @all_8                      ; 8 characters matched
            cmp         (ZP_IO_CHUNK),Y
            bne         @next_entry
            iny
            lda         ZP_IO_TMP
            bne         @compare                    ; Both ended: a match
            dey                                     ; .Y = the name's length
            bra         @found

@all_8:
            lda         ZP_IO_TMP
            bne         @next_entry                 ; The name is longer than 8

@found:
            sty         ZP_IO_TMP                   ; Name length
            lda         ZP_IO_CHUNK                 ; Device index = (entry - table) / 16
            sec
            sbc         #<IO_DEV_TABLE
            lsr
            lsr
            lsr
            lsr
            sta         ZP_IO_CNT                   ; ZP_IO_CNT = device index
            ldy         #IO_XFER_BANK
            sty         RAM_BANK_REG
            bra         @find_fd

@next_entry:
            lda         ZP_IO_CHUNK
            clc
            adc         #IO_DEV_SIZE
            sta         ZP_IO_CHUNK
            bne         @entry                      ; 16 entries: $8800-$88FF

@not_found:
            ldy         #IO_XFER_BANK
            sty         RAM_BANK_REG
            lda         #ERR_IO_NOT_FOUND
            bra         @fail

@find_fd:
            ldx         #0

@fd:
            lda         IO_FD_SERVER,X
            cmp         #IO_FD_CLOSED
            beq         @got_fd
            txa
            clc
            adc         #IO_FD_SIZE
            tax
            cpx         #IO_MAX_FDS * IO_FD_SIZE
            bne         @fd
            lda         #ERR_IO_NO_FDS
            bra         @fail

@got_fd:
            txa                                     ; fd = offset / 8
            lsr
            lsr
            lsr
            sta         ZP_IO_FD
            lda         ZP_IO_CNT
            sta         IO_FD_SERVER,X
            stz         IO_FD_FID,X
            lda         ZP_IO_MODE
            sta         IO_FD_MODE,X
            stz         IO_FD_OFS,X
            stz         IO_FD_OFS + 1,X
            stz         IO_FD_OFS + 2,X
            stz         IO_FD_OFS + 3,X
            lda         ZP_IO_MODE
            ldy         #IO_BLK_MODE
            sta         (ZP_IO_XFER),Y
            lda         ZP_IO_LEFT                  ; The rest of the name (after the device name) ->
            clc                                     ;   the data area, for the server
            adc         ZP_IO_TMP
            sta         ZP_IO_LEFT
            bcc         :+
            inc         ZP_IO_LEFT + 1
:
            ldy         #0

@rest:
            lda         (ZP_IO_LEFT),Y
            sta         (ZP_IO_DATA),Y
            beq         @open
            iny
            bne         @rest
            lda         #0                          ; (255 characters at most)
            dey
            sta         (ZP_IO_DATA),Y

@open:
            lda         #H9_OPEN
            jsr         IO_SERVE                    ; .A = fid
            bcs         @open_failed
            pha
            lda         ZP_IO_FD
            asl
            asl
            asl
            tax
            pla
            sta         IO_FD_FID,X
            lda         ZP_IO_FD
            clc
            bra         @done

@open_failed:
            pha
            lda         ZP_IO_FD
            asl
            asl
            asl
            tax
            lda         #IO_FD_CLOSED
            sta         IO_FD_SERVER,X
            pla

@fail:
            sec

@done:
            _M_IO_UNMAP
            PULL_YX
            rts

; Close an fd (the server's fid too).
; IN: .A = fd
; OUT (success): C = 0
; OUT (failure): .A = ERR_IO_BAD_FD or the server's error, C = 1 (the fd is closed anyway)
IO_CLOSE:
            PUSH_XY
            jsr         IO_FD_CHECK
            bcs         @done
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         #H9_CLUNK
            jsr         IO_SERVE
            _M_IO_UNMAP
            pha
            php
            lda         ZP_IO_FD
            asl
            asl
            asl
            tax
            lda         #IO_FD_CLOSED
            sta         IO_FD_SERVER,X
            plp
            pla

@done:
            PULL_YX
            rts

; Read from an fd.  Returns when the count is done, at end of file, or when the server has less (e.g.
; the keyboard: the bytes typed so far).
; IN: .A = fd, ZP_IO_BUF = buffer, ZP_IO_CNT = bytes to read
; OUT (success): ZP_IO_CNT = bytes read (0 = end of file), C = 0
; OUT (failure): .A = error, C = 1 (ZP_IO_CNT = bytes read before the error)
IO_READ:
            PUSH_XY
            jsr         IO_FD_CHECK
            bcs         @done
            lda         IO_FD_MODE,X
            and         #IO_MODE_READ
            bne         :+
            lda         #ERR_IO_MODE
            sec
            bra         @done
:
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         ZP_IO_CNT
            sta         ZP_IO_LEFT
            lda         ZP_IO_CNT + 1
            sta         ZP_IO_LEFT + 1
            stz         ZP_IO_CNT
            stz         ZP_IO_CNT + 1

@loop:
            lda         ZP_IO_LEFT
            ora         ZP_IO_LEFT + 1
            beq         @ok
            jsr         IO_SET_CHUNK
            lda         ZP_IO_CHUNK                 ; Keep the size asked for
            pha
            lda         ZP_IO_CHUNK + 1
            pha
            jsr         IO_SET_OFS
            lda         #H9_READ
            jsr         IO_SERVE
            bcs         @error
            jsr         IO_GET_DONE
            beq         @eof                        ; End of file
            jsr         IO_COPY_OUT
            jsr         IO_ADVANCE
            pla                                     ; Less than asked for: done
            cmp         ZP_IO_CHUNK + 1
            bne         @short
            pla
            cmp         ZP_IO_CHUNK
            bne         @ok
            bra         @loop

@short:
            pla
            bra         @ok

@eof:
            pla
            pla

@ok:
            clc
            bra         @unmap

@error:
            ply                                     ; (keep the error in .A)
            ply
            sec

@unmap:
            _M_IO_UNMAP

@done:
            PULL_YX
            rts

; Write to an fd.  Returns when the count is done, or when the server takes nothing (a server that takes
; less than offered, like the console when its TX ring fills up, is offered the rest again).
; IN: .A = fd, ZP_IO_BUF = buffer, ZP_IO_CNT = bytes to write
; OUT (success): ZP_IO_CNT = bytes written, C = 0
; OUT (failure): .A = error, C = 1 (ZP_IO_CNT = bytes written before the error)
IO_WRITE:
            PUSH_XY
            jsr         IO_FD_CHECK
            bcs         @done
            lda         IO_FD_MODE,X
            and         #IO_MODE_WRITE
            bne         :+
            lda         #ERR_IO_MODE
            sec
            bra         @done
:
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         ZP_IO_CNT
            sta         ZP_IO_LEFT
            lda         ZP_IO_CNT + 1
            sta         ZP_IO_LEFT + 1
            stz         ZP_IO_CNT
            stz         ZP_IO_CNT + 1

@loop:
            lda         ZP_IO_LEFT
            ora         ZP_IO_LEFT + 1
            beq         @ok
            jsr         IO_SET_CHUNK
            jsr         IO_COPY_IN
            jsr         IO_SET_OFS
            lda         #H9_WRITE
            jsr         IO_SERVE
            bcs         @unmap
            jsr         IO_GET_DONE
            beq         @ok                         ; The server took nothing
            jsr         IO_ADVANCE
            bra         @loop

@ok:
            clc

@unmap:
            _M_IO_UNMAP

@done:
            PULL_YX
            rts

; Read one byte.
; IN: .X = fd.  OUT (success): .A = byte, C = 0.  OUT (failure): .A = error (ERR_IO_EOF at end of file), C = 1
; Uses ZP_IO_BUF, ZP_IO_CNT
IO_GETC:
            lda         #<ZP_IO_BYTE
            sta         ZP_IO_BUF
            stz         ZP_IO_BUF + 1
            lda         #1
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            txa
            jsr         IO_READ
            bcs         @done
            lda         ZP_IO_CNT
            beq         @eof
            lda         ZP_IO_BYTE
            clc
            rts

@eof:
            lda         #ERR_IO_EOF
            sec

@done:
            rts

; Write one byte.
; IN: .X = fd, .A = byte.  OUT: C = 0; or .A = error, C = 1 (ERR_IO_EOF if the server took nothing)
; Uses ZP_IO_BUF, ZP_IO_CNT
IO_PUTC:
            sta         ZP_IO_BYTE
            lda         #<ZP_IO_BYTE
            sta         ZP_IO_BUF
            stz         ZP_IO_BUF + 1
            lda         #1
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            txa
            jsr         IO_WRITE
            bcs         @done
            lda         ZP_IO_CNT
            beq         @full
            lda         ZP_IO_BYTE
            clc
            rts

@full:
            lda         #ERR_IO_EOF
            sec

@done:
            rts

; Set an fd's offset (for the next read or write).
; IN: .A = fd, ZP_IO_OFS = 32-bit offset.  OUT: C = 0; or .A = ERR_IO_BAD_FD, C = 1
IO_SEEK:
            PUSH_XY
            jsr         IO_FD_CHECK
            bcs         @done
            ldy         #0

@copy:
            lda         ZP_IO_OFS,Y
            sta         IO_FD_OFS,X
            inx
            iny
            cpy         #4
            bne         @copy
            clc

@done:
            PULL_YX
            rts

; Get a 16-byte stat block from an fd's server.
; IN: .A = fd, ZP_IO_BUF = 16-byte buffer.  OUT: C = 0; or .A = error, C = 1
IO_STAT:
            PUSH_XY
            jsr         IO_FD_CHECK
            bcs         @done
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         #H9_STAT
            jsr         IO_SERVE
            bcs         @unmap
            lda         #16
            sta         ZP_IO_CHUNK
            jsr         IO_COPY_OUT
            clc

@unmap:
            _M_IO_UNMAP

@done:
            PULL_YX
            rts

; Device-specific control (e.g. a serial port's baud rate).
; IN: .A = fd, .X = control code, .Y = argument.  OUT: C = 0 and .A from the server; or .A = error, C = 1
IO_CTL:
            PUSH_XY
            stx         ZP_IO_TMP
            sty         ZP_IO_MODE
            jsr         IO_FD_CHECK
            bcs         @done
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         ZP_IO_TMP
            ldy         #IO_BLK_CTL_CODE
            sta         (ZP_IO_XFER),Y
            lda         ZP_IO_MODE
            iny
            sta         (ZP_IO_XFER),Y
            lda         #H9_CTL
            jsr         IO_SERVE
            _M_IO_UNMAP

@done:
            PULL_YX
            rts

; ****************************************************************************
; Task support: standard fds, closing everything, and fds inherited by new tasks

S_DEV_CONS:     .byte "/dev/cons", 0

; Open fds 0, 1 and 2 (stdin, stdout, stderr) on /dev/cons, for a shell.  The task's fds must be closed.
; OUT: C = 0; or .A = error, C = 1
IO_STD_OPEN:
            PUSH_XY
            ldy         #3

@open:
            phy
            lda         #<S_DEV_CONS
            ldy         #>S_DEV_CONS
            ldx         #IO_MODE_RDWR
            jsr         IO_OPEN
            ply
            bcs         @done
            dey
            bne         @open

@done:
            PULL_YX
            rts

; Close all of the task's fds (MM_TASK_RESET, when a task ends).  Modifies: .A, .X
IO_CLOSE_ALL:
            ldx         #IO_MAX_FDS - 1

@close:
            txa
            jsr         IO_CLOSE                    ; (Closed fds: ERR_IO_BAD_FD, ignored)
            dex
            bpl         @close
            clc
            rts

; Tell fd .A's server that another fd refers to its fid now (H9_DUP).  OUT: C = 0; or .A = error, C = 1
; Modifies: .A, .X, .Y
IO_DUP_SEND:
            sta         ZP_IO_FD
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            lda         #H9_DUP
            jsr         IO_SERVE
            _M_IO_UNMAP
            rts

; A free fd.  OUT: .A = fd, C = 0; or .A = ERR_IO_NO_FDS, C = 1.  Modifies: .X
IO_FD_FREE:
            ldx         #0

@fd:
            lda         IO_FD_SERVER,X
            cmp         #IO_FD_CLOSED
            beq         @free
            txa
            clc
            adc         #IO_FD_SIZE
            tax
            cpx         #IO_MAX_FDS * IO_FD_SIZE
            bne         @fd
            lda         #ERR_IO_NO_FDS
            sec
            rts

@free:
            txa                                     ; fd = offset / 8
            lsr
            lsr
            lsr
            clc
            rts

; Copy fd .A's entry to fd ZP_IO_TMP, with mode .X, and tell the server (H9_DUP).
; OUT: C = 0; or .A = error, C = 1 (the new fd is closed again)
IO_FD_COPY:
            phx
            asl
            asl
            asl
            tax                                     ; .X = the fd's entry
            lda         ZP_IO_TMP
            asl
            asl
            asl
            tay                                     ; .Y = the new fd's entry
            lda         #IO_FD_SIZE
            sta         ZP_IO_CHUNK

@copy:
            lda         IO_FD_TABLE,X
            sta         IO_FD_TABLE,Y
            inx
            iny
            dec         ZP_IO_CHUNK
            bne         @copy
            tya
            sec
            sbc         #IO_FD_SIZE
            tay
            pla
            sta         IO_FD_MODE,Y
            lda         ZP_IO_TMP
            jsr         IO_DUP_SEND
            bcc         @done
            pha
            lda         ZP_IO_TMP                   ; The server said no: no new fd
            asl
            asl
            asl
            tax
            lda         #IO_FD_CLOSED
            sta         IO_FD_SERVER,X
            pla
            sec

@done:
            rts

; Make fd .X refer to the same file as fd .A (closing .X first if it's open): e.g. .X = 1 redirects
; stdout.  IN: .A = fd, .X = new fd.  OUT: C = 0; or .A = error, C = 1
IO_DUP2:
            PUSH_XY
            cpx         #IO_MAX_FDS
            bcs         @bad
            stx         ZP_IO_TMP
            pha
            jsr         IO_FD_CHECK                 ; .X = its entry
            ply
            bcs         @done
            cpy         ZP_IO_TMP
            bne         :+
            clc                                     ; The same fd: nothing to do
            bra         @done
:
            lda         IO_FD_MODE,X
            pha
            lda         ZP_IO_TMP
            jsr         IO_CLOSE                    ; (If it's open)
            plx                                     ; .X = the mode
            tya
            jsr         IO_FD_COPY
            bra         @done

@bad:
            lda         #ERR_IO_BAD_FD
            sec

@done:
            PULL_YX
            rts

S_DEV_PIPE:     .byte "/dev/pipe", 0

; Make a pipe: what's written to one fd can be read from the other.
; OUT: .A = the read fd, .X = the write fd, C = 0; or .A = error, C = 1
IO_PIPE:
            phy
            lda         #<S_DEV_PIPE
            ldy         #>S_DEV_PIPE
            ldx         #IO_MODE_READ
            jsr         IO_OPEN                     ; The read end: a new pipe
            bcs         @done
            pha
            jsr         IO_FD_FREE                  ; The write end
            bcs         @close
            sta         ZP_IO_TMP
            pla
            pha
            ldx         #IO_MODE_WRITE
            jsr         IO_FD_COPY
            bcs         @close
            ldx         ZP_IO_TMP
            pla
            clc
            bra         @done

@close:
            tax                                     ; (The error)
            pla
            jsr         IO_CLOSE
            txa
            sec

@done:
            ply
            rts

; Give a new task copies of the current task's open fds (TASK_BUILD_FRAME), telling each server (H9_DUP).
; The fd table goes through the current task's IO transfer area (tasks can't see each other's RAM), and
; the new task copies it in (IO_ADOPT_FDS, run in it with TASK_CALL).  IRQs must be off.
; IN: .A = the new task.  Preserves .X, .Y
IO_INHERIT:
            PUSH_XY
            pha
            ldy         #IO_MAX_FDS - 1             ; Any open?  (If not, the new task's are all closed
            lda         #IO_FD_CLOSED               ;   already: it's a free task)
            sta         ZP_IO_TMP                   ; ZP_IO_TMP = $FF: none open yet

@fd:
            tya
            asl
            asl
            asl
            tax
            lda         IO_FD_SERVER,X
            cmp         #IO_FD_CLOSED
            beq         :+
            sty         ZP_IO_TMP
            phy
            tya
            jsr         IO_DUP_SEND                 ; (A server that says no: the new task has the fd anyway)
            ply
:
            dey
            bpl         @fd
            pla
            ldx         ZP_IO_TMP
            bmi         @done                       ; None open
            sta         ZP_IO_TMP

@copy:
            jsr         IO_XFER_SETUP
            _M_IO_MAP_XFER
            ldy         #IO_MAX_FDS * IO_FD_SIZE - 1

@byte:
            lda         IO_FD_TABLE,Y
            sta         (ZP_IO_DATA),Y
            dey
            bpl         @byte
            lda         ZP_IO_TMP
            sta         ZP_TC_TASK
            LOAD_ADDR   ::IO_ADOPT_FDS, ZP_TC_VEC   ; (Its page 0 gate)
            lda         T_REGISTER
            and         #$0F                        ; .A = this task
            jsr         TASK_CALL
            _M_IO_UNMAP

@done:
            PULL_YX
            rts

; Runs in the new task: copy the fd table from the parent's IO transfer area.  IN: .A = the parent
IO_ADOPT_FDS:
            asl                                     ; Its data area: $8000 + task * $200 + $100
            ora         #>(PAGED_RAM_BASE + IO_BLK_DATA)
            sta         ZP_IO_DATA + 1
            stz         ZP_IO_DATA
            _M_IO_MAP_XFER
            ldy         #IO_MAX_FDS * IO_FD_SIZE - 1

@byte:
            lda         (ZP_IO_DATA),Y
            sta         IO_FD_TABLE,Y
            dey
            bpl         @byte
            _M_IO_UNMAP
            clc
            rts

; ****************************************************************************
; The IO layer's own devices.  They run in the calling task (IO_DEV_CALLER_TASK).

; /dev/null: reads are empty (end of file), writes take everything
NULL_SERVE:
            cmp         #H9_READ
            beq         @read
            cmp         #H9_STAT
            beq         @stat
            cmp         #H9_CTL
            beq         @bad

@ok:                                                ; H9_OPEN (fid 0), H9_WRITE (count stays), H9_CLUNK
            lda         #0
            clc
            rts

@read:
            jsr         IO_SRV_MAP
            lda         #0
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            bra         @ok

@stat:
            jsr         STAT_ZERO
            bra         @ok

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; /dev/zero: reads return zeros, writes take everything
ZERO_SERVE:
            cmp         #H9_READ
            beq         @read
            cmp         #H9_STAT
            beq         @stat
            cmp         #H9_CTL
            beq         @bad

@ok:                                                ; H9_OPEN (fid 0), H9_WRITE (count stays), H9_CLUNK
            lda         #0
            clc
            rts

@read:                                              ; count bytes of zeros (the count stays)
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_IO_TMP
            inc         ZP_IO_REQ + 1               ; The data area
            ldy         #0
            lda         #0
:
            sta         (ZP_IO_REQ),Y
            iny
            cpy         ZP_IO_TMP                   ; (256: 0, so .Y wraps round to it)
            bne         :-
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            bra         @ok

@stat:
            jsr         STAT_ZERO
            bra         @ok

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; An all-zero stat block (size 0).  IN: .X = client
STAT_ZERO:
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1               ; The data area
            ldy         #15
            lda         #0
:
            sta         (ZP_IO_REQ),Y
            dey
            bpl         :-
            dec         ZP_IO_REQ + 1
            jmp         IO_SRV_UNMAP
