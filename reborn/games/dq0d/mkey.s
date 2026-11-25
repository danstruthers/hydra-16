; ****************************************************************************
; mkey.s - (dq0d) the minds' keyed dice (phase C, dice2, 2026-11-24): each decision's dice a function of its key
; alone, so no other denizen's, and no roll elsewhere, moves them.  The key: the game's seed and its dice offset
; (mbase, as the minds begin: dq0d.c's), the denizen's number (the hub's actor's did; dq0d.c's MK_* for the minds' own
; work, no one's), the moment it decides at (its ready, not when the minds come to it), and how many times in a row
; that one key's come (mn).  The generator's state (xorshift.s's xs_seed) the moment's xor the base, the number added
; to its low half and mn to its third byte, then three of the generator's steps, its high half added to its low after
; each: the sums' carries make it not linear (xorshift's own step is: keys a fixed mask apart would roll dice a fixed
; mask apart).  The dice after it the generator's (dice.h's random), as before.  In assembly: the main RAM's room
; (cc65's sums of it some 270 bytes)
;
;   void __fastcall__ mkey (unsigned id, unsigned long t);
;   extern unsigned long mbase;

            .export     _mkey, _mbase
            .import     _xs_seed, _rand16, popax
            .importzp   sreg, ptr1, ptr2

            .bss
_mbase:     .res        4
mlast:      .res        4                   ; (The last key: its moment's low half, its number)
mn:         .res        1

            .code

_mkey:      sta         ptr1                ; t (.A .X sreg): xor the base into the state
            stx         ptr1 + 1
            eor         _mbase
            sta         _xs_seed
            txa
            eor         _mbase + 1
            sta         _xs_seed + 1
            lda         sreg
            eor         _mbase + 2
            sta         _xs_seed + 2
            lda         sreg + 1
            eor         _mbase + 3
            sta         _xs_seed + 3
            jsr         popax               ; id (ptr2): the same key as the last, mn one more; else 0
            sta         ptr2
            stx         ptr2 + 1
            ldy         #3
:           lda         ptr1,y
            cmp         mlast,y
            bne         @new
            dey
            bpl         :-
            inc         mn
            bra         @add
@new:       ldy         #3
:           lda         ptr1,y
            sta         mlast,y
            dey
            bpl         :-
            stz         mn
@add:       clc                             ; id into the low half, mn into the third byte
            lda         ptr2
            adc         _xs_seed
            sta         _xs_seed
            lda         ptr2 + 1
            adc         _xs_seed + 1
            sta         _xs_seed + 1
            lda         mn
            eor         _xs_seed + 2
            sta         _xs_seed + 2
            ldy         #3                  ; Three steps, the high half added to the low after each
@step:      phy
            jsr         _rand16
            clc
            lda         _xs_seed
            adc         _xs_seed + 2
            sta         _xs_seed
            lda         _xs_seed + 1
            adc         _xs_seed + 3
            sta         _xs_seed + 1
            ply
            dey
            bne         @step
            rts
