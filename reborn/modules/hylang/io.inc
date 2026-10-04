; ****************************************************************************
; io.inc - hylang's input and output: what it writes goes to fd 1 through a buffer (out_flush sends it: at a line's
; end, and before a read), and the REPL's lines come from fd 0 (the console's cooked lines, or a file through rc's <).
; What's written may be caught instead: as a string being made (cap_start, cap_string: repr, format ...), or as
; output-of's (oc_buf), which goes on while expressions are evaluated.  Its code is in the task's RAM (the module's
; DATA), where both of hylang's banks see it.

OUT_SIZE        = 128
STR_MAX         = 126           ; A string's length, at most (as yet: a cell's)

.zeropage
io_n:       .res        2                                   ; A count

.bss
out_buf:    .res        OUT_SIZE                            ; What's to be written ...
out_n:      .res        1                                   ;   and how much
num_buf:    .res        6                                   ; A number's digits, made backwards
cap_on:     .res        1                                   ; $80: what's written is a string being made ...
cap_n:      .res        1                                   ;   its length (past STR_MAX: too long) ...
cap_buf:    .res        STR_MAX + 1                         ;   and its bytes
oc_on:      .res        1                                   ; $80: what's written is output-of's (else, if not
oc_n:       .res        1                                   ; cap_on, fd 1's)
oc_buf:     .res        STR_MAX + 1
.segment "DATA"

; .A written (to the buffer: sent when it's full, or at a line's end; or caught).  Keeps .X and .Y
out_char:
            bit         cap_on
            bmi         @cap
            bit         oc_on
            bmi         @oc
            phx
            ldx         out_n
            sta         out_buf,x
            inc         out_n
            cmp         #LF
            beq         @send
            cpx         #OUT_SIZE - 1
            bne         @done
@send:
            jsr         out_flush
@done:
            plx
            rts
@cap:
            phx                                             ; (A string being made: past STR_MAX, too long)
            ldx         cap_n
            cpx         #STR_MAX + 1
            bcs         :+
            sta         cap_buf,x
            inc         cap_n
:
            plx
            rts
@oc:
            phx                                             ; (output-of's)
            ldx         oc_n
            cpx         #STR_MAX + 1
            bcs         :+
            sta         oc_buf,x
            inc         oc_n
:
            plx
            rts

; What's written caught, a string being made (cap_string ends it)
cap_start:
            lda         #$80
            sta         cap_on
            stz         cap_n
            rts

; The string caught: h_v, and what's written goes where it did.  OUT: C = 0; C = 1, h_v an error (too long, or no
; room)
cap_string:
            stz         cap_on
            LDR         r0, cap_buf
            lda         cap_n
            ; (falls into buf_string)

; A string of the .A bytes at r0 (in the task's RAM): h_v.  OUT: C = 0; C = 1, h_v an error (too long: more than
; STR_MAX; or no room)
buf_string:
            cmp         #STR_MAX + 1
            bcs         @long
            jsr         make_string
            bcc         @done
            MOVR        h_v, err_room
            sec
@done:
            rts
@long:
            LDR         r0, io_too_long
            jsr         error_text
            bcc         :+
            MOVR        h_v, err_room
:
            sec
            rts

io_too_long: .byte      "A string too long (as yet)", 0

; The buffer sent to fd 1.  Keeps .X and .Y
out_flush:
            lda         out_n
            beq         @done
            phx
            phy
            lda         r0                                  ; (r0 and r1 kept: a caller's pointer)
            pha
            lda         r0 + 1
            pha
            lda         r1
            pha
            lda         r1 + 1
            pha
            LDR         r0, out_buf
            lda         out_n
            sta         r1
            stz         r1 + 1
            lda         #1
            jsr         WRITE
            stz         out_n
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
@done:
            rts

; The zero-terminated text at r0 written.  Keeps .X
out_text:
            ldy         #0
:
            lda         (r0),y
            beq         :+
            jsr         out_char
            iny
            bne         :-
:
            rts

; The zero-terminated text at ob_p written.  Keeps .X
out_text_p:
            ldy         #0
:
            lda         (ob_p),y
            beq         :+
            jsr         out_char
            iny
            bne         :-
:
            rts

; .Y bytes at ob_p written.  Keeps .X
out_bytes:
            sty         ob_n
            ldy         #0
:
            cpy         ob_n
            beq         :+
            lda         (ob_p),y
            jsr         out_char
            iny
            bra         :-
:
            rts

; A new line
out_newline:
            lda         #LF
            jmp         out_char

; The signed number .A/.X written in decimal
out_dec:
            sta         io_n
            stx         io_n + 1
            txa
            bpl         @digits
            lda         #'-'
            jsr         out_char
            sec                                             ; (Its size)
            lda         #0
            sbc         io_n
            sta         io_n
            lda         #0
            sbc         io_n + 1
            sta         io_n + 1
@digits:
            ldx         #0
@digit:
            lda         #0                                  ; (io_n / 10: the remainder in .A)
            ldy         #16
@bit:
            asl         io_n
            rol         io_n + 1
            rol
            cmp         #10
            bcc         :+
            sbc         #10
            inc         io_n
:
            dey
            bne         @bit
            ora         #'0'
            sta         num_buf,x
            inx
            lda         io_n
            ora         io_n + 1
            bne         @digit
@out:
            dex
            lda         num_buf,x
            jsr         out_char
            txa
            bne         @out
            rts

; A line from fd 0 (a console's cooked line, or a file's), into r0's buffer: .Y bytes at most.  OUT: .A = its
; length (0: the end), the line's end (LF) kept; C = 1 and .A an error, if the read failed
in_line:
            phy
            jsr         out_flush
            ply
            sty         r1
            stz         r1 + 1
            lda         #0
            jsr         READ
            rts

.code
