.debuginfo

.segment "BIOS"

.macro SPI_SEND_CMD b0, b1, b2, b3, b4, crc
                lda             #b0 | $40
                jsr             SPI_SEND
                lda             #b1
                jsr             SPI_TRANSCEIVE
                sta             ZP_TEMP_2
                lda             #b2
                jsr             SPI_TRANSCEIVE
                lda             #b3
                jsr             SPI_TRANSCEIVE
                lda             #b4
                jsr             SPI_TRANSCEIVE
.ifnblank       crc
                lda             #(crc << 1)+1
.else
                lda             #$FF
.endif
                jsr             SPI_TRANSCEIVE
                jsr             SPI_RECV
                sta             ZP_TEMP
.endmacro

SERIAL_INIT:
                lda             #$10 | SR_SELECT    ; 8-N-1
                sta             ACIA_CTRL
.if ROCKWELL_ACIA = 1
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLIE  ; No parity, no echo, tx & rx interrupts.
.else
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLID  ; No parity, no echo, rx interrupts.
.endif
                sta             ACIA_CMD
.if ROCKWELL_ACIA = 1
    .ifpc02
                stz             ZP_SERIAL_SEND_BUSY
    .else
                lda             #0
                sta             ZP_SERIAL_SEND_BUSY
    .endif
.else
                jsr             WRITE_DELAY
.endif
                rts

; Input a character from the serial interface.
; On return, carry flag indicates whether a key was pressed
; If a key was pressed, the key value will be in the A register
;
; Modifies: flags, A
; TODO: select the appropriate read stream for the current task
READCHAR:
SERIAL_READ:
                jsr             BUFFER_SIZE
                beq             @no_keypressed
                PUSH_X
                ldx             ZP_READ_PTR
                lda             INPUT_BUFFER, X
                inc             ZP_READ_PTR
                PULL_X
                jsr             WRITECHAR           ; echo
                sec
                bcs             @rc_cleanup

@no_keypressed:
                clc

@rc_cleanup:
                rts


; Output a character (from the A register) to the serial interface.
;
; Modifies: flags
; TODO: select appropriate output stream for the given task
WRITECHAR:
SERIAL_WRITE:
.if ROCKWELL_ACIA = 1
                PUSH_X
WRITE_DELAY:
                ldx             ZP_SERIAL_SEND_BUSY
                beq             @do_write
    .ifpc02
                wai
    .endif
                bne             WRITE_DELAY
.endif
@do_write:
                IO_PORT_WRITE   ACIA_DATA

.if ROCKWELL_ACIA = 1
                ldx             #1
                stx             ZP_SERIAL_SEND_BUSY
.else
WRITE_DELAY:
                PUSH_X
                ldx             #SWT_SELECT
@txdelay:
                dex
                bne             @txdelay
.endif
                PULL_X
                rts

; Convenience method to write CR/LF to output stream
WRITE_CRLF:
                lda             #ASCII_CR
                jsr             WRITECHAR
                lda             #ASCII_LF
                jmp             WRITECHAR                   ; WRITECHAR will rts, so jmp instead of jsr

; Taken from Wozmon
WRITE_BYTE:
                pha                                         ; Save A for LSD.
                lsr
                lsr
                lsr
                lsr                                         ; MSD to LSD position.
                jsr             @print_hex                  ; Output hex digit.
                pla                                         ; Restore A.
                and             #$0F                        ; Mask LSD for hex print.

@print_hex:
                cmp             #10                         ; Digit?
                bcc             @echo                       ; Yes, output it.
                adc             #ASCII_LETTER_OFFSET-1      ; Add offset for letter, -1 because carry is set.

@echo:
                adc             #ASCII_0                    ; Add "0".
                jmp             WRITECHAR                   ; WRITECHAR will rts, so jmp instead of jsr

; Initialize the circular input buffer
; Modifies: flags, A
INIT_BUFFER:
                MOV             ZP_READ_PTR, ZP_WRITE_PTR
                rts

; Set up the SPI interface registers on the VIA
SPI_INIT:
                pha
                IO_PORT_WRITE   VIA_AUX_CTRL, , 0
                IO_PORT_WRITE   VIA_INT_ENABLE
                IO_PORT_WRITE   VIA_PER_CTRL, , $FF
                IO_PORT_WRITE   IOR_SPI_DDR,  , SPI_DDR_BITS
                IO_PORT_WRITE   IOR_SPI_DATA, , SPI_BIT_CSB   ; de-select all SPI devices
                pla
                rts

; Macro to remove essentially duplicate code
.macro          SPI_SEND_SETUP  mode
                sta             ZP_SPI_DATA_OUT
                PUSH_Y
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
                SJMP            @spi_send
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
                SJMP            @spi_send_0
@had_1:
                asl             ZP_SPI_DATA_IN
                SJMP            @spi_send_1

SPI_OPERATION_DONE:
                lda             #SPI_BIT_CSB        ; de-select all SPI devices
.ifpc02
                tsb             IOR_SPI_DATA
.else
                ora             IOR_SPI_DATA
                sta             IOR_SPI_DATA
.endif
                PULL_Y
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
                SJMP            @spi_send
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
                PUSH_Y
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
.ifpc02
                inc
.else
                ora             #1
.endif
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

; Return (in A) the number of unread bytes in the circular input buffer as an unsigned byte
; Modifies: flags, A
BUFFER_SIZE:
                lda             ZP_WRITE_PTR
                sec
                sbc             ZP_READ_PTR
                rts

; Maskable interrupt request handler
IRQ_HANDLER:
.if ROCKWELL_ACIA = 1
                pha
                lda             #ACIA_STATUS_BIT_TDRE
.endif
                bit             ACIA_STATUS
                bpl             @not_acia 	            ; bit 7 not set, so N is not set
.if ROCKWELL_ACIA = 1
                beq             @do_recv                ; if not Tx, then must be Rx
    .ifpc02
                stz             ZP_SERIAL_SEND_BUSY
    .else
                lda             #0
                sta             ZP_SERIAL_SEND_BUSY
    .endif

@check_recv:
                lda             #ACIA_STATUS_BIT_RDRF   ; is read register full?
                bit             ACIA_STATUS
                beq             @skip_read
.else
                pha
.endif

@do_recv:
                IO_PORT_READ    ACIA_DATA
                PUSH_X
                ldx             ZP_WRITE_PTR
                sta             INPUT_BUFFER, X
                inc             ZP_WRITE_PTR
                PULL_X

@skip_read:
                pla

@not_acia:
@int_done:
                rti

.segment "IO_PORTS"
IO_PORT_0:      .tag IO_Port
IO_PORT_1:      .tag IO_Port
IO_PORT_2:      .tag IO_Port
IO_PORT_3:      .tag IO_Port
IO_PORT_4:      .tag IO_Port
IO_PORT_5:      .tag IO_Port
IO_PORT_6:      .tag IO_Port
IO_PORT_7:      .tag IO_Port
IO_PORT_8:      .tag IO_Port
IO_PORT_9:      .tag IO_Port
IO_PORT_A:      .tag IO_Port
IO_PORT_B:      .tag IO_Port
IO_PORT_C:      .tag IO_Port
IO_PORT_D:      .tag IO_Port
IO_PORT_E:      .tag IO_Port
IO_PORT_F:      .tag IO_Port_10_Bytes

.segment "RESETVEC"
                .word   NMI_HANDLER     ; NMI vector
                .word   OS_MAIN         ; RESET vector
                .word   IRQ_HANDLER     ; IRQ vector
