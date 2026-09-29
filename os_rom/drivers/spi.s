.debuginfo

; ****************************************************************************
; SPI, bit-banged on the VIA's port B (BIOS ROM page 3, the storage page; included inside `.scope PAGE3`,
; see all.s).  Only the storage task uses it (the SD card server), so its state is that task's ZP.
;
;   Port B: PB0 = SCLK, PB1 = /CS enable (low: the device selected by PB3-PB6 is selected), PB2 = MOSI,
;   PB3-PB5 = device 0-7 (a 74HC138 on the board: /nSPI_CS0-7, the SPI headers J18-J25), PB6 = 1 for
;   devices 8-15 (decoded on the card slots), PB7 = MISO (input).
;   Mode 0: SCLK idles low, both sides sample on the rising edge.  About 108 kHz for SPI_XFER and 199 kHz
;   for SPI_RECV at 3.58 MHz: under the 400 kHz an SD card allows while it starts up.
;   These are the SD card's inner loops (about 64% of a block read), so they're unrolled and kept short:
;   18 cycles a bit in, 33 out.  At 7.16 MHz (CPU_CLOCK_MULT 2) the receive loop is padded, or its SCLK
;   would reach 398 kHz, too close to the start-up limit (_M_SPI_PAD).

.segment "STORAGE_P3"

; Pad the receive loop at 7.16 MHz, to keep SCLK at the 3.58 MHz build's rate (about 275 kHz).  Nothing at
; 3.58 MHz, where 18 cycles a bit is already slow enough.
.macro _M_SPI_PAD
.if ::CPU_CLOCK_MULT > 1
            nop
            nop
            nop
            nop
.endif
.endmacro

; One bit in, MSB first: SCLK high (the device presents its bit), sample MISO, SCLK low, shift it into .A.
; IN: .Y = the port value with SCLK low; .X is the sample.  18 cycles
.macro _M_SPI_BIT_IN
            inc         IOR_SPI_DATA                        ; SCLK high: the device presents its bit
            ldx         IOR_SPI_DATA                        ; MISO = bit 7
            sty         IOR_SPI_DATA                        ; SCLK low
            cpx         #$80                                ; C = MISO
            rol                                             ; ... into the byte
            _M_SPI_PAD
.endmacro

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
; (The bit's store drops SCLK for the next one, so there's no separate SCLK-low write.)
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
            stx         IOR_SPI_DATA                        ; MOSI low, SCLK low
            bra         @clock

@one:
            sty         IOR_SPI_DATA                        ; MOSI high, SCLK low

@clock:
            inc         IOR_SPI_DATA                        ; SCLK high: both sides sample
            lda         IOR_SPI_DATA                        ; (MISO = bit 7)
            asl                                             ; C = MISO
            rol         SPI_IN
            bcc         @bit
            sty         IOR_SPI_DATA                        ; Idle: SCLK low, MOSI high
            ply
            plx
            lda         SPI_IN
            rts

; Receive a byte (sending $FF, MOSI high).  OUT: .A, and N/Z from it.  Preserves .X, .Y
SPI_RECV:
            phx
            phy
            ldy         SPI_PORT                            ; (SCLK low, MOSI high)
            sty         IOR_SPI_DATA
            lda         #0

            .repeat     8
            _M_SPI_BIT_IN
            .endrepeat

            ply
            plx
            ora         #0                                  ; N/Z from the byte (SD_CMD waits for one with bit 7
            rts                                             ;   clear: its bpl), as the pulls have clobbered them

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
