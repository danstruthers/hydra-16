; ****************************************************************************
; t_kcopy - copying between tasks (phase 1, spike S2), run as init: 4K to the kernel task's RAM and back (the
; marks "<kc" and "kc>" around the 4096 bytes out: sim/test.js divides), odd lengths (no byte past the end), a
; buffer in a RAM bank, and the error for no task.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_kcopy", main

THERE           = $4000                                     ; The kernel task's RAM: nothing there
BIG             = 4096

.zeropage
p:          .res        2
len:        .res        2
seed:       .res        1
k:          .res        1

.bss
bufa:       .res        BIG + 16
bufb:       .res        BIG + 256

.code

; DBG_KCOPY len bytes between here and there in the kernel task; dir: clc (out) or sec (in)
.macro KC       here, there, dir
            LDR         r0, here
            LDR         r1, there
            MOVR        r2, len
            lda         #0
            dir
            jsr         DBG_KCOPY
.endmacro

main:
            stz         T_FAILS

; ---- 4K out (timed) and back
            LDR         len, BIG
            lda         #1
            sta         seed
            LDR         p, bufa
            jsr         fill
            jsr         clearb
            MARK        "<kc"
            KC          bufa, THERE, clc
            php
            MARK        "kc>"
            jsr         t_crlf
            plp
            EXPECT_OK   "4096 bytes to the kernel task"
            KC          bufb, THERE, sec
            EXPECT_OK   "and back"
            jsr         same
            EXPECT_A    0, "4096 bytes there and back, the same"

; ---- Odd lengths: each the same, and nothing written past its end
            stz         k
@len:
            ldx         k
            lda         lens_lo,X
            sta         len
            sta         seed
            lda         lens_hi,X
            sta         len + 1
            LDR         p, bufa
            jsr         fill
            jsr         clearb
            KC          bufa, THERE + $1000, clc
            KC          bufb, THERE + $1000, sec
            jsr         same
            bne         @bad
            jsr         past                                ; (bufb's byte after the copy: still $EE?)
            cmp         #$EE
            bne         @bad
            inc         k
            lda         k
            cmp         #LENS
            bne         @len
            OK          "lengths 1, 7, 8, 9, 255, 256, 257, 1000: the same, and nothing past the end"
            bra         @bank

@bad:
            lda         k
            NOTOK       "an odd length (its index)"

; ---- A buffer in a RAM bank (this task's bank 1, at $8000)
@bank:
            lda         #1
            sta         $00
            LDR         len, 256
            lda         #$77
            sta         seed
            LDR         p, $8000
            jsr         fill
            KC          $8000, THERE + $2000, clc
            stz         $00
            jsr         clearb
            KC          bufb, THERE + $2000, sec
            LDR         p, bufa                             ; (The same pattern, here, to compare with)
            jsr         fill
            jsr         same
            EXPECT_A    0, "a buffer in a RAM bank"

; ---- No such task
            LDR         len, 1
            LDR         r0, bufa
            LDR         r1, THERE
            MOVR        r2, len
            lda         #16
            clc
            jsr         DBG_KCOPY
            EXPECT_ERR  E_SRCH, "kcopy with task 16: E_SRCH"

            DONE        "t_kcopy"

; len bytes of a pattern at p (from seed)
fill:
            MOVR        r3, p
            MOVR        r4, len
            ldy         #0
            ldx         seed
@byte:
            lda         r4
            ora         r4 + 1
            beq         @done
            txa
            sta         (r3)
            clc
            adc         #37
            tax
            inc         r3
            bne         :+
            inc         r3 + 1
:
            lda         r4
            bne         :+
            dec         r4 + 1
:
            dec         r4
            bra         @byte

@done:
            rts

; bufb: all $EE
clearb:
            LDR         r3, bufb
            ldx         #>(BIG + 256)
            lda         #$EE
            ldy         #0
:
            sta         (r3),Y
            iny
            bne         :-
            inc         r3 + 1
            dex
            bne         :-
            rts

; .A = 0 (and Z) if len bytes of bufa and bufb are the same
same:
            LDR         r3, bufa
            LDR         r4, bufb
            MOVR        r5, len
@byte:
            lda         r5
            ora         r5 + 1
            beq         @done
            lda         (r3)
            cmp         (r4)
            bne         @differ
            inc         r3
            bne         :+
            inc         r3 + 1
:
            inc         r4
            bne         :+
            inc         r4 + 1
:
            lda         r5
            bne         :+
            dec         r5 + 1
:
            dec         r5
            bra         @byte

@done:
            lda         #0
            rts

@differ:
            lda         #1
            rts

; .A = bufb's byte at len
past:
            clc
            lda         #<bufb
            adc         len
            sta         r3
            lda         #>bufb
            adc         len + 1
            sta         r3 + 1
            lda         (r3)
            rts

.rodata
LENS            = 8
lens_lo:    .byte       <1, <7, <8, <9, <255, <256, <257, <1000
lens_hi:    .byte       >1, >7, >8, >9, >255, >256, >257, >1000
