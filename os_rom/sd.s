.debuginfo

; ****************************************************************************
; SD card, SPI mode (BIOS ROM page 3, the storage page; included inside `.scope PAGE3`, see all.s).  Block
; level: SD_INIT, SD_READ_BLOCK and SD_WRITE_BLOCK move 512-byte blocks (sectors) by number, for the
; /dev/sd server and (later) the FAT32 server.  Runs in the storage task (its ZP: SD_* in zero.s).
;   SDHC / SDXC cards take block numbers; SDSC (v1, and v2 standard capacity) cards take byte addresses
;   (block * 512), set up by SD_INIT.
;   Errors: C = 1 with .A = ERR_IO_DEVICE (no card, or it didn't answer), ERR_IO_NOT_READY (SD_INIT not
;   done) or ERR_IO_MEDIA (the card refused the command or the data); SD_R1 = the card's last answer.

.segment "STORAGE_P3"

SD_CMD0             = 0         ; GO_IDLE_STATE
SD_CMD8             = 8         ; SEND_IF_COND
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

; Start the card: SDHC/SDXC or SDSC.  OUT: C = 0 (SD_STATE = SD_STATE_SDHC or SD_STATE_SDSC); or C = 1,
; .A = error.  Modifies: .A, .X, .Y
SD_INIT:
            stz         SD_STATE
            jsr         SPI_INIT
            lda         #10                                 ; 80 clocks, nothing selected
            jsr         SPI_IDLE_CLOCKS
            lda         #SD_SPI_DEVICE
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
            sta         SD_STATE
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
            lda         SD_STATE
            beq         SD_NOT_READY
            jsr         SD_BLOCK_ARG
            lda         #SD_SPI_DEVICE
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
            lda         SD_STATE
            beq         SD_NOT_READY
            jsr         SD_BLOCK_ARG
            lda         #SD_SPI_DEVICE
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

; SD_ARG = 0.  Preserves .A, .X, .Y
SD_ARG_ZERO:
            stz         SD_ARG
            stz         SD_ARG + 1
            stz         SD_ARG + 2
            stz         SD_ARG + 3
            rts

; SD_ARG = block SD_LBA's address: the block number (SDHC), or * 512 (SDSC).  Modifies: .A
SD_BLOCK_ARG:
            lda         SD_STATE
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
