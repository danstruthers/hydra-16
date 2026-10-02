; ****************************************************************************
; t_child - a program the tests start, to do one thing, named by its arguments: a letter, then a hex byte (hh):
;   "e" c       end with code c (a character) and its arguments as the message
;   "s" hh      spin (no yields) for hh ticks, then end with code 0
;   "y" hh      yield hh times, then end with code 0
;   "k"         end with code = the tick count's low byte when it started
;   "p"         pause until woken (WAKE), then end with code "p"
;   "o"         start "t_child e9" and end at once with code "o" (leaving an orphan)
;   "c" hh      call task F's serve entry (the test driver, t_drv): op 3, sleep hh ticks; end with its .A
;   anything else: end with code $EE

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "t_child", main

.zeropage
args:       .res        2
param:      .res        1
start:      .res        2

.code
main:
            MOVR        args, r0
            ldy         #1                                  ; The hex byte after the letter, if any
            jsr         hex
            sta         param
            lda         (args)
            cmp         #'e'
            beq         @e
            cmp         #'s'
            beq         @s
            cmp         #'y'
            beq         @y
            cmp         #'k'
            beq         @k
            cmp         #'p'
            beq         @p
            cmp         #'o'
            beq         @o
            cmp         #'c'
            beq         @c
            lda         #$EE
            bra         @end

@e:
            MOVR        r0, args
            ldy         #1
            lda         (args),Y
            jmp         EXITS

@s:
            jsr         TICKS
            sta         start
@spin:
            jsr         TICKS
            sec
            sbc         start
            cmp         param
            bcc         @spin
            lda         #0
            bra         @end

@y:
            lda         param
            beq         @end
            jsr         YIELD
            dec         param
            bra         @y

@k:
            jsr         TICKS
            bra         @end

@p:
            jsr         PAUSE
            lda         #'p'
            bra         @end

@o:
            LDR         r0, s_child
            LDR         r1, s_e9
            lda         #0
            jsr         SPAWN
            lda         #'o'
            bra         @end

@c:
            lda         #3
            ldx         param
            ldy         #$0F
            jsr         DBG_SCALL
@end:
            stz         r0
            stz         r0 + 1
            jmp         EXITS

; .A = the hex byte at (args),Y (two digits), or 0
hex:
            jsr         @digit
            bcs         @none
            asl
            asl
            asl
            asl
            sta         param
            iny
            jsr         @digit
            bcs         @none
            ora         param
            rts

@none:
            lda         #0
            rts

@digit:                                                     ; C = 0 and .A = a digit's value, or C = 1
            lda         (args),Y
            sec
            sbc         #'0'
            cmp         #10
            bcc         @ok
            sbc         #'a' - '0' - 10                     ; (Lower case, C = 1)
            cmp         #10
            bcc         @bad
            cmp         #16
            bcs         @bad
@ok:
            clc
            rts

@bad:
            sec
            rts

.rodata
s_child:    .byte       "#m/t_child", 0
s_e9:       .byte       "e9", 0
