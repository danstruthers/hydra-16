; ****************************************************************************
; hw.inc - the Hydra-16 V1 board, as the software sees it: the pseudo-registers, the bank registers, the I/O
; ports and their chips, the IRQ lines, the clock.  Facts only: no OS layout here (that's kernel.inc).
; Source: docs/hardware.md.

; ---- The CPU clock: a build-time setting on V1 (no clock register).  1 = 3.58 MHz (jumper J7), 2 = 7.16 MHz
.ifndef CPU_CLOCK_MULT
CPU_CLOCK_MULT  = 1
.endif
.assert     CPU_CLOCK_MULT = 1 .or CPU_CLOCK_MULT = 2, error, "CPU_CLOCK_MULT: 1 (3.58 MHz) or 2 (7.16 MHz)"
CLK_CPS         = 3579545 * CPU_CLOCK_MULT      ; CPU cycles a second

; ---- The pseudo-registers (system port $FFF0-$FFF3): 8-bit latches, read back whole.  No reset: the ROM sets them
T_REGISTER      = $FFF0         ; The task: T0-T3 select $0000-$7FFF (zero page and stack included) and the bank registers
U_REGISTER      = $FFF1         ; The shared macro-page: U0-U3 (shared bank IDs U << 4 | bank & $0F)
V_REGISTER      = $FFF2         ; The vector RAM's index when no IRQ line is active (BRK's vector, and writes)
W_REGISTER      = $FFF3         ; The BIOS ROM page at $E000-$FEFF (W0-W3 with a 128K chip)

; ---- Each task's bank registers: written at $00 and $01 (the hardware), read back from task RAM (a mirror)
RAM_BANK        = $00           ; The bank at $8000-$9FFF: $00-$EF the task's own (module m: $m0-$mF), $F0-$FF shared (with U)
ROM_BANK        = $01           ; The paged ROM bank at $A000-$DFFF (16K)

; ---- Memory windows
BANK_WINDOW     = $8000         ; $8000-$9FFF: the selected RAM bank
BANK_SIZE       = $2000
PROM_WINDOW     = $A000         ; $A000-$DFFF: the selected paged ROM bank
PROM_BANK_SIZE  = $4000
BIOS_BASE       = $E000         ; $E000-$FEFF: BIOS ROM page W
SHARED_BANK     = $F0           ; RAM_BANK values $F0-$FF: shared RAM, macro-page U
RAM_MODULES     = 15            ; Task RAM modules on daughter cards: 0-14 (banks $m0-$mF)

; ---- The vector RAM ($FFFE/$FFFF): 16 entries; the CPU reads the active line's (index = line XOR 7), or V0-V3's
VECTOR_RAM      = $FFFE
.define IRQ_INDEX(line)  ((line) ^ 7)

; ---- IRQ lines (0 has the highest priority)
LINE_VIA        = 0             ; VIA: timer 1, timer 2, CA1
LINE_ACIA       = 1             ; ACIA: the serial port
LINE_SLOT0A     = 2             ; Slot 0 (the Vera X: the VERA)
LINE_SLOT0B     = 3             ; Slot 0 (the Vera X: its input controller)
LINE_YM         = 4             ; YM2151: its timers
LINE_SLOT1A     = 5             ; Slots 1-5, select A: lines 5-9; select B: lines 10-14
LINE_SLOT1B     = 10
LINE_NONE       = 15            ; Not connected: its vector entry is BRK's (V0-V3 = IRQ_INDEX(15))
LINES           = 16

; ---- VIA, 65C22 (port 0, $FF00): timer 1 is the scheduler's tick; port B the SPI bus; port A GPIO and I2C
VIA_PORTB       = $FF00
VIA_PORTA       = $FF01
VIA_DDRB        = $FF02
VIA_DDRA        = $FF03
VIA_T1CL        = $FF04         ; (Read: clears T1's flag)
VIA_T1CH        = $FF05         ; (Write: loads the counter from the latch and starts it)
VIA_T1LL        = $FF06
VIA_T1LH        = $FF07
VIA_T2CL        = $FF08
VIA_T2CH        = $FF09
VIA_SR          = $FF0A
VIA_ACR         = $FF0B
VIA_PCR         = $FF0C
VIA_IFR         = $FF0D
VIA_IER         = $FF0E
VIA_PORTA_NH    = $FF0F         ; Port A without the handshake
VIA_ACR_T1_FREE = $40           ; ACR: timer 1 free-running
VIA_IRQ_T1      = $40           ; IFR / IER bits
VIA_IRQ_T2      = $20
VIA_IRQ_CA1     = $02
VIA_IER_SET     = $80           ; IER: set (1) or clear (0) the bits given

; ---- SPI, bit-banged on the VIA's port B.  Devices 0-7 are the headers (a 74HC138: /nSPI_CS0-7), 8-15 the slots'
SPI_SCLK        = $01           ; PB0: the clock (bit 0, so INC and DEC clock it)
SPI_CSB         = $02           ; PB1: /CS enable (low: the device on PB3-PB6 selected)
SPI_MOSI        = $04           ; PB2
SPI_DEV         = $78           ; PB3-PB5: device 0-7; PB6: 1 for devices 8-15
SPI_MISO        = $80           ; PB7 (an input; bit 7, so BIT puts it in N)
SPI_DDR         = $7F           ; Port B's directions: all out but MISO

; ---- ACIA, 65C51 (port 1, $FF10): the serial port.  Its clock is 1.790 MHz (V1 erratum): every rate is 2.9% slow
ACIA_DATA       = $FF10
ACIA_STATUS     = $FF11         ; (Write: a programmed reset)
ACIA_CMD        = $FF12
ACIA_CTRL       = $FF13
ACIA_ST_IRQ     = $80
ACIA_ST_TDRE    = $10           ; Transmitter empty (the WDC W65C51N's always reads 1)
ACIA_ST_RDRF    = $08           ; A byte received
ACIA_ST_OVR     = $04
ACIA_CTRL_BRG   = $10           ; Control: the receiver's clock is the baud rate generator ...
ACIA_RATE_115200 = $00          ;   and the rates (bits 0-3; 0: the clock / 16, "115200")
ACIA_RATE_9600  = $0E
ACIA_RATE_19200 = $0F
ACIA_CTRL_8N1   = $00           ;   8 data bits, 1 stop bit (bits 5-7)
ACIA_CMD_DTR    = $01           ; Command: the chip on (DTR) ...
ACIA_CMD_NO_RXIRQ = $02         ;   no receive interrupt ...
ACIA_CMD_TX_ON  = $08           ;   transmitter on, RTS low, no transmit interrupt
ACIA_CMD_TX_IRQ = $04           ;   transmitter on, RTS low, its interrupt on (Rockwell only)
ACIA_ROCKWELL   = 0             ; The chip (a build option): Rockwell R65C51, or WDC W65C51N (TDRE always set)
ACIA_WDC        = 1
.ifndef ACIA_CHIP
ACIA_CHIP       = ACIA_ROCKWELL
.endif

; ---- YM2151 (port 4, $FF40)
YM_REG          = $FF40         ; Write: select a register
YM_DATA         = $FF41         ; Write: the selected register; read: the status (bit 7 busy, bits 0-1 the timers'
                                ;   flags), as the old system's driver read it on the board (docs/plans/SOUND.md)

; ---- The DS1747 clock in U7 (if fitted): its registers are task F's $7FF8-$7FFF
RTC_TASK        = $0F
RTC_REGS        = $7FF8         ; Control (the century, BCD; W, R), seconds (OSC: bit 7), minutes, hours, day (BF: bit 7;
                                ;   FT: bit 6), date, month, year: BCD
RTC_W           = $80           ; Control: W, updates halted for a write (clearing it sets the clock from them)
RTC_R           = $40           ; Control: R, updates halted for a read
