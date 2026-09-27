.debuginfo

; ****************************************************************************
; SPI (bit-banged on the VIA).  BIOS ROM page 2, included inside `.scope PAGE2` (see all.s); moved here
; from page 0 (bios.s) to make room there.  Not called yet (SPI_INIT and SPI_TEST are commented out in
; os_main.s); the SD card server (IO plan, Phase 3) will use it.

.segment "SPI_P2"

; Set up the SPI interface registers on the VIA
SPI_INIT:
                pha
                IO_PORT_WRITE   VIA_R_AUX_CTRL, , 0
                IO_PORT_WRITE   VIA_R_INT_ENABLE
                IO_PORT_WRITE   VIA_R_PER_CTRL, , $FF
                IO_PORT_WRITE   IOR_SPI_DDR,  , SPI_DDR_BITS
                IO_PORT_WRITE   IOR_SPI_DATA, , SPI_BIT_CSB   ; de-select all SPI devices
                pla
                rts

; Macro to remove essentially duplicate code
.macro          SPI_SEND_SETUP  mode
                sta             ZP_SPI_DATA_OUT
                phy
                txa
                ora             #SPI_BIT_MOSI
                tay
                lda             ZP_SPI_DATA_OUT
                sei
.ifblank        mode
                asl             ZP_SPI_DATA_IN
.endif
                sec
                rol
.endmacro

; Write and Read SPI data
; Uses two ZP registers for data_in and data_out
; A: data to send
; X: device ID to send to/receive from
; Returns input data in A
; Modifies A, ZP_SPI_DATA_IN, ZP_SPI_DATA_OUT
SPI_TRANSCEIVE:
                SPI_SEND_SETUP
                bcs             @spi_send_1
@spi_send_0:
                stx             IOR_SPI_DATA
                bra             @spi_send
@spi_send_1:
                sty             IOR_SPI_DATA
@spi_send:
                inc             IOR_SPI_DATA        ; SPI_CLK = 1
                bit             IOR_SPI_DATA        ; MISO (bit 7) => N flag
                bpl             @spi_recv
                inc             ZP_SPI_DATA_IN      ; incoming bit was a 1 (set LSb = 1)
@spi_recv:
                asl
                beq             SPI_OPERATION_DONE
                bcs             @had_1
                asl             ZP_SPI_DATA_IN
                bra             @spi_send_0
@had_1:
                asl             ZP_SPI_DATA_IN
                bra             @spi_send_1

SPI_OPERATION_DONE:
                lda             #SPI_BIT_CSB        ; de-select all SPI devices
                tsb             IOR_SPI_DATA
                ply
                lda             ZP_SPI_DATA_IN      ; load the input for return in A
                cli
                rts

; Write SPI data
; Uses two ZP registers for data_in and data_out
; A: data to send
; X: device ID to send to
; Modifies A, ZP_SPI_DATA_OUT
SPI_SEND:
                SPI_SEND_SETUP  1
@send_loop:
                bcs             @spi_send_1
                stx             IOR_SPI_DATA
                bra             @spi_send
@spi_send_1:
                sty             IOR_SPI_DATA
@spi_send:
                inc             IOR_SPI_DATA        ; SPI_CLK = 1
                asl
                bne             @send_loop
                jmp             SPI_OPERATION_DONE

; Read from the SPI device
; X: device to read from
; Result returned in A
SPI_RECV:
                phy
                ldy             #8
                txa
                ora             #SPI_BIT_MOSI | SPI_BIT_CSB
                sta             IOR_SPI_DATA        ; Select the device to receive from
                sei
@recv_loop:
                asl                                 ; Shift in 0 to LSb of result
                inc             IOR_SPI_DATA
                bit             IOR_SPI_DATA        ; MISO (bit 7) => N flag
                bpl             @spi_recv_2
                                                    ; Set LSb = 1
                inc
@spi_recv_2:
                dey
                bne             @recv_loop
                sta             ZP_SPI_DATA_IN
                jmp             SPI_OPERATION_DONE

; Delay for some number of cycles to ensure SPI device is ready to start working
SPI_INIT_DELAY:
                PUSH_AXY
                txa                                         ; set SPI device
                ora             #SPI_BIT_CSB | SPI_BIT_MOSI ; de-select all devices
                tax
                ora             #SPI_BIT_CLK
                ldy             #SPI_INIT_DELAY_CYCLES
@loop:
                sta             IOR_SPI_DATA
                stx             IOR_SPI_DATA
                dey
                bne             @loop
                PULL_YXA
                rts

SPI_TEST:
            ldx                 #SPI_DEV_0
            jsr                 SPI_INIT_DELAY
            SPI_SEND_CMD        0, 0, 0, 0,   0, $4A                ; CMD0
            SPI_SEND_CMD        8, 0, 0, 1, $AA, $43                ; CMD8
@loop:
            SPI_SEND_CMD        58, 0, 0, 0,   0                    ; CMD58
            SPI_SEND_CMD        41, $40, 0, 0,   0                  ; ACMD41
            bne                 @loop
            rts
