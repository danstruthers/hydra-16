; ****************************************************************************
; fsys.inc - forth's second bank (included by forth.s, in CODE2; the table after it, in RODATA2: forthsys.inc,
; tools/apigen.js's).  The sys- words' headers are made in RAM as forth starts, as a definition's would be: none of
; the second bank's bytes is ever read from the first (only one bank is at $A000 at a time).

; Each sys- word's header made at HERE, after the BSS (a link, its name and length, then its code: jsr sys_call, and
; its descriptor, the table's entry but its name), in the FORTH word list.  (main's, through FAR2)
sys_build:
            lda         #<sys_table
            sta         w
            lda         #>sys_table
            sta         w + 1
@word:
            lda         (w)
            beq         @done
            lda         here                                ; (w2: the header)
            sta         w2
            lda         here + 1
            sta         w2 + 1
            lda         forth_wl                            ; Its link, its name
            jsr         @put
            lda         forth_wl + 1
            jsr         @put
            lda         (w)
            sta         cnt
            jsr         @put
            ldy         #1
:
            jsr         @copy1
            dec         cnt
            bne         :-
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
            lda         w2                                  ; FORTH's last
            sta         forth_wl
            lda         w2 + 1
            sta         forth_wl + 1
            tya                                             ; The next entry
            clc
            adc         w
            sta         w
            bcc         @word
            inc         w + 1
            bra         @word
@done:
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
