; ****************************************************************************
; ftext.inc - forth's input and output, parsing, numbers and strings (included by forth.s)

; ---- Output: fd 1, through obuf

; .A out.  Keeps .X, .Y
emit_a:
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

; The output written (a failed write: no matter).  Keeps .X, .Y
flush:
            lda         olen
            beq         @done
            phy
            stx         xsave
            sta         r1
            stz         r1 + 1
            LDR         r0, obuf
            lda         #1
            jsr         WRITE
            ldx         xsave
            stz         olen
            ply
@done:
            rts

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

            HEADER      "EMIT", 0
emit:
            lda         dlo,x
            inx
            jmp         emit_a

            HEADER      "TYPE", 0
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

            HEADER      "CR", 0
cr:
            lda         #LF
            jmp         emit_a

            HEADER      "SPACE", 0
space:
            lda         #' '
            jmp         emit_a

            HEADER      "SPACES", 0
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
            jsr         flush                               ; (What's out first: a prompt)
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

; REFILL for stdin: the next line in tib, SOURCE and >IN for it.  OUT: C = 1 at stdin's end
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
            clc
@done:
            rts

            HEADER      "REFILL", 0
refill:                                                     ; ( -- flag ): a string's (EVALUATE) can't
            lda         src_id
            ora         src_id + 1
            bne         @false
            jsr         refill_in
            bcs         @false
            dex
            jmp         true_tos
@false:
            dex
            jmp         zero_tos

            HEADER      "ACCEPT", 0
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

            HEADER      "KEY", 0
key:                                                        ; ( -- char ): the console's raw mode, a key
            lda         interactive
            bne         @raw
            jsr         getc_in                             ; (Not the console: stdin's next byte; its end, 4)
            bcc         :+
            lda         #4
:
            ldy         #0
            PUSHAY
            rts

@raw:
            jsr         flush
            stx         xsave
            lda         ctlfd
            bpl         :+
            LDR         r0, s_consctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         @done
            sta         ctlfd
:
            LDR         r0, s_rawon
            LDR         r1, 5
            lda         ctlfd
            jsr         WRITE
            LDR         r0, numtmp
            LDR         r1, 1
            lda         #0
            jsr         READ
            LDR         r0, s_rawoff
            LDR         r1, 6
            lda         ctlfd
            jsr         WRITE
@done:
            ldx         xsave
            lda         numtmp
            ldy         #0
            PUSHAY
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

            HEADER      "PARSE", 0
parse:                                                      ; ( char "ccc<char>" -- c-addr u )
            lda         dlo,x
            inx
            sta         cnt
            jmp         parse_to

            HEADER      "PARSE-NAME", 0
parse_name:                                                 ; ( "<spaces>name<space>" -- c-addr u )
            lda         #' '
            sta         cnt
            jsr         skip_delims
            jmp         parse_to

            HEADER      "WORD", 0
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

            HEADER      "SOURCE", 0
source:
            lda         src_addr
            ldy         src_addr + 1
            PUSHAY
            lda         src_len
            ldy         src_len + 1
            PUSHAY
            rts

            HEADER      "SOURCE-ID", 0
sourceid:
            lda         src_id
            ldy         src_id + 1
            PUSHAY
            rts

            HEADER      "SAVE-INPUT", 0
saveinput:                                                  ; ( -- >in 1 )
            lda         to_in
            ldy         to_in + 1
            PUSHAY
            lda         #1
            ldy         #0
            PUSHAY
            rts

            HEADER      "RESTORE-INPUT", 0
restoreinput:                                               ; ( >in 1 -- false )
            lda         dlo + 1,x
            sta         to_in
            lda         dhi + 1,x
            sta         to_in + 1
            inx
            jmp         zero_tos

            HEADER      "(", F_IMMEDIATE
paren:
            lda         #')'
            sta         cnt
            jsr         parse_to
            inx
            inx
            rts

            HEADER      "\", F_IMMEDIATE
backslash:
            lda         src_len
            sta         to_in
            lda         src_len + 1
            sta         to_in + 1
            rts

            HEADER      ".(", F_IMMEDIATE
dotparen:
            lda         #')'
            sta         cnt
            jsr         parse_to
            jmp         type

            HEADER      "CHAR", 0
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

            HEADER      "HOLD", 0
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

            HEADER      "HOLDS", 0
holds:                                                      ; ( addr u -- )
:
            lda         dlo,x
            ora         dhi,x
            beq         @done
            jsr         oneminus
            clc
            lda         dlo + 1,x
            adc         dlo,x
            sta         w
            lda         dhi + 1,x
            adc         dhi,x
            sta         w + 1
            lda         (w)
            jsr         hold_a
            bra         :-
@done:
            inx
            inx
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

            HEADER      "#S", 0
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

            HEADER      "SIGN", 0
sign:
            lda         dhi,x
            inx
            asl                                             ; (Its sign: C)
            bcc         :+
            lda         #'-'
            jmp         hold_a
:
            rts

; ( n -- addr u ): n as text, signed
n_text:
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
            dex
            jsr         zero_tos
            jsr         lessnum
            jsr         nums
            jmp         numgreater

            HEADER      ".", 0
dot:
            jsr         n_text
            jsr         type
            jmp         space

            HEADER      "U.", 0
udot:
            jsr         u_text
            jsr         type
            jmp         space

; ( addr u w -- ): spaces to w, then the text
right:
            sec
            lda         dlo,x
            sbc         dlo + 1,x
            sta         dlo,x
            lda         dhi,x
            sbc         dhi + 1,x
            sta         dhi,x
            jsr         spaces
            jmp         type

            HEADER      ".R", 0
dotr:                                                       ; ( n w -- )
            jsr         save_top
            jsr         n_text
            jsr         push_tmp3
            bra         right

            HEADER      "U.R", 0
udotr:
            jsr         save_top
            jsr         u_text
            jsr         push_tmp3
            bra         right

            HEADER      "?", 0
question:
            jsr         fetch
            jmp         dot

            HEADER      ".S", 0
dots:                                                       ; ( -- ): "<depth> items", the top last
            lda         #'<'
            jsr         emit_a
            jsr         depth
            jsr         u_text
            jsr         type
            lda         #'>'
            jsr         emit_a
            jsr         space
            stx         tmp3                                ; (The top's index; tmp3 + 1: the next item's, + 1)
            lda         #DS_N
            sta         tmp3 + 1
@item:
            dec         tmp3 + 1                            ; The deepest first, to the top
            lda         tmp3 + 1
            cmp         tmp3
            bcc         @done
            tay
            dex
            lda         dlo,y
            sta         dlo,x
            lda         dhi,y
            sta         dhi,x
            jsr         dot
            bra         @item
@done:
            rts

; ---- Numbers in: >NUMBER, and the interpreter's

            HEADER      ">NUMBER", 0
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
; BASE, a . at its end for a double; or 'c', a char
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
            beq         @not
            lda         cnt
            pha
            jsr         twotor_call                         ; ud 0 0, then the text
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
            bit         cnt
            bpl         :+
            jsr         dnegate
:
            lda         #2
            bra         @done

@single:
            inx
            inx
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

; S": ( -- c-addr u )
xsquote:
            jsr         inline_str
            jmp         count_w

; C": ( -- c-addr )
xcquote:
            jsr         inline_str
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

; The string (c-addr u on the stack) compiled after a jsr to routine .A/.Y: the routine, the count, the chars
comp_str:
            pha
            lda         #JSR_OP
            jsr         ccomma_a
            pla
            jsr         comma_ay
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

            HEADERQ     "S", F_IMMEDIATE
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

            HEADERQ     ".", F_IMMEDIATE
dotquote:
            lda         #'"'
            sta         cnt
            jsr         parse_to
            lda         #<xdotq
            ldy         #>xdotq
            jmp         comp_str

            HEADERQ     "C", F_IMMEDIATE
cquote:
            lda         #'"'
            sta         cnt
            jsr         parse_to
            lda         #<xcquote
            ldy         #>xcquote
            jmp         comp_str

            HEADERQ     "S\", F_IMMEDIATE
sbquote:                                                    ; S" with escapes: \a \b \e \f \l \m \n \q \r \t \v \z
            jsr         src_rest                            ;   \" \\ \xHH (into pad, then as S")
            lda         #<pad
            sta         w2
            lda         #>pad
            sta         w2 + 1
            stz         cnt                                 ; (cnt: the bytes in pad)
@char:
            jsr         @get
            bcs         @end
            cmp         #'"'
            beq         @end
            cmp         #'\'
            bne         @put
            jsr         @get
            bcs         @end
            ldy         #ESC_N - 1
:
            cmp         esc_from,y
            beq         @esc
            dey
            bpl         :-
            cmp         #'x'
            beq         @hex
            cmp         #'m'
            bne         @put
            lda         #$0D                                ; (\m: CR LF)
            jsr         @store
            lda         #LF
            bra         @put
@esc:
            lda         esc_to,y
@put:
            jsr         @store
            bra         @char
@hex:
            jsr         @get
            jsr         @nibble
            asl
            asl
            asl
            asl
            sta         tmp2
            jsr         @get
            jsr         @nibble
            ora         tmp2
            bra         @put
@end:
            dex
            lda         #<pad
            sta         dlo,x
            lda         #>pad
            sta         dhi,x
            dex
            lda         cnt
            sta         dlo,x
            stz         dhi,x
            jmp         str_comp

@get:                                                       ; The source's next char (C = 1: none), >IN past it
            lda         tmp
            ora         tmp + 1
            beq         @none
            lda         (w)
            pha
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
            pla
            clc
            rts
@none:
            sec
            rts

@store:
            ldy         cnt
            cpy         #PAD_SIZE
            bcs         :+
            sta         (w2),y
            inc         cnt
:
            rts

@nibble:
            phx                                             ; (A hex digit, any base)
            ldx         base
            ldy         #16
            sty         base
            jsr         digit
            stx         base
            plx
            bcc         :+
            lda         #0
:
            rts

esc_from:   .byte       "abefnqrtvz", $22, $5C, "l"
ESC_N       = * - esc_from
esc_to:     .byte       7, 8, 27, 12, LF, $22, $0D, 9, 11, 0, $22, $5C, LF
