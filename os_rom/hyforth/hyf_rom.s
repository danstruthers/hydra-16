;
;  COPYTORAM
;
COPYMAIN = COPYSTART
RAMST = RAMSTART
COPYENDS = ends - RAMSTART + COPYSTART

COPYTORAM:                     ; copies from mainoff thru endsoff to ramstart
    WCRLF_np
    WCRLF_np
    stz supprint               ; let it print
    STORE_LABEL COPYMAIN, mainoff
    STORE_LABEL COPYENDS, endsoff
    STORE_LABEL RAMST, ramstart
    PRINT_ADDR  ramstart
    jsr MEMCPY
    PRINT_ADDR  ramstart
    WCRLF_np
    jsr MEMCPY
    rts
;;
;
;     scan for beginning / end of code to copy
;      WRT, not even sure best way to proceed (5/9/26)
;;
;CPSCANEND:
;    lda #>ROMSTART
;    sta mainoff
;    sta endsoff
;    lda #<ROMSTART
;    sta mainoff+1
;    sta endsoff+1
;    ldy #0
;CPSCANLOOP:
;    lda (mainoff),y
;    cmp REMARKER,y
;    beq CPSC_HIT
;    lda mainoff
;    sta endsoff
;    lda mainoff+1
;    sta endsoff+1
;    ldy #0
;    inc mainoff
;    bne CPSC_SKIP
;    inc mainoff+1
;CPSC_SKIP:
;    lda mainoff+1
;    cmp #>something    ; bounds limit on scan
;    bcs CPSC_ERR
;    sta endsoff+1
;
;    bra CPSCANLOOP
;
;CPSC_HIT:
;    lda REMARKER,y
;    beq CPSC_FOUND
;    iny
;    bra CPSCANLOOP
;
;CPSC_FOUND:
;    clc
;    bra CPSC_END
;CPSC_ERR:
;    sec
;CPSC_END:
;    rts
;
;

