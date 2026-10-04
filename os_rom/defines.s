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
    Bytes       .byte 16
.endstruct

.struct IO_Port_10_Bytes
    Bytes       .byte 10
.endstruct

.macro HString Str
    .byte .strlen(Str), Str
.endmacro

;.struct SerialInfo
;    .byte   IOPort_IRQ
;    .byte   Read_Ptr_L
;    .byte   Read_Ptr_H
;    .byte   Write_Ptr_L
;    .byte   Write_Ptr_H
;.endstruct

;.macro SERIAL_INFO port, irq, task, buff_addr
;.endmacro

;.macro Serial_Buffer_Advance info
;.endmacro

.define IO_PORT_BYTE(port, byte) port + IO_Port::Bytes + byte

VIA1            = IO_PORT_0
ACIA            = IO_PORT_1
YM_SOUND        = IO_PORT_4

VIA_PORTB       = IO_PORT_BYTE VIA1, 0
VIA_PORTA       = IO_PORT_BYTE VIA1, 1
VIA_PORTA_NOHS  = IO_PORT_BYTE VIA1, $F
VIA_DDRB        = IO_PORT_BYTE VIA1, 2
VIA_DDRA        = IO_PORT_BYTE VIA1, 3

VIA_T1C_L       = IO_PORT_BYTE VIA1, 4
VIA_T1C_H       = IO_PORT_BYTE VIA1, 5
VIA_T1L_L       = IO_PORT_BYTE VIA1, 6
VIA_T1L_H       = IO_PORT_BYTE VIA1, 7

VIA_T2C_L       = IO_PORT_BYTE VIA1, 8
VIA_T2C_H       = IO_PORT_BYTE VIA1, 9

VIA_SHIFT_REG   = IO_PORT_BYTE VIA1, $A
VIA_AUX_CTRL    = IO_PORT_BYTE VIA1, $B
VIA_PER_CTRL    = IO_PORT_BYTE VIA1, $C
VIA_INT_FLAGS   = IO_PORT_BYTE VIA1, $D
VIA_INT_ENABLE  = IO_PORT_BYTE VIA1, $E

ACIA_DATA       = IO_PORT_BYTE ACIA, 0
ACIA_STATUS     = IO_PORT_BYTE ACIA, 1
ACIA_CMD        = IO_PORT_BYTE ACIA, 2
ACIA_CTRL       = IO_PORT_BYTE ACIA, 3

; IRQs, from highest priority (0) to lowest (15)
IRQ_NUMBER_HIGHEST_PRI = 0
IRQ_NUMBER_ONBOARD_VIA = 0         ; System timers, etc
IRQ_NUMBER_ONBOARD_SERIAL = 1      ; On-board serial

IRQ_NUMBER_SLOT_0_L = 2
IRQ_NUMBER_SLOT_0_H = 3

IRQ_NUMBER_ONBOARD_SOUND = 4       ; YM-2151

; SLOT-assigned IRQs, low (higher-priority)
IRQ_NUMBER_SLOT_1_L = 5
IRQ_NUMBER_SLOT_2_L = 6
IRQ_NUMBER_SLOT_3_L = 7
IRQ_NUMBER_SLOT_4_L = 8
IRQ_NUMBER_SLOT_5_L = 9

; SLOT-assigned IRQs, high (lower-priority)
IRQ_NUMBER_SLOT_1_H = 10
IRQ_NUMBER_SLOT_2_H = 11
IRQ_NUMBER_SLOT_3_H = 12
IRQ_NUMBER_SLOT_4_H = 13
IRQ_NUMBER_SLOT_5_H = 14

IRQ_NUMBER_15 = 15                  ; not assigned to any hardware or slot
IRQ_NUMBER_LOWEST_PRI = 15

YM_REG          = IO_PORT_BYTE YM_SOUND, 0
YM_DATA         = IO_PORT_BYTE YM_SOUND, 1

ACIA_STATUS_BIT_IRQ  =  $80
ACIA_STATUS_BIT_DSRB =  $40
ACIA_STATUS_BIT_DCD =   $20
ACIA_STATUS_BIT_TDRE =  $10         ; for WDC 65C51, this is never 1 during transmission
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
NUM_RAM_MODULES = 15            ;
NUM_BANKS_PER_MODULE = 16
NUM_RAM_BANKS   = NUM_RAM_MODULES * NUM_BANKS_PER_MODULE

T_REGISTER = $FFF0 ; IO_PORT_BYTE IO_PORT_F, 0
U_REGISTER = $FFF1 ; IO_PORT_BYTE IO_PORT_F, 1
V_REGISTER = $FFF2 ; IO_PORT_BYTE IO_PORT_F, 2
W_REGISTER = $FFF3 ; IO_PORT_BYTE IO_PORT_F, 3

; ERROR CODES
ERR_NO_TASKS_AVAILABLE = $F1

; TIMING
CLK_CPS = 3579545   ; ~3.58 MHz
CLK_CPMS = CLK_CPS/1000

; Task switcher interrupt timer (one interrupt per 5ms or so, with 64 cycles for INT Handler overhead)
TIMER_TASK_INT_H = 69
TIMER_TASK_INT_L = 169

; ASCII CODES
ASCII_BACKSPACE = $08
ASCII_LF        = $0A
ASCII_CR        = $0D
ASCII_ESC       = $1B
ASCII_SPACE     = ' '
ASCII_BANG      = '!'
ASCII_DQUOTE    = '"'
ASCII_HASH      = '#'
ASCII_DOLLAR    = '$'
ASCII_PERCENT   = '%'
ASCII_CARET     = '^'
ASCII_AMP       = '&'
ASCII_SQOUTE    = '''
ASCII_LPAREN    = '('
ASCII_RPAREN    = ')'
ASCII_STAR      = '*'
ASCII_PLUS      = '+'
ASCII_COMMA     = ','
ASCII_MINUS     = '-'
ASCII_DASH      = ASCII_MINUS
ASCII_HYPHEN    = ASCII_MINUS
ASCII_PERIOD    = '.'
ASCII_SLASH     = '/'
ASCII_0         = '0'
ASCII_1         = '1'
ASCII_2         = '2'
ASCII_3         = '3'
ASCII_4         = '4'
ASCII_5         = '5'
ASCII_6         = '6'
ASCII_7         = '7'
ASCII_8         = '8'
ASCII_9         = '9'
ASCII_COLON     = ':'
ASCII_SEMI      = ':'
ASCII_LT        = '<'
ASCII_EQ        = '='
ASCII_GT        = '>'
ASCII_QUESTION  = '?'
ASCII_A         = 'A'
ASCII_J         = 'J'
ASCII_R         = 'R'
ASCII_S         = 'S'
ASCII_T         = 'T'
ASCII_U         = 'U'
ASCII_V         = 'V'
ASCII_W         = 'W'
ASCII_X         = 'X'
ASCII_Y         = 'Y'
ASCII_Z         = 'Z'
ASCII_f         = 'f'
ASCII_LBRACKET  = '['
ASCII_BACKSLASH = '\'
ASCII_RBRACKET  = ']'
ASCII_LBRACE    = '{'
ASCII_RBRACE    = '}'

ASCII_LETTER_OFFSET = ASCII_A-ASCII_0-10

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

.macro PUSH_AX
                pha
                phx
.endmacro

.macro  PULL_XA
                plx
                pla
.endmacro

.macro PUSH_AY
                pha
                phy
.endmacro

.macro  PULL_YA
                ply
                pla
.endmacro

.macro PUSH_XY
                phx
                phy
.endmacro

.macro  PULL_YX
                ply
                plx
.endmacro

.macro PUSH_AXY
                pha
                phx
                phy
.endmacro

.macro  PULL_YXA
                ply
                plx
                pla
.endmacro

; convenience macros

; MOV
; Modifies: A
.macro MOV              addr1, addr2
                lda     addr1
                sta     addr2
.endmacro

.macro MOVA             addr1, addr2
                MOV     addr1, addr2
.endmacro

.macro MOVX             addr1, addr2
                ldx     addr1
                stx     addr2
.endmacro

.macro MOVY             addr1, addr2
                ldy     addr1
                sty     addr2
.endmacro

.macro MOV16            addr1, addr2
                lda     addr1
                sta     addr2
                lda     addr1+1
                sta     addr2+1
.endmacro

.macro MOVA16           addr1, addr2
                MOV16   addr1, addr2
.endmacro

.macro MOVX16           addr1, addr2
                ldx     addr1
                stx     addr2
                ldx     addr1+1
                stx     addr2+1
.endmacro

.macro MOVY16           addr1, addr2
                ldy     addr1
                sty     addr2
                ldy     addr1+1
                sty     addr2+1
.endmacro

.macro MOVAX            addr1, addr2
                lda     addr1,x
                sta     addr2,x
.endmacro

.macro MOVAY            addr1, addr2
                lda     addr1,y
                sta     addr2,y
.endmacro

.macro MOVAY16          addr1, addr2
                lda     addr1,y
                sta     addr2,y
                lda     addr1+1,y
                sta     addr2+1,y
.endmacro

; X: # of bytes to move
; Clobbers A, X
.macro BLKMOVX          addr1, addr2
@:
                dex
                lda     addr1,x
                sta     addr2,x
                bne     @-
.endmacro

; Y: # of bytes to move
; Clobbers A, Y
.macro BLKMOVY          addr1, addr2
@:
                dey
                lda     addr1,y
                sta     addr2,y
                bne     @-
.endmacro


.macro  INC16           addr
                inc     addr
                bne     @+
                inc     addr+1
                bra     @++
@:
                lda     addr+1
@:
.endmacro

.macro  INC32           addr
                inc     addr
                bne     @+
                inc     addr+1
                bne     @+
                INC16   addr+2
.endmacro

.macro  DEC16           addr
                lda     addr
                bne     @+
                dec     addr
                dec     addr+1
                bra     @+++
@:
                dec     addr
                bne     @+      ; if Z not set, don't take Z from HOB
                lda     addr+1  ; sets Z and N from HOB
                bra     @++
@:
                lda     addr+1
                ora     #1      ; reset Z, if set, without affecting N
@:
.endmacro

.macro  DEC32           addr
                lda     addr
                bne     @+++
                cmp     addr+1
                bne     @++
                cmp     addr+2
                bne     @+
                dec     addr+3
@:
                dec     addr+2
@:
                dec     addr+1
@:
                dec     addr

.endmacro

; No-clobber (NC) macros to wrap another macro that overwrites one or more registers
.macro  NC_A            MAC, p1, p2
                pha
                MAC     p1, p2
                pla
.endmacro

.macro  NC_X            MAC, p1, p2
                phx
                MAC     p1, p2
                plx
.endmacro

.macro  NC_Y            MAC, p1, p2
                phy
                MAC     p1, p2
                ply
.endmacro

.macro  NC_AX           MAC, p1, p2
                PUSH_AX
                MAC     p1, p2
                PULL_XA
.endmacro

.macro  NC_AY           MAC, p1, p2
                PUSH_AY
                MAC     p1, p2
                plyA
.endmacro

.macro  NC_XY           MAC, p1, p2
                PUSH_XY
                MAC     p1, p2
                plyX
.endmacro

.macro  NC_AXY          MAC, p1, p2
                PUSH_AXY
                MAC     p1, p2
                plyXA
.endmacro

.macro  DEC16_NC_A      addr
                NC_A    DEC16, addr
.endmacro
