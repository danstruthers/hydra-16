; ****************************************************************************
; ftext.inc - forth's input and output, parsing, numbers and strings (included by forth.s)

; ---- Output: fd 1, through obuf

; .A out (lastc: it, so the shell's prompt knows a line's been ended).  Keeps .X, .Y
emit_a:
            sta         lastc
            phy
            ldy         olen
            sta         obuf,y
            iny
            sty         olen
            cpy         #OBUF_SIZE
            bcc         :+
            jsr         flush
:
            ply
            rts

; The output written (a failed write: no matter); or, while a word's output is taken (out_hook: the shell's
; output-of and piped), given to the hook (.A its length, in obuf).  Keeps .X, .Y
flush:
            lda         olen
            beq         @done
            phy
            stx         xsave
            ldy         out_hook + 1
            bne         @hook
            sta         r1
            stz         r1 + 1
            LDR         r0, obuf
            lda         #1
            jsr         WRITE
@out:
            ldx         xsave
            stz         olen
            ply
@done:
            rts
@hook:
            jsr         @call
            bra         @out
@call:
            jmp         (out_hook)

; The zero-terminated string at w out.  Keeps .X
type_z:
            ldy         #0
:
            lda         (w),y
            beq         :+
            jsr         emit_a
            iny
            bne         :-
:
            rts

; tmp bytes at w out (tmp's low byte, 0-255 ... : a cell).  Keeps .X
type_w:
:
            lda         tmp
            ora         tmp + 1
            beq         @done
            lda         (w)
            jsr         emit_a
            inc         w
            bne         :+
            inc         w + 1
:
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            bra         :---
@done:
            rts

            HEADER      "emit", 0
emit:
            lda         dlo,x
            inx
            jmp         emit_a

            HEADER      "type", 0
type:                                                       ; ( addr u -- )
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            inx
            jmp         type_w

            HEADER      "cr", 0
cr:
            lda         #LF
            jmp         emit_a

            HEADER      "space", 0
space:
            lda         #' '
            jmp         emit_a

; .A out as two hex digits (tools.fl's and disasm.fl's).  Keeps .X, .Y
hex2:
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
            adc         #6                                  ; (C = 1: 'A' on)
:
            adc         #'0'
            jmp         emit_a

; Header w's name out, and a space (tools.fl's and disasm.fl's)
hdr_out:
            ldy         #2
            lda         (w),y
            and         #LEN_MASK
            sta         cnt
            ldy         #3
:
            lda         cnt
            beq         :+
            lda         (w),y
            jsr         emit_a
            iny
            dec         cnt
            bra         :-
:
            jmp         space

            HEADER      "spaces", 0
spaces:                                                     ; ( n -- ): none if n < 1
            lda         dhi,x
            bmi         @done
:
            lda         dlo,x
            ora         dhi,x
            beq         @done
            jsr         space
            jsr         oneminus
            bra         :-
@done:
            inx
            rts

; ---- Input: stdin, a line at a time

; stdin's next byte.  OUT: C = 0, .A = it; or C = 1: its end.  Keeps .X, .Y
getc_in:
            phy
            lda         ipos
            cmp         ilen
            lda         ipos + 1
            sbc         ilen + 1
            bcc         @have
            jsr         flush                               ; (What's out first: a prompt; the console cooked)
            jsr         raw_off
            stx         xsave
            LDR         r0, ibuf
            LDR         r1, IBUF_SIZE
            lda         #0
            jsr         READ
            bcs         @error                              ; (An error: the end too)
            sta         ilen
            stx         ilen + 1
            ldx         xsave
            stz         ipos
            stz         ipos + 1
            ora         ilen + 1
            beq         @end
@have:
            clc
            lda         #<ibuf
            adc         ipos
            sta         w3
            lda         #>ibuf
            adc         ipos + 1
            sta         w3 + 1
            lda         (w3)
            inc         ipos
            bne         :+
            inc         ipos + 1
:
            ply
            clc
            rts

@error:
            ldx         xsave
            cmp         #E_INTR                             ; (Ctrl-C: a new line, and again)
            bne         @end
            stz         intr
            lda         #LF
            jsr         emit_a
            ply
            jmp         getc_in
@end:
            stz         ilen
            stz         ilen + 1
            stz         ipos
            stz         ipos + 1
            ply
            sec
            rts

; A line from stdin into (w2), tmp2 bytes at most (the rest of a longer one dropped).  OUT: C = 0, .A/.Y its
; length (without the LF); or C = 1: stdin's end, nothing read
read_line:
            stz         tmp3
            stz         tmp3 + 1
            jsr         getc_in
            bcs         @eof
@char:
            cmp         #LF
            beq         @done
            cmp         #$0D                                ; (A CR: dropped, a file's CR LF)
            beq         @next
            pha
            lda         tmp3                                ; Room?
            cmp         tmp2
            lda         tmp3 + 1
            sbc         tmp2 + 1
            pla
            bcs         @next
            phy
            ldy         tmp3                                ; (Lines are short: 8 bits of index, w2 moved on a page
            sta         (w2),y                              ;   as tmp3's low byte wraps: TIB and ACCEPT's)
            ply
            inc         tmp3
            bne         @next
            inc         tmp3 + 1
            inc         w2 + 1
@next:
            jsr         getc_in
            bcc         @char
@done:
            lda         tmp3
            ldy         tmp3 + 1
            clc
            rts

@eof:
            sec
            rts

; REFILL for stdin: the next line in tib, SOURCE and >IN for it, and its number.  OUT: C = 1 at stdin's end
refill_in:
            LDR         w2, tib
            LDR         tmp2, TIB_SIZE
            jsr         read_line
            bcs         @done
            sta         src_len
            sty         src_len + 1
            lda         #<tib
            sta         src_addr
            lda         #>tib
            sta         src_addr + 1
            stz         to_in
            stz         to_in + 1
            stz         src_id
            stz         src_id + 1
            inc         src_line
            bne         :+
            inc         src_line + 1
:
            clc
@done:
            rts

; REFILL for stdin or a file (the source's).  OUT: C = 1 at its end
refill_src:
            lda         src_id
            bne         :+
            jmp         refill_in
:
            jmp         refill_file

            HEADER      "accept", 0
accept:                                                     ; ( c-addr +n1 -- +n2 )
            lda         dlo,x
            sta         tmp2
            lda         dhi,x
            sta         tmp2 + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            inx
            jsr         read_line
            bcc         :+
            lda         #0                                  ; (Its end: none)
            tay
:
            sta         dlo,x
            sty         dhi,x
            rts

            HEADER      "key", 0
key:                                                        ; ( -- char ): the console's raw mode, a key (KEY?'s,
            lda         key_pend                            ;   if it has one); not the console, stdin's next byte
            beq         :+                                  ;   (its end: 4)
            stz         key_pend
            lda         key_char
            bra         @push
:
            lda         interactive
            bne         @raw
            jsr         getc_in
            bcc         @push
            lda         #4
            bra         @push

@raw:
            jsr         flush
            jsr         raw_on
            stx         xsave
            LDR         r0, numtmp
            LDR         r1, 1
            lda         #0
            jsr         READ
            ldx         xsave
            bcc         :+
            cmp         #E_INTR                             ; (Ctrl-C: THROW -28; another note: again, the next word
            bne         :+                                  ;   taking it)
            jsr         intr_wait
            bra         @raw
:
            lda         numtmp
@push:
            ldy         #0
            PUSHAY
            rts

; The console in raw mode (KEY's, KEY?'s), till a line's wanted (raw_off).  Keeps .X, .Y
raw_on:
            lda         raw
            bne         @done
            phx
            phy
            lda         ctlfd
            bpl         :+
            LDR         r0, s_consctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         @fail
            sta         ctlfd
:
            LDR         r0, s_rawon
            LDR         r1, 5
            lda         ctlfd
            jsr         WRITE
            inc         raw
@fail:
            ply
            plx
@done:
            rts

; The console cooked again, for a line.  Keeps .X, .Y
raw_off:
            lda         raw
            beq         @done
            stz         raw
            phx
            phy
            LDR         r0, s_rawoff
            LDR         r1, 6
            lda         ctlfd
            jsr         WRITE
            ply
            plx
@done:
            rts

s_consctl:  .byte       "/dev/consctl", 0
s_rawon:    .byte       "rawon"
s_rawoff:   .byte       "rawoff"

; ---- Parsing SOURCE, from >IN

; w = the source's next char's address, tmp = the chars left (0 or more).  Keeps .X
src_rest:
            clc
            lda         src_addr
            adc         to_in
            sta         w
            lda         src_addr + 1
            adc         to_in + 1
            sta         w + 1
            sec
            lda         src_len
            sbc         to_in
            sta         tmp
            lda         src_len + 1
            sbc         to_in + 1
            sta         tmp + 1
            bcs         :+
            stz         tmp
            stz         tmp + 1
:
            rts

; >IN on a char
in_next:
            inc         to_in
            bne         :+
            inc         to_in + 1
:
            rts

; Is .A a delimiter: cnt itself, or (cnt a space) any control char too?  OUT: C = 1 yes.  Keeps .A
delim:
            cmp         cnt
            beq         @yes
            pha
            lda         cnt
            cmp         #' '
            bne         @no
            pla
            cmp         #' ' + 1
            bcc         @yes2
            clc
            rts
@no:
            pla
            clc
            rts
@yes2:
            sec
            rts
@yes:
            sec
            rts

; The source's chars from >IN to the next delimiter cnt (not skipping any first: skip, before, does): pushes
; ( c-addr u ), >IN past the delimiter
parse_to:
            jsr         src_rest
            lda         w                                   ; Its start
            ldy         w + 1
            PUSHAY
            stz         tmp2                                ; (tmp2: its length)
            stz         tmp2 + 1
@char:
            lda         tmp
            ora         tmp + 1
            beq         @end
            lda         (w)
            jsr         in_next
            jsr         delim
            bcs         @end
            inc         tmp2
            bne         :+
            inc         tmp2 + 1
:
            inc         w
            bne         :+
            inc         w + 1
:
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            bra         @char
@end:
            lda         tmp2
            ldy         tmp2 + 1
            PUSHAY
            rts

; >IN past the delimiters cnt at it
skip_delims:
            jsr         src_rest
:
            lda         tmp
            ora         tmp + 1
            beq         @done
            lda         (w)
            jsr         delim
            bcc         @done
            jsr         in_next
            inc         w
            bne         :+
            inc         w + 1
:
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            bra         :---
@done:
            rts

; PARSE-NAME's (the Core Extension library's header), the interpreter's: skip_delims and parse_to's, for a space
; (any control char a delimiter too), in loops of its own: with fewer than 256 characters left (a line's), one over
; them (.Y their index); else 255 characters a part.  Keeps .X
parse_name:                                                 ; ( "<spaces>name<space>" -- c-addr u )
            jsr         src_rest
            lda         tmp + 1
            bne         @skip
            ldy         #0                                  ; The delimiters before it ...
@skip1:
            cpy         tmp
            beq         @at1
            lda         (w),y
            cmp         #' ' + 1
            bcs         @at1
            iny
            bra         @skip1
@at1:
            sty         tmp2                                ;   its start (tmp2) ...
@char1:
            cpy         tmp
            beq         @end1
            lda         (w),y
            cmp         #' ' + 1
            bcc         @delim1
            iny
            bra         @char1
@delim1:
            sty         cnt                                 ;   its end (cnt), and >IN past it, a delimiter too
            iny
            bra         :+
@end1:
            sty         cnt
:
            tya                                             ; (>IN on .Y)
            clc
            adc         to_in
            sta         to_in
            bcc         :+
            inc         to_in + 1
:
            clc                                             ; ( w + start, end - start )
            lda         w
            adc         tmp2
            pha
            lda         w + 1
            adc         #0
            tay
            pla
            PUSHAY
            sec
            lda         cnt
            sbc         tmp2
            ldy         #0
            PUSHAY
            lda         #' '                                ; (cnt the delimiter, as parse_to's callers have it)
            sta         cnt
            rts
@skip:
            jsr         @limit                              ; The delimiters before it
            ldy         #0
:
            cpy         cnt
            beq         :+
            lda         (w),y
            cmp         #' ' + 1
            bcs         :+
            iny
            bra         :-
:
            jsr         @adv
            cpy         cnt                                 ; (A part all delimiters, and more: on)
            bne         @at
            lda         tmp
            ora         tmp + 1
            bne         @skip
@at:
            lda         w                                   ; Its start
            ldy         w + 1
            PUSHAY
            stz         tmp2                                ; Its characters (tmp2: how many)
            stz         tmp2 + 1
@part:
            jsr         @limit
            ldy         #0
@char:
            cpy         cnt
            beq         @past
            lda         (w),y
            cmp         #' ' + 1
            bcc         @delim
            iny
            bra         @char
@delim:
            jsr         @took                               ; (>IN past it too)
            inc         w
            bne         @end
            inc         w + 1
            bra         @end
@past:
            jsr         @took                               ; The part's end: the source's, or on
            lda         tmp
            ora         tmp + 1
            bne         @part
@end:
            sec                                             ; >IN: where it got to
            lda         w
            sbc         src_addr
            sta         to_in
            lda         w + 1
            sbc         src_addr + 1
            sta         to_in + 1
            lda         #' '                                ; (cnt the delimiter, as parse_to's callers have it)
            sta         cnt
            lda         tmp2
            ldy         tmp2 + 1
            PUSHAY
            rts
@limit:                                                     ; (cnt: what a part looks at, 255 at most)
            lda         #255
            ldy         tmp + 1
            bne         :+
            lda         tmp
:
            sta         cnt
            rts
@took:                                                      ; (tmp2 on .Y characters; then as @adv)
            tya
            clc
            adc         tmp2
            sta         tmp2
            bcc         @adv
            inc         tmp2 + 1
@adv:                                                       ; (w on .Y characters, tmp less them.  Keeps .Y)
            tya
            clc
            adc         w
            sta         w
            bcc         :+
            inc         w + 1
:
            tya
            eor         #$FF
            sec
            adc         tmp
            sta         tmp
            bcs         :+
            dec         tmp + 1
:
            rts

            HEADER      "word", 0
word:                                                       ; ( char "<chars>ccc<char>" -- c-addr )
            lda         dlo,x
            inx
            sta         cnt
            jsr         skip_delims
            jsr         parse_to
; ( c-addr u -- c-addr' ): the string, counted, in wbuf (255 chars at most)
to_wbuf:
            lda         dlo,x
            ldy         dhi,x
            beq         :+
            lda         #255
:
            sta         wbuf
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0
:
            cpy         wbuf
            beq         :+
            lda         (w),y
            sta         wbuf + 1,y
            iny
            bra         :-
:
            lda         #' '                                ; (A space after it, as some old code wants)
            sta         wbuf + 1,y
            inx
            lda         #<wbuf
            sta         dlo,x
            lda         #>wbuf
            sta         dhi,x
            rts

            HEADER      "source", 0
source:
            lda         src_addr
            ldy         src_addr + 1
            PUSHAY
            lda         src_len
            ldy         src_len + 1
            PUSHAY
            rts

            HEADER      "(", F_IMMEDIATE
paren:                                                      ; To ")": a comment; a file's (or stdin's but the
            lda         #')'                                ;   console's) on over lines, till there is one
            sta         cnt
            jsr         parse_to
            inx
            inx
            lda         to_in                               ; (Ended by it: the char before >IN)
            ora         to_in + 1
            beq         @more
            clc
            lda         src_addr
            adc         to_in
            sta         w
            lda         src_addr + 1
            adc         to_in + 1
            sta         w + 1
            lda         w
            bne         :+
            dec         w + 1
:
            dec         w
            lda         (w)
            cmp         #')'
            beq         @done
@more:
            lda         src_id + 1                          ; (A string's: no more)
            bmi         @done
            lda         src_id
            bne         :+
            lda         interactive
            bne         @done
:
            jsr         refill_src
            bcc         paren
@done:
            rts

            HEADER      "\", F_IMMEDIATE
backslash:                                                  ; The rest of the line: a block's, its 64-character one
            lda         src_blk
            ora         src_blk + 1
            beq         :+
            clc
            lda         to_in
            adc         #63
            and         #$C0
            sta         to_in
            bcc         @done
            inc         to_in + 1
@done:
            rts
:
            lda         src_len
            sta         to_in
            lda         src_len + 1
            sta         to_in + 1
            rts

            HEADER      "char", 0
char:
            jsr         parse_name
            inx                                             ; (Its first char)
            jmp         cfetch

; ---- Pictured numeric output

            HEADER      "<#", 0
lessnum:
            lda         #<hold_end
            sta         hld
            lda         #>hold_end
            sta         hld + 1
            rts

            HEADER      "hold", 0
hold:
            lda         dlo,x
            inx
hold_a:
            pha
            lda         hld
            bne         :+
            dec         hld + 1
:
            dec         hld
            lda         hld                                 ; (No further than the buffer's start)
            cmp         #<holdbuf
            lda         hld + 1
            sbc         #>holdbuf
            bcs         :+
            pla
            lda         #<-17
            jmp         throw_a
:
            lda         hld
            sta         w3
            lda         hld + 1
            sta         w3 + 1
            pla
            sta         (w3)
            rts

            HEADER      "#", 0
num:                                                        ; ( ud -- ud' )
            jsr         ud_base
            cmp         #10
            bcc         :+
            adc         #'A' - 10 - 1                       ; (C = 1)
            bra         hold_a
:
            ora         #'0'
            bra         hold_a

            HEADER      "#s", 0
nums:
:
            jsr         num
            lda         dlo,x
            ora         dhi,x
            ora         dlo + 1,x
            ora         dhi + 1,x
            bne         :-
            rts

            HEADER      "#>", 0
numgreater:                                                 ; ( ud -- addr u )
            lda         hld
            sta         dlo + 1,x
            lda         hld + 1
            sta         dhi + 1,x
            sec
            lda         #<hold_end
            sbc         hld
            sta         dlo,x
            lda         #>hold_end
            sbc         hld + 1
            sta         dhi,x
            rts

            HEADER      "sign", 0
sign:
            lda         dhi,x
            inx
            asl                                             ; (Its sign: C)
            bcc         :+
            lda         #'-'
            jmp         hold_a
:
            rts

; ( n -- addr u ): n as text, signed (in the numbers library's base, if it isn't a radix)
n_text:
            lda         #2
            jsr         num_textc
            bcs         :+
            rts
:
            lda         dhi,x                               ; (Its sign, kept)
            pha
            jsr         abs
            dex
            jsr         zero_tos
            jsr         lessnum
            jsr         nums
            pla
            bpl         :+
            lda         #'-'
            jsr         hold_a
:
            jmp         numgreater

; ( u -- addr u ): u as text
u_text:
            lda         #3
            jsr         num_textc
            bcs         :+
            rts
:
            dex
            jsr         zero_tos
            jsr         lessnum
            jsr         nums
            jmp         numgreater

; The numbers library's text for the top, num_call .A (2-4), if its base isn't a radix (num_custom).  OUT: C = 0
; done, ( c-addr u ); or C = 1
num_textc:
            ldy         num_custom
            bne         :+
            sec
            rts
:
            jmp         num_call

            HEADER      ".", 0
dot:
            jsr         n_text
            jsr         type
            jmp         space

            HEADER      "u.", 0
udot:
            jsr         u_text
            jsr         type
            jmp         space

; ---- Numbers in: >NUMBER, and the interpreter's

            HEADER      ">number", 0
tonumber:                                                   ; ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
:
            lda         dlo,x
            ora         dhi,x
            beq         @done
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         (w)
            jsr         digit                               ; (C = 1: not one)
            bcs         @done
            phx                                             ; (ud: at .X + 2, its high cell)
            pha
            txa
            clc
            adc         #2
            tay
            pla
            jsr         ud_mul_add
            plx
            inc         dlo + 1,x                           ; On a char
            bne         :+
            inc         dhi + 1,x
:
            jsr         oneminus
            bra         :--
@done:
            rts

; .A as a digit in BASE.  OUT: C = 0, .A = its value; or C = 1
digit:
            cmp         #'a'                                ; (Lower case as upper)
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            sec
            sbc         #'0'
            bcc         @no
            cmp         #10
            bcc         @base
            sbc         #'A' - '0'                          ; (C = 1)
            bcc         @no
            adc         #10 - 1                             ; (C = 1: + 10)
            cmp         #10
            bcc         @no
@base:
            cmp         base                                ; (BASE: 36 at most)
            bcs         @no
            clc
            rts
@no:
            sec
            rts

; The interpreter's number: ( c-addr u -- n 1 | d 2 | 0 ): a sign, a prefix (# decimal, $ hex, % binary), digits in
; BASE, a . at its end for a double; or 'c', a char.  With the numbers library, one past a cell (-32768 to 65535) or
; a double isn't (the library's, on its number stack: the interpreter's num_call), nor any in a base of its that
; isn't a radix (num_custom: the library reads them)
number:
            lda         dlo,x                               ; 'c'
            cmp         #3
            bne         @digits
            lda         dhi,x
            bne         @digits
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         (w)
            cmp         #$27
            bne         @digits
            ldy         #2
            lda         (w),y
            cmp         #$27
            bne         @digits
            dey
            lda         (w),y
            inx
            sta         dlo,x
            stz         dhi,x
            jmp         one

@digits:
            lda         num_custom                          ; (A base that isn't a radix: the numbers library's)
            beq         :+
            lda         #1
            jsr         num_call
            bcs         :+
            rts
:
            lda         base                                ; (BASE, kept: a prefix changes it a moment)
            pha
            lda         base + 1
            pha
            stz         cnt                                 ; (cnt bit 7: negative)
            jsr         num_prefix
            jsr         num_minus
            jsr         num_prefix                          ; (-$10 and $-10 alike)
            lda         dlo,x                               ; Nothing left: not a number
            ora         dhi,x
            bne         @lb200
            jmp         @not
@lb200:
            lda         cnt
            pha
            jsr         twotor_call                         ; ud 0 0, then the text
            stz         num_ovf
            jsr         tonumber
            pla
            sta         cnt
            lda         dlo,x                               ; All of it: a single
            ora         dhi,x
            beq         @single
            lda         dhi,x                               ; One char left, a '.': a double
            bne         @not2
            lda         dlo,x
            cmp         #1
            bne         @not2
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         (w)
            cmp         #'.'
            bne         @not2
            inx
            inx
            lda         num_vec + 1                         ; (Past 32 bits, with the numbers library: its;
            beq         @dok                                ;   below 0, -2147483648 at most: num_ovf 1 too)
            lda         num_ovf
            bne         @not
            bit         cnt
            bpl         @dok
            lda         dhi,x
            cmp         #$80
            bcc         @dok
            bne         @dbig
            lda         dlo,x
            ora         dlo + 1,x
            ora         dhi + 1,x
            beq         @dok
@dbig:
            inc         num_ovf
            bra         @not
@dok:
            bit         cnt
            bpl         :+
            jsr         dnegate
:
            lda         #2
            bra         @done

@single:
            inx
            inx
            lda         num_vec + 1                         ; (Past a cell, with the numbers library: its; below
            beq         @cell                               ;   0, -32768 at most)
            lda         num_ovf
            ora         dlo,x
            ora         dhi,x
            bne         @not
            bit         cnt
            bpl         @cell
            lda         dhi + 1,x
            cmp         #$80
            bcc         @cell
            bne         @not
            lda         dlo + 1,x
            bne         @not
@cell:
            inx                                             ; (ud's high cell: dropped)
            bit         cnt
            bpl         :+
            jsr         negate
:
            lda         #1
            bra         @done

@not2:
            inx                                             ; (The rest: dropped; ud's low cell, 0 below)
            inx
@not:
            inx
            lda         #0
            stz         dlo,x
            stz         dhi,x
            pla
            sta         base + 1
            pla
            sta         base
            rts

@done:
            dex
            sta         dlo,x
            stz         dhi,x
            pla
            sta         base + 1
            pla
            sta         base
            rts

one:
            dex
            lda         #1
            sta         dlo,x
            stz         dhi,x
            rts

; ( c-addr u -- 0 0 c-addr u ): two zeros under the text (an ud for >NUMBER)
twotor_call:
            dex
            dex
            lda         dlo + 2,x
            sta         dlo,x
            lda         dhi + 2,x
            sta         dhi,x
            lda         dlo + 3,x
            sta         dlo + 1,x
            lda         dhi + 3,x
            sta         dhi + 1,x
            stz         dlo + 2,x
            stz         dhi + 2,x
            stz         dlo + 3,x
            stz         dhi + 3,x
            rts

; The text's prefix (# $ %), if it has one: BASE for it, the text past it
num_prefix:
            lda         dlo,x
            ora         dhi,x
            beq         @done
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         (w)
            ldy         #10
            cmp         #'#'
            beq         @set
            ldy         #16
            cmp         #'$'
            beq         @set
            ldy         #2
            cmp         #'%'
            bne         @done
@set:
            sty         base
            stz         base + 1
            bra         text_next
@done:
            rts

; The text's -, if it has one: cnt negative, the text past it
num_minus:
            lda         dlo,x
            ora         dhi,x
            beq         @done
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         (w)
            cmp         #'-'
            bne         @done
            lda         #$80
            sta         cnt
            bra         text_next
@done:
            rts

; ( c-addr u -- c-addr+1 u-1 )
text_next:
            inc         dlo + 1,x
            bne         :+
            inc         dhi + 1,x
:
            jmp         oneminus

; ---- Strings in definitions: a jsr to the routine, then a counted string

; The counted string after the caller's jsr (its return address + 1): w its address (the count's), the return
; address moved past it.  Called from the routines below (their return address is two up)
inline_str:
            stx         xsave
            tsx
            lda         $0103,x                             ; (The definition's return address: the jsr's last
            sta         w                                   ;   byte; + 1 the count)
            lda         $0104,x
            sta         w + 1
            inc         w
            bne         :+
            inc         w + 1
:
            clc                                             ; Past it: + the count
            lda         (w)
            adc         w
            sta         $0103,x
            lda         w + 1
            adc         #0
            sta         $0104,x
            ldx         xsave
            rts

; The counted string whose address is after the caller's jsr (a definition's in a code bank: comp_str's, in the
; dictionary): w it, the return address moved past the address.  Called from the routines below
inline_ptr:
            stx         xsave
            tsx
            clc
            lda         $0103,x
            sta         w
            adc         #2
            sta         $0103,x
            lda         $0104,x
            sta         w + 1
            adc         #0
            sta         $0104,x
            ldy         #1
            lda         (w),y
            pha
            iny
            lda         (w),y
            sta         w + 1
            pla
            sta         w
            ldx         xsave
            rts

; S": ( -- c-addr u ).  Its string's address after it (code banks): xsquote_p, 5 bytes before (comp_str)
xsquote_p:
            jsr         inline_ptr
            bra         :+
xsquote:
            jsr         inline_str
:
            jmp         count_w

; C": ( -- c-addr ) (xcquote_p: as xsquote_p)
xcquote_p:
            jsr         inline_ptr
            bra         :+
xcquote:
            jsr         inline_str
:
            lda         w
            ldy         w + 1
            PUSHAY
            rts

; .": the text out
xdotq:
            jsr         inline_str
            jsr         count_w
            jmp         type

; ( -- c-addr u ) for the counted string at w
count_w:
            lda         w
            ldy         w + 1
            PUSHAY
            jmp         count

; The string (c-addr u on the stack) compiled after a jsr to routine .A/.Y: the routine, the count, the chars.  In a
; code bank (but ."'s, said as it's called) the string is in the dictionary, and its address after the jsr to the
; routine's _p, 5 bytes before it
comp_str:
            bit         cmode
            bpl         @inline
            cmp         #<xdotq
            bne         :+
            cpy         #>xdotq
            beq         @inline
:
            sec
            sbc         #5
            bcs         :+
            dey
:
            jsr         comp_jsr
            lda         dhere
            ldy         dhere + 1
            jsr         comma_ay
            jsr         here_swap
            jsr         @str
            jmp         here_swap
@inline:
            pha
            lda         #JSR_OP
            jsr         ccomma_a
            pla
            jsr         comma_ay
@str:
            lda         dlo,x                               ; (255 at most)
            ldy         dhi,x
            beq         :+
            lda         #255
:
            sta         dlo,x
            sta         tmp3
            jsr         ccomma_a
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0
:
            cpy         tmp3
            beq         :+
            lda         (w),y
            jsr         ccomma_a
            iny
            bra         :-
:
            inx
            inx
            rts
.assert xsquote - xsquote_p = 5 && xcquote - xcquote_p = 5 && xabortq - xabortq_p = 5, error, "comp_str's _p"

; ( c-addr u -- c-addr' u ): the string copied into the next of S"'s two buffers (interpreting)
to_sbuf:
            lda         sbuf_n
            eor         #1
            sta         sbuf_n
            beq         :+
            lda         #<(sbuf + SBUF_SIZE)
            ldy         #>(sbuf + SBUF_SIZE)
            bra         :++
:
            lda         #<sbuf
            ldy         #>sbuf
:
            sta         w2
            sty         w2 + 1
            lda         dlo,x                               ; (SBUF_SIZE at most)
            ldy         dhi,x
            bne         :+
            cmp         #SBUF_SIZE
            bcc         :++
:
            lda         #SBUF_SIZE
            sta         dlo,x
            stz         dhi,x
:
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp3
            ldy         #0
:
            cpy         tmp3
            beq         :+
            lda         (w),y
            sta         (w2),y
            iny
            bra         :-
:
            lda         w2
            sta         dlo + 1,x
            lda         w2 + 1
            sta         dhi + 1,x
            rts

            HEADERQ     "s", F_IMMEDIATE
squote:
            lda         #'"'
            sta         cnt
            jsr         parse_to
str_comp:                                                   ; Compiling: S" in the definition; else a buffer's
            lda         state
            beq         to_sbuf
            lda         #<xsquote
            ldy         #>xsquote
            jmp         comp_str

            HEADERQC    ".", F_IMMEDIATE
dotquote:
            lda         #'"'
            sta         cnt
            jsr         parse_to
            lda         #<xdotq
            ldy         #>xdotq
            jmp         comp_str
