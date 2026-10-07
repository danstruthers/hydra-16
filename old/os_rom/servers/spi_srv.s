.debuginfo

; ****************************************************************************
; /dev/spi: the SPI devices as files, as Plan 9's (BIOS ROM page D; included inside `.scope PAGED`, see all.s).
; Served in the storage task, which owns the bus (spi.s, page 3: a request's bytes go through its SPI_XFER_N or
; SPI_RECV_N), so a transfer never meets an SD card's: the task serves one request at a time.
;   /dev/spi/N          N = 0-f: SPI device N (0-7 the board's headers, 8-f the slots' cards).
;                       Write: the bytes are sent with the device selected (one transaction a write request:
;                       up to 256 bytes), and the bytes it sends back meanwhile are kept.
;                       Read: the bytes kept from the last write; or, if there are none, as many as asked,
;                       clocked in as a transaction of their own (sending $FF).
;                       One open at a time (the fd's dups share it): another is ERR_IO_BUSY; so is a device
;                       with an SD card started on it, and SD_INIT doesn't start a card on one that's open.
;   /dev/spi/N/ctl      read: "mode 0" or "mode 3".  Write: "mode 0" (SCLK idles low) or "mode 3" (it idles
;                       high); both send and sample on the clock's rising edge, MSB first.  Kept until changed.
; Server ZP (the storage task's): SD_CLIENT, SD_FID, SD_DEV (the device), SD_BUF (its page), SD_N, SD_SRC,
; SD_DST; ZP_IO_REQ.

.segment "SPI_PD"

; The storage task's init (STORAGE_INIT3): no device open, mode 0, and their pages.  OUT: C = 0; or C = 1, .A =
; an error (MM_ALLOC's)
SPI_SRV_INIT:
            ldx         #15
:
            stz         SPI_REFS,X
            stz         SPI_MODE,X
            stz         SPI_RXN,X
            dex
            bpl         :-
            lda         #<(16 * 256)
            ldy         #>(16 * 256)
            ldx         #0
            jsr         MM_ALLOC                            ; (Whole pages: they don't move)
            bcs         @done
            jsr         MM_LOCK                             ; .A.Y = the first page (.A = 0)
            sty         SPI_RX_PAGES
            clc

@done:
            rts

; A request.  IN: .A = request, .X = client, .Y = fid
SPI_SERVE:
            stx         SD_CLIENT
            sty         SD_FID
            pha
            tya
            and         #$0F
            sta         SD_DEV                              ; The device (not for H9_OPEN: it has no fid)
            clc
            adc         SPI_RX_PAGES
            sta         SD_BUF + 1                          ;   and its page
            stz         SD_BUF
            pla
            cmp         #H9_OPEN
            beq         SPI_OPEN
            cmp         #H9_CREATE
            bcs         SPI_BAD
            ldx         SD_FID
            cpx         #SPI_FID_DIR                        ; /dev/spi itself?
            bcc         :+
            jmp         SPI_DIR
:
            cpx         #SPI_FID_CTL
            bcs         @ctl
            ldx         SD_DEV
            cmp         #H9_READ
            bne         :+
            jmp         SPI_READ
:
            cmp         #H9_WRITE
            bne         :+
            jmp         SPI_WRITE
:
            cmp         #H9_DUP
            bne         :+
            inc         SPI_REFS,X                          ; (Another fd on it)
            bra         SPI_OK
:
            cmp         #H9_CLUNK
            bne         SPI_BAD
            dec         SPI_REFS,X                          ; The last one: nothing kept
            bne         SPI_OK
            stz         SPI_RXN,X
            bra         SPI_MODE_FULL_OFF

@ctl:
            cmp         #H9_READ
            bne         :+
            jmp         SPI_CTL_READ
:
            cmp         #H9_WRITE
            bne         :+
            jmp         SPI_CTL_WRITE
:
            cmp         #H9_CLUNK
            beq         SPI_OK
            cmp         #H9_DUP
            bne         SPI_BAD

SPI_OK:
            lda         #0
            clc
            rts

SPI_BAD:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

SPI_BUSY:
            lda         #ERR_IO_BUSY
            sec
            rts

SPI_MODE_FULL_OFF:                                          ; (.X = the device)
            lda         SPI_MODE,X
            and         #<~SPI_M_FULL
            sta         SPI_MODE,X
            bra         SPI_OK

; The rest of the name is in the data area: "/N" or "/N/ctl" (N = 0-f, either case).  OUT: .A = the fid
SPI_OPEN:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
            lda         (ZP_IO_REQ),Y
            bne         :+
            dec         ZP_IO_REQ + 1                       ; "": /dev/spi itself, a directory (read only)
            ldy         #IO_BLK_MODE
            lda         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            and         #IO_MODE_WRITE
            bne         @mode
            lda         #SPI_FID_DIR
            clc
            rts

@mode:
            lda         #ERR_IO_MODE
            sec
            rts
:
            cmp         #'/'
            bne         @not_found
            iny
            lda         (ZP_IO_REQ),Y                       ; The device: a hex digit
            ora         #$20                                ; (Lower case; digits stay)
            sec
            sbc         #'0'
            cmp         #10
            bcc         @device
            sbc         #'a' - '0' - 10                     ; (C = 1)
            cmp         #10
            bcc         @not_found
            cmp         #16
            bcs         @not_found

@device:
            sta         SD_DEV
            iny
            lda         (ZP_IO_REQ),Y
            beq         @data
            ldx         #0                                  ; "/ctl"?
:
            lda         (ZP_IO_REQ),Y
            cmp         SPI_CTL_NAME,X
            bne         @not_found
            iny
            inx
            cmp         #0
            bne         :-
            lda         #SPI_FID_CTL
            bra         @open

@data:
            ldx         SD_DEV                              ; Not open already, and no card started on it
            lda         SPI_REFS,X
            bne         @busy
            cpx         #SD_MAX_CARDS
            bcs         :+
            lda         SD_CARD_STATE,X
            bne         @busy
:
            inc         SPI_REFS,X
            stz         SPI_RXN,X                           ; (Nothing kept)
            lda         SPI_MODE,X
            and         #<~SPI_M_FULL
            sta         SPI_MODE,X
            lda         #SPI_FID_DATA

@open:
            ora         SD_DEV
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (Keeps .A)
            clc
            rts

@busy:
            lda         #ERR_IO_BUSY
            bra         @fail

@not_found:
            lda         #ERR_IO_NOT_FOUND

@fail:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            sec
            rts

SPI_CTL_NAME:   .byte   "/ctl", 0
SPI_DIR_NAMES:  .byte   "0", 0, "1", 0, "2", 0, "3", 0, "4", 0, "5", 0, "6", 0, "7", 0  ; (/dev/spi's listing)
                .byte   "8", 0, "9", 0, "a", 0, "b", 0, "c", 0, "d", 0, "e", 0, "f", 0, 0
SPI_S_SPI:      .byte   "spi", 0

; /dev/spi itself: its listing (DIR_LIST), its stat (DIR_STAT).  IN: .A = request
SPI_DIR:
            cmp         #H9_READ
            beq         @read
            cmp         #H9_STAT
            beq         @stat
            cmp         #H9_CLUNK
            beq         @ok
            cmp         #H9_DUP
            beq         @ok
            jmp         SPI_BAD

@read:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            lda         #<SPI_DIR_NAMES
            ldy         #>SPI_DIR_NAMES
            jsr         DIR_LIST
            bra         @unmap

@stat:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            lda         #<SPI_S_SPI
            ldy         #>SPI_S_SPI
            jsr         DIR_STAT

@unmap:
            jsr         IO_SRV_UNMAP

@ok:
            jmp         SPI_OK

; Select device SD_DEV for a transaction, in its mode (mode 3: SCLK high before the select, so the first
; bit's store is a falling edge).  SPI_PORT keeps SCLK low, as SPI_XFER and SPI_RECV want.  Modifies: .A, .X
SPI_DEV_ON:
            ldx         SD_DEV
            txa
            asl
            asl
            asl
            and         #SPI_DEV_F
            ora         #SPI_BIT_MOSI                       ; Selected, MOSI high, SCLK low
            sta         SPI_PORT
            ora         SPI_MODE,X                          ; (SPI_M_IDLE: SCLK high; SPI_M_FULL isn't a port
            and         #<~SPI_M_FULL                       ;   bit)
            pha
            ora         #SPI_BIT_CSB                        ; Its number on the lines first, deselected, SCLK
            sta         IOR_SPI_DATA                        ;   idle
            pla
            sta         IOR_SPI_DATA                        ; Then selected
            rts

; Deselect, and SCLK to the mode's idle.  Preserves .X, .Y.  Modifies: .A
SPI_DEV_OFF:
            lda         SPI_PORT                            ; (SPI_DESELECT's)
            ora         #SPI_BIT_CSB
            sta         SPI_PORT
            sta         IOR_SPI_DATA
            phx
            ldx         SD_DEV
            lda         SPI_MODE,X
            plx
            and         #SPI_M_IDLE
            beq         :+
            lda         SPI_PORT                            ; (Mode 3: SCLK back up)
            ora         #SPI_M_IDLE
            sta         IOR_SPI_DATA
:
            rts

; .A = the data area's page (the request block's next; mapped), SD_SRC's low byte too (0).  Modifies: .A
SPI_DATA_AREA:
            stz         SD_SRC
            lda         ZP_IO_REQ + 1
            inc
            rts

; SD_N = the request's count: 1-256 (SD_N + 1 = 1 for 256).  ZP_IO_REQ: mapped.  Modifies: .A, .Y
SPI_COUNT:
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         SD_N
            iny
            lda         (ZP_IO_REQ),Y
            sta         SD_N + 1
            rts

; Write: send the data area's bytes, keeping what comes back (a page: SD_BUF)
SPI_WRITE:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            jsr         SPI_COUNT
            jsr         SPI_DATA_AREA                       ; SD_SRC: the data area ...
            sta         SD_SRC + 1
            lda         SD_BUF                              ; ... SD_DST: the device's page
            sta         SD_DST
            lda         SD_BUF + 1
            sta         SD_DST + 1
            jsr         SPI_DEV_ON
            jsr         SPI_XFER_N                          ; (256: SD_N = 0, so .Y goes round once)
            jsr         SPI_DEV_OFF
            ldx         SD_DEV
            lda         SD_N
            sta         SPI_RXN,X                           ; Kept: all of them
            stz         SPI_RXAT,X
            lda         SD_N + 1
            beq         :+
            lda         SPI_MODE,X                         ; (256)
            ora         #SPI_M_FULL
            sta         SPI_MODE,X
:
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of them sent)
            jmp         SPI_OK

; Read: the bytes kept from the last write, as many as asked (and they're gone); none kept: a transaction of its
; own, clocking in as many as asked (sending $FF)
SPI_READ:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            jsr         SPI_COUNT                           ; SD_N: asked (1-256)
            ldx         SD_DEV
            lda         SPI_RXN,X                           ; Kept: SPI_RXN, or 256 (SPI_M_FULL)
            bne         @kept
            lda         SPI_MODE,X
            bmi         @kept                               ; (SPI_M_FULL: 256, all of them)
            jsr         SPI_DATA_AREA                       ; None: clock them in, to the data area
            sta         SD_DST + 1
            stz         SD_DST
            jsr         SPI_DEV_ON
            jsr         SPI_RECV_N
            jsr         SPI_DEV_OFF
            bra         @count

@kept:
            lda         SD_N + 1                            ; How many: the kept ones, if fewer are asked for
            bne         @all                                ; (256 asked: all that are kept)
            lda         SPI_MODE,X
            bmi         @some                               ; (256 kept: as many as asked)
            lda         SPI_RXN,X
            cmp         SD_N
            bcs         @some

@all:
            lda         SPI_RXN,X                           ; All of them (0, with SPI_M_FULL: 256)
            sta         SD_N
            stz         SD_N + 1
            lda         SPI_MODE,X
            bpl         @some
            inc         SD_N + 1

@some:
            lda         SPI_RXAT,X                          ; SD_BUF: the next kept byte
            sta         SD_BUF
            inc         ZP_IO_REQ + 1
            ldy         #0
:
            lda         (SD_BUF),Y
            sta         (ZP_IO_REQ),Y
            iny
            cpy         SD_N
            bne         :-
            dec         ZP_IO_REQ + 1
            lda         SPI_RXAT,X                          ; Gone: SD_N of them
            clc
            adc         SD_N
            sta         SPI_RXAT,X
            lda         SPI_RXN,X
            sec
            sbc         SD_N
            sta         SPI_RXN,X
            lda         SPI_MODE,X                         ; (Fewer than 256 now, or none)
            and         #<~SPI_M_FULL
            sta         SPI_MODE,X

@count:
            ldy         #IO_BLK_COUNT                       ; The count: SD_N (256: 0 and 1)
            lda         SD_N
            sta         (ZP_IO_REQ),Y
            iny
            lda         SD_N + 1
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            jmp         SPI_OK

; The ctl file: "mode 0" or "mode 3" (and CR LF), read from the offset
SPI_CTL_READ:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_OFS + 3                     ; Past the line: nothing (end of file)
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @eof                                ; (Only from its start: a line is read at once)
            inc         ZP_IO_REQ + 1
            ldy         #SPI_MODE_LINE_LEN - 1
:
            lda         SPI_MODE_LINE,Y
            sta         (ZP_IO_REQ),Y
            dey
            bpl         :-
            ldx         SD_DEV
            lda         SPI_MODE,X
            and         #SPI_M_IDLE
            beq         :+
            lda         #'3'
            ldy         #SPI_MODE_DIGIT
            sta         (ZP_IO_REQ),Y
:
            dec         ZP_IO_REQ + 1
            jsr         SPI_COUNT
            lda         SD_N + 1
            bne         :+                                  ; (256 asked: the line)
            lda         SD_N
            cmp         #SPI_MODE_LINE_LEN
            bcc         @n
:
            lda         #SPI_MODE_LINE_LEN

@n:
            bra         @counted

@eof:
            lda         #0

@counted:
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            jmp         SPI_OK

SPI_MODE_LINE:  .byte   "mode 0", ASCII_CR, ASCII_LF
SPI_MODE_LINE_LEN = * - SPI_MODE_LINE
SPI_MODE_DIGIT  = 5

; Write "mode 0" or "mode 3" (a space, CR, LF or 0 may follow): the whole write is taken
SPI_CTL_WRITE:
            ldx         SD_CLIENT
            jsr         IO_SRV_MAP
            jsr         SPI_COUNT
            inc         ZP_IO_REQ + 1
            ldy         #0
:
            lda         SPI_MODE_LINE,Y                     ; "mode "
            cmp         (ZP_IO_REQ),Y
            bne         @bad
            iny
            cpy         #SPI_MODE_DIGIT
            bne         :-
            lda         SD_N + 1                            ; (Long enough: 7 bytes, or the digit last)
            bne         :+
            lda         SD_N
            cmp         #SPI_MODE_DIGIT + 1
            bcc         @bad
            beq         @digit
:
            iny                                             ; What follows the digit
            lda         (ZP_IO_REQ),Y
            dey
            cmp         #' ' + 1
            bcs         @bad                                ; (A space, CR, LF or 0 ends it)

@digit:
            lda         (ZP_IO_REQ),Y
            ldx         SD_DEV
            cmp         #'0'
            beq         @mode0
            cmp         #'3'
            bne         @bad
            lda         SPI_MODE,X
            ora         #SPI_M_IDLE
            sta         SPI_MODE,X
            bra         @done

@mode0:
            lda         SPI_MODE,X
            and         #<~SPI_M_IDLE
            sta         SPI_MODE,X

@done:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of it taken)
            jmp         SPI_OK

@bad:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            jmp         SPI_BAD
