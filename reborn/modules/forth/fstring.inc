; ****************************************************************************
; fstring.inc - forth's String words, and the Facility words (included by forth.s).  REPLACES's substitutions are in
; substs, each its name (counted) then its text (counted): SUBSTITUTE finds a name in either case.  The Facility's
; screen words are the terminal's ANSI sequences; KEY? reads the console's cons non-blocking, in raw mode.

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

            HEADER      "UNESCAPE", 0
unescape:                                                   ; ( c-addr1 u1 c-addr2 -- c-addr2 u2 ): each % doubled
            lda         dlo,x
            sta         w2
            sta         w3
            lda         dhi,x
            sta         w2 + 1
            sta         w3 + 1
            lda         dlo + 1,x
            sta         tmp
            lda         dhi + 1,x
            sta         tmp + 1
            lda         dlo + 2,x
            sta         w
            lda         dhi + 2,x
            sta         w + 1
            inx
@char:
            lda         tmp
            ora         tmp + 1
            beq         @done
            lda         (w)
            cmp         #'%'
            bne         :+
            jsr         @put
            lda         #'%'
:
            jsr         @put
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
@done:
            lda         w2                                  ; ( c-addr2 u2 ): u2 = w3 - c-addr2
            sta         dlo + 1,x
            lda         w2 + 1
            sta         dhi + 1,x
            sec
            lda         w3
            sbc         w2
            sta         dlo,x
            lda         w3 + 1
            sbc         w2 + 1
            sta         dhi,x
            rts
@put:
            sta         (w3)
            inc         w3
            bne         :+
            inc         w3 + 1
:
            rts

            HEADER      "REPLACES", 0
replaces:                                                   ; ( c-addr1 u1 c-addr2 u2 -- ): SUBSTITUTE's %c-addr2%
            lda         dlo + 1,x                           ;   (its name) c-addr1 u1, copied (no room: THROW -79)
            sta         p2
            lda         dhi + 1,x
            sta         p2 + 1
            lda         dlo,x
            jsr         subst_find
            bcs         @add
            sec                                             ; One there by its name: out (those after it down over
            lda         (p2)                                ;   it)
            adc         p2
            sta         w3
            lda         p2 + 1
            adc         #0
            sta         w3 + 1
            phx
            sec
            lda         w3
            sbc         #<substs
            tax
            sec
            lda         p1
            sbc         #<substs
            tay
:
            cpx         subst_len
            bcs         :+
            lda         substs,x
            sta         substs,y
            inx
            iny
            bra         :-
:
            sty         subst_len
            plx
@add:
            lda         dhi,x                               ; Room: its name and text, counted?
            ora         dhi + 2,x
            bne         @full
            clc
            lda         subst_len
            adc         dlo,x
            bcs         @full
            adc         dlo + 2,x
            bcs         @full
            adc         #2                                  ; (SUBST_SIZE is 255: a carry is too many)
            bcs         @full
            lda         dlo + 1,x                           ; Its name, then its text
            sta         p2
            lda         dhi + 1,x
            sta         p2 + 1
            ldy         subst_len
            lda         dlo,x
            jsr         @copy
            lda         dlo + 3,x
            sta         p2
            lda         dhi + 3,x
            sta         p2 + 1
            lda         dlo + 2,x
            jsr         @copy
            sty         subst_len
            txa
            clc
            adc         #4
            tax
            rts
@full:
            lda         #<-79
            jmp         throw_a
@copy:                                                      ; .A bytes from (p2), counted, into substs at .Y
            sta         cnt
            sta         substs,y
            iny
:
            lda         cnt
            beq         :+
            lda         (p2)
            sta         substs,y
            iny
            inc         p2
            bne         @n
            inc         p2 + 1
@n:
            dec         cnt
            bra         :-
:
            rts

; The substitution named (p2, .A chars), in either case: C = 0, p1 its record, p2 its text (counted); or C = 1
subst_find:
            sta         cnt
            LDR         p1, substs
@rec:
            sec                                             ; (Past the last: none)
            lda         p1
            sbc         #<substs
            cmp         subst_len
            bcs         @none
            lda         (p1)
            cmp         cnt
            bne         @next
            tay
@char:
            cpy         #0
            beq         @found
            lda         (p1),y
            jsr         upper
            sta         numtmp
            dey
            lda         (p2),y
            jsr         upper
            cmp         numtmp
            bne         @next
            bra         @char
@found:
            sec                                             ; (Its text: after its name)
            lda         (p1)
            adc         p1
            sta         p2
            lda         p1 + 1
            adc         #0
            sta         p2 + 1
            clc
            rts
@next:
            sec                                             ; Past its name, then its text
            lda         (p1)
            adc         p1
            sta         p1
            bcc         :+
            inc         p1 + 1
:
            sec
            lda         (p1)
            adc         p1
            sta         p1
            bcc         @rec
            inc         p1 + 1
            bra         @rec
@none:
            sec
            rts

            HEADER      "SUBSTITUTE", 0
substitute:                                                 ; ( c-addr1 u1 c-addr2 u2 -- c-addr2 u3 n ): c-addr1 u1
            lda         dlo,x                               ;   into c-addr2, each %name% REPLACES's text (n of
            sta         tmp2                                ;   them), %% a %; n -78: no room, or the two overlap
            lda         dhi,x
            sta         tmp2 + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            lda         dlo + 2,x
            sta         tmp
            lda         dhi + 2,x
            sta         tmp + 1
            lda         dlo + 3,x
            sta         w
            lda         dhi + 3,x
            sta         w + 1
            stz         tmp3                                ; (tmp3: the chars out; numacc: n, then no room)
            stz         tmp3 + 1
            stz         numacc
            stz         numacc + 1
            stz         numacc + 2
            clc                                             ; Overlapping: c-addr1 < c-addr2 + u2 and c-addr2 <
            lda         w2                                  ;   c-addr1 + u1
            adc         tmp2
            sta         p1
            lda         w2 + 1
            adc         tmp2 + 1
            sta         p1 + 1
            lda         w
            cmp         p1
            lda         w + 1
            sbc         p1 + 1
            bcs         @char
            clc
            lda         w
            adc         tmp
            sta         p1
            lda         w + 1
            adc         tmp + 1
            sta         p1 + 1
            lda         w2
            cmp         p1
            lda         w2 + 1
            sbc         p1 + 1
            bcs         @char
            inc         numacc + 2
            jmp         @end
@char:
            lda         tmp
            ora         tmp + 1
            bne         :+
            jmp         @end
:
            lda         (w)
            cmp         #'%'
            beq         @pct
@lit:
            jsr         @out
            lda         #1
@adv:                                                       ; On .A chars
            sta         cnt
            clc
            adc         w
            sta         w
            bcc         :+
            inc         w + 1
:
            sec
            lda         tmp
            sbc         cnt
            sta         tmp
            bcs         @char
            dec         tmp + 1
            bra         @char
@pct:
            lda         tmp + 1                             ; %%: a %
            bne         :+
            lda         tmp
            cmp         #2
            bcc         @lone
:
            ldy         #1
            lda         (w),y
            cmp         #'%'
            bne         @name
            jsr         @out
            lda         #2
            bra         @adv
@name:
            ldy         #1                                  ; The % that ends the name (within 255 chars)
@scan:
            iny
            beq         @lone
            lda         tmp + 1
            bne         :+
            cpy         tmp
            bcs         @lone
:
            lda         (w),y
            cmp         #'%'
            bne         @scan
            phy
            clc
            lda         w
            adc         #1
            sta         p2
            lda         w + 1
            adc         #0
            sta         p2 + 1
            tya
            dec
            jsr         subst_find
            ply
            bcs         @lone
            phy                                             ; Its text out
            lda         (p2)
            sta         cnt
            ldy         #0
:
            cpy         cnt
            beq         :+
            iny
            lda         (p2),y
            jsr         @out
            bra         :-
:
            inc         numacc
            pla
            inc
            bra         @adv
@lone:
            lda         #'%'                                ; (No name ends: the % as it is)
            bra         @lit
@end:
            lda         w2                                  ; ( c-addr2 u3 n )
            sta         dlo + 3,x
            lda         w2 + 1
            sta         dhi + 3,x
            lda         tmp3
            sta         dlo + 2,x
            lda         tmp3 + 1
            sta         dhi + 2,x
            lda         numacc
            sta         dlo + 1,x
            stz         dhi + 1,x
            lda         numacc + 2
            beq         :+
            lda         #<-78
            sta         dlo + 1,x
            lda         #$FF
            sta         dhi + 1,x
:
            inx
            rts
@out:                                                       ; .A out (no room: noted).  Keeps .Y
            pha
            lda         tmp3
            cmp         tmp2
            lda         tmp3 + 1
            sbc         tmp2 + 1
            bcs         @over
            clc
            lda         w2
            adc         tmp3
            sta         p1
            lda         w2 + 1
            adc         tmp3 + 1
            sta         p1 + 1
            pla
            sta         (p1)
            inc         tmp3
            bne         :+
            inc         tmp3 + 1
:
            rts
@over:
            pla
            lda         #1
            sta         numacc + 2
            rts

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
