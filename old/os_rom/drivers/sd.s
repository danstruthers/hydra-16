.debuginfo

; ****************************************************************************
; SD card, SPI mode (BIOS ROM page 3, the storage page; included inside `.scope PAGE3`, see all.s).  Block
; level: SD_INIT, SD_READ_BLOCK and SD_WRITE_BLOCK move 512-byte blocks (sectors) by number, for the
; /dev/sd server and (later) the HydraFS server.  Runs in the storage task (its ZP: SD_* in zero.s).
;   The card is SPI device SD_DEV (0-7); each has its state in SD_CARD_STATE and its size in blocks in
;   SD_CARD_BLOCKS (the storage task's RAM, set by SD_INIT).
;   SDHC / SDXC cards take block numbers; SDSC (v1, and v2 standard capacity) cards take byte addresses
;   (block * 512), set up by SD_INIT.
;   Errors: C = 1 with .A = ERR_IO_DEVICE (no card, or it didn't answer), ERR_IO_NOT_READY (SD_INIT not
;   done) or ERR_IO_MEDIA (the card refused the command or the data); SD_R1 = the card's last answer.

.segment "STORAGE_P3"

SD_CMD0             = 0         ; GO_IDLE_STATE
SD_CMD8             = 8         ; SEND_IF_COND
SD_CMD9             = 9         ; SEND_CSD (the card's size)
SD_CMD16            = 16        ; SET_BLOCKLEN
SD_CMD17            = 17        ; READ_SINGLE_BLOCK
SD_CMD24            = 24        ; WRITE_BLOCK
SD_CMD55            = 55        ; APP_CMD (the next one is an ACMD)
SD_CMD58            = 58        ; READ_OCR
SD_ACMD41           = 41        ; SD_SEND_OP_COND
SD_R1_IDLE          = $01
SD_TOKEN_DATA       = $FE       ; Start of a data block (both ways)
SD_INIT_TRIES       = 1000      ; ACMD41 tries (about a second at 3.58 MHz)
SD_TOKEN_TRIES      = 4000      ; Bytes to wait for a data token or the end of busy (about 0.3 s / try)

; Start card SD_DEV: SDHC/SDXC or SDSC, and read its size.  OUT: C = 0 (its SD_CARD_STATE = SD_STATE_SDHC
; or SD_STATE_SDSC, and SD_CARD_BLOCKS its size); or C = 1, .A = error.  Modifies: .A, .X, .Y
SD_INIT:
            ldx         SD_DEV
            cpx         #SD_MAX_CARDS                       ; Not a card: the ROM disk, a RAM disk
            bcc         :+
            jmp         SD_DISK_INIT
:
            lda         SPI_REFS,X                          ; (Open as /dev/spi/N: not a card's now)
            beq         :+
            lda         #ERR_IO_BUSY
            sec
            rts
:
            stz         SD_CARD_STATE,X
            jsr         SPI_INIT
            lda         #10                                 ; 80 clocks, nothing selected
            jsr         SPI_IDLE_CLOCKS
            lda         SD_DEV
            jsr         SPI_SELECT
            ldy         #10                                 ; CMD0: to SPI mode, idle

@cmd0:
            jsr         SD_ARG_ZERO
            lda         #SD_CMD0
            jsr         SD_CMD
            cmp         #SD_R1_IDLE
            beq         @cmd8
            dey
            bne         @cmd0
            jmp         SD_NO_CARD

@cmd8:                                                      ; CMD8: a v2 card echoes the pattern
            jsr         SD_ARG_ZERO
            lda         #$01                                ; (2.7-3.6 V)
            sta         SD_ARG + 2
            lda         #$AA                                ; (The pattern)
            sta         SD_ARG + 3
            lda         #SD_CMD8
            jsr         SD_CMD
            ldx         #0                                  ; HCS = 0 for a v1 card (illegal command)
            cmp         #SD_R1_IDLE
            bne         @acmd41
            jsr         SPI_RECV                            ; The R7 answer: 3 bytes, then the pattern
            jsr         SPI_RECV
            jsr         SPI_RECV
            jsr         SPI_RECV
            cmp         #$AA
            beq         :+
            jmp         SD_NO_CARD
:
            ldx         #$40                                ; HCS: we take high-capacity cards

@acmd41:
            stx         SD_TMP                              ; (HCS)
            lda         #<SD_INIT_TRIES
            sta         SD_COUNT
            lda         #>SD_INIT_TRIES
            sta         SD_COUNT + 1

@acmd41_loop:                                               ; ACMD41 until the card leaves idle
            jsr         SD_ARG_ZERO
            lda         #SD_CMD55
            jsr         SD_CMD
            jsr         SD_ARG_ZERO
            lda         SD_TMP
            sta         SD_ARG
            lda         #SD_ACMD41
            jsr         SD_CMD
            beq         @ready                              ; (R1 = 0)
            bmi         SD_NO_CARD                          ; (No answer)
            jsr         SD_COUNT_DOWN
            bne         @acmd41_loop
            bra         SD_NO_CARD

@ready:
            lda         #SD_STATE_SDSC
            ldx         SD_TMP
            beq         @sdsc                               ; (A v1 card is SDSC)
            jsr         SD_ARG_ZERO                         ; CMD58: CCS in the OCR says SDHC/SDXC
            lda         #SD_CMD58
            jsr         SD_CMD
            bne         SD_REFUSED
            jsr         SPI_RECV                            ; OCR bits 31-24
            tax
            jsr         SPI_RECV
            jsr         SPI_RECV
            jsr         SPI_RECV
            lda         #SD_STATE_SDHC
            cpx         #$C0                                ; Powered up (bit 31) and CCS (bit 30)?
            bcs         @done

@sdsc:                                                      ; Standard capacity: 512-byte blocks
            jsr         SD_ARG_ZERO
            lda         #>512
            sta         SD_ARG + 2
            lda         #SD_CMD16
            jsr         SD_CMD
            bne         SD_REFUSED
            lda         #SD_STATE_SDSC

@done:
            sta         SD_TMP                              ; (The state)
            jsr         SD_READ_SIZE
            bcs         SD_REFUSED
            lda         SD_TMP
            ldx         SD_DEV
            sta         SD_CARD_STATE,X
            jsr         SD_END
            clc
            rts

SD_NO_CARD:
            jsr         SD_END
            lda         #ERR_IO_DEVICE
            sec
            rts

SD_REFUSED:
            jsr         SD_END
            lda         #ERR_IO_MEDIA
            sec
            rts

SD_NOT_READY:
            lda         #ERR_IO_NOT_READY
            sec
            rts
; Read block SD_LBA (32 bits) into the 512 bytes at SD_BUF.  OUT: C = 0; or C = 1, .A = error
; Modifies: .A, .X, .Y
SD_READ_BLOCK:
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            beq         SD_NOT_READY
            cmp         #SD_STATE_ROM
            bne         :+
            jmp         SD_ROM_READ
:
            bcc         :+                                  ; (Past the ROM's: a RAM disk)
            jmp         SD_RAM_READ
:
            jsr         SD_BLOCK_ARG
            lda         SD_DEV
            jsr         SPI_SELECT
            lda         #SD_CMD17
            jsr         SD_CMD
            bne         SD_REFUSED
            jsr         SD_TOKEN_WAIT                       ; The data token
            cmp         #SD_TOKEN_DATA
            bne         SD_REFUSED
            ldy         #0                                  ; 512 bytes: 2 pages from SD_BUF

@first:
            jsr         SPI_RECV
            sta         (SD_BUF),Y
            iny
            bne         @first
            inc         SD_BUF + 1

@second:
            jsr         SPI_RECV
            sta         (SD_BUF),Y
            iny
            bne         @second
            dec         SD_BUF + 1
            jsr         SPI_RECV                            ; (The CRC: not checked)
            jsr         SPI_RECV
            jsr         SD_END
            clc
            rts

; Write the 512 bytes at SD_BUF to block SD_LBA.  OUT: C = 0; or C = 1, .A = error
; Modifies: .A, .X, .Y
SD_WRITE_BLOCK:
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            beq         SD_NOT_READY
            cmp         #SD_STATE_ROM
            bne         :+
            jmp         SD_ROM_WRITE
:
            bcc         :+                                  ; (Past the ROM's: a RAM disk)
            jmp         SD_RAM_WRITE
:
            jsr         SD_BLOCK_ARG
            lda         SD_DEV
            jsr         SPI_SELECT
            lda         #SD_CMD24
            jsr         SD_CMD
            bne         SD_REFUSED
            jsr         SPI_RECV                            ; (A byte's gap)
            lda         #SD_TOKEN_DATA
            jsr         SPI_XFER
            ldy         #0

@first:
            lda         (SD_BUF),Y
            jsr         SPI_XFER
            iny
            bne         @first
            inc         SD_BUF + 1

@second:
            lda         (SD_BUF),Y
            jsr         SPI_XFER
            iny
            bne         @second
            dec         SD_BUF + 1
            lda         #$FF                                ; (The CRC: not checked in SPI mode)
            jsr         SPI_XFER
            lda         #$FF
            jsr         SPI_XFER
            jsr         SPI_RECV                            ; The data response: xxx0 0101 = accepted
            sta         SD_R1
            and         #$1F
            cmp         #$05
            beq         :+
            jmp         SD_REFUSED
:
            jsr         SD_BUSY_WAIT                        ; While it writes
            bcc         :+
            jmp         SD_NO_CARD
:
            jsr         SD_END
            clc
            rts

; ****************************************************************************
; Helpers

; Send command .A with argument SD_ARG (MSB first), and get its R1 answer.
; OUT: .A = R1 (N = 1: no answer), Z from it.  Modifies: .X
SD_CMD:
            pha
            jsr         SPI_RECV                            ; (A byte before each command)
            pla
            pha
            ora         #$40
            jsr         SPI_XFER
            ldx         #0

@arg:
            lda         SD_ARG,X
            jsr         SPI_XFER
            inx
            cpx         #4
            bne         @arg
            pla                                             ; The CRC: only CMD0 and CMD8 need a real one
            ldx         #$95                                ;   (CRC checking is off in SPI mode)
            cmp         #SD_CMD0
            beq         @crc
            ldx         #$87
            cmp         #SD_CMD8
            beq         @crc
            ldx         #$01                                ; (The end bit)

@crc:
            txa
            jsr         SPI_XFER
            ldx         #10                                 ; The answer: within 8 bytes

@answer:
            jsr         SPI_RECV
            bpl         @got
            dex
            bne         @answer

@got:
            sta         SD_R1
            ora         #0
            rts

; Read card SD_DEV's CSD register (into SD_CSD) and its size in blocks (into its SD_CARD_BLOCKS).  The
; card must be selected.  CSD v2 (SDHC, SDXC): (C_SIZE + 1) * 1024 blocks.  CSD v1 (SDSC):
; (C_SIZE + 1) << (C_SIZE_MULT + 2) bytes of READ_BL_LEN (as a shift), in 512-byte blocks.
; OUT: C = 0; or C = 1 (no answer, or a CSD version we don't know).  Modifies: .A, .X, SD_LBA, SD_ARG
SD_READ_SIZE:
            jsr         SD_ARG_ZERO
            lda         #SD_CMD9
            jsr         SD_CMD
            bne         @fail
            jsr         SD_TOKEN_WAIT                       ; The data token
            cmp         #SD_TOKEN_DATA
            beq         @read

@fail:
            sec
            rts

@read:
            ldx         #0

@csd:                                                       ; 16 bytes, MSB (bit 127) first
            jsr         SPI_RECV
            sta         SD_CSD,X
            inx
            cpx         #16
            bne         @csd
            jsr         SPI_RECV                            ; (The CRC: not checked)
            jsr         SPI_RECV
            stz         SD_LBA + 3
            lda         SD_CSD                              ; CSD_STRUCTURE: bits 127-126
            and         #$C0
            beq         @v1
            cmp         #$40
            bne         @fail

            lda         SD_CSD + 9                          ; v2: C_SIZE = bits 69-48
            sta         SD_LBA
            lda         SD_CSD + 8
            sta         SD_LBA + 1
            lda         SD_CSD + 7
            and         #$3F
            sta         SD_LBA + 2
            ldx         #10                                 ; (* 1024)
            bra         @plus_one

@v1:                                                        ; v1: C_SIZE = bits 73-62
            lda         SD_CSD + 8
            sta         SD_LBA
            lda         SD_CSD + 7
            sta         SD_LBA + 1
            lda         SD_CSD + 6
            and         #$03
            sta         SD_LBA + 2
            ldx         #6

@down:
            lsr         SD_LBA + 2
            ror         SD_LBA + 1
            ror         SD_LBA
            dex
            bne         @down
            lda         SD_CSD + 10                         ; C_SIZE_MULT = bits 49-47
            asl                                             ; (C = bit 47)
            lda         SD_CSD + 9
            and         #$03
            rol                                             ; (C = 0)
            sta         SD_ARG                              ; (Free: the command is done)
            lda         SD_CSD + 5                          ; READ_BL_LEN = bits 83-80
            and         #$0F
            adc         SD_ARG
            sec
            sbc         #7                                  ; Shift: C_SIZE_MULT + 2 + READ_BL_LEN - 9
            tax

@plus_one:                                                  ; SD_LBA = (C_SIZE + 1) << .X
            inc         SD_LBA
            bne         @up
            inc         SD_LBA + 1
            bne         @up
            inc         SD_LBA + 2

@up:
            asl         SD_LBA
            rol         SD_LBA + 1
            rol         SD_LBA + 2
            rol         SD_LBA + 3
            dex
            bne         @up
            lda         SD_DEV                              ; Its SD_CARD_BLOCKS
            asl
            asl
            tax
            lda         SD_LBA
            sta         SD_CARD_BLOCKS,X
            lda         SD_LBA + 1
            sta         SD_CARD_BLOCKS + 1,X
            lda         SD_LBA + 2
            sta         SD_CARD_BLOCKS + 2,X
            lda         SD_LBA + 3
            sta         SD_CARD_BLOCKS + 3,X
            clc
            rts

; SD_ARG = 0.  Preserves .A, .X, .Y
SD_ARG_ZERO:
            stz         SD_ARG
            stz         SD_ARG + 1
            stz         SD_ARG + 2
            stz         SD_ARG + 3
            rts

; SD_ARG = block SD_LBA's address: the block number (SDHC), or * 512 (SDSC).  Modifies: .A, .X
SD_BLOCK_ARG:
            ldx         SD_DEV
            lda         SD_CARD_STATE,X
            cmp         #SD_STATE_SDHC
            bne         @bytes
            lda         SD_LBA + 3                          ; (MSB first)
            sta         SD_ARG
            lda         SD_LBA + 2
            sta         SD_ARG + 1
            lda         SD_LBA + 1
            sta         SD_ARG + 2
            lda         SD_LBA
            sta         SD_ARG + 3
            rts

@bytes:                                                     ; SD_LBA << 9
            stz         SD_ARG + 3
            lda         SD_LBA
            asl
            sta         SD_ARG + 2
            lda         SD_LBA + 1
            rol
            sta         SD_ARG + 1
            lda         SD_LBA + 2
            rol
            sta         SD_ARG
            rts

; Wait for a byte other than $FF (a data token).  OUT: .A = it ($FF: gave up).  Modifies: .X
SD_TOKEN_WAIT:
            lda         #<SD_TOKEN_TRIES
            sta         SD_COUNT
            lda         #>SD_TOKEN_TRIES
            sta         SD_COUNT + 1

@wait:
            jsr         SPI_RECV
            cmp         #$FF
            bne         @got
            jsr         SD_COUNT_DOWN
            bne         @wait
            lda         #$FF

@got:
            sta         SD_R1
            rts

; Wait while the card is busy (it sends 0s).  OUT: C = 0; or C = 1 if it's still busy.  Modifies: .A
SD_BUSY_WAIT:
            lda         #<SD_TOKEN_TRIES
            sta         SD_COUNT
            lda         #>SD_TOKEN_TRIES
            sta         SD_COUNT + 1

@wait:
            jsr         SPI_RECV
            cmp         #$FF
            beq         @done                               ; $FF: not busy any more
            jsr         SD_COUNT_DOWN
            bne         @wait
            sec
            rts

@done:
            clc
            rts

; SD_COUNT - 1; Z = 1 when it reaches 0.  Modifies: .A
SD_COUNT_DOWN:
            lda         SD_COUNT
            bne         :+
            dec         SD_COUNT + 1
:
            dec         SD_COUNT
            lda         SD_COUNT
            ora         SD_COUNT + 1
            rts

; Deselect, and one more byte of clocks (the card lets go of MISO).  Preserves .A
SD_END:
            pha
            jsr         SPI_DESELECT
            jsr         SPI_RECV
            pla
            rts

; ****************************************************************************
; The ROM disk (disk DISK_ROM): the paged ROM as a block device, read only (docs/plans/DISKS.md;
; sim/tools/mkromdisk.js makes it).  Block n is bank n / 32, at $A000 + (n % 32) * 512, as the CPU sees it: a
; block is always inside one bank (and one of its 8K halves, which the board swaps), so a read selects its bank
; and copies 512 bytes from one place, and nothing assumes the next bank follows.
.assert     $4000 .mod HFS_BLOCK = 0 .and $2000 .mod HFS_BLOCK = 0, error, "A ROM disk block must be inside one bank, and one half of it"
.assert     DISK_ROM_BLOCKS = 256 * ($4000 / HFS_BLOCK), error, "The ROM disk: the whole paged ROM, 256 banks"

; Start it: nothing to start; its state and size.  OUT: C = 0.  Modifies: .A, .X
SD_ROM_INIT:
            lda         #SD_STATE_ROM
            sta         SD_CARD_STATE,X
            txa
            asl
            asl
            tax
            lda         #<DISK_ROM_BLOCKS
            sta         SD_CARD_BLOCKS,X
            lda         #>DISK_ROM_BLOCKS
            sta         SD_CARD_BLOCKS + 1,X
            stz         SD_CARD_BLOCKS + 2,X
            stz         SD_CARD_BLOCKS + 3,X
            clc
            rts

; Read block SD_LBA of the ROM disk into the 512 bytes at SD_BUF: the storage task's own paged ROM bank ($01)
; set to the block's for the copy, and put back.  Past the paged ROM: ERR_IO_MEDIA.  SD_ARG is the pointer (an SD
; card's command argument, not in use here).  OUT: C = 0; or C = 1, .A = error.  Modifies: .A, .X, .Y
SD_ROM_READ:
            lda         SD_LBA + 3
            ora         SD_LBA + 2
            bne         @past
            lda         SD_LBA + 1
            cmp         #>DISK_ROM_BLOCKS
            bcs         @past
            asl                                             ; The bank: SD_LBA / 32 (13 bits: 8 of them)
            asl
            asl
            sta         SD_ARG + 2
            lda         SD_LBA
            lsr
            lsr
            lsr
            lsr
            lsr
            ora         SD_ARG + 2
            tax
            lda         SD_LBA                              ; Where in it: $A000 + (SD_LBA % 32) * 512
            and         #$1F
            asl
            adc         #>PAGED_ROM_BASE                    ; (C = 0: the asl's bit 7 was 0)
            sta         SD_ARG + 1
            stz         SD_ARG
            lda         ROM_BANK_REG                        ; (Reads give this task's last write)
            pha
            stx         ROM_BANK_REG
            ldy         #0

@first:
            _M_COPY_PAGE SD_ARG, SD_BUF
            inc         SD_ARG + 1
            inc         SD_BUF + 1

@second:
            _M_COPY_PAGE SD_ARG, SD_BUF
            dec         SD_BUF + 1
            pla
            sta         ROM_BANK_REG
            clc
            rts

@past:
            lda         #ERR_IO_MEDIA
            sec
            rts

; A write to the ROM disk: refused
SD_ROM_WRITE:
            lda         #ERR_IO_MODE
            sec
            rts

; A disk that isn't a card, at SD_INIT (.X = SD_DEV): the ROM disk starts; a RAM disk is started by "start" on
; its ctl file (SD_RAM_START), so here it's ready or it isn't.  OUT: C = 0; or C = 1, .A = ERR_IO_DEVICE
SD_DISK_INIT:
            cpx         #DISK_ROM
            bne         @ram
            jmp         SD_ROM_INIT

@ram:
            lda         SD_CARD_STATE,X
            beq         :+
            clc
            rts
:
            lda         #ERR_IO_DEVICE
            sec
            rts

; ****************************************************************************
; The RAM disks (DISK_RAM, DISK_SRAM): 8K banks as a block device (docs/plans/DISKS.md; started and stopped in
; sd_srv.s).  Block n is bank n / 16 of the disk's run, from RAMD_FIRST, at $8000 + (n % 16) * 512: the RAM
; disk's banks are the storage task's own (its $00), the shared one's shared bank IDs (U and $00).  A block is
; inside one bank, so a read or write maps one bank, copies 512 bytes, and puts the bank (and U) back.
.assert     $2000 .mod HFS_BLOCK = 0, error, "A RAM disk block must be inside one bank"

; Read block SD_LBA of RAM disk SD_DEV into the 512 bytes at SD_BUF.  OUT: C = 0; or C = 1, .A = ERR_IO_MEDIA
; (past its end).  Modifies: .A, .X, .Y
SD_RAM_READ:
            jsr         SD_RAM_MAP
            bcs         @done
            ldy         #0

@first:
            _M_COPY_PAGE SD_ARG, SD_BUF
            inc         SD_ARG + 1
            inc         SD_BUF + 1

@second:
            _M_COPY_PAGE SD_ARG, SD_BUF
            dec         SD_BUF + 1
            jmp         SD_RAM_UNMAP

@done:
            rts

; Write the 512 bytes at SD_BUF to block SD_LBA of RAM disk SD_DEV.  OUT: C = 0; or C = 1, .A = ERR_IO_MEDIA.
; Modifies: .A, .X, .Y
SD_RAM_WRITE:
            jsr         SD_RAM_MAP
            bcs         @done
            ldy         #0

@first:
            _M_COPY_PAGE SD_BUF, SD_ARG
            inc         SD_ARG + 1
            inc         SD_BUF + 1

@second:
            _M_COPY_PAGE SD_BUF, SD_ARG
            dec         SD_BUF + 1
            jmp         SD_RAM_UNMAP

@done:
            rts

; Map block SD_LBA of RAM disk SD_DEV at $8000-$9FFF: SD_ARG -> it, and the storage task's RAM bank and U as they
; were in SD_ARG + 2 and SD_ARG + 3, for SD_RAM_UNMAP.  (The block cache, SD_BUF, is in task RAM, below $8000.)
; OUT: C = 0; or C = 1, .A = ERR_IO_MEDIA (past the disk's end: nothing mapped).  Modifies: .A, .X
SD_RAM_MAP:
            lda         SD_LBA + 3
            ora         SD_LBA + 2
            bne         @past
            lda         SD_DEV                              ; Inside the disk?
            asl
            asl
            tax
            lda         SD_LBA
            cmp         SD_CARD_BLOCKS,X
            lda         SD_LBA + 1
            sbc         SD_CARD_BLOCKS + 1,X
            bcs         @past
            lda         SD_LBA                              ; Where in its bank: $8000 + (SD_LBA % 16) * 512
            and         #$0F
            asl
            ora         #>::PAGED_RAM_BASE
            sta         SD_ARG + 1
            stz         SD_ARG
            lda         SD_LBA + 1                          ; The bank: the first + SD_LBA / 16 (the disk is
            asl                                             ;   256 banks at most: 8 bits)
            asl
            asl
            asl
            sta         SD_ARG + 2
            lda         SD_LBA
            lsr
            lsr
            lsr
            lsr
            ora         SD_ARG + 2
            ldx         SD_DEV
            clc
            adc         RAMD_FIRST,X
            tax
            lda         RAM_BANK_REG                        ; (Put back by SD_RAM_UNMAP)
            sta         SD_ARG + 2
            lda         U_REGISTER
            sta         SD_ARG + 3
            lda         SD_DEV
            cmp         #DISK_SRAM
            beq         @shared
            stx         RAM_BANK_REG                        ; The RAM disk: the storage task's own bank
            clc
            rts

@shared:                                                    ; The shared one: bank ID .X, U = ID >> 4 and the
            txa                                             ;   bank $F0 | (ID & $0F), as SH_SELECT_BANK maps it
            and         #$0F
            ora         #$F0
            sta         RAM_BANK_REG
            txa
            lsr
            lsr
            lsr
            lsr
            sta         U_REGISTER
            clc
            rts

@past:
            lda         #ERR_IO_MEDIA
            sec
            rts

; Put back what SD_RAM_MAP mapped.  OUT: C = 0
SD_RAM_UNMAP:
            lda         SD_ARG + 2
            sta         RAM_BANK_REG
            lda         SD_ARG + 3
            sta         U_REGISTER
            clc
            rts
