.segment "CODE"

PRSTRING:
        jsr     STRPRT
L297E:
        jsr     CHRGOT

; ----------------------------------------------------------------------------
; "PRINT" STATEMENT
; ----------------------------------------------------------------------------

PRINT:
        beq     CRDO
PRINT2:
        beq     L29DD
        cmp     #TOKEN_TAB
        beq     L29F5
        cmp     #TOKEN_SPC
        clc	; also AppleSoft II
        beq     L29F5
        cmp     #','
        beq     L29DE
        cmp     #$3B
        beq     L2A0D
        jsr     FRMEVL
        bit     VALTYP
        bmi     PRSTRING
        jsr     FOUT
        jsr     STRLIT
        ldy     #$00
        lda     (FAC_LAST-1),y
        clc
        adc     POSX
        cmp     Z17
        bcc     L29B1
        jsr     CRDO
L29B1:
        jsr     STRPRT
        jsr     OUTSP
        bne     L297E ; branch always

; A new line: LF (the system's), the column 0 again
CRDO:
        stz     POSX
        lda     #LF
        jsr     OUTDO

PRINTNULLS:
        eor     #$FF
L29DD:
        rts
L29DE:
        lda     POSX
        cmp     Z18
        bcc     L29EA
        jsr     CRDO
        jmp     L2A0D
L29EA:
        sec
L29EB:
        sbc     #$0E
        bcs     L29EB
        eor     #$FF
        adc     #$01
        bne     L2A08
L29F5:
        php
        jsr     GTBYTC
        cmp     #')'
        bne     SYNERR4
        plp
        bcc     L2A09
        txa
        sbc     POSX
        bcc     L2A0D
L2A08:
        tax
L2A09:
        inx
L2A0A:
        dex
        bne     L2A13
L2A0D:
        jsr     CHRGET
        jmp     PRINT2
L2A13:
        jsr     OUTSP
        bne     L2A0A

; ----------------------------------------------------------------------------
; PRINT STRING AT (Y,A)
; ----------------------------------------------------------------------------
STROUT:
        jsr     STRLIT

; ----------------------------------------------------------------------------
; PRINT STRING AT (FACMO,FACLO)
; ----------------------------------------------------------------------------
STRPRT:
        jsr     FREFAC
        tax
        ldy     #$00
        inx
L2A22:
        dex
        beq     L29DD
        lda     (INDEX),y
        jsr     OUTDO
        iny
        cmp     #$0D
        bne     L2A22
        jsr     PRINTNULLS
        jmp     L2A22
; ----------------------------------------------------------------------------
OUTSP:
        lda     #$20
        .byte   $2C
OUTQUES:
        lda     #$3F

; ----------------------------------------------------------------------------
; PRINT CHAR FROM (A)
; ----------------------------------------------------------------------------
OUTDO:
        bit     Z14
        bmi     L2A56
        cmp     #$20
        bcc     L2A4E
LCA6A:
        inc     POSX
L2A4E:
        jsr     MONCOUT
L2A56:
        and     #$FF
LE8F2:
        rts

; ----------------------------------------------------------------------------
; end of print.s
; ----------------------------------------------------------------------------
