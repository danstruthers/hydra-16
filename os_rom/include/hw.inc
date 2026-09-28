.debuginfo

; ****************************************************************************
; Hardware: the clock, the IO ports and the chips' registers (VIA, ACIA, YM2151), IRQ numbers, SPI,
; and the T / U / V / W pseudo-registers

RESET_ENTRY     = $E000

IO_PORT_BASE    = $FF00

; TIMING
; CPU clock: a build-time setting for now (board V2 will have a clock jumper register to read).  Timing
; constants are derived from it.  Above 3.58 MHz the YM2151 (on its own 3.58 MHz clock) needs bus wait
; states, which need the V2 RDY hold hardware (see IO_PLAN.md), so don't use the sound chip until then.
CPU_CLOCK_MULT  = 1         ; 1 = 3.58 MHz, 2 = 7.16 MHz
.assert     CPU_CLOCK_MULT = 1 .or CPU_CLOCK_MULT = 2, error, "CPU_CLOCK_MULT must be 1 (3.58 MHz) or 2 (7.16 MHz)"
CLK_BASE_CPS    = 3579545   ; ~3.58 MHz (the 14.318 MHz crystal / 4)
CLK_CPS         = CLK_BASE_CPS * CPU_CLOCK_MULT
CLK_CPMS        = (CLK_CPS / 1000) + 1

; ***  ONBOARD SERIAL ADAPTER, 65C51  ***

SR_2400         = $0A
SR_4800         = $0C
SR_9600         = $0E
SR_19200        = $0F
SR_115200       = $00

SR_SELECT       = SR_19200

.if SR_SELECT = SR_2400
SERIAL_RATE     = 2400
.elseif SR_SELECT = SR_4800
SERIAL_RATE     = 4800
.elseif SR_SELECT = SR_9600
SERIAL_RATE     = 9600
.elseif SR_SELECT = SR_19200
SERIAL_RATE     = 19200
.else
SERIAL_RATE     = 115200
.endif

; The ACIA chip (a build option).  The Rockwell R65C51 sends from its transmitter-empty (TDRE) interrupt.
; The WDC W65C51N's TDRE status bit and interrupt don't work (a known chip bug), so its sending is paced
; by the VIA's timer 2 instead: a one-shot of one character's time (10 bits, and a margin) per byte, whose
; interrupt sends the next.  Receiving is the same on both.  (VIA T2 is the serial driver's then.)
SER_ACIA_ROCKWELL = 0
SER_ACIA_WDC    = 1
SER_ACIA        = SER_ACIA_ROCKWELL
SER_CHAR_CYCLES = CLK_CPS * 11 / SERIAL_RATE                ; (WDC: the timer 2 count per character)
.assert     SER_CHAR_CYCLES < $10000, error, "SER_CHAR_CYCLES must fit VIA timer 2: a faster serial rate"

; ***  END OF ONBOARD SERIAL ADAPTER  ***

.struct IO_Port
    Bytes       .byte 16
.endstruct

.struct IO_Port_10_Bytes
    Bytes       .byte 10
.endstruct

.define IO_PORT_BYTE(port, byte)    port + IO_Port::Bytes + byte

VIA1            = IO_PORT_0
ACIA            = IO_PORT_1
YM_SOUND        = IO_PORT_4

VIA_R_PORTB         = IO_PORT_BYTE VIA1, 0
VIA_R_PORTA         = IO_PORT_BYTE VIA1, 1
VIA_R_PORTA_NOHS    = IO_PORT_BYTE VIA1, $F
VIA_R_DDRB          = IO_PORT_BYTE VIA1, 2
VIA_R_DDRA          = IO_PORT_BYTE VIA1, 3

VIA_R_T1C_L         = IO_PORT_BYTE VIA1, 4
VIA_R_T1C_H         = IO_PORT_BYTE VIA1, 5
VIA_R_T1L_L         = IO_PORT_BYTE VIA1, 6
VIA_R_T1L_H         = IO_PORT_BYTE VIA1, 7

VIA_R_T2C_L         = IO_PORT_BYTE VIA1, 8
VIA_R_T2C_H         = IO_PORT_BYTE VIA1, 9

VIA_R_SHIFT_REG     = IO_PORT_BYTE VIA1, $A
VIA_R_AUX_CTRL      = IO_PORT_BYTE VIA1, $B
VIA_R_PER_CTRL      = IO_PORT_BYTE VIA1, $C
VIA_R_INT_FLAGS     = IO_PORT_BYTE VIA1, $D
VIA_R_INT_ENABLE    = IO_PORT_BYTE VIA1, $E

ACIA_R_DATA         = IO_PORT_BYTE ACIA, 0
ACIA_R_STATUS       = IO_PORT_BYTE ACIA, 1
ACIA_R_CMD          = IO_PORT_BYTE ACIA, 2
ACIA_R_CTRL         = IO_PORT_BYTE ACIA, 3

VIA_T1_INT_BIT      = $40       ; VIA_R_INT_FLAGS / VIA_R_INT_ENABLE: timer 1 (the scheduler's tick)
VIA_T2_INT_BIT      = $20       ;   timer 2 (the WDC ACIA's TX pacing)
VIA_INT_ENABLE      = $80       ; VIA_R_INT_ENABLE: set (1) or clear (0) the bits given

; Send .A to the ACIA (the serial task, IRQs off, the transmitter idle or its last byte just sent), and
; for the WDC 65C51 start timer 2, which stands in for its missing TDRE interrupt (SER_ACIA); and a BEL beeps on the YM2151.
; Uses .Y
.macro _M_SER_TX_BYTE
            sta         ACIA_R_DATA
.if ::SER_ACIA = ::SER_ACIA_WDC                             ; (:: so it's constant in the page scopes too)
            ldy         #<SER_CHAR_CYCLES
            sty         VIA_R_T2C_L
            ldy         #>SER_CHAR_CYCLES
            sty         VIA_R_T2C_H                 ; (Starts the one-shot, and clears its IRQ)
.endif
            cmp         #ASCII_BELL                 ; The bell: the YM2151 beeps too, as the terminal's
            bne         * + 5                       ;   does (YM_BEEP; no label here: it would end the
            jsr         YM_BEEP                     ;   caller's @ labels)
.endmacro

.define  IRQ_NUMBER(num)    (num ^ 7)

; IRQs, from highest priority (0) to lowest (15)
IRQ_NUMBER_HIGHEST_PRI = IRQ_NUMBER(0)
IRQ_NUMBER_ONBOARD_VIA = IRQ_NUMBER(0)         ; System timers, etc
IRQ_NUMBER_ONBOARD_SERIAL = IRQ_NUMBER(1)      ; On-board serial

IRQ_NUMBER_SLOT_0_L = IRQ_NUMBER(2)
IRQ_NUMBER_SLOT_0_H = IRQ_NUMBER(3)

IRQ_NUMBER_ONBOARD_SOUND = IRQ_NUMBER(4)       ; YM-2151

; SLOT-assigned IRQs, low (higher-priority)
IRQ_NUMBER_SLOT_1_L = IRQ_NUMBER(5)
IRQ_NUMBER_SLOT_2_L = IRQ_NUMBER(6)
IRQ_NUMBER_SLOT_3_L = IRQ_NUMBER(7)
IRQ_NUMBER_SLOT_4_L = IRQ_NUMBER(8)
IRQ_NUMBER_SLOT_5_L = IRQ_NUMBER(9)

; SLOT-assigned IRQs, high (lower-priority)
IRQ_NUMBER_SLOT_1_H = IRQ_NUMBER(10)
IRQ_NUMBER_SLOT_2_H = IRQ_NUMBER(11)
IRQ_NUMBER_SLOT_3_H = IRQ_NUMBER(12)
IRQ_NUMBER_SLOT_4_H = IRQ_NUMBER(13)
IRQ_NUMBER_SLOT_5_H = IRQ_NUMBER(14)

IRQ_NUMBER_15 = IRQ_NUMBER(15)                   ; not assigned to any hardware or slot
IRQ_NUMBER_LOWEST_PRI = IRQ_NUMBER_15
IRQ_NUMBER_SW = IRQ_NUMBER_LOWEST_PRI

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

IOR_SPI_DATA        = VIA_R_PORTB
IOR_SPI_DDR         = VIA_R_DDRB

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

T_REGISTER = $FFF0 ; IO_PORT_BYTE IO_PORT_F, 0
U_REGISTER = $FFF1 ; IO_PORT_BYTE IO_PORT_F, 1
V_REGISTER = $FFF2 ; IO_PORT_BYTE IO_PORT_F, 2
W_REGISTER = $FFF3 ; IO_PORT_BYTE IO_PORT_F, 3
