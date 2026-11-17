; ****************************************************************************
; nameis.s - a name named (dq0.h's name_is: the main RAM's, so every overlay's chooser reaches it; in assembly, as
; the C was 550 bytes of the main RAM's and this is a third of it).  Does name w name a thing or a denizen of name k?
; The one rule (COMMANDS-CRITIQUE.md's 5.1 step F, 2026-11-07: the world's, things.c's named), each of w's words by it
; (the parser's "qualifier head", 2026-11-17, phase B: "iron key", "kite shield", "black orc": each a word of k's):
; a word of the name, or its start ("key" an iron key; "rat" a giant rat), or that word made plural ("sacks",
; "torches"), or the end of the name, a word longer than 3 ("pack" a backpack, "skin" a waterskin); "" any.  Its
; resolver's, first words', blows', tending's, new things'.
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     _name_is
            .import     popax

            .include    "zeropage.inc"

            .code

yes:        lda         #1
            ldx         #0
            rts
nope:       lda         #0
            tax
            rts

; unsigned char __fastcall__ name_is (const char* k, const char* w): 1 named, 0 not.  ptr1 the name, ptr2 the word
; (each of w's in turn), tmp2 its length; tmp1 how many of its letters word_of asks; ptr3 word_of's place
_name_is:
            sta         ptr2
            stx         ptr2+1
            jsr         popax
            sta         ptr1
            stx         ptr1+1
            lda         (ptr2)
            beq         yes                 ; ("": any)
each:       ldy         #$FF                ; (A word of w's: its length, to a space or the end)
@len:       iny
            lda         (ptr2),y
            beq         @end
            cmp         #' '
            bne         @len
@end:       sty         tmp2
            sty         tmp1
            jsr         from_k              ; (A word of the name, or its start)
            bcs         said
            ldy         tmp2
            cpy         #3
            bcc         nope
            dey
            lda         (ptr2),y
            cmp         #'s'
            bne         tail
            sty         tmp1                ; (Made plural: "sacks")
            jsr         from_k
            bcs         said
            ldy         tmp1
            dey
            lda         (ptr2),y
            cmp         #'e'
            bne         tail
            sty         tmp1                ; ("torches")
            jsr         from_k
            bcs         said
tail:       lda         tmp2                ; (The end of the name, a word longer than 3: "pack")
            cmp         #4
            bcc         nope
            ldy         #$FF
@klen:      iny
            lda         (ptr1),y
            bne         @klen
            cpy         tmp2
            bcc         nope
            beq         nope                ; (The name no longer than the word)
            tya
            sec
            sbc         tmp2
            clc
            adc         ptr1
            sta         ptr3
            lda         ptr1+1
            adc         #0
            sta         ptr3+1
            ldy         #0
@tail:      lda         (ptr3),y
            cmp         (ptr2),y
            jne         nope
            iny
            cpy         tmp2
            bne         @tail
said:       ldy         tmp2                ; (Named by this word: the next, if there's one)
            lda         (ptr2),y
            jeq         yes
            iny
            tya
            clc
            adc         ptr2
            sta         ptr2
            bcc         each
            inc         ptr2+1
            bra         each

; Does a word of the name (ptr1; word_of: of the words at ptr3) begin with tmp1 letters of ptr2's?  C set, yes
from_k:     lda         ptr1
            sta         ptr3
            lda         ptr1+1
            sta         ptr3+1
word_of:    ldy         #0
@cmp:       cpy         tmp1
            beq         @yes
            lda         (ptr3),y
            cmp         (ptr2),y
            bne         @skip
            iny
            bra         @cmp
@skip:      lda         (ptr3)              ; (Past the next space)
            beq         @no
            inc         ptr3
            bne         @sk2
            inc         ptr3+1
@sk2:       cmp         #' '
            bne         @skip
            bra         word_of
@yes:       sec
            rts
@no:        clc
            rts
