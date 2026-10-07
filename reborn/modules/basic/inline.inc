.segment "CODE"

L2420:
        dex
        bpl     INLIN2
L2423:
        jsr     CRDO

; ----------------------------------------------------------------------------
; READ A LINE, AND STRIP OFF SIGN BITS
; ----------------------------------------------------------------------------

INLIN:
        ldx     #$00
INLIN2:
        jsr     GETLN
        cmp     #$07
        beq     L2443
        cmp     #$0D
        beq     L2453
        cmp     #$20
        bcc     INLIN2
        cmp     #$7D
        bcs     INLIN2
        cmp     #$40 ; @
        beq     L2423
        cmp     #$5F ; _
        beq     L2420
L2443:
        cpx     #$47
        bcs     L244C
        sta     INPUTBUFFER,x
        inx
        bne     INLIN2
L244C:
        lda     #$07 ; BEL
L244E:
        jsr     OUTDO
        bne     INLIN2
L2453:
        jmp     L29B9
GETLN:
        jsr     MONRDKEY
        cmp     #$0F
        bne     L2465
        pha
        lda     Z14
        eor     #$FF
        sta     Z14
        pla
L2465:
        rts
