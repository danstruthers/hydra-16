.debuginfo

; ****************************************************************************
; /dev/gpio: the VIA's port A on J27 (BIOS ROM page D; included inside `.scope PAGED`, see all.s), its pins, and
; CA1 and CA2.  Served in its client's task (IO_DEV_CALLER_TASK: the shell registers it), with IRQs off for each
; change to the VIA's registers, so another task's can't come between a read and its write.
;   /dev/gpio/N         N = 0-7: pin PA N.  Read: "0" or "1" (and CR LF), from the offset (cat reads it once;
;                       a program polling it seeks back to 0).  Write: "0" or "1": the pin's level, and the pin
;                       an output.  PA0 and PA1 are the I2C bus's SCL and SDA too (pulled up on the board).
;   /dev/gpio/port      all 8 pins, a byte: a read gives them (any offset); a write sets the outputs.
;   /dev/gpio/ctl       read: a line a pin ("2 out 1", "3 in 0"), then CA1's ("ca1 fall 0003": its active
;                       edge and the edges counted) and CA2's ("ca2 in", "ca2 0" or "ca2 1").  Write a command:
;                       "in N", "out N", "ddr HH" (all 8 pins' directions, hex: 1 = out), "ca1 rise", "ca1 fall",
;                       "ca2 0", "ca2 1" (CA2 an output, low or high).
;   /dev/gpio/ca1       a read waits for CA1's next active edge (one since this task last read it, or opened it),
;                       then gives the edges counted so far ("0003" and CR LF).  CA1's interrupt is on while the
;                       file is open; VIA_IRQ_HANDLER (task 0) counts the edges (GPIO_CA1N) and wakes the readers
;                       waiting (GPIO_CA1W).
; Port A is read and written as ORA without the handshake (VIA_R_PORTA_NOHS), which leaves CA1's flag alone.
; Server ZP: ZP_CS (the client-task servers' scratch: GPIO_*, below); ZP_IO_REQ.

GPIO_FID        = ZP_CS + 0                             ; The fid
GPIO_LEN        = ZP_CS + 1                             ; A read's text: its length so far
GPIO_T          = ZP_CS + 2                             ; (Temporaries)
GPIO_N          = ZP_CS + 3                             ;   (2)
            CS_FITS     GPIO_FID, 5

.segment "GPIO_PD"

; A request.  IN: .A = request, .X = client, .Y = fid
GPIO_SERVE:
            sty         GPIO_FID
            cmp         #H9_OPEN
            beq         GPIO_OPEN
            cmp         #H9_CREATE
            bcs         GPIO_BAD
            pha
            jsr         IO_SRV_MAP                          ; (.X: the client)
            pla
            ldx         GPIO_FID
            cpx         #GPIO_FID_CA1
            bcs         @ca1
            cmp         #H9_READ
            bne         :+
            jmp         GPIO_READ
:
            cmp         #H9_WRITE
            bne         :+
            jmp         GPIO_WRITE
:
            cmp         #H9_CLUNK
            beq         GPIO_OK_UNMAP
            cmp         #H9_DUP
            beq         GPIO_OK_UNMAP
            bra         GPIO_BAD_UNMAP

@ca1:
            cmp         #H9_READ
            bne         :+
            jmp         GPIO_CA1_READ
:
            cmp         #H9_DUP
            bne         :+
            jsr         GPIO_CA1_REF                        ; (Another fd on it)
            bra         GPIO_OK_UNMAP
:
            cmp         #H9_CLUNK
            bne         GPIO_BAD_UNMAP
            jsr         GPIO_CA1_UNREF

GPIO_OK_UNMAP:
            jsr         IO_SRV_UNMAP

GPIO_OK:
            lda         #0
            clc
            rts

GPIO_BAD_UNMAP:
            jsr         IO_SRV_UNMAP

GPIO_BAD:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

; The rest of the name is in the data area: "/N" (0-7), "/port", "/ctl" or "/ca1".  OUT: .A = the fid
GPIO_OPEN:
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
            lda         (ZP_IO_REQ),Y
            cmp         #'/'
            bne         @not_found
            iny
            lda         (ZP_IO_REQ),Y                       ; A pin: a digit 0-7, and the end
            sec
            sbc         #'0'
            cmp         #8
            bcs         @name
            sta         GPIO_T
            iny
            lda         (ZP_IO_REQ),Y
            bne         @not_found
            lda         GPIO_T
            bra         @open                               ; (GPIO_FID_PIN | the pin)

@name:
            ldx         #0                                  ; Or one of GPIO_NAMES (GPIO_T: which)
            stz         GPIO_T

@try:
            ldy         #1
:
            lda         GPIO_NAMES,X
            cmp         (ZP_IO_REQ),Y
            bne         @skip
            inx
            iny
            cmp         #0
            bne         :-
            ldx         GPIO_T                              ; (Its fid)
            lda         GPIO_NAME_FIDS,X
            cmp         #GPIO_FID_CA1
            bne         @open
            jsr         GPIO_CA1_REF                        ; ca1: CA1's interrupt on; this task's count from now
            jsr         GPIO_CA1_COUNT
            dec         ZP_IO_REQ + 1                       ;   (ZP_IO_REQ: back at the request block)
            ldy         #IO_BLK_CA1
            sta         (ZP_IO_REQ),Y
            iny
            txa
            sta         (ZP_IO_REQ),Y
            inc         ZP_IO_REQ + 1
            lda         #GPIO_FID_CA1
            bra         @open

@skip:
            lda         GPIO_NAMES,X                        ; Not it: the next
            inx
            cmp         #0
            bne         @skip
            inc         GPIO_T
            lda         GPIO_NAMES,X
            bne         @try

@not_found:
            lda         #ERR_IO_NOT_FOUND
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            sec
            rts

@open:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (Keeps .A)
            clc
            rts

GPIO_NAMES:     .byte   "port", 0, "ctl", 0, "ca1", 0, 0
GPIO_NAME_FIDS: .byte   GPIO_FID_PORT, GPIO_FID_CTL, GPIO_FID_CA1
GPIO_BITS:      .byte   $01, $02, $04, $08, $10, $20, $40, $80

; CA1's fds: one more (its interrupt on with the first), or one fewer (off with the last).  A quick look at the
; system task's GPIO_CA1REFS.  Modifies: .A, .Y
GPIO_CA1_REF:
            php
            sei
            ldy         T_REGISTER
            stz         T_REGISTER                          ; Quick look (no stack use!)
            inc         GPIO_CA1REFS
            lda         GPIO_CA1REFS
            sty         T_REGISTER
            cmp         #1
            bne         @done
            lda         #VIA_CA1_INT_BIT                    ; The first: its flag clear (an old edge isn't one),
            sta         VIA_R_INT_FLAGS                     ;   and its interrupt on
            lda         #VIA_INT_ENABLE | VIA_CA1_INT_BIT
            sta         VIA_R_INT_ENABLE

@done:
            plp
            rts

GPIO_CA1_UNREF:
            php
            sei
            ldy         T_REGISTER
            stz         T_REGISTER                          ; Quick look (no stack use!)
            dec         GPIO_CA1REFS
            lda         GPIO_CA1REFS
            sty         T_REGISTER
            bne         :+
            lda         #VIA_CA1_INT_BIT                    ; The last: its interrupt off
            sta         VIA_R_INT_ENABLE
:
            plp
            rts

; .A.X = CA1's edges counted (GPIO_CA1N: a quick look at the system task).  Modifies: .Y
GPIO_CA1_COUNT:
            php
            sei
            ldy         T_REGISTER
            stz         T_REGISTER                          ; Quick look (no stack use!)
            lda         GPIO_CA1N
            ldx         GPIO_CA1N + 1
            sty         T_REGISTER
            plp
            rts

; Read /dev/gpio/ca1: an edge since this task's last read (its IO_BLK_CA1)?  Its count, as text; else wait
; (ERR_IO_WOULD_BLOCK, the task in GPIO_CA1W: the next edge wakes it, and the IO layer asks again).  The look,
; and the wait asked for, with IRQs off together, so an edge can't come between them.
GPIO_CA1_READ:
            php
            sei
            jsr         GPIO_CA1_COUNT
            sta         GPIO_N
            stx         GPIO_N + 1
            ldy         #IO_BLK_CA1
            cmp         (ZP_IO_REQ),Y
            bne         @edge
            iny
            txa
            cmp         (ZP_IO_REQ),Y
            bne         @edge
            lda         T_REGISTER                          ; None yet: this task waits for the next
            and         #$0F
            tax
            and         #7
            tay
            lda         GPIO_BITS,Y                         ; (Its bit)
            ldy         T_REGISTER
            stz         T_REGISTER                          ; Quick look (no stack use!)
            cpx         #8
            bcs         :+
            tsb         GPIO_CA1W
            bra         :++
:
            tsb         GPIO_CA1W + 1
:
            sty         T_REGISTER
            plp
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_WOULD_BLOCK
            sec
            rts

@edge:
            plp
            ldy         #IO_BLK_CA1                         ; Seen: the count now
            lda         GPIO_N
            sta         (ZP_IO_REQ),Y
            iny
            lda         GPIO_N + 1
            sta         (ZP_IO_REQ),Y
            stz         GPIO_LEN                            ; The text: the count, CR LF
            lda         GPIO_N + 1
            jsr         GPIO_PUT_BYTE
            lda         GPIO_N
            jsr         GPIO_PUT_BYTE
            jsr         GPIO_PUT_CRLF
            jsr         GPIO_COUNT_MIN                      ; (As many as asked, at most: from its start)
            jmp         GPIO_COUNTED

; Read a pin, the port or ctl
GPIO_READ:
            lda         GPIO_FID
            cmp         #GPIO_FID_PORT
            bcc         @pin
            bne         @ctl
            lda         VIA_R_PORTA_NOHS                    ; port: the byte (any offset)
            inc         ZP_IO_REQ + 1
            sta         (ZP_IO_REQ)
            dec         ZP_IO_REQ + 1
            lda         #1
            jmp         GPIO_COUNTED

@pin:
            stz         GPIO_LEN                            ; "0" or "1", CR LF
            tax
            lda         GPIO_BITS,X
            and         VIA_R_PORTA_NOHS
            jsr         GPIO_PUT_BIT
            jsr         GPIO_PUT_CRLF
            jmp         GPIO_TEXT_OUT

@ctl:
            stz         GPIO_LEN                            ; A line a pin: "N in 1", "N out 0"
            ldx         #0

@line:
            txa
            ora         #'0'
            jsr         GPIO_PUT
            lda         #' '
            jsr         GPIO_PUT
            lda         GPIO_BITS,X
            and         VIA_R_DDRA
            beq         :+
            lda         #'o'                                ; "out"
            jsr         GPIO_PUT
            lda         #'u'
            jsr         GPIO_PUT
            lda         #'t'
            bra         :++
:
            lda         #'i'                                ; "in"
            jsr         GPIO_PUT
            lda         #'n'
:
            jsr         GPIO_PUT
            lda         #' '
            jsr         GPIO_PUT
            lda         GPIO_BITS,X
            and         VIA_R_PORTA_NOHS
            jsr         GPIO_PUT_BIT
            jsr         GPIO_PUT_CRLF
            inx
            cpx         #8
            bne         @line
            ldx         #0                                  ; "ca1 fall NNNN" (or rise)
            jsr         GPIO_PUT_WORD
            ldx         #GPIO_W_FALL
            lda         VIA_R_PER_CTRL
            and         #VIA_PCR_CA1_RISE
            beq         :+
            ldx         #GPIO_W_RISE
:
            jsr         GPIO_PUT_WORD
            jsr         GPIO_CA1_COUNT
            pha
            txa
            jsr         GPIO_PUT_BYTE
            pla
            jsr         GPIO_PUT_BYTE
            jsr         GPIO_PUT_CRLF
            ldx         #GPIO_W_CA2                         ; "ca2 in", or "ca2 0" / "ca2 1" (an output)
            jsr         GPIO_PUT_WORD
            lda         VIA_R_PER_CTRL
            and         #VIA_PCR_CA2
            cmp         #VIA_PCR_CA2_LOW
            bcs         :+
            ldx         #GPIO_W_IN
            jsr         GPIO_PUT_WORD
            dec         GPIO_LEN                            ; (Its space: none at the end)
            bra         :++
:
            and         #$02                                ; (_HIGH's bit)
            jsr         GPIO_PUT_BIT
:
            jsr         GPIO_PUT_CRLF

; The text made in the data area (GPIO_LEN bytes): what's after the fd's offset, up to the count
GPIO_TEXT_OUT:
            ldy         #IO_BLK_OFS + 3                     ; Past the end: nothing more (end of file)
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @eof
            dey
            lda         (ZP_IO_REQ),Y
            cmp         GPIO_LEN
            bcs         @eof
            tax                                             ; .X = the offset
            inc         ZP_IO_REQ + 1
            ldy         #0                                  ; Move the text after it down

@move:
            phy
            txa
            tay
            lda         (ZP_IO_REQ),Y
            ply
            sta         (ZP_IO_REQ),Y
            iny
            inx
            cpx         GPIO_LEN
            bne         @move
            dec         ZP_IO_REQ + 1
            sty         GPIO_LEN                            ; (What's left after the offset)
            jsr         GPIO_COUNT_MIN
            bra         GPIO_COUNTED

@eof:
            lda         #0

; The count done: .A (0-255).  Then unmapped
GPIO_COUNTED:
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            lda         #0
            sta         (ZP_IO_REQ),Y
            jmp         GPIO_OK_UNMAP

; .A = GPIO_LEN, or the request's count if it's less
GPIO_COUNT_MIN:
            ldy         #IO_BLK_COUNT + 1
            lda         (ZP_IO_REQ),Y
            bne         @len                                ; (256 asked)
            dey
            lda         (ZP_IO_REQ),Y
            cmp         GPIO_LEN
            bcc         @done

@len:
            lda         GPIO_LEN

@done:
            rts

; The text: .A; "0" or "1" (.A = 0 or not); CR LF; a word of GPIO_WORDS (.X: its offset) and a space; a byte
; as 2 hex digits.  (In the data area, at GPIO_LEN.)  Preserve .X (but GPIO_PUT_WORD)
GPIO_PUT_BIT:
            cmp         #1                                  ; (C = 1: not 0)
            lda         #'0'
            adc         #0
            bra         GPIO_PUT

GPIO_PUT_CRLF:
            lda         #ASCII_CR
            jsr         GPIO_PUT
            lda         #ASCII_LF

GPIO_PUT:
            phy
            inc         ZP_IO_REQ + 1
            ldy         GPIO_LEN
            sta         (ZP_IO_REQ),Y
            dec         ZP_IO_REQ + 1
            inc         GPIO_LEN
            ply
            rts

GPIO_PUT_WORD:
            lda         GPIO_WORDS,X
            beq         @space
            jsr         GPIO_PUT
            inx
            bra         GPIO_PUT_WORD

@space:
            lda         #' '
            bra         GPIO_PUT

GPIO_PUT_BYTE:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         @digit
            pla
            and         #$0F

@digit:
            cmp         #10
            bcc         :+
            adc         #'A' - '9' - 2                      ; (C = 1)
:
            adc         #'0'
            bra         GPIO_PUT

GPIO_WORDS:     .byte   "ca1", 0
GPIO_W_FALL     = * - GPIO_WORDS
                .byte   "fall", 0
GPIO_W_RISE     = * - GPIO_WORDS
                .byte   "rise", 0
GPIO_W_CA2      = * - GPIO_WORDS
                .byte   "ca2", 0
GPIO_W_IN       = * - GPIO_WORDS
                .byte   "in", 0
GPIO_W_OUT      = * - GPIO_WORDS
                .byte   "out", 0
GPIO_W_DDR      = * - GPIO_WORDS
                .byte   "ddr", 0

; Write a pin ("0" or "1": its level, and it an output), the port (a byte: the outputs) or ctl (a command).  The
; whole write is taken
GPIO_WRITE:
            inc         ZP_IO_REQ + 1                       ; (The data area: ZP_IO_REQ, till GPIO_DONE)
            lda         (ZP_IO_REQ)                         ; Its first byte
            ldx         GPIO_FID
            cpx         #GPIO_FID_PORT
            bcc         @pin
            bne         GPIO_CTL_WRITE
            sta         VIA_R_PORTA_NOHS                    ; port: the outputs
            bra         GPIO_DONE

@pin:
            sec
            sbc         #'0'
            cmp         #2
            bcs         GPIO_REFUSE
            php
            sei
            ldy         GPIO_BITS,X
            tax                                             ; (0 or 1)
            tya
            ora         VIA_R_DDRA                          ; An output ...
            sta         VIA_R_DDRA
            tya
            cpx         #0
            beq         :+
            ora         VIA_R_PORTA_NOHS                    ; ... high
            bra         :++
:
            eor         #$FF                                ; ... low
            and         VIA_R_PORTA_NOHS
:
            sta         VIA_R_PORTA_NOHS
            plp

GPIO_DONE:
            dec         ZP_IO_REQ + 1
            jmp         GPIO_OK_UNMAP                       ; (The count stays: all of it taken)

GPIO_REFUSE:
            dec         ZP_IO_REQ + 1
            jmp         GPIO_BAD_UNMAP

; A ctl command (ZP_IO_REQ: the data area): "in N", "out N", "ddr HH", "ca1 rise", "ca1 fall", "ca2 0", "ca2 1"
GPIO_CTL_WRITE:
            ldx         #GPIO_W_IN
            jsr         GPIO_WORD_IS
            bcc         @in
            ldx         #GPIO_W_OUT
            jsr         GPIO_WORD_IS
            bcc         @out
            ldx         #GPIO_W_DDR
            jsr         GPIO_WORD_IS
            bcc         @ddr
            ldx         #GPIO_W_CA2
            jsr         GPIO_WORD_IS
            bcc         @ca2
            ldx         #0                                  ; "ca1"
            jsr         GPIO_WORD_IS
            bcs         GPIO_REFUSE
            ldx         #GPIO_W_RISE
            jsr         GPIO_WORD_AT
            lda         #VIA_PCR_CA1_RISE
            bcc         @set_pcr
            ldx         #GPIO_W_FALL
            jsr         GPIO_WORD_AT
            bcs         GPIO_REFUSE
            lda         #0

@set_pcr:                                                   ; .A = CA1's edge bit, with PCR's others
            php
            sei
            sta         GPIO_T
            lda         VIA_R_PER_CTRL
            and         #<~VIA_PCR_CA1_RISE
            ora         GPIO_T
            bra         @pcr

@ca2:
            jsr         GPIO_ARG_BIT                        ; .A = 0 or 1
            bcs         GPIO_REFUSE
            asl                                             ; ($02: high)
            ora         #VIA_PCR_CA2_LOW
            php
            sei
            sta         GPIO_T
            lda         VIA_R_PER_CTRL
            and         #<~VIA_PCR_CA2
            ora         GPIO_T

@pcr:
            sta         VIA_R_PER_CTRL
            plp
            bra         GPIO_DONE

@in:
            jsr         GPIO_ARG_PIN                        ; .A = the pin's bit
            bcs         GPIO_REFUSE
            eor         #$FF
            php
            sei
            and         VIA_R_DDRA
            bra         @set_ddr

@out:
            jsr         GPIO_ARG_PIN
            bcs         GPIO_REFUSE
            php
            sei
            ora         VIA_R_DDRA

@set_ddr:
            sta         VIA_R_DDRA
            plp
            bra         @done

@ddr:
            lda         (ZP_IO_REQ),Y                       ; Two hex digits
            jsr         GPIO_HEX
            bcs         @refuse
            asl
            asl
            asl
            asl
            sta         GPIO_T
            iny
            lda         (ZP_IO_REQ),Y
            jsr         GPIO_HEX
            bcs         @refuse
            ora         GPIO_T
            sta         VIA_R_DDRA
            bra         @done

@done:
            jmp         GPIO_DONE

@refuse:
            jmp         GPIO_REFUSE

; Does the command start with word .X of GPIO_WORDS, and a space?  OUT: C = 0 yes, .Y = after the space.
; Modifies: .A, .X
GPIO_WORD_IS:
            ldy         #0

; ... at .Y (and then the end, a space, CR or LF: GPIO_WORD_AT's)
GPIO_WORD_AT_Y:
            lda         GPIO_WORDS,X
            beq         @end
            cmp         (ZP_IO_REQ),Y
            bne         @no
            inx
            iny
            bra         GPIO_WORD_AT_Y

@end:
            lda         (ZP_IO_REQ),Y
            cmp         #' '
            bne         @no
            iny
            clc
            rts

@no:
            sec
            rts

; Is word .X of GPIO_WORDS the argument, at .Y (then the line's end: 0, CR, LF, or the write's end)?  OUT: C = 0
; yes.  Modifies: .A, .X; preserves .Y
GPIO_WORD_AT:
            phy
:
            lda         GPIO_WORDS,X
            beq         @end
            cmp         (ZP_IO_REQ),Y
            bne         @no
            inx
            iny
            bra         :-

@end:
            jsr         GPIO_ARG_END
            ply
            rts

@no:
            ply
            sec
            rts

; Is .Y the argument's end: the write's end, or 0, CR or LF there?  OUT: C = 0 yes.  Modifies: .A
GPIO_ARG_END:
            dec         ZP_IO_REQ + 1                       ; (The count: the request block's)
            phy
            ldy         #IO_BLK_COUNT + 1
            lda         (ZP_IO_REQ),Y
            bne         @more                               ; (256: more)
            dey
            lda         (ZP_IO_REQ),Y
            sta         GPIO_T
            ply
            phy
            cpy         GPIO_T
            bcs         @yes                                ; (The write ends here)

@more:
            ply
            inc         ZP_IO_REQ + 1
            lda         (ZP_IO_REQ),Y
            cmp         #ASCII_CR + 1
            bcs         @not
            cmp         #0
            beq         @end
            cmp         #ASCII_CR
            beq         @end
            cmp         #ASCII_LF
            beq         @end

@not:
            sec
            rts

@yes:
            ply
            inc         ZP_IO_REQ + 1

@end:
            clc
            rts

; The argument at .Y: a pin (0-7: OUT .A = its bit), or "0"/"1" (GPIO_ARG_BIT: .A = 0 or 1), then its end.
; OUT: C = 0; or C = 1 (not one).  Modifies: .X
GPIO_ARG_PIN:
            lda         (ZP_IO_REQ),Y
            sec
            sbc         #'0'
            cmp         #8
            bcs         @no
            tax
            iny
            jsr         GPIO_ARG_END
            bcs         @no
            lda         GPIO_BITS,X
            rts

@no:
            sec
            rts

GPIO_ARG_BIT:
            lda         (ZP_IO_REQ),Y
            sec
            sbc         #'0'
            cmp         #2
            bcs         @no
            tax
            iny
            jsr         GPIO_ARG_END
            txa
            rts

@no:
            sec
            rts

; .A = a hex digit's value (0-F, either case).  OUT: C = 0; or C = 1 (not one)
GPIO_HEX:
            ora         #$20                                ; (Lower case; digits stay)
            sec
            sbc         #'0'
            cmp         #10
            bcc         @ok
            sbc         #'a' - '0' - 10                     ; (C = 1)
            cmp         #10
            bcc         @no
            cmp         #16
            bcs         @no

@ok:
            clc
            rts

@no:
            sec
            rts
