; ****************************************************************************
; t_drv - a test driver (started at boot: task E, the driver after kdev), for t_scall.  Its serve entry takes an op
; in .A and an argument in .X:
;   0   .A = .X + 1                         (the lean round trip: spike S3)
;   1   .A = the caller (.Y)
;   2   fail: C = 1, .A = E_INVAL
;   3   sleep .X ticks, then .A = 0         (busy meanwhile: other callers wait)
;   4   .A/.X = the calls it has served
;   5   .A = $5A if its init ran
;   else: C = 1, .A = E_NOSYS

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_DRIVER "t_drv", init, serve, 0, 0, HF_BOOT

.zeropage
calls:      .res        2

.bss
started:    .res        1

.code
init:
            lda         #$5A
            sta         started
            stz         calls
            stz         calls + 1
            clc
            rts

serve:
            inc         calls
            bne         :+
            inc         calls + 1
:
            cmp         #0
            bne         @1
            inx
            txa
            clc
            rts

@1:
            cmp         #1
            bne         @2
            tya
            clc
            rts

@2:
            cmp         #2
            bne         @3
            lda         #E_INVAL
            sec
            rts

@3:
            cmp         #3
            bne         @4
            txa
            ldx         #0
            jsr         SLEEP
            lda         #0
            clc
            rts

@4:
            cmp         #4
            bne         @5
            lda         calls
            ldx         calls + 1
            clc
            rts

@5:
            cmp         #5
            bne         @nosys
            lda         started
            clc
            rts

@nosys:
            lda         #E_NOSYS
            sec
            rts
