.debuginfo

; ****************************************************************************
; The serial port's settings: baud rate, data bits, parity and stop bits (BIOS ROM page 9, included inside
; `.scope PAGE9`, see all.s; part of the serial driver's server, ser_srv.s on page 2, which reaches it through
; gates, as it reaches ser_srv's: page9.s).  Runs in the serial task.
;   /dev/ser/ctl  read: the settings as text, "b9600 l8 pn s1" and CR LF.  write: commands, separated by
;                 spaces (or CR, LF): bN the baud rate (300, 600, 1200, 1800, 2400, 3600, 4800, 7200,
;                 9600, 19200, 115200), lN the data bits (5-8), pX the parity (n none, o odd, e even,
;                 m mark, s space), sN the stop bits (1-2).  E.g. "b19200", "l7 pe s1".  A bad command
;                 changes nothing (ERR_IO_BAD_REQ).
;   IO_CTL        SER_CTL_RATE (.Y = SER_RATE_*), SER_CTL_FORMAT (.Y = SER_FMT_*), on any serial fid.
; A change waits until what's queued for sending has gone (SER_DRAIN), then takes effect for both
; directions.  The terminal has to be switched to match.  (The ACIA's rates are 2.9% slow on this board:
; see hw.inc.)  Not possible: 2 stop bits with 8 data bits and parity (the 65C51 sends 1), and with the
; WDC ACIA, rates whose character doesn't fit VIA timer 2 (below about 1200 at 3.58 MHz).
; Server ZP (the serial task's): ZP_IO_CHUNK = the client; ZP_IO_TMP, ZP_IO_BYTE, ZP_IO_LEFT, ZP_IO_BUF,
; ZP_IO_CNT, ZP_IO_OFS as scratch.

.segment "SYS_P9"

; The rates (SER_RATE_*): the ACIA's code, a bit's time in CPU cycles (2 * 1843200 / rate, times the clock
; multiplier: the ACIA's clock is half the CPU's base clock), and the text
SER_RATE_CODE:  .byte   SR_300, SR_600, SR_1200, SR_1800, SR_2400, SR_3600, SR_4800, SR_7200, SR_9600, SR_19200
                .byte   SR_115200
SER_RATE_CYC_L: .lobytes 2 * 1843200 / 300 * CPU_CLOCK_MULT, 2 * 1843200 / 600 * CPU_CLOCK_MULT
                .lobytes 2 * 1843200 / 1200 * CPU_CLOCK_MULT, 2 * 1843200 / 1800 * CPU_CLOCK_MULT
                .lobytes 2 * 1843200 / 2400 * CPU_CLOCK_MULT, 2 * 1843200 / 3600 * CPU_CLOCK_MULT
                .lobytes 2 * 1843200 / 4800 * CPU_CLOCK_MULT, 2 * 1843200 / 7200 * CPU_CLOCK_MULT
                .lobytes 2 * 1843200 / 9600 * CPU_CLOCK_MULT, 2 * 1843200 / 19200 * CPU_CLOCK_MULT
                .lobytes 32 * CPU_CLOCK_MULT                        ; (115200: the ACIA's clock / 16)
SER_RATE_CYC_H: .hibytes 2 * 1843200 / 300 * CPU_CLOCK_MULT, 2 * 1843200 / 600 * CPU_CLOCK_MULT
                .hibytes 2 * 1843200 / 1200 * CPU_CLOCK_MULT, 2 * 1843200 / 1800 * CPU_CLOCK_MULT
                .hibytes 2 * 1843200 / 2400 * CPU_CLOCK_MULT, 2 * 1843200 / 3600 * CPU_CLOCK_MULT
                .hibytes 2 * 1843200 / 4800 * CPU_CLOCK_MULT, 2 * 1843200 / 7200 * CPU_CLOCK_MULT
                .hibytes 2 * 1843200 / 9600 * CPU_CLOCK_MULT, 2 * 1843200 / 19200 * CPU_CLOCK_MULT
                .hibytes 32 * CPU_CLOCK_MULT
SER_RATE_TEXT:  .byte   "300", 0, "600", 0, "1200", 0, "1800", 0, "2400", 0, "3600", 0, "4800", 0, "7200", 0
                .byte   "9600", 0, "19200", 0, "115200", 0
.assert     SER_RATE_CYC_L - SER_RATE_CODE = SER_RATES, error, "The serial rate table has SER_RATES entries"
.assert     SR_SELECT = SR_9600 .and SER_RATE_BOOT = SER_RATE_9600, error, "SR_SELECT and SER_RATE_BOOT must agree"

; The parity (SER_PAR_*): the command register's bits 5-7, and the letter
SER_PAR_BITS:   .byte   $00, $20, $60, $A0, $E0
SER_PAR_TEXT:   .byte   "noems"

; Set the port: IN: .A = rate (SER_RATE_*), .Y = format (SER_FMT_*), ZP_IO_CHUNK = the client
; OUT (a serve routine's): C = 0; or C = 1, .A = ERR_IO_BAD_REQ, or ERR_IO_WOULD_BLOCK (what's queued is
; still being sent: the client is woken when it has gone, and asks again)
SER_SET:
            sta         ZP_IO_TMP
            sty         ZP_IO_BYTE
            jsr         SER_FRAME                           ; (Check it first)
            bcs         @done
            jsr         SER_DRAIN
            bcs         @wait
            lda         ZP_IO_TMP
            ldy         ZP_IO_BYTE
            jsr         SER_CONFIG
            bcs         @done
            jmp         SER_OK

@wait:
            jmp         SER_WOULD_BLOCK

@done:
            rts

; Set up the ACIA for rate .A (SER_RATE_*) and format .Y (SER_FMT_*), at once (SER_SET waits for the
; transmitter first).  The serial driver's init calls it too.  Runs in the serial task.
; OUT: C = 0; or C = 1, .A = ERR_IO_BAD_REQ.  Modifies: .A, .X, .Y
SER_CONFIG:
            sta         ZP_IO_LEFT
            sty         ZP_IO_LEFT + 1
            jsr         SER_FRAME
            bcs         @done
            php
            sei
            lda         ZP_IO_BUF
            sta         ACIA_R_CTRL
            lda         ZP_IO_BUF + 1
            sta         ACIA_R_CMD
            lda         ZP_IO_LEFT
            sta         SER_RATE
            lda         ZP_IO_LEFT + 1
            sta         SER_FORMAT
            lda         ZP_IO_CNT
            sta         SER_BIT_CYC
            lda         ZP_IO_CNT + 1
            sta         SER_BIT_CYC + 1
            lda         ZP_IO_OFS
            sta         SER_T2_CHAR
            lda         ZP_IO_OFS + 1
            sta         SER_T2_CHAR + 1
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL
            stz         SER_PACED                           ; At 115200, sending paced by timer 2 (one-shot,
            lda         #VIA_T2_INT_BIT                     ;   its interrupt on; SER_IRQ_FAST: SER_T2_FAST);
            ldx         ZP_IO_LEFT                          ;   otherwise by TDRE, and timer 2's interrupt off
            cpx         #SER_RATE_115200
            bne         :+
            inc         SER_PACED
            trb         VIA_R_AUX_CTRL                      ; (ACR bit 5, as VIA_T2_INT_BIT: one-shot)
            ora         #VIA_INT_ENABLE
:
            sta         VIA_R_INT_ENABLE
.endif
            plp
            clc

@done:
            rts

; Check rate .A and format .Y, and work out the ACIA's registers and the timing.
; OUT: C = 0, ZP_IO_BUF = the control register, ZP_IO_BUF + 1 = the command register, ZP_IO_CNT = a bit's
; time, ZP_IO_OFS = a character's time and a bit (VIA timer 2's count, for the WDC ACIA); or C = 1,
; .A = ERR_IO_BAD_REQ.  Modifies: .A, .X, ZP_IO_OFS + 2, + 3
SER_FRAME:
            cmp         #SER_RATES
            bcc         @rate_ok

@bad_arg:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

@rate_ok:
            tax                                             ; .X = the rate
            tya
            and         #<~(SER_FMT_BITS | SER_FMT_PARITY | SER_FMT_STOP2)
            bne         @bad_arg
            tya
            and         #SER_FMT_PARITY
            cmp         #(SER_PAR_SPACE + 1) << 2
            bcs         @bad_arg
            tya                                             ; 8 data bits, parity and 2 stop bits: no
            and         #SER_FMT_BITS | SER_FMT_STOP2
            cmp         #3 | SER_FMT_STOP2
            bne         :+
            tya
            and         #SER_FMT_PARITY
            bne         @bad_arg
:
            tya                                             ; Control: the rate, its generator (bit 4),
            and         #SER_FMT_BITS                       ;   the word length (bits 5-6: 0 = 8 data
            eor         #3                                  ;   bits ... 3 = 5), 2 stop bits (bit 7)
            asl
            asl
            asl
            asl
            asl
            ora         SER_RATE_CODE,X
            ora         #$10
            sta         ZP_IO_BUF
            tya
            and         #SER_FMT_STOP2
            beq         :+
            lda         #$80
            tsb         ZP_IO_BUF
:
            tya                                             ; Command: SER_CMD_BASE and the parity
            and         #SER_FMT_PARITY
            lsr
            lsr
            phx
            tax
            lda         SER_PAR_BITS,X
            plx
            ora         #SER_CMD_BASE
            sta         ZP_IO_BUF + 1
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL
            cpx         #SER_RATE_115200                    ; (Paced at 115200: no TDRE interrupt)
            bne         :+
            eor         #ACIA_CMD_BIT_TLIE | ACIA_CMD_BIT_TLID
            sta         ZP_IO_BUF + 1
:
.endif
            lda         SER_RATE_CYC_L,X                    ; A bit's time
            sta         ZP_IO_CNT
            lda         SER_RATE_CYC_H,X
            sta         ZP_IO_CNT + 1
            tya                                             ; The bits: start, data, parity, stop, and a
            and         #SER_FMT_BITS                       ;   bit's margin
            clc
            adc         #1 + 5 + 1 + 1
            sta         ZP_IO_OFS + 2
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL
            cpx         #SER_RATE_115200                    ; (Paced at 115200: SER_PACE_GAP bits of idle
            bne         :+                                  ;   line, not a bit's margin)
            clc
            adc         #SER_PACE_GAP - 1
            sta         ZP_IO_OFS + 2
:
.endif
            tya
            and         #SER_FMT_PARITY
            beq         :+
            inc         ZP_IO_OFS + 2
:
            tya
            and         #SER_FMT_STOP2
            beq         :+
            inc         ZP_IO_OFS + 2
:
            stz         ZP_IO_OFS                           ; ZP_IO_OFS = the bits * a bit's time (24 bits)
            stz         ZP_IO_OFS + 1
            stz         ZP_IO_OFS + 3
            ldx         ZP_IO_OFS + 2

@times:
            clc
            lda         ZP_IO_OFS
            adc         ZP_IO_CNT
            sta         ZP_IO_OFS
            lda         ZP_IO_OFS + 1
            adc         ZP_IO_CNT + 1
            sta         ZP_IO_OFS + 1
            bcc         :+
            inc         ZP_IO_OFS + 3
:
            dex
            bne         @times
.if ::SER_ACIA = ::SER_ACIA_WDC
            lda         ZP_IO_OFS + 3                       ; The WDC ACIA: it must fit VIA timer 2
            bne         @bad
.endif
            clc
            rts

@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; Wait until what's queued for sending has gone: the TX ring empty and the transmitter idle, then about
; 20 bits' time more (the Rockwell 65C51 is idle when its last byte moves to its shift register).
; OUT: C = 0 done; or C = 1: not yet, and the client (ZP_IO_CHUNK) is on the write wait list (SER_TX_NEXT
; wakes it when the ring empties).  Modifies: .A, .X, .Y, ZP_IO_LEFT
SER_DRAIN:
            php
            sei
            lda         ZP_SER_SEND_STATUS
            beq         @idle                               ; SER_SEND_STATUS_READY: the ring is empty
            ldx         ZP_IO_CHUNK
            ldy         #SER_WR_WAIT
            jsr         SER_ADD_WAIT
            plp
            sec
            rts

@idle:
            plp
            lda         SER_BIT_CYC                         ; A bit's time, 20 cycles a count
            sta         ZP_IO_LEFT
            lda         SER_BIT_CYC + 1
            sta         ZP_IO_LEFT + 1

@delay:
            lda         ZP_IO_LEFT
            bne         :+
            dec         ZP_IO_LEFT + 1
:
            dec         ZP_IO_LEFT
            lda         ZP_IO_LEFT
            ora         ZP_IO_LEFT + 1
            bne         @delay
            clc
            rts

; Read /dev/ser/ctl: the settings as text ("b9600 l8 pn s1", CR LF), from the fd's offset (up to the count)
SER_CTL_READ:
            ldx         ZP_IO_CHUNK
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            stz         ZP_IO_TMP                           ; The text's length
            lda         #'b'
            jsr         SER_PUT
            ldy         #0                                  ; The rate: its text
            ldx         SER_RATE
            beq         @rate

@skip:
            lda         SER_RATE_TEXT,Y
            iny
            ora         #0
            bne         @skip
            dex
            bne         @skip

@rate:
            lda         SER_RATE_TEXT,Y
            beq         @bits
            jsr         SER_PUT
            iny
            bra         @rate

@bits:
            lda         #' '
            jsr         SER_PUT
            lda         #'l'
            jsr         SER_PUT
            lda         SER_FORMAT
            and         #SER_FMT_BITS
            clc
            adc         #'5'
            jsr         SER_PUT
            lda         #' '
            jsr         SER_PUT
            lda         #'p'
            jsr         SER_PUT
            lda         SER_FORMAT
            and         #SER_FMT_PARITY
            lsr
            lsr
            tax
            lda         SER_PAR_TEXT,X
            jsr         SER_PUT
            lda         #' '
            jsr         SER_PUT
            lda         #'s'
            jsr         SER_PUT
            lda         SER_FORMAT
            and         #SER_FMT_STOP2
            cmp         #SER_FMT_STOP2                      ; (C = 1: 2)
            lda         #'1'
            adc         #0
            jsr         SER_PUT
            lda         #ASCII_CR
            jsr         SER_PUT
            lda         #ASCII_LF
            jsr         SER_PUT

; ... a ctl file's text (made in the data area, ZP_IO_TMP long) out: what's after the request's offset
SER_TEXT_OUT:
            dec         ZP_IO_REQ + 1
            ldy         #IO_BLK_OFS + 3                     ; Past the end: nothing more (end of file)
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @eof
            dey
            lda         (ZP_IO_REQ),Y
            cmp         ZP_IO_TMP
            bcs         @eof
            tax                                             ; .X = the offset
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_IO_BYTE                          ; Bytes wanted (1-256; 256 = 0)
            inc         ZP_IO_REQ + 1
            ldy         #0                                  ; Move the text after the offset down

@move:
            phy
            txa
            tay
            lda         (ZP_IO_REQ),Y
            ply
            sta         (ZP_IO_REQ),Y
            iny
            cpy         ZP_IO_BYTE                          ; (256: 0, never reached: the text is shorter)
            beq         @moved
            inx
            cpx         ZP_IO_TMP
            bne         @move

@moved:
            dec         ZP_IO_REQ + 1
            tya                                             ; The count
            bra         @count

@eof:
            lda         #0

@count:
            jsr         IO_SRV_COUNT
            jmp         SER_OK

; /dev/cons/ctl's requests (ser_srv.s: the file).  IN: .A = request
CONSCTL_REQUEST:
            cmp         #H9_READ
            beq         CONSCTL_READ
            cmp         #H9_WRITE
            beq         CONSCTL_WRITE
            cmp         #H9_DUP
            beq         @ref
            cmp         #H9_CLUNK
            beq         @unref
            cmp         #H9_STAT
            bne         @bad
            jsr         STAT_ZERO
            jmp         SER_OK

@ref:
            inc         SER_RAW_REFS                        ; (Another fd on it: IO_DUP, a new task's copy)
            jmp         SER_OK

@unref:
            dec         SER_RAW_REFS                        ; (The last one closed: raw ends)
            bne         :+
            stz         SER_RAW
:
            jmp         SER_OK

@bad:
            jmp         SER_REFUSE

; Read /dev/cons/ctl: "rawon" or "rawoff", and CR LF
CONSCTL_READ:
            ldx         ZP_IO_CHUNK
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            stz         ZP_IO_TMP                           ; The text's length
            ldx         #CONS_S_RAWON - CONS_S
            lda         SER_RAW
            bne         @put
            ldx         #CONS_S_RAWOFF - CONS_S

@put:
            lda         CONS_S,X
            beq         @end
            jsr         SER_PUT
            inx
            bra         @put

@end:
            lda         #ASCII_CR
            jsr         SER_PUT
            lda         #ASCII_LF
            jsr         SER_PUT
            jmp         SER_TEXT_OUT

; Write /dev/cons/ctl: "rawon" or "rawoff" (then a space, CR, LF or 0, or the write's end).  The whole write is
; taken; anything else is refused
CONSCTL_WRITE:
            ldx         ZP_IO_CHUNK
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            bne         :+
            dec                                             ; (256 bytes: look at 255)
:
            sta         ZP_IO_TMP                           ; The write's length
            inc         ZP_IO_REQ + 1                       ; The data area
            ldx         #CONS_S_RAWOFF - CONS_S
            jsr         CONS_WORD
            lda         #0
            bcc         @set
            ldx         #CONS_S_RAWON - CONS_S
            jsr         CONS_WORD
            lda         #1
            bcc         @set
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            jmp         SER_REFUSE

@set:
            sta         SER_RAW
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of it taken)
            jmp         SER_OK

; Is the write's text the word at CONS_S + .X?  OUT: C = 0: it is.  Modifies: .A, .X, .Y
CONS_WORD:
            ldy         #0

@char:
            lda         CONS_S,X
            beq         @end
            cpy         ZP_IO_TMP
            beq         @no
            cmp         (ZP_IO_REQ),Y
            bne         @no
            inx
            iny
            bra         @char

@end:
            cpy         ZP_IO_TMP                           ; Then the write's end ...
            beq         @yes
            lda         (ZP_IO_REQ),Y                       ; ... or a separator
            beq         @yes
            cmp         #ASCII_SPACE
            beq         @yes
            cmp         #ASCII_CR
            beq         @yes
            cmp         #ASCII_LF
            bne         @no

@yes:
            clc
            rts

@no:
            sec
            rts

CONS_S:
CONS_S_RAWON:   .byte   "rawon", 0
CONS_S_RAWOFF:  .byte   "rawoff", 0

; Add .A to the ctl file's text (in the data area: ZP_IO_REQ, moved up to it).  Preserves .X, .Y
SER_PUT:
            phy
            ldy         ZP_IO_TMP
            sta         (ZP_IO_REQ),Y
            inc         ZP_IO_TMP
            ply
            rts

; Write /dev/ser/ctl: commands (see the top).  The whole write is taken; a bad command changes nothing.
SER_CTL_WRITE:
            ldx         ZP_IO_CHUNK
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            bne         :+
            dec                                             ; (256 bytes: look at 255)
:
            sta         ZP_IO_TMP                           ; The write's length
            inc         ZP_IO_REQ + 1                       ; The data area
            lda         SER_RATE                            ; The new settings: the current ones, changed
            sta         ZP_IO_LEFT                          ;   by the commands
            lda         SER_FORMAT
            sta         ZP_IO_LEFT + 1
            ldy         #0

@next:
            jsr         SER_GETC
            bne         @command
            cpy         ZP_IO_TMP
            bne         @next                               ; A separator
            dec         ZP_IO_REQ + 1                       ; The end: set them
            jsr         IO_SRV_UNMAP
            lda         ZP_IO_LEFT
            ldy         ZP_IO_LEFT + 1
            jmp         SER_SET

@bad_pla:
            pla

@bad:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_BAD_REQ
            sec
            rts

@command:
            ora         #$20                                ; (Lower case)
            cmp         #'b'
            beq         @rate
            cmp         #'p'
            beq         @parity
            pha
            jsr         SER_GETC                            ; lN and sN: a digit
            sec
            sbc         #'0'
            sta         ZP_IO_BYTE
            jsr         SER_GETC                            ; Then the end of the word
            bne         @bad_pla
            pla
            cmp         #'l'
            beq         @bits
            cmp         #'s'
            bne         @bad
            lda         ZP_IO_BYTE                          ; Stop bits: 1 or 2
            dec
            cmp         #2
            bcs         @bad
            lsr                                             ; (C = 1: 2)
            lda         ZP_IO_LEFT + 1
            and         #<~SER_FMT_STOP2
            bcc         :+
            ora         #SER_FMT_STOP2
:
            sta         ZP_IO_LEFT + 1
            bra         @next

@bits:
            lda         ZP_IO_BYTE                          ; Data bits: 5-8
            sec
            sbc         #5
            cmp         #4
            bcs         @bad
            sta         ZP_IO_BYTE
            lda         ZP_IO_LEFT + 1
            and         #<~SER_FMT_BITS
            ora         ZP_IO_BYTE
            sta         ZP_IO_LEFT + 1
            bra         @next

@parity:
            jsr         SER_GETC
            ora         #$20
            ldx         #SER_PAR_SPACE

@letter:
            cmp         SER_PAR_TEXT,X
            beq         :+
            dex
            bpl         @letter
            bra         @bad
:
            jsr         SER_GETC                            ; Then the end of the word
            bne         @bad
            txa
            asl
            asl
            sta         ZP_IO_BYTE
            lda         ZP_IO_LEFT + 1
            and         #<~SER_FMT_PARITY
            ora         ZP_IO_BYTE
            sta         ZP_IO_LEFT + 1
            jmp         @next

@rate:                                                      ; The number, against each rate's text
            sty         ZP_IO_BYTE                          ; (Where it starts)
            ldx         #0                                  ; (In SER_RATE_TEXT)
            stz         ZP_IO_BUF                           ; (The rate)

@try:
            ldy         ZP_IO_BYTE

@digit:
            jsr         SER_GETC
            cmp         SER_RATE_TEXT,X
            bne         @other_rate
            inx
            ora         #0
            bne         @digit                              ; (Both ended: this one)
            lda         ZP_IO_BUF
            sta         ZP_IO_LEFT
            jmp         @next

@other_rate:
            lda         SER_RATE_TEXT,X                     ; To the next rate's text
            inx
            ora         #0
            bne         @other_rate
            inc         ZP_IO_BUF
            lda         ZP_IO_BUF
            cmp         #SER_RATES
            bcc         @try
            jmp         @bad

; The next character of the ctl write at .Y (then .Y + 1), or 0 for a separator (space, CR, LF, 0) or the end
; of the write (then .Y stays).  IN: ZP_IO_TMP = the write's length.  OUT: .A, Z from it.  Preserves .X
SER_GETC:
            cpy         ZP_IO_TMP
            beq         @none
            lda         (ZP_IO_REQ),Y
            iny
            cmp         #' '
            beq         @none
            cmp         #ASCII_CR
            beq         @none
            cmp         #ASCII_LF
            beq         @none
            ora         #0
            rts

@none:
            lda         #0
            rts
