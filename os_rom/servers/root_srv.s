.debuginfo

; ****************************************************************************
; The root, `/`, and `/dev`, as directories (BIOS ROM page D; included inside `.scope PAGED`, see all.s): the device
; root, as Plan 9's root device (#/) is, served in its client's task (IO_DEV_CALLER_TASK: the shell registers it).
; IO_OPEN sends it the names no namespace entry has: "/" as /dev/root, and "/dev" as /dev/root/dev (io.s).
;   /       a line a name in the root: "dev/", then the first element of each namespace entry, the task's and the
;           system's (each once; not a hide's, nor one the task hides: `hide /sram`), as "name/"
;   /dev    a line a device in the device table (DEV_REGISTER), as "name" (but root)
; Opened with IO_MODE_STAT, each gives 48-byte stat records instead (a name, and the mode: a directory for the
; root's).  The listing is made again at every read, and the bytes before the offset thrown away, as HydraFS's
; directories are.  Stat: the directory's own record.  Read-only; the filesystem's requests refused.
;   The listing's routines are for the other servers' directories too (the devices' own: /dev/sd, /dev/gpio ...):
; DIR_LIST, a list of names as a read's listing, and DIR_STAT, a directory's stat record (their gates: page 3's).
; Server ZP: ZP_CS (the client-task servers' scratch: ROOT_*, below); ZP_IO_REQ (the request block).

ROOT_FID        = ZP_CS + 0                             ; The fid (ROOT_FID_DEV); a listing's ROOT_FID_STAT, _ISDIR
ROOT_SKIP       = ZP_CS + 1                             ; A read: the bytes still to throw away (2) ...
ROOT_LEFT       = ZP_CS + 3                             ;   the room left (2) ...
ROOT_OUT        = ZP_CS + 5                             ;   where the next byte goes in the data area ...
ROOT_N          = ZP_CS + 6                             ;   and the bytes of the name's line or record so far
ROOT_K          = ZP_CS + 7                             ; The entry being listed (0-31: the task's, 32-63: the
ROOT_J          = ZP_CS + 8                             ;   system's; /dev: the device), and one it's checked against
ROOT_P          = ZP_CS + 9                             ;   their addresses (2 each; /dev: the device's entry, and
ROOT_Q          = ZP_CS + 11                            ;   a byte of "root")
ROOT_XB         = ZP_CS + 12                            ; /dev: the client's transfer bank, the device table's between
                                                        ;   (ROOT_Q's high byte: the root's listing's alone)
            CS_FITS     ROOT_FID, 13

ROOT_FID_DEV    = $01                                   ; The fid: /dev (0: the root)
ROOT_FID_STAT   = $80                                   ; A listing (ROOT_FID): stat records (the fd's IO_MODE_STAT) ...
ROOT_FID_ISDIR  = $40                                   ;   and the name being listed is a directory's

.segment "ROOT_PD"

; A request.  IN: .A = request, .X = client, .Y = fid
ROOT_SERVE:
            sty         ROOT_FID
            cmp         #H9_OPEN
            beq         ROOT_OPEN
            cmp         #H9_READ
            bne         :+
            jmp         ROOT_READ
:
            cmp         #H9_STAT
            bne         :+
            jmp         ROOT_STAT
:
            cmp         #H9_CLUNK
            beq         ROOT_OK
            cmp         #H9_DUP
            beq         ROOT_OK
            lda         #ERR_IO_BAD_REQ                     ; (Writes, ctl, and the filesystem's requests)
            sec
            rts

ROOT_OK:
            lda         #0
            clc
            rts

; The rest of the name: "" (the root) or "/dev".  Read only.  OUT: .A = the fid
ROOT_OPEN:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_MODE
            lda         (ZP_IO_REQ),Y
            and         #IO_MODE_WRITE
            bne         @mode
            stz         ROOT_FID
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
            lda         (ZP_IO_REQ),Y
            beq         @open                               ; "": the root
            ldy         #4                                  ; "/dev"?
:
            lda         (ZP_IO_REQ),Y
            cmp         ROOT_S_DEV,Y
            bne         @not_found
            dey
            bpl         :-
            lda         #ROOT_FID_DEV
            tsb         ROOT_FID

@open:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         ROOT_FID
            clc
            rts

@not_found:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

@mode:
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_MODE
            sec
            rts

ROOT_S_DEV:     .byte   "/dev", 0
ROOT_S_ROOT:    .byte   "root", 0                       ; (Not in /dev's listing)

; The directory's own stat record: its name ("" or "dev") and its mode (a directory)
ROOT_STAT:
            jsr         IO_SRV_MAP
            lda         #<(ROOT_S_DEV + 4)                  ; ("": the root)
            ldy         #>(ROOT_S_DEV + 4)
            ldx         ROOT_FID
            beq         :+
            lda         #<(ROOT_S_DEV + 1)                  ; ("dev")
            ldy         #>(ROOT_S_DEV + 1)
:
            jsr         DIR_STAT
            jsr         IO_SRV_UNMAP
            jmp         ROOT_OK

; The listing, from the offset, up to the count; the count set to what it gave
ROOT_READ:
            jsr         IO_SRV_MAP
            lda         RAM_BANK_REG                        ; (/dev's)
            sta         ROOT_XB
            lda         ROOT_FID
            pha
            jsr         DIR_BEGIN
            pla
            bcs         @done                               ; (Nothing there)
            and         #ROOT_FID_DEV
            beq         :+
            jsr         ROOT_DEV_LIST
            bra         @listed
:
            jsr         ROOT_LIST

@listed:
            jsr         DIR_END

@done:
            jsr         IO_SRV_UNMAP
            jmp         ROOT_OK

; A read's listing starts (the client's request block mapped: ZP_IO_REQ): ROOT_SKIP = the offset, ROOT_LEFT = the
; count; ROOT_FID = stat records or not (the fd's IO_MODE_STAT).  OUT: C = 1: the offset's past 64K, no listing
; reaches there: the count set to 0.  Modifies: .A, .Y
DIR_BEGIN:
            ldy         #IO_BLK_MODE
            lda         (ZP_IO_REQ),Y
            and         #IO_MODE_STAT
            beq         :+
            lda         #ROOT_FID_STAT
:
            sta         ROOT_FID
            stz         ROOT_OUT
            stz         ROOT_N
            ldy         #IO_BLK_OFS
            lda         (ZP_IO_REQ),Y
            sta         ROOT_SKIP
            iny
            lda         (ZP_IO_REQ),Y
            sta         ROOT_SKIP + 1
            iny
            lda         (ZP_IO_REQ),Y
            iny
            ora         (ZP_IO_REQ),Y
            bne         @past
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ROOT_LEFT
            iny
            lda         (ZP_IO_REQ),Y
            sta         ROOT_LEFT + 1
            clc
            rts

@past:
            ldy         #IO_BLK_COUNT
            lda         #0
            sta         (ZP_IO_REQ),Y
            iny
            sta         (ZP_IO_REQ),Y
            sec
            rts

; The listing's end: the count set to what it gave (what it asked for, less the room left).  Modifies: .A, .Y
DIR_END:
            ldy         #IO_BLK_COUNT
            sec
            lda         (ZP_IO_REQ),Y
            sbc         ROOT_LEFT
            sta         (ZP_IO_REQ),Y
            iny
            lda         (ZP_IO_REQ),Y
            sbc         ROOT_LEFT + 1
            sta         (ZP_IO_REQ),Y
            rts

; A read of a directory whose names are known (a device's: /dev/sd, /dev/gpio ...): its listing, as lines or stat
; records (the fd's IO_MODE_STAT), from the offset, up to the count; the count set.  IN: the client's request block
; mapped (ZP_IO_REQ); .A.Y = the names, zero-terminated, then an empty one (in the task's RAM, or on page D): a name
; that ends in '/' is a directory's.  Modifies: .A, .X, .Y, ZP_CS
DIR_LIST:
            sta         ROOT_P
            sty         ROOT_P + 1
            jsr         DIR_BEGIN
            bcs         @done

@name:
            lda         (ROOT_P)
            beq         @end                                ; (The empty one: the end)
            ldy         #0                                  ; ROOT_J = its length, with its 0
:
            iny
            lda         (ROOT_P),Y
            bne         :-
            iny
            sty         ROOT_J
            dey
            dey
            sty         ROOT_K                              ; ROOT_K = its name's: but a '/' at its end
            lda         #ROOT_FID_ISDIR
            trb         ROOT_FID
            lda         (ROOT_P),Y
            cmp         #'/'
            bne         :+
            lda         #ROOT_FID_ISDIR                     ; (A directory)
            tsb         ROOT_FID
            bra         :++
:
            inc         ROOT_K
:
            ldy         #0
:
            cpy         ROOT_K
            beq         :+
            lda         (ROOT_P),Y
            jsr         ROOT_EMIT
            iny
            bra         :-
:
            jsr         ROOT_EMIT_END
            lda         ROOT_P                              ; The next name
            clc
            adc         ROOT_J
            sta         ROOT_P
            bcc         @name
            inc         ROOT_P + 1
            bra         @name

@end:
            jsr         DIR_END

@done:
            rts

; A directory's own stat record, in the data area (the client's request block mapped: ZP_IO_REQ): its name (.A.Y:
; zero-terminated, in the task's RAM or on page D), and its mode, a directory's; the rest 0.  Modifies: .A, .Y
DIR_STAT:
            sta         ROOT_P
            sty         ROOT_P + 1
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #IO_STAT_SIZE - 1
            lda         #0
:
            sta         (ZP_IO_REQ),Y
            dey
            bpl         :-
            ldy         #IO_ST_MODE
            lda         #HFS_M_DIR
            sta         (ZP_IO_REQ),Y
            ldy         #0
:
            lda         (ROOT_P),Y
            beq         :+
            sta         (ZP_IO_REQ),Y
            iny
            cpy         #IO_ST_MODE - 1
            bne         :-
:
            dec         ZP_IO_REQ + 1
            rts

; The root's names: "dev", then each entry's first element (ROOT_K's), unless an earlier entry has it (ROOT_J
; < ROOT_K) or the task hides it.  Modifies: .A, .X, .Y
ROOT_LIST:
            lda         #ROOT_FID_ISDIR                     ; (Directories, all)
            tsb         ROOT_FID
            ldx         #0                                  ; "dev"
:
            lda         ROOT_S_DEV + 1,X
            beq         :+
            jsr         ROOT_EMIT
            inx
            bra         :-
:
            jsr         ROOT_EMIT_END
            stz         ROOT_K

@entry:
            lda         ROOT_K
            jsr         ROOT_ENTRY                          ; ROOT_P = it
            sta         ROOT_P
            sty         ROOT_P + 1
            lda         (ROOT_P)
            and         #NS_KIND
            beq         @next                               ; (Free)
            cmp         #NS_HIDE
            beq         @next
            ldy         #NS_PREFIX + 1                      ; (After its '/')
            lda         (ROOT_P),Y
            beq         @next                               ; ("/": no name)
            ldx         #0                                  ; "dev": listed already
:
            lda         ROOT_S_DEV + 1,X
            beq         :+
            cmp         (ROOT_P),Y
            bne         @earlier
            iny
            inx
            bra         :-
:
            lda         (ROOT_P),Y
            beq         @next
            cmp         #'/'
            beq         @next

@earlier:
            stz         ROOT_J                              ; An earlier entry's?

@j:
            lda         ROOT_J
            cmp         ROOT_K
            beq         @hidden
            jsr         ROOT_ENTRY_Q
            and         #NS_KIND
            beq         @j_next                             ; (Free)
            cmp         #NS_HIDE
            beq         @j_next
            jsr         ROOT_SAME
            bcs         @next

@j_next:
            inc         ROOT_J
            bra         @j

@hidden:
            stz         ROOT_J                              ; Hidden by the task: a hide of "/name" itself

@h:
            lda         ROOT_J
            jsr         ROOT_ENTRY_Q
            and         #NS_KIND
            cmp         #NS_HIDE
            bne         @h_next
            jsr         ROOT_SAME                           ; (.Y: after the name, in ROOT_Q's too)
            bcc         @h_next
            lda         (ROOT_Q),Y
            beq         @next

@h_next:
            inc         ROOT_J
            lda         ROOT_J
            cmp         #NS_ENTRIES
            bne         @h
            ldy         #NS_PREFIX + 1                      ; Listed: its name
:
            lda         (ROOT_P),Y
            beq         :+
            cmp         #'/'
            beq         :+
            jsr         ROOT_EMIT
            iny
            bra         :-
:
            jsr         ROOT_EMIT_END

@next:
            inc         ROOT_K
            lda         ROOT_K
            cmp         #NS_ENTRIES * 2
            beq         :+
            jmp         @entry
:
            rts

; ROOT_Q = entry .A, and .A = its type byte
ROOT_ENTRY_Q:
            jsr         ROOT_ENTRY
            sta         ROOT_Q
            sty         ROOT_Q + 1
            lda         (ROOT_Q)
            rts

; Entry .A's address (0-31: the task's, in its transfer area; 32-63: the system's, NS_SYS).  OUT: .A.Y
ROOT_ENTRY:
            pha
            lsr                                             ; (.A / 8: its page in the table)
            lsr
            lsr
            cmp         #NS_PAGES
            bcs         @sys
            adc         #>IO_BLK_NS                         ; (C = 0)
            adc         ZP_IO_REQ + 1
            bra         @page

@sys:
            sbc         #NS_PAGES                           ; (C = 1)
            clc
            adc         #>NS_SYS

@page:
            tay
            pla
            and         #7                                  ; (8 entries a page)
            asl
            asl
            asl
            asl
            asl
            rts
.assert     NS_ENTRY_SIZE = 32 .and NS_PAGES = 4 .and <IO_BLK_NS = 0, error, "ROOT_ENTRY: 8 entries of 32 bytes a page"

; Do ROOT_P's and ROOT_Q's paths start with the same element?  OUT: C = 1 yes, .Y = just after it (in both)
ROOT_SAME:
            ldy         #NS_PREFIX + 1

@char:
            lda         (ROOT_P),Y
            beq         @end_p
            cmp         #'/'
            beq         @end_p
            cmp         (ROOT_Q),Y
            bne         @no
            iny
            bra         @char

@end_p:                                                     ; ROOT_P's ends: ROOT_Q's must too
            lda         (ROOT_Q),Y
            beq         @yes
            cmp         #'/'
            bne         @no

@yes:
            sec
            rts

@no:
            clc
            rts

; /dev's names: the device table's (but root), a line or a stat record each.  Modifies: .A, .X, .Y
ROOT_DEV_LIST:
            lda         #ROOT_FID_ISDIR                     ; (Files, as far as /dev knows)
            trb         ROOT_FID
            stz         ROOT_K                              ; (The device)

@dev:
            lda         ROOT_K
            asl
            asl
            asl
            asl
            sta         ROOT_P                              ; (Its entry, in the table)
            tay
            jsr         ROOT_DEV_BYTE
            beq         @next                               ; (Free)
            ldx         #0                                  ; root: not listed
:
            lda         ROOT_S_ROOT,X
            sta         ROOT_Q
            jsr         ROOT_DEV_BYTE
            cmp         ROOT_Q
            bne         @name
            iny
            inx
            cpx         #5
            bne         :-
            bra         @next

@name:
            ldy         ROOT_P                              ; Its name: up to IO_DEV_NAME_LEN, zero-padded
            ldx         #IO_DEV_NAME_LEN
:
            jsr         ROOT_DEV_BYTE
            beq         :+
            jsr         ROOT_EMIT
            iny
            dex
            bne         :-
:
            jsr         ROOT_EMIT_END

@next:
            inc         ROOT_K
            lda         ROOT_K
            cmp         #IO_MAX_DEVS
            bne         @dev
            rts
.assert     IO_DEV_NAME = 0 .and IO_DEV_SIZE = 16 .and IO_MAX_DEVS * IO_DEV_SIZE = 256, error, "ROOT_DEV_LIST: 16 entries of 16 bytes"

; The device table's byte .Y (IO_DEV_TABLE, in the system's bank), with the client's transfer bank mapped again
; after.  OUT: .A, and Z by it.  Preserves .X, .Y
ROOT_DEV_BYTE:
            phx
            ldx         #SYS_BANK
            stx         RAM_BANK_REG
            ldx         IO_DEV_TABLE,Y
            lda         ROOT_XB
            sta         RAM_BANK_REG
            txa
            plx
            ora         #0
            rts
.assert     SYS_SHARED_U = 0, error, "ROOT_DEV_BYTE: the system's bank and the transfer banks share U = 0"

; A name's end, in the listing: for a line, "/" (a directory's: ROOT_FID_ISDIR) and CR LF; for a stat record, the
; name's 0s to IO_ST_MODE, the mode (HFS_M_DIR, or 0), and 0s to its end.  Modifies: .A, .X
ROOT_EMIT_END:
            bit         ROOT_FID
            bmi         @stat
            bvc         :+
            lda         #'/'
            jsr         ROOT_EMIT
:
            lda         #ASCII_CR
            jsr         ROOT_EMIT
            lda         #ASCII_LF
            jsr         ROOT_EMIT
            stz         ROOT_N
            rts

@stat:
            lda         #0                                  ; The name's 0s
:
            ldx         ROOT_N
            cpx         #IO_ST_MODE
            bcs         :+
            jsr         ROOT_EMIT
            bra         :-
:
            lda         #0                                  ; The mode
            bit         ROOT_FID
            bvc         :+
            lda         #HFS_M_DIR
:
            jsr         ROOT_EMIT
            lda         #0                                  ; The rest
:
            jsr         ROOT_EMIT
            ldx         ROOT_N
            cpx         #IO_STAT_SIZE
            bcc         :-
            stz         ROOT_N
            rts

; A byte of the listing: thrown away while ROOT_SKIP lasts, then into the data area while ROOT_LEFT does (and
; then dropped); counted in ROOT_N either way.  IN: .A.  Preserves .A, .X, .Y
ROOT_EMIT:
            inc         ROOT_N
            pha
            lda         ROOT_SKIP
            ora         ROOT_SKIP + 1
            beq         @put
            lda         ROOT_SKIP                           ; (Before the offset)
            bne         :+
            dec         ROOT_SKIP + 1
:
            dec         ROOT_SKIP
            pla
            rts

@put:
            lda         ROOT_LEFT
            ora         ROOT_LEFT + 1
            bne         :+
            pla                                             ; (The count's full)
            rts
:
            lda         ROOT_LEFT
            bne         :+
            dec         ROOT_LEFT + 1
:
            dec         ROOT_LEFT
            pla
            phy
            ldy         ROOT_OUT
            inc         ZP_IO_REQ + 1                       ; (The data area)
            sta         (ZP_IO_REQ),Y
            dec         ZP_IO_REQ + 1
            inc         ROOT_OUT
            ply
            rts
