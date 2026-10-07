; ****************************************************************************
; chrget.inc - CHRGET and CHRGOT: the text's next character (and the one at TXTPTR), spaces passed over.  Microsoft's
; ran in the zero page, its own code holding TXTPTR (lda abs, changed as it went); here it's in ROM and reads through
; TXTPTR in the zero page (a cycle more a character).  OUT: .A = the character; C = 0 a digit; Z = 1 its end
; (0) or ':'

.segment "CODE"
CHRGET:
        inc     TXTPTR
        bne     CHRGOT
        inc     TXTPTR+1
CHRGOT:
        lda     (TXTPTR)
        cmp     #$3A
        bcs     L4058
CHRGOT2:
        cmp     #$20
        beq     CHRGET
        sec
        sbc     #$30
        sec
        sbc     #$D0
L4058:
        rts
