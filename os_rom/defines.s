.debuginfo

SYSTEM_TASK_NUM     = 0

; ERROR VALUES
ERR_SUCCESS         = $00
ERR_NOT_SYSTEM_TASK = $01
ERR_OUT_OF_MEMORY   = $02

RESET_ENTRY     = $E000

IO_PORT_BASE    = $FF00

; TIMING
CLK_CPS         = 3579545   ; ~3.58 MHz
CLK_CPMS        = (CLK_CPS / 1000) + 1

; ***  ONBOARD SERIAL ADAPTER, 65C51  ***

ROCKWELL_ACIA   = 1
ACIA_USE_VIA_TIMER = 0

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

.if ROCKWELL_ACIA = 0
    .if ACIA_USE_VIA_TIMER = 0
SWT_INNER_LOOP_CYCLES = 5
BITS_PER_CHAR   = 10          ; 8 + start + stop.
SWT             = ((((CLK_CPS / SERIAL_RATE) + 1) * BITS_PER_CHAR) / SWT_INNER_LOOP_CYCLES) + 1
    .else
BITS_PER_CHAR   = 10          ; 8 + start + stop.
HWT_OVERHEAD    = 50
SWT             = (((CLK_CPS / SERIAL_RATE) + 1) * BITS_PER_CHAR) - HWT_OVERHEAD
    .endif

SWT_SELECT_L    = SWT .MOD 256
SWT_SELECT_H    = SWT / 256
.endif

; ***  END OF ONBOARD SERIAL ADAPTER  ***

.struct IO_Port
    Bytes       .byte 16
.endstruct

.struct IO_Port_10_Bytes
    Bytes       .byte 10
.endstruct

.macro HString Str
    .byte       .strlen(Str), Str
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
ASCII_DOT       = ASCII_PERIOD
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
ASCII_B         = 'B'
ASCII_C         = 'C'
ASCII_D         = 'D'
ASCII_E         = 'E'
ASCII_F         = 'F'
ASCII_G         = 'G'
ASCII_H         = 'H'
ASCII_I         = 'I'
ASCII_J         = 'J'
ASCII_K         = 'K'
ASCII_L         = 'L'
ASCII_M         = 'M'
ASCII_N         = 'N'
ASCII_O         = 'O'
ASCII_P         = 'P'
ASCII_Q         = 'Q'
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

; FORTH Defines

F_CELL          := 2
; data stack, 24 cells,
; moves backwards, push decreases before copy
F_DATA_SIZE     := 24 * F_CELL

; return stack, 24 cells, 
; moves backwards, push decreases before copy
F_RETURN_SIZE   := 24 * F_CELL



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
                MOV     addr1, addr2
                MOV     addr1 + 1, addr2 + 1
.endmacro

.macro MOVA16           addr1, addr2
                MOV16   addr1, addr2
.endmacro

.macro MOVX16           addr1, addr2
                MOVX    addr1, addr2
                MOVX    addr1 + 1, addr2 + 1
.endmacro

.macro MOVY16           addr1, addr2
                MOVY    addr1, addr2
                MOVY    addr1 + 1, addr2 + 1
.endmacro

.macro MOVAX            addr1, addr2
                lda     addr1,X
                sta     addr2,X
.endmacro

.macro MOVAY            addr1, addr2
                lda     addr1,Y
                sta     addr2,Y
.endmacro

.macro MOVAX16          addr1, addr2
                MOVAX   addr1, addr2
                inx
                MOVAX   addr1, addr2
.endmacro

.macro MOVAY16          addr1, addr2
                MOVAY   addr1, addr2
                iny
                MOVAY   addr1, addr2
.endmacro

; X: # of bytes to move
; Clobbers A, X
.macro BLKMOVX          addr1, addr2
:
                dex
                lda     addr1,X
                sta     addr2,X
                bne     :-
.endmacro

; Y: # of bytes to move
; Clobbers A, Y
.macro BLKMOVY          addr1, addr2
:
                dey
                lda     addr1,Y
                sta     addr2,Y
                bne     :-
.endmacro

; _M_INCC: inc and set C/V if rollover.  Clobbers .A, C
.macro  _M_INCC    addr
                sec
                lda     #0
                adc     addr
                sta     addr

.macro  _M_INCC16          addr
                inc     addr
                bne     :+
                _M_INCC    addr + 1
                bra     :++
:
                lda     addr + 1
                ora     #1
:
.endmacro

.macro  _M_INCC32          addr
                inc     addr
                bne     :+
                inc     addr+1
                bne     :+
                _M_INCC16  addr+2
.endmacro

.macro  INC16           addr
                inc     addr
                bne     :+
                inc     addr + 1
                bra     :++
:
                lda     addr + 1
                ora     #1
:
.endmacro

.macro  INC32           addr
                inc     addr
                bne     :+
                inc     addr+1
                bne     :+
                INC16   addr+2
.endmacro

.macro  DEC16           addr
                lda     addr
                bne     :+
                dec     addr
                dec     addr+1
                bra     :+++
:
                dec     addr
                bne     :+
                lda     addr+1      ; LOB is zero, so use Z and N from HOB
                bra     :++
:
                lda     addr+1
                ora     #1          ; reset Z, if set, without affecting N
:
.endmacro

.macro  DEC32           addr
                lda     addr
                bne     :+++
                cmp     addr+1
                bne     :++
                cmp     addr+2
                bne     :+
                dec     addr+3
:
                dec     addr+2
:
                dec     addr+1
:
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
                PULL_YA
.endmacro

.macro  NC_XY           MAC, p1, p2
                PUSH_XY
                MAC     p1, p2
                PULL_YX
.endmacro

.macro  NC_AXY          MAC, p1, p2
                PUSH_AXY
                MAC     p1, p2
                PULL_YXA
.endmacro

.macro  DEC16_NC_A      addr
                NC_A    DEC16, addr
.endmacro

.macro SWAP_AX
            pha
            txa
            plx
.endmacro

.macro SWAP_AY
            pha
            tya
            ply
.endmacro

; SPI
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

; PRINT HELPERS
.define LOADA(arg)      lda arg

.macro  LDA_CORA    CharOrAddr
.ifnblank   CharOrAddr
                LOADA           CharOrAddr
.endif
.endmacro

.macro  PRINT_CHAR      C1, C2, C3, C4, C5, C6, C7, C8, C9
    .ifblank C2
                LDA_CORA        {C1}
                jsr             WRITE_CHAR
                .exitmacro
    .else
                lda             C1
                jsr             WRITE_CHAR
    .endif
                PRINT_CHAR      C2, C3, C4, C5, C6, C7, C8, C9
.endmacro

.macro  PRINT_CHAR_JMP  C1, C2, C3, C4, C5, C6, C7, C8, C9
    .ifblank    C2
                LDA_CORA        {C1}
                jmp             WRITE_CHAR
                .exitmacro
    .else
                lda             C1
                jsr             WRITE_CHAR
    .endif
                PRINT_CHAR_JMP  C2, C3, C4, C5, C6, C7, C8, C9
.endmacro

.macro  PRINT_SPACE
                PRINT_CHAR      #ASCII_SPACE
.endmacro

.macro  PRINT_ESC_SEQ   C1, C2, C3, C4, C5, C6, C7, C8
                PRINT_CHAR      #ASCII_ESC, C1, C2, C3, C4, C5, C6, C7, C8
.endmacro

.macro  PRINT_ESC_SEQ_JMP   C1, C2, C3, C4, C5, C6, C7, C8
                PRINT_CHAR_JMP  #ASCII_ESC, C1, C2, C3, C4, C5, C6, C7, C8
.endmacro

.macro  PRINT_BYTE      CharOrAddr
                LDA_CORA        {CharOrAddr}
                jsr             WRITE_BYTE
.endmacro

.macro  PRINT_BYTE_JMP  CharOrAddr
                LDA_CORA        {CharOrAddr}
                jmp             WRITE_BYTE
.endmacro

.macro  PRINT_HEX       CharOrAddr
                LDA_CORA        {CharOrAddr}
                jsr             WRITE_HEX
.endmacro

.macro  PRINT_HEX_MASK  CharOrAddr
                LDA_CORA        {CharOrAddr}
                jsr             WRITE_HEX_MASK
.endmacro

.macro  PRINT_CRLF
                jsr             WRITE_CRLF
.endmacro

.macro  PRINT_CRLF_JMP
                jmp             WRITE_CRLF
.endmacro

.macro  _M_WRITE_HSTRING        addr
                lda             #<addr
                ldy             #>addr
                jsr             WRITE_HSTRING
.endmacro

; JSR using JMP
.macro  _M_JSRR                 addrTo, addrFrom
                lda             #>(addrFrom-1)
                pha
                lda             #<(addrFrom-1)
                pha
                jmp             (addrTo)
.endmacro

.macro _M_JSRR_NC_A             addrFrom, addrTo
                NC_A            _M_JSRR, addrFrom, addrTo
.endmacro

.macro SL_N     n
    .if     n > 0
                asl
                SL_N    n-1
    .endif
.endmacro

.macro SR_N     n
    .if     n > 0
                lsr
                SR_N    n-1
    .endif
.endmacro

.macro SKIPNEXT
    .byte   $22     ; Undocumented 2-byte NOP, 2 cycles; uses 1 byte to skip the next byte
.endmacro

.macro SKIPNEXT2
    .byte   $DC     ; Undocumented 3-byte NOP, 4 cycles, reads absolute address IP+1, IP+2; uses 1 byte to skip two bytes
.endmacro

.macro MEMCP addrFrom, addrTo, size
            PUSH_AY
            lda         #<addrFrom
            sta         ZP_TEMP_VEC
            lda         #>addrFrom
            sta         ZP_TEMP_VEC+1
            lda         #<addrTo
            sta         ZP_TEMP_VEC2
            lda         #>addrTo
            sta         ZP_TEMP_VEC2+1
            lda         #<size
            ldy         #>size
            jsr         MEM_COPY
            PULL_YA
.endmacro
