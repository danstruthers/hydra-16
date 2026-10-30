; ****************************************************************************
; lbsaid.s - a look's words said (dq0.h's lb_said: words.c's and seen.c's lb_out, the main RAM's so both overlays
; reach it): lb's words so far, the first on a line of its own (msg's, as a look's said), the rest after it (say's);
; but not a sentence a look has said already this answer (the user's: nothing told twice; a light lit, the place told
; again as it shows it now: only what's new).  Each sentence, to a full stop and a space or the end, known by a hash
; of its letters (but the spaces and new lines before them) and of where it's said of (lb_salt: the character's square
; as a journey's leg comes there, main.c's E_ARRIVE; but a thing's sentence, its first letter after a 2, seen.c's, of nothing: the 2
; left out of the words said; so a place's words are told again where it's elsewhere, a journey's next leg, but the same
; thing seen from two places once), kept in
; look.c's told (ntold of them, none as a command's taken, main.c's; TOLD at most, then the oldest given up for the
; newest); lb made of the ones not said yet, then said; lb begun again.  lb_tell (dq0.h's): lb's first sentence said
; whatever (a stir's "Something stirs."), the rest but those said already this answer (a denizen gone off and back,
; its words the same: the told-once rule whatever the tasks' interleaving, 2026-10-30), each kept as told, as lb_said's
; are (look.c's stir: a denizen come into sight told as it stirs, so the look after, a walk stopped by it, doesn't
; tell it again).
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     _lb_said, _lb_tell, _lb_salt
            .import     _lb, _lbe, _lbn, _told, _ntold, _say, _msg, _t_flush

            .include    "zeropage.inc"

TOLD        = 24                    ; (dq0.h's)

            .bss
h:          .res        2           ; A sentence's hash
_lb_salt:   .res        2           ; (Where it's said of: its hash begun with it)
tnext:      .res        1           ; (The told list full: the next to give up, the oldest)
lead:       .res        1           ; (Its spaces before it still)
all:        .res        1           ; (lb_tell's: $80, its first sentence said whatever)

            .code
_lb_tell:   lda         #$80
            bra         :+
_lb_said:   lda         #0
:           sta         all
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
            lda         _lb_salt
            sta         h
            lda         _lb_salt + 1
            sta         h + 1
            lda         #1
            sta         lead
scan:       lda         (ptr1)
            beq         ended
            cmp         #'.'
            bne         letter
            ldy         #1                  ; (A full stop, a space, a thing's mark or the end after it: the
            lda         (ptr1),y            ;   sentence's end)
            beq         stop
            cmp         #' '
            beq         stop
            cmp         #2
            beq         stop
            lda         #'.'
letter:     ldx         lead
            beq         hash
            cmp         #' '
            beq         next
            cmp         #10
            beq         next
            cmp         #2
            bne         :+
            stz         h                   ; (A thing's: said of nothing, not of the place)
            stz         h + 1
            bra         next
:           stz         lead
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
ended:      bit         all                 ; (lb_tell's first sentence, a stir's "Something stirs.": said, and kept
            bpl         @check              ;   as told; the rest as a look's: none told already this answer
            stz         all                 ;   said again, the told-once rule whatever the interleaving,
            bra         @new                ;   2026-10-30)
@check:
            ldy         #0                  ; Said already?
            ldx         _ntold
@told:      beq         @new
            lda         _told,y
            cmp         h
            bne         :+
            lda         _told + 1,y
            cmp         h + 1
            jeq         sentence            ; (Yes: not again)
:           iny
            iny
            dex
            bra         @told
@new:       lda         _ntold
            cmp         #TOLD
            bcc         @add
            lda         tnext               ; (Full: over the oldest, in turn)
            inc         tnext
            ldx         tnext
            cpx         #TOLD
            bcc         :+
            stz         tnext
:           bra         @put
@add:       inc         _ntold
            stz         tnext
@put:       asl         a
            tay
            lda         h
            sta         _told,y
            lda         h + 1
            sta         _told + 1,y
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
            cmp         #2
            beq         @mark               ; (A thing's mark: not said)
            sta         (ptr2)
            inc         ptr2
            bne         @mark
            inc         ptr2 + 1
@mark:      inc         ptr3
            bne         @move
            inc         ptr3 + 1
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
