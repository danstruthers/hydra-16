; ****************************************************************************
; xmodem -r file, xmodem -s [-k] file - a file received, or sent, by XMODEM on the serial port (/dev/ser), with a
; terminal program on the PC (any that speaks XMODEM).  While it runs the line is its: the console keeps the
; windows' text (and /pc's frames) till it's done, then repaints the window shown, and the keys the console acts on
; as they come in (Ctrl-C ...) are bytes like the rest.
;   -r file     receive (the PC sends): the first block asked for with a CRC ('C', every 3 seconds; after 4 asks,
;               with a checksum, NAK, for a PC that knows only that), for a minute; 128-byte blocks (SOH) and 1K
;               ones (STX), each ACKed as it comes whole and right, or NAKed (the line let go quiet first), the file
;               written as they come.  The padding at the end of the last block (SUB, $1A) isn't kept
;   -s file     send (the PC receives): it waits a minute for the PC's 'C' (CRC) or NAK (checksum); 128-byte blocks
;               (-k: 1K ones, with a CRC; the last one 128 bytes if that's enough), the last padded with SUB; each
;               sent again on a NAK, or no answer in 10 seconds; then EOT
; 10 tries a block (or EOT), then it cancels (CAN CAN CAN): "too many errors"; CAN CAN from the PC, or Ctrl-C before
; the first block: "cancelled"; nothing for a minute: "no answer".  Done, it says the file's bytes ("file: 1234
; bytes").  At 115200, receive 128-byte blocks (the PC's plain XMODEM or XMODEM-CRC, not 1K): a 1K block comes in
; faster than it can be taken, as the console's receive ring holds 255 bytes (sending 1K blocks is fine).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "xmodem", main

F_R             = $01           ; -r
F_S             = $02           ; -s
F_K             = $04           ; -k
XM_SOH          = $01           ; A 128-byte block's start ...
XM_STX          = $02           ;   a 1K one's
XM_EOT          = $04           ; The end
XM_ACK          = $06
XM_NAK          = $15
XM_CAN          = $18           ; (Twice: cancelled)
XM_SUB          = $1A           ; The last block's padding
CTRL_C          = $03
TRIES           = 10            ; A block's tries
ASKS            = 20            ; -r: asks for the first block (T_ASK apart: a minute) ...
ASKS_CRC        = 4             ;   with a CRC ('C'), then with a checksum (NAK)
WAITS           = 60            ; -s: seconds for the receiver's first ask
T_BYTE          = TICK_HZ       ; The time (ticks) for a block's next byte, and the quiet before a NAK ...
T_ASK           = TICK_HZ * 3   ;   between asks ...
T_BLOCK         = TICK_HZ * 10  ;   for the next block, or a block's answer
IBUF            = 64            ; The line's bytes, read this many at a time
BLOCK_MAX       = 1024
DATA            = frame + 3     ; A block's data, after its start, its number and the number's complement

.zeropage
bp:         .res        2                                   ; A block's next byte
name:       .res        2                                   ; The file's name
why:        .res        2                                   ; What's wrong (fail's)

.bss
rfd:        .res        1                                   ; /dev/ser, for reading (not waiting) ...
wfd:        .res        1                                   ;   and for writing ...
ffd:        .res        1                                   ;   and the file
crcm:       .res        1                                   ; <> 0: blocks with a CRC, else a checksum
crc:        .res        2
sum:        .res        1
blk:        .res        1                                   ; The block's number (expected, or being sent)
num:        .res        1                                   ; (A block's number, as it came)
size:       .res        2                                   ; The block's size: 128 or 1024
cnt:        .res        2                                   ; (A count)
tries:      .res        1
asks:       .res        1
total:      .res        4                                   ; The file's bytes
pend:       .res        2                                   ; -r: SUBs held back (the end's padding, if it's last)
got:        .res        2                                   ; -s: the block's bytes from the file
t:          .res        2                                   ; (Scratch)
deadline:   .res        2                                   ; (get's)
ibuf:       .res        IBUF                                ; The line's bytes, as read ...
ilen:       .res        1                                   ;   how many ...
ipos:       .res        1                                   ;   and how many taken
out:        .res        1                                   ; A byte to send
subs:       .res        IBUF                                ; SUBs, to write
frame:      .res        3 + BLOCK_MAX + 2                   ; A block: its start, number, complement, data, check

.code
main:
            jsr         tl_start
            and         #F_R | F_S                          ; -r or -s, and a file
            beq         @usage
            cmp         #F_R | F_S
            beq         @usage
            lda         (tl_arg)
            beq         @usage
            MOVR        name, tl_arg
            jsr         tl_next
            bne         @usage
            stz         total
            stz         total + 1
            stz         total + 2
            stz         total + 3
            lda         tl_flags
            and         #F_R
            beq         :+
            jmp         receive
:
            jmp         send

@usage:
            jmp         tl_badusage

; ****************************************************************************
; Receiving

receive:
            MOVR        r0, name                            ; The file: emptied if it's there, else made
            lda         #O_WRITE | O_TRUNC
            jsr         OPEN
            bcc         @open
            cmp         #E_NOENT
            bne         @error
            MOVR        r0, name
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcc         @open
@error:
            jmp         file_error

@open:
            sta         ffd
            jsr         line_open
            stz         pend
            stz         pend + 1
            ldx         #IBUF - 1
            lda         #XM_SUB
:
            sta         subs,X
            dex
            bpl         :-
            lda         #1
            sta         blk
            stz         tries
            stz         asks
@ask:                                                       ; The first block asked for: with a CRC, then with a
            lda         asks                                ;   checksum
            cmp         #ASKS
            bcc         :+
            jmp         no_answer
:
            ldx         #1
            ldy         #'C'
            cmp         #ASKS_CRC
            bcc         :+
            ldx         #0
            ldy         #XM_NAK
:
            stx         crcm
            inc         asks
            tya
            jsr         put
@first:
            lda         #<T_ASK
            ldx         #>T_ASK
            jsr         get
            bcs         @ask                                ; (None: ask again)
            cmp         #CTRL_C
            bne         :+
            jmp         cancelled
:
            cmp         #XM_CAN
            beq         r_can
            cmp         #XM_EOT                             ; (An empty file)
            beq         r_eot
            cmp         #XM_SOH
            beq         r_block
            cmp         #XM_STX
            beq         r_block
            bra         @first                              ; (Anything else: not for us)

; The next block's start, or the end
r_next:
            lda         #<T_BLOCK
            ldx         #>T_BLOCK
            jsr         get
            bcs         r_none
            cmp         #XM_SOH
            beq         r_block
            cmp         #XM_STX
            beq         r_block
            cmp         #XM_EOT
            beq         r_eot
            cmp         #XM_CAN
            bne         r_none
r_can:                                                      ; CAN: cancelled, if another comes with it
            lda         #<T_BYTE
            ldx         #>T_BYTE
            jsr         get
            bcs         r_none
            cmp         #XM_CAN
            bne         r_none
            jmp         cancelled

r_none:                                                     ; (Nothing, or not a block's start)
            jmp         r_bad

; The end: ACKed, and the file's done (its SUBs held back dropped)
r_eot:
            lda         #XM_ACK
            jsr         put
            jmp         done

; A block, .A its start (SOH, STX): its number and complement, its data, its check
r_block:
            stz         size                                ; Its size: STX's 1K, SOH's 128
            ldx         #>BLOCK_MAX
            cmp         #XM_STX
            beq         :+
            lda         #128
            sta         size
            ldx         #0
:
            stx         size + 1
            jsr         r_byte
            sta         num
            jsr         r_byte
            eor         num
            cmp         #$FF
            beq         :+
            jmp         r_bad
:
            stz         crc
            stz         crc + 1
            stz         sum
            LDR         bp, DATA
            lda         size
            sta         cnt
            lda         size + 1
            sta         cnt + 1
@data:
            jsr         r_byte
            sta         (bp)
            pha
            clc
            adc         sum
            sta         sum
            pla
            jsr         crc_byte
            inc         bp
            bne         :+
            inc         bp + 1
:
            lda         cnt
            bne         :+
            dec         cnt + 1
:
            dec         cnt
            lda         cnt
            ora         cnt + 1
            bne         @data
            lda         crcm                                ; Its check: the CRC, high byte first, or the sum
            beq         @sum
            jsr         r_byte
            cmp         crc + 1
            bne         r_bad
            jsr         r_byte
            cmp         crc
            bne         r_bad
            bra         @checked

@sum:
            jsr         r_byte
            cmp         sum
            bne         r_bad
@checked:
            lda         num
            cmp         blk
            beq         @new
            inc         a                                   ; The last one again (its ACK lost): ACKed again
            cmp         blk
            beq         @ack
            lda         #<s_step                            ; Neither: out of step
            ldx         #>s_step
            jmp         cancel

@new:
            jsr         store
            inc         blk
            stz         tries
@ack:
            lda         #XM_ACK
            jsr         put
            jmp         r_next

; A block's next byte (T_BYTE ticks at most: else r_bad, from r_block's caller's place)
r_byte:
            lda         #<T_BYTE
            ldx         #>T_BYTE
            jsr         get
            bcs         :+
            rts
:
            pla                                             ; (Out of r_block)
            pla
                                                            ; (On to r_bad)

; Damaged, or cut short: a NAK once the line's quiet (10 tries)
r_bad:
            inc         tries
            lda         tries
            cmp         #TRIES
            bcc         :+
            jmp         too_many
:
            jsr         purge
            lda         #XM_NAK
            jsr         put
            jmp         r_next

; The block (size bytes at DATA) into the file: the SUBs at its end held back (pend), and the ones held before it
; written first if it has more than SUBs
store:
            lda         size                                ; t: its bytes but the SUBs at its end
            sta         t
            lda         size + 1
            sta         t + 1
@scan:
            lda         t
            ora         t + 1
            beq         @subs
            clc
            lda         #<(DATA - 1)
            adc         t
            sta         bp
            lda         #>(DATA - 1)
            adc         t + 1
            sta         bp + 1
            lda         (bp)
            cmp         #XM_SUB
            bne         @data
            lda         t
            bne         :+
            dec         t + 1
:
            dec         t
            bra         @scan

@subs:                                                      ; SUBs alone: held back, all of them
            clc
            lda         pend
            adc         size
            sta         pend
            lda         pend + 1
            adc         size + 1
            sta         pend + 1
            rts

@data:
@pend:                                                      ; Those held before it, written
            lda         pend
            ora         pend + 1
            beq         @block
            lda         #IBUF
            ldx         pend + 1
            bne         :+
            cmp         pend
            bcc         :+
            lda         pend
:
            sta         r1
            stz         r1 + 1
            sec
            lda         pend
            sbc         r1
            sta         pend
            lda         pend + 1
            sbc         #0
            sta         pend + 1
            LDR         r0, subs
            jsr         fwrite
            bra         @pend

@block:
            LDR         r0, DATA                            ; Its bytes, then its SUBs held back
            MOVR        r1, t
            jsr         fwrite
            sec
            lda         size
            sbc         t
            sta         pend
            lda         size + 1
            sbc         t + 1
            sta         pend + 1
            rts

; r1 bytes at r0 into the file, counted (an error: the transfer cancelled, and the error said)
fwrite:
            lda         r1
            ora         r1 + 1
            beq         @done
            clc
            lda         total
            adc         r1
            sta         total
            lda         total + 1
            adc         r1 + 1
            sta         total + 1
            bcc         :+
            inc         total + 2
            bne         :+
            inc         total + 3
:
            lda         ffd
            jsr         WRITE
            bcc         @done
            pha
            jsr         cancel_line
            pla
            jmp         file_error

@done:
            rts

; ****************************************************************************
; Sending

send:
            MOVR        r0, name
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         file_error
:
            sta         ffd
            jsr         line_open
            lda         #WAITS                              ; The receiver's first ask: 'C' (a CRC) or NAK (a
            sta         asks                                ;   checksum), a minute at most
@wait:
            lda         #<T_BYTE
            ldx         #>T_BYTE
            jsr         get
            bcc         :+
            dec         asks
            bne         @wait
            jmp         no_answer
:
            ldx         #1
            cmp         #'C'
            beq         @go
            ldx         #0
            cmp         #XM_NAK
            beq         @go
            cmp         #CTRL_C
            beq         :+
            cmp         #XM_CAN
            bne         @wait
:
            jmp         cancelled

@go:
            stx         crcm
            lda         #1
            sta         blk

; The next block: as much of the file as a block takes (none: the end), padded with SUB
s_next:
            stz         size                                ; 1K blocks with -k, and a CRC
            lda         #>BLOCK_MAX
            sta         size + 1
            lda         tl_flags
            and         #F_K
            beq         :+
            lda         crcm
            bne         :++
:
            lda         #128
            sta         size
            stz         size + 1
:
            jsr         fill
            lda         got
            ora         got + 1
            bne         :+
            jmp         s_eot
:
            lda         got + 1                             ; (A 1K block's worth of 128 bytes or less: a
            bne         :+                                  ;   128-byte block)
            lda         got
            cmp         #128 + 1
            bcs         :+
            lda         #128
            sta         size
            stz         size + 1
:
            clc                                             ; Counted
            lda         total
            adc         got
            sta         total
            lda         total + 1
            adc         got + 1
            sta         total + 1
            bcc         :+
            inc         total + 2
            bne         :+
            inc         total + 3
:
            lda         #XM_SOH                             ; Its start, number and complement
            ldx         size + 1
            beq         :+
            lda         #XM_STX
:
            sta         frame
            lda         blk
            sta         frame + 1
            eor         #$FF
            sta         frame + 2
            stz         crc                                 ; The padding, and the check over the data
            stz         crc + 1
            stz         sum
            LDR         bp, DATA
            lda         size
            sta         cnt
            lda         size + 1
            sta         cnt + 1
            stz         t                                   ; (t: the bytes so far)
            stz         t + 1
@byte:
            lda         t                                   ; Past the file's bytes: SUB
            cmp         got
            lda         t + 1
            sbc         got + 1
            bcc         :+
            lda         #XM_SUB
            sta         (bp)
:
            lda         (bp)
            pha
            clc
            adc         sum
            sta         sum
            pla
            jsr         crc_byte
            inc         bp
            bne         :+
            inc         bp + 1
:
            inc         t
            bne         :+
            inc         t + 1
:
            lda         cnt
            bne         :+
            dec         cnt + 1
:
            dec         cnt
            lda         cnt
            ora         cnt + 1
            bne         @byte
            lda         crcm                                ; The check after the data: the CRC (high byte first),
            beq         @sum                                ;   or the sum
            lda         crc + 1
            sta         (bp)
            ldy         #1
            lda         crc
            sta         (bp),Y
            bra         @checked

@sum:
            lda         sum
            sta         (bp)
@checked:
            stz         tries

; The block out, and its answer: ACK, the next; NAK or nothing, again
s_try:
            clc                                             ; Its length: 3, the data, the check
            lda         size
            adc         #3 + 1
            sta         r1
            lda         size + 1
            adc         #0
            sta         r1 + 1
            lda         crcm
            beq         :+
            inc         r1                                  ; (Never a carry: 128 or 1024, and 4)
:
            LDR         r0, frame
            lda         wfd
            jsr         WRITE
@answer:
            lda         #<T_BLOCK
            ldx         #>T_BLOCK
            jsr         get
            bcs         s_again
            cmp         #XM_ACK
            beq         @acked
            cmp         #XM_NAK
            beq         s_again
            cmp         #XM_CAN
            bne         @answer                             ; (Anything else: not an answer)
            lda         #<T_BYTE
            ldx         #>T_BYTE
            jsr         get
            bcs         s_again
            cmp         #XM_CAN
            bne         s_again
            jmp         cancelled

@acked:
            inc         blk
            jmp         s_next

s_again:
            inc         tries
            lda         tries
            cmp         #TRIES
            bcc         s_try
            jmp         too_many

; The end: EOT, till it's ACKed
s_eot:
            stz         tries
@eot:
            lda         #XM_EOT
            jsr         put
            lda         #<T_BLOCK
            ldx         #>T_BLOCK
            jsr         get
            bcs         :+
            cmp         #XM_ACK
            beq         done
:
            inc         tries
            lda         tries
            cmp         #TRIES
            bcc         @eot
            jmp         too_many

; got = the file's next bytes, size at most (fewer only at its end: a read can come short)
fill:
            stz         got
            stz         got + 1
@read:
            clc
            lda         #<DATA
            adc         got
            sta         r0
            lda         #>DATA
            adc         got + 1
            sta         r0 + 1
            sec
            lda         size
            sbc         got
            sta         r1
            lda         size + 1
            sbc         got + 1
            sta         r1 + 1
            ora         r1
            beq         @done
            lda         ffd
            jsr         READ
            bcc         :+
            pha
            jsr         cancel_line
            pla
            jmp         file_error
:
            sta         t
            stx         t + 1
            ora         t + 1
            beq         @done
            clc
            lda         got
            adc         t
            sta         got
            lda         got + 1
            adc         t + 1
            sta         got + 1
            bra         @read

@done:
            rts

; ****************************************************************************
; The end

; Done: the line the console's again, and the file's bytes said ("file: 1234 bytes")
done:
            jsr         line_close
            lda         ffd
            jsr         CLOSE
            MOVR        r0, name
            jsr         tl_puts
            LDR         r0, s_colon
            jsr         tl_puts
            ldx         #3
:
            lda         total,X
            sta         tl_num,X
            dex
            bpl         :-
            lda         #1
            jsr         tl_dec
            LDR         r0, s_bytes
            jsr         tl_puts
            jmp         tl_end

no_answer:
            lda         #<s_noanswer
            ldx         #>s_noanswer
            bra         ended

cancelled:
            lda         #<s_cancelled
            ldx         #>s_cancelled
            bra         ended

too_many:
            lda         #<s_toomany
            ldx         #>s_toomany
                                                            ; (On to cancel)

; Cancelled from here (CAN CAN CAN), why .A/.X: the line given back, "xmodem: file: why" on fd 2, and the end, why
; its status
cancel:
            pha
            phx
            jsr         cancel_line
            plx
            pla
            bra         fail

; Ended, why .A/.X (the PC knows): the same, without the CANs
ended:
            pha
            phx
            jsr         line_close
            plx
            pla
fail:
            sta         why
            stx         why + 1
            MOVR        r0, name
            jsr         tl_prefix
            LDR         r0, s_colon
            jsr         tl_puts2
            MOVR        r0, why
            jsr         tl_puts2
            LDR         r0, tl_s_nl
            jsr         tl_puts2
            MOVR        r0, why
            lda         #1
            jmp         EXITS

; The file's error .A: "xmodem: file: why" (the line given back first, if it's open), and the end
file_error:
            pha
            jsr         line_close
            MOVR        r0, name
            pla
            jsr         tl_err
            jmp         tl_end

; ****************************************************************************
; The line

; /dev/ser opened: for reading, not waiting (rfd), and for writing (wfd).  The line is this program's from here (an
; error: said, and the end)
line_open:
            lda         #$FF
            sta         rfd
            sta         wfd
            stz         ilen
            stz         ipos
            LDR         r0, s_ser
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            bcs         @error
            sta         rfd
            LDR         r0, s_ser
            lda         #O_WRITE
            jsr         OPEN
            bcs         @error
            sta         wfd
            rts

@error:
            pha
            jsr         line_close
            LDR         r0, s_ser
            pla
            jsr         tl_err
            jmp         tl_end

; The line given back: the console's again (it repaints the window shown)
line_close:
            lda         rfd
            bmi         :+
            jsr         CLOSE
:
            lda         wfd
            bmi         :+
            jsr         CLOSE
:
            lda         #$FF
            sta         rfd
            sta         wfd
            rts

; CAN CAN CAN (the PC's end of it cancelled), and the line given back
cancel_line:
            lda         wfd
            bmi         :+
            lda         #XM_CAN
            jsr         put
            lda         #XM_CAN
            jsr         put
            lda         #XM_CAN
            jsr         put
:
            jmp         line_close

; .A to the line
put:
            sta         out
            LDR         r0, out
            LDR         r1, 1
            lda         wfd
            jmp         WRITE

; The line's next byte, waiting .A/.X ticks at most (it's read IBUF bytes at a time, without waiting: the program
; sleeps a tick between looks).  OUT: C = 0, .A = it; or C = 1: none came
get:
            ldy         ipos
            cpy         ilen
            bcc         @have
            sta         t                                   ; The time it's waited for
            stx         t + 1
            jsr         TICKS
            clc
            adc         t
            sta         deadline
            txa
            adc         t + 1
            sta         deadline + 1
@look:
            LDR         r0, ibuf
            LDR         r1, IBUF
            lda         rfd
            jsr         READ
            bcs         @none
            cmp         #0
            beq         @none
            sta         ilen
            ldy         #0
@have:
            lda         ibuf,Y
            iny
            sty         ipos
            clc
            rts

@none:
            jsr         TICKS                               ; Its time come?  (Now - then: not negative)
            sec
            sbc         deadline
            txa
            sbc         deadline + 1
            bpl         @late
            lda         #1
            ldx         #0
            jsr         SLEEP
            bra         @look

@late:
            sec
            rts

; The line let go quiet (nothing for T_BYTE ticks), what came dropped
purge:
            lda         #<T_BYTE
            ldx         #>T_BYTE
            jsr         get
            bcc         purge
            rts

; .A into the CRC (CRC-16 XMODEM: $1021, from 0; Greg Cook's, a byte at a time, no table).  Modifies .A, .X, .Y
crc_byte:
            eor         crc + 1
            sta         crc + 1
            lsr
            lsr
            lsr
            lsr
            tax
            asl
            eor         crc
            sta         crc
            txa
            eor         crc + 1
            sta         crc + 1
            asl
            asl
            asl
            tax
            asl
            asl
            eor         crc + 1
            tay
            txa
            rol
            eor         crc
            sta         crc + 1
            sty         crc
            rts

.rodata
s_ser:      .byte       "/dev/ser", 0
s_colon:    .byte       ": ", 0
s_bytes:    .byte       " bytes", LF, 0
s_noanswer: .byte       "no answer", 0
s_cancelled: .byte      "cancelled", 0
s_toomany:  .byte       "too many errors", 0
s_step:     .byte       "out of step", 0
tl_name:    .byte       "xmodem", 0
tl_flagset: .byte       "rsk", 0
tl_usage:   .byte       "xmodem -r file | -s [-k] file", 0

.include "toollib.s"
