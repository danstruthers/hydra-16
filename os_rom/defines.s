.debuginfo
.macpack        cpu

ZP_VAR_START    = $10

ZP_READ_PTR     = ZP_VAR_START
ZP_WRITE_PTR    = ZP_READ_PTR + 1

ZP_TEMP         = ZP_WRITE_PTR + 1
ZP_TEMP_2       = ZP_TEMP + 1

ZP_SPI_DATA_IN  = ZP_TEMP_2 + 1
ZP_SPI_DATA_OUT = ZP_SPI_DATA_IN + 1

ZP_TEMP_VEC_L   = ZP_SPI_DATA_OUT + 1
ZP_TEMP_VEC_H   = ZP_TEMP_VEC_L + 1

ZP_A_SAVE       = ZP_TEMP_VEC_H + 1
ZP_X_SAVE       = ZP_A_SAVE + 1
ZP_Y_SAVE       = ZP_X_SAVE + 1

ZP_LAST_USED    = ZP_Y_SAVE

IO_PORT_BASE    = $FF00
INPUT_BUFFER    = $7F00

ROCKWELL_ACIA   = 1
MHZ_CLOCK       = 2

SR_19200        = $0F
SR_115200       = $00

SR_SELECT       = SR_115200

.if ROCKWELL_ACIA = 1
ZP_SERIAL_SEND_BUSY = $08
.else
SWT_19200_BASE  = 90
SWT_19200       = SWT_19200_BASE * MHZ_CLOCK
SWT_115200      = SWT_19200 / 6

    .if SR_SELECT = SR_19200
SWT_SELECT      = SWT_19200
    .else
SWT_SELECT      = SWT_115200
    .endif
.endif

.struct IO_Port
    Bytes       .byte $10
.endstruct

.struct IO_Port_10_Bytes
    Bytes       .byte 10
.endstruct

.define IO_PORT_BYTE(port, byte) port + IO_Port::Bytes + byte

VIA1            = IO_PORT_0
ACIA            = IO_PORT_1

VIA_PORTB       = IO_PORT_BYTE VIA1, 0
VIA_PORTA       = IO_PORT_BYTE VIA1, 1
VIA_DDRB        = IO_PORT_BYTE VIA1, 2
VIA_DDRA        = IO_PORT_BYTE VIA1, 3

VIA_AUX_CTRL    = IO_PORT_BYTE VIA1, $B
VIA_PER_CTRL    = IO_PORT_BYTE VIA1, $C
VIA_INT_FLAGS   = IO_PORT_BYTE VIA1, $D
VIA_INT_ENABLE  = IO_PORT_BYTE VIA1, $E

ACIA_DATA       = IO_PORT_BYTE ACIA, 0
ACIA_STATUS     = IO_PORT_BYTE ACIA, 1
ACIA_CMD        = IO_PORT_BYTE ACIA, 2
ACIA_CTRL       = IO_PORT_BYTE ACIA, 3

ACIA_STATUS_BIT_IRQ  =  $80
ACIA_STATUS_BIT_DSRB =  $40
ACIA_STATUS_BIT_DCD =   $20
ACIA_STATUS_BIT_TDRE =  $10     ; for WDC 65C51, this is never 1 during transmission
ACIA_STATUS_BIT_RDRF =  $08
ACIA_STATUS_BIT_OVR =   $04
ACIA_STATUS_BIT_FE =    $02
ACIA_STATUS_BIT_PE =    $01

ACIA_CMD_BIT_PME =      $20
ACIA_CMD_BIT_RECHO =    $10
ACIA_CMD_BIT_TLID =     $08
ACIA_CMD_BIT_TLIE =     $04
ACIA_CMD_BIT_RID  =     $02
ACIA_CMD_BIT_RIE  =     $00
ACIA_CMD_BIT_DTRL =     $01

; SPI Defines

IOR_SPI_DATA        = VIA_PORTB
IOR_SPI_DDR         = VIA_DDRB

; SPI DATA BITS
SPI_BIT_CLK     = 1     ; bit 0, so INC/DEC cycle the clock
SPI_BIT_CSB     = 2     ; bit 1
SPI_BIT_MOSI    = 4     ; bit 2
SPI_BIT_CS_1    = 8     ; bit 3
SPI_BIT_CS_2    = $10   ; bit 4
SPI_BIT_CS_4    = $20   ; bit 5
SPI_BIT_CS_8    = $40   ; bit 6
SPI_BIT_MISO    = $80   ; bit 7, so BIT opcode stores MISO in N

SPI_DEV_0       = 0
SPI_DEV_1       = SPI_BIT_CS_1
SPI_DEV_2       = SPI_BIT_CS_2
SPI_DEV_3       = SPI_BIT_CS_2 | SPI_BIT_CS_1
SPI_DEV_4       = SPI_BIT_CS_4
SPI_DEV_5       = SPI_BIT_CS_4 | SPI_BIT_CS_1
SPI_DEV_6       = SPI_BIT_CS_4 | SPI_BIT_CS_2
SPI_DEV_7       = SPI_BIT_CS_4 | SPI_BIT_CS_2 | SPI_BIT_CS_1

SPI_DEV_8       = SPI_BIT_CS_8 | SPI_DEV_0
SPI_DEV_9       = SPI_BIT_CS_8 | SPI_DEV_1
SPI_DEV_A       = SPI_BIT_CS_8 | SPI_DEV_2
SPI_DEV_B       = SPI_BIT_CS_8 | SPI_DEV_3
SPI_DEV_C       = SPI_BIT_CS_8 | SPI_DEV_4
SPI_DEV_D       = SPI_BIT_CS_8 | SPI_DEV_5
SPI_DEV_E       = SPI_BIT_CS_8 | SPI_DEV_6
SPI_DEV_F       = SPI_BIT_CS_8 | SPI_DEV_7

SPI_DDR_BITS    = SPI_BIT_CLK | SPI_BIT_CSB | SPI_BIT_MOSI | SPI_DEV_F

SPI_INIT_DELAY_CYCLES = 80

; max task idle
MAX_TASK_NUMBER = $0F           ; 16 tasks, numbered 0-F
NUM_RAM_MODULES = $01           ; 
NUM_RAM_BANKS   = NUM_RAM_MODULES * 16

; ERROR CODES
ERR_NO_TASKS_AVAILABLE = $F1

; ASCII CODES
ASCII_BACKSPACE = $08
ASCII_LF        = $0A
ASCII_CR        = $0D
ASCII_ESC       = $1B
ASCII_SPACE     = $20
ASCII_BANG      = $21
ASCII_STAR      = $2A
ASCII_PERIOD    = $2E
ASCII_0         = $30
ASCII_COLON     = $3A
ASCII_R         = $52
ASCII_BACKSLASH = $5C

; write a byte in A to the IO PORT
.macro IO_PORT_WRITE    port, byte, imm
.ifnblank       imm
                lda     #imm
.endif
.ifblank        byte
                sta     port
.else
                sta     IO_PORT_BYTE port, byte
.endif
.endmacro

; read a byte into A from the IO PORT/Byte (PORT_N | BYTE_M)
.macro IO_PORT_READ     port, byte
.ifblank        byte
                lda     port
.else
                lda     IO_PORT_BYTE port, byte
.endif
.endmacro


; Register save macros
; Modifies: A (if not in 65C02 mode)
.macro  PUSH_X
.ifpc02
                phx
.else
                sta ZP_A_SAVE
                txa
                pha
                lda ZP_A_SAVE
.endif
.endmacro

.macro  PULL_X
.ifpc02
                plx
.else
                sta ZP_A_SAVE
                pla
                tax
                lda ZP_A_SAVE
.endif
.endmacro

; Register restore macros
; Modifies: A (if not in 65C02 mode)
.macro  PUSH_Y
.ifpc02
                phy
.else
                sta ZP_A_SAVE
                tya
                pha
                lda ZP_A_SAVE
.endif
.endmacro

.macro  PULL_Y
.ifpc02
                ply
.else
                sta ZP_A_SAVE
                pla
                tay
                lda ZP_A_SAVE
.endif
.endmacro

.macro PUSH_AX
.ifpc02
                pha
                phx
.else
                pha
                txa
                pha
.endif
.endmacro

.macro  PULL_XA
.ifpc02
                plx
                pla
.else
                pla
                tax
                pla
.endif
.endmacro

.macro PUSH_AY
.ifpc02
                pha
                phy
.else
                pha
                tya
                pha
.endif
.endmacro

.macro  PULL_YA
.ifpc02
                ply
                pla
.else
                pla
                tay
                pla
.endif
.endmacro

.macro PUSH_XY
.ifpc02
                phx
                phy
.else
                sta ZP_A_SAVE
                txa
                pha
                tya
                pha
                lda ZP_A_SAVE
.endif
.endmacro

.macro  PULL_YX
.ifpc02
                ply
                plx
.else
                sta ZP_A_SAVE
                pla
                tay
                pla
                tax
                lda ZP_A_SAVE
.endif
.endmacro

.macro PUSH_AXY
.ifpc02
                pha
                phx
                phy
.else
                pha
                txa
                pha
                tya
                pha
.endif
.endmacro

.macro  PULL_YXA
.ifpc02
                ply
                plx
                pla
.else
                pla
                tay
                pla
                tax
                pla
.endif
.endmacro

; convenience macros

; MOV
; Modifies: A
.macro MOV              addr1, addr2
                lda     addr1
                sta     addr2
.endmacro

.macro  SJMP            addr
.ifpc02
                bra     addr
.else
                jmp     addr
.endif
.endmacro

.macro  INC16           addr
.local  @no_carry
                inc     addr
                bcc     @no_carry
                inc     addr + 1
@no_carry:
.endmacro

.macro  DEC16           addr
.local  @no_borrow
                cmp     addr
                bne     @no_borrow
                dec     addr + 1
@no_borrow:
                dec     addr
.endmacro

