.debuginfo

SER_SEND_STATUS_READY = 0
SER_SEND_STATUS_BUSY  = 1
SER_SEND_STATUS_ERROR = $FF

.segment "BIOS"

HEX_MAP: .byte "0123456789ABCDEF"
NamedHString HYDRA_WELCOME, "Welcome to the HYDRA-16!"

; ****************************************************************************
; Serial driver.  Runs in its own Resident task (SERIAL_TASK_NUM, started by DRV_START at boot), so
; its state (ZP_SER_SEND_STATUS, ZP_SER_CAPTURE) lives in that task's ZP.
;   RX: the IRQ handler sends each received byte to the serial-capture task as a message.
;   TX: WRITE_CHAR writes the ACIA directly from the calling task, using the send status in the
;       serial task's ZP.

SERIAL_DRIVER:
                .word           SERIAL_INIT             ; DriverInfo::init
                .word           SERIAL_STOP             ; DriverInfo::stop
                .word           SERIAL_NAME             ; DriverInfo::name
NamedHString SERIAL_NAME, "SERIAL"

; Gate into the serial task: set the serial-capture task (the task that receives serial input)
; IN: .A = task
TASK_GATE       SER_CALL_SET_CAPTURE, SERIAL_SET_CAPTURE, SERIAL_TASK_NUM

; Driver init (runs in the serial task).  OUT: C = 0 on success, or C = 1 and .A = error
SERIAL_INIT:
                php                                     ; Save caller's I flag
                sei
                lda             #$10 | SR_SELECT    ; 8-N-1
                sta             ACIA_R_CTRL
.if ROCKWELL_ACIA = 1
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLIE  ; No parity, no echo, tx & rx interrupts.
.else
                lda             #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLID  ; No parity, no echo, rx interrupts.
.endif
                sta             ACIA_R_CMD
.if ROCKWELL_ACIA = 1 .OR ACIA_USE_VIA_TIMER = 1
                lda             #SER_SEND_STATUS_READY
                sta             ZP_SER_SEND_STATUS
.endif
                lda             #SHELL_TASK_NUM         ; Serial input goes to the shell by default
                sta             ZP_SER_CAPTURE
.if ACIA_USE_VIA_TIMER = 1
                ldx             #IRQ_NUMBER_ONBOARD_VIA ; VIA T2 paces TX
                lda             #<SERIAL_VIA_T2_HANDLER
                ldy             #>SERIAL_VIA_T2_HANDLER
                jsr             IRQ_REGISTER
                bcs             @done
.endif
                ldx             #IRQ_NUMBER_ONBOARD_SERIAL
                lda             #<SERIAL_IRQ_HANDLER
                ldy             #>SERIAL_IRQ_HANDLER
                jsr             IRQ_REGISTER            ; Handler runs in this (the serial) task

@done:
                jmp             MM_RETURN               ; Restore caller's I flag, keep C

SERIAL_STOP:
                clc
                rts

; Set the serial-capture task (runs in the serial task; use SER_CALL_SET_CAPTURE)
; IN: .A = task
SERIAL_SET_CAPTURE:
                sta             ZP_SER_CAPTURE
                clc
                rts

; Input a character from the serial interface: the next byte the serial task has sent to the
; current task.  Only the serial-capture task receives input.
; On return, carry flag indicates whether a key was pressed
; If a key was pressed, the key value will be in the A register
;
; Modifies: flags, A
READ_CHAR:
SERIAL_READ:
                phx
                ldx             #SERIAL_TASK_NUM
                jsr             MSG_RECV_BYTE           ; C = 0: .A = byte
                plx
                bcc             :+
                clc                                     ; Nothing waiting
                rts
:
                PRINT_CHAR                                  ; echo
                sec
                rts

; Write decimal value of .A to output
WRITE_DEC:
                cmp             #0
                bpl             WRITE_DEC_U
                pha
                PRINT_CHAR      #ASCII_MINUS
                pla
                cmp             #$80                        ; special case for -128
                bne             :+
                PRINT_CHAR      #ASCII_1
                PRINT_BYTE_JMP  #$28
:
                jsr             NEGATE

WRITE_DEC_U:
                jsr             MOD_10
                cpx             #0
                beq             :++++
                phy
                tay
                txa
                jsr             MOD_10
                cpx             #0
                bne             :+
                cmp             #0
                bne             :++
                bra             :+++
:
                pha
                txa
                jsr             WRITE_HEX
                pla
:
                jsr             WRITE_HEX
:
                tya
                ply
:
                jmp             WRITE_HEX

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
                SKIPNEXT

; Output a character (from the A register) to the serial interface.
;
; Modifies: flags
; Can be called from any task: the send status lives in the serial owner task's ZP.
; TODO: select appropriate output stream for the given task
WRITE_CHAR:
SERIAL_WRITE:
                phx                                         ; Must stay 1 byte (WRITE_HEX SKIPNEXTs over it)
                phy

WRITE_DELAY:
.if ROCKWELL_ACIA = 1 .OR ACIA_USE_VIA_TIMER = 1
                php                                         ; Save caller's I flag
                sei
                ldy             T_REGISTER
                ldx             #SERIAL_OWNER_TASK
                stx             T_REGISTER                  ; Quick switch to the serial owner task (no stack use!)
                ldx             ZP_SER_SEND_STATUS
                cpx             #SER_SEND_STATUS_READY
                beq             :+
                sty             T_REGISTER                  ; Back to the calling task
                plp                                         ; Restore caller's I flag
                wai                                         ; Leave this in, even if RDY has a pull-up
                bra             WRITE_DELAY
:
                ldx             #SER_SEND_STATUS_BUSY
                stx             ZP_SER_SEND_STATUS
                IO_PORT_WRITE   ACIA_R_DATA
                sty             T_REGISTER                  ; Back to the calling task
                plp                                         ; Restore caller's I flag
.else
                IO_PORT_WRITE   ACIA_R_DATA
.endif

.if ROCKWELL_ACIA = 0
    .if ACIA_USE_VIA_TIMER = 1
                pha
                lda             #SWT_SELECT_L
                ldy             #SWT_SELECT_H
                jsr             VIA_START_T2
                pla
    .else
                ldx             #SWT_SELECT_L + 1
                ldy             #SWT_SELECT_H + 1
:
                dex
                bne             :-
                dey
                bne             :-
    .endif
.endif

                ply
                plx
                rts

; Convenience method to write CR/LF to output stream
WRITE_CRLF:
                PRINT_CHAR_JMP  #ASCII_CR, #ASCII_LF

WRITE_PROMPT:
                PRINT_CRLF
                PRINT_CHAR      #ASCII_T
                PRINT_HEX_MASK  $FFF0
                PRINT_SPACE
                PRINT_BYTE      RAM_BANK_REG
                cmp             #$F0
                bcc             @not_shared
                PRINT_CHAR      #ASCII_LPAREN
                PRINT_HEX_MASK  $FFF1                       ; Shared RAM sub-bank
                PRINT_CHAR      #ASCII_RPAREN

@not_shared:
                PRINT_CHAR      #ASCII_COLON
                PRINT_BYTE      ROM_BANK_REG
                PRINT_CHAR_JMP  #ASCII_GT

; .A, .Y hold the addr of HString to write
; Clobbers .A, .Y; Preserves .X
WRITE_HSTRING:
                phx
                sta             ZP_HS_TEMP
                sty             ZP_HS_TEMP + 1
                lda             (ZP_HS_TEMP)                ; Length of HString
                tax
                ldy             #0
@write_loop:
                iny
                PRINT_CHAR      {(ZP_HS_TEMP),Y}
                dex
                bne             @write_loop
                plx
                rts

; Escape sequences
CLEAR_SCR:
                PRINT_ESC_SEQ #ASCII_LBRACKET, #ASCII_2, #ASCII_J
                PRINT_ESC_SEQ_JMP #ASCII_LBRACKET, #ASCII_0, #ASCII_SEMI, #ASCII_0, #ASCII_f

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

; Serial IRQ handler (registered with IRQ_REGISTER; runs in the serial task)
; OUT: C = 1 if the ACIA was interrupting
SERIAL_IRQ_HANDLER:
.if ROCKWELL_ACIA = 1
                lda             #ACIA_STATUS_BIT_TDRE
.endif

                bit             ACIA_R_STATUS
                bpl             @not_mine 	            ; bit 7 not set, so not ACIA IRQ

.if ROCKWELL_ACIA = 1
                beq             @do_recv                ; if not Tx, then must be Rx
                lda             #SER_SEND_STATUS_READY
                sta             ZP_SER_SEND_STATUS

@check_recv:
                lda             #ACIA_STATUS_BIT_RDRF   ; is read register full?
                bit             ACIA_R_STATUS
                beq             @int_done
.endif

@do_recv:
                IO_PORT_READ    ACIA_R_DATA
                ldx             ZP_SER_CAPTURE
                jsr             MSG_SEND_BYTE           ; To the capture task (dropped if its ring is full)

@int_done:
                sec
                rts

@not_mine:
                clc
                rts

;; *****************************************************************
.if 0
; I2C

I2C_SCL = $01
I2C_SDA = $02
I2C_CTRL_PORT = VIA_R_PORTA
I2C_DATA_PORT = VIA_R_DDRA

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

VIA_T2_INT_BIT = $20
VIA_T1_INT_BIT = $40
VIA_INT_ENABLE = $80


; Must be called from the system task (the VIA handler runs there)
VIA_INIT:
.if ROCKWELL_ACIA <> 1 .AND ACIA_USE_VIA_TIMER = 1
            pha
            lda     #0
            sta     VIA_R_AUX_CTRL
            pla
.endif
            PUSH_AXY
            ldx     #IRQ_NUMBER_ONBOARD_VIA
            lda     #<VIA_IRQ_HANDLER
            ldy     #>VIA_IRQ_HANDLER
            jsr     IRQ_REGISTER
            PULL_YXA
            rts

VIA_ENABLE_T1_INT:
            lda     #VIA_T1_INT_BIT
            ora     #VIA_INT_ENABLE
            sta     VIA_R_INT_ENABLE
            rts

VIA_ENABLE_T2_INT:
            lda     #VIA_T2_INT_BIT
            ora     #VIA_INT_ENABLE
            sta     VIA_R_INT_ENABLE
            rts

VIA_DISABLE_T1_INT:
            pha
            lda     #VIA_T1_INT_BIT
            sta     VIA_R_INT_ENABLE
            pla
            rts

VIA_DISABLE_T2_INT:
            pha
            lda     #VIA_T2_INT_BIT
            sta     VIA_R_INT_ENABLE
            pla
            rts

; .A.Y = Timer value
VIA_START_T2:
            sta     VIA_R_T2C_L
            sty     VIA_R_T2C_H
            jmp     VIA_ENABLE_T2_INT

VIA_STOP_T2:
            jmp     VIA_DISABLE_T2_INT

; .A = Sub-component interrupt flag to test
VIA_IS_INT:
            clc
            and     VIA_R_INT_FLAGS
            beq     :+
            sec
:
            rts

; OUT: C = 1 if T2 timer set the IRQ, 0 if not
VIA_IS_T1_INT:
            lda     #VIA_T1_INT_BIT
            bra     VIA_IS_INT

VIA_IS_T2_INT:
            lda     #VIA_T2_INT_BIT
            bra     VIA_IS_INT

VIA_CLEAR_T1_INT:       ; READ LOB OF T1 Counter
            lda     VIA_R_T1C_L
            rts

VIA_CLEAR_T2_INT:       ; READ LOB OF T2 Counter
            lda     VIA_R_T2C_L
            rts

; ****************************************************************************

; Serial driver's VIA T2 handler (TX pacing; registered by SERIAL_INIT, runs in the serial task)
; OUT: C = 1 if T2 was interrupting
SERIAL_VIA_T2_HANDLER:
            lda     #VIA_T2_INT_BIT
            and     VIA_R_INT_FLAGS
            beq     :+
            lda     #SER_SEND_STATUS_READY
            sta     ZP_SER_SEND_STATUS     ; signals serial send buffer is ready for another byte
            lda     VIA_R_T2C_L             ; clear the interrupt
            lda     #VIA_T2_INT_BIT         ; disable the T2 interrupt
            sta     VIA_R_INT_ENABLE
            sec
            rts

:
            clc
            rts

; VIA IRQ handler (registered by VIA_INIT; runs in the system task).  T2 belongs to the serial driver.
; OUT: C = 1 if T1 was interrupting
VIA_IRQ_HANDLER:
; check which sub-device is triggering the IRQ
            lda     #VIA_T1_INT_BIT
            and     VIA_R_INT_FLAGS
            beq     :+
                                            ; Do whatever T1 timer would do
            lda     VIA_R_T1C_L             ; clear the interrupt
            sec
            rts

:
            clc
            rts


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
                .word   NMI_ENTRY       ; NMI vector (COMMON block, same address on every page)
                .word   RESET_ENTRY     ; RESET vector
                .word   IRQ_STUB_F      ; IRQ vector (COMMON block S/W IRQ stub; unused, see below)
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
