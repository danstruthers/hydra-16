; ****************************************************************************
; line.s - (dq0w) one line between two squares, line.inc's: the game's library's (the paged ROM's), its stubs here,
; the line's in part 65 (its line of fire's, wshoot.c's lof: no other part draws one), the rest in the main RAM

            .segment    "SW65CODE"
            .include    "line.inc"

; ln_seen: the same line, part 81's own (a denizen's take, what it sees: wtake.c's, 2026-11-17; a part's code calls
; no other part's but through a stub, and the line's stub is part 65's): line ()'s arguments, its answer
            .segment    "SW81CODE"
            .export     _ln_seen
_ln_seen:
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
