; ****************************************************************************
; table.s - (dq0lib) the world's act table (GAME-CRITIQUE.md's E6, ONE-PATH.md's 2.1, 2026-11-19): its rows, made of
; games/dq0w/acts.txt by acts.js as the build begins (obj/gen/dq0acts.inc), here in the paged ROM, no main RAM; and
; their one reader, AC_ROW.  Each actor's act reads its numbers here, the character's and a denizen's the same row.
;   AC_ROW      a row copied out: IN: r0 = the caller's block (games/dq0w/acts.h's struct actq: its entry's low byte,
;               the act's number, then the row's room, AC_SIZE bytes).  OUT: C = 0, the row in the block; C = 1, no
;               such act (the block as it was).  Modifies .A, .Y, r0, r1, r2

.include "hydra.inc"

            .export     ac_row

.segment "RODATA"
            .include    "dq0acts.inc"

.code
ac_row:
            ldy         #1
            lda         (r0),y                              ; The act's number
            cmp         #AC_N
            bcs         @none
            sta         r1                                  ; r1 = n * 12: n * 4 (r2), n * 8, their sum
            stz         r1 + 1
            asl         r1
            rol         r1 + 1
            asl         r1
            rol         r1 + 1
            lda         r1
            sta         r2
            lda         r1 + 1
            sta         r2 + 1
            asl         r1
            rol         r1 + 1
            clc
            lda         r1
            adc         r2
            sta         r1
            lda         r1 + 1
            adc         r2 + 1
            sta         r1 + 1
            clc                                         ; ... and the table's start
            lda         r1
            adc         #<ac_rows
            sta         r1
            lda         r1 + 1
            adc         #>ac_rows
            sta         r1 + 1
            clc                                         ; The block's row: past its entry and the act's number
            lda         r0
            adc         #2
            sta         r0
            bcc         :+
            inc         r0 + 1
:           ldy         #AC_SIZE - 1
@copy:
            lda         (r1),y
            sta         (r0),y
            dey
            bpl         @copy
            clc
            rts
@none:
            sec
            rts
