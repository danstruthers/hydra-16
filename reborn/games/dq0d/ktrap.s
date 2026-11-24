; ****************************************************************************
; ktrap.s - (dq0d) a trap the walker knows of underfoot, wet_ok's (dq0d.c: its ways go round it): on chunk C's square
; cx_, cy_ (at ()'s, found already), a set trap, not a door's, in plain sight (its hide 0) or known to those of knowers'
; bit kb (world.h's TRAP_KNOWN, TRAP_KB: the user's, 2026-11-23, each faction knows its own traps, another's once
; found).  In assembly for the main RAM's room: cc65's loop was some 200 bytes; dq0w/hz.s's trapsq its pattern.
;
;   unsigned char __fastcall__ known_trap (unsigned char kb);   1 if there is one, else 0 (and Z with it)

            .export     _known_trap
            .import     _C, _cx_, _cy_
            .importzp   ptr1, tmp1
            .include    "dq0hub_hz.inc"         ; (world.h's chunk, trap: the build's, ../dq0/hubgen.js)
.assert     CH_TRAP = CH_NTRAPS + 1, error, "ktrap.s: chunk's trap[] not after its ntraps"

            .code
_known_trap:
            sta         tmp1
            clc
            lda         _C
            adc         #<CH_NTRAPS
            sta         ptr1
            lda         _C + 1
            adc         #>CH_NTRAPS
            sta         ptr1 + 1
            lda         (ptr1)
            beq         @none
            tax
            inc         ptr1                ; (The first: trap[], after ntraps)
            bne         @loop
            inc         ptr1 + 1
@loop:      ldy         #TR_X
            lda         (ptr1),y
            cmp         _cx_
            bne         @next
            ldy         #TR_Y
            lda         (ptr1),y
            cmp         _cy_
            bne         @next
            ldy         #TR_STATE
            lda         (ptr1),y
            and         #TS_SPRUNG | TS_DISARMED | TS_BROKEN
            bne         @next
            ldy         #TR_TRIGGER
            lda         (ptr1),y
            cmp         #TT_DOOR
            beq         @next
            ldy         #TR_HIDE
            lda         (ptr1),y
            beq         @one                ; (In plain sight: known to all)
            ldy         #TR_KNOW
            lda         (ptr1),y
            and         tmp1
            bne         @one
@next:      lda         ptr1
            clc
            adc         #TR_SIZE
            sta         ptr1
            bcc         :+
            inc         ptr1 + 1
:           dex
            bne         @loop
@none:      lda         #0
            tax
            rts
@one:       ldx         #0
            lda         #1                  ; (Z clear with it)
            rts
