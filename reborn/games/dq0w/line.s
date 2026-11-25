; ****************************************************************************
; line.s - (dq0w) one line between two squares, line.inc's: the game's library's (the paged ROM's), its stubs here,
; the line's in part missiles (its line of fire's, wshoot.c's lof: no other part draws one), the rest in the main RAM

            .segment    "SW_missiles_CODE"
            .include    "line.inc"

; ln_seen: the same line, part take_rule's own (a denizen's take, what it sees: wtake.c's, 2026-11-17; a part's code calls
; no other part's but through a stub, and the line's stub is part missiles's): line ()'s arguments, its answer; ln_seen94,
; part hide_self's (an actor's search, the squares about it it sees: wsearch.c's, 2026-11-21)
.macro      LNSEEN      seg, name
            .segment    seg
            .export     name
name:
            sta         LZ_MODE
            ldy         #8
            lda         (sp),y
            sta         LZ_SN
            dey
:
            lda         (sp),y
            sta         LZ_XY,y
            dey
            bpl         :-
            ldy         #9
            jsr         addysp
            lda         #LN_LINE
            jmp         dq0x
.endmacro
            LNSEEN      "SW_take_rule_CODE", _ln_seen
            LNSEEN      "SW_hide_self_CODE", _ln_seen94
