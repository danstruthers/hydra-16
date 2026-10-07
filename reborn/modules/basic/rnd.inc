.segment "CODE"

; ----------------------------------------------------------------------------
; "RND" FUNCTION
; ----------------------------------------------------------------------------
CONRND1:
        .byte   $98,$35,$44,$7A,$6B
CONRND2:
        .byte   $68,$28,$B1,$46,$C7
RND:
        jsr     SIGN
        tax
        bmi     L3F01
        lda     #<RNDSEED
        ldy     #>RNDSEED
        jsr     LOAD_FAC_FROM_YA
        txa
        beq     RTS19
        lda     #<CONRND1
        ldy     #>CONRND1
        jsr     FMULT
        lda     #<CONRND2
        ldy     #>CONRND2
        jsr     FADD
L3F01:
        ldx     FAC_LAST
        lda     FAC+1
        sta     FAC_LAST
        stx     FAC+1
        lda     #$00
        sta     FACSIGN
        lda     FAC
        sta     FACEXTENSION
        lda     #$80
        sta     FAC
        jsr     NORMALIZE_FAC2
        ldx     #<RNDSEED
        ldy     #>RNDSEED
GOMOVMF:
        jmp     STORE_FAC_AT_YX_ROUNDED

; ----------------------------------------------------------------------------
; INITIAL VALUE FOR RANDOM NUMBER (copied to RNDSEED as BASIC starts: all five
; bytes, where Microsoft's copy with CHRGET missed the last)
; ----------------------------------------------------------------------------
GENERIC_RNDSEED:
        .byte   $80,$4F,$C7,$52,$58
