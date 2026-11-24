; ****************************************************************************
; hz.s - (dq0w) body bd's step, its hazards (wbody.h's den_hz: the one path's step, every actor's, step1's, 2026-11-23):
; the cheap test, the main RAM's, before part step (wstep.c's den_trap) or part time_drift (time_kept, the pools') is swapped
; in: its way d, before it, the pit it's in (a round's climb out) or a set trap on the square ahead (noticed: 1, it
; goes no further); DT_ON, on its square after, a set trap there set off, lamp oil slipped in (1: dead of it); a winged
; body none of them (the user's, 2026-11-23: it passes over a floor's traps, pits and spilt oil).  In C in
; dq0w.c till room7 (2026-11-23): some 370 bytes of cc65's, the world's main RAM's room; the same tests, the same calls.
;
;   unsigned char __fastcall__ den_hz (unsigned char d);

            .export     _den_hz
            .import     _at, _C, _cx_, _cy_, _bd, _npools, _head_x, _head_y, _den_trap, _time_kept
            .import     pusha, pushax
            .importzp   ptr1
            .include    "dq0hub_hz.inc"         ; (world.h's chunk, trap; hub.h's actor: the build's, ../dq0/hubgen.js)

DT_ON       = $FF                           ; (wbody.h's: dq0w.c's typedef hz_same holds them to it)
DT_NOTICE   = 0
DT_SPRING   = 1
DT_PIT      = 2
POOL_OP     = $FFF0
PC_WOKE     = $80
.assert     CH_TRAP = CH_NTRAPS + 1, error, "hz.s: chunk's trap[] not after its ntraps"

            .bss
hzk:        .res        1                   ; (d)
hdx:        .res        1                   ; (The square's offset from bd's, head_x's, head_y's)
hdy:        .res        1

            .code
_den_hz:
            sta         hzk
            stz         hdx
            stz         hdy
            ldx         _bd + A_BODY        ; (A winged body over a floor's traps, its pits and spilt oil, none of
            cpx         #BD_WINGS           ;   them its: the user's, 2026-11-23; a door's trap still its, wdoor.c's)
            bne         :+
            cmp         #DT_ON
            beq         @ret
            bra         none
:           cmp         #DT_ON
            bne         @step
            jsr         atbd                ; (On its square: a set trap there set off, part step's)
            jsr         trapsq
            beq         @pool
            lda         #DT_SPRING
            jsr         pusha
            lda         hzk
            jsr         _den_trap
@pool:      lda         _bd + A_STATE       ; (Lamp oil there: slipped in, the user's H15, every actor's)
            beq         @ret
            lda         _npools
            and         #<~PC_WOKE
            beq         @ret
            lda         #<(POOL_OP + 1)
            ldx         #>(POOL_OP + 1)
            jsr         _time_kept
@ret:       ldx         #0                  ; (!bd.state)
            lda         _bd + A_STATE
            beq         one
            txa
            rts
@step:      jsr         atbd                ; (In a pit: a round's climb out)
            sta         ptr1
            stx         ptr1 + 1
            lda         (ptr1)
            and         #$0F
            cmp         #T_PIT
            bne         @ahead
            lda         #DT_PIT
            jsr         pusha
            lda         hzk
            jmp         _den_trap
@ahead:     ldy         hzk                 ; (A set trap on the square ahead: noticed, or not, part step's)
            lda         _head_x,y
            sta         hdx
            lda         _head_y,y
            sta         hdy
            jsr         atbd
            stx         ptr1
            ora         ptr1
            beq         none
            jsr         trapsq
            beq         none
            lda         #DT_NOTICE
            jsr         pusha
            lda         hzk
            jsr         _den_trap
            tax
            bne         one
none:       lda         #0
            tax
            rts
one:        ldx         #0
            lda         #1
            rts

; at (bd.sn, bd.x + hdx, bd.y + hdy) (hdx, hdy signed)
atbd:
            lda         _bd + A_SN
            jsr         pusha
            lda         hdx
            ldy         #A_X
            jsr         addw
            jsr         pushax
            lda         hdy
            ldy         #A_Y
            jsr         addw
            jmp         _at

; bd's word at .Y, plus .A (signed): .A, .X
addw:       ldx         #0
            cmp         #$80
            bcc         :+
            dex
:           clc
            adc         _bd,y
            pha
            txa
            adc         _bd + 1,y
            tax
            pla
            rts

; 1 if C's square cx_, cy_ has a set trap, set off underfoot (not a door's); else 0 (.A, and Z with it)
trapsq:
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
            ldx         #0
            lda         #1                  ; (Z clear with it: trapsq's callers branch on it)
            rts
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
