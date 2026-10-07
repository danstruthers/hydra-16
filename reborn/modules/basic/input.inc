.segment "CODE"

; ----------------------------------------------------------------------------
; INPUT CONVERSION ERROR:  ILLEGAL CHARACTER
; IN NUMERIC FIELD.  MUST DISTINGUISH
; BETWEEN INPUT, READ, AND GET
; ----------------------------------------------------------------------------
INPUTERR:
        lda     INPUTFLG
        beq     RESPERR	; INPUT
; without this, it treats GET errors
; like READ errors
        bmi     L2A63	; READ
        ldy     #$FF	; GET
        bne     L2A67
L2A63:
        lda     Z8C
        ldy     Z8C+1
L2A67:
        sta     CURLIN
        sty     CURLIN+1
SYNERR4:
        jmp     SYNERR
RESPERR:
        lda     #<ERRREENTRY
        ldy     #>ERRREENTRY
        jsr     STROUT
        lda     OLDTEXT
        ldy     OLDTEXT+1
        sta     TXTPTR
        sty     TXTPTR+1
RTS20:
        rts

; ----------------------------------------------------------------------------
; "GET" STATEMENT
; ----------------------------------------------------------------------------
GET:
        jsr     ERRDIR
        ldx     #<(INPUTBUFFER+1)
        ldy     #>(INPUTBUFFER+1)
        stz     INPUTBUFFER+1                   ; (Its end: the buffer isn't in the zero page, .Y not 0)
        lda     #$40
        jsr     PROCESS_INPUT_LIST
        rts

; ----------------------------------------------------------------------------
; "INPUT" STATEMENT
; ----------------------------------------------------------------------------
INPUT:
        lsr     Z14
        cmp     #$22
        bne     L2A9E
        jsr     STRTXT
        lda     #$3B
        jsr     SYNCHR
        jsr     STRPRT
L2A9E:
        jsr     ERRDIR
        lda     #$2C
        sta     INPUTBUFFER-1
LCAF8:
        jsr     NXIN
        lda     INPUTBUFFER
        bne     L2ABE
        asl     intr                            ; (Ctrl-C while it waited: BREAK, said; an empty line: the
        jmp     CONTROL_C_TYPED                 ;   program's end, Microsoft's way, quietly)

NXIN:
        jsr     OUTQUES	; '?'
        jsr     OUTSP
LCB21:
        jmp     INLIN

; ----------------------------------------------------------------------------
; "READ" STATEMENT
; ----------------------------------------------------------------------------
READ_:
        ldx     DATPTR
        ldy     DATPTR+1
        .byte   $A9	; LDA #$98
L2ABE:
        tya

; ----------------------------------------------------------------------------
; PROCESS INPUT LIST
;
; (Y,X) IS ADDRESS OF INPUT DATA STRING
; (A) = VALUE FOR INPUTFLG:  $00 FOR INPUT
; 				$40 FOR GET
;				$98 FOR READ
; ----------------------------------------------------------------------------
PROCESS_INPUT_LIST:
        sta     INPUTFLG
        stx     INPTR
        sty     INPTR+1
PROCESS_INPUT_ITEM:
        jsr     PTRGET
        sta     FORPNT
        sty     FORPNT+1
        lda     TXTPTR
        ldy     TXTPTR+1
        sta     TXPSV
        sty     TXPSV+1
        ldx     INPTR
        ldy     INPTR+1
        stx     TXTPTR
        sty     TXTPTR+1
        jsr     CHRGOT
        bne     INSTART
        bit     INPUTFLG
        bvc     L2AF0
        jsr     GETKEY                          ; (A key, or 0 for none: hyio.inc)
        sta     INPUTBUFFER
        ldx     #<(INPUTBUFFER-1)
        ldy     #>(INPUTBUFFER-1)
        bra     L2AF8                           ; (Microsoft's beq or bne here hung on where the buffer was)
L2AF0:
        bmi     FINDATA
        jsr     OUTQUES
LCB64:
        jsr     NXIN
L2AF8:
        stx     TXTPTR
        sty     TXTPTR+1

; ----------------------------------------------------------------------------
INSTART:
        jsr     CHRGET
        bit     VALTYP
        bpl     L2B34
        bit     INPUTFLG
        bvc     L2B10
        inx
        stx     TXTPTR
        lda     #$00
        sta     CHARAC
        beq     L2B1C
L2B10:
        sta     CHARAC
        cmp     #$22
        beq     L2B1D
        lda     #$3A
        sta     CHARAC
        lda     #$2C
L2B1C:
        clc
L2B1D:
        sta     ENDCHR
        lda     TXTPTR
        ldy     TXTPTR+1
        adc     #$00
        bcc     L2B28
        iny
L2B28:
        jsr     STRLT2
        jsr     POINT
        jsr     PUTSTR
        jmp     INPUT_MORE
; ----------------------------------------------------------------------------
L2B34:
        jsr     FIN
        lda     VALTYP+1
        jsr     LET2
; ----------------------------------------------------------------------------
INPUT_MORE:
        jsr     CHRGOT
        beq     L2B48
        cmp     #$2C
        beq     L2B48
        jmp     INPUTERR
L2B48:
        lda     TXTPTR
        ldy     TXTPTR+1
        sta     INPTR
        sty     INPTR+1
        lda     TXPSV
        ldy     TXPSV+1
        sta     TXTPTR
        sty     TXTPTR+1
        jsr     CHRGOT
        beq     INPDONE
        jsr     CHKCOM
        jmp     PROCESS_INPUT_ITEM
; ----------------------------------------------------------------------------
FINDATA:
        jsr     DATAN
        iny
        tax
        bne     L2B7C
        ldx     #ERR_NODATA
        iny
        lda     (TXTPTR),y
        beq     GERR
        iny
        lda     (TXTPTR),y
        sta     Z8C
        iny
        lda     (TXTPTR),y
        iny
        sta     Z8C+1
L2B7C:
        lda     (TXTPTR),y
        tax
        jsr     ADDON
        cpx     #$83
        bne     FINDATA
        jmp     INSTART
; ---NO MORE INPUT REQUESTED------
INPDONE:
        lda     INPTR
        ldy     INPTR+1
        ldx     INPUTFLG
        bpl     L2B94; INPUT or GET
        jmp     SETDA
L2B94:
        ldy     #$00
        lda     (INPTR),y
        beq     L2BA1
        lda     #<ERREXTRA
        ldy     #>ERREXTRA
        jmp     STROUT
L2BA1:
        rts

; ----------------------------------------------------------------------------
ERREXTRA:
        .byte   "?EXTRA IGNORED"
        .byte   $0D,$0A,$00
ERRREENTRY:
        .byte   "?REDO FROM START"
        .byte   $0D,$0A,$00
