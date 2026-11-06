; ****************************************************************************
; letter.s - (dq0w) the letters of what the character carries (hub.h's designators, the user's, 2026-11-06), in
; assembly (the parts' room): each thing its letter as what it carries is made for the UI (part 102's, things.c's
; ix_make), and a thing's put in the inventory's words (part 11's, things.c's contents).
;
;   void __fastcall__ ix_letters (unsigned* low);   (Part 102's) each of the ixl_n things in ixd (their keys: an id,
;                                               or a record's number | LT_REC) its letter, into ixc's let (its place
;                                               in low plus 1, 0 none): its own, or by its record's number till it has
;                                               an id (that letter its id's now); one with none, of a ... z the first
;                                               free, else the one whose thing was let go of the longest ago (by low's
;                                               stamps after it, lowt, ixl_t now), then of aa ... az the same; each
;                                               carried stamped ixl_t.  The UI bank in the window (low its table)
;   void __fastcall__ lput (unsigned key, unsigned* low);   (Part 11's) the letter of the thing whose key is key,
;                                               "(c) ", put (tlist.s's put: the hub's header in the window after);
;                                               none, nothing.  The UI bank in the window
; ****************************************************************************

            .setcpu     "65C02"
            .export     _ix_letters, _ixl_n, _ixl_t, _ixl_old, _lput
            .import     _ixd, _ixi, _ixc, _put, popax
            .importzp   ptr1, ptr2, ptr3

NLET        = 52                    ; (hub.h's)
HALF        = NLET / 2
LT_REC      = $80                   ; (hub.h's, its high byte)
CIX         = 4                     ; (struct cix's size; its let the fourth)

            .segment    "SW102RODATA"
_ixl_n:     .res        1           ; The things
_ixl_t:     .res        2           ; The clock's stamp
_ixl_old:   .res        2           ; ix as it was last made (the UI bank's, its let a hint)
mark:       .res        NLET        ; (Each letter: 1 if its thing's carried now)
m:          .res        1           ; (The thing)
key:        .res        2           ; (find's)
id:         .res        2           ; (Its key)
b:          .res        1           ; (The letter chosen, $FF none yet)
e:          .res        1           ; (The first of the half looked through)
o:          .res        2           ; (The longest ago so far)
d:          .res        2

            .segment    "SW102CODE"

; .A = the letter whose word in low (ptr1) is key, or NLET
find:       ldy         #0
@look:      lda         (ptr1),y
            cmp         key
            bne         @next
            iny
            lda         (ptr1),y
            dey
            cmp         key + 1
            beq         @found
@next:      iny
            iny
            cpy         #NLET * 2
            bne         @look
@found:     tya
            lsr         a
            rts

; low[.A] = id.  Keeps .A
setlow:     pha
            asl         a
            tay
            lda         id
            sta         (ptr1),y
            iny
            lda         id + 1
            sta         (ptr1),y
            pla
            rts

; Letter .A's thing carried now: marked, stamped.  Keeps .A
stamp:      tax
            lda         #1
            sta         mark,x
            txa
            asl         a
            tay
            lda         _ixl_t
            sta         (ptr2),y
            iny
            lda         _ixl_t + 1
            sta         (ptr2),y
            txa
            rts

; The letter thing m had at its place in ix as it was last made (ixl_old's let), if it's still its key's: .A, C = 0; else
; C = 1.  Keeps .X
hint:       lda         m
            asl         a
            asl         a
            ora         #3                  ; (struct cix's let, m * 4 + 3)
            tay
            lda         (ptr3),y
            beq         @no
            dec         a
            cmp         #NLET
            bcs         @no
            pha
            asl         a
            tay
            lda         (ptr1),y
            cmp         key
            bne         @not
            iny
            lda         (ptr1),y
            cmp         key + 1
            bne         @not
            pla
            clc
            rts
@not:       pla
@no:        sec
            rts

; Thing m's let = .A
setlet:     pha
            lda         m
            asl         a
            asl         a
            tax
            pla
            sta         _ixc + 3,x
            rts

; Thing m's key into id, and .X = m * 2
getid:      lda         m
            asl         a
            tax
            lda         _ixd,x
            sta         id
            sta         key
            lda         _ixd + 1,x
            sta         id + 1
            sta         key + 1
            rts

_ix_letters:
            ldy         _ixl_old            ; (ix as it was last made: the hints)
            sty         ptr3
            ldy         _ixl_old + 1
            sty         ptr3 + 1
            sta         ptr1
            stx         ptr1 + 1
            clc
            adc         #NLET * 2           ; (lowt after it)
            sta         ptr2
            txa
            adc         #0
            sta         ptr2 + 1
            ldx         #NLET - 1
@clear:     stz         mark,x
            dex
            bpl         @clear
            stz         m                   ; The first pass: those with a letter
@one:       lda         m
            cmp         _ixl_n
            bcs         pass2
            jsr         getid
            jsr         hint                ; (The letter it had at its place in ix last: most often its own)
            bcc         @have
            jsr         find
            cmp         #NLET
            bcc         @have
            bit         id + 1
            bmi         @none               ; (No id: by its record's number already)
            lda         _ixi,x              ; (Its id now: the letter it had by its record's number)
            sta         key
            lda         _ixi + 1,x
            ora         #LT_REC
            sta         key + 1
            jsr         find
            cmp         #NLET
            bcs         @none
            jsr         setlow
@have:      jsr         stamp
            inc         a
            bra         @let
@none:      lda         #$FF                ; (One to give a letter: the second pass)
@let:       jsr         setlet
            inc         m
            bra         @one

pass2:      stz         m                   ; The second pass: those with none
@one:       lda         m
            cmp         _ixl_n
            bcc         :+
            rts
:           asl         a
            asl         a
            tax
            lda         _ixc + 3,x
            cmp         #$FF
            beq         :+
            jmp         @next
:           jsr         getid
            lda         #$FF
            sta         b
            stz         e
@half:      lda         b                   ; (One in a ... z: never aa ... az)
            cmp         #$FF
            bne         @chosen
            lda         e
            cmp         #NLET
            bcs         @chosen
            stz         o
            stz         o + 1
            ldx         e
@j:         txa
            asl         a
            tay
            lda         (ptr1),y
            iny
            ora         (ptr1),y
            beq         @free
            lda         mark,x
            bne         @nj                 ; (Its thing carried)
            dey
            sec                             ; (How long since its thing was let go of)
            lda         _ixl_t
            sbc         (ptr2),y
            sta         d
            iny
            lda         _ixl_t + 1
            sbc         (ptr2),y
            sta         d + 1
            cmp         o + 1
            bcc         @nj
            bne         @take
            lda         d
            cmp         o
            bcc         @nj
@take:      lda         d
            sta         o
            lda         d + 1
            sta         o + 1
            stx         b
@nj:        inx
            txa
            sec
            sbc         e
            cmp         #HALF
            bcc         @j
            lda         e
            clc
            adc         #HALF
            sta         e
            bra         @half
@free:      stx         b
@chosen:    lda         b
            cmp         #$FF
            beq         @no
            jsr         setlow
            jsr         stamp
            inc         a
            bra         @set
@no:        lda         #0
@set:       jsr         setlet
@next:      inc         m
            jmp         @one

            .segment    "SW11RODATA"
lbuf:       .res        6               ; ("(aa) ")
lkey:       .res        2

            .segment    "SW11CODE"

_lput:      sta         ptr1
            stx         ptr1 + 1
            jsr         popax
            sta         lkey
            stx         lkey + 1
            ldy         #0
@look:      lda         (ptr1),y
            cmp         lkey
            bne         @next
            iny
            lda         (ptr1),y
            dey
            cmp         lkey + 1
            beq         @found
@next:      iny
            iny
            cpy         #NLET * 2
            bne         @look
            rts                             ; (None)
@found:     tya
            lsr         a
            ldx         #0
            ldy         #'('
            sty         lbuf
            cmp         #HALF
            bcc         @single
            sbc         #HALF               ; (Two letters: an a first)
            ldy         #'a'
            sty         lbuf + 1
            inx
@single:    clc
            adc         #'a'
            sta         lbuf + 1,x
            lda         #')'
            sta         lbuf + 2,x
            lda         #' '
            sta         lbuf + 3,x
            stz         lbuf + 4,x
            lda         #<lbuf
            ldx         #>lbuf
            jmp         _put
