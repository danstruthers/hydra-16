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
    lda #>COPYMAIN
    sta mainoff+1
    lda #<COPYMAIN
    sta mainoff
    ;
    lda #<COPYENDS
    sta endsoff
    lda #>COPYENDS
    sta endsoff+1
    ;
    lda #>RAMST
    sta ramstart+1
    PRINT_BYTE
    lda #<RAMST
    sta ramstart
    PRINT_BYTE
    jsr MEMCPY
    lda ramstart+1
    PRINT_BYTE
    lda ramstart
    PRINT_BYTE
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

