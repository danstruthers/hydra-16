; ****************************************************************************
; HydraFS's requests, walking and reading (in hfs.s: the storage driver's second bank), ported from the old OS's
; fs/hfs_srv.s.  Its format and what it does are docs/plans/HYDRAFS.md's.
;   #f                  (no spec) the cards: a directory of those started, 0-f; N/a/b is card N's file a/b.  The
;                       disks in memory aren't here: only through a spec
;   #f with a spec      one disk's files: x (the ROM disk), r, s, or a card's (0-f); or a directory on it (r/5:
;                       the RAM disk's directory 5, as the root)
;   Names               case-sensitive, 1-31 bytes (any but '/' and 0); the kernel has taken . and .. out already
;   Reading a directory its entries' stat records (SR_SIZE bytes each, whole ones, from a record's start), made
;                       again at every read, so nothing is kept between reads
;   Writing             at the fd's offset (an append-only file's end), up to the end and past it (with a hole
;                       between: sparse.s); O_TRUNC empties a file as it's opened.  CREATE (a file that's there
;                       already is emptied, as in Plan 9), REMOVE, WSTAT (its name, in its directory; its mode;
;                       its length: longer with zeros, or cut short)
; A disk with no HydraFS on it gives E_NOTFS.
;
; What reaches the disk when: a file's data at once, a block at a time through the block buffer (storage.s's blk);
; its directory entry when the file gets a cluster, and when the last fid on it is clunked (so after a crash a file
; being written can be shorter than what was written, but no cluster is ever lost or used twice); the free map,
; extent blocks and entries in that order, through the metadata buffer, by the end of each request.

.segment "CODE2"

HFS_MAGIC:  .byte   "HYDRAFS1"
HFS_NAMES:  .byte   "0123456789abcdefxrs"                   ; (A disk's name, by its number)

; A request for #f (storage.s's h_fs, through FAR2).  IN: .A = the request (R_*), and TASK_INBOX, TASK_PATH.
; OUT: C = 0; or C = 1, .A = the error.  What it changed goes to the disk first (HFS_FINISH); one that can change a
; directory (a create, a remove, a wstat: a rename) forgets its disk's names in the walk cache, whether it did or not
hfs_serve:
            sta         HFS_RQ
            lda         TASK_INBOX + RQ_FID
            sta         SD_FID
            lda         TASK_INBOX + RQ_MODE
            sta         SD_OP
            lda         HFS_RQ
            jsr         HFS_REQUEST
            php
            pha
            lda         HFS_RQ
            cmp         #R_CREATE
            beq         :+
            cmp         #R_REMOVE
            beq         :+
            cmp         #R_WSTAT
            bne         :++
:
            lda         HFS_CARD                            ; (Its disk's names forgotten: the walk cache)
            jsr         HFS_WC_FORGET
:
            pla
            plp
            jmp         HFS_FINISH

HFS_REQUEST:
            ldx         #HFS_NREQ - 1
:
            cmp         HFS_REQS,X
            beq         :+
            dex
            bpl         :-
            lda         #E_NOSYS
            sec
            rts
:
            txa
            asl
            tax
            jmp         (HFS_REQVEC,X)

HFS_REQS:   .byte       R_OPEN, R_CREATE, R_READ, R_WRITE, R_CLUNK, R_STAT, R_WSTAT, R_REMOVE, R_FLUSH, R_DUP
HFS_NREQ    = * - HFS_REQS
HFS_REQVEC: .word       HFS_OPEN_REQ, HFS_CREATE_REQ, HFS_READ_REQ, HFS_WRITE_REQ, HFS_CLUNK, HFS_STAT_REQ
            .word       HFS_WSTAT_REQ, HFS_REMOVE_REQ, HFS_OK, HFS_DUP

; ****************************************************************************
; The requests

; R_CLUNK, and R_DUP (the same fid again: the fids on an open file share it).  The last fid gone: the slot is free
; again, and if the file changed, its entry goes to the disk (from the slot's copy: it's still there)
HFS_CLUNK:
            lda         SD_FID
            cmp         #HFS_FID_DISKS
            beq         HFS_OK
            jsr         HFS_FID_CHECK
            bcs         HFS_RET
            dec         HFS_H_REFS,X
            bne         HFS_OK
            lda         #$FF
            sta         HFS_H_CARD,X
            lda         HFS_H_FLAGS,X
            bpl         HFS_OK                              ; (HFS_HF_DIRTY)
            jmp         HFS_ENT_PUT

HFS_OK:
            lda         #0
            clc
HFS_RET:
            rts

HFS_DUP:
            lda         SD_FID
            cmp         #HFS_FID_DISKS
            bne         :+
            jmp         HFS_DISKS_OPENED
:
            jsr         HFS_FID_CHECK
            bcs         HFS_RET
            inc         HFS_H_REFS,X
            lda         HFS_FID
            jmp         HFS_OPENED

; R_OPEN: walk the name (HFS_NAME: the spec's and the request's), then take an open file slot
HFS_OPEN_REQ:
            jsr         HFS_NAME
            bcs         HFS_RET
            ldy         #1
            lda         (HFS_NM),Y
            bne         :+
            jmp         HFS_OPEN_DISKS                      ; ("/" alone: the cards' directory)
:
            jsr         HFS_WALK
            bcs         HFS_RET
            jsr         HFS_PARENT_LOC                      ; HFS_PLOC = the directory it's in
            jmp         HFS_TAKE_SLOT

; The cards' directory (HFS_FID_DISKS): for reading
HFS_OPEN_DISKS:
            lda         SD_OP
            and         #O_RW_MASK | O_TRUNC
            beq         HFS_DISKS_OPENED
            lda         #E_ISDIR
            sec
            rts

HFS_DISKS_OPENED:
            lda         #HFS_FID_DISKS
            sta         TASK_INBOX + RQ_FID
            lda         #QT_DIR
            sta         TASK_INBOX + RQ_PERM
            clc
            rts

; An open's answer: fid .A, and its qid type (the entry at HFS_FP's: QT_DIR for a directory).  OUT: C = 0
HFS_OPENED:
            sta         TASK_INBOX + RQ_FID
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            and         #HFS_M_DIR                          ; (QT_DIR)
            sta         TASK_INBOX + RQ_PERM
            clc
            rts

; HFS_NM -> HFS_PATH: the request's name (TASK_PATH, after its mount point) with its mount's spec before it:
; "/N/a/b" (N the disk's name), or "/" alone (the cards' directory: no spec, no name).  HFS_SPEC <> 0: a spec.
; OUT: C = 0; or C = 1, .A = E_NAMETOOLONG
HFS_NAME:
            LDR         HFS_NM, HFS_PATH
            lda         #'/'
            sta         HFS_PATH
            ldx         #1
            ldy         #0
@spec:
            lda         TASK_INBOX + RQ_SPEC,Y              ; The spec (8 at most, zero-padded)
            beq         @specd
            sta         HFS_PATH,X
            inx
            iny
            cpy         #8
            bne         @spec
@specd:
            sty         HFS_SPEC
            tya
            beq         :+
            lda         #'/'                                ; (A spec: a / after it)
            sta         HFS_PATH,X
            inx
:
            ldy         #0
:
            lda         TASK_PATH,Y                         ; The name, past its first /
            cmp         #'/'
            bne         @name
            iny
            bra         :-
@name:
            lda         TASK_PATH,Y
            sta         HFS_PATH,X
            beq         @named
            inx
            iny
            cpx         #HFS_PATH_MAX - 1
            bcc         @name
            lda         #E_NAMETOOLONG
            sec
            rts

@named:
            dex                                             ; A / at its end (a spec's, with no name after it):
            beq         @done                               ;   off (but "/" alone)
            lda         HFS_PATH,X
            cmp         #'/'
            bne         @done
            stz         HFS_PATH,X
@done:
            clc
            rts

; Is disk HFS_CARD the ROM disk (read only)?  OUT: C = 1 and .A = E_ROFS if it is; C = 0 if not.  Modifies: .A
HFS_RO_DISK:
            lda         HFS_CARD
            cmp         #DISK_X
            bne         :+
            lda         #E_ROFS
            sec
            rts
:
            clc
            rts

; HFS_PLOC = where the entry a walk ended at is listed: in the walk's last directory (HFS_STK); for a
; disk's root, which isn't in one, its own place.  Modifies: .A, .X, .Y
HFS_PARENT_LOC:
            lda         HFS_DEPTH
            beq         @root
            jsr         HFS_STK_AT
            ldy         #0
:
            lda         HFS_STK,X
            sta         HFS_PLOC,Y
            inx
            iny
            cpy         #5
            bne         :-
            rts

@root:
            ldx         #4
:
            lda         HFS_LOC,X
            sta         HFS_PLOC,X
            dex
            bpl         :-
            rts

; Open the entry a walk found (HFS_ENT, at HFS_LOC, listed in the directory at HFS_PLOC) with the open mode
; SD_OP: check the mode suits it, empty it for O_TRUNC, and take an open file slot for it.  (The mode bits are
; checked here, when it's opened, as in Plan 9: a file made read-only while it's open for writing can still be
; written through that fid.)
; OUT: C = 0: the answer (RQ_FID, RQ_PERM); or C = 1, .A = E_ROFS, E_ISDIR, E_PERM, E_NFILE or a disk error
HFS_TAKE_SLOT:
            lda         SD_OP
            and         #O_RW_MASK | O_TRUNC
            beq         HFS_TAKE_NEW
            jsr         HFS_RO_DISK                         ; Writing: not on the ROM disk ...
            bcs         @done
            lda         HFS_ENT + HFS_E_MODE                ;   not a directory ...
            bmi         @isdir
            and         #HFS_M_RO                           ;   nor a read-only file
            bne         @perm
            lda         SD_OP
            and         #O_TRUNC
            beq         HFS_TAKE_NEW
            lda         SD_OP                               ; (Emptying it is writing it)
            and         #O_RW_MASK
            beq         @perm
            jsr         HFS_TRUNCATE                        ; (HFS_FP -> HFS_ENT, from the walk)
            bcc         HFS_TAKE_NEW
@done:
            rts

@isdir:
            lda         #E_ISDIR
            sec
            rts

@perm:
            lda         #E_PERM
            sec
            rts

; A file just made comes here, past the mode check: the fid that made it can write it, even if its mode
; says read-only
HFS_TAKE_NEW:
            jsr         HFS_FID_NEW                         ; A free slot: HFS_FID, .X = it
            bcs         @done
            lda         HFS_CARD
            sta         HFS_H_CARD,X
            lda         SD_OP
            sta         HFS_H_OMODE,X
            lda         #1
            sta         HFS_H_REFS,X
            stz         HFS_H_FLAGS,X
            lda         HFS_LOC + 4
            sta         HFS_H_EIDX,X
            lda         HFS_PLOC + 4
            sta         HFS_H_PIDX,X
            txa                                             ; Where its entry is, and its directory's (at the
            asl                                             ;   fid * 4)
            asl
            tax
            ldy         #0
:
            lda         HFS_LOC,Y
            sta         HFS_H_EBLK,X
            lda         HFS_PLOC,Y
            sta         HFS_H_PBLK,X
            inx
            iny
            cpy         #4
            bne         :-
            jsr         HFS_FILE_PTR                        ; And the entry itself, so a read needs no
            ldy         #HFS_ENTRY_SIZE - 1                 ;   block read to find the file's extents
:
            lda         HFS_ENT,Y
            sta         (HFS_FP),Y
            dey
            bpl         :-
            lda         HFS_FID
            jmp         HFS_OPENED

@done:
            rts

; R_STAT: the fid's copy of its directory entry, as a stat record (or the cards' directory's)
HFS_STAT_REQ:
            lda         SD_FID
            cmp         #HFS_FID_DISKS
            bne         :+
            lda         #'/'
            jsr         HFS_DIR_REC
            bra         @send
:
            jsr         HFS_FID_CHECK
            bcs         @done
            lda         HFS_FP
            sta         HFS_PTR
            lda         HFS_FP + 1
            sta         HFS_PTR + 1
            jsr         HFS_REC
@send:
            LDR         r0, HFS_STAT
            MOVR        r1, TASK_INBOX + RQ_BUF
            LDR         r2, SR_SIZE
            MOVR        TASK_INBOX + RQ_DONE, r2
            jsr         CLIENT_WRITE
            clc
@done:
            rts

; R_READ: a file's bytes at the request's offset, or a directory's stat records (or the cards' directory's)
HFS_READ_REQ:
            jsr         HFS_REQ_ARGS
            lda         SD_FID
            cmp         #HFS_FID_DISKS
            bne         :+
            jmp         HFS_DISKS_READ
:
            jsr         HFS_FID_CHECK
            bcc         :+
            rts
:
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            bpl         HFS_FILE_READ                       ; (HFS_M_DIR: its stat records instead)
            jmp         HFS_DIR_READ

; The request's offset (SD_POS) and count (SD_LEFT: 512 at most); SD_DONE = 0.  Modifies: .A, .X
HFS_REQ_ARGS:
            ldx         #3
:
            lda         TASK_INBOX + RQ_OFFSET,X
            sta         SD_POS,X
            dex
            bpl         :-
            lda         TASK_INBOX + RQ_COUNT
            sta         SD_LEFT
            lda         TASK_INBOX + RQ_COUNT + 1
            sta         SD_LEFT + 1
            stz         SD_DONE
            stz         SD_DONE + 1
            rts

; A file: its bytes, the part of one block at a time, stopping at the end of the file
HFS_FILE_READ:
            lda         SD_LEFT
            ora         SD_LEFT + 1
            beq         HFS_READ_DONE
            jsr         HFS_AT_END                          ; End of file: that's all there is
            bcs         HFS_READ_DONE
            stz         HFS_HOLEF
            jsr         HFS_FILE_BLOCK                      ; The disk block byte SD_POS is in
            bcc         @load
            cmp         #HFS_IN_HOLE                        ; (In a hole: zeros)
            bne         HFS_READ_ERR
            dec         HFS_HOLEF
            bra         @loaded

@load:
            jsr         HFS_LOAD
            bcs         HFS_READ_ERR
@loaded:
            jsr         HFS_BLOCK_N                         ; SD_N: the bytes to this block's end ...
            stz         HFS_CL + 2                          ; ... but no more than the count wants
            stz         HFS_CL + 3
            lda         SD_LEFT
            sta         HFS_CL
            lda         SD_LEFT + 1
            sta         HFS_CL + 1
            jsr         HFS_N_MIN
            jsr         HFS_TAIL                            ; ... or than there is before the end of file
            jsr         HFS_N_MIN
            bit         HFS_HOLEF
            bmi         @zeros
            clc                                             ; r0 = the block + (SD_POS & 511) ...
            lda         SD_POS
            adc         SD_CACHE
            sta         r0
            lda         SD_POS + 1
            and         #1
            adc         SD_CACHE + 1
            sta         r0 + 1
            bra         @send

@zeros:
            jsr         HFS_ZEROED                          ;   or zeros (a hole's)
@send:
            jsr         HFS_TO_CLIENT
            jsr         HFS_ADVANCE
            bra         HFS_FILE_READ

; An error, part way: what was moved, if anything was (the next request gets the error); or the error
HFS_READ_ERR:
            ldx         SD_DONE
            bne         HFS_READ_DONE
            ldx         SD_DONE + 1
            bne         HFS_READ_DONE
            sec
            rts

; The count moved (SD_DONE): the request's answer
HFS_READ_DONE:
            lda         SD_DONE
            sta         TASK_INBOX + RQ_DONE
            lda         SD_DONE + 1
            sta         TASK_INBOX + RQ_DONE + 1
            lda         #0
            clc
            rts

; SD_N = the bytes from SD_POS to its block's end (1-512).  Modifies: .A, .X
HFS_BLOCK_N:
            lda         SD_POS + 1
            and         #1
            eor         #1                                  ; (Its high byte)
            tax
            lda         SD_POS
            eor         #$FF
            clc
            adc         #1                                  ; (Its low byte: 0 - SD_POS)
            sta         SD_N
            bne         :+
            inx                                             ; (SD_POS & 511 = 0: a whole block)
:
            stx         SD_N + 1
            rts

; SD_N bytes from r0 to the client's buffer, at SD_DONE in it
HFS_TO_CLIENT:
            clc
            lda         TASK_INBOX + RQ_BUF
            adc         SD_DONE
            sta         r1
            lda         TASK_INBOX + RQ_BUF + 1
            adc         SD_DONE + 1
            sta         r1 + 1
            MOVR        r2, SD_N
            jmp         CLIENT_WRITE

; r0 -> a block of zeros (HFS_ZBUF's, cleared).  Modifies: .A, .Y
HFS_ZEROED:
            MOVR        r0, HFS_ZBUF
            lda         #0
            tay
:
            sta         (r0),Y
            iny
            bne         :-
            inc         r0 + 1
:
            sta         (r0),Y
            iny
            bne         :-
            dec         r0 + 1
            rts

; SD_POS and SD_DONE on by SD_N, SD_LEFT down by it.  Modifies: .A
HFS_ADVANCE:
            lda         SD_POS
            clc
            adc         SD_N
            sta         SD_POS
            lda         SD_POS + 1
            adc         SD_N + 1
            sta         SD_POS + 1
            bcc         :+
            inc         SD_POS + 2
            bne         :+
            inc         SD_POS + 3
:
            lda         SD_LEFT
            sec
            sbc         SD_N
            sta         SD_LEFT
            lda         SD_LEFT + 1
            sbc         SD_N + 1
            sta         SD_LEFT + 1
            lda         SD_DONE
            clc
            adc         SD_N
            sta         SD_DONE
            lda         SD_DONE + 1
            adc         SD_N + 1
            sta         SD_DONE + 1
            rts

; ****************************************************************************
; Reading a directory: its entries' stat records, whole ones from a record's start (as srvlib's are), made again
; from the start at every read, the records before the offset skipped, so a read at any offset gives the same and
; nothing has to be remembered between reads.  Directories are small.

HFS_DIR_READ:
            jsr         HFS_DIR_SKIP                        ; HFS_SKIP: the records before the offset
            bcs         HFS_DIR_RET
            stz         SD_POS                              ; Back to the directory's first entry
            stz         SD_POS + 1
            stz         SD_POS + 2
            stz         SD_POS + 3
@entry:
            jsr         HFS_DIR_ROOM                        ; Room for another record?
            bcs         HFS_DIR_END
            jsr         HFS_AT_END                          ; The last entry: stop
            bcs         HFS_DIR_END
            jsr         HFS_FILE_BLOCK                      ; The block this entry is in
            bcs         HFS_DIR_ERR
            jsr         HFS_LOAD
            bcs         HFS_DIR_ERR
            lda         SD_POS                              ; HFS_PTR = the entry, in the block buffer
            sta         HFS_OFS
            lda         SD_POS + 1
            and         #1
            sta         HFS_OFS + 1
            jsr         HFS_AT
            lda         (HFS_PTR)
            beq         @next                               ; A free entry: nothing for it
            jsr         HFS_DIR_SKIPPED                     ; (Before the offset: skipped)
            bcc         @next
            jsr         HFS_REC                             ; Its stat record, to the client
            jsr         HFS_REC_OUT
@next:
            lda         SD_POS                              ; On to the next entry
            clc
            adc         #HFS_ENTRY_SIZE
            sta         SD_POS
            bcc         @entry
            inc         SD_POS + 1
            bne         @entry
            inc         SD_POS + 2
            bne         @entry
            inc         SD_POS + 3
            bra         @entry

HFS_DIR_END:
            jmp         HFS_READ_DONE

HFS_DIR_ERR:
            jmp         HFS_READ_ERR

HFS_DIR_RET:
            rts

; The cards' directory (HFS_FID_DISKS): a directory for each card started, 0-f
HFS_DISKS_READ:
            jsr         HFS_DIR_SKIP
            bcs         HFS_DIR_RET
            ldx         #0
@disk:
            phx
            jsr         HFS_DIR_ROOM
            plx
            bcs         HFS_DIR_END
            lda         d_state,X
            beq         @next
            phx
            jsr         HFS_DIR_SKIPPED
            bcc         :+
            lda         HFS_NAMES,X
            jsr         HFS_DIR_REC
            jsr         HFS_REC_OUT
:
            plx
@next:
            inx
            cpx         #SPI_DEVS
            bcc         @disk
            bra         HFS_DIR_END

; HFS_SKIP = the records before the request's offset (SD_POS / SR_SIZE), which must be a record's start (an
; offset past 4 MB: all of them).  OUT: C = 0; or C = 1, .A = E_INVAL (in a record)
HFS_DIR_SKIP:
            lda         SD_POS
            and         #SR_SIZE - 1
            bne         @inval
            lda         SD_POS + 2
            ora         SD_POS + 3
            bne         @past
            lda         SD_POS + 1
            sta         HFS_SKIP + 1
            lda         SD_POS
            ldx         #6
:
            lsr         HFS_SKIP + 1
            ror
            dex
            bne         :-
            sta         HFS_SKIP
            clc
            rts

@past:
            lda         #$FF                                ; (Further than a directory reaches: none)
            sta         HFS_SKIP
            sta         HFS_SKIP + 1
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

.assert     SR_SIZE = 64, error, "HFS_DIR_SKIP: a record is 64 bytes (offset >> 6)"

; Room for another record (SD_LEFT >= SR_SIZE)?  OUT: C = 0: there is.  Modifies: .A
HFS_DIR_ROOM:
            lda         SD_LEFT + 1
            bne         :+
            lda         SD_LEFT
            cmp         #SR_SIZE
            bcc         @no
:
            clc
            rts

@no:
            sec
            rts

; A record that's there: skipped (HFS_SKIP down by 1), if any are still to be.  OUT: C = 1: it's to go to the
; client; C = 0: skipped.  Modifies: .A
HFS_DIR_SKIPPED:
            lda         HFS_SKIP
            ora         HFS_SKIP + 1
            beq         @take
            lda         HFS_SKIP
            bne         :+
            dec         HFS_SKIP + 1
:
            dec         HFS_SKIP
            clc
            rts

@take:
            sec
            rts

; HFS_STAT (a record) to the client, at SD_DONE; SD_DONE up and SD_LEFT down by SR_SIZE
HFS_REC_OUT:
            LDR         r0, HFS_STAT
            LDR         SD_N, SR_SIZE
            jsr         HFS_TO_CLIENT
            clc
            lda         SD_DONE
            adc         #SR_SIZE
            sta         SD_DONE
            bcc         :+
            inc         SD_DONE + 1
:
            sec
            lda         SD_LEFT
            sbc         #SR_SIZE
            sta         SD_LEFT
            bcs         :+
            dec         SD_LEFT + 1
:
            rts

; The entry at HFS_PTR as a stat record, in HFS_STAT (SR_*): its name; its qid (its type, its version's low byte,
; its id); its mode (rw for all, or r for a read-only file and the ROM disk's; DM_DIR, DM_APPEND, as Plan 9's); its
; size (a directory's: 0); its stamp; the device (f) and the disk's name.  Modifies: .A, .X, .Y
HFS_REC:
            ldx         #SR_SIZE - 1
:
            stz         HFS_STAT,X
            dex
            bpl         :-
            ldy         #HFS_E_NAME
:
            lda         (HFS_PTR),Y
            sta         HFS_STAT + SR_NAME,Y
            iny
            cpy         #HFS_NAME_MAX + 1
            bne         :-
            ldy         #HFS_E_MODE
            lda         (HFS_PTR),Y
            pha
            and         #HFS_M_DIR | HFS_M_APPEND           ; Its qid's type, and its mode's high byte
            sta         HFS_STAT + SR_QTYPE
            sta         HFS_STAT + SR_MODE + 1
            ldy         #HFS_E_QVER
            lda         (HFS_PTR),Y
            sta         HFS_STAT + SR_QVERS
            ldy         #HFS_E_QID
            ldx         #0
:
            lda         (HFS_PTR),Y
            sta         HFS_STAT + SR_QPATH,X
            iny
            inx
            cpx         #4
            bne         :-
            ldy         #HFS_E_STAMP
            ldx         #0
:
            lda         (HFS_PTR),Y
            sta         HFS_STAT + SR_MTIME,X
            iny
            inx
            cpx         #4
            bne         :-
            lda         HFS_STAT + SR_QTYPE                 ; A file's size
            bmi         :++
            ldy         #HFS_E_SIZE
            ldx         #0
:
            lda         (HFS_PTR),Y
            sta         HFS_STAT + SR_LENGTH,X
            iny
            inx
            cpx         #4
            bne         :-
:
            pla                                             ; Its permissions
            and         #HFS_M_RO
            bne         HFS_REC_RO
            lda         HFS_CARD
            cmp         #DISK_X
            beq         HFS_REC_RO
            lda         #<$1B6                              ; (rw-rw-rw-)
            ldx         #>$1B6
            bra         HFS_REC_PERM

HFS_REC_RO:
            lda         #<$124                              ; (r--r--r--)
            ldx         #>$124
HFS_REC_PERM:
            sta         HFS_STAT + SR_MODE
            txa
            ora         HFS_STAT + SR_MODE + 1
            sta         HFS_STAT + SR_MODE + 1
            lda         #'f'
            sta         HFS_STAT + SR_DEV
            ldx         HFS_CARD
            lda         HFS_NAMES,X
            sta         HFS_STAT + SR_INST
            rts

; A directory with no entry of its own, as a stat record in HFS_STAT: named .A (one character: a card's in the
; cards' directory, or / for that directory).  Modifies: .A, .X
HFS_DIR_REC:
            ldx         #SR_SIZE - 1
:
            stz         HFS_STAT,X
            dex
            bpl         :-
            sta         HFS_STAT + SR_NAME
            sta         HFS_STAT + SR_INST
            lda         #QT_DIR
            sta         HFS_STAT + SR_QTYPE
            lda         #<$124                              ; (r--r--r--, and DM_DIR)
            sta         HFS_STAT + SR_MODE
            lda         #>$124 | DM_DIR
            sta         HFS_STAT + SR_MODE + 1
            lda         #'f'
            sta         HFS_STAT + SR_DEV
            rts

; ****************************************************************************
; Walking a name

; Walk the name at (HFS_NM) ("/N", the disk, then the path on it: HFS_NAME's) down to the file or directory it
; names.  A disk in memory (x, r, s) only if the mount had a spec (HFS_SPEC).
; OUT: C = 0: HFS_CARD = the disk, HFS_LOC = where the entry is, HFS_ENT = the entry (and HFS_FP -> it);
;      C = 1, .A = E_NOENT, E_NAMETOOLONG, E_NOTFS or a disk error
HFS_WALK:
            ldy         #0
            lda         (HFS_NM),Y                          ; "/N": the disk
            cmp         #'/'
            bne         @no_disk
            iny
            lda         (HFS_NM),Y
            ldx         #DISKS - 1                          ; (Its name: one of HFS_NAMES)
:
            cmp         HFS_NAMES,X
            beq         :+
            dex
            bpl         :-
            bra         @no_disk
:
            txa
            cmp         #SPI_DEVS                           ; (A card's, or a spec's)
            bcc         @disk
            ldx         HFS_SPEC
            bne         @disk

@no_disk:
            jmp         HFS_W_NOT_FOUND

@disk:
            sta         HFS_CARD

@path:
            iny
            lda         (HFS_NM),Y                          ; Then the path in it, or nothing: its root
            beq         :+
            cmp         #'/'
            bne         @no_disk
:
            sty         HFS_ELEM
            jsr         HFS_VOLUME                          ; The card's superblock
            bcs         HFS_W_DONE
            stz         HFS_DEPTH                           ; Start at the root, whose entry is in the
            stz         HFS_LOC                             ;   superblock (block 0)
            stz         HFS_LOC + 1
            stz         HFS_LOC + 2
            stz         HFS_LOC + 3
            lda         #HFS_SB_ROOT / HFS_ENTRY_SIZE
            sta         HFS_LOC + 4
            jsr         HFS_WC_ROOT                         ; (Its entry from the walk cache, if the walk goes on
            bcc         HFS_W_ELEM                          ;   past it; else read, and kept there)
            jsr         HFS_ENT_READ
            bcs         HFS_W_DONE
            jsr         HFS_WC_KEEP

HFS_W_ELEM:                                                 ; The next path element, if there is one
            ldy         HFS_ELEM
            lda         (HFS_NM),Y                          ; (A '/', or the name's 0)
            beq         HFS_W_DONE                          ; (C = 0 from HFS_ENT_READ: this is it)
            iny
            sty         HFS_ELEM
            lda         (HFS_NM),Y
            beq         HFS_W_DONE                          ; A trailing '/'
            cmp         #'.'
            bne         HFS_W_LOOK
            iny                                             ; "." (this directory) or ".." (the one above)?
            lda         (HFS_NM),Y
            beq         HFS_W_DOT
            cmp         #'/'
            beq         HFS_W_DOT
            cmp         #'.'
            bne         HFS_W_LOOK
            iny
            lda         (HFS_NM),Y
            beq         HFS_W_UP
            cmp         #'/'
            bne         HFS_W_LOOK

HFS_W_UP:
            sty         HFS_ELEM
            jsr         HFS_POP                             ; (At the root, it stays there)
            bcc         HFS_W_ELEM
            bra         HFS_W_DONE

HFS_W_DOT:
            sty         HFS_ELEM
            bra         HFS_W_ELEM

HFS_W_LOOK:
            lda         HFS_ENT + HFS_E_MODE
            bpl         HFS_W_NOT_FOUND                     ; Not a directory: nothing is in it (HFS_M_DIR)
            jsr         HFS_PUSH                            ; Remember it, for a ".." later
            bcs         HFS_W_DONE
            jsr         HFS_LOOKUP                          ; HFS_LOC = the element's entry
            bcs         HFS_W_DONE
            jsr         HFS_WC_ON                           ; (A directory on the way: its entry from the walk
            bcc         HFS_W_ELEM                          ;   cache; else read, and kept there)
            jsr         HFS_ENT_READ
            bcs         HFS_W_DONE
            jsr         HFS_WC_KEEP
            bra         HFS_W_ELEM

HFS_W_NOT_FOUND:
            lda         #E_NOENT
            sec

HFS_W_DONE:
            rts

; Find the path element at HFS_ELEM in the directory HFS_ENT describes (its entry at HFS_LOC), and move HFS_ELEM
; past it: from the walk cache if it was looked up there before, else by a scan of the directory (and what that
; found, there or not, kept in the cache).
; OUT: C = 0: HFS_LOC = where its entry is; or C = 1, .A = E_NOENT, E_NAMETOOLONG or a card error
HFS_LOOKUP:
            jsr         HFS_WC_FIND
            bcs         @scan
            cmp         #1                                  ; (C: 0 there, 1 not, .A E_NOENT)
            rts
@scan:
            lda         #HFS_SCAN_NAME
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcs         @none
            ldx         #4
:
            lda         HFS_NLOC,X
            sta         HFS_LOC,X
            dex
            bpl         :-
            lda         #1
            jsr         HFS_WC_PUT
            clc
            rts
@none:
            cmp         #E_NOENT                            ; (Not there: kept too; an error: not)
            bne         @done
            lda         #0
            jsr         HFS_WC_PUT
            lda         #E_NOENT
@done:
            sec
            rts

; ****************************************************************************
; The walk cache: the path elements HFS_LOOKUP has found (or found aren't there), HFS_WC_N of them, each by its disk,
; the place of its directory's entry and its name; there, the place of its entry, and a directory's entry itself
; (the root's too: a record of its own, its name empty).  So a name looked up again (a program's through /bin's
; union, a library's through /lib's, in each of its directories, there or not) costs no scan of a directory, and
; the directories on the way to it no reads of their entries' blocks (the last element's entry is read as before:
; a file's size, its times).  What can change a directory, or a directory's entry, forgets the disk's records
; (HFS_WC_FORGET: a create, a remove, a wstat, a format, a label, a disk started again or stopped); a check, all of
; them (HFS_WC_CLEAR: the records are in its buffer, HFS_CK_BUF, and it uses it all; without one, there's no cache).
; A new one takes the next record, round.

; Is the path element at HFS_ELEM, in the directory whose entry is at HFS_LOC on disk HFS_CARD, in the cache?
; OUT: C = 0: it is (HFS_WCP its record), HFS_ELEM past it, .A = 0 (there: HFS_LOC = its entry's place) or
; E_NOENT; C = 1: it isn't (HFS_ELEM as it was; its key in HFS_WCK and HFS_WCL, for HFS_WC_PUT).  Modifies: .A,
; .X, .Y
HFS_WC_FIND:
            jsr         HFS_WC_KEY
            ldy         HFS_ELEM                            ; Its length (longer than a name: HFS_DIR_SCAN's to
            ldx         #0                                  ;   say so), its characters added to the hash
:
            lda         (HFS_NM),Y
            beq         :+
            cmp         #'/'
            beq         :+
            clc
            adc         HFS_WCH
            sta         HFS_WCH
            iny
            inx
            cpx         #HFS_NAME_MAX + 1
            bne         :-
            sec
            rts
:
            stx         HFS_WCL
            jsr         HFS_WC_SEARCH
            bcs         @done
            lda         HFS_WCL                             ; HFS_ELEM past it, HFS_LEN its length (as a scan
            sta         HFS_LEN                             ;   leaves them)
            clc
            adc         HFS_ELEM
            sta         HFS_ELEM
            ldy         #HFS_WC_OK
            lda         (HFS_WCP),Y
            beq         @absent
            ldx         #0                                  ; There: its place
            ldy         #HFS_WC_LOC
:
            lda         (HFS_WCP),Y
            sta         HFS_LOC,X
            iny
            inx
            cpx         #5
            bne         :-
            lda         #0
            clc
            rts
@absent:
            lda         #E_NOENT
            clc
@done:
            rts

; HFS_WCK = the key of a name in the directory whose entry is at HFS_LOC on disk HFS_CARD, and HFS_WCH = its bytes
; added: a name's hash (its characters are added to it).  Modifies: .A, .X
HFS_WC_KEY:
            lda         HFS_CARD
            sta         HFS_WCK
            sta         HFS_WCH
            ldx         #4
:
            lda         HFS_LOC,X
            sta         HFS_WCK + 1,X
            clc
            adc         HFS_WCH
            sta         HFS_WCH
            dex
            bpl         :-
            rts

; The record of key HFS_WCK and the name at HFS_ELEM, HFS_WCL long (0: a root's), its hash HFS_WCH.  OUT: C = 0,
; HFS_WCP = it; or C = 1: none.  Modifies: .A, .X, .Y
HFS_WC_SEARCH:
            lda         HFS_CK_BUF + 1                      ; (No buffer: no cache)
            beq         @none
            ldx         #HFS_WC_N - 1
@scan:
            lda         HFS_WCH                             ; The records with its hash
:
            cmp         HFS_WC_H,X
            beq         @rec
            dex
            bpl         :-
@none:
            sec
            rts
@rec:
            stx         HFS_WCI
            jsr         HFS_WC_ADDR
            ldy         #HFS_WC_LEN                         ; (Its name's length, its disk, its directory)
            lda         (HFS_WCP),Y
            cmp         HFS_WCL
            bne         @next
            ldy         #HFS_WC_DIR + 4
:
            lda         (HFS_WCP),Y
            cmp         HFS_WCK,Y
            bne         @next
            dey
            bpl         :-
            ldx         #0                                  ; Its name
@char:
            cpx         HFS_WCL
            beq         @hit
            txa
            clc
            adc         #HFS_WC_NAME
            tay
            lda         (HFS_WCP),Y
            sta         SD_TMP
            txa
            clc
            adc         HFS_ELEM
            tay
            lda         (HFS_NM),Y
            cmp         SD_TMP
            bne         @next
            inx
            bra         @char
@next:
            ldx         HFS_WCI
            dex
            bpl         @scan
            sec
            rts
@hit:
            clc
            rts

; HFS_WCP = record .X.  Modifies: .A
HFS_WC_ADDR:
            clc
            lda         HFS_WC_OLO,X
            adc         HFS_CK_BUF
            sta         HFS_WCP
            lda         HFS_WC_OHI,X
            adc         HFS_CK_BUF + 1
            sta         HFS_WCP + 1
            rts

HFS_WC_OLO:                                                 ; (Each record's offset in the buffer)
            .repeat     HFS_WC_N, I
            .byte       <(I * HFS_WC_SIZE)
            .endrepeat
HFS_WC_OHI:
            .repeat     HFS_WC_N, I
            .byte       >(I * HFS_WC_SIZE)
            .endrepeat

; The path element just looked up (HFS_DIR_SCAN's: the HFS_WCL characters before HFS_ELEM; none, a root's), with
; its key (HFS_WCK) and hash (HFS_WCH), into the cache's next record (HFS_WCP), no entry kept yet: .A = 1, there
; (its entry at HFS_LOC), or 0, not.  Modifies: .A, .X, .Y
HFS_WC_PUT:
            ldx         HFS_CK_BUF + 1                      ; (No buffer: no cache)
            beq         @done
            pha
            ldx         HFS_WC_NEXT                         ; The next record, its hash
            jsr         HFS_WC_ADDR
            lda         HFS_WCH
            sta         HFS_WC_H,X
            inx                                             ; (The one after it next, round)
            cpx         #HFS_WC_N
            bcc         :+
            ldx         #0
:
            stx         HFS_WC_NEXT
            ldy         #HFS_WC_DIR + 4                     ; Its disk and directory
:
            lda         HFS_WCK,Y
            sta         (HFS_WCP),Y
            dey
            bpl         :-
            ldy         #HFS_WC_LEN
            lda         HFS_WCL
            sta         (HFS_WCP),Y
            pla                                             ; There, and where
            ldy         #HFS_WC_OK
            sta         (HFS_WCP),Y
            ldy         #HFS_WC_HASENT
            lda         #0
            sta         (HFS_WCP),Y
            ldx         #0
            ldy         #HFS_WC_LOC
:
            lda         HFS_LOC,X
            sta         (HFS_WCP),Y
            iny
            inx
            cpx         #5
            bne         :-
            ldx         #0                                  ; Its name
@char:
            cpx         HFS_WCL
            beq         @done
            txa
            clc
            adc         HFS_ELEM
            sec
            sbc         HFS_WCL
            tay
            lda         (HFS_NM),Y
            pha
            txa
            clc
            adc         #HFS_WC_NAME
            tay
            pla
            sta         (HFS_WCP),Y
            inx
            bra         @char
@done:
            rts

; The root of disk HFS_CARD's entry (its place at HFS_LOC: the walk's start) from the cache, if it's kept and the
; walk goes on past it (HFS_WC_ON's): C = 0, HFS_ENT it, HFS_FP -> it; or C = 1, its record (HFS_WCP: found, or a
; new one) for HFS_WC_KEEP to keep it in once it's read.  Modifies: .A, .X, .Y
HFS_WC_ROOT:
            jsr         HFS_WC_KEY
            stz         HFS_WCL
            jsr         HFS_WC_SEARCH
            bcc         HFS_WC_ON                           ; (Kept: as a directory's on the way)
            lda         #1
            jsr         HFS_WC_PUT
            sec
            rts

; The entry of the element HFS_LOOKUP just found (HFS_WCP: its record) from the cache, if it's a directory's that's
; kept and the walk goes on past it (not the last element: that one's read, as it is now): C = 0, HFS_ENT it,
; HFS_FP -> it; or C = 1.  Modifies: .A, .Y
HFS_WC_ON:
            ldy         HFS_ELEM                            ; (Another element after it: a '/', and more)
            lda         (HFS_NM),Y
            beq         HFS_WC_NO
            iny
            lda         (HFS_NM),Y
            beq         HFS_WC_NO
HFS_WC_ENT_GET:
            ldy         #HFS_WC_HASENT
            lda         (HFS_WCP),Y
            beq         HFS_WC_NO
            jsr         HFS_WC_TO_ENT                       ; HFS_ENT = it
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         (HFS_WCP),Y
            sta         HFS_ENT,Y
            dey
            bpl         :-
            jsr         HFS_WC_FROM_ENT
            LOAD_ADDR   HFS_ENT, HFS_FP
            clc
            rts
HFS_WC_NO:
            sec
            rts

; HFS_ENT, just read, kept in record HFS_WCP if it's a directory's (a file's changes as it's written).  Keeps C.
; Modifies: .A, .Y
HFS_WC_KEEP:
            php
            lda         HFS_CK_BUF + 1                      ; (No buffer: no cache)
            beq         @done
            lda         HFS_ENT + HFS_E_MODE
            bpl         @done                               ; (HFS_M_DIR)
            jsr         HFS_WC_TO_ENT
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         HFS_ENT,Y
            sta         (HFS_WCP),Y
            dey
            bpl         :-
            jsr         HFS_WC_FROM_ENT
            ldy         #HFS_WC_HASENT
            lda         #1
            sta         (HFS_WCP),Y
@done:
            plp
            rts

; HFS_WCP on its record's entry, and back.  Modifies: .A
HFS_WC_TO_ENT:
            clc
            lda         HFS_WCP
            adc         #HFS_WC_ENT
            sta         HFS_WCP
            bcc         :+
            inc         HFS_WCP + 1
:
            rts

HFS_WC_FROM_ENT:
            sec
            lda         HFS_WCP
            sbc         #HFS_WC_ENT
            sta         HFS_WCP
            bcs         :+
            dec         HFS_WCP + 1
:
            rts

; The cache emptied (HFS_WC_CLEAR), or of disk .A's records (HFS_WC_FORGET).  Modifies: .A, .X
HFS_WC_CLEAR:
            lda         #$FF                                ; (Every disk's)
HFS_WC_FORGET:
            sta         HFS_WCD
            lda         HFS_CK_BUF + 1                      ; (No buffer: no cache)
            beq         @done
            MOVR        HFS_WCP, HFS_CK_BUF
            ldx         #HFS_WC_N
@rec:
            lda         HFS_WCD                             ; (Its disk: $FF, none)
            cmp         #$FF
            beq         :+
            cmp         (HFS_WCP)
            bne         @next
:
            lda         #$FF
            sta         (HFS_WCP)
@next:
            clc
            lda         HFS_WCP
            adc         #HFS_WC_SIZE
            sta         HFS_WCP
            bcc         :+
            inc         HFS_WCP + 1
:
            dex
            bne         @rec
@done:
            rts

HFS_SCAN_FREE       = 0         ; HFS_DIR_SCAN: a free entry
HFS_SCAN_USED       = 1         ;   an entry in use
HFS_SCAN_NAME       = $80       ;   the entry named by the path element at HFS_ELEM

; Look through the directory whose entry is at HFS_FP for an entry: HFS_SCAN says which.  For a name,
; HFS_LEN = its length, and HFS_ELEM moves past it.
; OUT: C = 0: HFS_NLOC = where it is, SD_POS = its offset in the directory, HFS_PTR -> it (in the cache);
;      or C = 1, .A = E_NOENT (none), E_NAMETOOLONG (too long a name) or a card error
HFS_DIR_SCAN:
            bit         HFS_SCAN
            bpl         @entries
            ldy         HFS_ELEM                            ; How long the element is
            ldx         #0
:
            lda         (HFS_NM),Y
            beq         :+
            cmp         #'/'
            beq         :+
            iny
            inx
            cpx         #HFS_NAME_MAX + 1
            bne         :-
            lda         #E_NAMETOOLONG                      ; Longer than a name can be
            sec
            rts
:
            stx         HFS_LEN
            sty         HFS_ELEM                            ; (Past it: HFS_NAME_EQ works back from here)

@entries:
            stz         SD_POS                              ; Over the directory's entries
            stz         SD_POS + 1
            stz         SD_POS + 2
            stz         SD_POS + 3

@entry:
            jsr         HFS_AT_END
            bcs         @not_found
            jsr         HFS_FILE_BLOCK
            bcs         @done
            jsr         HFS_LOAD
            bcs         @done
            lda         SD_POS                              ; HFS_PTR = the entry, in the cache
            sta         HFS_OFS
            lda         SD_POS + 1
            and         #1
            sta         HFS_OFS + 1
            jsr         HFS_AT
            bit         HFS_SCAN                            ; This one?
            bmi         @name
            lda         (HFS_PTR)                           ; (Free: a 0 first byte)
            beq         @free
            lda         HFS_SCAN
            bne         @found                              ; (HFS_SCAN_USED)
            bra         @next

@free:
            lda         HFS_SCAN
            beq         @found                              ; (HFS_SCAN_FREE)
            bra         @next

@name:
            jsr         HFS_NAME_EQ
            bcc         @found

@next:
            lda         SD_POS                              ; On to the next entry
            clc
            adc         #HFS_ENTRY_SIZE
            sta         SD_POS
            bcc         @entry
            inc         SD_POS + 1
            bne         @entry
            inc         SD_POS + 2
            bne         @entry
            inc         SD_POS + 3
            bra         @entry

@found:
            ldx         #3                                  ; HFS_NLOC = the block it's in (SD_CACHE_LOAD
:                                                           ;   leaves SD_LBA alone) ...
            lda         SD_LBA,X
            sta         HFS_NLOC,X
            dex
            bpl         :-
            lda         HFS_OFS + 1                         ; ... and its index in it: the offset / 64
            asl
            asl
            sta         SD_TMP
            lda         HFS_OFS
            lsr
            lsr
            lsr
            lsr
            lsr
            lsr
            ora         SD_TMP
            sta         HFS_NLOC + 4
            clc
            rts

@not_found:
            lda         #E_NOENT
            sec

@done:
            rts

; Is the entry at HFS_PTR the path element (the HFS_LEN characters before HFS_ELEM)?  A free entry isn't.
; OUT: C = 0: it is.  Modifies: .A, .X, .Y
HFS_NAME_EQ:
            lda         (HFS_PTR)
            beq         @no                                 ; A free entry
            lda         HFS_ELEM                            ; .X = where the element starts
            sec
            sbc         HFS_LEN
            tax
            ldy         #0

@char:
            cpy         HFS_LEN
            beq         @end
            lda         (HFS_PTR),Y                         ; The entry's name, against the element
            beq         @no                                 ; (The name is the shorter)
            sta         SD_TMP
            phy
            txa
            tay
            lda         (HFS_NM),Y
            ply
            cmp         SD_TMP
            bne         @no
            inx
            iny
            bra         @char

@end:                                                       ; The element ended: the name must too
            lda         (HFS_PTR),Y
            bne         @no
            clc
            rts

@no:
            sec
            rts

; Remember the directory a walk is in (HFS_LOC), so a ".." can go back to it.
; OUT: C = 0; or C = 1, .A = E_NAMETOOLONG (the path has more elements than HFS_DEPTH_MAX)
HFS_PUSH:
            lda         HFS_DEPTH
            cmp         #HFS_DEPTH_MAX
            bcs         @full
            inc         HFS_DEPTH
            jsr         HFS_STK_AT
            ldy         #0
:
            lda         HFS_LOC,Y
            sta         HFS_STK,X
            inx
            iny
            cpy         #5
            bne         :-
            clc
            rts

@full:
            lda         #E_NAMETOOLONG
            sec
            rts

; Back to the directory above (".."): HFS_LOC and HFS_ENT again.  At the root it stays there, as it has
; nothing above it.  OUT: C = 0; or C = 1, .A = a card error
HFS_POP:
            lda         HFS_DEPTH
            beq         @root
            jsr         HFS_STK_AT
            ldy         #0
:
            lda         HFS_STK,X
            sta         HFS_LOC,Y
            inx
            iny
            cpy         #5
            bne         :-
            dec         HFS_DEPTH
            jmp         HFS_ENT_READ

@root:
            clc
            rts

; .X = where level HFS_DEPTH - 1's 5 bytes are in HFS_STK.  Modifies: .A
HFS_STK_AT:
            lda         HFS_DEPTH
            asl                                             ; (HFS_DEPTH - 1) * 5
            asl
            clc
            adc         HFS_DEPTH
            sec
            sbc         #5
            tax
            rts

; ****************************************************************************
; The card: its superblock, its entries, and the block a file's byte is in

; Make sure card HFS_CARD's superblock has been read (HFS_V_*), starting the card if it isn't started.
; OUT: C = 0; or C = 1, .A = E_NOTFS, E_NODEV or a card error
HFS_VOLUME:
            ldx         HFS_CARD
            lda         HFS_V_STATE,X
            beq         @look
            bmi         @not_fs                             ; ($FF: looked at, and it isn't one)
            clc
            rts

@not_fs:
            lda         #E_NOTFS
            sec
            rts

@look:
            lda         HFS_CARD                            ; Started (a card not yet: now; it may not be there)
            sta         SD_DEV
            FAR1        disk_start
            bcc         :+
            jmp         @fail
:
            jsr         HFS_CARD_X                          ; Block 0 of the card: a superblock (the card is
            ldy         #4                                  ;   all HydraFS), or a partition table
:
            stz         HFS_V_BASE,X
            inx
            dey
            bne         :-
            jsr         HFS_SB_LOAD
            bcc         :+
            jmp         @fail
:
            jsr         HFS_SB_OK
            bcc         @superblock
            jsr         HFS_PART_FIND                       ; HFS_PTR -> its HydraFS partition's entry
            bcc         :+
            jmp         @no
:
            jsr         HFS_CARD_X                          ; Its blocks count from the partition's first
            ldy         #MBR_P_START
:
            lda         (HFS_PTR),Y
            sta         HFS_V_BASE,X
            inx
            iny
            cpy         #MBR_P_START + 4
            bne         :-
            jsr         HFS_SB_LOAD                         ; Its block 0: the superblock
            bcc         :+
            jmp         @fail
:
            jsr         HFS_SB_OK
            bcc         :+
            jmp         @no
:

@superblock:                                                ; The numbers the server works from, and the counters
            V_FROM_SB   HFS_SB_CLUSTERS, HFS_V_CLUSTERS
            V_FROM_SB   HFS_SB_MAP, HFS_V_MAP
            V_FROM_SB   HFS_SB_MAPSZ, HFS_V_MAPSZ
            V_FROM_SB   HFS_SB_DATA, HFS_V_DATA
            V_FROM_SB   HFS_SB_FREE, HFS_V_FREE
            V_FROM_SB   HFS_SB_HINT, HFS_V_HINT
            V_FROM_SB   HFS_SB_NEXT_QID, HFS_V_QID
            V_FROM_SB   HFS_SB_STAMP, HFS_V_STAMP
            V_FROM_SB   HFS_SB_MAPINIT, HFS_V_MINIT         ; (For version 1, the map's size: all written)
            ldy         #HFS_SB_VERSION
            lda         (SD_CACHE),Y
            cmp         #HFS_VERSION_FULL
            bne         @counted
            jsr         HFS_CARD_X
            ldy         #4
:
            lda         HFS_V_MAPSZ,X
            sta         HFS_V_MINIT,X
            inx
            dey
            bne         :-

@counted:
            ldx         HFS_CARD
            lda         #1
            sta         HFS_V_STATE,X
            clc
            rts

@no:
            ldx         HFS_CARD                            ; Not a HydraFS: don't look again
            lda         #$FF
            sta         HFS_V_STATE,X
            lda         #E_NOTFS

@fail:
            sec
            rts

; Read block 0 of card HFS_CARD's HydraFS (from HFS_V_BASE) into the cache.
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
HFS_SB_LOAD:
            stz         SD_LBA
            stz         SD_LBA + 1
            stz         SD_LBA + 2
            stz         SD_LBA + 3
            jmp         HFS_LOAD

; Is the block in the cache a HydraFS superblock this can read: "HYDRAFS1", version 1 or 2, the cluster
; size?  OUT: C = 0: it is.  Modifies: .A, .Y
HFS_SB_OK:
            ldy         #7
:
            lda         (SD_CACHE),Y
            cmp         HFS_MAGIC,Y
            bne         @no
            dey
            bpl         :-
            ldy         #HFS_SB_VERSION                     ; (Versions 1 and 2)
            lda         (SD_CACHE),Y
            beq         @no
            cmp         #HFS_VERSION + 1
            bcs         @no
            iny
            lda         (SD_CACHE),Y
            cmp         #HFS_CSHIFT
            bne         @no
            clc
            rts

@no:
            sec
            rts

; ****************************************************************************
; The storage driver's block calls (storage.s: FAR1), for a disk's HydraFS: its block numbers count from its
; partition's first block (HFS_V_BASE: 0 on a disk that's all HydraFS), so SD_LBA is moved there for the call, and
; back after it.  SD_CACHE_LOAD: block SD_LBA of disk SD_DEV in the block buffer (blk_get); SD_READ_BLOCK and
; SD_WRITE_BLOCK: into or from the 512 bytes at SD_BUF (blk_read, blk_write).  OUT: C = 0; or C = 1, .A = a disk
; error.  Modifies: .A, .X, .Y
SD_CACHE_LOAD:
            jsr         HFS_BASE_ADD
            FAR1        blk_get
            bra         HFS_BASE_SUB

SD_READ_BLOCK:
            jsr         HFS_BASE_ADD
            FAR1        blk_read
            bra         HFS_BASE_SUB

SD_WRITE_BLOCK:
            jsr         HFS_BASE_ADD
            FAR1        blk_write

; SD_LBA back to a block of the HydraFS.  Keeps .A and C
HFS_BASE_SUB:
            php
            pha
            jsr         HFS_BASE_X
            sec
            lda         SD_LBA
            sbc         HFS_V_BASE,X
            sta         SD_LBA
            lda         SD_LBA + 1
            sbc         HFS_V_BASE + 1,X
            sta         SD_LBA + 1
            lda         SD_LBA + 2
            sbc         HFS_V_BASE + 2,X
            sta         SD_LBA + 2
            lda         SD_LBA + 3
            sbc         HFS_V_BASE + 3,X
            sta         SD_LBA + 3
            pla
            plp
            rts

; SD_LBA = block SD_LBA of card SD_DEV's HydraFS, on the card.  Modifies: .A, .X
HFS_BASE_ADD:
            jsr         HFS_BASE_X
            clc
            lda         SD_LBA
            adc         HFS_V_BASE,X
            sta         SD_LBA
            lda         SD_LBA + 1
            adc         HFS_V_BASE + 1,X
            sta         SD_LBA + 1
            lda         SD_LBA + 2
            adc         HFS_V_BASE + 2,X
            sta         SD_LBA + 2
            lda         SD_LBA + 3
            adc         HFS_V_BASE + 3,X
            sta         SD_LBA + 3
            rts

; .X = card SD_DEV * 4 (its HFS_V_BASE).  Modifies: .A
HFS_BASE_X:
            lda         SD_DEV
            asl
            asl
            tax
            rts

; Disk SD_DEV is being started again (a different card may be in the socket now), or stopped: its superblock must
; be read afresh, any HydraFS file open on it is let go of (those fids give E_BADF from here on, and are clunked as
; usual), a check of it is forgotten, and its names in the walk cache.  (storage.s: FAR2.)  OUT: C = 0.  Modifies:
; .A, .X
hfs_forget:
            lda         SD_DEV
            jsr         HFS_WC_FORGET
            ldx         SD_DEV
            stz         HFS_V_STATE,X
            ldx         #HFS_MAX_OPEN - 1
:
            lda         HFS_H_CARD,X
            cmp         SD_DEV
            bne         :+
            lda         #$FF
            sta         HFS_H_CARD,X
:
            dex
            bpl         :--
            lda         HFS_CK_CARD
            cmp         SD_DEV
            bne         :+
            lda         #$FF
            sta         HFS_CK_CARD
:
            clc
            rts

; Is a HydraFS file on disk SD_DEV open?  (storage.s: FAR2.)  OUT: C = 0: no; or C = 1, .A = E_BUSY.  Modifies: .A,
; .X
hfs_in_use:
            ldx         #HFS_MAX_OPEN - 1
:
            lda         HFS_H_CARD,X
            cmp         SD_DEV
            beq         @busy
            dex
            bpl         :-
            clc
            rts

@busy:
            lda         #E_BUSY
            sec
            rts

; Read block SD_LBA of disk HFS_CARD into the block buffer (SD_CACHE_LOAD: storage.s's blk, which #d shares).
; OUT: C = 0; or C = 1, .A = a disk error.  Modifies: .A, .X, .Y
HFS_LOAD:
            lda         HFS_CARD
            sta         SD_DEV
            bit         HFS_MSTATE                          ; The same block, changed in the metadata
            bpl         :+                                  ;   buffer?  Write it first, so the cache
            jsr         HFS_META_SAME                       ;   reads it as it is now
            bcs         :+
            jsr         HFS_META_FLUSH
            bcs         @done
:
            jmp         SD_CACHE_LOAD

@done:
            rts

; HFS_PTR = the cache + HFS_OFS (an offset inside the block).  Modifies: .A
HFS_AT:
            lda         SD_CACHE
            clc
            adc         HFS_OFS
            sta         HFS_PTR
            lda         SD_CACHE + 1
            adc         HFS_OFS + 1
            sta         HFS_PTR + 1
            rts

; Read the directory entry at HFS_LOC into HFS_ENT, and point HFS_FP at it.
; OUT: C = 0; or C = 1, .A = a card error
HFS_ENT_READ:
            ldx         #3
:
            lda         HFS_LOC,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jsr         HFS_LOAD
            bcs         @done
            jsr         HFS_LOC_OFS                         ; Its offset in the block
            jsr         HFS_AT
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         (HFS_PTR),Y
            sta         HFS_ENT,Y
            dey
            bpl         :-
            ldx         #0                                  ; But an open file's copy is newer (its size
            jsr         HFS_SLOT_FIND                       ;   goes to the card when it's closed)
            bcs         :++
            ldy         #HFS_ENTRY_SIZE - 1
:
            lda         (HFS_PTR),Y
            sta         HFS_ENT,Y
            dey
            bpl         :-
:
            LOAD_ADDR   HFS_ENT, HFS_FP
            clc

@done:
            rts

; HFS_OFS = the offset in its block of the entry at HFS_LOC: its index * 64.  Modifies: .A, .X
HFS_LOC_OFS:
            stz         HFS_OFS + 1
            lda         HFS_LOC + 4
            ldx         #6
:
            asl
            rol         HFS_OFS + 1
            dex
            bne         :-
            sta         HFS_OFS
            rts

; Find an open file (from fid .X on) whose entry is the one at HFS_LOC on disk HFS_CARD.
; OUT: C = 0: .X = its fid, HFS_PTR -> its copy of the entry; or C = 1: none.  Modifies: .A, .Y
HFS_SLOT_FIND:
            cpx         #HFS_MAX_OPEN
            bcs         @done
            lda         HFS_H_CARD,X                        ; (A free slot's disk is $FF)
            cmp         HFS_CARD
            bne         @next
            lda         HFS_H_EIDX,X
            cmp         HFS_LOC + 4
            bne         @next
            txa                                             ; (Its block: at the fid * 4)
            asl
            asl
            tay
            lda         HFS_H_EBLK,Y
            cmp         HFS_LOC
            bne         @next
            lda         HFS_H_EBLK + 1,Y
            cmp         HFS_LOC + 1
            bne         @next
            lda         HFS_H_EBLK + 2,Y
            cmp         HFS_LOC + 2
            bne         @next
            lda         HFS_H_EBLK + 3,Y
            cmp         HFS_LOC + 3
            bne         @next
            txa                                             ; Its copy: HFS_FILES + the fid * 64
            lsr                                             ; (The pages: the fid / 4)
            lsr
            sta         HFS_PTR + 1
            txa
            and         #3                                  ; (And the fid % 4, * 64)
            lsr
            ror
            ror
            clc
            adc         #<HFS_FILES
            sta         HFS_PTR
            lda         HFS_PTR + 1
            adc         #>HFS_FILES
            sta         HFS_PTR + 1
            clc
            rts

@next:
            inx
            bra         HFS_SLOT_FIND

@done:
            rts                                             ; (C = 1)

.assert     HFS_ENTRY_SIZE = 64, error, "HFS_SLOT_FIND, HFS_FILE_PTR: an entry is 64 bytes (the fid % 4 * 64)"

; SD_LBA = the card block holding byte SD_POS of the file whose entry is at HFS_FP: the file's cluster,
; then the extent it's in (the entry's two, then its extent blocks').
; OUT: C = 0; or C = 1, .A = HFS_EOF (past the file's clusters) or a card error; or C = 1, .A =
;      HFS_IN_HOLE: it's in a hole (zeros, on no block: HFS_HOLEP -> the hole's extent, in the extent
;      block HFS_XBC is (the cache holds it), or in the entry if that's 0; HFS_CL = the cluster's place in
;      the hole, HFS_XLEN = the hole's clusters: hfs_sparse.s)
HFS_FILE_BLOCK:
            ldx         #3                                  ; (The entry's extents first)
:
            stz         HFS_XBC,X
            dex
            bpl         :-
            lda         SD_POS + 3                          ; HFS_CL = SD_POS >> 9: the block in the file
            lsr
            sta         HFS_CL + 2
            lda         SD_POS + 2
            ror
            sta         HFS_CL + 1
            lda         SD_POS + 1
            ror
            sta         HFS_CL
            stz         HFS_CL + 3
            lda         HFS_CL
            and         #HFS_CLUSTER_BLOCKS - 1
            sta         HFS_SUB                             ; The block inside its cluster
            ldx         #HFS_CSHIFT                         ; HFS_CL >>= 3: the cluster in the file
:
            lsr         HFS_CL + 3
            ror         HFS_CL + 2
            ror         HFS_CL + 1
            ror         HFS_CL
            dex
            bne         :-
            lda         HFS_FP                              ; The entry's own two extents
            sta         HFS_XP
            lda         HFS_FP + 1
            sta         HFS_XP + 1
            ldy         #HFS_E_EXT1
            jsr         HFS_EXT_TRY
            bcc         HFS_FB_DONE
            ldy         #HFS_E_EXT2
            jsr         HFS_EXT_TRY
            bcc         HFS_FB_DONE
            ldy         #HFS_E_EXTBLK                       ; Then the extent blocks, if it has any
            ldx         #0
:
            lda         (HFS_FP),Y
            sta         HFS_XBLK,X
            iny
            inx
            cpx         #4
            bne         :-

HFS_FB_CHAIN:
            lda         HFS_XBLK
            ora         HFS_XBLK + 1
            ora         HFS_XBLK + 2
            ora         HFS_XBLK + 3
            beq         HFS_FB_PAST                         ; No more extents: past the file's clusters
            ldx         #3
:
            lda         HFS_XBLK,X
            sta         SD_LBA,X
            dex
            bpl         :-
            jsr         HFS_LOAD
            bcs         HFS_FB_DONE
            ldx         #3                                  ; (The block its extents are in, for a hole's)
:
            lda         SD_LBA,X
            sta         HFS_XBC,X
            dex
            bpl         :-
            ldy         #HFS_X_NEXT                         ; The one after this, and the extents in it
            ldx         #0
:
            lda         (SD_CACHE),Y
            sta         HFS_XBLK,X
            iny
            inx
            cpx         #4
            bne         :-
            lda         (SD_CACHE),Y                        ; (HFS_X_COUNT, 84 at most)
            sta         SD_TMP
            lda         SD_CACHE                            ; HFS_XP = the first of them
            clc
            adc         #HFS_X_FIRST
            sta         HFS_XP
            lda         SD_CACHE + 1
            adc         #0
            sta         HFS_XP + 1

HFS_FB_EXT:
            lda         SD_TMP
            beq         HFS_FB_CHAIN                        ; None left in this block: the next one
            dec         SD_TMP
            ldy         #0
            jsr         HFS_EXT_TRY
            bcc         HFS_FB_DONE
            lda         HFS_XP
            clc
            adc         #HFS_EXT_SIZE
            sta         HFS_XP
            bcc         HFS_FB_EXT
            inc         HFS_XP + 1
            bra         HFS_FB_EXT

HFS_FB_PAST:
            lda         #HFS_EOF
            sec

HFS_FB_DONE:
            rts

; Is the file cluster HFS_CL (what's left of it) inside the extent at (HFS_XP),Y?
; OUT: C = 0: SD_LBA = the card block it wants (with HFS_SUB, the block inside the cluster);
;      C = 1: HFS_CL less this extent's clusters, for the next extent (an unused one has none).  In a
;      hole, it returns from HFS_FILE_BLOCK (its only caller) instead: C = 1, .A = HFS_IN_HOLE
; Modifies: .A, .X, .Y, HFS_XCL, HFS_XLEN
HFS_EXT_TRY:
            ldx         #0
:
            lda         (HFS_XP),Y                          ; The extent: its first cluster, then its clusters
            sta         HFS_XCL,X
            iny
            inx
            cpx         #HFS_EXT_SIZE
            bne         :-
            lda         HFS_CL + 2                          ; In it if HFS_CL < its clusters
            ora         HFS_CL + 3
            bne         @past
            lda         HFS_CL + 1
            cmp         HFS_XLEN + 1
            bcc         @in
            bne         @past
            lda         HFS_CL
            cmp         HFS_XLEN
            bcc         @in

@past:
            lda         HFS_CL                              ; HFS_CL -= its clusters
            sec
            sbc         HFS_XLEN
            sta         HFS_CL
            lda         HFS_CL + 1
            sbc         HFS_XLEN + 1
            sta         HFS_CL + 1
            lda         HFS_CL + 2
            sbc         #0
            sta         HFS_CL + 2
            lda         HFS_CL + 3
            sbc         #0
            sta         HFS_CL + 3
            sec
            rts

@in:
            lda         HFS_XCL                             ; A hole (its first cluster HFS_HOLE)?
            and         HFS_XCL + 1
            and         HFS_XCL + 2
            and         HFS_XCL + 3
            cmp         #HFS_HOLE
            bne         @data
            tya                                             ; HFS_HOLEP -> its extent: (HFS_XP),Y, less the
            sec                                             ;   extent's size (read past it)
            sbc         #HFS_EXT_SIZE
            clc
            adc         HFS_XP
            sta         HFS_HOLEP
            lda         HFS_XP + 1
            adc         #0
            sta         HFS_HOLEP + 1
            pla                                             ; (Not back to HFS_FILE_BLOCK: to its caller)
            pla
            lda         #HFS_IN_HOLE
            sec
            rts

@data:                                                      ; SD_LBA = the data area's first block +
            lda         HFS_XCL                             ;   (its first cluster + HFS_CL) * 8 + HFS_SUB
            clc
            adc         HFS_CL
            sta         SD_LBA
            lda         HFS_XCL + 1
            adc         HFS_CL + 1
            sta         SD_LBA + 1
            lda         HFS_XCL + 2
            adc         HFS_CL + 2
            sta         SD_LBA + 2
            lda         HFS_XCL + 3
            adc         HFS_CL + 3
            sta         SD_LBA + 3
            ldx         #HFS_CSHIFT                         ; * the blocks in a cluster
:
            asl         SD_LBA
            rol         SD_LBA + 1
            rol         SD_LBA + 2
            rol         SD_LBA + 3
            dex
            bne         :-
            lda         SD_LBA
            ora         HFS_SUB
            sta         SD_LBA
            lda         HFS_CARD                            ; (.X = the card * 4)
            asl
            asl
            tax
            lda         SD_LBA
            clc
            adc         HFS_V_DATA,X
            sta         SD_LBA
            lda         SD_LBA + 1
            adc         HFS_V_DATA + 1,X
            sta         SD_LBA + 1
            lda         SD_LBA + 2
            adc         HFS_V_DATA + 2,X
            sta         SD_LBA + 2
            lda         SD_LBA + 3
            adc         HFS_V_DATA + 3,X
            sta         SD_LBA + 3
            clc
            rts

; ****************************************************************************
; Odds and ends

; Is SD_POS at or past the end of the file whose entry is at HFS_FP (its size)?  OUT: C = 1: it is
; Modifies: .A, .X, .Y
HFS_AT_END:
            ldy         #HFS_E_SIZE + 3
            ldx         #3
:
            lda         SD_POS,X
            cmp         (HFS_FP),Y
            bcc         @under
            bne         @over
            dey
            dex
            bpl         :-

@over:                                                      ; (Or the same: nothing is left)
            sec
            rts

@under:
            clc
            rts

; HFS_CL = the bytes from SD_POS to the end of the file whose entry is at HFS_FP (it must be inside it).
; Modifies: .A, .X, .Y
; (Unrolled: a loop's cpx or cpy would lose the carry between the subtractions.)
HFS_TAIL:
            ldy         #HFS_E_SIZE                         ; (The size, low byte first)
            sec
            lda         (HFS_FP),Y
            sbc         SD_POS
            sta         HFS_CL
            iny
            lda         (HFS_FP),Y
            sbc         SD_POS + 1
            sta         HFS_CL + 1
            iny
            lda         (HFS_FP),Y
            sbc         SD_POS + 2
            sta         HFS_CL + 2
            iny
            lda         (HFS_FP),Y
            sbc         SD_POS + 3
            sta         HFS_CL + 3
            rts

; SD_N = min(SD_N, HFS_CL) (16-bit; SD_N stays if HFS_CL is 65536 or more).  Modifies: .A
HFS_N_MIN:
            lda         HFS_CL + 2
            ora         HFS_CL + 3
            bne         @done
            lda         SD_N + 1
            cmp         HFS_CL + 1
            bcc         @done
            bne         @take
            lda         SD_N
            cmp         HFS_CL
            bcc         @done
            beq         @done

@take:
            lda         HFS_CL
            sta         SD_N
            lda         HFS_CL + 1
            sta         SD_N + 1

@done:
            rts

; Check the fid the request came with (SD_FID).
; OUT: C = 0: HFS_FID = it, HFS_CARD = its disk, SD_OP = its open mode, .X = it, HFS_FP = its entry, HFS_LOC =
;      where the entry is on the disk; or C = 1, .A = E_BADF
HFS_FID_CHECK:
            lda         SD_FID
            cmp         #HFS_MAX_OPEN
            bcs         @bad
            sta         HFS_FID
            tax
            lda         HFS_H_CARD,X
            bmi         @bad                                ; ($FF: not open)
            sta         HFS_CARD
            lda         HFS_H_OMODE,X
            sta         SD_OP
            lda         HFS_H_EIDX,X                        ; HFS_LOC = where its entry is
            sta         HFS_LOC + 4
            txa
            asl
            asl
            tay
            lda         HFS_H_EBLK,Y
            sta         HFS_LOC
            lda         HFS_H_EBLK + 1,Y
            sta         HFS_LOC + 1
            lda         HFS_H_EBLK + 2,Y
            sta         HFS_LOC + 2
            lda         HFS_H_EBLK + 3,Y
            sta         HFS_LOC + 3
            jsr         HFS_FILE_PTR                        ; (It leaves .X alone)
            clc
            rts

@bad:
            lda         #E_BADF
            sec
            rts

; A free open file slot.  OUT: C = 0: HFS_FID = it, .X = it; or C = 1, .A = E_NFILE
; (HFS_FID_NEW doesn't point HFS_FP at it: an open does that when it has filled the slot in.)
HFS_FID_NEW:
            ldx         #0
@slot:
            lda         HFS_H_CARD,X
            bmi         @free                               ; ($FF)
            inx
            cpx         #HFS_MAX_OPEN
            bne         @slot
            lda         #E_NFILE
            sec
            rts

@free:
            stx         HFS_FID
            clc
            rts

; HFS_FP = the open file HFS_FID's copy of its directory entry (HFS_FILES + the fid * HFS_ENTRY_SIZE).
; Preserves .X, .Y.  Modifies: .A
HFS_FILE_PTR:
            lda         HFS_FID
            lsr                                             ; The fid * 64: (fid >> 2) pages, and
            lsr                                             ;   (fid & 3) * 64 in them
            sta         HFS_FP + 1
            lda         HFS_FID
            and         #3
            lsr
            ror
            ror
            clc
            adc         #<HFS_FILES
            sta         HFS_FP
            lda         HFS_FP + 1
            adc         #>HFS_FILES
            sta         HFS_FP + 1
            rts
