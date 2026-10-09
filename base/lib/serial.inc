; ****************************************************************************
; serial.inc - the serial port's layer, the one both console drivers are built on: the base's (modules/ser) and
; HydraOS's (reborn/modules/cons, its windows on top).  Macros, each expanded where its driver wants the code:
;   SER_RATES       the rates: each one's ACIA code and timer 2's count for a character (rate_code, rate_lo,
;                   rate_hi, rate_rounds: RODATA)
;   SER_T2_NEXT     t2_next, timer 2's interrupt: the send ring's next byte, a character's time after the last
;                   (the irq entry's, in the module's RAM: DATA); with nothing more to send it goes on to the
;                   driver's own t2_idle (which clears tx_busy, adds to the event count, and stops timer 2 or keeps
;                   it running)
;   SER_TX          tx_start (timer 2 started, if nothing's going and there's something to send), tx_put (.A into
;                   the send ring), tx_free (the room in it)
;   SER_TX_RING     tx_put and tx_free alone (a driver's second bank's, which sends from there too)
;   SER_RX_GET      rx_get: a byte from the receive ring (its first page)
;   SER_RATE_SET    rate .X set: the ACIA's control register (8N1, the rate's code), then rate_t2: timer 2's count for
;                   it (rate_t2 alone: the ACIA left as it is)
;   SER_ON          the rate the kernel's bring-up console left (RATE_BOOT, 9600), and the ACIA's receive interrupt on
;                   (its transmit one stays off: timer 2 paces the sending)
; Sending is paced, at every rate, on both chips: the WDC W65C51N's TDRE doesn't work, and on the board the
; Rockwell's sending back to back at 115200 loses characters (2 idle bits then; 1 otherwise).  Receiving is each
; driver's own irq entry: a byte into the ring (rx_buf at rx_head), and TASK_EVENT counted.
; The driver declares, on its zero page: rx_head, rx_tail (the receive ring: the irq entry's end, the serve entry's),
; tx_head, tx_tail (the send ring's), tx_busy (<> 0: a byte is going), t2_lo, t2_hi, t2_rounds, t2_left (timer 2
; for a character: a round's count, the rounds, those left), rate (the rate, its index in the tables); and t2_nap
; if timer 2 also runs with nothing to send (tx_start clears it); in its RAM, rx_buf (256 bytes, at least) and
; tx_buf (256).  It owns LINE_ACIA and LINE_VIA_T2.

RATE_BOOT       = 5             ; 9600: the kernel's bring-up console's
RATE_N          = 8             ; 300, 600, 1200, 2400, 4800, 9600, 19200, 115200

; The rates: the ACIA's code, and timer 2 for a character (10 bits, and the idle bits after it: 2 at 115200 on
; the Rockwell, 1 otherwise), in rounds of at most 65000 cycles.  A bit is 32 cycles at 115200 (the CPU's clock
; is the ACIA's * 2, times CPU_CLOCK_MULT).  RATE i, div, gap: T2C_i the cycles, T2R_i the rounds, T2P_i a round's
; count (less the 2 the timer adds).  (Symbols, not .define functions: ca65 loses nested ones' values)
.macro RATE i, div, gap
            .ident(.sprintf("T2C_%d", i)) = (10 + (gap)) * 32 * (div) * CPU_CLOCK_MULT
            .ident(.sprintf("T2R_%d", i)) = (.ident(.sprintf("T2C_%d", i)) + 64999) / 65000
            .ident(.sprintf("T2P_%d", i)) = .ident(.sprintf("T2C_%d", i)) / .ident(.sprintf("T2R_%d", i)) - 2
.endmacro
GAP         = 1
.if ACIA_CHIP = ACIA_WDC
GAP_FAST    = 1
.else
GAP_FAST    = 2
.endif
            RATE        0, 384, GAP                         ; 300
            RATE        1, 192, GAP                         ; 600
            RATE        2, 96, GAP                          ; 1200
            RATE        3, 48, GAP                          ; 2400
            RATE        4, 24, GAP                          ; 4800
            RATE        5, 12, GAP                          ; 9600
            RATE        6, 6, GAP                           ; 19200
            RATE        7, 1, GAP_FAST                      ; 115200

.macro SER_RATES
rate_code:  .byte       $06, $07, $08, $0A, $0C, ACIA_RATE_9600, ACIA_RATE_19200, ACIA_RATE_115200
rate_lo:    .byte       <T2P_0, <T2P_1, <T2P_2, <T2P_3, <T2P_4, <T2P_5, <T2P_6, <T2P_7
rate_hi:    .byte       >T2P_0, >T2P_1, >T2P_2, >T2P_3, >T2P_4, >T2P_5, >T2P_6, >T2P_7
rate_rounds: .byte      T2R_0, T2R_1, T2R_2, T2R_3, T2R_4, T2R_5, T2R_6, T2R_7
.assert     ACIA_RATE_9600 = $0E .and ACIA_RATE_19200 = $0F, error, "The rates' codes"
.endmacro

; Timer 2 ran out: the next byte (a character's time since the last went); a slow rate's rounds; or nothing more to
; send: the driver's t2_idle, next
.macro SER_T2_NEXT
t2_next:
            dec         t2_left                             ; (A slow rate, or a driver's wait: another round)
            bne         t2_round
            ldy         tx_tail
            cpy         tx_head
            beq         t2_idle
            lda         tx_buf,Y
            sta         ACIA_DATA
            iny
            sty         tx_tail
            lda         t2_rounds
            sta         t2_left
t2_round:
            lda         t2_lo
            sta         VIA_T2CL
            lda         t2_hi
            sta         VIA_T2CH                            ; (It starts, and its interrupt's cleared)
            lda         #0
            rts
.endmacro

; Timer 2 started, if nothing's going and there's something to send: it sends the first byte a character's time
; from now, and the rest after it.  (Not at once: the kernel's bring-up console may have sent a byte just now; and
; the status register isn't for reading here, which would lose a receive interrupt.)  Modifies .A, .Y
.macro SER_TX
tx_start:
            php
            sei
            lda         tx_busy
            bne         @done
            ldy         tx_tail
            cpy         tx_head
            beq         @done
            inc         tx_busy
.ifdef t2_nap
            stz         t2_nap
.endif
            ldy         rate                                ; (A character's time: a wait may have had timer 2)
            lda         rate_hi,Y
            sta         t2_hi
            lda         t2_rounds
            sta         t2_left
            lda         t2_lo
            sta         VIA_T2CL
            lda         t2_hi
            sta         VIA_T2CH
@done:
            plp
            rts

            SER_TX_RING
.endmacro

.macro SER_TX_RING
; .A into the send ring (the caller has made sure of the room).  Keeps .A, .X
tx_put:
            ldy         tx_head
            sta         tx_buf,Y
            iny
            sty         tx_head
            rts

; .A = the room in the send ring
tx_free:
            sec
            lda         tx_tail
            sbc         tx_head
            dec         a
            rts
.endmacro

; A byte from the receive ring (its first page).  OUT: C = 0, .A = it; or C = 1: none.  Modifies .Y
.macro SER_RX_GET
rx_get:
            ldy         rx_tail
            cpy         rx_head
            beq         @none
            lda         rx_buf,Y
            inc         rx_tail
            clc
            rts

@none:
            sec
            rts
.endmacro

; Rate .X: the ACIA's control register (8N1, its code), then timer 2 for its characters (rate_t2: that alone).
; OUT: C = 0
.macro SER_RATE_SET
            stx         rate
            lda         #ACIA_CTRL_BRG | ACIA_CTRL_8N1
            ora         rate_code,X
            sta         ACIA_CTRL
rate_t2:                                                    ; Timer 2 for rate .X's characters
            stx         rate
            lda         rate_lo,X
            sta         t2_lo
            lda         rate_hi,X
            sta         t2_hi
            lda         rate_rounds,X
            sta         t2_rounds
            clc
            rts
.endmacro

; The rate the bring-up console left the ACIA at (9600), and its receive interrupt on (its transmit one off).
; Modifies .A, .X (rate_t2's)
.macro SER_ON
            ldx         #RATE_BOOT                          ; (The ACIA is at it already: the kernel set it)
            jsr         rate_t2
            sei
            lda         #ACIA_CMD_DTR | ACIA_CMD_TX_ON      ; Its receive interrupt on (its transmit one stays off)
            sta         ACIA_CMD
            lda         ACIA_STATUS
            lda         ACIA_DATA
            cli
.endmacro
