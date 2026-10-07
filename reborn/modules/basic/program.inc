; error
; line input, line editing
; tokenize
; detokenize
; BASIC program memory management

; MICROTAN has some nonstandard extension to LIST here

.segment "CODE"

MEMERR:
        ldx     #ERR_MEMFULL

; ----------------------------------------------------------------------------
; HANDLE AN ERROR
;
; (X)=OFFSET IN ERROR MESSAGE TABLE
; (ERRFLG) > 128 IF "ON ERR" TURNED ON
; (CURLIN+1) = $FF IF IN DIRECT MODE
; ----------------------------------------------------------------------------
ERROR:
        lsr     Z14
        jsr     CRDO
        jsr     OUTQUES
L2329:
        cpx     #ERR_OS                         ; (The system's: its text, os_err's: hyio.inc)
        bne     :+
        jsr     OS_ERRTEXT
        bra     L2330
:
        lda     ERROR_ADDRS,x                   ; (Its message: the table's address of it)
        ldy     ERROR_ADDRS+1,x
        jsr     OUTHTASC
L2330:
        jsr     STKINI
        lda     #<QT_ERROR
        ldy     #>QT_ERROR

; ----------------------------------------------------------------------------
; PRINT STRING AT (Y,A)
; PRINT CURRENT LINE # UNLESS IN DIRECT MODE
; FALL INTO WARM RESTART
; ----------------------------------------------------------------------------
PRINT_ERROR_LINNUM:
        ldx     #1                              ; (A script's code: 1, after an error or a BREAK)
        stx     exit_code
        jsr     STROUT
        ldy     CURLIN+1
        iny
        beq     L2340
        jsr     INPRT
L2340:
        lda     interactive                     ; (Not at the console, or a script: no OK, so its line
        beq     :+                              ;   ended here)
        lda     script
        beq     RESTART
:
        jsr     CRDO

; ----------------------------------------------------------------------------
; WARM RESTART ENTRY
; ----------------------------------------------------------------------------
RESTART:
        jsr     IO_RESET                        ; (Files LOAD or SAVE had: closed; a script: its end)
        lsr     Z14
        lda     interactive                     ; (OK: at the console only)
        beq     L2351
        lda     #<QT_OK
        ldy     #>QT_OK
        jsr     GOSTROUT
L2351:
        ldx     #$FF                            ; (Direct mode while the line's read: INLIN tells it from INPUT's)
        stx     CURLIN+1
        jsr     INLIN
        stx     TXTPTR
        sty     TXTPTR+1
        jsr     CHRGET
; bug in pre-1.1: CHRGET sets Z on '\0'
; and ':' - a line starting with ':' in
; direct mode gets ignored
        tax
        beq     L2351
        ldx     #$FF
        stx     CURLIN+1
        bcc     NUMBERED_LINE
        bit     ldfd                            ; (A file being loaded: its lines without a number, a #!
        bpl     L2351                           ;   line say, passed over)
        jsr     PARSE_INPUT_LINE
        jmp     NEWSTT2

; ----------------------------------------------------------------------------
; HANDLE NUMBERED LINE
; ----------------------------------------------------------------------------
NUMBERED_LINE:
        jsr     LINGET
        jsr     PARSE_INPUT_LINE
        sty     EOLPNTR
        jsr     FNDLIN
        bcc     PUT_NEW_LINE
        ldy     #$01
        lda     (LOWTR),y
        sta     INDEX+1
        lda     VARTAB
        sta     INDEX
        lda     LOWTR+1
        sta     DEST+1
        lda     LOWTR
        dey
        sbc     (LOWTR),y
        clc
        adc     VARTAB
        sta     VARTAB
        sta     DEST
        lda     VARTAB+1
        adc     #$FF
        sta     VARTAB+1
        sbc     LOWTR+1
        tax
        sec
        lda     LOWTR
        sbc     VARTAB
        tay
        bcs     L23A5
        inx
        dec     DEST+1
L23A5:
        clc
        adc     INDEX
        bcc     L23AD
        dec     INDEX+1
        clc
L23AD:
        lda     (INDEX),y
        sta     (DEST),y
        iny
        bne     L23AD
        inc     INDEX+1
        inc     DEST+1
        dex
        bne     L23AD
; ----------------------------------------------------------------------------
PUT_NEW_LINE:
        jsr     SETPTRS
        jsr     LE33D
        lda     INPUTBUFFER
        jeq     L2351
        clc
        lda     VARTAB
        sta     HIGHTR
        adc     EOLPNTR
        sta     HIGHDS
        ldy     VARTAB+1
        sty     HIGHTR+1
        bcc     L23D6
        iny
L23D6:
        sty     HIGHDS+1
        jsr     BLTU
        lda     LINNUM                          ; (The line's number before it, and a link that isn't 0, which
        ldy     LINNUM+1                        ;   FIX_LINKS would take for the program's end: the buffer was in
        sta     INPUTBUFFER-2                   ;   the zero page after Z17, Z18 and LINNUM)
        sty     INPUTBUFFER-1
        lda     #$FF
        sta     INPUTBUFFER-3
        lda     STREND
        ldy     STREND+1
        sta     VARTAB
        sty     VARTAB+1
        ldy     EOLPNTR
        dey
; ---COPY LINE INTO PROGRAM-------
L23E6:
        lda     INPUTBUFFER-4,y
        sta     (LOWTR),y
        dey
        bpl     L23E6

; ----------------------------------------------------------------------------
; CLEAR ALL VARIABLES
; RE-ESTABLISH ALL FORWARD LINKS
; ----------------------------------------------------------------------------
FIX_LINKS:
        jsr     SETPTRS
        jsr     LE33D
        jmp     L2351
LE33D:
        lda     TXTTAB
        ldy     TXTTAB+1
        sta     INDEX
        sty     INDEX+1
        clc
L23FA:
        ldy     #$01
        lda     (INDEX),y
        beq     RET3
        ldy     #$04
L2405:
        iny
        lda     (INDEX),y
        bne     L2405
        iny
        tya
        adc     INDEX
        tax
        ldy     #$00
        sta     (INDEX),y
        lda     INDEX+1
        adc     #$00
        iny
        sta     (INDEX),y
        stx     INDEX
        sta     INDEX+1
        bcc     L23FA	; always

; ----------------------------------------------------------------------------
; !!! kbd_loadsave.s requires an RTS here!
RET3:
        rts

; ----------------------------------------------------------------------------
; TOKENIZE THE INPUT LINE
; ----------------------------------------------------------------------------
; (Letters outside strings, REM and DATA in either case: upper case, as the keywords and names are)
PARSE_INPUT_LINE:
        ldx     TXTPTR
        ldy     #$04
        sty     DATAFLG
L246C:
        lda     INPUTBUFFERX,x
        cmp     #$20
        beq     L24AC
        sta     ENDCHR
        cmp     #$22
        beq     L24D0
        bit     DATAFLG
        bvs     L24AC
        jsr     UPCASE_X
        cmp     #$3F
        bne     L2484
        lda     #TOKEN_PRINT
        bne     L24AC
L2484:
        cmp     #$30
        bcc     L248C
        cmp     #$3C
        bcc     L24AC
; ----------------------------------------------------------------------------
; SEARCH TOKEN NAME TABLE FOR MATCH STARTING
; WITH CURRENT CHAR FROM INPUT LINE
; ----------------------------------------------------------------------------
; (The table is past 256 bytes now, so KWPTR walks it a keyword at a time and .Y counts in a keyword)
L248C:
        sty     STRNG2
        stz     EOLPNTR
        stx     TXTPTR
        jsr     KW_FIRST
L2490:
        ldx     TXTPTR
        ldy     #$00
L2498:
        jsr     UPCASE_X
        sec
        sbc     (KWPTR),y
        bne     L249E
        iny
        inx
        bra     L2498
L249E:
        cmp     #$80
        bne     L24D7
        ora     EOLPNTR
        cmp     #FIRST_ALIAS                    ; (An alias: its keyword's token)
        bcc     L24AA
        sbc     #FIRST_ALIAS
        tay
        lda     ALIAS_MAP,y
; ----------------------------------------------------------------------------
; STORE CHARACTER OR TOKEN IN OUTPUT LINE
; ----------------------------------------------------------------------------
L24AA:
        ldy     STRNG2
L24AC:
        inx
        iny
        sta     INPUTBUFFER-5,y
        lda     INPUTBUFFER-5,y
        beq     L24EA
        sec
        sbc     #$3A
        beq     L24BF
        cmp     #$49
        bne     L24C1
L24BF:
        sta     DATAFLG
L24C1:
        sec
        sbc     #TOKEN_REM-':'
        bne     L246C
        sta     ENDCHR
; ----------------------------------------------------------------------------
; HANDLE LITERAL (BETWEEN QUOTES) OR REMARK,
; BY COPYING CHARS UP TO ENDCHR.
; ----------------------------------------------------------------------------
L24C8:
        lda     INPUTBUFFERX,x
        beq     L24AC
        cmp     ENDCHR
        beq     L24AC
L24D0:
        iny
        sta     INPUTBUFFER-5,y
        inx
        bne     L24C8
; ----------------------------------------------------------------------------
; ADVANCE POINTER TO NEXT TOKEN NAME
; ----------------------------------------------------------------------------
L24D7:
        inc     EOLPNTR
        jsr     KW_NEXT
        lda     (KWPTR)
        bne     L2490
        ldx     TXTPTR
        jsr     UPCASE_X
        bpl     L24AA
; ---END OF LINE------------------
L24EA:
        sta     INPUTBUFFER-3,y
        lda     #<INPUTBUFFER-1
        sta     TXTPTR
        rts

; KWPTR = the keyword table's first keyword (KW_FIRST), or the next after its own (KW_NEXT).  Keep .X
KW_FIRST:
        lda     #<TOKEN_NAME_TABLE
        sta     KWPTR
        lda     #>TOKEN_NAME_TABLE
        sta     KWPTR+1
        rts
KW_NEXT:
        ldy     #0
:
        lda     (KWPTR),y
        iny
        asl     a
        bcc     :-
        tya
        clc
        adc     KWPTR
        sta     KWPTR
        bcc     :+
        inc     KWPTR+1
:
        rts

; .A = the line's character at .X, a letter in upper case.  N and Z are .A's
UPCASE_X:
        lda     INPUTBUFFERX,x
        cmp     #'a'
        bcc     :+
        cmp     #'z'+1
        bcs     :+
        and     #$DF
:
        ora     #0
        rts

; ----------------------------------------------------------------------------
; SEARCH FOR LINE
;
; (LINNUM) = LINE # TO FIND
; IF NOT FOUND:  CARRY = 0
;	LOWTR POINTS AT NEXT LINE
; IF FOUND:      CARRY = 1
;	LOWTR POINTS AT LINE
; ----------------------------------------------------------------------------
FNDLIN:
        lda     TXTTAB
        ldx     TXTTAB+1
FL1:
        ldy     #$01
        sta     LOWTR
        stx     LOWTR+1
        lda     (LOWTR),y
        beq     L251F
        iny
        iny
        lda     LINNUM+1
        cmp     (LOWTR),y
        bcc     L2520
        beq     L250D
        dey
        bne     L2516
L250D:
        lda     LINNUM
        dey
        cmp     (LOWTR),y
        bcc     L2520
        beq     L2520
L2516:
        dey
        lda     (LOWTR),y
        tax
        dey
        lda     (LOWTR),y
        bcs     FL1
L251F:
        clc
L2520:
        rts

; ----------------------------------------------------------------------------
; "NEW" STATEMENT
; ----------------------------------------------------------------------------
NEW:
        bne     L2520
SCRTCH:
        lda     #$00
        tay
        sta     (TXTTAB),y
        iny
        sta     (TXTTAB),y
        lda     TXTTAB
        clc
        adc     #$02
        sta     VARTAB
        lda     TXTTAB+1
        adc     #$00
        sta     VARTAB+1
; ----------------------------------------------------------------------------
SETPTRS:
        jsr     STXTPT
        lda     #$00

; ----------------------------------------------------------------------------
; "CLEAR" STATEMENT
; ----------------------------------------------------------------------------
CLEAR:
        bne     L256A
CLEARC:
        jsr     CH_CLOSE_ALL                    ; (The files OPEN made: closed, as RUN, NEW, CLEAR and a line
                                                ;   entered clear the variables)
        lda     MEMSIZ
        ldy     MEMSIZ+1
        sta     FRETOP
        sty     FRETOP+1
        lda     VARTAB
        ldy     VARTAB+1
        sta     ARYTAB
        sty     ARYTAB+1
        sta     STREND
        sty     STREND+1
        jsr     RESTORE
; ----------------------------------------------------------------------------
STKINI:
        ldx     #TEMPST
        stx     TEMPPT
        pla
        tay
        pla
        ldx     #STACK_TOP
        txs
        pha
        tya
        pha
        lda     #$00
        sta     OLDTEXT+1
        sta     SUBFLG
L256A:
        rts

; ----------------------------------------------------------------------------
; SET TXTPTR TO BEGINNING OF PROGRAM
; ----------------------------------------------------------------------------
STXTPT:
        clc
        lda     TXTTAB
        adc     #$FF
        sta     TXTPTR
        lda     TXTTAB+1
        adc     #$FF
        sta     TXTPTR+1
        rts

; ----------------------------------------------------------------------------

; ----------------------------------------------------------------------------
; "LIST" STATEMENT
; ----------------------------------------------------------------------------
LIST:
        bcc     L2581
        beq     L2581
        cmp     #TOKEN_MINUS
        bne     L256A
L2581:
        jsr     LINGET
        jsr     FNDLIN
        jsr     CHRGOT
        beq     L2598
        cmp     #TOKEN_MINUS
        bne     L2520
        jsr     CHRGET
        jsr     LINGET
        bne     L2520
L2598:
        pla
        pla
        lda     LINNUM
        ora     LINNUM+1
        bne     :+
        lda     #$FF
        sta     LINNUM
        sta     LINNUM+1
:
        jsr     LIST_LINES
        jmp     RESTART

; The lines from LOWTRX to the one numbered LINNUM out, each after a new line (none before the first with nolf1
; set: SAVE's).  Ctrl-C stops it (ISCNTC)
LIST_LINES:
L25A6:
        ldy     #$01
        lda     (LOWTRX),y
        beq     L25E5
        jsr     ISCNTC
        lda     nolf1
        stz     nolf1
        bne     :+
        jsr     CRDO
:
        ldy     #$01
        iny
        lda     (LOWTRX),y
        tax
        iny
        lda     (LOWTRX),y
        cmp     LINNUM+1
        bne     L25C1
        cpx     LINNUM
        beq     L25C3
L25C1:
        bcs     L25E5
; ---LIST ONE LINE----------------
L25C3:
        sty     FORPNT
        jsr     LINPRT_NS
        lda     #$20
L25CA:
        ldy     FORPNT
        and     #$7F
L25CE:
        jsr     OUTDO
        iny
        beq     L25E5
        lda     (LOWTRX),y
        bne     L25E8
        tay
        lda     (LOWTRX),y
        tax
        iny
        lda     (LOWTRX),y
        stx     LOWTRX
        sta     LOWTRX+1
        bne     L25A6
L25E5:
        rts
L25E8:
        bpl     L25CE
        sec
        sbc     #$7F
        tax
        sty     FORPNT
        jsr     KW_FIRST                        ; (The token's keyword: KWPTR, as the tokenizer's)
L25F2:
        dex
        beq     L25FD
        jsr     KW_NEXT
        bra     L25F2
L25FD:
        ldy     #$FF
L25FE:
        iny
        lda     (KWPTR),y
        bmi     L25CA
        jsr     OUTDO
        bne     L25FE	; always
