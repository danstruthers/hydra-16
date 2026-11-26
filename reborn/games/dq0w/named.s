; ****************************************************************************
; named.s - (dq0w) a name said, the one rule (COMMANDS-CRITIQUE.md's 5.1 step F, 2026-11-07: things.c's named, in C
; till even4's, 2026-11-26): a word of the kind's ("key" an iron key; "sword" a broken one too), or its start; or that
; word made plural ("sacks", "torches": "drop two sacks"); or the end of the name, a word longer than 3 ("pack" a
; backpack, "skin" a waterskin, 2026-10-26).  "" names any.  In assembly: the chooser asks it of every kind in the
; store as a thing's named (things.c's pick_), some 1,100 cycles a kind in C (dq0letwhich's, even4's).  The chooser's,
; the light's, the market's, the search's; the UI's main.c's name_is the same rule.  Both names under 256 bytes.
;
;   unsigned char __fastcall__ named (const char* k, const char* w);     1 if w names a thing called k, else 0
; ****************************************************************************

            .setcpu     "65C02"
            .export     _named
            .import     incsp2
            .importzp   sp, ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

            .code

; named (k, w): w in .A .X, k on the C stack
_named:     sta         ptr2                ; (w)
            stx         ptr2 + 1
            ldy         #1
            lda         (sp),y
            sta         ptr1 + 1            ; (k)
            dey
            lda         (sp),y
            sta         ptr1
            jsr         incsp2
            ldy         #0                  ; Its length, n
@len:       lda         (ptr2),y
            beq         @lend
            iny
            bne         @len
@lend:      sty         tmp4
            sty         tmp1
            jsr         word_of             ; A word of the kind's, or its start
            bcs         @yes
            ldy         tmp4
            cpy         #3
            bcc         @end                ; (n > 2: made plural)
            dey
            lda         (ptr2),y
            cmp         #'s'
            bne         @end
            sty         tmp1                ; ("sacks": the word, n - 1)
            jsr         word_of
            bcs         @yes
            ldy         tmp1
            dey
            lda         (ptr2),y
            cmp         #'e'
            bne         @end
            sty         tmp1                ; ("torches": n - 2)
            jsr         word_of
            bcs         @yes
@end:       lda         tmp4                ; The end of the name, a word longer than 3
            cmp         #4
            bcc         @no
            ldy         #0
@klen:      lda         (ptr1),y            ; (Its length, m)
            beq         @kl
            iny
            bne         @klen
@kl:        cpy         tmp4
            beq         @no
            bcc         @no                 ; (m > n)
            tya
            sec
            sbc         tmp4
            sta         tmp2                ; (Its last n: from m - n)
            ldx         #0
@ec:        cpx         tmp4
            bcs         @yes
            txa
            clc
            adc         tmp2
            tay
            lda         (ptr1),y
            stx         tmp3
            ldy         tmp3
            cmp         (ptr2),y
            bne         @no
            inx
            bne         @ec
@no:        lda         #0
            tax
            rts
@yes:       lda         #1
            ldx         #0
            rts

; A word of k (ptr1), or its start, the first tmp1 letters of w (ptr2): carry set if one is (0 letters: any)
word_of:    stz         tmp2                ; (The word's start in k)
@word:      lda         tmp1
            beq         @match
            ldy         tmp2
            lda         (ptr1),y
            ldy         #0
            cmp         (ptr2),y
            bne         @next
            ldx         #1
@cmp:       cpx         tmp1
            bcs         @match
            txa
            clc
            adc         tmp2
            tay
            lda         (ptr1),y
            stx         tmp3
            ldy         tmp3
            cmp         (ptr2),y
            bne         @next
            inx
            bne         @cmp
@next:      ldy         tmp2                ; (The next word: past the next space, none past the name's end)
@scan:      lda         (ptr1),y
            beq         @none
            iny
            cmp         #' '
            bne         @scan
            sty         tmp2
            bra         @word
@none:      clc
            rts
@match:     sec
            rts
