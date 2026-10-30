; ****************************************************************************
; dice.inc - (the referees': rshouse.h's RS_HOUSE_DICE, a referee's .s includes it) the dice by who asks: each
; requester's own stream of dice.h's generator (xorshift.s's xs_seed), the world's slot's (0: the game's own, as the
; referee began), the UI's (1) and the denizens' (2: DSLOT), swapped in as each answer begins, so a game's dice go by
; its seed and its commands, not by the order the tasks' requests reach the referee.  In assembly, and in an overlay if
; the referee says (DICE_OVL = 1 and .define DICE_SEG "OVnCODE" before including it: its entry dice_entry, its stub the referee's): the main RAM's room
;
;   void __fastcall__ dice_to (unsigned char k);    stream k in xs_seed, the one there kept; k $FF: the streams begun
;                                                   from the generator's state as it is now, stream 0 in

            .import     _xs_seed

            .bss
dice_of:    .res        4 * 3               ; (Each stream's state, as it was left)
dice_at:    .res        1                   ; (The stream in xs_seed now)

.ifdef DICE_OVL
            .segment    DICE_SEG
dice_entry:
.else
            .export     _dice_to
            .code
_dice_to:
.endif
            cmp         #$FF
            bne         @to
            ldx         #7                  ; Begun: streams 1 and 2 the state, their high bytes changed ("UI", "DN")
:           txa
            and         #3
            tay
            lda         _xs_seed,y
            eor         dice_mix,x
            sta         dice_of + 4,x
            dex
            bpl         :-
            stz         dice_at
            rts

@to:        cmp         dice_at
            beq         @done
            pha
            lda         dice_at             ; (The one there swapped out, then stream k in: each an exchange)
            jsr         @exch
            pla
            sta         dice_at
@exch:      asl         a
            asl         a
            tax
            ldy         #0
:           lda         _xs_seed,y
            pha
            lda         dice_of,x
            sta         _xs_seed,y
            pla
            sta         dice_of,x
            inx
            iny
            cpy         #4
            bne         :-
@done:      rts

dice_mix:   .byte       0, 0, $49, $55, 0, 0, $5A, $44
