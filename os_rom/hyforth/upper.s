;
;   upper.s  - utility functions for HyForth - eventually ROM resident
;
;---------------------------------------------------------------------
;  error messaging
;
wrterror:
    lda #>err_jumptable
    sta ERRPTR+1
    lda ERRFLAG
    beq ERREND
    asl a
    clc
    adc #<err_jumptable
    sta ERRPTR
    bcc ERRSKIP
    inc ERRPTR+1
ERRSKIP:
    WERR ERRPTR
ERREND:
    lda #0
    sta ERRFLAG
    sta ERRPTR
    sta ERRPTR+1
    rts
;
;
err_jumptable:
    .res 2
    ERR_entry RPTR_ERR              ; RT stack full/empty  - error $01
    ERR_entry SPTR_ERR              ; DS stack full/empty  - error $02
    ERR_entry DIV_ERR               ; divide by zero - error $03
    ERR_entry OOM_ERR               ; out of memory  - error $04
    ERR_entry UKW_ERR               ; no existing word - error $05
    ERR_entry SEC_ERR               ; writing to dangerous RAM areas - error $06
    ERR_entry SYS_ERR               ; error on return from SYSCALL - error $07
LASTERR = 7
;
;  error messages
RPTR_ERR:
    .byte " !RT PTR ERROR!"
    .byte 0
SPTR_ERR:
    .byte " !DS PTR ERROR!"
    .byte 0
DIV_ERR:
    .byte " !DIV ZERO!"
    .byte 0
OOM_ERR:
    .byte " !LOW MEM!"
    .byte 0
UKW_ERR:
    .byte " !UNK WORD!"
    .byte 0
SEC_ERR:
    .byte " !SECURITY!"
    .byte 0
SYS_ERR:
    .byte " !SYS ERR!"
    .byte 0

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
;-------- malloc and mlen

MALLOC:
    ;  TEMP1 and TEMP2 should have bytes / record type if 'jsr MALLOC'
    ;  uses TEMP3, y, x, a
    lda MEMLAST
    sec
    sbc #3
    sta MEMLAST
    bcs MALSK00
    dec MEMLAST+1
MALSK00:
    sec
    sbc TEMP1
    sta MEMLAST
    lda MEMLAST+1
    sbc TEMP1+1
    sta MEMLAST+1
                        ;MEMLAST updated to start of new record
    lda MEMLAST
    sta TEMP3
    lda MEMLAST+1
    sta TEMP3+1         ; use TEMP3 to walk through clearing of memory
    ldy #0
    lda TEMP2            ; write type first
    sta (TEMP3),y
    iny
    lda TEMP1            ; LSB length
    sta (TEMP3),y
    iny
    lda TEMP1+1          ; MSB length
    sta (TEMP3),y
    ldx #TEMP3
    lda #3
    jsr addwx            ; increment TEMP3 by 3
MALLOOP:
    lda #0
    ldy #0
    sta (TEMP3),y
    dec TEMP1
    bne MALSK02    
    lda TEMP1+1
    beq MALCONT
    lda TEMP1
    cmp #$FF
    bne MALSK02       
    dec TEMP1+1
MALSK02:
    inc TEMP3
    bne MALSK01
    inc TEMP3+1
MALSK01:    
    bra MALLOOP
MALCONT:                   ; now store MEMLAST at MEMPTR
    ldy #0
    lda MEMLAST
    sta (MEMPTR),y
    iny
    lda MEMLAST+1
    sta (MEMPTR),y
    lda MEMPTR+1
    sta TEMP1+1
    lda MEMPTR           
    sta TEMP1             ; copy to TEMP1 before incrementing
    sec                   ; MEMPTR + 2
    sbc #2
    sta MEMPTR
    bcs MALLOCEND
    dec MEMPTR+1
MALLOCEND:
    rts    
; 
MEMLEN:           ; address in TEMP2
     ldy #0
     lda (TEMP2),y
     sta TEMP3
     iny
     lda (TEMP2),y   ; and deref once
     sta TEMP3+1
     ldy #1
     lda (TEMP3),y   ; skip over type, get length
     sta TEMP1
     iny
     lda (TEMP3),y
     sta TEMP1+1
     rts
;
;-------------------------------------------------------------
;                MATH routines
;         with MULT16 / DIV16, signs handled by calling word.
;         we just do the math here.
;
;
MULT16:                             ; 16 x 16 multiply; TEMP1 and TEMP2 are #'s, TEMP1 will be result
    stz TEMP3                       ; with TEMP3 as high bytes
    stz TEMP3+1
    ldx #17
    clc
MULTLOOP:
    ror TEMP3+1                     ; RIGHT.  if you need to go backwards, go backwards stupid fuck.
    ror TEMP3
    ror TEMP1+1
    ror TEMP1
    bcc MULTDECCNT
    clc
    lda TEMP2
    adc TEMP3
    sta TEMP3
    lda TEMP2+1
    adc TEMP3+1
    sta TEMP3+1
MULTDECCNT:
    dex
    bne MULTLOOP
    rts

;
;
       ; 16 x 16 divide; TEMP1 and TEMP2 #'s - TEMP3 is 'overflow'
       ;  TEMP2 divisor, TEMP1 dividend, TEMP1 + 3 = result + remainder
DIV16:
    stz TEMP3
    stz TEMP3+1
    ldx #16
UDIVLP:
    rol TEMP1
    rol TEMP1+1
    rol TEMP3
    rol TEMP3+1
UDIVCHK:
    sec
    lda TEMP3
    sbc TEMP2
    tay
    lda TEMP3+1
    sbc TEMP2+1
    bcc UDIVCNT
    sty TEMP3
    sta TEMP3+1
UDIVCNT:
    dex
    bne UDIVLP
    rol TEMP1
    rol TEMP1+1
    rts
;
;     galois32o - LSFR psuedo-random # generator
;
;  -- boilerplate --
; 6502 LFSR PRNG - 32-bit
; Brad Smith, 2019
; http://rainwarrior.ca
;
;
galois32o:
    ; rotate the middle bytes left
    ldy RSEED+2                     ; will move to RSEED+3 at the end
    lda RSEED+1
    sta RSEED+2
    ; compute RSEED+1 ($C5>>1 = %1100010)
    lda RSEED+3                     ; original high byte
    lsr
    sta RSEED+1                     ; reverse: 100011
    lsr
    lsr
    lsr
    lsr
    eor RSEED+1
    lsr
    eor RSEED+1
    eor RSEED+0                     ; combine with original low byte
    sta RSEED+1
    ; compute RSEED+0 ($C5 = %11000101)
    lda RSEED+3                     ; original high byte
    asl
    eor RSEED+3
    asl
    asl
    asl
    asl
    eor RSEED+3
    asl
    asl
    eor RSEED+3
    sty RSEED+3                     ; finish rotating byte 2 into 3
    sta RSEED+0
    rts
;-------------------------------------------------------------------
;              get delimited text from INBUF, store in string
;

.ifndef TXT2STACK
;
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
    cmp #ASCII_SPACE                        ; skip leading spaces
    bne TX2SK00
    iny
    bra TX2SKSPC
TX2SK00:
    cmp #ASCII_q
    bne TX2NOGOOD
    iny
    lda (NXTTOK),y
    cmp #ASCII_CARET
    bne TX2NOGOOD
    iny
    sty TEMP6                       ; temp6 stores pos of first char
TX2SCAN:
    lda (NXTTOK),y                  ; find delimiting '^'
    iny
    cmp #ASCII_CARET
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
    bcc TX2SK02
    inc TEMP2+1
TX2SK02:
    ldy TEMP6
TX2CPYLOOP:
    lda (NXTTOK),y
    sta (TEMP2),y
    iny
    cpy TEMP5
    bne TX2CPYLOOP
TX2SK99:
    lda #0
    sta (TEMP2),y                   ; put zero on end
    jsr spush_0                     ; push address from mem stack on DS
    lda TEMP5
    sec
    sbc TEMP6
    clc
    adc #4
    tax
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
    cmp #ASCII_SPACE                        ; skip leading spaces
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

;-----------------------   NUMBER CONVERSIONS
;
DEC2ASCII:       ;  X is # - return as two digits in TEMP3, TEMP3+1
    lda #ASCII_0
    sta TEMP3+1
    txa
    sta TEMP3
D2ASCLOOP:
    sec
    sbc #ASCII_LF
    bcc D2ASCNEXT
    inc TEMP3+1
    bra D2ASCLOOP
D2ASCNEXT:
    clc
    adc #$3A
    sta TEMP3
    rts

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
H2NUM: .byte $27,$10
 .byte $03,$E8
 .byte $00,$64
 .byte $00,$0A
HEX2DEC:                            ; low/high in A,Y - use X, TEMP1, TEMP3, TEMP4, TEMP6
    sty TEMP3+1
    sta TEMP3
    ldx #0
H2DDIV10:
    lda H2NUM,x
    sta TEMP4+1
    inx
    lda H2NUM,x
    sta TEMP4
    inx
    stz TEMP6
H2DLOOP:
    lda TEMP3+1
    cmp TEMP4+1
    bcc  H2DSK1
    bne  H2DSK0
    lda TEMP3
    cmp TEMP4
    bcc  H2DSK1
H2DSK0:
    lda TEMP3
    sec
    sbc TEMP4
    sta TEMP3
    lda TEMP3+1
    sbc TEMP4+1
    sta TEMP3+1
    inc TEMP6
    bra H2DLOOP
H2DSK1:
    lda TEMP6
    clc
    adc #ASCII_0
    sta TEMP1
    stz TEMP1+1
    phx
    jsr spush_0                      ; remember!  A/X both destroyed with push and pull!
    plx
    cpx #8
    beq H2DFIN
    jmp H2DDIV10
H2DFIN:
    lda TEMP3
    clc
    adc #ASCII_0
    sta TEMP1
    stz TEMP1+1
    jsr spush_0
    rts

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
        ;  meet and greet
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
;   Include DEBUG code here
;
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
    jsr   READ_CHAR             ; wait for a key
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
    ldy  #0
COPYLOOP:
    lda (mainoff),y
    sta (ramstart),y
    lda mainoff
    cmp endsoff
    bne SKIP1
    lda mainoff+1
    cmp endsoff+1
    beq CPRTS
SKIP1:
    lda supprint
    bne SKIP01
    PRINT_CHAR #ASCII_PERIOD
SKIP01:
    inc mainoff
    bne SKIP2
    inc mainoff + 1
SKIP2:
    inc ramstart
    bne SKIP3
    inc ramstart+1
SKIP3:
    bra COPYLOOP
CPRTS:
    rts
; ---------------- end of upper.s
;