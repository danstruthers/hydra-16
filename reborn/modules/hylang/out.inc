; ****************************************************************************
; out.inc - hylang's output, in the task's RAM (every bank writes through it): to stdout, a buffer at a time; or,
; while a string is being made (cap_begin to cap_end: +, format, repr, output-of, an error's message), to the
; capture bank, and the string made from it.  Captures nest, each the bytes from where it began.

OBUF_SIZE       = 128           ; The output written this much at a time
CAP_MAX         = $1FF8         ; The capture bank's bytes at most (a string's most: a blob's)

.zeropage
op:         .res        2                                   ; out_text's text
cp:         .res        2                                   ; (out_byte's place in the capture bank)

.bss
obuf:       .res        OBUF_SIZE                           ; The output not written yet
olen:       .res        1
cap_bank:   .res        1                                   ; The capture bank ...
cap_on:     .res        1                                   ;   the captures open ...
cap_len:    .res        2                                   ;   its bytes so far ...
cap_over:   .res        1                                   ;   and <> 0 if one ran out of room
ce_start:   .res        2                                   ; (cap_end's: where the capture began, the string's
ce_blob:    .res        1                                   ;   blob's bank, bytes on their way to it, and how many)
ce_buf:     .res        32
ce_n:       .res        1
msg_bank:   .res        1                                   ; (msg_out's and msg_err's: the bank that called)
od_n:       .res        2                                   ; (out_dec's: the magnitude, a digit, a difference)
od_digit:   .res        1
od_diff:    .res        1

.segment "DATA4"
; .A out (to stdout, or the capture open).  Keeps .A, .X, .Y and the window ($00)
out_byte:
            phy
            ldy         cap_on
            bne         @cap
            ldy         olen
            sta         obuf,y
            iny
            sty         olen
            cpy         #OBUF_SIZE
            bcc         :+
            jsr         out_flush
:
            ply
            rts
@cap:
            phx
            ldx         cap_len + 1                         ; (Room: below CAP_MAX)
            cpx         #>CAP_MAX
            bcc         @room
            bne         @full
            ldx         cap_len
            cpx         #<CAP_MAX
            bcs         @full
@room:
            pha
            lda         cap_len
            sta         cp
            lda         cap_len + 1
            ora         #$80
            sta         cp + 1
            ldx         $00
            lda         cap_bank
            sta         $00
            pla
            sta         (cp)
            stx         $00
            inc         cap_len
            bne         @done
            inc         cap_len + 1
            bra         @done
@full:
            ldx         #1
            stx         cap_over
@done:
            plx
            ply
            rts

; What's out written, to stdout (a failure: no matter).  Keeps .X, .Y and the window ($00)
out_flush:
            lda         olen
            beq         @done
            phx
            phy
            sta         r1
            stz         r1 + 1
            lda         $00
            pha
            LDR         r0, obuf
            lda         #1
            jsr         WRITE
            pla
            sta         $00
            stz         olen
            ply
            plx
@done:
            rts

; The zero-terminated text at .A, .X (low, high: in the task's RAM, or in the caller's bank) out.  Keeps .Y
out_text:
            sta         op
            stx         op + 1
            phy
            ldy         #0
@byte:
            lda         (op),y
            beq         @done
            jsr         out_byte
            iny
            bne         @byte
@done:
            ply
            rts

; A number (.A, .X: -32768 to 32767) out, in decimal.  Keeps .Y
out_dec:
            phy
            cpx         #$80
            bcc         @plus
            pha
            lda         #'-'
            jsr         out_byte
            pla
            eor         #$FF
            clc
            adc         #1
            pha
            txa
            eor         #$FF
            adc         #0
            tax
            pla
@plus:
            sta         od_n                                ; (The magnitude)
            stx         od_n + 1
            ldy         #0                                  ; (Digits out: none, so 0s before them aren't)
            ldx         #3
@power:
            lda         #'0'
            sta         od_digit
@less:
            lda         od_n                                ; (The power of ten taken while it can be)
            sec
            sbc         od_pow_lo,x
            sta         od_diff
            lda         od_n + 1
            sbc         od_pow_hi,x
            bcc         @digit
            sta         od_n + 1
            lda         od_diff
            sta         od_n
            inc         od_digit
            bra         @less
@digit:
            lda         od_digit
            cpy         #0
            bne         @out
            cmp         #'0'
            beq         @next
@out:
            jsr         out_byte
            iny
@next:
            dex
            bpl         @power
            lda         od_n
            ora         #'0'
            jsr         out_byte
            ply
            rts
od_pow_lo:  .lobytes    10, 100, 1000, 10000
od_pow_hi:  .hibytes    10, 100, 1000, 10000

; A capture begun: the output goes to the capture bank till cap_end.  OUT: .A, .X = where it begins (cap_end's)
cap_begin:
            lda         cap_on
            bne         :+
            stz         cap_len
            stz         cap_len + 1
            stz         cap_over
:
            inc         cap_on
            lda         cap_len
            ldx         cap_len + 1
            rts

; The capture begun at .A, .X (cap_begin's) ended: hv = a string of its bytes (or, if one ran out of room, the
; error "Too long ..."), the output back where it was.  OUT: C = 1 and .A = E_NOMEM if there's no room.  It may
; collect
cap_end:
            sta         ce_start
            stx         ce_start + 1
            lda         cap_over
            beq         @string
            jsr         @drop
            lda         #<s_caplong
            ldx         #>s_caplong
            sta         hq
            stx         hq + 1
            lda         #s_caplong_n
            sta         hn
            stz         hn + 1
            jmp         error_make
@string:
            lda         cap_len                             ; (Its bytes, then the string)
            sec
            sbc         ce_start
            sta         hn
            lda         cap_len + 1
            sbc         ce_start + 1
            sta         hn + 1
            jsr         string_alloc
            bcc         :+
            jsr         @drop
            lda         #E_NOMEM
            sec
            rts
:
            lda         $00
            sta         ce_blob
            lda         ce_start                            ; (From the capture bank, 32 bytes at a time, through
            sta         cp                                  ;   ce_buf)
            lda         ce_start + 1
            ora         #$80
            sta         cp + 1
@chunk:
            lda         hn
            ora         hn + 1
            beq         @copied
            lda         #32
            ldx         hn + 1
            bne         :+
            cmp         hn
            bcc         :+
            lda         hn
:
            sta         ce_n
            lda         cap_bank
            sta         $00
            ldy         #0
@in:
            lda         (cp),y
            sta         ce_buf,y
            iny
            cpy         ce_n
            bne         @in
            lda         ce_blob
            sta         $00
            ldy         #0
@out:
            lda         ce_buf,y
            sta         (hp),y
            iny
            cpy         ce_n
            bne         @out
            lda         cp
            clc
            adc         ce_n
            sta         cp
            bcc         :+
            inc         cp + 1
:
            lda         hp
            clc
            adc         ce_n
            sta         hp
            bcc         :+
            inc         hp + 1
:
            lda         hn
            sec
            sbc         ce_n
            sta         hn
            bcs         @chunk
            dec         hn + 1
            bra         @chunk
@copied:
            jsr         @drop
            clc
            rts
@drop:
            lda         ce_start
            ldx         ce_start + 1
; The capture begun at .A, .X given up (its bytes dropped), the output back where it was
cap_drop:
            sta         cap_len
            stx         cap_len + 1
            dec         cap_on
            rts

; The captures all closed (the machine stopped)
cap_reset:
            stz         cap_on
            stz         cap_len
            stz         cap_len + 1
            stz         cap_over
            rts

; The messages (errors' texts) are in the fourth bank (RODATA4): read with it at $A000 for a while, from here
; The zero-terminated message at .A, .X out.  Keeps .Y
msg_out:
            pha
            lda         $01
            sta         msg_bank
            lda         hyx2_bank4
            sta         $01
            pla
            jsr         out_text
            lda         msg_bank
            sta         $01
            rts

; An error made of the zero-terminated message at .A, .X: hv.  OUT: C = 1 and .A = E_NOMEM if there's no room
msg_err:
            sta         hq
            stx         hq + 1
            lda         $01
            sta         msg_bank
            lda         hyx2_bank4
            sta         $01
            ldy         #0
:
            lda         (hq),y
            beq         :+
            iny
            bne         :-
:
            sty         hn
            stz         hn + 1
            jsr         error_make
            php
            ldx         msg_bank
            stx         $01
            plp
            rts

s_caplong:  .byte       "Too long: a string of more than 8184 bytes"
s_caplong_n = * - s_caplong
