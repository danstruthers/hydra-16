.debuginfo

; ****************************************************************************
; The storage task's file server: /dev/sd, the SD cards (BIOS ROM page 3, the storage page; included
; inside `.scope PAGE3`, see all.s).  The storage task (STORAGE_TASK_NUM) owns the SPI bus, so its
; requests run one at a time.  The HydraFS server will live here too, on the same block cache.
;   /dev/sd/N/data      card N (SPI device 0-7) as one big file of bytes.  Open: starts the card (SD_INIT)
;                       if it isn't yet.  Read / write: at the fd's offset (IO_SEEK), through a one-block
;                       cache (SD_CACHE, 512 bytes from the MMU); writes go through to the card at once.
;                       The offset is 32 bits, so this reaches the first 4 GB of the card (the HydraFS
;                       server uses block numbers, for all of it).
;   /dev/sd/N/ctl       read: the card, as a line of text: "sdhc 7580 MB 15523840 blocks" (or sdsc), or
;                       "none" (it starts the card first if it isn't yet).  Write: a command: "init"
;                       starts the card again (e.g. after changing it); "format [-f] [-s size] [label]"
;                       makes an empty HydraFS on it (HFS_FORMAT, hfs_format.s); "label <text>" sets its
;                       HydraFS label; "check" and
;                       "check fix" check its HydraFS (HFS_CHECK).  A HydraFS card's text has lines about
;                       it too: its label, its free space, the last check's results (HFS_CTL_LINES).
;   Ctl (either file): SD_CTL_INIT starts the card again.
; Server ZP (the storage task's): SD_* (zero.s); ZP_IO_REQ (IO_SRV_MAP).  Each card's state and size:
; SD_CARD_STATE, SD_CARD_BLOCKS (the storage task's RAM).

.segment "STORAGE_P3"

; The storage task's init (from STORAGE_INIT on page 0, in the task): the block cache, SPI idle.
; OUT: C = 0; or C = 1, .A = error
STORAGE_INIT3:
            lda         #HFS_SCRATCH_FLOOR                  ; HydraFS's scratch page ($0800): not the MMU's
            jsr         MM_SET_FLOOR
            bcs         @done
            lda         #$FF                                ; (No progress being shown)
            sta         HFS_PG_TENS
            stz         HFS_WZERO                           ; (Writes write the request's bytes)
            ldx         #SD_MAX_CARDS - 1                   ; (The cards start at their first open)
:
            stz         SD_CARD_STATE,X
            stz         HFS_V_STATE,X                       ; (Nor is a card's superblock read before then)
            dex
            bpl         :-
            lda         #$FF                                ; No HydraFS check yet, nor its buffer
            sta         HFS_CK_CARD
            stz         HFS_CK_BUF + 1
            ldx         #(HFS_MAX_OPEN - 1) * HFS_FHDR_SIZE ; No HydraFS files open
:
            lda         #$FF
            sta         HFS_FHDR + HFS_H_CARD,X
            txa
            sec
            sbc         #HFS_FHDR_SIZE
            tax
            bpl         :-
            stz         SD_CVALID
            jsr         SPI_INIT
            lda         #<512
            ldy         #>512
            ldx         #0
            jsr         MM_ALLOC                            ; Whole pages: they don't move
            bcs         @done
            jsr         MM_LOCK                             ; .A.Y = the address
            sta         SD_CACHE
            sty         SD_CACHE + 1
            lda         #<512                               ; And HydraFS's metadata buffer
            ldy         #>512
            ldx         #0
            jsr         MM_ALLOC
            bcs         @done
            jsr         MM_LOCK
            sta         HFS_META
            sty         HFS_META + 1
            lda         #<512                               ; And the block of zeros a new map block gets
            ldy         #>512                               ;   (HFS_MAP_WRITTEN)
            ldx         #0
            jsr         MM_ALLOC
            bcs         @done
            jsr         MM_LOCK
            sta         HFS_ZBUF
            sty         HFS_ZBUF + 1
            stz         HFS_MSTATE
            stz         HFS_SBDIRTY
            clc

@done:
            rts

; IN: .A = request, .X = client, .Y = fid
SD_SERVE:
            stx         SD_CLIENT
            sty         SD_FID
            pha
            tya
            and         #SD_MAX_CARDS - 1
            sta         SD_DEV                              ; The card (not for H9_OPEN: it has no fid)
            pla
            cmp         #H9_CREATE
            bcs         SD_BAD                              ; (The filesystem's requests: that's hfs)
            cmp         #H9_OPEN
            beq         SD_OPEN
            cmp         #H9_READ
            beq         SD_REQ_RW
            cmp         #H9_WRITE
            beq         SD_REQ_RW
            cmp         #H9_CTL
            beq         SD_REQ_CTL
            cmp         #H9_STAT
            beq         SD_BAD

SD_OK:                                                      ; H9_CLUNK, H9_DUP
            lda         #0
            clc
            rts

SD_REQ_RW:
            ldx         SD_FID
            cpx         #SD_FID_CTL
            bcs         :+
            jmp         SD_RW
:
            cmp         #H9_READ
            bne         :+
            jmp         SD_CTL_READ
:
            jmp         SD_CTL_WRITE

SD_REQ_CTL:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_CTL_CODE
            lda         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            cmp         #SD_CTL_INIT
            bne         SD_BAD

SD_RESTART:                                                 ; Start card SD_DEV (again)
            jsr         SD_START
            bcc         SD_OK
            rts

SD_BAD:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; Start card SD_DEV (SD_INIT), forgetting the cache.  OUT: C = 0; or C = 1, .A = error
SD_START:
            stz         SD_CVALID
            jsr         HFS_FORGET                          ; (It may be a different card now)
            jmp         SD_INIT

; The rest of the name is in the data area: "/N/data" or "/N/ctl" (N = 0-7)
SD_OPEN:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
            lda         (ZP_IO_REQ),Y
            cmp         #'/'
            bne         @not_found
            iny
            lda         (ZP_IO_REQ),Y
            sec
            sbc         #'0'
            cmp         #SD_MAX_CARDS
            bcs         @not_found
            sta         SD_DEV
            iny
            lda         (ZP_IO_REQ),Y
            cmp         #'/'
            bne         @not_found
            iny
            sty         SD_TMP                              ; (Where the file's name starts)
            ldx         #SD_S_DATA - SD_NAMES
            jsr         SD_MATCH
            bcc         @data
            ldy         SD_TMP
            ldx         #SD_S_CTL - SD_NAMES
            jsr         SD_MATCH
            bcs         @not_found
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #SD_FID_CTL
            ora         SD_DEV
            clc
            rts

@data:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            bne         :+                                  ; (Started already)
            jsr         SD_START
            bcs         @done
:
            lda         SD_DEV                              ; (SD_FID_DATA | the card)
            clc

@done:
            rts

@not_found:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

; Does the name at (ZP_IO_REQ),Y end with the name at SD_NAMES,X?  OUT: C = 0 yes.  Modifies: .A, .X, .Y
SD_MATCH:
            lda         SD_NAMES,X
            cmp         (ZP_IO_REQ),Y
            bne         @no
            inx
            iny
            ora         #0
            bne         SD_MATCH                            ; (Both ended: a match)
            clc
            rts

@no:
            sec
            rts

SD_NAMES:
SD_S_DATA:  .byte   "data", 0
SD_S_CTL:   .byte   "ctl", 0

; Read the ctl file: make its text in the data area, then hand over what's after the fd's offset (up to
; the count)
SD_CTL_READ:
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            bne         :+
            jsr         SD_START                            ; (Not started yet: try.  C = 1: "none")
:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            stz         SD_N                                ; The text's length
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            bne         @card
            ldx         #SD_S_NONE - SD_TEXTS
            jsr         SD_PUT_TEXT
            bra         @made

@card:
            ldx         #SD_S_SDHC - SD_TEXTS
            cmp         #SD_STATE_SDHC
            beq         :+
            ldx         #SD_S_SDSC - SD_TEXTS
:
            jsr         SD_PUT_TEXT
            jsr         SD_CARD_SIZE                        ; The size in MB: blocks >> 11
            ldx         #11
:
            lsr         SD_LBA + 3
            ror         SD_LBA + 2
            ror         SD_LBA + 1
            ror         SD_LBA
            dex
            bne         :-
            jsr         SD_PUT_DEC
            ldx         #SD_S_MB - SD_TEXTS
            jsr         SD_PUT_TEXT
            jsr         SD_CARD_SIZE                        ; And in blocks
            jsr         SD_PUT_DEC
            ldx         #SD_S_BLOCKS - SD_TEXTS
            jsr         SD_PUT_TEXT
            jsr         HFS_CTL_LINES                       ; (HydraFS's, if there's one on it)

@made:                                                      ; The text is SD_N bytes
            dec         ZP_IO_REQ + 1
            ldy         #IO_BLK_OFS + 3                     ; Past the end: nothing more (end of file)
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @eof
            dey
            lda         (ZP_IO_REQ),Y
            cmp         SD_N
            bcs         @eof
            tax                                             ; .X = the offset
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         SD_LEFT                             ; Bytes wanted (1-256; 256 = 0)
            inc         ZP_IO_REQ + 1
            ldy         #0                                  ; Move the text after the offset down

@move:
            phy
            txa
            tay
            lda         (ZP_IO_REQ),Y
            ply
            sta         (ZP_IO_REQ),Y
            iny
            cpy         SD_LEFT                             ; (256: 0, never reached: the text is shorter)
            beq         @moved
            inx
            cpx         SD_N
            bne         @move

@moved:
            dec         ZP_IO_REQ + 1
            tya                                             ; The count
            bra         @count

@eof:
            lda         #0

@count:
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            jmp         SD_OK

SD_TEXTS:
SD_S_NONE:  .byte   "none", ASCII_CR, ASCII_LF, 0
SD_S_SDHC:  .byte   "sdhc ", 0
SD_S_SDSC:  .byte   "sdsc ", 0
SD_S_MB:    .byte   " MB ", 0
SD_S_BLOCKS: .byte  " blocks", ASCII_CR, ASCII_LF, 0

; SD_LBA = card SD_DEV's size in blocks.  Modifies: .A, .X
SD_CARD_SIZE:
            lda         SD_DEV
            asl
            asl
            tax
            lda         SD_CARD_BLOCKS,X
            sta         SD_LBA
            lda         SD_CARD_BLOCKS + 1,X
            sta         SD_LBA + 1
            lda         SD_CARD_BLOCKS + 2,X
            sta         SD_LBA + 2
            lda         SD_CARD_BLOCKS + 3,X
            sta         SD_LBA + 3
            rts

; Add the text at SD_TEXTS,X to the ctl file's text.  Modifies: .A, .X, .Y
SD_PUT_TEXT:
            lda         SD_TEXTS,X
            beq         @done
            jsr         SD_PUT
            inx
            bra         SD_PUT_TEXT

@done:
            rts

; Add SD_LBA (32 bits) in decimal to the ctl file's text.  SD_LBA ends as 0.  Modifies: .A, .X, .Y
SD_PUT_DEC:
            ldx         #0                                  ; Digits (on the stack, the last one first)

@digit:                                                     ; SD_LBA /= 10: .A = the remainder
            lda         #0
            ldy         #32

@bit:
            asl         SD_LBA
            rol         SD_LBA + 1
            rol         SD_LBA + 2
            rol         SD_LBA + 3
            rol
            cmp         #10
            bcc         :+
            sbc         #10                                 ; (C = 1)
            inc         SD_LBA
:
            dey
            bne         @bit
            pha
            inx
            lda         SD_LBA
            ora         SD_LBA + 1
            ora         SD_LBA + 2
            ora         SD_LBA + 3
            bne         @digit

@put:
            pla
            ora         #'0'
            jsr         SD_PUT
            dex
            bne         @put
            rts

; Add .A to the ctl file's text (in the data area: ZP_IO_REQ, moved up to it).  Modifies: .Y
SD_PUT:
            ldy         SD_N
            sta         (ZP_IO_REQ),Y
            inc         SD_N
            rts

; Write to the ctl file: a command, "init" (a space, CR or LF may follow it).  The whole write is taken.
SD_CTL_WRITE:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            bne         :+
            dec                                             ; (256 bytes: look at 255)
:
            sta         SD_N
            inc         ZP_IO_REQ + 1                       ; The data area
            ldx         #0                                  ; The command: offset in SD_CMDS
            stz         SD_TMP                              ;   and number

@cmd:
            lda         SD_CMDS,X
            beq         @bad                                ; (The end of the table)
            ldy         #0

@char:
            lda         SD_CMDS,X
            beq         @word
            cpy         SD_N
            beq         @skip                               ; (The write is shorter)
            cmp         (ZP_IO_REQ),Y
            bne         @skip
            inx
            iny
            bra         @char

@word:                                                      ; It matches if the write ends here, or a
            cpy         SD_N                                ;   space, CR, LF or 0 comes next
            beq         @match
            lda         (ZP_IO_REQ),Y
            beq         @match
            cmp         #' '
            beq         @match
            cmp         #ASCII_CR
            beq         @match
            cmp         #ASCII_LF
            beq         @match

@skip:
            inx
            lda         SD_CMDS - 1,X
            bne         @skip
            inc         SD_TMP
            bra         @cmd

@bad:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            jmp         SD_BAD

@match:                                                     ; SD_TMP = the command (0: init)
            lda         SD_TMP
            bne         SD_CTL_FS
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of it taken)
            jmp         SD_RESTART

SD_CMDS:    .byte   "init", 0, "format", 0, "label", 0, "check", 0, 0

; "format [options] [label]" (SD_TMP = 1), "label <text>" (2) and "check [fix]" (3), for HydraFS: the text
; after the word (spaces before it skipped, up to 47 characters, to the end of the line) -> HFS_STAT,
; zero-padded to 48.  (A label is cut to 31 characters where it's used: HFS_LABEL_CUT.)
; IN: .Y = where the word ended, in the data area (mapped); SD_N = the write's length
SD_CTL_FS:
            ldx         #IO_STAT_SIZE
:
            stz         HFS_STAT - 1,X
            dex
            bne         :-

@space:
            cpy         SD_N
            beq         @copied
            lda         (ZP_IO_REQ),Y
            cmp         #' '
            bne         @text
            iny
            bra         @space

@text:
            cpy         SD_N
            beq         @copied
            lda         (ZP_IO_REQ),Y
            beq         @copied
            cmp         #ASCII_CR
            beq         @copied
            cmp         #ASCII_LF
            beq         @copied
            cpx         #IO_STAT_SIZE - 1
            beq         @copied                             ; (Any more is cut off)
            sta         HFS_STAT,X
            inx
            iny
            bra         @text

@copied:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of it taken)
            lda         SD_TMP
            cmp         #1
            bne         :+
            jmp         HFS_FORMAT
:
            cmp         #2
            bne         :+
            jmp         HFS_LABEL
:
            jmp         HFS_CHECK

; A read or write of a data file: .A = H9_READ / H9_WRITE.  Up to 256 bytes at the offset, a block (or two)
; at a time.
SD_RW:
            sta         SD_OP
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            bne         :+
            lda         #ERR_IO_NOT_READY
            sec
            rts
:
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

@piece:                                                     ; The part of this block the request wants
            lda         SD_LEFT
            ora         SD_LEFT + 1
            bne         :+
            jmp         @finished
:
            jsr         SD_POS_TO_LBA                       ; SD_LBA = SD_POS >> 9
            jsr         SD_CACHE_LOAD                       ; The block, in the cache
            bcc         :+
            jmp         @error
:
            lda         SD_POS + 1                          ; SD_N = min(512 - (SD_POS & 511), SD_LEFT)
            and         #1
            eor         #1                                  ; (Bytes to the block's end, high byte)
            tax
            lda         SD_POS
            eor         #$FF
            clc
            adc         #1                                  ; (Low byte: 0 - SD_POS)
            sta         SD_N
            bne         :+
            inx                                             ; (SD_POS & 511 = 0: 512 bytes)
:
            stx         SD_N + 1
            lda         SD_N + 1                            ; SD_N > SD_LEFT?  Take SD_LEFT
            cmp         SD_LEFT + 1
            bcc         @copy
            bne         @left
            lda         SD_N
            cmp         SD_LEFT
            bcc         @copy
            beq         @copy

@left:
            lda         SD_LEFT
            sta         SD_N
            lda         SD_LEFT + 1
            sta         SD_N + 1

@copy:                                                      ; SD_N bytes (1-256): the cache <-> the data area
            lda         SD_POS                              ; SD_SRC = the cache + (SD_POS & 511)
            clc
            adc         SD_CACHE
            sta         SD_SRC
            lda         SD_POS + 1
            and         #1
            adc         SD_CACHE + 1
            sta         SD_SRC + 1
            lda         SD_DONE                             ; SD_DST = the data area + SD_DONE
            sta         SD_DST
            lda         ZP_IO_REQ + 1
            inc                                             ; (IO_BLK_DATA = $100)
            sta         SD_DST + 1
            ldy         #0
            lda         SD_OP
            cmp         #H9_WRITE
            beq         @into_cache

@out:
            lda         (SD_SRC),Y                          ; Read: the cache -> the data area
            sta         (SD_DST),Y
            iny
            cpy         SD_N                                ; (SD_N = 256: 0, so .Y wraps round to it)
            bne         @out
            bra         @advance

@into_cache:
            lda         (SD_DST),Y                          ; Write: the data area -> the cache
            sta         (SD_SRC),Y
            iny
            cpy         SD_N
            bne         @into_cache
            lda         SD_CACHE                            ; ... -> the card
            sta         SD_BUF
            lda         SD_CACHE + 1
            sta         SD_BUF + 1
            jsr         SD_WRITE_BLOCK
            bcc         @advance
            stz         SD_CVALID                           ; (The cache and the card may differ now)
            bra         @error

@advance:
            lda         SD_POS                              ; SD_POS += SD_N
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
            lda         SD_LEFT                             ; SD_LEFT -= SD_N
            sec
            sbc         SD_N
            sta         SD_LEFT
            lda         SD_LEFT + 1
            sbc         SD_N + 1
            sta         SD_LEFT + 1
            lda         SD_DONE                             ; SD_DONE += SD_N (at most 256: then 0)
            clc
            adc         SD_N
            sta         SD_DONE
            jmp         @piece

@finished:                                                  ; The whole count: it stays
            jsr         IO_SRV_UNMAP
            lda         #0
            clc
            rts

@error:
            jsr         IO_SRV_UNMAP
            sec
            rts

; SD_LBA = SD_POS >> 9 (32 bits).  Modifies: .A
SD_POS_TO_LBA:
            lda         SD_POS + 3
            lsr
            sta         SD_LBA + 2
            lda         SD_POS + 2
            ror
            sta         SD_LBA + 1
            lda         SD_POS + 1
            ror
            sta         SD_LBA
            stz         SD_LBA + 3
            rts

; Make sure block SD_LBA of card SD_DEV is in the cache.  OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
SD_CACHE_LOAD:
            lda         SD_CVALID
            beq         @read
            lda         SD_DEV
            cmp         SD_CCARD
            bne         @read
            ldx         #3

@same:
            lda         SD_LBA,X
            cmp         SD_CBLOCK,X
            bne         @read
            dex
            bpl         @same
            clc
            rts

@read:
            stz         SD_CVALID
            lda         SD_CACHE
            sta         SD_BUF
            lda         SD_CACHE + 1
            sta         SD_BUF + 1
            jsr         SD_READ_BLOCK
            bcs         @done
            ldx         #3

@copy:
            lda         SD_LBA,X
            sta         SD_CBLOCK,X
            dex
            bpl         @copy
            lda         SD_DEV
            sta         SD_CCARD
            inc         SD_CVALID
            clc

@done:
            rts
