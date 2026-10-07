.debuginfo

; ****************************************************************************
; env: each task's environment, as files (like Plan 9's /env).  BIOS ROM page 9, included inside `.scope
; PAGE9` (see all.s).  It runs in the client's task (IO_DEV_CALLER_TASK); the shell registers it and mounts it
; at /env (SH_BOOT).
;   /env            read: the variables, a line each: "NAME=value" and CR LF
;   /env/NAME       read: its value.  Write: its value: a write at the file's start replaces it, one after
;                   that adds to it; CRs and LFs are left out (so echo x > /env/NAME gives it "x").  Create (a
;                   > redirection): made, or emptied.  Remove: gone
; The blocks, the slots and the buffers are in the system's shared bank (ENV_BLOCKS ...: io.inc), seen at
; $8000 with RAM_BANK_REG = SYS_BANK; a request's data area is in the IO transfer bank (IO_XFER_BANK), seen
; at the same place: between the two, the bank register goes back and forth (U = 0 for both: IO_SRV_MAP).
; A request holds the CPU (NO_PREEMPT): the slots and buffers are shared by every task.
; Server ZP: ZP_ENV_P (a block), ZP_ENV_Q (a slot), and ZP_PROC_* for scratch (/dev/proc's: a task makes one
; request at a time).

.segment "SYS_P9"

ENV_OWN         = ZP_PROC_OWN                               ; An entry's start in its block
ENV_VAL         = ZP_PROC_IDX                               ; Its value's start
ENV_END         = ZP_PROC_FG                                ; A block's end (its final 0)
ENV_N           = ZP_PROC_LEN                               ; The request; a length
            CS_FITS     ENV_OWN, 1
            CS_FITS     ENV_VAL, 1
            CS_FITS     ENV_END, 1
            CS_FITS     ENV_N, 1

; A request.  IN: .A = request, .X = client, .Y = fid
ENV_SERVE:
            jsr         NO_PREEMPT
            jsr         @request
            jmp         PREEMPT                             ; (Keeps .A and C)

@request:
            cmp         #H9_OPEN
            beq         ENV_OPEN
            cmp         #H9_CREATE
            beq         ENV_OPEN
            cmp         #H9_REMOVE
            beq         ENV_OPEN
            cmp         #H9_READ
            bne         :+
            jmp         ENV_READ
:
            cmp         #H9_WRITE
            bne         :+
            jmp         ENV_WRITE
:
            cmp         #H9_STAT
            bne         :+
            jsr         STAT_ZERO
            bra         ENV_OK
:
            cmp         #H9_CLUNK
            beq         ENV_CLUNK
            cmp         #H9_DUP
            beq         ENV_DUP
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; H9_CLUNK and H9_DUP: the fds sharing a variable's slot
ENV_CLUNK:
            jsr         ENV_SLOT_AT                         ; (The list: nothing to do)
            bcs         ENV_OK
            ldy         #ENV_S_REFS
            lda         (ZP_ENV_Q),Y
            dec
            sta         (ZP_ENV_Q),Y
            bne         ENV_UNMAP_OK
            lda         #$FF                                ; The last one: the slot is free
            sta         (ZP_ENV_Q)
            bra         ENV_UNMAP_OK

ENV_DUP:
            jsr         ENV_SLOT_AT
            bcs         ENV_OK
            ldy         #ENV_S_REFS
            lda         (ZP_ENV_Q),Y
            inc
            sta         (ZP_ENV_Q),Y

ENV_UNMAP_OK:
            jsr         IO_SRV_UNMAP

ENV_OK:
            lda         #0
            clc
            rts

; ZP_ENV_Q = fid .Y's slot (IO_SRV_MAP, and the system's bank).  OUT: C = 0; or C = 1: the list's fid
; (nothing mapped).  Modifies: .A
ENV_SLOT_AT:
            tya
            bpl         @list
            jsr         IO_SRV_MAP
            jsr         ENV_SYS
            and         #ENV_SLOT_COUNT - 1                 ; ENV_SLOTS + the slot * 32
            asl
            asl
            asl
            asl
            asl
            sta         ZP_ENV_Q
            lda         #>ENV_SLOTS
            adc         #0
            sta         ZP_ENV_Q + 1
            clc
            rts

@list:
            sec
            rts

; ****************************************************************************
; Open, create, remove: "" or "/" (the list), or "/NAME"

ENV_OPEN:
            sta         ENV_N                               ; (The request)
            jsr         IO_SRV_MAP
            txa
            and         #$0F
            jsr         ENV_BLOCK                           ; The client's environment
            jsr         ENV_NAME_IN                         ; ENV_NAMEBUF = the name (.A = its length)
            bcc         @far2
            jmp         @fail
@far2:
            tax
            lda         ENV_N
            cpx         #0
            bne         @named
            cmp         #H9_OPEN                            ; The list: only to open (to read)
            beq         @far1
            jmp         @bad_name
@far1:
            lda         #0                                  ; (Its fid)
            clc
            bra         @done

@named:
            cmp         #H9_REMOVE
            beq         @remove
            cmp         #H9_CREATE
            beq         @create
            jsr         ENV_FIND                            ; Open: it has to be there
            bcc         @slot

@not_found:
            lda         #ERR_IO_NOT_FOUND
            bra         @fail

@remove:
            jsr         ENV_FIND
            bcs         @not_found
            jsr         ENV_DELETE
            lda         #0
            clc
            bra         @done

@create:                                                    ; Made, or emptied: NAME= at the end
            jsr         ENV_FIND
            bcs         :+
            jsr         ENV_DELETE
:
            stz         ENV_VALBUF                          ; (An empty value)
            jsr         ENV_ADD
            bcs         @fail

@slot:                                                      ; A free slot: this task's, and the name
            stz         ZP_ENV_Q
            lda         #>ENV_SLOTS
            sta         ZP_ENV_Q + 1

@look:
            lda         (ZP_ENV_Q)
            cmp         #$FF
            beq         @free
            lda         ZP_ENV_Q
            clc
            adc         #ENV_SLOT_SIZE
            sta         ZP_ENV_Q
            bcc         @look
            inc         ZP_ENV_Q + 1
            lda         ZP_ENV_Q + 1
            cmp         #>(ENV_SLOTS + ENV_SLOT_COUNT * ENV_SLOT_SIZE)
            bne         @look
            lda         #ERR_IO_NO_FDS                      ; (None: 16 variables open)
            bra         @fail

@free:
            lda         T_REGISTER
            and         #$0F
            sta         (ZP_ENV_Q)
            ldy         #ENV_S_REFS
            lda         #1
            sta         (ZP_ENV_Q),Y
            ldx         #0
            ldy         #ENV_S_NAME
:
            lda         ENV_NAMEBUF,X
            sta         (ZP_ENV_Q),Y
            beq         :+
            inx
            iny
            bra         :-
:
            lda         ZP_ENV_Q + 1                        ; Its fid: ENV_FID_VAR + (it - ENV_SLOTS) / 32
            lsr
            lda         ZP_ENV_Q
            ror
            lsr
            lsr
            lsr
            lsr
            ora         #ENV_FID_VAR
            clc

@done:
            jmp         IO_SRV_UNMAP                        ; (Keeps .A and C)

@bad_name:
            lda         #ERR_IO_NAME

@fail:
            sec
            bra         @done

.assert     ENV_SLOT_SIZE = 32 .and ENV_SLOT_COUNT = 16 .and ENV_SLOTS = $8000, error, "ENV_OPEN: a slot's fid from its address"

; The name in the request's data area ("", "/" or "/NAME") into ENV_NAMEBUF, zero-terminated.  IN: the
; transfer bank mapped (IO_SRV_MAP); OUT: the system's bank.  OUT: C = 0: .A = its length (0: none, the
; list); or C = 1, .A = ERR_IO_NAME (a '/' or '=' in it, or too long).  Modifies: .X, .Y
ENV_NAME_IN:
            inc         ZP_IO_REQ + 1                       ; (The data area)
            ldy         #0
            ldx         #0
            lda         (ZP_IO_REQ),Y
            beq         @end
            cmp         #'/'
            bne         @bad
            iny

@char:
            jsr         ENV_XFER
            lda         (ZP_IO_REQ),Y
            beq         @end
            cmp         #'/'
            beq         @bad
            cmp         #'='
            beq         @bad
            cpx         #ENV_NAME_MAX
            bcs         @bad
            jsr         ENV_SYS
            sta         ENV_NAMEBUF,X
            inx
            iny
            bra         @char

@end:
            jsr         ENV_SYS
            stz         ENV_NAMEBUF,X
            dec         ZP_IO_REQ + 1
            txa
            clc
            rts

@bad:
            jsr         ENV_SYS
            dec         ZP_IO_REQ + 1
            lda         #ERR_IO_NAME
            sec
            rts

; ****************************************************************************
; Read: the list, or a variable's value, from the fd's offset (PROC_TEXT_OUT: the text made in the data
; area, ZP_PROC_IDX bytes of it)

ENV_READ:
            tya
            bmi         @var
            jsr         IO_SRV_MAP                          ; The list: the client's variables
            txa
            jsr         ENV_LIST
            bra         @made

@var:                                                       ; A variable: its value
            jsr         ENV_SLOT_AT
            jsr         ENV_SLOT_FIND                       ; ZP_ENV_P = its block, ENV_VAL = its value
            ldx         #0
            bcs         @made                               ; (Removed since it was opened: nothing)
            ldy         ENV_VAL
:
            jsr         ENV_PEEK
            beq         @made
            jsr         ENV_OUT
            iny
            bne         :-

@made:
            jsr         ENV_XFER
            stx         ZP_PROC_IDX
            jmp         PROC_TEXT_OUT

; Task .A's variables, "NAME=value" and CR LF each (as much of it as fits in 255 bytes), as the text in the
; data area (the client's transfer area mapped: IO_SRV_MAP).  For /dev/proc/N/env too.  OUT: .X = its
; length, the transfer bank mapped.  Modifies: .A, .Y
ENV_LIST:
            jsr         ENV_BLOCK
            ldy         #0
            ldx         #0

@entry:
            jsr         ENV_PEEK
            beq         @done

@char:
            jsr         ENV_PEEK
            iny
            cmp         #0
            beq         @line_end
            jsr         ENV_OUT
            bra         @char

@line_end:
            lda         #ASCII_CR
            jsr         ENV_OUT
            lda         #ASCII_LF
            jsr         ENV_OUT
            bra         @entry

@done:
            rts

; ****************************************************************************
; Write: a variable's value (the list can't be written)

ENV_WRITE:
            tya
            bmi         :+
            lda         #ERR_IO_MODE
            sec
            rts
:
            jsr         ENV_SLOT_AT
            jsr         ENV_SLOT_FIND                       ; ZP_ENV_P, ENV_OWN, ENV_VAL (C = 1: gone)
            ldx         #0                                  ; ENV_VALBUF = the new value (.X: its length)
            bcs         @data                               ; (Gone: made again, with just this)
            jsr         ENV_XFER                            ; At the file's start: the data alone
            ldy         #IO_BLK_OFS
            lda         (ZP_IO_REQ),Y
            iny
            ora         (ZP_IO_REQ),Y
            iny
            ora         (ZP_IO_REQ),Y
            iny
            ora         (ZP_IO_REQ),Y
            jsr         ENV_SYS
            beq         @old_gone
            ldy         ENV_VAL                             ; After it: the old value, then the data
:
            lda         (ZP_ENV_P),Y
            beq         @old_gone
            sta         ENV_VALBUF,X
            inx
            iny
            bra         :-

@old_gone:
            jsr         ENV_DELETE

@data:                                                      ; The data, without CRs and LFs (255 at most)
            jsr         ENV_XFER
            ldy         #IO_BLK_COUNT + 1                   ; ENV_N = the count (256: 255)
            lda         (ZP_IO_REQ),Y
            beq         :+
            lda         #$FF
            bra         :++
:
            dey
            lda         (ZP_IO_REQ),Y
:
            sta         ENV_N
            ldy         #0
            inc         ZP_IO_REQ + 1

@byte:
            cpy         ENV_N
            beq         @made
            jsr         ENV_XFER
            lda         (ZP_IO_REQ),Y
            iny
            cmp         #ASCII_CR
            beq         @byte
            cmp         #ASCII_LF
            beq         @byte
            cpx         #$FF
            beq         @byte                               ; (Full: the rest is left out)
            jsr         ENV_SYS
            sta         ENV_VALBUF,X
            inx
            bra         @byte

@made:
            dec         ZP_IO_REQ + 1
            jsr         ENV_SYS
            stz         ENV_VALBUF,X
            jsr         ENV_ADD                             ; NAME=value at the end (the count stays: all taken)
            jmp         IO_SRV_UNMAP                        ; (Keeps .A and C)

; ****************************************************************************
; Blocks

; ZP_ENV_P = task .A's environment block.  Modifies: .A
ENV_BLOCK:
            and         #$0F
            clc
            adc         #>ENV_BLOCKS
            sta         ZP_ENV_P + 1
            stz         ZP_ENV_P
            rts

.assert     <ENV_BLOCKS = 0, error, "ENV_BLOCK: blocks start on a page"

; The system's bank (ENV_SYS), or the IO transfer bank (ENV_XFER), at $8000.  Preserves .A, .X, .Y; the flags
; N and Z reflect .A
ENV_SYS:
            pha
            lda         #SYS_BANK
            bra         :+

ENV_XFER:
            pha
            lda         T_REGISTER                          ; (The client's: this task's, IO_XFER_BANK + task / 4:
            and         #$0F                                ;   IO_XFER_OF)
            lsr
            lsr
            clc
            adc         #IO_XFER_BANK
:
            sta         RAM_BANK_REG
            pla
            rts

; .A = byte .Y of the block (ZP_ENV_P), and the transfer bank mapped again.  Preserves .X, .Y; N and Z
; reflect .A
ENV_PEEK:
            jsr         ENV_SYS
            lda         (ZP_ENV_P),Y
            jmp         ENV_XFER

; .A to byte .X of the text in the data area (the transfer bank mapped), and .X on by 1 (255 at most: the
; rest is left out).  Preserves .Y
ENV_OUT:
            cpx         #$FF
            beq         @done
            phy
            pha
            txa
            tay
            pla
            inc         ZP_IO_REQ + 1
            sta         (ZP_IO_REQ),Y
            dec         ZP_IO_REQ + 1
            ply
            inx

@done:
            rts

; Find ENV_NAMEBUF's variable in the block (the system's bank mapped).  OUT: C = 0: ENV_OWN = its entry's
; start, ENV_VAL = its value's start; or C = 1: it isn't there.  Modifies: .A, .X, .Y
ENV_FIND:
            ldy         #0

@entry:
            lda         (ZP_ENV_P),Y
            beq         @none                               ; (The block's end)
            sty         ENV_OWN
            ldx         #0

@char:
            lda         ENV_NAMEBUF,X
            beq         @name_end
            cmp         (ZP_ENV_P),Y
            bne         @skip
            inx
            iny
            bra         @char

@name_end:
            lda         (ZP_ENV_P),Y
            cmp         #'='
            bne         @skip
            iny
            sty         ENV_VAL
            clc
            rts

@skip:                                                      ; On past this entry's 0
            lda         (ZP_ENV_P),Y
            iny
            cmp         #0
            bne         @skip
            bra         @entry

@none:
            sec
            rts

; A variable's slot (ZP_ENV_Q): its name into ENV_NAMEBUF, ZP_ENV_P = its task's block, and it's found there
; (ENV_FIND).  OUT: as ENV_FIND.  Modifies: .A, .X, .Y
ENV_SLOT_FIND:
            ldx         #0
            ldy         #ENV_S_NAME
:
            lda         (ZP_ENV_Q),Y
            sta         ENV_NAMEBUF,X
            beq         :+
            inx
            iny
            bra         :-
:
            lda         (ZP_ENV_Q)
            jsr         ENV_BLOCK
            bra         ENV_FIND

; Delete the entry at ENV_OWN (ENV_FIND's): the rest of the block moves down over it.  Modifies: .A, .X, .Y
ENV_DELETE:
            ldy         ENV_OWN                             ; Past its 0: .Y = the next entry
:
            lda         (ZP_ENV_P),Y
            iny
            cmp         #0
            bne         :-
            tya                                             ; ZP_ENV_Q = the block + (the next - this): what
            sec                                             ;   comes after it, seen from where it starts
            sbc         ENV_OWN
            sta         ZP_ENV_Q
            lda         ZP_ENV_P + 1
            sta         ZP_ENV_Q + 1
            tya                                             ; .X = the bytes after it (to the block's end)
            eor         #$FF
            tax
            inx
            ldy         ENV_OWN
:
            lda         (ZP_ENV_Q),Y
            sta         (ZP_ENV_P),Y
            iny
            dex
            bne         :-
            rts

; Add ENV_NAMEBUF=ENV_VALBUF at the block's end (the system's bank mapped).  OUT: C = 0; or C = 1,
; .A = ERR_IO_FULL (no room: the block's 256 bytes).  Modifies: .A, .X, .Y
ENV_ADD:
            ldy         #0                                  ; ENV_END = the block's final 0
@entry:
            lda         (ZP_ENV_P),Y
            beq         @end
:
            lda         (ZP_ENV_P),Y
            iny
            cmp         #0
            bne         :-
            bra         @entry

@end:
            sty         ENV_END
            ldx         #0                                  ; Room?  The end + the name + = + the value + 0,
:                                                           ;   and the final 0
            lda         ENV_NAMEBUF,X
            beq         :+
            inx
            bra         :-
:
            stx         ENV_N
            ldx         #0
:
            lda         ENV_VALBUF,X
            beq         :+
            inx
            bra         :-
:
            txa
            clc
            adc         ENV_N
            bcs         @full
            adc         ENV_END
            bcs         @full
            adc         #3
            bcs         @full
            ldy         ENV_END                             ; NAME=value, 0, 0
            ldx         #0
:
            lda         ENV_NAMEBUF,X
            beq         :+
            sta         (ZP_ENV_P),Y
            inx
            iny
            bra         :-
:
            lda         #'='
            sta         (ZP_ENV_P),Y
            iny
            ldx         #0
:
            lda         ENV_VALBUF,X
            sta         (ZP_ENV_P),Y
            iny
            inx
            cmp         #0
            bne         :-
            sta         (ZP_ENV_P),Y                        ; (.A = 0: the block's end)
            clc
            rts

@full:
            lda         #ERR_IO_FULL
            sec
            rts

; ****************************************************************************
; Tasks' environments

; A new task (.A) gets a copy of this task's environment (IO_INHERIT, as it starts).  Preserves .A, .X
ENV_COPY:
            pha
            sta         ZP_ENV_Q + 1                        ; (The new task, while the bank's mapped)
            _M_SYS_ENTER
            lda         ZP_ENV_Q + 1
            jsr         ENV_BLOCK                           ; (Its block: into ZP_ENV_Q)
            lda         ZP_ENV_P + 1
            sta         ZP_ENV_Q + 1
            stz         ZP_ENV_Q
            lda         T_REGISTER                          ; ... from this one's
            jsr         ENV_BLOCK
            ldy         #0
:
            lda         (ZP_ENV_P),Y
            sta         (ZP_ENV_Q),Y
            iny
            bne         :-
            _M_SYS_LEAVE
            pla
            rts

; /dev/proc/N/mem's numbers, counted in task N (TASK_CALL, from PROC_READ: through its page 0 gate).
; OUT: .A = the MMU pages it has, .Y = its page floor (both 0: its MMU isn't set up).  (Not its banks: the
; map has the banks of RAM that isn't fitted marked in use too.)
PROC_MEM_COUNT:
            lda         MMU_HDR                             ; (Its status: MMU_VERSION once it's set up)
            cmp         #MMU_VERSION
            beq         :+
            lda         #0
            tax
            tay
            rts
:
            ldx         #MMU_BANK_MAP - MMU_PAGE_MAP - 1    ; The pages' bits (the map; the banks' is next)
            ldy         #0
:
            lda         MMU_PAGE_MAP,X
            jsr         @bits
            dex
            bpl         :-
            tya                                             ; (Less the pages that are always marked: below
            sec                                             ;   MMU_PAGE_BOTTOM, and from the task system
            sbc         #MMU_PAGE_BOTTOM + $80 - MMU_SYS_PAGE ;   page up)
            ldy         MMU_PAGE_FLOOR
            rts

@bits:                                                      ; .Y + the 1 bits in .A
            asl
            bcc         :+
            iny
:
            cmp         #0
            bne         @bits
            rts

; At boot (SH_BOOT, in the boot shell): no variables open, and this task's environment empty (the tasks it
; starts get copies of it).  Modifies: .A, .X, .Y
ENV_INIT:
            _M_SYS_ENTER
            ldx         #0                                  ; The slots: all free
            lda         #$FF
:
            sta         ENV_SLOTS + ENV_S_TASK,X
            sta         ENV_SLOTS + 256 + ENV_S_TASK,X
            pha
            txa
            clc
            adc         #ENV_SLOT_SIZE
            tax
            pla
            bcc         :-
            lda         T_REGISTER
            jsr         ENV_BLOCK
            lda         #0
            sta         (ZP_ENV_P)
            _M_SYS_LEAVE
            rts

.assert     ENV_SLOT_COUNT * ENV_SLOT_SIZE = 512, error, "ENV_INIT: the slots are two pages"
