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
;   SER_RATE_IX     .Y = the rate's index in the tables, for the chip
; The two chips differ (the R65C51's sending, the W65C51N's TDRE): the code is the same for both, and asks the BIOS
; which it is built for (BIOS_ACIA: hw.inc), so the paged ROM is one image whichever is in U3.
;   SER_FLOW        the sender held back, for a driver that declares fl_send (below): rx_marks, fl_run, fl_put
;
; Flow control (a driver that declares fl_send and rx_mark on its zero page, and fl_x, has it: HydraOS's console; the
; base's has none).  It is XON/XOFF, the receiving side's: with FL_HI bytes unread in the receive ring the driver
; sends XOFF, and the terminal (one that's set to honour it) sends no more; when the ring is down to FL_LO it sends
; XON.  Above FL_HI there's room for what the terminal sends before it has heard (255 - FL_HI bytes).
;   Not RTS and CTS, though the board wires both to the DE-9: the R65C51 turns its transmitter off, at once, while
; its ~RTS is high (command bits 3-2 = 00: its data sheet's note), so the Hydra could hold the PC back only while it
; sent nothing itself, not even a key's echo; and each byte it then sent would let one in.
;   fl_x is the setting (x1: <> 0; x0).  The irq entry checks the ring against one byte, rx_mark: the tail (the ring
; is full there: a byte is dropped), or with x1, and the sender not told to stop, the tail + FL_HI.  There the irq
; entry makes the mark the tail, and asks for XOFF in fl_send; timer 2 sends fl_send's byte before the ring's next
; (t2_flow).  rx_get moves the mark with the tail (rx_marks), and at FL_LO leaves XON.  So "told to stop" is: x1,
; and the mark at the tail.
;   The irq entry has few cycles for this (the kernel's budget for IRQs off: its high-mark path is as long as a
; Ctrl-C's): what it leaves in fl_send is FL_POKE, which it also writes to timer 2's count's high byte, starting
; the timer whether a byte's going or not.  t2_flow sends XON for XON, and XOFF for anything else.
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

XON             = $11           ; Flow control's bytes: go on (DC1, Ctrl-Q) ...
XOFF            = $13           ;   and stop (DC3, Ctrl-S)
FL_HI           = 96            ; The unread bytes at which the sender's told to stop (159 more fit) ...
FL_LO           = 32            ;   and to go on
; The irq entry's count for timer 2 (its high byte), XOFF wanted: timer 2 runs out that soon, and XOFF goes.  A
; byte may be going: the count is 9600's character and a little, so at 9600 and above its time isn't cut short (the
; ring's next byte is only late).  Below 9600 it may be: the R65C51 keeps XOFF in its transmitter data register
; till the byte's out.  The W65C51N has none to trust (a byte written over one going garbles both): its count is
; long enough for 2400 and up.  (Under $80: it's the irq entry's answer too, no task switch.  And not XON)  The
; driver's irq entry (in its RAM) has the Rockwell's, and puts the WDC's in its place as it starts, if it's that.
FL_POKE         = T2C_5 / 256 + 2
FL_POKE_WDC     = $7F
.assert     FL_POKE < $80 .and FL_POKE <> XON .and FL_POKE * 256 >= T2C_5, error, "FL_POKE: timer 2's count for an XOFF"
.assert     FL_POKE_WDC < $80 .and FL_POKE_WDC <> XON .and FL_POKE_WDC * 256 >= T2C_5, error, "FL_POKE_WDC"

; The rates: the ACIA's code, and timer 2 for a character (10 bits, and the idle bits after it: 2 at 115200 on
; the Rockwell, 1 otherwise: the table's 115200 is the Rockwell's, and the WDC's after the rest, RATE_N's), in rounds
; of at most 65000 cycles.  A bit is 32 cycles at 115200 (the CPU's clock
; is the ACIA's * 2, times CPU_CLOCK_MULT).  RATE i, div, gap: T2C_i the cycles, T2R_i the rounds, T2P_i a round's
; count (less the 2 the timer adds).  (Symbols, not .define functions: ca65 loses nested ones' values)
.macro RATE i, div, gap
            .ident(.sprintf("T2C_%d", i)) = (10 + (gap)) * 32 * (div) * CPU_CLOCK_MULT
            .ident(.sprintf("T2R_%d", i)) = (.ident(.sprintf("T2C_%d", i)) + 64999) / 65000
            .ident(.sprintf("T2P_%d", i)) = .ident(.sprintf("T2C_%d", i)) / .ident(.sprintf("T2R_%d", i)) - 2
.endmacro
GAP         = 1
GAP_FAST    = 2
            RATE        0, 384, GAP                         ; 300
            RATE        1, 192, GAP                         ; 600
            RATE        2, 96, GAP                          ; 1200
            RATE        3, 48, GAP                          ; 2400
            RATE        4, 24, GAP                          ; 4800
            RATE        5, 12, GAP                          ; 9600
            RATE        6, 6, GAP                           ; 19200
            RATE        7, 1, GAP_FAST                      ; 115200 (the R65C51's)
            RATE        8, 1, GAP                           ; 115200 (the W65C51N's: SER_RATE_IX)

.macro SER_RATES
rate_code:  .byte       $06, $07, $08, $0A, $0C, ACIA_RATE_9600, ACIA_RATE_19200, ACIA_RATE_115200
rate_lo:    .byte       <T2P_0, <T2P_1, <T2P_2, <T2P_3, <T2P_4, <T2P_5, <T2P_6, <T2P_7, <T2P_8
rate_hi:    .byte       >T2P_0, >T2P_1, >T2P_2, >T2P_3, >T2P_4, >T2P_5, >T2P_6, >T2P_7, >T2P_8
rate_rounds: .byte      T2R_0, T2R_1, T2R_2, T2R_3, T2R_4, T2R_5, T2R_6, T2R_7, T2R_8
.assert     ACIA_RATE_9600 = $0E .and ACIA_RATE_19200 = $0F, error, "The rates' codes"
.endmacro

; Timer 2 ran out: the next byte (a character's time since the last went); a slow rate's rounds; or nothing more to
; send: the driver's t2_idle, next
.macro SER_T2_NEXT
t2_next:
.ifdef fl_send
            lda         fl_send                             ; (Flow control's byte goes first: t2_flow)
            bne         t2_flow
.endif
            dec         t2_left                             ; (A slow rate, or a driver's wait: another round)
            bne         t2_round
            ldy         tx_tail
            cpy         tx_head
            beq         t2_idle
            lda         tx_buf,Y
            sta         ACIA_DATA
            iny
            sty         tx_tail
t2_sent:
            lda         t2_rounds
            sta         t2_left
t2_round:
            lda         t2_hi                               ; It starts, and its interrupt's cleared.  (The count's
            sta         VIA_T2CH                            ;   low byte is the VIA's latch still: tx_start's)
            lda         #0
            rts
.ifdef fl_send

; Flow control's byte (.A: XON, or XOFF's request: XOFF, or the irq entry's FL_POKE): at once if nothing's going
; (timer 2 was started for it, or was napping: the last byte has had its time); else in the ring's next byte's
; place, when the one that's going has had its
t2_flow:
            ldx         tx_busy
            beq         :+
            dec         t2_left
            bne         t2_round
:
            cmp         #XON
            beq         :+
            lda         #XOFF
:
            sta         ACIA_DATA
            stz         fl_send
            bra         t2_sent
.endif
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
            SER_RATE_IX                                     ; (A character's time: a wait may have had timer 2)
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

; A byte from the receive ring (its first page).  OUT: C = 0, .A = it; or C = 1: none.  Modifies .Y.  (With flow
; control the tail and the irq entry's mark move as one: IRQs off)
.macro SER_RX_GET
rx_get:
.ifdef fl_send
            php
            sei
.endif
            ldy         rx_tail
            cpy         rx_head
            beq         @none
            lda         rx_buf,Y
            inc         rx_tail
.ifdef fl_send
            jsr         rx_marks
            plp
.endif
            clc
            rts

@none:
.ifdef fl_send
            plp
.endif
            sec
            rts
.endmacro

; Flow control's routines (a driver with fl_send): each with IRQs off
.macro SER_FLOW
; The irq entry's mark after a byte's taken (the tail one on).  x0: the tail.  x1: the tail + FL_HI; or, the sender
; told to stop (the mark was the tail), the tail still, till the ring is down to FL_LO: then XON, and the high mark.
; Keeps .A, .X
rx_marks:
            pha
            lda         fl_x
            beq         @tail
            lda         rx_tail
            dec         a
            cmp         rx_mark
            bne         @high
            sec
            lda         rx_head
            sbc         rx_tail
            cmp         #FL_LO + 1
            bcs         @tail
            lda         #XON
            jsr         fl_put
@high:
            clc
            lda         rx_tail
            adc         #FL_HI
            bra         @set

@tail:
            lda         rx_tail
@set:
            sta         rx_mark
            pla
            rts

; The mark from here, as fl_x says (the setting changed, or the ring emptied).  x0: the tail.  x1: the tail + FL_HI,
; the sender told to go on (XON) if the mark was at the tail (it had been told to stop; or this is x1's start: a
; terminal this Hydra told to stop before its reset is let go); or, with FL_HI in the ring already, to stop.
; Modifies .A
fl_run:
            lda         fl_x
            beq         @tail
            sec
            lda         rx_head
            sbc         rx_tail
            cmp         #FL_HI
            bcs         @stop
            lda         rx_mark
            cmp         rx_tail
            bne         :+
            lda         #XON
            jsr         fl_put
:
            clc
            lda         rx_tail
            adc         #FL_HI
            bra         @set

@stop:
            lda         #XOFF
            jsr         fl_put
@tail:
            lda         rx_tail
@set:
            sta         rx_mark
            rts

; .A (XOFF or XON) for timer 2 to send before the ring's next byte; timer 2 started for it if nothing's going (a
; count of its low latch alone: at once).  Modifies .A
fl_put:
            sta         fl_send
            lda         tx_busy
            bne         :+
            stz         VIA_T2CH
:
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
            phy
            SER_RATE_IX
            lda         rate_lo,Y
            sta         t2_lo
            lda         rate_hi,Y
            sta         t2_hi
            lda         rate_rounds,Y
            sta         t2_rounds
            ply
            clc
            rts
.endmacro

; .Y = rate's index in the tables: itself, but the W65C51N's 115200 (BIOS_ACIA) RATE_N's.  Modifies .A
.macro SER_RATE_IX
            ldy         rate
            cpy         #RATE_N - 1
            bne         :+
            lda         BIOS_ACIA
            beq         :+
            ldy         #RATE_N
:
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
