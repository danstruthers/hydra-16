; ****************************************************************************
; io.inc - hylang's input and output: what it writes goes to fd 1 through a buffer (out_flush sends it: at a line's
; end, and before a read), and the REPL's lines come from fd 0 (the console's cooked lines, or a file through rc's <).
; What's written may be caught instead: as a string being made (cap_start, cap_string: repr, format ...), or as
; output-of's (oc_buf), which goes on while expressions are evaluated: each a buffer of STR_LONG bytes, so nothing's
; made (nothing's collected) while values are written.  A string of more than STR_MAX bytes is OT_LSTR, a chain of
; cells of STR_MAX (buf_string makes one); str_flat copies any string's bytes to a buffer.  Its code is in the task's
; RAM (the module's DATA), where every bank sees it.

OUT_SIZE        = 128
STR_MAX         = 126           ; A string cell's bytes, at most (a longer string: OT_LSTR, its cells chained)
STR_LONG        = 4096          ; A string's bytes, at most

.zeropage
io_n:       .res        2                                   ; A count
io_p:       .res        2                                   ; A buffer's next byte (what's caught; str_flat's)

.bss
out_buf:    .res        OUT_SIZE                            ; What's to be written ...
out_n:      .res        1                                   ;   and how much
num_buf:    .res        6                                   ; A number's digits, made backwards
fl_n1:      .res        2                                   ; A string flat in text: its length (str_cmp2's
fl_n2:      .res        2                                   ; first, str_arg's ...); one in cap_buf (str_cmp2's
fl_b:       .res        2                                   ; second, fl_b)
cap_on:     .res        1                                   ; $80: what's written is a string being made ...
cap_n:      .res        2                                   ;   its length (past STR_LONG: too long) ...
cap_buf:    .res        STR_LONG                            ;   and its bytes
oc_on:      .res        1                                   ; $80: what's written is output-of's (else, if not
oc_n:       .res        2                                   ; cap_on, fd 1's)
oc_buf:     .res        STR_LONG
io_len:     .res        2                                   ; buf_string's length ...
io_k:       .res        1                                   ;   its cells, one by one
io_out:     .res        1                                   ; $80: str_walk writes the bytes (else, copies them)
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
            pha                                             ; (A string being made: past STR_LONG, too long)
            lda         cap_n + 1
            cmp         #>STR_LONG
            bcs         @past
            clc
            lda         #<cap_buf
            adc         cap_n
            sta         io_p
            lda         #>cap_buf
            adc         cap_n + 1
            sta         io_p + 1
            pla
            sta         (io_p)
            inc         cap_n
            bne         :+
            inc         cap_n + 1
:
            rts
@past:
            lda         #>STR_LONG                          ; (STR_LONG + 1: too long)
            sta         cap_n + 1
            lda         #1
            sta         cap_n
            pla
            rts
@oc:
            pha                                             ; (output-of's)
            lda         oc_n + 1
            cmp         #>STR_LONG
            bcs         @oc_past
            clc
            lda         #<oc_buf
            adc         oc_n
            sta         io_p
            lda         #>oc_buf
            adc         oc_n + 1
            sta         io_p + 1
            pla
            sta         (io_p)
            inc         oc_n
            bne         :+
            inc         oc_n + 1
:
            rts
@oc_past:
            lda         #>STR_LONG
            sta         oc_n + 1
            lda         #1
            sta         oc_n
            pla
            rts

; What's written caught, a string being made (cap_string ends it)
cap_start:
            lda         #$80
            sta         cap_on
            stz         cap_n
            stz         cap_n + 1
            rts

; The string caught: h_v, and what's written goes where it did.  OUT: C = 0; C = 1, h_v an error (too long, or no
; room)
cap_string:
            stz         cap_on
            LDR         r0, cap_buf
            lda         cap_n
            ldx         cap_n + 1
            ; (falls into buf_string)

; A string of the .A/.X bytes at r0 (in the task's RAM): h_v (more than STR_MAX of them: OT_LSTR).  OUT: C = 0;
; C = 1, h_v an error (too long: more than STR_LONG; or no room)
buf_string:
            cpx         #0
            bne         @long
            cmp         #STR_MAX + 1
            bcs         @long
            jsr         make_string
            bcc         @done
@room:
            MOVR        h_v, err_room
            sec
@done:
            rts
@long:
            sta         io_len
            stx         io_len + 1
            cpx         #>STR_LONG                          ; (STR_LONG, at most)
            bcc         :+
            jne         @too_long
            ora         #0
            jne         @too_long
:
            stz         io_k                                ; ---- Its cells: how many, the last's length
            lda         io_len
            sta         io_n
            lda         io_len + 1
            sta         io_n + 1
:
            inc         io_k
            sec
            lda         io_n
            sbc         #STR_MAX
            tax
            lda         io_n + 1
            sbc         #0
            bcc         :+
            bne         @more
            cpx         #0
            beq         :+
@more:
            sta         io_n + 1
            stx         io_n
            bra         :-
:
            LDR         h_v, NIL                            ; (The chain, made from the last cell back)
            jsr         root_push
            lda         io_k                                ; (r0 at the last cell: + STR_MAX for each before it)
            dec         a
            tax
:
            beq         @cells
            clc
            lda         r0
            adc         #STR_MAX
            sta         r0
            bcc         :+
            inc         r0 + 1
:
            dex
            bra         :--
@cells:
            lda         io_n
@cell:
            jsr         make_string
            bcs         @fail
            MOVR        h_a, h_v
            jsr         root_get
            MOVR        h_d, h_v
            jsr         cons
            bcs         @fail
            jsr         root_set
            dec         io_k
            beq         @made
            sec
            lda         r0
            sbc         #STR_MAX
            sta         r0
            bcs         :+
            dec         r0 + 1
:
            lda         #STR_MAX
            bra         @cell
@made:
            jsr         root_get                            ; (OT_LSTR: the chain, and its length, a fixnum)
            MOVR        h_a, h_v
            lda         io_len
            asl
            ora         #1
            sta         h_d
            lda         io_len + 1
            rol
            sta         h_d + 1
            jsr         root_drop
            lda         #OT_LSTR
            ldx         #0
            jsr         make_cell2
            jcs         @room
            rts
@fail:
            jsr         root_drop
            jmp         @room
@too_long:
            LDR         r0, io_too_long
            jsr         error_text
            jcs         @room
            sec
            rts

; Whether h_v is a string (or a character).  OUT: C = 1, it isn't; C = 0, .A = 0, a character (.X its byte), or
; OT_STRING or OT_LSTR (h_p its cell, its bank selected)
str_is:
            lda         h_v
            lsr
            bcs         @no
            lda         h_v + 1
            cmp         #>CHARS + 2
            bcs         @object
            cmp         #>CHARS
            bcc         @no
            lsr                                             ; (A character: its code, (the value - $0100) / 2)
            lsr
            lda         h_v
            ror
            tax
            lda         #0
            clc
            rts
@object:
            tax
            lda         page_kind,x
            cmp         #PK_C8
            bcc         @no
            jsr         deref
            lda         (h_p)
            cmp         #OT_STRING
            beq         @yes
            cmp         #OT_LSTR
            beq         @yes
@no:
            sec
            rts
@yes:
            clc
            rts

; The string (or character) h_v's length: .A/.X.  OUT: C = 1, it isn't a string
str_len:
            jsr         str_is
            bcs         @done
            ldx         #0
            cmp         #OT_STRING
            beq         @cell
            bcs         @long
            lda         #1                                  ; (A character)
            rts
@cell:
            ldy         #1
            lda         (h_p),y
            clc
            rts
@long:
            ldy         #5                                  ; (Long: its length, a fixnum)
            lda         (h_p),y
            lsr
            tax
            dey
            lda         (h_p),y
            ror
            clc
@done:
            rts

; The string (or character) h_v's bytes copied to io_p's buffer (str_flat), or written (str_out: as print shows
; it): .A/.X = how many (h_v changed).  OUT: C = 1, it isn't a string
str_flat:
            stz         io_out
            bra         str_walk
str_out:
            lda         #$80
            sta         io_out
str_walk:
            stz         io_len
            stz         io_len + 1
            jsr         str_is
            bcs         @no
            cmp         #OT_STRING
            beq         @cell
            bcs         @long
            txa                                             ; (A character: its byte)
            jsr         @byte
            bra         @done
@long:
            ldy         #3                                  ; ---- Long: each cell of its chain
            lda         (h_p),y
            tax
            dey
            lda         (h_p),y
@next:
            sta         h_v
            stx         h_v + 1
            ora         h_v + 1
            beq         @done
            lda         h_v + 1                             ; (The chain kept, as its cell's bytes go)
            pha
            lda         h_v
            pha
            jsr         car
            jsr         deref
            jsr         @bytes
            pla
            sta         h_v
            pla
            sta         h_v + 1
            jsr         cdr
            lda         h_v
            ldx         h_v + 1
            bra         @next
@cell:
            jsr         @bytes
@done:
            lda         io_len
            ldx         io_len + 1
            clc
@no:
            rts
@bytes:
            ldy         #1                                  ; (A cell's bytes)
            lda         (h_p),y
            beq         @end
            sta         io_k
            ldy         #2
:
            lda         (h_p),y
            jsr         @byte
            iny
            dec         io_k
            bne         :-
@end:
            rts
@byte:
            inc         io_len                              ; (One: to io_p, on; or written)
            bne         :+
            inc         io_len + 1
:
            bit         io_out
            bmi         @out
            sta         (io_p)
            inc         io_p
            bne         :+
            inc         io_p + 1
:
            rts
@out:
            jmp         out_char

; The flat strings in text (fl_n1 bytes) and cap_buf (fl_n2) compared, a byte at a time (the shorter first, if one
; begins the other): .A = $FF, 0 or 1
flat_cmp:
            LDR         io_p, text
            LDR         io_n, cap_buf
            ldy         #0
@byte:
            lda         fl_n1                               ; (One at its end?)
            ora         fl_n1 + 1
            beq         @a_end
            lda         fl_n2
            ora         fl_n2 + 1
            beq         @more
            lda         (io_p),y
            cmp         (io_n),y
            bne         @differ
            iny
            bne         :+
            inc         io_p + 1
            inc         io_n + 1
:
            lda         fl_n1
            bne         :+
            dec         fl_n1 + 1
:
            dec         fl_n1
            lda         fl_n2
            bne         :+
            dec         fl_n2 + 1
:
            dec         fl_n2
            bra         @byte
@a_end:
            lda         fl_n2
            ora         fl_n2 + 1
            beq         @same
@less:
            lda         #$FF
            rts
@differ:
            bcc         @less
@more:
            lda         #1
            rts
@same:
            lda         #0
            rts

; What's caught ended, no string made: its bytes in cap_buf, fl_n2 of them (STR_LONG at most)
cap_end:
            stz         cap_on
            lda         cap_n
            ldx         cap_n + 1
            cpx         #>STR_LONG
            bcc         :+
            lda         #<STR_LONG
            ldx         #>STR_LONG
:
            sta         fl_n2
            stx         fl_n2 + 1
            rts

; What's caught ended, and copied to text: fl_n1 bytes
cap_text:
            jsr         cap_end
            sta         fl_n1
            stx         fl_n1 + 1
            LDR         io_p, cap_buf
            LDR         io_n, text
            ldy         #0
            cpx         #0
            beq         @part
@page:
            lda         (io_p),y                            ; (Its whole pages ...
            sta         (io_n),y
            iny
            bne         @page
            inc         io_p + 1
            inc         io_n + 1
            dex
            bne         @page
@part:
            cpy         fl_n1                               ;   and the rest)
            beq         @done
            lda         (io_p),y
            sta         (io_n),y
            iny
            bra         @part
@done:
            rts

; Whether str_b (str_bn bytes) is in text (fl_n1 bytes) at io_n.  OUT: C = 1, it is
text_at:
            clc                                             ; (Room for it there?)
            lda         io_n
            adc         str_bn
            tax
            lda         io_n + 1
            adc         #0
            cmp         fl_n1 + 1
            bcc         @room
            bne         @no
            cpx         fl_n1
            beq         @room
            bcs         @no
@room:
            clc
            lda         #<text
            adc         io_n
            sta         io_p
            lda         #>text
            adc         io_n + 1
            sta         io_p + 1
            ldy         #0
@byte:
            cpy         str_bn
            beq         @yes
            lda         (io_p),y
            cmp         str_b,y
            bne         @no
            iny
            bra         @byte
@yes:
            sec
            rts
@no:
            clc
            rts

; Where str_b (str_bn bytes) is in text (fl_n1 bytes), from io_n on.  OUT: C = 1, found, io_n its place
text_find:
            jsr         text_at
            bcs         @done
            lda         io_n                                ; (At its end: not found)
            cmp         fl_n1
            lda         io_n + 1
            sbc         fl_n1 + 1
            bcs         @none
            inc         io_n
            bne         text_find
            inc         io_n + 1
            bra         text_find
@none:
            clc
@done:
            rts

; The strings h_v and fl_b compared (flat: text and cap_buf), as flat_cmp.  OUT: C = 1, one isn't a string
str_cmp2:
            LDR         io_p, text
            jsr         str_flat
            bcs         @no
            sta         fl_n1
            stx         fl_n1 + 1
            MOVR        h_v, fl_b
            LDR         io_p, cap_buf
            jsr         str_flat
            bcs         @no
            sta         fl_n2
            stx         fl_n2 + 1
            jsr         flat_cmp
            clc
@no:
            rts

io_too_long: .byte      "A string too long (4096 bytes at most)", 0

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
