; ****************************************************************************
; fsys.inc - forth's second bank: the RAM headers (included by forth.s, in CODE2; the tables are in RODATA2:
; far_table, FARWORD's entries, and sys_table, forthsys.inc, tools/apigen.js's).  The second bank's words and the
; sys- words have their headers made in RAM as forth starts, as a definition's would be: none of the second bank's
; bytes is ever read from the first (only one bank is at $A000 at a time).

; Each one's header made at HERE, after the BSS (a link, its flags and name's length, its name, then its code), in
; the FORTH word list: the second bank's words' (far_table: their code jsr far_word, then their address here), then
; the sys- words' (sys_table: jsr sys_call, then the table's entry but its name).  (main's, through FAR2)
sys_build:
            lda         #<far_table
            sta         w
            lda         #>far_table
            sta         w + 1
@far:
            lda         (w)
            beq         @sys
            jsr         @header
            lda         #JSR_OP                             ; jsr far_word, and its address
            jsr         @put
            lda         #<far_word
            jsr         @put
            lda         #>far_word
            jsr         @put
            jsr         @copy1
            jsr         @copy1
            jsr         @next
            bra         @far
@sys:
            lda         #<sys_table
            sta         w
            lda         #>sys_table
            sta         w + 1
@word:
            lda         (w)
            beq         @done
            jsr         @header
            lda         #JSR_OP                             ; jsr sys_call
            jsr         @put
            lda         #<sys_call
            jsr         @put
            lda         #>sys_call
            jsr         @put
            jsr         @copy1                              ; The call's address, its flags
            jsr         @copy1
            jsr         @copy1
            jsr         @copyn                              ; Its inputs, then its outputs: a count, then them
            jsr         @copyn
            jsr         @next
            bra         @word
@done:
            rts

@header:                                                    ; The entry at w's header: w2 it, .Y past its name
            lda         here
            sta         w2
            lda         here + 1
            sta         w2 + 1
            lda         forth_wl                            ; Its link, its flags and length, its name
            jsr         @put
            lda         forth_wl + 1
            jsr         @put
            lda         (w)
            jsr         @put
            and         #LEN_MASK
            sta         cnt
            ldy         #1
:
            jsr         @copy1
            dec         cnt
            bne         :-
            rts

@next:                                                      ; FORTH's last: it; w on to the next entry
            lda         w2
            sta         forth_wl
            lda         w2 + 1
            sta         forth_wl + 1
            tya
            clc
            adc         w
            sta         w
            bcc         :+
            inc         w + 1
:
            rts

@copyn:                                                     ; A count, then that many bytes
            lda         (w),y
            sta         cnt
            jsr         @copy1
:
            lda         cnt
            beq         :+
            jsr         @copy1
            dec         cnt
            bra         :-
:
            rts

@copy1:                                                     ; The entry's next byte (.Y on)
            lda         (w),y
            iny
@put:                                                       ; .A at HERE, and HERE on
            sta         (here)
            inc         here
            bne         :+
            inc         here + 1
:
            rts
