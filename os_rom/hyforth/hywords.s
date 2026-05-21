;
;HyForth extra commands
;
; hywords.s
;
;----------------------UTILITIES--------------------------------------
def_word "cr", "crlf", 0
    PRINT_CRLF
    jmp next
;
def_word ">in", "inptr", 0                  ; CURBUF - pointer into INBUF (TIB)
    lda #CURBUF
REGIN:
    sta TEMP1
    lda #0
    sta TEMP1+1
    jmp this
;
def_word "last", "last", 0                   ; LASTHEAP addr onto stack
    lda #LASTHEAP
    bra REGIN
;
def_word "here", "here", 0                   ; NEXTHEAP addr onto stack
    lda #NEXTHEAP
    bra REGIN
;
def_word "back", "back", 0                   ; NEXTHEAP addr onto stack
    lda #BACKHEAP
    bra REGIN
;
def_word "sp", "sp", 0                       ; DSPTR pointer addr onto stack
    lda #DSPTR
    bra REGIN
;
def_word "rp", "rp", 0                       ; RTPTR pointer addr onto stack
    lda #RTPTR
    bra REGIN
;
def_word "memptr", "memptr", 0
    lda MEMPTR
    sta TEMP1
    lda MEMPTR+1
    jmp keeps
;
;
; removed 4/24/26 - not used, do var/cons differently
;def_word "allot", "allot", 0
;
;
def_word ">r", "s_to_r", 0
    jsr spull_0   ; put top of stack in TEMP1
    ldy #TEMP1
    jsr rpush
    jmp next
;                             ': r> rp @ @ rp @ 2 + rp ! rp @ @ swap rp @ ! ;'
def_word "r>", "r_to_s", 0
    ldy #TEMP1
    jsr rpull
    jsr spush_0     ; and push on DS stack
    jmp next
;
;
;   BRANCH / ?BRANCH -- neither currently work.  Ugh
;
;
;def_word "bbra", "bbra", 0        ; [IP] = IP  next
;
;def_word "?bra", "qbra", 0      ; POP PSP  0= IF skip ELSE [IP] = IP THEN next
;
;def_word "?nbra", "qnbra", 0      ; POP PSP  0 <> IF skip ELSE [IP] = IP THEN next
;
;
;         Get next byte off INBUF, advance CURBUF
def_word "in>", "intib", 0
    stz TEMP1+1
    ldy #0
    lda (CURBUF),y
    sta TEMP1
    jsr spush_0
    inc CURBUF
    jmp next
;

; ( a -- >[a])   -  : c@ @ ffh and ;  - load a byte
def_word "c@", "c_from", 0
    jsr spull_1
    ldy #0
    lda (TEMP2),y
    sta TEMP1
    stz TEMP1+1
    jmp this
;
; (0b a -- )  0b -> [a]  --- store a byte
def_word "c!", "c_to", 0
    jsr spull_1   ; load address
    jsr spull_0   ; load byte to store (TEMP1 lsb only)
    ldy #0
    lda TEMP1    ; ignore upper byte
    sta (TEMP2),y
    jmp next
;
; ( -- 2)   - : cells lit [ 2 , ] ;
def_word "cells", "cells", 0
    lda #2
    sta TEMP1
    stz TEMP1+1
    jmp this

; ( -- 32)   - : bl lit [ 1 2* 2* 2* 2* 2* , ] ;
def_word "spc", "spc", 0
    lda #ASCII_SPACE
    sta TEMP1
    stz TEMP1+1
    jmp this
;----------------------------------------------
; (? -- ?)      -  LIT -
;                - ': lit rp @ @ dup 2 + rp @ ! @ ;'
;                     OR  - [IP] PUSH DS, IP += 2, next
;
;                    yep, it's this easy...
def_word "lit", "literal", 0
    ldy #0
    lda (INSTPTR),y
    sta TEMP1
    iny
    lda (INSTPTR),y
    sta TEMP1+1
    jsr spush_0
    ldx #INSTPTR  ; 'skip' (IP += 2)
    lda #2
    jsr addwx
    jmp next
;
;
;       MEMORY management:  VAR, CONS, ARRAY, etc.
;
;
;         HF version of 'variable'
;
; ( -- )
def_word "var", "var", 0
    lda NEXTHEAP
    sta BACKHEAP                ; backup NEXTHEAP to BACKHEAP
    lda NEXTHEAP + 1
    sta BACKHEAP + 1

VCHEAD:
; copy LASTHEAP into (NEXTHEAP)
    ldy #LASTHEAP
    jsr comma                    ; change NEXTHEAP to point to LASTHEAP  ('here' <= 'last')
    jsr token                    ; get first token, the name of new word
    ldy #0                       ; copy it to heap: length and name
                                 ; code field comes with later proc
VCLOOP:
    lda (NXTTOK), y
    cmp #ASCII_SPACE                    ; copy in length and name
    beq VCMID
VCCOPY:
    sta (NEXTHEAP), y
    iny
    bne VCLOOP
VCMID:
    tya                          ; and update NEXTHEAP  :  'here' incremented by length
    ldx #(NEXTHEAP)
    jsr addwx                   ; 'here' now at CFA


    ldx #(VCEND0-VCCODE)        ; calc offset
    ldy #0
VCLOOP2:
    lda VCCODE, y
    sta (NEXTHEAP),y
    iny
    dex
    bne VCLOOP2
    phy                        ; save for later
    lda NEXTHEAP+1
    sta TEMP1+1
    lda NEXTHEAP
    clc
    adc #15                     ; calc addresses for indirect data
    sta TEMP1                   ; read.
    bcc VCSKIP00
    inc TEMP1+1
VCSKIP00:                      ; update addresses for lda's
    ldy #3                     ; <VCDATA
    lda TEMP1
    sta (NEXTHEAP),y
    ldy #7                     ; >VCDATA
    lda TEMP1+1
    sta (NEXTHEAP),y
    lda #0
    ldy #15
    sta (NEXTHEAP),y          ; store zero in data word
    iny
    sta (NEXTHEAP),y

    pla                        ; to update
    ldx #NEXTHEAP
    jsr addwx
VCFINISH:
    lda BACKHEAP
    sta LASTHEAP                ; bring back BACKHEAP to LASTHEAP
    lda BACKHEAP + 1
    sta LASTHEAP + 1

    jsr spush_0
    jmp next
;
;    VCCODE - 'dovar' - copied into a var word ( 'DOES>' )
;
VCCODE:         ; NEXTHEAP ('here') should start here
    bit #00
    lda #<VCDATA               ; <NEXTHEAP+15   (at 'here' +3)
    sta TEMP1
    lda #>VCDATA               ; >NEXTHEAP+15   (at 'here' +7)
    sta TEMP1+1
    jsr spush_0
    bra VCEND                  ; bra 2
VCDATA:
   .word 0
VCEND:
    jmp next
VCEND0:
;
;  HF version of 'constant'
;
; ( cv -- )
def_word "cons", "cons", 0
    lda NEXTHEAP
    sta BACKHEAP                ; backup NEXTHEAP to BACKHEAP
    lda NEXTHEAP + 1
    sta BACKHEAP + 1
    ldy #TEMP8
    jsr spull

    jsr DUMPREG

CCHEAD:
; copy LASTHEAP into (NEXTHEAP)
    ldy #LASTHEAP
    jsr comma                    ; change NEXTHEAP to point to LASTHEAP  ('here' <= 'last')
    jsr token                    ; get first token, the name of new word
    ldy #0                       ; copy it to heap: length and name
                                 ; code field comes with later proc
CCLOOP0:
    lda (NXTTOK), y
    cmp #ASCII_SPACE
    beq CCMID
CCCOPY:
    sta (NEXTHEAP), y
    iny
    bne CCLOOP0
CCMID:
    tya                          ; and update NEXTHEAP  :  'here' incremented by length
    ldx #(NEXTHEAP)
    jsr addwx                   ; 'here' now at CFA
                                ; copy in code from CCCODE ('docon')
    ldx #(CCEND0-CCCODE)        ; calc offset
    ldy #0
CCLOOP:
    lda CCCODE, y
    sta (NEXTHEAP),y
    iny
    dex
    bne CCLOOP
    phy
    lda NEXTHEAP+1
    sta TEMP1+1
    lda NEXTHEAP
    clc
    adc #17                     ; calc addresses for indirect data
    sta TEMP1                   ; read.
    bcc CCSKIP00
    inc TEMP1+1
CCSKIP00:
    ldy #3                     ; offset to lda CCDATA
    lda TEMP1
    sta (NEXTHEAP),y
    iny
    lda TEMP1+1
    sta (NEXTHEAP),y
    inc TEMP1
    bne CCSKIP01
    inc TEMP1+1
CCSKIP01:
    ldy #8                     ; offset to lda CCDATA+1
    lda TEMP1
    sta (NEXTHEAP),y
    iny
    lda TEMP1+1
    sta (NEXTHEAP),y
    lda TEMP8                  ; store constant val LSB
    ldy #17
    sta (NEXTHEAP),y
    lda TEMP8+1                ; store constant val MSB
    iny
    sta (NEXTHEAP),y
             ; pull back offset from above
    pla                        ; update NEXTHEAP
    ldx #NEXTHEAP
    jsr addwx
CCFINISH:
    lda BACKHEAP
    sta LASTHEAP                ; bring back BACKHEAP to LASTHEAP
    lda BACKHEAP + 1
    sta LASTHEAP + 1

    ldy #TEMP8
    jsr spush
    jmp next
;
;   cons copies this in and 'customizes it' by
;   plugging in corrected address for CCDATA
;
;  'DOCONS'
CCCODE:
    bit #00
    lda CCDATA
    sta TEMP1
    lda CCDATA+1
    sta TEMP1+1
    jsr spush_0
    bra CCEND                   ; bra 2
CCDATA:
    .word 0                        ; 0 0
CCEND:
    jmp next
CCEND0:

.ifdef SINGLE
;----------------------NUMERALS----------------------------------------
; ( -- n ) numeral 1
def_word "$1", "onehex", 0
    jmp ONEHEX
def_word "1", "one", 0
ONEHEX:
    lda #1
DOWITHONE:
    sta TEMP1
    stz TEMP1+1
;     jsr spush_0
    jmp this

; ( -- n ) numeral 2
def_word "$2", "twohex", 0
    lda #2
    jmp DOWITHONE
def_word "2", "two", 0
    lda #2
    jmp DOWITHONE

; ( -- n ) numeral 3
def_word "$3", "threehex", 0
    lda #3
    jmp DOWITHONE
def_word "3", "three", 0
    lda #3
    jmp DOWITHONE

; ( -- n ) numeral 4
def_word "$4", "fourhex", 0
    lda #4
    jmp DOWITHONE
def_word "4", "four", 0
    lda #4
    jmp DOWITHONE

; ( -- n ) numeral 5
def_word "$5", "fivehex", 0
    lda #5
    jmp DOWITHONE
def_word "5", "five", 0
    lda #5
    jmp DOWITHONE

; ( -- n ) numeral 6
def_word "$6", "sixhex", 0
    lda #6
    jmp DOWITHONE
def_word "6", "six", 0
    lda #6
    jmp DOWITHONE

; ( -- n ) numeral 7
def_word "$7", "sevenhex", 0
    lda #7
    jmp DOWITHONE
def_word "7", "seven", 0
    lda #7
    jmp DOWITHONE

; ( -- n ) numeral 8
def_word "$8", "eighthex", 0
    lda #8
    jmp DOWITHONE
def_word "8", "eight", 0
    lda #8
    jmp DOWITHONE

; ( -- n ) numeral 9
def_word "$9", "ninehex", 0
    lda #9
    jmp DOWITHONE
def_word "9", "nine", 0
    lda #9
    jmp DOWITHONE

; ( -- n ) numeral -1
def_word "-1", "negone", 0
    lda #$FF
    sta TEMP1
DOWITHNEG:
    ; sta TEMP1+1
    jmp keeps

; ( -- n ) numeral 0
def_word "$0", "zerohex", 0
    jmp DOITZERO
def_word "0", "zero", 0
DOITZERO:
    stz TEMP1
    stz TEMP1+1
    jmp this

; ( -- n ) numeral -2
def_word "-2", "negtwo", 0
    lda #$FE
    sta TEMP1
    jmp DOWITHNEG

; ( -- n ) numeral -3
def_word "-3", "negthree", 0
    lda #$FD
    sta TEMP1
    jmp DOWITHNEG

def_word "-4", "negfour", 0
; ( -- n ) numeral -4
    lda #$FC
    sta TEMP1
    jmp DOWITHNEG

def_word "-5", "negfive", 0
; ( -- n ) numeral -5
    lda #$FB
    sta TEMP1
    jmp DOWITHNEG

def_word "-6", "negsix", 0
; ( -- n ) numeral -6
    lda #$FA
    sta TEMP1
    jmp DOWITHNEG

def_word "-7", "negseven", 0
; ( -- n ) numeral -7
    lda #$F9
    sta TEMP1
    jmp DOWITHNEG

def_word "-8", "negeight", 0
; ( -- n ) numeral -8
    lda #$F8
    sta TEMP1
    jmp DOWITHNEG

def_word "-9", "negnine", 0
; ( -- n ) numeral -9
    lda #$F7
    sta TEMP1
    jmp DOWITHNEG

; ( -- n ) numeral A
def_word "$A", "zeroa", 0
    lda #10
    jmp DOWITHONE

; ( -- n ) numeral B
def_word "$B", "zerob", 0
    lda #11
    jmp DOWITHONE

; ( -- n ) numeral C
def_word "$C", "zeroc", 0
    lda #12
    jmp DOWITHONE

; ( -- n ) numeral D
def_word "$D", "zerod", 0
    lda #13
    jmp DOWITHONE

; ( -- n ) numeral E
def_word "$E", "zeroe", 0
    lda #14
    jmp DOWITHONE

; ( -- n ) numeral F
def_word "$F", "zerof", 0
    lda #15
    jmp DOWITHONE

; ( -- n ) numeral %0
def_word "%0", "binary0", 0
    lda #0
    jmp DOWITHONE

; ( -- n ) numeral %1
def_word "%1", "binary1", 0
    lda #1
    jmp DOWITHONE
.endif   ; SINGLE

;----------------------LOGIC FUNCTIONS--------------------------
; ( w1 w2 -- w1 AND w2 )
def_word "and", "xand", 0   ; had to use 'xand' cuz 'and' used for something important...
    jsr spull_1             ; load TEMP2, TEMP1 from stack
    jsr spull_0
    lda TEMP2
    and TEMP1
    sta TEMP1
    lda TEMP2 + 1
    and TEMP1 + 1
    jmp keeps  ; uncomment if carry could be set
;
; ( w1 w2 -- w1 OR w2 )
def_word "or", "or", 0
    jsr spull_1             ; load TEMP2, TEMP1 from stack
    jsr spull_0
    lda TEMP2
    ora TEMP1
    sta TEMP1
    lda TEMP2 + 1
    ora TEMP1 + 1
    jmp keeps  ; sta TEMP1+1 included!
;
; ( w1 w2 -- w1 XOR w2 )
def_word "xor", "xor", 0
    jsr spull_1             ;load TEMP2, TEMP1 from stack
    jsr spull_0
    lda TEMP2
    eor TEMP1
    sta TEMP1
    lda TEMP2 + 1
    eor TEMP1 + 1
    jmp keeps  ; sta TEMP1+1 included!
;
; ( w1 -- NOT w1 )
def_word "not", "not", 0
    jsr spull_0             ; load TEMP1 from stack
    lda TEMP1
    eor #$FF
    sta TEMP1
    lda TEMP1+1
    eor #$FF
    jmp keeps  ; sta TEMP1+1 included!
;
; ( w1 -- NEG w1 )  1's complement
def_word "neg", "neg", 0
    jsr spull_0             ; load TEMP1 from stack
    lda TEMP1
    eor #$FF
    sta TEMP1
    lda TEMP1+1
    eor #$FF
    inc TEMP1
    bne NEGENDS
    inc TEMP1+1
NEGENDS:
    jmp next
;
; (n1 n2 -- n1<>n2)  not equal
def_word "<>", "ne", 0
    jsr spull_1
    jsr spull_0
CMPLINK:
    lda TEMP1
    cmp TEMP2
    bne IEQTRUE
    lda TEMP1+1
    cmp TEMP2+1
    beq IEQFALSE
    jmp PUSHTRUE
;
; (n1 n2 -- n1=n2)  equal
def_word "=", "eq", 0
    jsr spull_1
    jsr spull_0
    lda TEMP1
    cmp TEMP2
    bne IEQFALSE
    lda TEMP1+1
    cmp TEMP2+1
    bne IEQFALSE
    jmp PUSHTRUE
;
; (n1 -- n1=0)  equal to zero
def_word "0=", "eqz", 0
    jsr spull_0
    lda TEMP1
    ora TEMP1+1       ; only zero if both zero
    bne IEQFALSE
    jmp PUSHTRUE
;

; (n1 n2 -- n1>n2)  more than
def_word ">", "gt", 0
    jsr spull_1
    jsr spull_0
    lda TEMP1+1
    cmp TEMP2+1
    bcc IEQFALSE
    bne IEQTRUE
    lda TEMP1
    cmp TEMP2
    bcc IEQFALSE
    beq IEQFALSE
IEQTRUE:
    jmp PUSHTRUE
IEQFALSE:
    jmp PUSHFALSE

;
; (n1 n2 == n1<=n2) less than or equal
def_word "<=", "lte", 0
    jsr spull_1
    jsr spull_0
    lda TEMP1+1
    cmp TEMP2+1
    bcc IEQTRUE
    bne IEQFALSE
    lda TEMP1
    cmp TEMP2
    bcc IEQTRUE
    beq IEQTRUE
    jmp PUSHFALSE

;
; (n1 n2 == n1>=n2) greater than or equal
def_word ">=", "gte", 0
    jsr spull_1
    jsr spull_0
GTELINK:
    lda TEMP1+1
    cmp TEMP2+1
    bcc IEQFALSE
    bne IEQTRUE
    lda TEMP1
    cmp TEMP2
    bcc IEQFALSE
    jmp PUSHTRUE
;
; (n1 n2 == n1<n2) less than
def_word "<", "lt", 0
    jsr spull_1
    jsr spull_0
    lda TEMP1+1
    cmp TEMP2+1
    bcc IEQTRUE
    bne IEQFALSE
    lda TEMP1
    cmp TEMP2
    bcc IEQTRUE
    jmp PUSHFALSE
;

;----------------------CLASSIC FORTH-------------------------
; (a b -- b a)
def_word "swap", "swap", 0
    jsr spull_0          ; pull top
    lda TEMP1
    sta TEMP2
    lda TEMP1+1
    sta TEMP2+1
    jsr spull_0          ; pull next, will end up in TEMP1
    jsr spush_1          ; push TEMP2 back on now
    jmp this            ; includes jsr spush_0
;
;  (a -- a a)
def_word "dup", "dup", 0
    jsr spull_0          ; pull top to TEMP1
    jsr spush_0          ; and then push back
    jmp this            ; includes jsr spush_0 for second time
;
; (a -- )
def_word "drop", "drop", 0
    jsr spull_0          ; pull top, ends up in TEMP1 but...
    jmp next
;
; (a a a ... -- )    clear DS
def_word "xS", "xS", 0
    lda #DSEND
    sta DSPTR
    lda #0
    ldy #0
    sta (DSPTR),y
    iny
    sta (DSPTR),y
    jmp next
;
; (RT:a a a ... -- RT: )    clear RT
def_word "xR", "xR", 0
    lda #RTEND
    sta RTPTR
    lda #0
    ldy #0
    sta (RTPTR),y
    iny
    sta (RTPTR),y
    jmp next
;
; (a b c -- b c a)
def_word "rot", "rot", 0
    jsr spull_0
    jsr spull_1
    jsr spull_2
    jsr spush_1
    jsr spush_0
    jsr spush_2
    jmp next
;
; (a b -- a b a)
def_word "over", "over", 0
    jsr spull_0
    jsr spull_1
    jsr spush_1
    jsr spush_0
    jsr spush_1
    jmp next
;
; (a b -- b)
def_word "nip", "nip", 0
    jsr spull_0
    jsr spull_1
    jsr spush_0
    jmp next
;
; (a b -- b a b)
def_word "tuck", "tuck", 0
    jsr spull_0
    jsr spull_1
    jsr spush_0
    jsr spush_1
    jsr spush_0
    jmp next
;
; (a b -- a b a b)
def_word "2dup", "dup2", 0
    jsr spull_1          ; pull top to TEMP2
    jsr spull_0          ; then TEMP1
    jsr spush_0          ; and then push them back
    jsr spush_1
    jsr spush_0
    jsr spush_1
    jmp next
;
; (a b -- )
def_word "2drop", "drop2", 0
    jsr spull_0
    jsr spull_0
    jmp next
;
; (a b c d -- a b c d a b)
def_word "2over", "over2", 0
    ldy #TEMP9
    jsr spull     ; to TEMP9 'd'
    jsr spull_2  ; TEMP3 'c'
    jsr spull_1  ; TEMP2 'b'
    jsr spull_0  ; TEMP1 'a'
    jsr spush_0  ;  'a'
    jsr spush_1  ;  'b'
    jsr spush_2  ;  'c'
    ldy #TEMP9
    jsr spush    ; 'd'
    jsr spush_0  ;  'a' again
    jsr spush_1  ;  'b' again
    jmp next
;
def_word "pick", "xpick", 0
    jsr spull_0
    lda TEMP1
    asl
    clc
    adc DSPTR
    sta TEMP8
    cmp #DSEND           ; boundary check
    bcs PICKPTRERR
    MOV DSPTR+1, TEMP8+1
    MOV (TEMP8), TEMP1
    ldy #1
    MOV {(TEMP8),y}, TEMP1+1
    jsr spush_0
    jmp next
PICKPTRERR:
    lda #ERR_SPTR
    sta ERRFLAG
    jmp errrtn
;
def_word "snip", "snip", 0
    lda DSPTR
    cmp #DSEND
    beq SNIPERR
    MOV DSPTR+1, TEMP2+1
    sta TEMP1+1
    lda #DSEND
    sta TEMP2
SNIPLOOP:
    sec
    sbc #2
    sta TEMP1
    ldy #0
    MOVAY16 (TEMP1), (TEMP2)
    MOV16_HL TEMP1, TEMP2
    cmp #DSEND
    bne SNIPLOOP
SNIPEND:
    ldx #DSPTR
    lda #2
    jsr addwx
    jmp next
SNIPERR:
    lda #ERR_SPTR
    sta ERRFLAG
    jmp errrtn
;
;-------------------------AUTOLOAD and CLOAD----------------------
CLOADMSG:
   .byte "cload>"
   .byte 0
BLOADMSG:
   .byte "bload:"
   .byte ASCII_CR, ASCII_LF, 0
;
; ( -- )     autoload a list of scripts
def_word "autoload", "autoload", 0
    jsr spull_0   ; get end of list
    lda TEMP1
    sta TEMP7
    lda TEMP1+1
    sta TEMP7+1
    lda #1
    sta ALFLAG
    jmp next

; (a -- )       load a compile-able script from memory, zero term'd
def_word "cload", "cload", 0
    ;stz ALFLAG         ; clear autoload flag
CLOAD_IN:
    WSEQ_raw CLOADMSG
    jsr spull_0  ; address from stack to TEMP1
    ldy #0
    lda #ASCII_SPACE
    sta (TIB), y   ; put space at start
CLOAD_LP:
    lda (TEMP1), y
    beq CLOAD_CONT     ; stops at first 0
    PRINT_CHAR     ; echo char out so can see what's being pulled in
    iny           ; yep, skip
    sta (TIB), y
    bra CLOAD_LP
CLOAD_CONT:
    iny
    lda #ASCII_SPACE
    sta (TIB),y     ; not sure but won't hurt to add a space
    PRINT_CRLF
    lda #ASCII_SPACE
    phy
    ldy #0
    sta (TIB), y       ; start with space
    ply
    sta (TIB), y        ; ends with space
    lda #0            ; mark eol with 0
    iny
    sta (TIB), y
    dey
; start it
    sta CURBUF
    tya                ; calc next address if want to load more
    clc
    adc TEMP1
    sta TEMP1
    bcc CLSKIP
    inc TEMP1+1
CLSKIP:
    jsr spush_0        ; push next addr on stack if wanted
.ifdef DEBUG
    jsr DUMPREG
.endif
    jsr token            ; massage the buffer, oh yeah
    jmp RESFIND           ; works perfectly!
;
;
;  BLOAD  -- see example in 'bload.s'
;
;   note, only loads ONE word at the moment
;
; (a -- )       load native code from memory, zero term'd
def_word "bload", "bload", 0
BLOAD_IN:
    WSEQ_raw BLOADMSG
    jsr spull_0          ; address from stack to TEMP1

BLAGAIN:
    lda NEXTHEAP
    sta BACKHEAP                ; backup NEXTHEAP to BACKHEAP
    lda NEXTHEAP + 1
    sta BACKHEAP + 1

BLHEAD:
    ldy #LASTHEAP
    jsr comma                    ; change NEXTHEAP to point to LASTHEAP  ('here' <= 'last')
    ldy #0                       ; copy it to heap: length and name
                                 ; code field comes with later proc
                                 ;
                                 ; NOTE:  This cannot pull more than 255 bytes including
                                 ;  leading or trailing zeros!
                                 ;
BLZEROSKIP:
    lda (TEMP1), y
    bne BLNSK1                  ; skip any LEADING zeros
    iny
    bra BLZEROSKIP
BLNSK1:
    tya
    ldx #TEMP1
    jsr addwx
    ldy #0
BLNLOOP:
    lda (TEMP1), y
    bne BLCOPY
    iny
    lda (TEMP1), y
    beq BLSKIP0
    dey
    lda #0
    bra BLCOPY
BLSKIP0:
    dey
    dey
    bra BLEND
BLCOPY:
    sta (NEXTHEAP), y
    iny
    bne BLNLOOP
BLEND:
    iny
    tya                          ; and update NEXTHEAP  :  'here' incremented by length
    ldx #NEXTHEAP
    jsr addwx
    iny
    iny
    tya
    ldx #TEMP1
    jsr addwx
BLFINISH:
    lda BACKHEAP
    sta LASTHEAP                ; bring back BACKHEAP to LASTHEAP
    lda BACKHEAP + 1
    sta LASTHEAP + 1
    jsr BLTESTEND
    bcs BLENDEND
    jmp BLAGAIN                  ; next word
;    jsr spush_0                 ; push next/last address?
BLENDEND:
    jmp next

BLTESTEND:
    phy
    ldy #0
    lda (TEMP1), y
    cmp #'E'
    bne BLNOEND
    iny
    lda (TEMP1), y
    cmp #'N'
    bne BLNOEND
    iny
    lda (TEMP1), y
    cmp #'D'
    bne BLNOEND
    sec
    bcs BLTEND
BLNOEND:
    clc
BLTEND:
    ply
    rts
;
;
;
; ( -- )   ANSI clear screen
.ifdef ANSIOK
def_word "Acls", "Acls", 0
    jsr CLEAR_SCR
    jmp next
;
; (c r -- )      ANSI screen position ESC[<r>;<c>f
def_word "Ascr", "Ascr", 0
    PRINT_ANSI_ESC_SEQ
    jsr spull_0
    ldx TEMP1
    jsr DEC2ASCII
    lda TEMP3+1
    cmp #ASCII_0
    beq ASCRNXT1
    PRINT_CHAR
ASCRNXT1:
    PRINT_CHAR TEMP3, #ASCII_SEMI
    jsr spull_0
    ldx TEMP1
    jsr DEC2ASCII
    lda TEMP3+1
    cmp #ASCII_0
    beq ASCRNXT2
    PRINT_CHAR
ASCRNXT2:
    PRINT_CHAR TEMP3, #ASCII_f
    jmp next
;
; (c -- )      ANSI attributes ESC[<c>m
def_word "Acol", "Acol", 0
    PRINT_ANSI_ESC_SEQ
    jsr spull_0
    ldx TEMP1
    jsr DEC2ASCII
    lda TEMP3+1
    cmp #ASCII_0
    beq ACOLNXT2
    PRINT_CHAR
ACOLNXT2:
    PRINT_CHAR TEMP3, #ASCII_m
    jmp next
.endif   ; ANSIOK

;
;--------------------------RANDOM #'s
; ( s s -- )   random #
def_word "rseed", "rseed", 0
    jsr spull_0
    lda TEMP1
    sta RSEED
    lda TEMP1+1
    sta RSEED+1
    jsr spull_0
    lda TEMP1
    sta RSEED+2
    lda TEMP1+1
    sta RSEED+3
    jmp next
; ( -- r)          16 bit rand -> stack
;
def_word "rand", "rand", 0
    jsr galois32o
    jmp RAND32IN
;
; ( -- r r)          32 bit rand -> stack
def_word "rand32", "rand32", 0
    jsr galois32o
    lda RSEED+2
    sta TEMP1
    lda RSEED+3
    sta TEMP1+1
    jsr spush_0
RAND32IN:
    lda RSEED
    sta TEMP1
    lda RSEED+1
    sta TEMP1+1
    jsr spush_0
    jmp next
;
; (u1 u2 -- ux um)  min
; (u1 u2 -- um ux)  max
def_word "min", "min16", 0
    jsr spull_1
    jsr spull_0
MININ16:
    lda TEMP1+1
    cmp TEMP2+1
    bcc MINSWAP
    lda TEMP1
    cmp TEMP2
    bcc MINSWAP
    jmp MINDONE
MINSWAP:
    jsr spush_1
    jsr spush_0
    jmp next
MINDONE:
    jsr spush_0
    jsr spush_1
    jmp next

def_word "max", "max16", 0
    jsr spull_1
    jsr spull_0
    lda TEMP1+1
    cmp TEMP2+1
    bcc MINDONE
    lda TEMP1
    cmp TEMP2
    bcc MINDONE
    jmp MINSWAP

;
; (ux um -- rh rl)      16x16 multiply, result in reverse order
; faster if TEMP2 (um) is smaller of two.  Need a MIN/MAX swap routine!
;
def_word "*", "mult16", 0
    jsr spull_1
    jsr spull_0
    lda TEMP1            ; handle zeros  3/22 1320
    ora TEMP1+1
    beq m16zero          ; if either is 0-0, can skip to end
    lda TEMP2
    ora TEMP2+1
    beq m16zero          ; TEMP2 is zero?  same deal.
    jsr MULT16
    bra m16push
m16zero:
    stz TEMP1
    stz TEMP1+1
    stz TEMP3
    stz TEMP3+1
m16push:
    jsr spush_2           ; MSbyte
    jsr spush_0           ; push LSbyte in TEMP1 on top
    jmp next

;
; (ux um -- r m)      16x16 divide, result + MOD
;
def_word "/", "div16", 0
    jsr spull_1
    jsr spull_0
    lda TEMP1            ; handle zeros  3/22 1320
    ora TEMP1+1
    beq d16zero         ; shortcut!
    lda TEMP2
    bne d16skip0
    ora TEMP2+1
    bne d16skip1
    jmp div0err                     ; divide by zero error
d16skip0:
    cmp #1
    bne d16skip1
    lda TEMP2+1
    bne d16skip1
    bra d16rzero                   ; dividing by 1, just return TEMP1
d16skip1:
    jsr DIV16                      ; results TEMP2, remainder TEMP3
    jmp d16done
d16zero:
    stz TEMP1
    stz TEMP1+1
d16rzero:
    stz TEMP3
    stz TEMP3+1
d16done:
    jsr spush_2        ; remainder first
    jsr spush_0        ; result on top
    jmp next
;
;                      divide by zero error
div0err:                      ; pop jsr off stack, throw error
    lda #ERR_DIV0
    sta ERRFLAG
    jmp errrtn
;
; ( hex -- d1 d2 d3 d4 d5 )
def_word "xdrv", "xdrv", 0
    jsr spull_0
    lda TEMP1
    ldy TEMP1+1
    jsr HEX2DEC
    jmp next
;
;------------------------------MEMORY OPERATIONS----------------------------
;
; (as ae ad -- )    copy from $as thru $ae to $ad
def_word "memcpy", "memcpy", 0
    jsr spull_2              ; dest addr
    jsr spull_1              ; end
    jsr spull_0              ; start
    lda TEMP1+1
    cmp TEMP2+1
    bcc MEMCPYDOIT
    bne MEMCPYEND
    lda TEMP1
    cmp TEMP2
    bcs MEMCPYEND
MEMCPYDOIT:
    lda #1
    sta supprint
    jsr MEMCPY
    stz supprint
MEMCPYEND:
    jmp next
;
;                   free memory between BACKHEAP and MEMPTR
def_word "free", "free", 0
    lda MEMLAST
    sec
    sbc NEXTHEAP
    sta TEMP1
    lda MEMLAST+1
    sbc NEXTHEAP+1
    sta TEMP1+1
    jsr spush_0
    jmp next
;
; (daddr n -- ) start disassembly from daddr, do it
; $F600 is entry point --A+Y for starting address, C=1 for multiple opcodes, X for # of codes
def_word "disasm", "disasm", 0
    jsr spull_1    ; # of instructions (??)
    jsr spull_0    ; addr
    ldx TEMP2
    cpx #$FE
    bcs DISEND
    lda #1           ; and ZP_D_STATE has to be =1 in order to show mnemonics etc
    sta ZP_D_STATE
    lda TEMP1
    ldy TEMP1+1
    sec              ; set the carry to make sure multiple ops returned
    jsr DISASM_AY
    lda ZP_D_XAM
    sta TEMP1
    ldy ZP_D_XAM+1
    sty TEMP1+1
    jsr spush_0     ; push last address on stack?
DISEND:
    jmp next

; (jsaddr 0a 0y-- 0x) jump to external code with parms passed via A,Y, result in X
def_word "syscall", "syscall", 0
    jsr spull_2    ; parm to pass to y
    jsr spull_1    ; parm to pass to a
    jsr spull_0    ; addr
    lda TEMP1
    sta SYSCALL+1
    lda TEMP1+1
    sta SYSCALL+2
    ldy TEMP3
    lda TEMP2
    jsr SYSCALL
    jmp next
;
;  below must be in RAM to work
;
SYSCALL:
    jsr SCDUMMY        ; store into SYSCALL+1, +2 to customize jump
    txa                ; returned stuff in X
    beq SCSKIP         ; if returns zero, don't do anything else
    sta TEMP1          ; otherwise...
    stz TEMP1+1
    jsr spush_0        ; push result onto stack
    rts
SCSKIP:
    pla
    pla
    jmp errrtn
SCDUMMY:
    ldx #0
    rts
;
;
; ( paddr -- )
def_word "mktemp", "mktemp", 0
    jsr spull_1       ; get mptr in TEMP2
    jsr MEMLEN        ; len in TEMP1, maddr in TEMP3
    ldy #0
    lda (TEMP3),y
    ora #$80          ; set high bit on type byte
    sta (TEMP3),y
    jmp next
;
;   ( -- )  --- 'deallocate' last memory item in stack if marked 'temp'
;
def_word "purge0", "purge0", 0
    lda MEMPTR
    sta TEMP2
    lda MEMPTR+1
    sta TEMP2+1
    cmp #>(MEMSTK+MEMEND)
    bcc PURGECONT
    bne PURGEEND
    lda TEMP2
    cmp #<(MEMSTK+MEMEND)
    bcs PURGEEND
PURGECONT:
    lda TEMP2
    clc
    adc #2
    bcc  PURGESK00
    inc TEMP2+1
PURGESK00:
    jsr MEMLEN        ; len in TEMP1, maddr in TEMP3
    ldy #0
    lda (TEMP3),y
    and #$80          ; mask off all but temp bit
    beq  PURGEEND
    lda TEMP2         ; change pointers if temp
    sta MEMPTR
    lda TEMP2+1
    sta MEMPTR+1
    lda TEMP3
;    clc
;    adc #3
;    bcc  PURGESK01
;    inc TEMP3+1
;PURGESK01:
;    clc
;    adc TEMP1
    sta MEMLAST
    lda TEMP3+1
;    adc TEMP1+1
    sta MEMLAST+1
PURGEEND:
    jmp next
;
;
; ( bytes type -- staddr )
def_word "malloc", "malloc", 0
    jsr spull_1       ; type ( word ($00), char ($01), words ($02), bytes ($03), sz ($04) ..)
    jsr spull_0       ; # bytes
    jsr MALLOC        ; will return address in TEMP1
    jsr spush_0       ; push ptr address to new record on stack
    jmp next

;
; ( maddr -- len )
def_word "mlen", "mlen", 0
    jsr spull_1
    jsr MEMLEN   ; returns length in TEMP1, maddr in TEMP3
    jsr spush_0
    jmp next
;
;
.ifdef YSOUND
;
;                        word definitions for Yamaha 2151 sound chip
;
def_word "sndinit", "sndinit", 0
    jsr SOUND_INIT
    jmp next

def_word "sndtest", "sndtest", 0
    jsr SOUND_TEST
    jmp next

; ( xxaa -- )    send byte(a) to register(x) on yamaha 2151
def_word "ywrite", "ywrite", 0
    jsr spull_0
    ldx TEMP1+1
    lda TEMP1
    jsr YM_WRITE
    bcc YMGOOD
    jmp PUSHFALSE
YMGOOD:
    jmp PUSHTRUE
.endif
;
;
;----------------------------------------------------------------------------
HYWORDS_END:
;  end hywords.s
;----------------------------------------------------------------------------