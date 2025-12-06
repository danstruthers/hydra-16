.debuginfo

.segment "BUFFERS"
INPUT_BUFFER:
                .res 256

.segment "BIOS"

HEX_MAP: .byte "0123456789ABCDEF"
HYDRA_WELCOME: HString "Welcome to the HYDRA-16!"

SERIAL_INIT:
                sei
                lda             #$10 | SR_SELECT    ; 8-N-1
                sta             ACIA_CTRL
.if ROCKWELL_ACIA = 1
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLIE  ; No parity, no echo, tx & rx interrupts.
.else
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLID  ; No parity, no echo, rx interrupts.
.endif
                sta             ACIA_CMD
.if ROCKWELL_ACIA = 1
                stz             ZP_SERIAL_SEND_BUSY
.else
                jsr             WRITE_DELAY
.endif
                lda             #IRQ_NUMBER_ONBOARD_SERIAL
                ldx             #<SERIAL_IRQ_HANDLER
                ldy             #>SERIAL_IRQ_HANDLER
                jsr             IRQ_SET_VECTOR
                cli
                rts

; Input a character from the serial interface.
; On return, carry flag indicates whether a key was pressed
; If a key was pressed, the key value will be in the A register
;
; Modifies: flags, A
; TODO: select the appropriate read stream for the current task
READ_CHAR:
SERIAL_READ:
                jsr             BUFFER_SIZE
                beq             @no_keypressed
                phx
                ldx             ZP_READ_PTR
                lda             INPUT_BUFFER,x
                inc             ZP_READ_PTR
                plx
                ;cmp             #ASCII_ESC           ; do not echo 'ESC'
                ;beq             @no_echo
                jsr             WRITE_CHAR           ; echo
@no_echo:
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
WRITE_DEC:
                phx
                ldx             #0
                cmp             #0
                bcs             @do_hund
                pha
                PRINT_CHAR      #ASCII_MINUS
                pla
                cmp             #80                         ; special case for -128
                beq             @is_max
                jsr             NEGATE

@do_hund:
                cmp             #100
                bcc             @do_tens
                pha
                PRINT_CHAR      #ASCII_1                    ; must be 100-127
                pla
                sec
                sbc             #100
                cmp             #10                         ; special case for 100-109..need to print the 0 in tens
                bcc             @out_tens

@do_tens:
                cmp             #10
                bcc             @do_ones

@gt_ten:
                inx
                cmp             #10
                bcc             @out_tens
                sbc             #10
                bpl             @gt_ten

@out_tens:
                pha
                txa
                adc             #ASCII_0
                jsr             WRITE_CHAR
                pla

@do_ones:
                plx
                jmp             WRITE_HEX

@is_max:
                PRINT_CHAR      #ASCII_1
                lda             #28
                bpl             @do_tens

WRITE_BYTE_MIN:
                cmp             #$10
                bcc             WRITE_HEX

WRITE_BYTE:
                pha                                         ; Save A for LSD.
                lsr
                lsr
                lsr
                lsr                                         ; MSD to LSD position.
                jsr             WRITE_HEX                   ; Output hex digit.
                pla                                         ; Restore A.
WRITE_HEX_MASK:
                and             #$0F                        ; Mask LSD for hex print.

WRITE_HEX:
                phx
                tax
                lda             HEX_MAP,x
                plx
                ; Then fall through to WRITE_CHAR below.

WRITE_CHAR:
SERIAL_WRITE:
.if ROCKWELL_ACIA = 1
                phx
WRITE_DELAY:
                ldx             ZP_SERIAL_SEND_BUSY
                beq             @do_write
                wai                                         ; Leave this in, even if RDY has a pull-up
                bra            WRITE_DELAY
.endif
@do_write:
                IO_PORT_WRITE   ACIA_DATA

.if ROCKWELL_ACIA = 1
                ldx             #1
                stx             ZP_SERIAL_SEND_BUSY
.else
WRITE_DELAY:
                phx
                ldx             #SWT_SELECT
@txdelay:
                dex
                bne             @txdelay
.endif
                plx
                rts

; Convenience method to write CR/LF to output stream
WRITE_CRLF:
                PRINT_CHAR      #ASCII_CR
                PRINT_CHAR_JMP  #ASCII_LF

WRITE_PROMPT:
                PRINT_CRLF
                PRINT_CHAR      #ASCII_T
                PRINT_HEX_MASK  $FFF0
                PRINT_CHAR      #ASCII_SPACE
                PRINT_BYTE      $0
                lda             $0
                cmp             #$F0
                bcc             @not_shared
                PRINT_CHAR      #ASCII_LPAREN
                PRINT_HEX_MASK  $FFF1                       ; Shared RAM sub-bank
                PRINT_CHAR      #ASCII_RPAREN

@not_shared:
                PRINT_CHAR      #ASCII_COLON
                PRINT_BYTE      $1
                PRINT_CHAR_JMP  #ASCII_GT

; Initialize the circular input buffer
; Modifies: flags, A
INIT_BUFFER:
                MOV             ZP_READ_PTR, ZP_WRITE_PTR
                rts

; Escape sequences
CLEAR_SCR:
                PRINT_ESC_SEQ #ASCII_LBRACKET, #ASCII_2, #ASCII_J
                PRINT_ESC_SEQ_JMP #ASCII_LBRACKET, #ASCII_0, #ASCII_SEMI, #ASCII_0, #ASCII_f

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

; Return (in A) the number of unread bytes in the circular input buffer as an unsigned byte
; Modifies: flags, A
BUFFER_SIZE:
                lda             ZP_WRITE_PTR
                sec
                sbc             ZP_READ_PTR
                rts

; Maskable interrupt request handler; by default, do nothing
IRQ_HANDLER:
                rti


SERIAL_IRQ_HANDLER:
                pha

.if ROCKWELL_ACIA = 1
                lda             #ACIA_STATUS_BIT_TDRE
.endif

                bit             ACIA_STATUS
                bpl             @int_done 	            ; bit 7 not set, so not ACIA IRQ

.if ROCKWELL_ACIA = 1
                beq             @do_recv                ; if not Tx, then must be Rx
                stz             ZP_SERIAL_SEND_BUSY

@check_recv:
                lda             #ACIA_STATUS_BIT_RDRF   ; is read register full?
                bit             ACIA_STATUS
                beq             @int_done
.endif

@do_recv:
                IO_PORT_READ    ACIA_DATA
                phx
                ldx             ZP_WRITE_PTR
                sta             INPUT_BUFFER, X
                inc             ZP_WRITE_PTR
                plx

@int_done:
                pla
                rti

;; *****************************************************************
.if 0
; I2C

I2C_SCL = $01
I2C_SDA = $02
I2C_CTRL_PORT = VIA_PORTA
I2C_DATA_PORT = VIA_DDRA

.macro I2C_ON       val
            tay
            lda     #val
            ora     I2C_DATA_PORT
            sta     I2C_DATA_PORT
            tya
.endmacro

.macro I2C_OFF      val
            tay
            lda     #~val
            and     I2C_DATA_PORT
            sta     I2C_DATA_PORT
            tya
.endmacro

.macro SDA_LOW
            I2C_OFF I2C_SDA
.endmacro

.macro SCL_LOW
            I2C_OFF I2C_SCL
.endmacro

.macro SDA_HIGH
            I2C_ON  I2C_SDA
.endmacro

.macro SCL_HIGH
            I2C_ON  I2C_SDA
.endmacro

.macro SCL_PULSE
            inc     I2C_DATA_PORT
            dec     I2C_DATA_PORT
.endmacro

; A: Byte to send
; Return (in A): 1 = SUCCESS, 0 = FAILURE
I2C_SEND:
            ldx     #$00
            stx     I2C_CTRL_PORT
            ldx     #$09
@loop:
            dex
            beq     @ack
            rol
            jsr     I2C_SEND_BIT
            bra    @loop
@ack:
            jsr     I2C_RECV_BIT    ; ack in A, 0 = success
            eor     #$01            ; return 1 on success, 0 on fail
@end:
            rts


I2C_RECV:   lda     #$00
            sta     I2C_CTRL_PORT
            pha
            ldx     #$09
@loop:      dex
            beq     @end
            jsr     rec_bit
            ror
            pla
            rol
            pha
            jmp     @loop
@end:
            pla
            rts

; A: Bit to send
I2C_SEND_BIT:
            bcc     @send_one
            SDA_LOW
            bra    @clock_out
@send_one:
            SDA_HIGH

@clock_out:	
            SCL_PULSE
            SDA_LOW
            rts

I2C_RECV_BIT:
            SDA_HIGH
            SCL_HIGH
            lda     I2C_CTRL_PORT
            and     #I2C_SDA
            bne     @is_one
            lda     #$00
            jmp     @end
@is_one:
            lda     #$01
@end:
            SCL_LOW
            SDA_LOW
            rts


I2C_START:
            SDA_LOW
            SCL_LOW
            rts


I2C_STOP:
            SCL_HIGH
            SDA_HIGH
            rts


I2C_ACK:
            pha
            lda     #$00
            jsr     I2C_SEND_BIT
            pla
            rts

I2C_NACK:
            pha
            lda     #$01
            jsr     I2C_SEND_BIT
            pla
            rts
.endif

; ****************************************************************************

IRQ_VECTOR_INIT:
            sei
            PUSH_AX
            ldx     #$0F
            stx     V_REGISTER
            lda     #<SW_IRQ_HANDLER
            sta     $FFFE
            lda     #>SW_IRQ_HANDLER
            sta     $FFFF
            dex

@loop:
            stx     V_REGISTER
            lda     #<SERIAL_IRQ_HANDLER
            sta     $FFFE
            lda     #>SERIAL_IRQ_HANDLER
            sta     $FFFF
            dex
            bpl     @loop
            PULL_XA
            cli
            rts

; A: IRQ#, X: L, Y: H
IRQ_SET_VECTOR:
            sei
            pha
            lda     V_REGISTER
            sta     ZP_V_SAVE
            pla
            sta     V_REGISTER
            stx     $FFFE
            sty     $FFFF
            lda     ZP_V_SAVE
            sta     V_REGISTER
            cli
            rts

SW_IRQ_HANDLER:
            pha
            lsr
            lsr
            lsr
            lsr
            pla
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

.macro VECTORS
                .word   NMI_HANDLER     ; NMI vector
                .word   RESET_ENTRY     ; RESET vector
                .word   IRQ_HANDLER     ; IRQ vector
                ;       will actually be pulled from the Vector RAM, depending on lowest priority IRQ currently triggered,
                ;       or vector for IRQ# set in V[0..3] if none are triggered.  Can trigger S/W IRQs by setting V and then
                ;       calling BRK.  S/W IRQ# is $F, so setting V[0..3] to $F and v[4..7] to a different number, you can 
                ;       have up to 16 unique S/W IRQs.  You can invoke a hardware device IRQ handler in the same way
.endmacro

.segment "RESETVEC_P0"
    VECTORS

.segment "RESETVEC_P1"
    VECTORS

.segment "RESETVEC_P2"
    VECTORS

.segment "RESETVEC_P3"
    VECTORS

.segment "RESETVEC_P4"
    VECTORS

.segment "RESETVEC_P5"
    VECTORS

.segment "RESETVEC_P6"
    VECTORS

.segment "RESETVEC_P7"
    VECTORS

.segment "RESETVEC_P8"
    VECTORS

.segment "RESETVEC_P9"
    VECTORS

.segment "RESETVEC_PA"
    VECTORS

.segment "RESETVEC_PB"
    VECTORS

.segment "RESETVEC_PC"
    VECTORS

.segment "RESETVEC_PD"
    VECTORS

.segment "RESETVEC_PE"
    VECTORS

.segment "RESETVEC_PF"
    VECTORS
