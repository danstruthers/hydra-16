; ****************************************************************************
; nameis.s - a name named (dq0.h's name_is: the main RAM's, so every overlay's chooser reaches it; in assembly, as
; the C was 550 bytes of the main RAM's and this is a third of it).  Does word w name a thing or a denizen of name
; k?  The one rule (COMMANDS-CRITIQUE.md's 5.1 step F, 2026-11-07: the world's, things.c's named): a word of the name,
; or its start ("key" an iron key; "rat" a giant rat), or that word made plural ("sacks", "torches"), or the end of
; the name, a word longer than 3 ("pack" a backpack, "skin" a waterskin); "" any.  And the word said before w in the
; line (cl, main.c's), if it's one that makes one kind another (a word of quals, or its start, of 3 letters or more),
; in the name too, by the same rule ("attack the black orc" and only a small orc there: none, 2026-11-04; "the
; sleeping rat", a word not known, passed over).  Its resolver's, first words', blows', tending's, new things'.
; ****************************************************************************

            .setcpu     "65C02"
            .macpack    longbranch
            .export     _name_is
            .import     _cl, popax

            .include    "zeropage.inc"

            .rodata
quals:      .byte       "black brown red white small great cave giant dwarf kobold iron brass broken", 0

            .code

yes:        lda         #1
            ldx         #0
            rts
nope:       lda         #0
            tax
            rts

; unsigned char __fastcall__ name_is (const char* k, const char* w): 1 named, 0 not.  ptr1 the name, ptr2 the word,
; tmp2 its length; tmp1 how many of its letters word_of asks; ptr3 word_of's place
_name_is:
            sta         ptr2
            stx         ptr2+1
            jsr         popax
            sta         ptr1
            stx         ptr1+1
            ldy         #$FF
@len:       iny
            lda         (ptr2),y
            bne         @len
            sty         tmp2
            sty         tmp1
            tya
            jeq         yes                 ; ("": any)
            jsr         from_k              ; (A word of it, or its start)
            bcs         said
            ldy         tmp2
            cpy         #3
            jcc         nope
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
            jcc         nope
            ldy         #$FF
@klen:      iny
            lda         (ptr1),y
            bne         @klen
            cpy         tmp2
            jcc         nope
            jeq         nope                ; (The name no longer than the word)
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

said:       lda         _cl                 ; (Named: w found in the line, the word before it, if it's a kind's other,
            sta         ptr3                ;   in the name too)
            lda         _cl+1
            sta         ptr3+1
@find:      ldy         #0
@fc:        lda         (ptr2),y
            beq         @found
            cmp         (ptr3),y
            bne         @next
            iny
            bra         @fc
@next:      lda         (ptr3)
            jeq         yes                 ; (Not in the line)
            inc         ptr3
            bne         @find
            inc         ptr3+1
            bra         @find
@found:     lda         ptr3                ; (Its place in the line: none before it, or past 255, passed over)
            sec
            sbc         _cl
            tay
            lda         ptr3+1
            sbc         _cl+1
            jne         yes
            tya
            jeq         yes
            lda         _cl
            sta         ptr3
            lda         _cl+1
            sta         ptr3+1
            ldx         #0
            dey                             ; (The space before it)
@back:      tya
            beq         @word
            dey
            lda         (ptr3),y
            cmp         #' '
            beq         @sp
            inx
            bra         @back
@sp:        iny
@word:      cpx         #3                  ; (The word before it: Y its place, X its length)
            jcc         yes
            stx         tmp1
            tya
            clc
            adc         ptr3
            sta         ptr2
            lda         ptr3+1
            adc         #0
            sta         ptr2+1
            lda         #<quals
            sta         ptr3
            lda         #>quals
            sta         ptr3+1
            jsr         word_of
            jcc         yes                 ; (Not a kind's other: "the", "sleeping")
            jsr         from_k
            jcs         yes
            jmp         nope

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
