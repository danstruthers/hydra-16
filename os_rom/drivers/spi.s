.debuginfo

; ****************************************************************************
; SPI, bit-banged on the VIA's port B (BIOS ROM page 3, the storage page; included inside `.scope PAGE3`,
; see all.s).  Only the storage task uses it (the SD card server), so its state is that task's ZP.
;
;   Port B: PB0 = SCLK, PB1 = /CS enable (low: the device selected by PB3-PB6 is selected), PB2 = MOSI,
;   PB3-PB5 = device 0-7 (a 74HC138 on the board: /nSPI_CS0-7, the SPI headers J18-J25), PB6 = 1 for
;   devices 8-15 (decoded on the card slots), PB7 = MISO (input).
;   Mode 0: SCLK idles low, both sides sample on the rising edge.  About 90 kHz for SPI_XFER and 130 kHz
;   for SPI_RECV at 3.58 MHz: under the 400 kHz an SD card allows while it starts up.

.segment "STORAGE_P3"

; Set up port B for SPI (nothing selected).  Doesn't touch the VIA's timers or interrupts (T1 is the
; scheduler's tick).  Modifies: .A
SPI_INIT:
            lda         #SPI_BIT_CSB | SPI_BIT_MOSI         ; Deselected, SCLK low, MOSI high
            sta         IOR_SPI_DATA
            lda         #SPI_DDR_BITS
            sta         IOR_SPI_DDR
            rts

; Select SPI device .A (0-15).  Modifies: .A
SPI_SELECT:
            asl
            asl
            asl
            and         #SPI_DEV_F                          ; (PB3-PB6)
            ora         #SPI_BIT_MOSI                       ; /CS enable low, SCLK low, MOSI high
            sta         SPI_PORT
            sta         IOR_SPI_DATA
            rts

; Deselect (the device keeps its number; the enable goes high).  Preserves .X, .Y, .A
SPI_DESELECT:
            pha
            lda         SPI_PORT
            ora         #SPI_BIT_CSB
            sta         SPI_PORT
            sta         IOR_SPI_DATA
            pla
            rts

; Send .A and return the byte received at the same time.  Preserves .X, .Y
SPI_XFER:
            phx
            phy
            sta         SPI_OUT
            lda         SPI_PORT
            and         #<~SPI_BIT_MOSI
            tax                                             ; .X = the port with MOSI low
            ora         #SPI_BIT_MOSI
            tay                                             ; .Y = with MOSI high
            lda         #1                                  ; (The 1 comes out after 8 bits)
            sta         SPI_IN

@bit:
            asl         SPI_OUT                             ; C = the bit to send
            bcs         @one
            stx         IOR_SPI_DATA
            bra         @clock

@one:
            sty         IOR_SPI_DATA

@clock:
            inc         IOR_SPI_DATA                        ; SCLK high: both sides sample
            lda         IOR_SPI_DATA                        ; (MISO = bit 7)
            dec         IOR_SPI_DATA                        ; SCLK low
            asl
            rol         SPI_IN
            bcc         @bit
            ply
            plx
            lda         SPI_IN
            rts

; Receive a byte (sending $FF, MOSI high).  OUT: .A.  Preserves .X, .Y
SPI_RECV:
            lda         SPI_PORT                            ; (MOSI high)
            sta         IOR_SPI_DATA
            lda         #1
            sta         SPI_IN

@bit:
            inc         IOR_SPI_DATA                        ; SCLK high
            lda         IOR_SPI_DATA
            dec         IOR_SPI_DATA                        ; SCLK low
            asl
            rol         SPI_IN
            bcc         @bit
            lda         SPI_IN
            rts

; Clock .A * 8 cycles with nothing selected and MOSI high (an SD card needs 74 before it starts).
; Modifies: .A
SPI_IDLE_CLOCKS:
            pha
            jsr         SPI_DESELECT
            pla

@byte:
            pha
            jsr         SPI_RECV
            pla
            dec
            bne         @byte
            rts
