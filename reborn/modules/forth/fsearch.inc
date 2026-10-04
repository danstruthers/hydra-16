; ****************************************************************************
; fsearch.inc - forth's word lists and search order: the Search-Order words (included by forth.s).  A word list (its
; wid) is 4 bytes: its last header (0: none yet), then the word list made before it (wl_last's chain: WORDLIST's are
; in the dictionary, FORTH's in the BSS).  The search order is order: order_n wids, the first searched first; new
; definitions go into current's.

            HEADER      "FORTH-WORDLIST", 0
forthwordlist:
            lda         #<forth_wl
            ldy         #>forth_wl
            PUSHAY
            rts

            HEADER      "GET-CURRENT", 0
getcurrent:
            lda         current
            ldy         current + 1
            PUSHAY
            rts

            HEADER      "SET-CURRENT", 0
setcurrent:
            lda         dlo,x
            sta         current
            lda         dhi,x
            sta         current + 1
            inx
            rts

            HEADER      "DEFINITIONS", 0
definitions:                                                ; The first word list in the order: definitions' (an
            lda         order_n                             ;   empty order: as they were)
            beq         :+
            lda         order
            sta         current
            lda         order + 1
            sta         current + 1
:
            rts

            HEADER      "WORDLIST", 0
wordlist:                                                   ; ( -- wid ): a new one, empty, in the dictionary
            jsr         here_
            lda         #0
            tay
            jsr         comma_ay
            lda         wl_last
            ldy         wl_last + 1
            jsr         comma_ay
            lda         dlo,x
            sta         wl_last
            lda         dhi,x
            sta         wl_last + 1
            rts

            HEADER      "SEARCH-WORDLIST", 0
searchwordlist:                                             ; ( c-addr u wid -- 0 | xt 1 | xt -1 )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            jsr         find_wl
            inx
            bcc         :+
            jmp         zero_tos
:
            jmp         found_xt

            HEADER      "GET-ORDER", 0
getorder:                                                   ; ( -- widn ... wid1 n ): wid1 the first searched
            lda         order_n
            asl
            tay
@wid:
            dey
            dey
            bmi         @n
            lda         order,y
            sta         tmp
            lda         order + 1,y
            phy
            tay
            lda         tmp
            PUSHAY
            ply
            bra         @wid
@n:
            lda         order_n
            ldy         #0
            PUSHAY
            rts

            HEADER      "SET-ORDER", 0
setorder:                                                   ; ( widn ... wid1 n -- ): n -1, ONLY's; more than
            lda         dhi,x                               ;   ORDER_MAX: THROW -49
            bpl         :+
            inx
            bra         only
:
            bne         @full
            lda         dlo,x
            cmp         #ORDER_MAX + 1
            bcc         :+
@full:
            lda         #<-49
            jmp         throw_a
:
            sta         order_n
            inx
            ldy         #0
@wid:
            tya
            lsr
            cmp         order_n
            bcs         @done
            lda         dlo,x
            sta         order,y
            lda         dhi,x
            sta         order + 1,y
            inx
            iny
            iny
            bra         @wid
@done:
            rts

            HEADER      "ONLY", 0
only:                                                       ; The search order: FORTH alone
            lda         #<forth_wl
            sta         order
            lda         #>forth_wl
            sta         order + 1
            lda         #1
            sta         order_n
            rts

            HEADER      "ALSO", 0
also:                                                       ; The first word list in the order twice (full: THROW -49)
            lda         order_n
            beq         @done
            cmp         #ORDER_MAX
            bcc         :+
            lda         #<-49
            jmp         throw_a
:
            asl
            tay
:
            lda         order - 1,y
            sta         order + 1,y
            dey
            bne         :-
            inc         order_n
@done:
            rts

            HEADER      "PREVIOUS", 0
previous:                                                   ; The first word list out of the order (none: THROW -50)
            lda         order_n
            bne         :+
            lda         #<-50
            jmp         throw_a
:
            dec         order_n
            ldy         #0
:
            lda         order + 2,y
            sta         order,y
            iny
            cpy         #ORDER_MAX * 2 - 2
            bne         :-
            rts

            HEADER      "FORTH", 0
forth_w:                                                    ; FORTH the first word list in the order (an empty
            lda         order_n                             ;   order: it alone)
            bne         :+
            inc         order_n
:
            lda         #<forth_wl
            sta         order
            lda         #>forth_wl
            sta         order + 1
            rts

            HEADER      "ORDER", 0
order_w:                                                    ; The search order, the first first, then the
            ldy         #0                                  ;   compilation word list
@wid:
            tya
            lsr
            cmp         order_n
            bcs         @current
            phy
            lda         order,y
            pha
            lda         order + 1,y
            tay
            pla
            jsr         wl_name
            ply
            iny
            iny
            bra         @wid
@current:
            LDR         w, s_current
            jsr         type_z
            lda         current
            ldy         current + 1
            jsr         wl_name
            jmp         cr

; The word list .A/.Y's name out, and a space: FORTH, or its address
wl_name:
            cmp         #<forth_wl
            bne         @addr
            cpy         #>forth_wl
            bne         @addr
            LDR         w, s_forth
            jmp         type_z
@addr:
            PUSHAY
            lda         base
            pha
            lda         #16
            sta         base
            lda         #'$'
            jsr         emit_a
            jsr         udot
            pla
            sta         base
            rts

s_forth:    .byte       "FORTH ", 0
s_current:  .byte       " current: ", 0
