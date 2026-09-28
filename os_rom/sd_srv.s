.debuginfo

; ****************************************************************************
; The storage task's file server: /dev/sd, the SD card as one big file of bytes (BIOS ROM page 3, the
; storage page; included inside `.scope PAGE3`, see all.s).  The storage task (STORAGE_TASK_NUM) owns
; the SPI bus, so its requests run one at a time.  The FAT32 server will live here too, on the same
; block cache.
;   Open: starts the card (SD_INIT) if it isn't yet.
;   Read / write: at the fd's offset (IO_SEEK), through a one-block cache (SD_CACHE, 512 bytes from the
;         MMU); writes go through to the card at once.  The offset is 32 bits, so /dev/sd reaches the
;         first 4 GB of the card (the FAT32 server uses block numbers, for all of it).
;   Ctl: SD_CTL_INIT starts the card again (e.g. after changing it).
; Server ZP (the storage task's): SD_* (zero.s); ZP_IO_REQ (IO_SRV_MAP).

.segment "STORAGE_P3"

; The storage task's init (from STORAGE_INIT on page 0, in the task): the block cache, SPI idle.
; OUT: C = 0; or C = 1, .A = error
STORAGE_INIT3:
            stz         SD_STATE                            ; (The card starts at the first open)
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
            clc

@done:
            rts

; IN: .A = request, .X = client, .Y = fid
SD_SERVE:
            stx         SD_CLIENT
            cmp         #H9_READ
            beq         SD_RW
            cmp         #H9_WRITE
            beq         SD_RW
            cmp         #H9_OPEN
            beq         @open
            cmp         #H9_CTL
            beq         @ctl
            cmp         #H9_STAT
            beq         @bad

@ok:                                                        ; H9_CLUNK, H9_DUP
            lda         #0
            clc
            rts

@open:
            lda         SD_STATE
            bne         @ok                                 ; (Started already)

@init:
            stz         SD_CVALID
            jsr         SD_INIT
            bcc         @ok
            rts

@ctl:
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_CTL_CODE
            lda         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            cmp         #SD_CTL_INIT
            beq         @init

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; A read or write: .A = H9_READ / H9_WRITE.  Up to 256 bytes at the offset, a block (or two) at a time.
SD_RW:
            sta         SD_OP
            lda         SD_STATE
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

; Make sure block SD_LBA is in the cache.  OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
SD_CACHE_LOAD:
            lda         SD_CVALID
            beq         @read
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
            inc         SD_CVALID
            clc

@done:
            rts
