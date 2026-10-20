; ****************************************************************************
; lbsaid.s - a look's words said (dq0.h's lb_said: words.c's and seen.c's lb_out, the main RAM's so both overlays
; reach it): lb's words so far, the first on a line of its own (msg's, as a look's said), the rest after it (say's);
; but not a sentence a look has said already this answer (the user's: nothing told twice; a light lit, the place told
; again as it shows it now: only what's new).  Each sentence, to a full stop and a space or the end, known by a hash
; of its letters (but the spaces and new lines before them), kept in look.c's told (ntold of them: none as a
; command's taken, main.c's); lb made of the ones not said yet, then said; lb begun again.
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     _lb_said
            .import     _lb, _lbe, _lbn, _told, _ntold, _say, _msg, _t_flush

            .include    "zeropage.inc"

TOLD        = 24                    ; (dq0.h's)

            .bss
h:          .res        2           ; A sentence's hash
lead:       .res        1           ; (Its spaces before it still)

            .code
_lb_said:
            lda         #<_lb               ; ptr1 read, ptr2 written: lb made again of what's new
            sta         ptr1
            sta         ptr2
            lda         #>_lb
            sta         ptr1 + 1
            sta         ptr2 + 1
sentence:   lda         (ptr1)
            jeq         done
            lda         ptr1                ; ptr3 its start
            sta         ptr3
            lda         ptr1 + 1
            sta         ptr3 + 1
            stz         h
            stz         h + 1
            lda         #1
            sta         lead
scan:       lda         (ptr1)
            beq         ended
            cmp         #'.'
            bne         letter
            ldy         #1                  ; (A full stop, a space or the end after it: the sentence's end)
            lda         (ptr1),y
            beq         stop
            cmp         #' '
            beq         stop
            lda         #'.'
letter:     ldx         lead
            beq         hash
            cmp         #' '
            beq         next
            cmp         #10
            beq         next
            stz         lead
hash:       sta         tmp1                ; h = h rotated left a bit, the letter in its low byte
            asl         h
            rol         h + 1
            lda         h
            adc         #0
            eor         tmp1
            sta         h
next:       inc         ptr1
            bne         scan
            inc         ptr1 + 1
            bra         scan
stop:       inc         ptr1                ; (Its full stop with it)
            bne         ended
            inc         ptr1 + 1
ended:      ldy         #0                  ; Said already?
            ldx         _ntold
@told:      beq         @new
            lda         _told,y
            cmp         h
            bne         :+
            lda         _told + 1,y
            cmp         h + 1
            beq         sentence            ; (Yes: not again)
:           iny
            iny
            dex
            bra         @told
@new:       lda         _ntold
            cmp         #TOLD
            bcs         @copy
            asl         a
            tay
            lda         h
            sta         _told,y
            lda         h + 1
            sta         _told + 1,y
            inc         _ntold
@copy:      lda         _lbn                ; (The look's first words: no spaces before them, at a line's start)
            ora         _lbn + 1
            bne         @move
            lda         ptr2
            cmp         #<_lb
            bne         @move
            lda         ptr2 + 1
            cmp         #>_lb
            bne         @move
@space:     lda         ptr3
            cmp         ptr1
            bne         :+
            lda         ptr3 + 1
            cmp         ptr1 + 1
            jeq         sentence
:           lda         (ptr3)
            cmp         #' '
            bne         @move
            inc         ptr3
            bne         @space
            inc         ptr3 + 1
            bra         @space
@move:      lda         ptr3                ; It, from ptr3 to ptr1, to ptr2 on
            cmp         ptr1
            bne         :+
            lda         ptr3 + 1
            cmp         ptr1 + 1
            jeq         sentence
:           lda         (ptr3)
            sta         (ptr2)
            inc         ptr3
            bne         :+
            inc         ptr3 + 1
:           inc         ptr2
            bne         @move
            inc         ptr2 + 1
            bra         @move
done:       lda         #0
            sta         (ptr2)
            sec                             ; Its bytes (h): any to say?
            lda         ptr2
            sbc         #<_lb
            sta         h
            lda         ptr2 + 1
            sbc         #>_lb
            sta         h + 1
            ora         h
            beq         again
            lda         _lbn
            ora         _lbn + 1
            bne         @say
            lda         #<_lb
            ldx         #>_lb
            jsr         _msg
            bra         @said
@say:       lda         #<_lb
            ldx         #>_lb
            jsr         _say
@said:      clc
            lda         _lbn
            adc         h
            sta         _lbn
            lda         _lbn + 1
            adc         h + 1
            sta         _lbn + 1
            jsr         _t_flush
again:      lda         #<_lb               ; lb begun again
            sta         _lbe
            lda         #>_lb
            sta         _lbe + 1
            lda         #0
            sta         _lb
            rts
