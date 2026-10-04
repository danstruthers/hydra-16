; ****************************************************************************
; fstring.inc - forth's String words, and the Facility words (included by forth.s; UNESCAPE, REPLACES and SUBSTITUTE
; are the second bank's: fbank2.inc).  The Facility's screen words are the terminal's ANSI sequences; KEY? reads the
; console's cons non-blocking, in raw mode.

; ---- Strings

            HEADER      "-TRAILING", 0
dtrailing:                                                  ; ( c-addr u1 -- c-addr u2 ): without the spaces at its end
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
@char:
            lda         dlo,x
            ora         dhi,x
            beq         @done
            clc                                             ; (Its last char: w + u - 1)
            lda         w
            adc         dlo,x
            sta         w2
            lda         w + 1
            adc         dhi,x
            sta         w2 + 1
            lda         w2
            bne         :+
            dec         w2 + 1
:
            dec         w2
            lda         (w2)
            cmp         #' '
            bne         @done
            jsr         oneminus
            bra         @char
@done:
            rts

            HEADER      "/STRING", 0
slashstring:                                                ; ( c-addr u n -- c-addr+n u-n )
            clc
            lda         dlo + 2,x
            adc         dlo,x
            sta         dlo + 2,x
            lda         dhi + 2,x
            adc         dhi,x
            sta         dhi + 2,x
            sec
            lda         dlo + 1,x
            sbc         dlo,x
            sta         dlo + 1,x
            lda         dhi + 1,x
            sbc         dhi,x
            sta         dhi + 1,x
            inx
            rts

            HEADER      "BLANK", 0
blank:                                                      ; ( c-addr u -- )
            dex
            lda         #' '
            sta         dlo,x
            stz         dhi,x
            jmp         fill

            HEADER      "COMPARE", 0
compare:                                                    ; ( c-addr1 u1 c-addr2 u2 -- n ): -1, 0, 1, by its chars'
            lda         dlo + 3,x                           ;   values (and a shorter one first)
            sta         w
            lda         dhi + 3,x
            sta         w + 1
            lda         dlo + 2,x
            sta         tmp
            lda         dhi + 2,x
            sta         tmp + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            lda         dlo,x
            sta         tmp2
            lda         dhi,x
            sta         tmp2 + 1
            inx
            inx
            inx
@char:
            lda         tmp
            ora         tmp + 1
            bne         :+
            lda         tmp2
            ora         tmp2 + 1
            beq         @equal
            bra         @less
:
            lda         tmp2
            ora         tmp2 + 1
            beq         @more
            lda         (w)
            cmp         (w2)
            bcc         @less
            bne         @more
            inc         w
            bne         :+
            inc         w + 1
:
            inc         w2
            bne         :+
            inc         w2 + 1
:
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            lda         tmp2
            bne         :+
            dec         tmp2 + 1
:
            dec         tmp2
            bra         @char
@equal:
            jmp         zero_tos
@less:
            jmp         true_tos
@more:
            lda         #1
            sta         dlo,x
            stz         dhi,x
            rts

            HEADER      "SEARCH", 0
search:                                                     ; ( c-addr1 u1 c-addr2 u2 -- c-addr3 u3 flag ): c-addr3
            lda         dlo + 3,x                           ;   u3 from where c-addr2 u2 is in it; or (false)
            sta         w                                   ;   c-addr1 u1
            lda         dhi + 3,x
            sta         w + 1
            lda         dlo + 2,x
            sta         tmp
            lda         dhi + 2,x
            sta         tmp + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            lda         dlo,x
            sta         tmp2
            lda         dhi,x
            sta         tmp2 + 1
            inx
@try:
            lda         tmp                                 ; Shorter than it: not there
            cmp         tmp2
            lda         tmp + 1
            sbc         tmp2 + 1
            bcc         @no
            lda         w                                   ; Here?
            sta         w3
            lda         w + 1
            sta         w3 + 1
            lda         w2
            sta         p1
            lda         w2 + 1
            sta         p1 + 1
            lda         tmp2
            sta         p2
            lda         tmp2 + 1
            sta         p2 + 1
@cmp:
            lda         p2
            ora         p2 + 1
            beq         @yes
            lda         (w3)
            cmp         (p1)
            bne         @next
            inc         w3
            bne         :+
            inc         w3 + 1
:
            inc         p1
            bne         :+
            inc         p1 + 1
:
            lda         p2
            bne         :+
            dec         p2 + 1
:
            dec         p2
            bra         @cmp
@next:
            inc         w
            bne         :+
            inc         w + 1
:
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            bra         @try
@yes:
            lda         w
            sta         dlo + 2,x
            lda         w + 1
            sta         dhi + 2,x
            lda         tmp
            sta         dlo + 1,x
            lda         tmp + 1
            sta         dhi + 1,x
            jmp         true_tos
@no:
            jmp         zero_tos

            HEADER      "SLITERAL", F_IMMEDIATE
sliteral:                                                   ; ( c-addr u -- ): compiled, as S" is
            lda         #<xsquote
            ldy         #>xsquote
            jmp         comp_str

; ---- The Facility words

            HEADER      "KEY?", 0
keyq:                                                       ; ( -- flag ): a key waiting (the console in raw mode,
            lda         key_pend                            ;   till a line's read); not the console: true
            bne         @yes
            lda         interactive
            beq         @yes
            jsr         flush
            jsr         raw_on
            stx         xsave
            lda         kq_fd
            bpl         :+
            LDR         r0, s_cons
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            bcs         @no_x
            sta         kq_fd
:
            LDR         r0, key_char
            LDR         r1, 1
            lda         kq_fd
            jsr         READ
            ldx         xsave
            bcs         @no
            cmp         #0
            beq         @no
            inc         key_pend
@yes:
            dex
            jmp         true_tos
@no_x:
            ldx         xsave
@no:
            dex
            jmp         zero_tos

s_cons:     .byte       "/dev/cons", 0

            HEADER      "MS", 0
ms:                                                         ; ( u -- ): u milliseconds (in ticks: 5 ms each, at least
            lda         dlo,x                               ;   that long), the output out first
            sta         numacc
            lda         dhi,x
            sta         numacc + 1
            stz         numacc + 2
            stz         numacc + 3
            inx
            clc
            lda         numacc
            adc         #<(1000 / TICK_HZ - 1)
            sta         numacc
            bcc         :+
            inc         numacc + 1
            bne         :+
            inc         numacc + 2
:
            lda         #1000 / TICK_HZ
            jsr         div32_8
            jsr         flush
            stx         xsave
            lda         numacc
            ldx         numacc + 1
            jsr         SLEEP
            ldx         xsave
            bcc         :+
            cmp         #E_INTR
            bne         :+
            jmp         intr_throw
:
            rts

; numacc (32 bits) / .A (8 bits): numacc the quotient, .A the remainder.  Keeps .X
div32_8:
            sta         cnt
            lda         #0
            ldy         #32
@bit:
            asl         numacc
            rol         numacc + 1
            rol         numacc + 2
            rol         numacc + 3
            rol         a
            bcs         @sub
            cmp         cnt
            bcc         @next
@sub:
            sbc         cnt
            inc         numacc
@next:
            dey
            bne         @bit
            rts

            HEADER      "TIME&DATE", 0
timedate:                                                   ; ( -- +n1 +n2 +n3 +n4 +n5 +n6 ): the second, minute,
            stx         xsave                               ;   hour, day, month and year (the clock's: 2000 on)
            jsr         TIME
            ldx         xsave
            ldy         #3
:
            lda         r0,y
            sta         numacc,y
            dey
            bpl         :-
            lda         #60
            jsr         @part
            lda         #60
            jsr         @part
            lda         #24
            jsr         @part
            lda         #<2000                              ; The year: tmp (numacc: the days into it)
            sta         tmp
            lda         #>2000
            sta         tmp + 1
@year:
            jsr         @leap                               ; (cnt: 1 in a leap year)
            lda         numacc                              ; Fewer days than it has?
            cmp         #<365
            lda         numacc + 1
            sbc         #>365
            bcc         @month
            lda         numacc + 1
            cmp         #>365
            bne         :+
            lda         numacc
            cmp         #<365
            bne         :+
            lda         cnt                                 ; (365: the leap year's last day)
            bne         @month
:
            sec
            lda         numacc
            sbc         #<365
            sta         numacc
            lda         numacc + 1
            sbc         #>365
            sta         numacc + 1
            sec
            lda         numacc
            sbc         cnt
            sta         numacc
            bcs         :+
            dec         numacc + 1
:
            inc         tmp
            bne         @year
            inc         tmp + 1
            bra         @year
@month:
            ldy         #0                                  ; The month (.Y), from the days into the year
@mon:
            lda         month_days,y
            cpy         #1
            bne         :+
            clc
            adc         cnt
:
            sta         tmp2
            lda         numacc + 1
            bne         :+
            lda         numacc
            cmp         tmp2
            bcc         @found
:
            sec
            lda         numacc
            sbc         tmp2
            sta         numacc
            bcs         :+
            dec         numacc + 1
:
            iny
            bra         @mon
@found:
            inc                                             ; The day, the month, the year
            phy
            ldy         #0
            PUSHAY
            pla
            inc
            ldy         #0
            PUSHAY
            lda         tmp
            ldy         tmp + 1
            PUSHAY
            rts
@part:                                                      ; numacc / .A: the remainder pushed
            jsr         div32_8
            ldy         #0
            PUSHAY
            rts
@leap:                                                      ; cnt = 1 if tmp is a leap year (2100 isn't)
            stz         cnt
            lda         tmp
            and         #3
            bne         :+
            lda         tmp
            cmp         #<2100
            bne         @is
            lda         tmp + 1
            cmp         #>2100
            beq         :+
@is:
            inc         cnt
:
            rts

month_days: .byte       31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31

            HEADER      "AT-XY", 0
atxy:                                                       ; ( u1 u2 -- ): the cursor to column u1, row u2 (from 0)
            jsr         esc_csi
            jsr         oneplus
            jsr         dec_out
            lda         #';'
            jsr         emit_a
            jsr         oneplus
            jsr         dec_out
            lda         #'H'
            jmp         emit_a

            HEADER      "PAGE", 0
page:                                                       ; The screen cleared, the cursor at its top left
            jsr         esc_csi
            lda         #'2'
            jsr         emit_a
            lda         #'J'
            jsr         emit_a
            jsr         esc_csi
            lda         #'H'
            jmp         emit_a

; ESC [ out
esc_csi:
            lda         #27
            jsr         emit_a
            lda         #'['
            jmp         emit_a

; ( u -- ): u out in decimal
dec_out:
            lda         base
            pha
            lda         #10
            sta         base
            jsr         u_text
            jsr         type
            pla
            sta         base
            rts

            HEADER      "BEGIN-STRUCTURE", 0
beginstructure:                                             ; ( "name" -- addr 0 ): name a constant, its size
            lda         #0                                  ;   (END-STRUCTURE's: addr is its literal's place)
            jsr         make_hdr
            clc
            lda         here
            adc         #2
            pha
            lda         here + 1
            adc         #0
            tay
            pla
            PUSHAY
            lda         #0
            tay
            jsr         comp_lit
            lda         #RTS_OP
            jsr         ccomma_a
            dex
            jmp         zero_tos

            HEADER      "END-STRUCTURE", 0
endstructure:                                               ; ( addr +n -- ): the structure's size n
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         (w)
            ldy         #4                                  ; (The literal's high byte: 4 on)
            lda         dhi,x
            sta         (w),y
            inx
            inx
            rts

            HEADER      "+FIELD", 0
plusfield:                                                  ; ( n1 n2 "name" -- n3 ): name adds n1; n3 = n1 + n2
            lda         #0
            jsr         make_hdr
            lda         dlo + 1,x
            ldy         dhi + 1,x
            jsr         comp_lit
            lda         #<plus
            ldy         #>plus
            jsr         comp_jmp
            jmp         plus

            HEADER      "FIELD:", 0
fieldc:                                                     ; ( n1 "name" -- n2 ): a cell's
            lda         #2
            bra         :+

            HEADER      "CFIELD:", 0
cfieldc:                                                    ; ( n1 "name" -- n2 ): a char's
            lda         #1
:
            dex
            sta         dlo,x
            stz         dhi,x
            bra         plusfield
