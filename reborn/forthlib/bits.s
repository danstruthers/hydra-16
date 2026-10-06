; ****************************************************************************
; bits.s - HyForth's bit words, the old HyForth's (/lib/forth/bits.fl: lib bits): tbit, sbit and cbit, a cell's bit
; 0-15 tested, set or cleared (only b's low 4 bits count).  Its shifts and flags are the standard's: lshift, rshift,
; 0<> (the old <<, >> and bool).

.include "forthlib.inc"

            HEADER      "tbit", 0
tbit:                                                       ; ( n b -- n flag ): bit b of n set?  n kept
            jsr         bit_mask
            dex
            lda         dlo + 1,x
            and         tmp
            bne         :+
            lda         dhi + 1,x
            and         tmp + 1
            bne         :+
            jmp         zero_tos
:
            jmp         true_tos

            HEADER      "sbit", 0
sbit:                                                       ; ( n b -- n' ): n with bit b set
            jsr         bit_mask
            lda         dlo,x
            ora         tmp
            sta         dlo,x
            lda         dhi,x
            ora         tmp + 1
            sta         dhi,x
            rts

            HEADER      "cbit", 0
cbit:                                                       ; ( n b -- n' ): n with bit b clear
            jsr         bit_mask
            lda         tmp
            eor         #$FF
            and         dlo,x
            sta         dlo,x
            lda         tmp + 1
            eor         #$FF
            and         dhi,x
            sta         dhi,x
            rts

; tmp = the mask of bit b, the top (dropped)
bit_mask:
            lda         dlo,x
            inx
            and         #$0F
            stz         tmp
            stz         tmp + 1
            cmp         #8
            bcs         :+
            tay
            lda         bit_of,y
            sta         tmp
            rts
:
            and         #7
            tay
            lda         bit_of,y
            sta         tmp + 1
            rts

bit_of:     .byte       $01, $02, $04, $08, $10, $20, $40, $80
