; ****************************************************************************
; cons.s - the bring-up console: the serial port, polled, at 9600 8N1.  Enough to boot, print and read keys
; until the console driver (phase 2) owns the ACIA; then PUTC, PUTS and GETC become writes and reads on fds 1
; and 0.  The ACIA's own interrupts stay off here.
;   Rockwell R65C51: wait for TDRE before each byte.  WDC W65C51N (ACIA_CHIP): its TDRE always reads 1, so wait a
;   character's time after each byte instead.
; Every routine keeps the caller's I flag (the boot prints with IRQs off).

.include "kdefs.inc"

CHAR_CYCLES     = CLK_CPS * 11 / 9600                       ; (A character at 9600, and a margin)

.segment "KCODE"

; The ACIA: reset, 9600 8N1, its interrupts off
K_CONS_INIT:
            sta         ACIA_STATUS                         ; (A programmed reset)
            lda         #ACIA_CTRL_BRG | ACIA_RATE_9600 | ACIA_CTRL_8N1
            sta         ACIA_CTRL
            lda         #ACIA_CMD_DTR | ACIA_CMD_NO_RXIRQ | ACIA_CMD_TX_ON
            sta         ACIA_CMD
            lda         ACIA_DATA                           ; (Nothing received yet)
            rts

; PUTC: .A to the serial port.  Keeps .A, .X, .Y
K_PUTC:
            php
            pha
.if ACIA_CHIP = ACIA_ROCKWELL
@wait:
            sei
            lda         ACIA_STATUS
            and         #ACIA_ST_TDRE
            bne         @ready
            pla                                             ; (Not yet: a moment for interrupts, as the caller had them)
            plp
            php
            pha
            bra         @wait

@ready:
            pla
            sta         ACIA_DATA
            plp
.else
            sei
            pla
            sta         ACIA_DATA
            plp
            phx                                             ; A character's time (1280 cycles per .X)
            phy
            ldx         #(CHAR_CYCLES + 1279) / 1280
            ldy         #0
:
            dey
            bne         :-
            dex
            bne         :-
            ply
            plx
.endif
            clc
            rts

; PUTS: the string at r0 (any length).  Modifies .A, .Y, r0
K_PUTS:
            ldy         #0
@next:
            lda         (r0),Y
            beq         @done
            jsr         K_PUTC
            iny
            bne         @next
            inc         r0 + 1
            bra         @next

@done:
            clc
            rts

; The kernel's own strings (KPRINT): the string at r0, up to 255 bytes.  Keeps .A, .X, .Y
K_PUTSTR:
            pha
            phy
            ldy         #0
@next:
            lda         (r0),Y
            beq         @done
            jsr         K_PUTC
            iny
            bne         @next
@done:
            ply
            pla
            rts

; GETC: a byte from the serial port, waiting for one (the other tasks run meanwhile).  OUT: .A; or C = 1,
; .A = E_INTR (a note came: taken on the way out)
K_GETC:
            php
            sei
            lda         ACIA_STATUS
            and         #ACIA_ST_RDRF
            beq         @none
            lda         ACIA_DATA
            plp
            clc
            rts

@none:
            plp
            lda         TK_NOTED                            ; A note: E_INTR, and the note (notes.s)
            bne         @intr
            jsr         K_YIELD
            bra         K_GETC

@intr:
            lda         #E_INTR
            sec
            jmp         K_NOTE_RETURN

; PUTHEX: .A as two hex digits.  Keeps .A, .X, .Y
K_PUTHEX:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         K_PUTNIB
            pla
            pha
            jsr         K_PUTNIB
            pla
            clc
            rts

K_PUTNIB:
            and         #$0F
            cmp         #10
            bcc         :+
            adc         #'A' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            jmp         K_PUTC
