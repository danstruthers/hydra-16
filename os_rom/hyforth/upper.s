;
;   upper.s  - utility functions for HyForth - eventually ROM resident
;

;-------------------------------------------------------------
;
;           CODE called by engine AND heap
;
;------- used by 'exit'
;------------------------NOTE:  this is ALL of 'exit' but the screaming.
;----------------- For modularity purposes this is the only sensible place for it.
;
EXIT:
unnest:                             ; EXIT - done with previous word, on to next whether compiled or primitive

    ldy #INSTPTR
    jsr rpull                       ; IP = [RTPTR], RTPTR += 2

next:                               ;    go on to next word; IP is pointing at next entry in data field of word
; WORKREG = (INSTPTR) ; INSTPTR += 2
    ldx #INSTPTR
    ldy #WORKREG
    jsr copyfrom                    ; W = [IP], IP += 2  ( and either ENTER or EXECUTE )

pick:                               ;        'THE SWITCHER' - COMPILED OR PRIMITIVE?
                                    ;
.ifdef DEBUG                        ;  DEBUG
    lda DFLAG
    bne PRINTWSKIP                  ; print W and [W] etc if DEBUG
    WSEQ_raw WDISP
    lda WORKREG+1
    PRINT_BYTE
    lda WORKREG
    PRINT_BYTE
    WSEQ_raw WATDISP
    ldy #0
    lda (WORKREG),y
    PRINT_BYTE
PRINTWSKIP:
.endif
; .. use old-fashioned way AND new tricks
;
    lda WORKREG + 1                 ; compare pages (MSBs)
    cmp #>ends + 1                  ;  !! Compiled must be higher in memory than hardcoded !! (see below)
    bmi jump                        ; jump over nest if native code
    ldy #1                          ; ldy #0 if check extra byte
    lda (WORKREG),y
    beq jump                        ; yep, jump to native execution

nest:                               ; ENTER in classic Forth lingo   ( COMPILED )
    ldy #INSTPTR
    jsr rpush                       ; RTPTR -=2, [RTPTR] = [IP]
    lda WORKREG                     ; W = IP
    sta INSTPTR
    lda WORKREG + 1
    sta INSTPTR + 1
    jmp next                        ; next

jump:                               ; EXECUTE                      ( PRIMITIVE )
    jmp (WORKREG)                   ; start running code at next word
                                    ;  JUMP [W]
;
;------- used by 'autoload'
ALOADTIB:
    ldy #0
    lda #ASCII_SPACE
    sta (TIB),y
ALOADLOOP:
    lda (TEMP7),y
    beq ALOADSKIP
    iny
    sta (TIB),y
    bra ALOADLOOP
ALOADSKIP:
    jsr ALOADCHKDONE
    iny
    rts                             ; y points at trailing space

ALOADCHKDONE:
    iny
    lda (TEMP7),y
    bne ALNOTSKIP                   ; calc next address if not done
    stz ALFLAG                      ; turn off autoload
    bra ALNOTDONE
ALNOTSKIP:
    tya
    clc
    adc TEMP7
    sta TEMP7
    bcc ALNOTDONE
    inc TEMP7+1
ALNOTDONE:
    dey
    rts
;
;
;  Keep the MMU from handing out the pages the dictionary grows into: raise the MMU page floor
;  (DICTLIM) to FORTH_DICT_MARGIN pages above 'here' when needed.
;  OUT: C = 1 OK; C = 0 the dictionary is full (the MMU has allocated those pages)
;  uses a, x, y
DICTCHK:
    lda NEXTHEAP+1
    clc
    adc #FORTH_DICT_MARGIN + 1   ; floor wanted: page of 'here' + margin + 1
    cmp DICTLIM
    bcc DICTOK                   ; already at or above it
    beq DICTOK
    pha
    jsr MM_SET_FLOOR             ; MMU: C = 0 OK
    pla
    bcs DICTNO
    sta DICTLIM
DICTOK:
    sec
    rts
DICTNO:
    clc
    rts
;
;-------------------------------------------------------------------
;              get delimited text from INBUF, store in string
;

.ifndef TXT2STACK
;
;  a token that starts q^ (ended by the next ^) or " (ended by the next "): its text, spaces and all
;  uses TEMP1, TEMP2, TEMP3, TEMP5, TEMP6, X, Y
;
TEXTGET:
    stz TEMP1+1                     ; will store length here
    lda #$04
    sta TEMP2                       ; record type is 'sz'
    stz TEMP2+1
    stz TEMP6                       ; to save y for later copy
    ldy #1                          ; skip len of first token
TX2SKSPC:
    lda (NXTTOK),y
    cmp #ASCII_SPACE                ; skip leading spaces
    bne TX2SK00
    iny
    bra TX2SKSPC
TX2SK00:
    cmp #ASCII_DQUOTE               ; "text": ended by a '"'
    beq TX2OPEN
    cmp #ASCII_q                    ; q^text^: ended by a '^'
    bne TX2NOGOOD
    iny
    lda (NXTTOK),y
    cmp #ASCII_CARET
    bne TX2NOGOOD
TX2OPEN:
    sta TEMP3                       ; temp3 = the delimiter that ends it
    iny
    sty TEMP6                       ; temp6 stores pos of first char
TX2SCAN:
    lda (NXTTOK),y                  ; find delimiting '^' (or '"')
    beq TX2NOGOOD                   ; (the line's end: not a string)
    iny
    cmp TEMP3
    beq TX2FOUND
    tya
    clc
    adc NXTTOK
    cmp #MAXSTR
    bcs TX2NOGOOD
    bra TX2SCAN
TX2FOUND:
    dey
    sty TEMP5                       ; temp5 pos+1 last letter
    tya
    sec
    sbc TEMP6
    sta TEMP1                       ; and this should be the length
    inc TEMP1                       ; and add one for zero at end

    jsr MALLOC                      ; TEMP1 now has address on mem stack
    bcs TX2ROOM
    lda #ERR_MEM                    ; out of memory: reported when the token isn't found
    sta ERRFLAG
    jmp TX2NOGOOD
TX2ROOM:

    ldy #0
    lda (TEMP1),y
    sta TEMP2
    iny
    lda (TEMP1),y
    sta TEMP2+1                     ; address in memory area

    ; now for math.  NXTTOK ptr needs to be ref'd with same y
    ; as TEMP2, so we need to match them up...and we are hitting
    ; the record at +3, so....TEMP6 is start
    lda TEMP2
    sec
    sbc TEMP6
    bcs TX2SK01
    dec TEMP2+1
TX2SK01:
    clc
    adc #3
    sta TEMP2
    bcc TX2SK02
    inc TEMP2+1
TX2SK02:
    ldy TEMP6
TX2CPYLOOP:
    cpy TEMP5                       ; (an empty string: nothing to copy)
    beq TX2SK99
    lda (NXTTOK),y
    sta (TEMP2),y
    iny
    bra TX2CPYLOOP
TX2SK99:
    lda #0
    sta (TEMP2),y                   ; put zero on end
    jsr spush_0                     ; push address from mem stack on DS
    ldx TEMP5                       ; the length byte through the delimiter: blanked by 'token'
    inx
    clc
    jmp TX2END
TX2NOGOOD:
    sec
TX2END:
    rts
;
;  end of TXG2  (new text capture)
;

.else  ;  TXT2STACK

TEXTGET:
    stz TEMP6
    stz TEMP1
    stz TEMP1+1
    ldy #1                          ; skip len
TXTSKIPSPC:
    lda (NXTTOK),y
    cmp #ASCII_SPACE                ; skip leading spaces
    bne TXTSPCS
    iny
    bra TXTSKIPSPC
TXTSPCS:
    cmp #ASCII_q
    bne TEXTNOGOOD
    iny
    lda (NXTTOK),y
    cmp #ASCII_CARET
    bne TEXTNOGOOD
    sta TEMP3                       ; remember....
    iny
TXTSCAN:
    lda (NXTTOK),y                  ; find delimiting '^'
    iny
    cmp #ASCII_CARET
    beq TXTFOUND
    tya
    clc
    adc NXTTOK
    cmp #MAXSTR
    bcs TEXTNOGOOD
    bra TXTSCAN
TXTFOUND:
    ldx #0
    dey
    dey                             ; now points at last letter
    tya
    sec
    sbc TEMP3                       ; and this should be the length
    stx TEMP3
    and #1
    beq TEXTGLOOP
    inc TEMP3
    inx
TEXTGLOOP:
    lda (NXTTOK),y
    cmp #ASCII_CARET                        ; when we hit the other end again...
    beq TEXTOK
    sta TEMP6

.ifdef DEBUG
    jsr DUMPREG                     ; DEBUG
.endif

    txa
    and #1
    bne TEXTODD
    lda TEMP6
    sta TEMP1
    stz TEMP1+1
    bra TEXTSKIP2
TEXTODD:
    lda TEMP6
    sta TEMP1+1
    phx
    phy
    jsr spush_0
    ply
    plx
TEXTSKIP2:
    inx
    dey
.ifdef DEBUG
    jsr DUMPREG                     ; DEBUG
.endif
    bra TEXTGLOOP
TEXTOK:
    phy
    phx
    txa
    and #1
    beq TEXTCONT
    jsr spush_0
TEXTCONT:
    plx
    txa
    sec
    sbc TEMP3
    sta TEMP3
    stz TEMP3+1
    jsr spush_2                     ; push length on top
    ply
    clc
    bra TEXTGEND
TEXTNOGOOD:
    sec
TEXTGEND:
    rts

.endif  ; ---TXT2STACK

.ifdef numbers
;------------------------
;      CONVERT DIGITS, PUSH on DS

DIGCONVT:    ;  Y is index into NXTPTR, X is length
    stz TEMP1
    stz TEMP1+1
    stz TEMP2                       ; no necc for bin or hex, but
    stz TEMP2+1                     ; since is convenient....
    stz TEMP6
    ldy #1                          ; skip over length
    lda #10
    sta DIGBASE
    lda (NXTTOK),y
    cmp #ASCII_MINUS
    beq DIGCONV_MINUS
    cmp #ASCII_DOLLAR
    beq DIGHEX1
    cmp #ASCII_PERCENT
    beq DIGBIN1
    bra DIG_SNG
DIGCONV_MINUS:
    inc TEMP6
    jmp DIG_XY
DIGBIN1:
    lda #2
    sta DIGBASE
    jmp DIG_XY
DIGHEX1:
    lda #16
    sta DIGBASE
DIG_XY:
    iny
    dex
DIG_SNG:
  .ifdef SINGLE
    cpx #1                          ; skip single digit, handle hard-wired or...
    bne DIGCONV_LOOP                ; only one digit left, we can handle that elsewhere
    jmp DIGCONV_ERR                 ; not really an error, just return and let 'find' to it
  .endif
DIGCONV_LOOP:
    jsr GETDIG
    bcc DIGCONT0
    jmp DIGCONV_ERR
DIGCONT0:
    pha                             ; else save it to add in a bit
    asl TEMP1                       ; do first shift
    rol TEMP1+1
    lda DIGBASE                     ; check base that was set above
    cmp #10
    beq DIGDEC                      ; it's DEC, go there
    cmp #2
    beq DIGBIN                      ; it's BIN, go there
    bra DIGHEX                      ; Go to hex if others not true
DIGDEC:
    asl TEMP1                       ; 2nd shift; upper nybble doesn't end up right
    rol TEMP1+1
    lda TEMP1                       ; load results of two shifts
    clc
    adc TEMP2                       ; add in previous total from last loop
    sta TEMP1
    lda TEMP1+1                     ; and same with second digit, and the carry
    adc TEMP2+1
    sta TEMP1+1
DIGDEC2:
    asl TEMP1                       ; final shift
    rol TEMP1+1
    pla                             ; bring back read digit
    cmp #10                         ; make sure it's not hex, mostly
    bcc DIGCONT                     ; jump to continue
    jmp DIGCONV_ERR                 ; else throw error
DIGHEX:
    asl TEMP1
    rol TEMP1+1
    asl TEMP1
    rol TEMP1+1
    asl TEMP1
    rol TEMP1+1                     ; shifted three more times
    pla
    jmp DIGCONT
DIGBIN:                             ; already did the single shift
    pla
    cmp #2
    bcs DIGCONV_ERR                 ; fall through to the additon of new digit
DIGCONT:
    clc                             ; add digit finally
    adc TEMP1
    sta TEMP1
    sta TEMP2                       ; copy to intermediate result in case another dec digit
    bcc DIGCONT2
    inc TEMP1+1                     ; and inc 2nd byte if necc.
DIGCONT2:
    lda DIGBASE
    cmp #10
    bne DIGCONT3
    lda TEMP1+1
    sta TEMP2+1                     ; save it for next round, regardless
    cmp #$80                        ; check to see if > $8000
    bcs DIGCONV_ERR
DIGCONT3:
    iny
    dex
    bne DIGCONV_LOOP
    lda DIGBASE
    cmp #10
    bne DIGCONT4
DECFINISH:
    lda TEMP6                       ; check for minus
    beq DIGCONT4
    lda TEMP1
    eor #$FF
    sta TEMP1
    lda TEMP1+1
    eor #$FF
    sta TEMP1+1
    inc TEMP1
    bne DIGCONT4
    inc TEMP1+1
DIGCONT4:
    jsr spush_0                     ; push TEMP1 on to stack
    clc
    rts
DIGCONV_ERR:
    sec
    rts

;--------------------------
GETDIG:                             ; y is index to next char in NXTTOK
    lda (NXTTOK),y
    sec
    sbc #ASCII_0
    bcc GETDIG_ERR
    cmp #10
    bcc GETDIG_RTN
    sbc #7
    cmp #10
    bcc GETDIG_ERR
    cmp #16
    bcs GETDIG_ERR
GETDIG_RTN:                         ; pass carry clear for good digit
    clc
    rts
GETDIG_ERR:                         ; pass carry set for no digit
    sec
    rts
;  end of new number conv
;

;
;
.endif    ; numbers
;--------------------------------- CLEAR ------------------------------
;
;  zero out $C8 - $FF, meet and greet
;  zero out INBUF also
;
HYWELCOME:
    .byte ASCII_CR, ASCII_LF
    .byte "HyForth 0.91 05-07-2026"
    .byte ASCII_CR, ASCII_LF, 0
CLEAR:
    lda #1
    sta DFLAG                       ; debug OFF by default
zpclear:
    ldx #ZPSTART                    ; whoops, this is all the ZP that is used
    lda #0
zerolp:
    sta  $00,x
    inx
    bne zerolp
    ldx  #0
inbufzlp:                           ; and then clear out INBUF
    sta  INBUF,x
    inx
    bne  inbufzlp
    ldx  #0
stackslp:                           ; and then clear out DS / RT
    sta  DS,x
    inx
    bne  stackslp
        ;  meet and greet (not a command shell's: CMDFLAG)
    lda  CMDFLAG
    bne  CLREXIT
    ldy  #0
hywelclp:
    lda  HYWELCOME,y
    beq  CLREXIT
    PRINT_CHAR
    iny
    bra  hywelclp
CLREXIT:
    rts
;
;
.ifdef DEBUG
;      argh.
;   Include DEBUG code here (page 1's room after the BIOS thunks, $F800: segment FORTH_HIGH)
;
.segment "FORTH_HIGH"
;
;  debug stuff   ------------------------------ DUMPREG ---------------------
;
DUMPTXT1:
        .byte "SP/PC/nv-bdizc/A/X/Y -> "
        .byte 0
DUMPMSG1:
        .byte "<spc> to continue, x for Wozmon, z for ZP, t for TIB, s for stacks"
        .byte ASCII_CR, ASCII_LF, 0

DUMPREG:            ; dump registers safely and print
    php                         ; -3
    pha                         ; -4
    phx                         ; -5
    phy                         ; -6
    lda   DFLAG
    beq   DPFCHECK
    jmp   DREGNXT
DPFCHECK:
    ldy   #0
    WCRLF_np
DUMPLP0:
    lda   DUMPTXT1, y
    beq   DUMPCON
    PRINT_CHAR
    iny
    bra   DUMPLP0
DUMPCON:
    tsx                         ; -6
    txa
    clc
    adc   #6
    PRINT_BYTE      ; print stack pointer b4 jump
    PRINT_CHAR #ASCII_SLASH
    inx                         ; -5
    inx                         ; -4
    inx                         ; -3
    inx                         ; -2
    inx                         ; -1
    lda   $0100,x               ; get PC LSB
    tay
    inx                         ; 0
    PRINT_BYTE {$0100,x}        ; get PC MSB
    tya
    PRINT_BYTE
    PRINT_CHAR #ASCII_SLASH
        ;   whew
        ;
    dex                         ; -1
    dex                         ; -2
    lda   $0100,x               ; get status byte
    ldy   #8
DUMPLP2:
    rol
    pha
    bcc   DUMPSKx               ; NV-BDIZC
    lda   #ASCII_1
    bra   DUMPSKy
DUMPSKx:
    lda   #ASCII_0
DUMPSKy:
    PRINT_CHAR                  ; print y-th bit
    pla
    dey
    bne   DUMPLP2
    PRINT_CHAR #ASCII_SLASH
    dex                         ; -3
    PRINT_BYTE {$0100,x}        ; print A
    PRINT_CHAR #ASCII_SLASH
    dex                         ; -4
    lda   $0100,x
    PRINT_BYTE                  ; print X
    PRINT_CHAR #ASCII_SLASH
    dex                         ; -5
    PRINT_BYTE {$0100,x}        ; print Y
    WCRLF_np
    ldy   #0
DUMPLP1:                        ; print message
    lda   DUMPMSG1, y
    beq   DREGLP1
    PRINT_CHAR
    iny
    bra   DUMPLP1
DREGLP1:
    jsr   GET_CHAR              ; wait for a key
    bcc   DREGLP1
    cmp   #ASCII_s              ; print out stacks?
    bne   DREGSK3
    jsr   DUMPSTACK
DREGSK3:
    cmp   #ASCII_z              ; print out zp?
    bne  DREGSK5
    jsr  DUMPZP
DREGSK5:
    cmp   #ASCII_t              ; print out INBUF?
    bne  DREGSK4
    jsr  DUMPTIB
DREGSK4:
    cmp   #ASCII_x
    bne   DREGELSE
    jsr   $FE00                 ; go to Wozmon if necc
DREGELSE:
    cmp   #ASCII_CR                 ; continue on enter
    beq   DREGNXT
    jmp   DPFCHECK              ; otherwise print eveything again
DREGNXT:
    ply                         ; and restore everything
    plx
    pla
    plp
    rts
;
;   subs for particular pages
;
DUMPTIB:                            ; dump TIB
    php
    pha
    lda #>INBUF
    sta TEMP0+1
    jmp DUMPPDBG
DUMPSTACK:                          ; dump DS/RT stack area
    php
    pha
    lda #>DS
    sta TEMP0+1
    jmp DUMPPDBG
DUMPZP:                             ; dump ZP
    php
    pha
    stz TEMP0+1
    jmp DUMPPDBG
.segment "FORTH_ROM"
.endif

DUMPPAGE:                           ; general purpose page dumper
                                    ; TEMP0 starts at zero, TEMP0+1 is page #
    php
    pha
DUMPPDBG:
    ldy #0
    sty TEMP0
DUMPLOOP:
    lda TEMP0+1
    PRINT_BYTE
    tya
    PRINT_BYTE
    PRINT_CHAR #ASCII_COLON
DPLOOP2:
    lda (TEMP0), y
    PRINT_BYTE
    PRINT_SPACE
    iny
    tya
    and #$0F                    ; 00001111
    beq  DPSKIP
    bra  DPLOOP2
DPSKIP:
    PRINT_SPACE
    phy
    tya
    sec
    sbc #$10                    ; subtract 16 to rewind the line
    tay
DPPLOOP:
    lda (TEMP0), y              ; printable ascii
    cmp #ASCII_SPACE
    bcc PPERIOD
    cmp #$7F
    bcs PPERIOD
    PRINT_CHAR
    bra DPPSKIP
PPERIOD:
    PRINT_CHAR #ASCII_PERIOD
DPPSKIP:
    iny
    tya
    and #$0F
    beq DPPEND
    bra DPPLOOP
DPPEND:
    ply
    WCRLF_np
    tya                         ; need this to check y = 0
    bne DUMPLOOP
DUMPSTKEND:
    WCRLF_np
    pla
    plp
    rts
;
;    load mainoff (TEMP1 $F0), ramstart (TEMP3 $F4), and endsoff (TEMP2 $F2)
;     with current ROM offsets (main = start, ends = finish)
;     and ramstart with destination corresponding to main.
;     Set 'supprint' <> 0 (TEMP5 $FC) to suppress printing of progress bar.
;
MEMCPY:
COPYLOOP:
    MOV (mainoff), (ramstart)
    BNE16 mainoff, endsoff, SKIP1
    rts

SKIP1:
    lda supprint
    bne SKIP01
    PRINT_CHAR #ASCII_PERIOD

SKIP01:
    INC16_BARE mainoff
    INC16_BARE ramstart
    bra COPYLOOP
; ---------------- end of upper.s
;