.debuginfo

; ****************************************************************************
; The HydraFS server: the files on the SD cards, /sd/N/... (BIOS ROM page 6; included inside
; `.scope PAGE6`, see all.s and page6.s).  The device is "hfs", served in the storage task, so it shares
; the SPI bus, the block cache and the ZP with /dev/sd (sd_srv.s, page 3), one request at a time.  The format, and what each
; build step covers, is in docs/plans/HYDRAFS.md; sim/tools/hydrafs.js makes card images on the PC.
;
;   /sd/0 ... /sd/7      card 0-7's root directory (the shell mounts the device at /sd, and the tasks it
;                        starts inherit the mount; IO_MOUNT can put it anywhere)
;   /x, /r, /s           the disks in memory: the ROM disk, the RAM disk (its areas, /r/N), the shared RAM
;                        disk; only through a mount with a spec (mount hfs /rom x: IO_BLK_SPEC), or for task
;                        0, so /sd has the cards alone (HFS_SPEC_CHECK)
;   /sd/N/a/b            the file or directory b in the directory a on card N.  "." and ".." are understood
;                        while walking (they aren't stored); names are case-sensitive, 1-31 characters
;   Reading a directory  gives a line per entry, "name size" ("name/" for a directory), then CR LF; or,
;                        opened with IO_MODE_STAT, IO_STAT_SIZE-byte stat records (read a multiple of
;                        IO_STAT_SIZE bytes at a time).  The lines are made again from the entries at every
;                        read and the bytes before the fd's offset thrown away, so it keeps no state
;   IO_STAT              the file's stat record, from the copy of its directory entry the fid holds
;   Writing              at the fd's offset, up to the end of the file and past it (it grows; a write can't
;                        start past the end: no holes).  IO_MODE_TRUNC empties the file as it's opened
;   IO_CREATE, IO_REMOVE, IO_WSTAT (rename, mode bits); "format" and "label" on /dev/sd/N/ctl
;
; A card with no HydraFS on it gives ERR_IO_NOT_FS (an unreadable one ERR_IO_DEVICE).  This file has the
; requests and the reading side; hfs_write.s has the writing side: the metadata buffer, allocating, growing
; and freeing, create, remove, wstat, format.
;
; What reaches the card when: a file's data at once, a block at a time through the cache; its directory
; entry when the file gets a cluster, and when the last fd on it is closed (so after a crash a file being
; written can be shorter than what was written, but no cluster is ever lost or used twice); the free map,
; extent blocks and entries in that order, through the metadata buffer, by the end of each request.

.segment "HFS_P6"

HFS_MAGIC:  .byte   "HYDRAFS1"

; IN: .A = request, .X = client, .Y = fid
HFS_SERVE:
            stx         SD_CLIENT
            sty         SD_FID
            jsr         HFS_REQUEST
            jmp         HFS_FINISH                          ; (What it changed goes to the card now)

HFS_REQUEST:
            jmp         HFS_REQ_DISKS                       ; (/sd itself?  Above COMMON: back at HFS_REQUEST_ON)

HFS_REQUEST_ON:
            cmp         #H9_READ
            beq         HFS_TO_READ
            cmp         #H9_WRITE
            beq         HFS_TO_WRITE
            cmp         #H9_OPEN
            beq         HFS_TO_OPEN
            cmp         #H9_CREATE
            beq         HFS_TO_CREATE
            cmp         #H9_REMOVE
            beq         HFS_TO_REMOVE
            cmp         #H9_WSTAT
            beq         HFS_TO_WSTAT
            cmp         #H9_STAT
            beq         HFS_TO_STAT
            cmp         #H9_CLUNK
            beq         HFS_CLUNK
            cmp         #H9_DUP
            beq         HFS_DUP
            lda         #ERR_IO_BAD_REQ                     ; (H9_CTL: there's nothing to control)

HFS_ERR:
            sec

HFS_RET:
            rts

HFS_TO_READ:
            jmp         HFS_READ_REQ

HFS_TO_WRITE:
            jmp         HFS_WRITE_REQ

HFS_TO_OPEN:
            jmp         HFS_OPEN_REQ

HFS_TO_CREATE:
            jmp         HFS_CREATE_REQ

HFS_TO_REMOVE:
            jmp         HFS_REMOVE_REQ

HFS_TO_WSTAT:
            jmp         HFS_WSTAT_REQ

HFS_TO_STAT:
            jmp         HFS_STAT_REQ

; ****************************************************************************
; The requests

; H9_CLUNK and H9_DUP: the fds sharing the slot (a dup, or a new task inheriting the fd, is another one)
HFS_CLUNK:
            jsr         HFS_FID_CHECK
            bcs         HFS_RET
            dec         HFS_FHDR + HFS_H_REFS,X
            bne         HFS_OK
            lda         #$FF                                ; The last one: the slot is free again, and if
            sta         HFS_FHDR + HFS_H_CARD,X             ;   the file changed, its entry goes to the card
            lda         HFS_FHDR + HFS_H_FLAGS,X            ;   (from the slot's copy: it's still there)
            bpl         HFS_OK                              ; (HFS_HF_DIRTY)
            jmp         HFS_ENT_PUT

HFS_OK:
            lda         #0
            clc
            rts

HFS_DUP:
            jsr         HFS_FID_CHECK
            bcs         HFS_RET
            inc         HFS_FHDR + HFS_H_REFS,X
            bra         HFS_OK

; H9_OPEN: walk the rest of the name ("/N/a/b", in the client's data area), then take an open file slot
HFS_OPEN_REQ:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_MODE
            lda         (ZP_IO_REQ),Y
            sta         SD_OP                               ; The open mode
            inc         ZP_IO_REQ + 1                       ; The data area: the name after the mount point
            lda         (ZP_IO_REQ)
            bne         :+
            jmp         HFS_OPEN_DISKS                      ; ("": /sd itself)
:
            jsr         HFS_WALK
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (It keeps .A and C)
            bcs         HFS_OPEN_RET
            jsr         HFS_PARENT_LOC                      ; HFS_PLOC = the directory it's in
            jmp         HFS_TAKE_SLOT

HFS_OPEN_RET:
            rts

.pushseg
.segment "HIGH_P6"                                          ; (Page 6's room above COMMON)
FAR_GATE_INLINE     SD_DIR_REQ,     PAGE3::SD_DIR_REQ,      3   ; (/sd itself: the cards, sd_srv.s)

; A request on /sd itself (HFS_FID_DISKS: the cards, a directory) goes to sd_srv.s; the rest on (HFS_REQUEST_ON)
HFS_REQ_DISKS:
            cpy         #HFS_FID_DISKS
            beq         :+
            jmp         HFS_REQUEST_ON
:
            ldx         #SD_MAX_CARDS
            jmp         SD_DIR_REQ

HFS_OPEN_DISKS:                                             ; /sd itself: the cards, read only (HFS_FID_DISKS)
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         SD_OP
            and         #IO_MODE_WRITE
            bne         :+
            lda         #HFS_FID_DISKS
            clc
            rts
:
            lda         #ERR_IO_MODE
            sec
            rts
.popseg

; Is disk HFS_CARD read only (the ROM disk)?  OUT: C = 1 and .A = ERR_IO_MODE if it is; C = 0 if not.
; Modifies: .A
HFS_RO_DISK:
            lda         HFS_CARD
            cmp         #DISK_ROM
            bne         :+
            lda         #ERR_IO_MODE
            sec
            rts
:
            clc
            rts

; May the client (SD_CLIENT) use the name at (ZP_IO_REQ)?  On the RAM disk ("/r/..."), the root's entries are the
; tasks' areas (docs/plans/DISKS.md): "/r/N" (N a hex digit, 0-9 a-f) is task N's, for task N and the tasks it
; started (and theirs, up its owner chain: TASK_MAY); any other name there isn't one.  Task 0, the system's, may use them all, and the root itself is everyone's.  Other
; disks: no check.  (Before a walk, and before a create cuts the name at its last '/'.)  An area a task may use is
; marked in RAMD_AREAS, so its task's end removes it (HFS_AREA_END).
; OUT: C = 0; or C = 1, .A = ERR_IO_PERM.  Modifies: .A, .X, .Y, HFS_ELEM, ZP_TEMP, ZP_TEMP_2
HFS_AREA_CHECK:
            ldy         #1
            lda         (ZP_IO_REQ),Y
            cmp         #DISK_NAME_RAM
            bne         @ok
            iny
            lda         (ZP_IO_REQ),Y                       ; "/r", the root
            beq         @ok
            cmp         #'/'
            bne         @ok                                 ; (Not the RAM disk: the walk finds no such disk)
            lda         SD_CLIENT
            beq         @ok                                 ; (Task 0: anything)
            iny
            lda         (ZP_IO_REQ),Y                       ; The area: one hex digit ...
            sec
            sbc         #'0'
            cmp         #10
            bcc         :+
            sbc         #'a' - '0' - 10                     ; (C = 1)
            cmp         #10
            bcc         @no
            cmp         #16
            bcs         @no
:
            sta         HFS_ELEM                            ; (The area's task)
            iny
            lda         (ZP_IO_REQ),Y                       ; ... then the end, or a '/'
            beq         :+
            cmp         #'/'
            bne         @no
:
            lda         SD_CLIENT                           ; The client: the area's task's family?
            ldx         HFS_ELEM
            ldy         #0
            jsr         TASK_MAY
            bcs         @no
            lda         HFS_ELEM                            ; It may: the area may be there now
            jsr         HFS_AREA_BIT
            ora         RAMD_AREAS,X
            sta         RAMD_AREAS,X
            clc
            rts

@no:
            lda         #ERR_IO_PERM
            sec
            rts

@ok:
            clc
            rts

; The name at (ZP_IO_REQ) (the client's data area, its request block the page before): a disk in memory ("/x",
; "/r", "/s": a letter past the cards' hex digits) only if a mount's spec named it (IO_BLK_SPEC), or for task 0
; (SD_CLIENT: HFS_AREA_END's own walks too).  OUT: C = 0; or .A = ERR_IO_NOT_FOUND, C = 1.  Modifies: .A, .Y
HFS_SPEC_CHECK:
            lda         SD_CLIENT
            beq         @ok
            ldy         #1
            lda         (ZP_IO_REQ),Y                       ; The disk's name
            cmp         #'f' + 1
            bcc         @ok                                 ; (A card's, or the root, or nothing)
            dec         ZP_IO_REQ + 1
            ldy         #IO_BLK_SPEC
            lda         (ZP_IO_REQ),Y
            inc         ZP_IO_REQ + 1
            cmp         #0
            bne         @ok
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

@ok:
            clc
            rts
.assert     DISK_NAME_ROM > 'f' .and DISK_NAME_RAM > 'f' .and DISK_NAME_SRAM > 'f', error, "HFS_SPEC_CHECK: the disks in memory have names past f"

; Area .A's bit in RAMD_AREAS: .X = which byte, .A = the mask.  Modifies: .Y
HFS_AREA_BIT:
            ldx         #0
            cmp         #8
            bcc         :+
            inx
            and         #7
:
            tay
            lda         #1
:
            dey
            bmi         :+
            asl
            bra         :-
:
            rts

; As task .A ends (TASK_AREA_END, page 5, through TASK_CALL: in the storage task, between requests): its area on
; the RAM disk, /r/N, removed with everything in it.  Over and over: from the area down, the first entry of each
; directory, to a file or an empty directory, which is removed; the area's own last.  An open file in it (a task
; it started may still have one), or a path too long or too deep, stops it, and the area's bit stays, for the next
; time task N ends; gone, the bit goes.  Requests from here are task 0's (SD_CLIENT): any area.
HFS_AREA_END:
            sta         RAMD_AREA_TASK
            lda         RAMD_AREA_PATH                      ; The name to walk: "/r/N" (N as a hex digit)
            sta         ZP_IO_REQ
            lda         RAMD_AREA_PATH + 1
            sta         ZP_IO_REQ + 1
            stz         SD_CLIENT
            ldy         #3
:
            lda         HFS_AREA_ROOT,Y
            sta         (ZP_IO_REQ),Y
            dey
            bpl         :-
            lda         RAMD_AREA_TASK
            cmp         #10
            bcc         :+
            adc         #'a' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            ldy         #3
            sta         (ZP_IO_REQ),Y

@again:                                                     ; From the area down ...
            lda         #4
            sta         RAMD_AREA_LEN

@down:
            ldy         RAMD_AREA_LEN
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         HFS_WALK
            bcs         @not_there
            lda         HFS_ENT + HFS_E_MODE
            bpl         @remove                             ; (A file)
            lda         #HFS_SCAN_USED                      ; A directory: its first entry?
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcc         @entry
            cmp         #ERR_IO_NOT_FOUND
            beq         @remove                             ; (Empty)
            bra         @stop

@entry:
            ldy         RAMD_AREA_LEN                       ; The path: then '/' and its name
            lda         #'/'
            sta         (ZP_IO_REQ),Y
            iny
            sty         RAMD_AREA_LEN
            ldy         #HFS_E_NAME

@name:
            lda         (HFS_PTR),Y
            beq         @down
            phy
            ldy         RAMD_AREA_LEN
            sta         (ZP_IO_REQ),Y
            ply
            inc         RAMD_AREA_LEN
            beq         @stop                               ; (Too long a path)
            iny
            cpy         #HFS_NAME_MAX + HFS_E_NAME
            bne         @name
            bra         @down

@remove:
            jsr         HFS_REMOVE_AT
            bcs         @stop                               ; (Open, or an error)
            lda         RAMD_AREA_LEN
            cmp         #4
            bne         @again                              ; (Something in it: again, from the area)
            bra         @gone

@not_there:
            lda         RAMD_AREA_LEN                       ; The area itself not there: none to remove
            cmp         #4
            bne         @stop                               ; (Deeper: too deep a walk, or an error)

@gone:
            lda         RAMD_AREA_TASK                      ; No area now: its bit goes
            jsr         HFS_AREA_BIT
            eor         #$FF
            and         RAMD_AREAS,X
            sta         RAMD_AREAS,X

@stop:
            jmp         HFS_FINISH                          ; (What it changed goes to the disk now)

HFS_AREA_ROOT:  .byte   "/", DISK_NAME_RAM, "/", 0

; A wstat (rename, mode) of the open file at header .X: on the RAM disk, not an area itself (an entry in its root),
; but for task 0.  OUT: C = 0; or C = 1, .A = ERR_IO_PERM.  Preserves .X
HFS_AREA_WSTAT:
            lda         HFS_CARD
            cmp         #DISK_RAM
            bne         @ok
            lda         SD_CLIENT
            beq         @ok
            lda         HFS_FHDR + HFS_H_PBLK,X             ; Its directory: the root (its entry is in block
            ora         HFS_FHDR + HFS_H_PBLK + 1,X         ;   0, the superblock)?
            ora         HFS_FHDR + HFS_H_PBLK + 2,X
            ora         HFS_FHDR + HFS_H_PBLK + 3,X
            bne         @ok
            lda         HFS_FHDR + HFS_H_PIDX,X
            cmp         #HFS_SB_ROOT / HFS_ENTRY_SIZE
            bne         @ok
            lda         #ERR_IO_PERM
            sec
            rts

@ok:
            clc
            rts

; HFS_PLOC = where the entry a walk ended at is listed: in the walk's last directory (HFS_STK); for a
; card's root, which isn't in one, its own place.  Modifies: .A, .X, .Y
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
; SD_OP: check the mode suits it, empty it for IO_MODE_TRUNC, and take an open file slot for it.  (The
; mode bits are checked here, when it's opened, as in Plan 9: a file made read-only while it's open for
; writing can still be written through that fd.)
; OUT: C = 0: .A = the fid; or C = 1, .A = ERR_IO_MODE, ERR_IO_NO_FDS or a card error
HFS_TAKE_SLOT:
            lda         SD_OP
            and         #IO_MODE_WRITE | IO_MODE_TRUNC
            beq         HFS_TAKE_NEW
            jsr         HFS_RO_DISK                         ; Writing: not on the ROM disk ...
            bcs         @bad_mode
            lda         HFS_ENT + HFS_E_MODE                ;   not a directory, nor a read-only file
            and         #HFS_M_DIR | HFS_M_RO
            bne         @bad_mode
            lda         SD_OP
            and         #IO_MODE_TRUNC
            beq         HFS_TAKE_NEW
            lda         SD_OP                               ; (Emptying it is writing it)
            and         #IO_MODE_WRITE
            beq         @bad_mode
            jsr         HFS_TRUNCATE                        ; (HFS_FP -> HFS_ENT, from the walk)
            bcc         HFS_TAKE_NEW
            rts

@bad_mode:
            lda         #ERR_IO_MODE
            sec
            rts

; A file just made comes here, past the mode check: the fd that made it can write it, even if its mode
; says read-only
HFS_TAKE_NEW:
            jsr         HFS_FID_NEW                         ; A free slot: HFS_FID, .X = its header
            bcs         @done
            lda         HFS_CARD
            sta         HFS_FHDR + HFS_H_CARD,X
            lda         SD_OP
            sta         HFS_FHDR + HFS_H_OMODE,X
            lda         #1
            sta         HFS_FHDR + HFS_H_REFS,X
            stz         HFS_FHDR + HFS_H_FLAGS,X
            lda         HFS_LOC + 4
            sta         HFS_FHDR + HFS_H_EIDX,X
            lda         HFS_PLOC + 4
            sta         HFS_FHDR + HFS_H_PIDX,X
            ldy         #0                                  ; Where its entry is, and its directory's
:
            lda         HFS_LOC,Y
            sta         HFS_FHDR + HFS_H_EBLK,X
            lda         HFS_PLOC,Y
            sta         HFS_FHDR + HFS_H_PBLK,X
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
            clc

@done:
            rts

; H9_STAT: the fid's copy of its directory entry, as a stat record
HFS_STAT_REQ:
            jsr         HFS_FID_CHECK
            bcc         :+
            rts
:
            lda         HFS_FP
            sta         HFS_PTR
            lda         HFS_FP + 1
            sta         HFS_PTR + 1
            jsr         HFS_REC                             ; HFS_STAT = the record
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #IO_STAT_SIZE - 1
:
            lda         HFS_STAT,Y
            sta         (ZP_IO_REQ),Y
            dey
            bpl         :-
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #0
            clc
            rts

; H9_READ: a file's bytes at the fd's offset, or a directory's listing
HFS_READ_REQ:
            jsr         HFS_FID_CHECK
            bcc         :+
            rts
:
            jsr         HFS_REQ_ARGS
            ldy         #HFS_E_MODE
            lda         (HFS_FP),Y
            bpl         HFS_FILE_READ                       ; (HFS_M_DIR: a directory's listing instead)
            jmp         HFS_DIR_READ

; Map the client's request (IO_SRV_MAP), and take its offset (SD_POS) and count (SD_LEFT, 1-256); SD_DONE
; = 0.  Modifies: .A, .X, .Y
HFS_REQ_ARGS:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_OFS + 3                     ; SD_POS = the offset
            ldx         #3
:
            lda         (ZP_IO_REQ),Y
            sta         SD_POS,X
            dey
            dex
            bpl         :-
            ldy         #IO_BLK_COUNT                       ; SD_LEFT = the count (1-256)
            lda         (ZP_IO_REQ),Y
            sta         SD_LEFT
            iny
            lda         (ZP_IO_REQ),Y
            sta         SD_LEFT + 1
            stz         SD_DONE
            rts

; A file: its bytes, the part of one block at a time (as SD_RW does, but stopping at the end of the file)
HFS_FILE_READ:
            lda         SD_LEFT
            ora         SD_LEFT + 1
            bne         @far2
            jmp         HFS_READ_DONE
@far2:
            jsr         HFS_AT_END                          ; End of file: that's all there is
            bcc         @more
            jmp         HFS_READ_DONE

@more:
            stz         HFS_HOLEF
            jsr         HFS_FILE_BLOCK                      ; The card block byte SD_POS is in
            bcc         @load
            cmp         #HFS_IN_HOLE                        ; (In a hole: zeros)
            beq         @hole
            jmp         HFS_READ_ERR

@hole:
            dec         HFS_HOLEF
            bra         @loaded

@load:
            jsr         HFS_BLOCK_AT                        ; (The ROM disk's: in the paged ROM; else the cache)
            bcc         @loaded
            jmp         HFS_READ_ERR

@loaded:
            lda         SD_POS + 1                          ; SD_N = the bytes to this block's end
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
            stz         HFS_CL + 2                          ; ... but no more than the count wants
            stz         HFS_CL + 3
            lda         SD_LEFT
            sta         HFS_CL
            lda         SD_LEFT + 1
            sta         HFS_CL + 1
            jsr         HFS_N_MIN
            jsr         HFS_TAIL                            ; ... or than there is before the end of file
            jsr         HFS_N_MIN
            lda         SD_POS                              ; SD_SRC = the block + (SD_POS & 511)
            clc
            adc         HFS_BLK
            sta         SD_SRC
            lda         SD_POS + 1
            and         #1
            adc         HFS_BLK + 1
            sta         SD_SRC + 1
            lda         SD_DONE                             ; SD_DST = the data area + SD_DONE
            sta         SD_DST
            lda         ZP_IO_REQ + 1
            inc
            sta         SD_DST + 1
            ldy         #0
            bit         HFS_HOLEF
            bmi         @zeros
            ldx         ROM_BANK_REG                        ; (Reads give this task's last write)
            phx
            lda         HFS_ROMB                            ; (The block's paged ROM bank, or this one)
            sta         ROM_BANK_REG
            _M_COPY_N   SD_SRC, SD_DST, SD_N                ; SD_N bytes (1-256; 0: 256): the block -> the data area
            pla
            sta         ROM_BANK_REG
            bra         @next

@zeros:                                                     ; (A hole's: zeros)
            lda         #0
:
            sta         (SD_DST),Y
            iny
            cpy         SD_N
            bne         :-

@next:
            jsr         HFS_ADVANCE
            jmp         HFS_FILE_READ

HFS_READ_ERR:
            jsr         IO_SRV_UNMAP
            sec
            rts

HFS_READ_DONE:
            ldy         #IO_BLK_COUNT                       ; The count done: what was wanted, less what's
            lda         (ZP_IO_REQ),Y                       ;   left (a read can stop at the end of file)
            sec
            sbc         SD_LEFT
            sta         SD_TMP
            iny
            lda         (ZP_IO_REQ),Y
            sbc         SD_LEFT + 1
            sta         (ZP_IO_REQ),Y
            dey
            lda         SD_TMP
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            lda         #0
            clc
            rts

; SD_POS and SD_DONE on by SD_N, SD_LEFT down by it (SD_DONE stays under 256: the count does).
; Modifies: .A
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
            rts

; ****************************************************************************
; Reading a directory: its entries as text lines, or as stat records (IO_MODE_STAT).  Both are made again
; from the start at every read and the bytes before the fd's offset thrown away, so a read at any offset
; gives the same listing and nothing has to be remembered between reads.  Directories are small.

HFS_DIR_DONE:                                               ; (Too far from the directory code to branch to)
            jmp         HFS_READ_DONE

HFS_DIR_ERR:
            jmp         HFS_READ_ERR

HFS_DIR_READ:
            lda         SD_POS + 2                          ; HFS_SKIP = the offset: what to throw away
            ora         SD_POS + 3
            bne         HFS_DIR_DONE                        ; (Further out than a listing reaches)
            lda         SD_POS
            sta         HFS_SKIP
            lda         SD_POS + 1
            sta         HFS_SKIP + 1
            stz         SD_POS                              ; Back to the directory's first entry
            stz         SD_POS + 1
            stz         SD_POS + 2
            stz         SD_POS + 3
            stz         SD_DST                              ; SD_DST = the data area (SD_DONE indexes it)
            lda         ZP_IO_REQ + 1
            inc
            sta         SD_DST + 1

HFS_DIR_ENTRY:
            lda         SD_LEFT                             ; The count is full: stop
            ora         SD_LEFT + 1
            beq         HFS_DIR_DONE
            jsr         HFS_AT_END                          ; The last entry: stop
            bcs         HFS_DIR_DONE
            jsr         HFS_FILE_BLOCK                      ; The block this entry is in
            bcs         HFS_DIR_ERR
            jsr         HFS_LOAD
            bcs         HFS_DIR_ERR
            lda         SD_POS                              ; HFS_PTR = the entry, in the cache
            sta         HFS_OFS
            lda         SD_POS + 1
            and         #1
            sta         HFS_OFS + 1
            jsr         HFS_AT
            lda         (HFS_PTR)
            beq         HFS_DIR_NEXT                        ; A free entry: nothing for it
            lda         SD_OP                               ; (The open mode)
            and         #IO_MODE_STAT
            beq         :+
            jsr         HFS_REC                             ; Its stat record, or ...
            bra         :++
:
            jsr         HFS_LINE                            ;   its line: in HFS_STAT, HFS_LEN long
:
            jsr         HFS_EMIT                            ; ... past HFS_SKIP, into the data area

HFS_DIR_NEXT:
            lda         SD_POS                              ; On to the next entry
            clc
            adc         #HFS_ENTRY_SIZE
            sta         SD_POS
            bcc         HFS_DIR_ENTRY
            inc         SD_POS + 1
            bne         HFS_DIR_ENTRY
            inc         SD_POS + 2
            bne         HFS_DIR_ENTRY
            inc         SD_POS + 3
            bra         HFS_DIR_ENTRY

; The HFS_LEN bytes in HFS_STAT: throw away HFS_SKIP of them, then hand over what the count still wants
; (into the data area at SD_DONE).  Modifies: .A, .X, .Y
HFS_EMIT:
            ldx         #0

@byte:
            cpx         HFS_LEN
            beq         @done
            lda         HFS_SKIP                            ; Still throwing bytes away (before the offset)?
            ora         HFS_SKIP + 1
            beq         @take
            lda         HFS_SKIP
            bne         :+
            dec         HFS_SKIP + 1
:
            dec         HFS_SKIP
            inx
            bra         @byte

@take:
            lda         SD_LEFT
            ora         SD_LEFT + 1
            beq         @done
            lda         HFS_STAT,X
            ldy         SD_DONE
            sta         (SD_DST),Y
            inc         SD_DONE
            lda         SD_LEFT
            bne         :+
            dec         SD_LEFT + 1
:
            dec         SD_LEFT
            inx
            bra         @byte

@done:
            rts

; The entry at HFS_PTR as a listing line, in HFS_STAT: "name size", or "name/" for a directory, and CR LF.
; OUT: HFS_LEN = its length.  Modifies: .A, .X, .Y
HFS_LINE:
            stz         HFS_LEN
            ldy         #0

@name:
            lda         (HFS_PTR),Y
            beq         @named
            jsr         HFS_PUT
            iny
            cpy         #HFS_NAME_MAX + 1
            bne         @name

@named:
            ldy         #HFS_E_MODE
            lda         (HFS_PTR),Y
            bpl         @size
            lda         #'/'                                ; A directory: its name and a '/', no size
            jsr         HFS_PUT
            bra         @end

@size:
            lda         #' '
            jsr         HFS_PUT
            ldy         #HFS_E_SIZE                         ; HFS_CL = the size, to put in decimal
            ldx         #0
:
            lda         (HFS_PTR),Y
            sta         HFS_CL,X
            iny
            inx
            cpx         #4
            bne         :-
            jsr         HFS_PUT_DEC

@end:
            lda         #ASCII_CR
            jsr         HFS_PUT
            lda         #ASCII_LF
            jmp         HFS_PUT

; Add HFS_CL (32 bits) in decimal to the line.  HFS_CL ends as 0.  Modifies: .A, .X, .Y
HFS_PUT_DEC:
            ldx         #0                                  ; The digits (on the stack, the last one first)

@digit:                                                     ; HFS_CL /= 10: .A = the remainder
            lda         #0
            ldy         #32

@bit:
            asl         HFS_CL
            rol         HFS_CL + 1
            rol         HFS_CL + 2
            rol         HFS_CL + 3
            rol
            cmp         #10
            bcc         :+
            sbc         #10                                 ; (C = 1)
            inc         HFS_CL
:
            dey
            bne         @bit
            pha
            inx
            lda         HFS_CL
            ora         HFS_CL + 1
            ora         HFS_CL + 2
            ora         HFS_CL + 3
            bne         @digit

@put:
            pla
            ora         #'0'
            jsr         HFS_PUT
            dex
            bne         @put
            rts

; Add .A to the line in HFS_STAT.  Preserves .Y (not .A)
HFS_PUT:
            phx
            ldx         HFS_LEN
            sta         HFS_STAT,X
            inc         HFS_LEN
            plx
            rts

; The entry at HFS_PTR as a stat record, in HFS_STAT: the entry without its extents, and the card in the
; entry's reserved byte.  OUT: HFS_LEN = IO_STAT_SIZE.  Modifies: .A, .Y
HFS_REC:
            ldy         #IO_STAT_SIZE - 1
:
            lda         (HFS_PTR),Y
            sta         HFS_STAT,Y
            dey
            bpl         :-
            lda         HFS_CARD
            sta         HFS_STAT + IO_ST_CARD
            lda         #IO_STAT_SIZE
            sta         HFS_LEN
            rts

; ****************************************************************************
; Walking a name

; Walk the name at (ZP_IO_REQ) (the client's data area: "/N", the card, then the path in it) down to the
; file or directory it names.
; OUT: C = 0: HFS_CARD = the card, HFS_LOC = where the entry is, HFS_ENT = the entry (and HFS_FP -> it);
;      C = 1, .A = ERR_IO_NOT_FOUND, ERR_IO_NAME, ERR_IO_NOT_FS or a card error
HFS_WALK:
            jsr         HFS_SPEC_CHECK                      ; (A disk in memory: through a spec?)
            bcc         :+
            rts
:
            jsr         HFS_AREA_CHECK                      ; (On the RAM disk: an area the client may use?)
            bcc         :+
            rts
:
            ldy         #0
            lda         (ZP_IO_REQ),Y                       ; "/N": the card
            cmp         #'/'
            bne         @no_disk
            iny
            lda         (ZP_IO_REQ),Y
            DISK_FROM_NAME                                  ; (0-7, x)
            bcc         @disk

@no_disk:
            jmp         HFS_W_NOT_FOUND

@disk:
            sta         HFS_CARD

@path:
            iny
            lda         (ZP_IO_REQ),Y                       ; Then the path in it, or nothing: its root
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
            jsr         HFS_ENT_READ
            bcs         HFS_W_DONE

HFS_W_ELEM:                                                 ; The next path element, if there is one
            ldy         HFS_ELEM
            lda         (ZP_IO_REQ),Y                       ; (A '/', or the name's 0)
            beq         HFS_W_DONE                          ; (C = 0 from HFS_ENT_READ: this is it)
            iny
            sty         HFS_ELEM
            lda         (ZP_IO_REQ),Y
            beq         HFS_W_DONE                          ; A trailing '/'
            cmp         #'.'
            bne         HFS_W_LOOK
            iny                                             ; "." (this directory) or ".." (the one above)?
            lda         (ZP_IO_REQ),Y
            beq         HFS_W_DOT
            cmp         #'/'
            beq         HFS_W_DOT
            cmp         #'.'
            bne         HFS_W_LOOK
            iny
            lda         (ZP_IO_REQ),Y
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
            jsr         HFS_ENT_READ
            bcc         HFS_W_ELEM
            bra         HFS_W_DONE

HFS_W_NOT_FOUND:
            lda         #ERR_IO_NOT_FOUND
            sec

HFS_W_DONE:
            rts

; Find the path element at HFS_ELEM in the directory HFS_ENT describes, and move HFS_ELEM past it.
; OUT: C = 0: HFS_LOC = where its entry is; or C = 1, .A = ERR_IO_NOT_FOUND, ERR_IO_NAME or a card error
HFS_LOOKUP:
            lda         #HFS_SCAN_NAME
            sta         HFS_SCAN
            jsr         HFS_DIR_SCAN
            bcs         @done
            ldx         #4
:
            lda         HFS_NLOC,X
            sta         HFS_LOC,X
            dex
            bpl         :-

@done:
            rts

HFS_SCAN_FREE       = 0         ; HFS_DIR_SCAN: a free entry
HFS_SCAN_USED       = 1         ;   an entry in use
HFS_SCAN_NAME       = $80       ;   the entry named by the path element at HFS_ELEM

; Look through the directory whose entry is at HFS_FP for an entry: HFS_SCAN says which.  For a name,
; HFS_LEN = its length, and HFS_ELEM moves past it.
; OUT: C = 0: HFS_NLOC = where it is, SD_POS = its offset in the directory, HFS_PTR -> it (in the cache);
;      or C = 1, .A = ERR_IO_NOT_FOUND (none), ERR_IO_NAME (too long a name) or a card error
HFS_DIR_SCAN:
            bit         HFS_SCAN
            bpl         @entries
            ldy         HFS_ELEM                            ; How long the element is
            ldx         #0
:
            lda         (ZP_IO_REQ),Y
            beq         :+
            cmp         #'/'
            beq         :+
            iny
            inx
            cpx         #HFS_NAME_MAX + 1
            bne         :-
            lda         #ERR_IO_NAME                        ; Longer than a name can be
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
            lda         #ERR_IO_NOT_FOUND
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
            lda         (ZP_IO_REQ),Y
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
; OUT: C = 0; or C = 1, .A = ERR_IO_NAME (the path has more elements than HFS_DEPTH_MAX)
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
            lda         #ERR_IO_NAME
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
; OUT: C = 0; or C = 1, .A = ERR_IO_NOT_FS, ERR_IO_NOT_READY or a card error
HFS_VOLUME:
            ldx         HFS_CARD
            lda         HFS_V_STATE,X
            beq         @look
            bmi         @not_fs                             ; ($FF: looked at, and it isn't one)
            clc
            rts

@not_fs:
            lda         #ERR_IO_NOT_FS
            sec
            rts

@look:
            lda         SD_CARD_STATE,X
            bne         :+
            lda         HFS_CARD                            ; Not started yet: start it (it may not be there)
            sta         SD_DEV
            jsr         SD_START
            bcc         @far1
            jmp         @fail
@far1:
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
            bcs         @no
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
            bcs         @fail
            jsr         HFS_SB_OK
            bcs         @no

@superblock:
            jsr         HFS_CARD_X                          ; The numbers the server works from: 4 of 4 bytes,
            ldy         #HFS_SB_CLUSTERS                    ;   4 apart in the block and HFS_V_STRIDE apart in
                                                            ;   RAM (at the disk * 4), four arrays to a page
@number:
            lda         (SD_CACHE),Y
            sta         HFS_V_CLUSTERS,X
            iny
            inx
            txa
            and         #3                                  ; (.X started as a multiple of 4)
            bne         @number
            txa
            clc
            adc         #HFS_V_STRIDE - 4                   ; The next one, in RAM
            tax
            bcc         @number                             ; (Past the page's fourth, .X wraps: the disk * 4)

@counter:                                                   ; The counters: the next page (HFS_V_FREE ...)
            lda         (SD_CACHE),Y
            sta         HFS_V_FREE,X
            iny
            inx
            txa
            and         #3
            bne         @counter
            txa
            clc
            adc         #HFS_V_STRIDE - 4
            tax
            bcc         @counter
            jsr         HFS_CARD_X                          ; HFS_V_MINIT (256 on from HFS_V_CLUSTERS: out
            ldy         #HFS_SB_MAPINIT                     ;   of the loop's reach): the superblock's; for
:                                                           ;   version 1, the map's size (all written)
            lda         (SD_CACHE),Y
            sta         HFS_V_MINIT,X
            inx
            iny
            cpy         #HFS_SB_MAPINIT + 4
            bne         :-
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
            lda         #ERR_IO_NOT_FS

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
; Page 3's block calls, for a card's HydraFS: its block numbers count from its partition's first block
; (HFS_V_BASE: 0 on a card that's all HydraFS), so SD_LBA is moved there for the call, and back after it.
; IN and OUT as the calls they stand for (SD_DEV = the card, SD_LBA = the block; C = 1, .A = a card error).
; Modifies: .A, .X, .Y
SD_CACHE_LOAD:
            jsr         HFS_BASE_ADD
            jsr         SD_CACHE_LOAD_P3
            bra         HFS_BASE_SUB

SD_READ_BLOCK:
            jsr         HFS_BASE_ADD
            jsr         SD_READ_BLOCK_P3
            bra         HFS_BASE_SUB

SD_WRITE_BLOCK:
            jsr         HFS_BASE_ADD
            jsr         SD_WRITE_BLOCK_P3

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

; Card SD_DEV is being started (again): its superblock must be read afresh, since a different card may be
; in the socket now, and any HydraFS file open on it is let go of (those fds give ERR_IO_BAD_FD from here
; on, and close as usual).  Called by SD_START.  Modifies: .A, .X
HFS_FORGET:
            ldx         SD_DEV
            stz         HFS_V_STATE,X
            ldx         #(HFS_MAX_OPEN - 1) * HFS_FHDR_SIZE
:
            lda         HFS_FHDR + HFS_H_CARD,X
            cmp         SD_DEV
            bne         :+
            lda         #$FF
            sta         HFS_FHDR + HFS_H_CARD,X
:
            txa
            sec
            sbc         #HFS_FHDR_SIZE
            tax
            bpl         :--
            rts

.pushseg
.segment "HIGH_P6"      ; (Page 6's room above COMMON, $FE00)

; Block SD_LBA of disk HFS_CARD, for a file's read (HFS_FILE_READ): HFS_BLK = where it is, seen with paged ROM bank
; HFS_ROMB.  The ROM disk's is read where it is, in the paged ROM at $A000-$DFFF (as SD_ROM_READ finds it), with
; no copy into the cache first (it never changes, so the cache can't have a newer one); any other disk's is read
; into the cache (HFS_LOAD), and HFS_ROMB is the bank this task has.  OUT: C = 0; or C = 1, .A = an error.
; Modifies: .A, .X, .Y
HFS_BLOCK_AT:
            lda         ROM_BANK_REG
            sta         HFS_ROMB
            lda         HFS_CARD
            cmp         #DISK_ROM
            beq         @rom
            lda         SD_CACHE
            sta         HFS_BLK
            lda         SD_CACHE + 1
            sta         HFS_BLK + 1
            bra         HFS_LOAD

@rom:
            sta         SD_DEV
            jsr         HFS_BASE_ADD                        ; SD_LBA: on the disk
            lda         SD_LBA + 3
            ora         SD_LBA + 2
            bne         @past
            lda         SD_LBA + 1
            cmp         #>DISK_ROM_BLOCKS
            bcs         @past
            asl                                             ; The bank: SD_LBA / 32 (13 bits: 8 of them)
            asl
            asl
            sta         HFS_ROMB
            lda         SD_LBA
            lsr
            lsr
            lsr
            lsr
            lsr
            ora         HFS_ROMB
            sta         HFS_ROMB
            lda         SD_LBA                              ; Where in it: $A000 + (SD_LBA % 32) * 512
            and         #$1F
            asl
            adc         #>PAGED_ROM_BASE                    ; (C = 0: the asl's bit 7 was 0)
            sta         HFS_BLK + 1
            stz         HFS_BLK
            clc
            jmp         HFS_BASE_SUB                        ; (SD_LBA back: keeps C)

@past:
            lda         #ERR_IO_MEDIA
            sec
            jmp         HFS_BASE_SUB
.assert     DISK_ROM_BLOCKS / 32 <= 256, error, "HFS_BLOCK_AT: the ROM disk's banks, 8 bits"

; Read block SD_LBA of card HFS_CARD into the cache (SD_CACHE_LOAD, which shares it with /dev/sd).
; OUT: C = 0; or C = 1, .A = a card error.  Modifies: .A, .X, .Y
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
.popseg

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

; Find an open file (from header .X on) whose entry is the one at HFS_LOC on card HFS_CARD.
; OUT: C = 0: .X = its header, HFS_PTR -> its copy of the entry; or C = 1: none.  Modifies: .A
HFS_SLOT_FIND:
            cpx         #HFS_MAX_OPEN * HFS_FHDR_SIZE
            bcs         @done
            lda         HFS_FHDR + HFS_H_CARD,X             ; (A free slot's card is $FF)
            cmp         HFS_CARD
            bne         @next
            lda         HFS_FHDR + HFS_H_EIDX,X
            cmp         HFS_LOC + 4
            bne         @next
            lda         HFS_FHDR + HFS_H_EBLK,X
            cmp         HFS_LOC
            bne         @next
            lda         HFS_FHDR + HFS_H_EBLK + 1,X
            cmp         HFS_LOC + 1
            bne         @next
            lda         HFS_FHDR + HFS_H_EBLK + 2,X
            cmp         HFS_LOC + 2
            bne         @next
            lda         HFS_FHDR + HFS_H_EBLK + 3,X
            cmp         HFS_LOC + 3
            bne         @next
            txa                                             ; Its copy: HFS_FILES + the header * 4 (the
            asl                                             ;   fid * 64)
            asl
            sta         HFS_PTR
            lda         #>HFS_FILES
            adc         #0
            sta         HFS_PTR + 1
            clc
            rts

@next:
            txa
            clc
            adc         #HFS_FHDR_SIZE
            tax
            bra         HFS_SLOT_FIND

@done:
            rts                                             ; (C = 1)

.assert     HFS_FHDR_SIZE * 4 = HFS_ENTRY_SIZE, error, "HFS_SLOT_FIND: a copy's offset is its header's * 4"
.assert     <HFS_FILES = 0, error, "HFS_SLOT_FIND: HFS_FILES starts a page"

; SD_LBA = the card block holding byte SD_POS of the file whose entry is at HFS_FP: the file's cluster,
; then the extent it's in (the entry's two, then its extent blocks').
; OUT: C = 0; or C = 1, .A = ERR_IO_EOF (past the file's clusters) or a card error; or C = 1, .A =
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
            lda         #ERR_IO_EOF
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
; OUT: C = 0: HFS_FID = it, HFS_CARD = its card, SD_OP = its open mode, .X = its header, HFS_FP = its
;      entry, HFS_LOC = where the entry is on the card; or C = 1, .A = ERR_IO_BAD_FD
HFS_FID_CHECK:
            lda         SD_FID
            cmp         #HFS_MAX_OPEN
            bcs         @bad
            sta         HFS_FID
            asl
            asl
            asl
            asl
            tax
            lda         HFS_FHDR + HFS_H_CARD,X
            bmi         @bad                                ; ($FF: not open)
            sta         HFS_CARD
            lda         HFS_FHDR + HFS_H_OMODE,X
            sta         SD_OP
            lda         HFS_FHDR + HFS_H_EIDX,X             ; HFS_LOC = where its entry is
            sta         HFS_LOC + 4
            lda         HFS_FHDR + HFS_H_EBLK,X
            sta         HFS_LOC
            lda         HFS_FHDR + HFS_H_EBLK + 1,X
            sta         HFS_LOC + 1
            lda         HFS_FHDR + HFS_H_EBLK + 2,X
            sta         HFS_LOC + 2
            lda         HFS_FHDR + HFS_H_EBLK + 3,X
            sta         HFS_LOC + 3
            jsr         HFS_FILE_PTR                        ; (It leaves .X alone)
            clc
            rts

@bad:
            lda         #ERR_IO_BAD_FD
            sec
            rts

.assert     HFS_FHDR_SIZE = 16, error, "HFS_FID_CHECK shifts the fid by 4 for its header"

; A free open file slot.  OUT: C = 0: HFS_FID = it, .X = its header; or C = 1, .A = ERR_IO_NO_FDS
; (HFS_FID_NEW doesn't point HFS_FP at it: H9_OPEN does that when it has filled the header in.)
HFS_FID_NEW:
            ldx         #0
            stz         HFS_FID

@slot:
            lda         HFS_FHDR + HFS_H_CARD,X
            bmi         @free                               ; ($FF)
            inc         HFS_FID
            txa
            clc
            adc         #HFS_FHDR_SIZE
            tax
            cpx         #HFS_MAX_OPEN * HFS_FHDR_SIZE
            bne         @slot
            lda         #ERR_IO_NO_FDS
            sec
            rts

@free:
            clc
            rts

; HFS_FP = the open file HFS_FID's copy of its directory entry (HFS_FILES + the fid * HFS_ENTRY_SIZE).
; Preserves .X, .Y.  Modifies: .A
HFS_FILE_PTR:
            lda         HFS_FID
            lsr                                             ; The fid * 64: (fid >> 2) pages, and
            lsr                                             ;   (fid & 3) * 64 in the page
            clc
            adc         #>HFS_FILES
            sta         HFS_FP + 1
            lda         HFS_FID
            and         #3
            asl
            asl
            asl
            asl
            asl
            asl
            sta         HFS_FP
            rts

.assert     HFS_ENTRY_SIZE = 64 && HFS_MAX_OPEN = 8, error, "HFS_FILE_PTR: 8 entries of 64 bytes, in 2 pages"
