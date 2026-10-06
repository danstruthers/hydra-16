; ****************************************************************************
; cons.s - PUTC, PUTS and GETC: a write to fd 1 and a read from fd 0 when the task has them (the console driver
; serves them, through init's #c/cons), else the bring-up console: the serial port, polled, at 9600 8N1, for the
; boot and the kernel's own messages until the console driver owns the ACIA.  The ACIA's own interrupts stay off
; here.
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

; PUTC: .A to stdout (fd 1), or the serial port.  Keeps .A, .X, .Y (and r0, r1: the kernel's KPRINT uses it)
K_PUTC:
            pha
            lda         TA_FD + 1                           ; Fd 1 open?
            cmp         #CH_MAX
            pla
            bcc         @fd
            jsr         K_KMESG_PUT                         ; (The kernel's messages keep it)
            php                                             ; ---- The serial port, polled
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

@fd:                                                        ; ---- A write to fd 1
            sta         TA_PUTC
            pha
            phx
            phy
            lda         r0
            pha
            lda         r0 + 1
            pha
            lda         r1
            pha
            lda         r1 + 1
            pha
            lda         #<TA_PUTC
            sta         r0
            lda         #>TA_PUTC
            sta         r0 + 1
            lda         #1
            sta         r1
            stz         r1 + 1
            FARCALL     K_WRITE
            pla
            sta         r1 + 1
            pla
            sta         r1
            pla
            sta         r0 + 1
            pla
            sta         r0
            ply
            plx
            pla
            clc
            rts

; .A onto the kernel's messages (K_KMESG_BUF, a ring in the kernel task's RAM: KMESG and /dev/kmesg read it), from any
; task: a quick look, IRQs off for some 40 cycles and no stack meanwhile (KM_PTR the kernel task's).  Keeps .A, .X,
; .Y and the I flag
K_KMESG_PUT:
            php
            phx
            phy
            pha
            sei
            ldy         T_REGISTER                          ; (.Y: this task, to come back to)
            tax                                             ; (.X: the byte)
            stz         T_REGISTER                          ; ---- The kernel task
            lda         K_KMESG_HEAD
            sta         KM_PTR
            lda         K_KMESG_HEAD + 1
            ora         #>K_KMESG_BUF
            sta         KM_PTR + 1
            txa
            sta         (KM_PTR)
            inc         K_KMESG_HEAD                        ; The next place, round the ring
            bne         :+
            lda         K_KMESG_HEAD + 1
            inc         a
            and         #>(KMESG_SIZE - 1)
            sta         K_KMESG_HEAD + 1
:
            lda         K_KMESG_LEN + 1                     ; One more held, till it's full
            cmp         #>KMESG_SIZE
            beq         :+
            inc         K_KMESG_LEN
            bne         :+
            inc         K_KMESG_LEN + 1
:
            sty         T_REGISTER                          ; ---- Back
            pla
            ply
            plx
            plp
            rts

; PUTS: the string at r0 (any length) to stdout.  OUT: C = 0; or C = 1, .A: the write's error.  Modifies .A, .Y,
; r0-r2
K_PUTS:
            lda         TA_FD + 1                           ; Fd 1 open: one write
            cmp         #CH_MAX
            bcs         @polled
            lda         r0                                  ; Its length: r1
            sta         r2
            lda         r0 + 1
            sta         r2 + 1
            stz         r1
            stz         r1 + 1
@length:
            lda         (r2)
            beq         @write
            inc         r1
            bne         :+
            inc         r1 + 1
:
            inc         r2
            bne         @length
            inc         r2 + 1
            bra         @length

@write:
            lda         #1
            FARCALL     K_WRITE
            bcs         :+
            clc
:
            jmp         K_NOTE_CHECK                        ; (A note while it waited: taken on the way out)

@polled:
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

; GETC: a byte from stdin (fd 0), or the serial port, waiting for one (the other tasks run meanwhile).  OUT: .A;
; or C = 1, .A = E_EOF (the end of stdin), E_INTR (a note came: taken on the way out), or the read's error
K_GETC:
            lda         TA_FD                               ; Fd 0 open: a read
            cmp         #CH_MAX
            bcs         @polled
            lda         #<TA_PUTC
            sta         r0
            lda         #>TA_PUTC
            sta         r0 + 1
            lda         #1
            sta         r1
            stz         r1 + 1
            lda         #0
            FARCALL     K_READ
            bcs         @end
            cmp         #0
            beq         @eof
            lda         TA_PUTC
            clc
@end:
            jmp         K_NOTE_CHECK

@eof:
            lda         #E_EOF
            sec
            bra         @end

@polled:
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
            bra         @polled

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
