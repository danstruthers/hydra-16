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
PROM_SOCKET_BANKS = 32          ; A paged ROM socket's banks (a 512K chip; 8 sockets).  The V1 board swaps ROM_BANK's
                                ;   bits 6 and 7 (and 2 and 3) on their way to the sockets: socket k holds the banks
                                ;   whose numbers, bits 6 and 7 swapped, are 32k to 32k + 31 (socket 2: 128-159)
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
                                ;   flags), as the old system's driver read it on the board (docs/design/plans/SOUND.md)

; ---- The Vera X in slot 0 (ports 2 and 3, $FF20-$FF3F; IRQ line 2): the VERA, its register n at VERA_BASE + n.
;   The X16's names (its io.inc), so X16 sources port with a change of base.  Its gateware is the X16 community's
;   (v47 on: DCSEL is 6 bits, its set 63 the version); the programmer's reference is the X16's, chapter 9
VERA_BASE       = $FF20
VERA_ADDR_L     = VERA_BASE + $00   ; ADDRx (x: CTRL's ADDRSEL): VRAM address bits 0-7 ...
VERA_ADDR_M     = VERA_BASE + $01   ;   8-15 ...
VERA_ADDR_H     = VERA_BASE + $02   ;   16 (bit 0); the increment (bits 4-7: VERA_INC_*), DECR (bit 3)
VERA_DATA0      = VERA_BASE + $03   ; VRAM at ADDR0, then ADDR0 steps ...
VERA_DATA1      = VERA_BASE + $04   ;   and at ADDR1
VERA_CTRL       = VERA_BASE + $05   ; Reset (bit 7: the FPGA configures itself again), DCSEL (bits 1-6), ADDRSEL (bit 0)
VERA_IEN        = VERA_BASE + $06   ; The interrupts on (VERA_IRQ_*); bit 7 IRQ_LINE's bit 8; bit 6 (read) SCANLINE's
VERA_ISR        = VERA_BASE + $07   ; Those that came (a 1 written clears VSYNC, LINE, SPRCOL); bits 4-7 the collisions
VERA_IRQ_LINE_L = VERA_BASE + $08   ; Write: the LINE interrupt's line (bits 0-7); read: SCANLINE (bits 0-7)
VERA_SCANLINE_L = VERA_BASE + $08
VERA_DC_VIDEO   = VERA_BASE + $09   ; DCSEL 0: the field (bit 7), sprites, layer 1, layer 0 on (6-4), 240P, chroma
VERA_DC_HSCALE  = VERA_BASE + $0A   ;   off / HV sync, the output (bits 0-1: 0 off, 1 VGA); the scales (128: 1 to 1) ...
VERA_DC_VSCALE  = VERA_BASE + $0B
VERA_DC_BORDER  = VERA_BASE + $0C   ;   the border's colour
VERA_DC_HSTART  = VERA_BASE + $09   ; DCSEL 1: the active area (H: bits 9-2; V: bits 8-1)
VERA_DC_HSTOP   = VERA_BASE + $0A
VERA_DC_VSTART  = VERA_BASE + $0B
VERA_DC_VSTOP   = VERA_BASE + $0C
VERA_DC_VER0    = VERA_BASE + $09   ; DCSEL 63: "V" (VERA_VER_ID), then the version's major, minor and build numbers
VERA_DC_VER1    = VERA_BASE + $0A
VERA_DC_VER2    = VERA_BASE + $0B
VERA_DC_VER3    = VERA_BASE + $0C
VERA_L0_CONFIG  = VERA_BASE + $0D   ; Layer 0: map height, width (bits 7-6, 5-4), T256C, bitmap, colour depth (bits 1-0)
VERA_L0_MAPBASE = VERA_BASE + $0E   ;   the map's address (bits 16-9) ...
VERA_L0_TILEBASE = VERA_BASE + $0F  ;   the tiles' (bits 16-11: bits 7-2), tile height, width (bits 1, 0) ...
VERA_L0_HSCROLL_L = VERA_BASE + $10 ;   the scrolls (12 bits)
VERA_L0_HSCROLL_H = VERA_BASE + $11
VERA_L0_VSCROLL_L = VERA_BASE + $12
VERA_L0_VSCROLL_H = VERA_BASE + $13
VERA_L1_CONFIG  = VERA_BASE + $14   ; Layer 1: the same
VERA_L1_MAPBASE = VERA_BASE + $15
VERA_L1_TILEBASE = VERA_BASE + $16
VERA_L1_HSCROLL_L = VERA_BASE + $17
VERA_L1_HSCROLL_H = VERA_BASE + $18
VERA_L1_VSCROLL_L = VERA_BASE + $19
VERA_L1_VSCROLL_H = VERA_BASE + $1A
VERA_AUDIO_CTRL = VERA_BASE + $1B   ; PCM: FIFO full (read) / reset (write), empty, 16-bit, stereo, volume (bits 3-0)
VERA_AUDIO_RATE = VERA_BASE + $1C   ;   the rate (128: 48828 Hz) ...
VERA_AUDIO_DATA = VERA_BASE + $1D   ;   the FIFO
VERA_SPI_DATA   = VERA_BASE + $1E   ; The SD card's SPI: a byte ...
VERA_SPI_CTRL   = VERA_BASE + $1F   ;   busy (bit 7), auto-TX, slow clock, select (bit 0)
VERA_CTRL_RESET = $80
VERA_CTRL_ADDRSEL = $01
VERA_CTRL_DCSEL = $7E           ; (DCSEL n: n << 1)
VERA_DCSEL_VER  = 63 << 1       ; CTRL's DCSEL for the version
VERA_DCSEL_FX   = 2 << 1        ; CTRL's DCSELs for FX (v47 on: the X16's Programmer's Reference, chapter 10): 2 ...
VERA_FX_CTRL    = VERA_BASE + $09   ;   FX_CTRL: transparent writes (7), cache write (6), cache fill (5), byte
                                    ;   cycling (4), 16-bit hop (3), 4-bit mode (2), ADDR1's mode (1-0) ...
VERA_FX_TILEBASE = VERA_BASE + $0A  ;   the affine tiles' base (7-2), clip (1), 2-bit polygons (0) ...
VERA_FX_MAPBASE = VERA_BASE + $0B   ;   the affine map's base (7-2), size (1-0) ...
VERA_FX_MULT    = VERA_BASE + $0C   ;   reset (7) and accumulate (6), subtract (5), the multiplier on (4), the
                                    ;   cache's byte (3-2), nibble (1), increment mode (0)
VERA_DCSEL_FX_INCR = 3 << 1     ;   3: the X and Y increments (6.9, signed; x 32 with bit 15) ...
VERA_FX_X_INCR_L = VERA_BASE + $09
VERA_FX_X_INCR_H = VERA_BASE + $0A
VERA_FX_Y_INCR_L = VERA_BASE + $0B
VERA_FX_Y_INCR_H = VERA_BASE + $0C
VERA_DCSEL_FX_POS = 4 << 1      ;   4: the X and Y positions (11 bits) ...
VERA_FX_X_POS_L = VERA_BASE + $09
VERA_FX_X_POS_H = VERA_BASE + $0A
VERA_FX_Y_POS_L = VERA_BASE + $0B
VERA_FX_Y_POS_H = VERA_BASE + $0C
VERA_DCSEL_FX_FILL = 5 << 1     ;   5: their fractions, and the polygon's fill length (a read) ...
VERA_FX_X_POS_S = VERA_BASE + $09
VERA_FX_Y_POS_S = VERA_BASE + $0A
VERA_FX_POLY_FILL_L = VERA_BASE + $0B
VERA_FX_POLY_FILL_H = VERA_BASE + $0C
VERA_DCSEL_FX_CACHE = 6 << 1    ;   6: the 32-bit cache (writes); a read, the accumulator reset or added to
VERA_FX_CACHE_L = VERA_BASE + $09
VERA_FX_CACHE_M = VERA_BASE + $0A
VERA_FX_CACHE_H = VERA_BASE + $0B
VERA_FX_CACHE_U = VERA_BASE + $0C
VERA_FX_ACCUM_RESET = VERA_BASE + $09
VERA_FX_ACCUM   = VERA_BASE + $0A
VERA_VER_ID     = $56           ; DC_VER0's "V": the version follows (gateware v0.9 has no DCSEL 63)
VERA_IRQ_VSYNC  = $01           ; IEN, ISR
VERA_IRQ_LINE   = $02
VERA_IRQ_SPRCOL = $04
VERA_IRQ_AFLOW  = $08
VERA_INC_0      = $00           ; ADDRx_H's increments (VRAM address bit 16 in bit 0)
VERA_INC_1      = $10
VERA_INC_2      = $20
VERA_INC_4      = $30
VERA_INC_160    = $D0
VERA_INC_320    = $E0
VERA_NIBBLE_INC = $04           ; ADDRx_H: FX's 4-bit mode's nibble increment (its nibble: bit 1)
VERA_DECR       = $08
VERA_DC_OUT_VGA = $01           ; DC_VIDEO: VGA ...
VERA_DC_OUT_NTSC = $02          ;   NTSC (composite and S-video) ...
VERA_DC_OUT_RGB = $03           ;   RGB, 15 kHz (interlaced, or 240P) ...
VERA_DC_CHROMA_OFF = $04        ;   NTSC without colour ...
VERA_DC_240P    = $08           ;   NTSC and RGB progressive (240 lines) ...
VERA_DC_LAYER0  = $10           ;   and what's on
VERA_DC_LAYER1  = $20
VERA_DC_SPRITES = $40
VERA_PSG_BASE   = $1F9C0        ; VRAM's write-only registers: the PSG (16 x 4 bytes) ...
VERA_PALETTE_BASE = $1FA00      ;   the palette (256 x 2: $GB, $0R) ...
VERA_SPRITES_BASE = $1FC00      ;   the sprites' attributes (128 x 8)

; ---- The DS1747 clock in U7 (if fitted): its registers are task F's $7FF8-$7FFF
RTC_TASK        = $0F
RTC_REGS        = $7FF8         ; Control (the century, BCD; W, R), seconds (OSC: bit 7), minutes, hours, day (BF: bit 7;
                                ;   FT: bit 6), date, month, year: BCD
RTC_W           = $80           ; Control: W, updates halted for a write (clearing it sets the clock from them)
RTC_R           = $40           ; Control: R, updates halted for a read
