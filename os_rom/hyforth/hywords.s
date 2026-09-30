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
    jsr DICTCHK                 ; room for the dictionary to grow?
    bcs VARROOM
    jmp DICTFULL
VARROOM:
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
    jsr DICTCHK                 ; room for the dictionary to grow?
    bcs CONSROOM
    jmp DICTFULL
CONSROOM:
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
    lda #$FF                        ; high byte (keeps stores .A there)
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
; ( -- addr )  the built-in training scripts, for autoload:  ftrain autoload
;   They're in the paged ROM only: the RAM copy (COPYTORAM) stops at 'ends', and the dictionary grows
;   over the rest.  (Their RAM address holds whatever the dictionary or old RAM has there.)
def_word "ftrain", "ftrain", 0
    lda #<(ftrain_0 - RAMSTART + COPYSTART)
    sta TEMP1
    lda #>(ftrain_0 - RAMSTART + COPYSTART)
    jmp keeps
;
; ( -- addr )  the built-in bload test library (BLtest, BLtest2), in the paged ROM:  bltest bload
def_word "bltest", "bltest", 0
    lda #<(test00 - RAMSTART + COPYSTART)
    sta TEMP1
    lda #>(test00 - RAMSTART + COPYSTART)
    jmp keeps
;
; ( addr -- )     autoload a list of scripts (zero-terminated lines, then an empty one), e.g. ftrain
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
    jsr DICTCHK                 ; room for the dictionary to grow?
    bcs BLROOM
    jmp DICTFULL
BLROOM:
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
    lda #0                     ; room between 'here' and the lowest page the MMU has allocated
    sec
    sbc NEXTHEAP
    sta TEMP1
    lda MMU_LOW_WATER
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
    sta TEMP2         ; TEMP2 = the last record's slot
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
    lda (TEMP3)       ; large record (its own MMU block)?
    and #MEM_MMU
    beq PURGEARENA
    jsr MMUFREE       ; free its block; the arena (MEMLAST) is unchanged
    bra PURGEEND
PURGEARENA:
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
    bcs MALLOCOK
    lda #ERR_MEM      ; out of memory
    sta ERRFLAG
    jmp errrtn
MALLOCOK:
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
;-------- MMU handles (see docs/plans/MMU_PLAN.md)
;
; ( bytes flags -- h )  allocate MMU memory; flags 0 = task RAM, 1 = 8K RAM banks (AI_PAGED)
def_word "halloc", "halloc", 0
    jsr spull_1       ; flags
    jsr spull_0       ; bytes
    lda TEMP1
    ldy TEMP1+1
    ldx TEMP2
    jsr MM_ALLOC      ; .A = handle
    bcs HMERR
    sta TEMP1
    stz TEMP1+1
    jsr spush_0
    jmp next
HMERR:
    lda #ERR_MEM      ; out of memory (or a bad handle)
    sta ERRFLAG
    jmp errrtn
;
; ( h -- )  free MMU memory
def_word "hfree", "hfree", 0
    jsr spull_0
    lda TEMP1
    jsr MM_FREE
    bcs HMERR
    jmp next
;
; ( h -- addr )  raw address of MMU memory (8K RAM bank allocations: selects the bank at $8000);
;               can't be freed until hunlock.  One hlock at a time.
def_word "hlock", "hlock", 0
    jsr spull_0
    lda TEMP1
    jsr MM_LOCK       ; .A.Y = address, .X = previous RAM bank
    bcs HMERR
    stx HLBANK
    sta TEMP1
    sty TEMP1+1
    jsr spush_0
    jmp next
;
; ( h -- )  undo hlock (restores the RAM bank)
def_word "hunlock", "hunlock", 0
    jsr spull_0
    lda TEMP1
    ldx HLBANK
    jsr MM_UNLOCK
    bcs HMERR
    jmp next
;
; ( -- )  run the MMU self test
def_word "mmtest", "mmtest", 0
    jsr MMU_TEST
    jmp next
;
;-------- IO: files (see docs/plans/IO_PLAN.md).  A failed call gives !IO ERR!, and 'ioerr' the IO layer's
;         error code ($70 not found, $71 bad fd, $72 wrong mode, $73 would block, $75 no fds, ...).
;         fds 0, 1, 2 are the console (key, emit); buffers must be in task RAM ($0000-$7FFF).
;
; ( sz mode -- fd )  open a file: sz = a q^...^ string, e.g. q^/dev/cons^; mode 1 = read, 2 = write,
;                   3 = both, + $80 = don't wait (reads and writes give ioerr $73 instead)
def_word "open", "open", 0
    jsr spull_1       ; mode
    jsr spull_0       ; the string
    ldy #0
    lda (TEMP1),y
    sta TEMP3
    iny
    lda (TEMP1),y
    sta TEMP3+1       ; TEMP3 = the record
    lda (TEMP3)
    and #$7F          ; (the temp flag)
    cmp #MEM_SZ
    beq IOPENSZ
    lda #ERR_IO_NAME  ; not a string
    bra IOFAIL
IOPENSZ:
    lda TEMP3         ; its text, after the 3-byte header
    clc
    adc #3
    pha
    lda TEMP3+1
    adc #0
    tay
    pla
    ldx TEMP2
    jsr IO_OPEN       ; .A = fd
    bcs IOFAIL
IOPUSHA:
    sta TEMP1
    stz TEMP1+1
    jmp this
IOFAIL:
    sta IOERR
    lda #ERR_IO
    sta ERRFLAG
    jmp errrtn
;
; ( fd -- )  close a file
def_word "close", "close", 0
    jsr spull_0
    lda TEMP1
    jsr IO_CLOSE
    bcs IOFAIL
    jmp next
;
; ( fd addr n -- n' )  read up to n bytes into addr; n' = bytes read (0 = end of file).  Waits for data
;                     (the console: at least one key), unless the fd was opened with $80.
def_word "read", "read", 0
    jsr IOARGS
    jsr IO_READ
IODONE:
    bcs IOFAIL
    lda ZP_IO_CNT
    sta TEMP1
    lda ZP_IO_CNT+1
    jmp keeps
;
; ( fd addr n -- n' )  write n bytes from addr; n' = bytes written
def_word "write", "write", 0
    jsr IOARGS
    jsr IO_WRITE
    bra IODONE
;
; ( fd lo hi -- )  set fd's offset for the next read or write (32 bits: hi * 65536 + lo), e.g. in /dev/sd
def_word "seek", "seek", 0
    jsr spull_2       ; hi
    jsr spull_1       ; lo
    jsr spull_0       ; fd
    lda TEMP2
    sta ZP_IO_OFS
    lda TEMP2+1
    sta ZP_IO_OFS+1
    lda TEMP3
    sta ZP_IO_OFS+2
    lda TEMP3+1
    sta ZP_IO_OFS+3
    lda TEMP1
    jsr IO_SEEK
    bcs SEEKFAIL
    jmp next
SEEKFAIL:
    jmp IOFAIL
;
; ( fd code arg -- )  device control, e.g. fd 1 task ioctl: make task the foreground task (the
;                    console's input goes to it)
def_word "ioctl", "ioctl", 0
    jsr spull_2       ; arg
    jsr spull_1       ; code
    jsr spull_0       ; fd
    lda TEMP1
    ldx TEMP2
    ldy TEMP3
    jsr IO_CTL
    bcs SEEKFAIL      ; (IOFAIL, in reach)
    jmp next
;
; ( fd newfd -- )  make newfd refer to the same file as fd (closing newfd first), e.g. fd 1 fdup2
;                  sends emit's output to fd's file
def_word "fdup2", "fdup2", 0
    jsr spull_1       ; newfd
    jsr spull_0       ; fd
    lda TEMP1
    ldx TEMP2
    jsr IO_DUP2
    bcs IOFAIL2
    jmp next
IOFAIL2:
    jmp IOFAIL
;
; ( -- rfd wfd )  make a pipe: what's written to wfd can be read from rfd
def_word "pipe", "pipe", 0
    jsr IO_PIPE       ; .A = read fd, .X = write fd
    bcs IOFAIL2
    sta TEMP1
    stz TEMP1+1
    stx TEMP2
    jsr spush_0
    lda TEMP2
    jmp IOPUSHA
;
;-------- Files on the SD cards (HydraFS, at /sd/N: e.g. q^/sd/0/games^ ls)
;
; ( sz mode -- fd )  create a file (mode 0; $40 append-only, $01 read-only) or a directory ($80), and open
;                   it: a file for reading and writing, a directory for reading.  A file that's there already
;                   is emptied (e.g. q^/sd/0/notes^ 0 create)
def_word "create", "create", 0
    jsr spull_1       ; the mode
    jsr spull_0       ; the name
    ldx #TEMP1
    jsr SZTEXT        ; .A.Y = its text
    bcs FSFAIL
    ldx TEMP2
    stx ZP_IO_BUF     ; (IO_CREATE: the new file's mode)
    ldx #IO_MODE_RDWR
    jsr IO_CREATE
    bcs FSFAIL
    jmp IOPUSHA
FSFAIL:
    jmp IOFAIL
;
;-------- The shell: the current directory, and commands that take their arguments from the line
;   A parsing word (cd games) takes the words after it on the line (ARGGET); its stack form, for
;   definitions, takes q^...^ strings: (cd).  The work is done on BIOS page 7 (shell/shell.s).
;
; cd [dir]  change directory (relative, or not; ".." understood); cd alone: the current card's root
def_word "cd", "cd", 0
    jsr ARGGET          ; .A.Y = the path; C = 1: none
    bcc CDGO
    ldy #0              ; (SH_CD: .Y = 0, no path)
CDGO:
    jsr SH_CD
    bcs SHFAIL
    jmp next
;
; ( sz -- )  change directory
def_word "(cd)", "pcd", 0
    jsr SHARG1          ; .A.Y = the q^...^ string's text
    bcc CDGO
SHFAIL:
    jmp IOFAIL
;
; pwd  show the current directory
def_word "pwd", "pwd", 0
    jsr SH_PWD
    jmp next
;
; ( sz -- )  set the prompt's format: %v the volume ("0:"), %d the directory on the card (or the whole
;           path off the cards), %p the whole path, %l the card's label, %t the task, %% a %
;           (default: "%v%d> " prompt)
def_word "prompt", "prompt", 0
    jsr SHARG1
    bcs SHFAIL
    sta TEMP3
    sty TEMP3+1
    ldy #0
PROMPTCP:
    lda (TEMP3),y
    sta PROMPTFMT,y
    beq PROMPTDONE
    iny
    cpy #PROMPTFMT_SIZE - 1
    bne PROMPTCP
    lda #0
    sta PROMPTFMT,y
PROMPTDONE:
    jmp next
;
; ( sz -- ) -> .A.Y = its text, C = 0; or .A = ERR_IO_NAME, C = 1 (not a string)
SHARG1:
    jsr spull_0
    ldx #TEMP1
    jmp SZTEXT
;
; A parsing word's argument: the next word on the line (up to a space, or the line's end), or "a name in
; quotes" (spaces and all), into ARGBUF, zero-terminated; the interpreter goes on after it.
; OUT: C = 0: .A.Y = ARGBUF; or C = 1: none.  Uses TEMP6
ARGGET:
    ldy CURBUF
AGSKIP:
    lda (TIB),y         ; (spaces before it)
    beq AGNONE
    cmp #ASCII_SPACE
    bne AGWORD
    iny
    bra AGSKIP
AGWORD:
    ldx #ASCII_SPACE    ; (what ends it: a space; or in quotes, the closing '"')
    cmp #ASCII_DQUOTE
    bne AGPLAIN
    iny
    ldx #ASCII_DQUOTE
AGPLAIN:
    stx TEMP6
    ldx #0
AGCOPY:
    lda (TIB),y
    beq AGEND
    cmp TEMP6
    beq AGCLOSE
    cpx #ARGBUF_SIZE - 1
    bcs AGLONG          ; (too long: the rest is left out)
    sta ARGBUF,x
    inx
AGLONG:
    iny
    bra AGCOPY
AGCLOSE:
    cmp #ASCII_DQUOTE   ; (past the closing '"')
    bne AGEND
    iny
AGEND:
    stz ARGBUF,x
    sty CURBUF
    lda #<ARGBUF
    ldy #>ARGBUF
    clc
    rts
AGNONE:
    sty CURBUF
    sec
    rts
;
; include file  read a HyForth script (.hys) into this shell, as if it were typed: its definitions stay.
;               An error, or Ctrl-C, stops it (and the scripts that include it), with its line number
def_word "include", "include", 0
    jsr ARGGET
    bcc INCGO
    lda #ERR_IO_NAME
    bra INCFAIL
;
; ( sz -- )  read a script, as include does
def_word "(include)", "pinclude", 0
    jsr SHARG1
    bcs INCFAIL
INCGO:
    jsr INCOPEN
    bcs INCFAIL
    jmp next
INCFAIL:
    jmp IOFAIL
;
; Start reading the script at .A.Y (a name): stdin is kept (another fd for it), and the script becomes
; stdin until its end (INCEND, from the line reader).  OUT: C = 0; or C = 1, .A = error
INCOPEN:
    ldx INCDEPTH
    cpx #INC_MAX
    bcs INCDEEP
    ldx #IO_MODE_READ
    jsr IO_OPEN
    bcs INCODONE
INCOPENFD:              ; (.A = the script's fd, closed here: stdin has it)
    sta TEMP5           ; the script's fd
    jsr INSAVE          ; stdin, kept
    bcs INCOCLOSE
    ldx INCDEPTH
    sta INCFD,x
    txa
    asl
    tax
    stz INCLINE,x
    stz INCLINE+1,x
    inc INCDEPTH
    lda TEMP5
    ldx #0
    jsr IO_DUP2         ; stdin = the script
INCOCLOSE:
    php
    pha
    lda TEMP5
    jsr IO_CLOSE        ; (its own fd: stdin has it now)
    pla
    plp
INCODONE:
    rts
INCDEEP:
    lda #ERR_IO_NAME
    sec
    rts
;
; A script's end: stdin back to what it was
INCEND:
    dec INCDEPTH
    ldx INCDEPTH
    lda INCFD,x
    pha
    ldx #0
    jsr IO_DUP2
    pla
    jmp IO_CLOSE
;
; The next line of the script being read: count it
INCCOUNT:
    lda INCDEPTH
    asl
    tax
    inc INCLINE-2,x
    bne INCCDONE
    inc INCLINE-1,x
INCCDONE:
    rts
;
; An error while a script is read: say which line of it, and stop it and every script that includes it
INCABORT:
    lda INCDEPTH
    beq INCCDONE
    PRINT_CHAR #'l', #'i', #'n', #'e', #' '
    lda INCDEPTH
    asl
    tax
    lda INCLINE-1,x
    PRINT_BYTE
    lda INCDEPTH
    asl
    tax
    lda INCLINE-2,x
    PRINT_BYTE
INCALOOP:
    jsr INCEND
    lda INCDEPTH
    bne INCALOOP
    rts
;
S_BOOTHYS:
    .byte "boot.hys", 0
;
; run file  run a program in a task of its own, and wait for it to end: a Hydra executable (its .hyx
;           header says so), or else a HyForth script (.hys), read by a copy of this shell (which starts
;           with its stack and definitions, and takes its own away with it).  It has the console while it
;           runs: Ctrl-C stops it.  A word HyForth doesn't know runs the program of that name (RUNNAME)
def_word "run", "run", 0
    jsr ARGGET
    bcc RUNARGS
    lda #ERR_IO_NAME
    bra RUNFAIL
RUNARGS:                ; the rest of the line: the program's arguments
    jsr ARGREST
    lda #<ARGBUF        ; (the name: ARGGET's)
    ldy #>ARGBUF
    bra RUNGO
;
; ( sz -- )  run a program, as run does (with no arguments)
def_word "(run)", "prun", 0
    jsr SHARG1
    bcs RUNFAIL
    stz ARGLINE         ; (no arguments)
RUNGO:
    ldx #SHC_RUN
    jsr RUNCMD
    bcs RUNFAIL
    jmp next
RUNFAIL:
    jmp IOFAIL
;
; A program: SH_CMD .X (SHC_RUN, SHC_EXEC) on the name at .A.Y.  An executable has run when it returns;
; a script runs here: a copy of the shell (TASK_CLONE) reads it from SH_RUN_FD, and we wait for it to end.
; OUT: C = 0; or C = 1, .A = error
RUNCMD:
    jsr SH_CMD
    bcs RCDONE
    tax
    beq RCDONE          ; (an executable: done)
    lda #<run_start
    ldy #>run_start
    ldx #1              ; (HyForth's ROM page)
    jsr TASK_CLONE      ; .A = the copy
    php
    pha
    lda #SH_RUN_FD
    jsr IO_CLOSE        ; (the copy has it)
    pla
    plp
    bcs RCDONE
    ldx #SHC_WAIT
    jmp SH_CMD
RCDONE:
    rts
;
; A word HyForth doesn't know (the token at NXTTOK), when interpreting: the program of that name (SH_EXEC:
; name.hyx or name.hys, here or in the card's /bin), run as run does.
; OUT: C = 0: it ran; or C = 1, .A = error (ERR_IO_NOT_FOUND: no such program)
RUNNAME:
    ldy #0
    lda (NXTTOK),y      ; (its length, then its characters)
    cmp #ARGBUF_SIZE
    bcs RNNONE
    tax
RNCOPY:
    iny
    lda (NXTTOK),y
    sta ARGBUF-1,y
    dex
    bne RNCOPY
    lda #0
    sta ARGBUF,y
    jsr ARGREST         ; (the rest of the line: its arguments)
    lda #<ARGBUF
    ldy #>ARGBUF
    ldx #SHC_EXEC
    bra RUNCMD
RNNONE:
    lda #ERR_IO_NOT_FOUND
    sec
    rts
;
; A program's arguments: the rest of the line (from CURBUF, without the spaces around it; 63 characters at
; most) into ARGLINE, and the line ends there (the program has them, not the shell).  Modifies: .A, .X, .Y
ARGREST:
    ldy CURBUF
ARSKIP:
    lda (TIB),y         ; (spaces before them)
    cmp #ASCII_SPACE
    bne ARCOPY0
    iny
    bra ARSKIP
ARCOPY0:
    ldx #0
ARCOPY:
    lda (TIB),y
    sta ARGLINE,x
    beq ARTRIM
    iny
    inx
    cpx #ARGLINE_SIZE - 1
    bcc ARCOPY
ARTRIM:                 ; (.X = how many: spaces after them go, the line's own and blanked-out redirections)
    cpx #0
    beq AREND
    lda ARGLINE-1,x
    cmp #ASCII_SPACE
    bne AREND
    dex
    bra ARTRIM
AREND:
    stz ARGLINE,x
    ldy CURBUF          ; the line ends here
    lda #0
    sta (TIB),y
    rts
;
; ( -- sz )  a program's arguments: the rest of the line after a program's name (run go.hys a b, or go a b),
;           as a string (e.g. args .sz; args (cat)).  In a script run with them: its own
def_word "args", "args", 0
    lda #MEM_SZ         ; (ARGLINE, as a string record)
    sta ARGREC
    lda #ARGLINE_SIZE
    sta ARGREC+1
    stz ARGREC+2
    lda #<ARGREF
    sta TEMP1
    lda #>ARGREF
    sta TEMP1+1
    jmp this
ARGREF:
    .word ARGREC
;
; edit [file]  edit a text file (a new one, if it isn't there): a line editor, in a task of its own (its h:
;              its commands)
def_word "edit", "edit", 0
    ldx #SHC_EDIT
    jmp SHPARSE
;
; echo text  print the rest of the line, and a new line (without its "s): echo hello > greeting.txt
def_word "echo", "echo", 0
    ldy CURBUF          ; .X = where the text ends (after its last character that isn't a space)
    ldx CURBUF
ECHOEND:
    iny
    lda (TIB),y
    beq ECHOSTART
    cmp #ASCII_SPACE
    beq ECHOEND
    tya
    tax
    inx
    bra ECHOEND
ECHOSTART:
    stx TEMP1
    ldy CURBUF
    iny                 ; (the space after echo)
ECHOCHAR:
    cpy TEMP1
    bcs ECHODONE
    lda (TIB),y
    cmp #ASCII_DQUOTE
    beq ECHONEXT
    PRINT_CHAR
ECHONEXT:
    iny
    bra ECHOCHAR
ECHODONE:
    PRINT_CHAR #ASCII_CR, #ASCII_LF
    lda #0              ; the line ends here: it was echo's
    ldy CURBUF
    sta (TIB),y
    jmp next
;
; A script's copy of the shell starts here (TASK_CLONE, ROM page 1; see RUNCMD): the scripts this shell
; was reading aren't the copy's (stdin goes back), the script is its stdin (SH_RUN_FD), and the rest of
; the line is this shell's.  At the script's end, getline ends the task (BATCH), as for a pipeline's
; left side.
run_start:
    tsx
    stx CHILDSP
    lda #1
    sta BATCH
RSUNWIND:
    lda INCDEPTH
    beq RSOPEN
    jsr INCEND
    bra RSUNWIND
RSOPEN:
    lda #SH_RUN_FD
    jsr INCOPENFD
    bcs RSFAIL          ; (the task ends)
    ldy CURBUF
    lda #0
    sta (TIB),y
    jmp resolve
RSFAIL:
    rts
;
;-------- Shell commands: files and the cards.  The work is done on BIOS page 7 (shell/files.s: SH_CMD).
;   Each has a parsing form, which takes its arguments from the words after it on the line (ls games),
;   and a stack form in parentheses for definitions, which takes q^...^ strings ((ls))
;
; ls [dir]  list a directory: a line per entry, "name size" or "name/" (ls alone: the current one); or
;           show any file's text (ls /dev/sd/0/ctl)
def_word "ls", "ls", 0
    ldx #SHC_LS
    jmp SHPARSE
def_word "(ls)", "pls", 0
    ldx #SHC_LS
    jmp SHSTACK
;
; rm file  remove a file (a directory: rmdir)
def_word "rm", "rm", 0
    ldx #SHC_RM
    jmp SHPARSE
def_word "(rm)", "prm", 0
    ldx #SHC_RM
    jmp SHSTACK
;
; rmdir dir  remove an empty directory
def_word "rmdir", "rmdir", 0
    ldx #SHC_RMDIR
    jmp SHPARSE
def_word "(rmdir)", "prmdir", 0
    ldx #SHC_RMDIR
    jmp SHSTACK
;
; mkdir dir  make a directory
def_word "mkdir", "mkdir", 0
    ldx #SHC_MKDIR
    jmp SHPARSE
def_word "(mkdir)", "pmkdir", 0
    ldx #SHC_MKDIR
    jmp SHSTACK
;
; cp from to  copy a file: to a new name, or into a directory with the same name
def_word "cp", "cp", 0
    ldx #SHC_CP
    jmp SHPARSE2
def_word "(cp)", "pcp", 0
    ldx #SHC_CP
    jmp SHSTACK2
;
; mv from to  rename a file or directory (to: a name, in the same directory); or move a file (to: a path,
;             or a directory to move it into)
def_word "mv", "mv", 0
    ldx #SHC_MV
    jmp SHPARSE2
def_word "(mv)", "pmv", 0
    ldx #SHC_MV
    jmp SHSTACK2
;
; ( -- )  the cards: for each of 0-7, what it is (or none), and its HydraFS: label, free space, last check
def_word "vols", "vols", 0
    ldx #SHC_VOLS
    bra SHDO
;
; ( n sz-label -- )  make an empty HydraFS on card n (0-7), with that label, and show the card: everything
;                   that was on it is lost (e.g. 0 "GAMES" mkfs).  A quick format: the free map is written
;                   as the card fills, so it takes a moment whatever the card's size
def_word "mkfs", "mkfs", 0
    stz SHOPT
MKFSWHOLE:
    stz SHSIZE          ; (the whole card)
    stz SHSIZE+1
MKFSGO:
    ldx #SHC_MKFS
    bra SHCARDSZ
;
; ( n sz-label -- )  mkfs, with the whole free map written now (a version 1 HydraFS, as older ROMs read);
;                   a big card takes minutes, and shows its progress (10% 20% ...)
def_word "mkfs-full", "mkfsfull", 0
    lda #1
    sta SHOPT
    bra MKFSWHOLE
;
; ( n sz-label mb -- )  mkfs, making the HydraFS mb megabytes (up to 65535: $FFFF), if the card is bigger:
;                      the rest of the card isn't used (e.g. 0 "SMALL" 4096 mkfs-size)
def_word "mkfs-size", "mkfssize", 0
    jsr spull_0
    lda TEMP1
    sta SHSIZE
    lda TEMP1+1
    sta SHSIZE+1
    stz SHOPT
    bra MKFSGO
;
; ( n sz-label -- )  give card n's HydraFS a new label (e.g. 0 q^TOYS^ relabel)
def_word "relabel", "relabel", 0
    ldx #SHC_RELABEL
    bra SHCARDSZ
;
; ( n -- )  check card n's HydraFS, and show what it found: clusters lost (marked in use, but nothing uses
;          them), unmarked (in use, but marked free) and used twice
def_word "fsck", "fsck", 0
    ldx #SHC_FSCK
    bra SHCARD
;
; ( n -- )  check it as fsck does, and repair its free map (lost clusters freed, unmarked ones marked; a
;          cluster used twice needs a person, so it's only shown)
def_word "fsfix", "fsfix", 0
    ldx #SHC_FSFIX
SHCARD:                 ; ( n -- ): .A = the card
    phx
    jsr spull_0
    lda TEMP1
    plx
SHDO:                   ; the command .X (SH_CMD, page 7)
    jsr SH_CMD
    bcs SHDOFAIL
    jmp next
SHDOFAIL:
    jmp IOFAIL
SHCARDSZ:               ; ( n sz -- ): .A = the card, SHBUF2 = the text
    phx
    jsr spull_1
    jsr spull_0
    ldx #TEMP2
    jsr SZTEXT
    bcs SHFAILX
    jsr SHCOPY2
    lda TEMP1
    plx
    bra SHDO
SHPARSE:                ; the line's next word (none: .Y = 0)
    phx
    jsr ARGGET
    bcc SHPGO
    ldy #0
SHPGO:
    plx
    bra SHDO
SHSTACK:                ; ( sz -- )
    phx
    jsr SHARG1
    bcs SHFAILX
    plx
    bra SHDO
SHPARSE2:               ; the line's next two words: the first in SHBUF2
    phx
    jsr ARGGET
    bcs SHP2NONE
    jsr SHCOPY2
    jsr ARGGET
    bcs SHP2NONE
    plx
    bra SHDO
SHP2NONE:
    lda #ERR_IO_NAME
SHFAILX:
    plx
    bra SHDOFAIL
SHSTACK2:               ; ( sz1 sz2 -- ): the first in SHBUF2
    phx
    jsr NSARGS          ; .A.Y = the first's text, ZP_IO_BUF = the second's
    bcs SHFAILX
    jsr SHCOPY2
    lda ZP_IO_BUF
    ldy ZP_IO_BUF+1
    plx
    bra SHDO
;
; SHBUF2 = the text at .A.Y (63 characters at most).  Uses TEMP3
SHCOPY2:
    sta TEMP3
    sty TEMP3+1
    ldy #0
SHC2LP:
    lda (TEMP3),y
    sta SHBUF2,y
    beq SHC2DONE
    iny
    cpy #63
    bne SHC2LP
    lda #0
    sta SHBUF2,y
SHC2DONE:
    rts
;
SDCMD_SIZE = 64
SHOWBUF_SIZE = 256
ARGBUF_SIZE = 64
ARGLINE_SIZE = 64
PROMPTFMT_SIZE = 32
; The shell's RAM that starts with a value (its buffers are after 'ends': hyforth.s).  The shell's routines,
; on BIOS page 7, use it too (shell/shell.s)
PROMPTFMT:              ; the prompt's format (prompt)
    .byte "%v%d> ", 0
    .res PROMPTFMT_SIZE - 7
PROMPTLAST:             ; the prompt's last character (put back when the console's echo erases it)
    .byte '>'
REDOUT:                 ; the line's > or >> redirection: stdout, kept ($FF: none; shell/redir.s)
    .byte $FF
REDIN:                  ;   and its <: stdin
    .byte $FF
BOOTFLAG:               ; <> 0: run boot.hys before the first prompt (the boot shell: page 7's SH_BOOT)
    .byte 0
LBLCARD:                ; the card whose label SHLABEL holds ($FF: none yet, or it may have changed)
    .byte $FF
SHN:                    ; the shell routines' scratch
    .byte 0
SHSEL:
    .byte 0
SHFD:
    .byte 0
SHFD2:
    .byte 0
SHTASK:
    .byte 0
INCDEPTH:               ; include: how many scripts deep it is (their fds and lines: INCFD, INCLINE)
    .byte 0
ECHOCR:                 ; <> 0: this line is from the console (a CR LF after it, as it's typed)
    .byte 0
LASTCR:                 ; <> 0: the last line read ended with a CR (so an LF after it is nothing: CR LF)
    .byte 0
;
; ( sz-path sz-dev -- )  mount a device at a path in this task's namespace: names under the path go to
;                       the device (e.g. q^/z^ q^zero^ mount  then  q^/z^ 1 open)
def_word "mount", "mount", 0
    jsr NSARGS
    bcs NSFAIL
    jsr IO_MOUNT
    bra NSDONE
;
; ( sz-path sz-target -- )  bind a path to another: names under the path stand for names under the
;                          target (e.g. q^/tty^ q^/dev/cons^ bind)
def_word "bind", "bind", 0
    jsr NSARGS
    bcs NSFAIL
    jsr IO_BIND
NSDONE:
    bcs NSFAIL
    jmp next
NSFAIL:
    jmp IOFAIL
;
; ( sz-path -- )  remove a path's mount or bind
def_word "unmount", "unmount", 0
    jsr spull_0
    ldx #TEMP1
    jsr SZTEXT
    bcs NSFAIL
    jsr IO_UNMOUNT
    bra NSDONE
;
; ( -- )  list this task's namespace
def_word "ns", "ns", 0
    jsr IO_NS_LIST
    jmp next
;
; ( sz -- )  change the serial port's settings: commands for /dev/ser/ctl, e.g. q^b19200^ stty, or
;            q^l7 pe s1^ stty (b = baud rate, l = data bits, p = parity n/o/e/m/s, s = stop bits).  It
;            waits until the output so far has gone; then switch the terminal to match
def_word "stty", "stty", 0
    jsr spull_0
    ldx #TEMP1
    jsr SZTEXT          ; .A.Y = the commands
    bcs STTYFAIL
    sta TEMP2
    sty TEMP2+1
    lda #<STTY_CTL
    ldy #>STTY_CTL
    ldx #IO_MODE_WRITE
    jsr IO_OPEN
STTYOPEN:               ; (ctl: .A = the fd or the error, TEMP2 = the text)
    bcs STTYFAIL
    sta TEMP3           ; the fd
    lda TEMP2
    sta ZP_IO_BUF
    lda TEMP2+1
    sta ZP_IO_BUF+1
    ldy #0              ; the length
STTYLEN:
    lda (TEMP2),y
    beq STTYWRITE
    iny
    bne STTYLEN
STTYWRITE:
    sty ZP_IO_CNT
    stz ZP_IO_CNT+1
    lda TEMP3
    jsr IO_WRITE
    php
    pha
    lda TEMP3
    jsr IO_CLOSE
    pla
    plp
    bcs STTYFAIL
    jmp next
STTYFAIL:
    jmp IOFAIL
;
; ( -- )  show the serial port's settings (/dev/ser/ctl), e.g. b9600 l8 pn s1
def_word "stty?", "sttyq", 0
    lda #<STTY_CTL
    ldy #>STTY_CTL
    ldx #IO_MODE_READ
    jsr IO_OPEN
    bcs STTYFAIL
    sta TEMP3
STTYSHOW:
    ldx TEMP3
    jsr IO_GETC
    bcs STTYSHOWN       ; (the end)
    PRINT_CHAR
    bra STTYSHOW
STTYSHOWN:
    lda TEMP3
    jsr IO_CLOSE
    jmp next
STTY_CTL:
    .byte "/dev/ser/ctl", 0
;
; ( sz-file sz-text -- )  write a line of text to a file: a command to a ctl file, e.g.
;                        q^/dev/sd/0/ctl^ q^check^ ctl  (then q^/dev/sd/0/ctl^ ls shows what it found)
def_word "ctl", "ctl", 0
    jsr NSARGS          ; .A.Y = the file's name, ZP_IO_BUF = the text
    bcs CTLFAIL
    ldx ZP_IO_BUF
    stx TEMP2
    ldx ZP_IO_BUF+1
    stx TEMP2+1
    ldx #IO_MODE_WRITE
    jsr IO_OPEN
    jmp STTYOPEN        ; (stty's: it writes TEMP2's text, and closes the file)
CTLFAIL:
    jmp IOFAIL
;
; ( sz-path sz-2 -- ) -> .A.Y = the path's text, ZP_IO_BUF = the second's (C = 1: not strings)
NSARGS:
    jsr spull_1
    jsr spull_0
    ldx #TEMP2
    jsr SZTEXT
    bcs NSARGEND
    sta ZP_IO_BUF
    sty ZP_IO_BUF+1
    ldx #TEMP1
    jmp SZTEXT
NSARGEND:
    rts
;
; The text of a q^...^ string.  IN: .X = the ZP address of its slot (TEMP1, TEMP2)
; OUT: .A.Y = its text, C = 0; or .A = ERR_IO_NAME, C = 1.  Uses TEMP3
SZTEXT:
    lda 0,x
    sta TEMP3
    lda 1,x
    sta TEMP3+1
    ldy #0
    lda (TEMP3),y         ; the record
    pha
    iny
    lda (TEMP3),y
    sta TEMP3+1
    pla
    sta TEMP3
    lda (TEMP3)
    and #$7F              ; (the temp flag)
    cmp #MEM_SZ
    bne SZBAD
    lda TEMP3             ; its text, after the 3-byte header
    clc
    adc #3
    pha
    lda TEMP3+1
    adc #0
    tay
    pla
    clc
    rts
SZBAD:
    lda #ERR_IO_NAME
    sec
    rts
;
; ( -- n )  the last IO error code
def_word "ioerr", "ioerr", 0
    lda IOERR
    jmp IOPUSHA
;
; cat [file]  show a file; or with none, copy stdin to stdout, to the end of the file (e.g. the right side
;             of a pipeline:  words | cat)
def_word "cat", "cat", 0
    jsr ARGGET          ; cat file: the file
    bcs CATLOOP
    ldx #SHC_CAT
    jmp SHDO
CATLOOP:
    jsr GET_CHAR
    bcc CATEND        ; end of file (or an error)
    PRINT_CHAR
    bra CATLOOP
CATEND:
    jmp next
;
; ( -- lines words chars )  count stdin's lines (ended by CR, LF or CR LF), words (runs of characters
;                           other than space, tab, CR and LF) and characters, to the end of the file
;                           (e.g.  words | wc .S; or typed, ended with Ctrl-D)
def_word "wc", "wc", 0
    ldx #5
WCCLR:
    stz TEMP1,x       ; TEMP1 = characters, TEMP2 = words, TEMP3 = lines
    dex
    bpl WCCLR
    stz TEMP4         ; TEMP4 <> 0: in a word
    stz TEMP4+1       ; TEMP4+1 <> 0: the last character was a CR
WCLOOP:
    jsr GET_CHAR
    bcc WCEND
    inc TEMP1         ; a character
    bne WCSK0
    inc TEMP1+1
WCSK0:
    ldx TEMP4+1       ; (the last one a CR?)
    stz TEMP4+1
    cmp #ASCII_CR
    bne WCNOTCR
    inc TEMP4+1
    bra WCLINE
WCNOTCR:
    cmp #ASCII_LF
    bne WCSK1
    cpx #0
    bne WCSK1         ; CR LF: one line
WCLINE:
    inc TEMP3         ; a line
    bne WCSK1
    inc TEMP3+1
WCSK1:
    cmp #ASCII_SPACE
    beq WCGAP
    cmp #ASCII_TAB
    beq WCGAP
    cmp #ASCII_CR
    beq WCGAP
    cmp #ASCII_LF
    beq WCGAP
    lda TEMP4
    bne WCLOOP        ; (still in the word)
    inc TEMP4         ; a new word
    inc TEMP2
    bne WCLOOP
    inc TEMP2+1
    bra WCLOOP
WCGAP:
    stz TEMP4
    bra WCLOOP
WCEND:
    lda TEMP1         ; ( -- lines words chars )
    pha
    lda TEMP1+1
    pha
    lda TEMP2
    pha
    lda TEMP2+1
    pha
    lda TEMP3
    sta TEMP1
    lda TEMP3+1
    sta TEMP1+1
    jsr spush_0       ; lines
    pla
    sta TEMP1+1
    pla
    sta TEMP1
    jsr spush_0       ; words
    pla
    sta TEMP1+1
    pla
    sta TEMP1
    jmp this          ; characters
;
;-------- Tasks: another shell, the foreground, kill, and the task list (/dev/proc)
;
; ( -- n )  start another shell (HyForth, in a task of its own); n = its task.  It prints its banner and
;           waits for input until it's brought to the front (fg, or Ctrl-] then n)
def_word "shell", "shell", 0
    lda #<::SHELL_MAIN
    ldy #>::SHELL_MAIN
    ldx #0            ; (ROM page 0)
    jsr TASK_RUN
    bcc TKPUSH
    jmp IOFAIL
TKPUSH:
    jmp IOPUSHA
;
; ( n -- )  bring task n to the front: the console reads for it, and only it (and the tasks it started)
;           write to it; the others wait.  Ctrl-] then n does the same
def_word "fg", "fg", 0
    jsr spull_0
    lda TEMP1
    jsr CONS_SET_FG
    bcc TKOK
    jmp IOFAIL
TKOK:
    jmp next
;
; ( n -- )  kill task n, and the tasks it started (as Ctrl-\ does to the foreground task)
def_word "kill", "kill", 0
    jsr spull_0
    ldx TEMP1
    lda #TASK_KILL_FLAG
    jsr TASK_SIGNAL
    bcc TKOK
    jmp IOFAIL
;
; ( n -- )  sleep for n ticks (200 a second: 200 sleep is 1 second, up to 32767): the other tasks run
;           meanwhile, or the system idles.  Ctrl-C ends it
def_word "sleep", "sleep", 0
    jsr spull_0
    lda TEMP1
    ldy TEMP1 + 1
    jsr TASK_SLEEP
    jmp next
;
; ( -- )  list the tasks (/dev/proc): the task, its state (R runnable, W waiting (IO or sleep), P paused, D a
;         driver) and the task that started it; * = the foreground task
def_word "ps", "ps", 0
    lda #<PSNAME      ; (This code, and the name, run from RAM: IO_OPEN can read it)
    ldy #>PSNAME
    ldx #IO_MODE_READ
    jsr IO_OPEN
    bcc PSOPEN
    jmp IOFAIL
PSOPEN:
    sta TEMP1         ; the fd
PSLOOP:
    ldx TEMP1
    jsr IO_GETC
    bcs PSEND         ; end of file
    PRINT_CHAR
    bra PSLOOP
PSEND:
    lda TEMP1
    jsr IO_CLOSE
    jmp next
PSNAME:
    .byte "/dev/proc", 0
;
;-------- Pipelines: a line  left | right  runs 'left' in a copy of this task (TASK_CLONE), with its
;         stdout into a pipe, and 'right' here, with stdin from the pipe.  Called by getline.
;
; If the line (TIB) has a '|' with spaces around it (outside q^...^ and "..." strings) and we're interpreting,
; start the left side, and go on after the '|'; and again for each '|' after it (a | b | c: 'b' runs in a
; copy too, between two pipes).  (No '|': returns with the line untouched.)
PIPECHK:
    lda STATUS
    bne PCNONE        ; compiling
    ldy CURBUF        ; (What's left of the line)
    iny
    ldx #0            ; inside a string: 1 (q^...^) or 2 ("..."); 0: not
PCSCAN:
    lda (TIB),y
    beq PCNONE
    cmp #ASCII_CARET
    bne PCQUOTE
    cpx #2
    beq PCNEXT        ; (a '^' in a "..." string)
    txa
    eor #1
    tax
    bra PCNEXT
PCQUOTE:
    cmp #ASCII_DQUOTE
    bne PCBAR
    cpx #1
    beq PCNEXT        ; (a '"' in a q^...^ string)
    txa
    eor #2
    tax
    bra PCNEXT
PCBAR:
    cmp #ASCII_PIPE
    bne PCNEXT
    cpx #0
    bne PCNEXT        ; inside a string
    dey
    lda (TIB),y
    iny
    cmp #ASCII_SPACE
    bne PCNEXT
    iny
    lda (TIB),y
    dey
    cmp #ASCII_SPACE
    beq PCFOUND
PCNEXT:
    iny
    bne PCSCAN
PCNONE:
    rts
PCFOUND:                   ; .Y = the '|'
    phy
    jsr IO_PIPE            ; .A = read fd, .X = write fd
    bcs PCFAIL
    sta PIPER
    stx PIPEW
    ply
    phy
    lda #0
    sta (TIB),y            ; the copy's line ends at the '|'...
    lda #<child_start
    ldy #>child_start
    ldx #1                 ; (HyForth's ROM page)
    jsr TASK_CLONE
    ply
    pha
    lda #ASCII_SPACE
    sta (TIB),y            ; ...and this one goes on after it
    sty CURBUF
    pla
    bcs PCNOTASK
    lda PIPEW              ; the copy has the write end now
    jsr IO_CLOSE
    lda PIPEIN
    bpl PCSAVED            ; (a later '|': stdin is the previous pipe; the new one replaces it)
    jsr INSAVE             ; save stdin (with its read-ahead given back: a pipeline in a script)
    bcs PCFAIL2
    sta PIPEIN
PCSAVED:
    lda PIPER
    ldx #0
    jsr IO_DUP2            ; stdin from the pipe
    bcs PCFAIL2
    lda PIPER
    jsr IO_CLOSE
    jmp PIPECHK            ; another '|'?
PCNOTASK:                  ; no task for the copy: no pipeline
    pha
    lda PIPEW
    jsr IO_CLOSE
    lda PIPER
    jsr IO_CLOSE
    pla
    bra PCFAIL2
PCFAIL:
    ply
PCFAIL2:                   ; drop the returns to getline and 'resolve', then the error
    ply
    ply
    ply
    ply
    jmp IOFAIL
;
; The end of a pipeline's line (getline): stdin back
PIPEEND:
    lda PIPEIN
    bmi PEDONE             ; ($FF: not redirected)
    ldx #0
    jsr IO_DUP2
    lda PIPEIN
    jsr IO_CLOSE
    lda #$FF
    sta PIPEIN
PEDONE:
    rts
;
; A pipeline's left side starts here, in a copy of the shell's task (TASK_CLONE, ROM page 1): stdout into
; the pipe, then run the line, which ends at the '|'.  At its end, getline ends the task (BATCH).
child_start:
    tsx
    stx CHILDSP
    lda #1
    sta BATCH
    lda PIPEW
    ldx #1
    jsr IO_DUP2
    lda PIPEW
    jsr IO_CLOSE
    lda PIPER
    jsr IO_CLOSE
    jmp resolve
;
; ( fd addr n -- ) -> ZP_IO_BUF = addr, ZP_IO_CNT = n, .A = fd
IOARGS:
    jsr spull_2       ; n
    jsr spull_1       ; addr
    jsr spull_0       ; fd
    lda TEMP2
    sta ZP_IO_BUF
    lda TEMP2+1
    sta ZP_IO_BUF+1
    lda TEMP3
    sta ZP_IO_CNT
    lda TEMP3+1
    sta ZP_IO_CNT+1
    lda TEMP1
    rts
;
;
.ifdef YSOUND
;
;                        word definitions for Yamaha 2151 sound chip
;
; They go through the sound driver's file, /dev/snd: IO_CTL codes, and register/value pairs to write
;
; ( -- )  clear the YM2151 (and stop the test tune)
def_word "sndinit", "sndinit", 0
    lda #SND_CTL_INIT
    jmp SNDCTL
;
; ( -- )  play the test tune, in the background: it goes on while you do other things (sndstop ends it)
def_word "sndtest", "sndtest", 0
    lda #SND_CTL_TEST
    jmp SNDCTL
;
; ( -- )  stop the test tune
def_word "sndstop", "sndstop", 0
    lda #SND_CTL_STOP
SNDCTL:               ; IO_CTL code .A on /dev/snd
    sta TEMP2
    jsr SNDOPEN
    sta TEMP1         ; the fd
    ldx TEMP2
    jsr IO_CTL
SNDCLOSE:            ; close fd TEMP1, keeping .A and C
    php
    pha
    lda TEMP1
    jsr IO_CLOSE
    pla
    plp
    bcs SNDFAIL
    jmp next
SNDFAIL:
    jmp IOFAIL
;
; Open /dev/snd for writing: .A = the fd (a failure: IOFAIL)
SNDOPEN:
    lda #<SNDNAME
    ldy #>SNDNAME
    ldx #IO_MODE_WRITE
    jsr IO_OPEN
    bcs SNDOPENF
    rts
SNDOPENF:
    ply               ; (drop the return: fail from the word)
    ply
    jmp IOFAIL
SNDNAME:
    .byte "/dev/snd", 0
;
; ( xxaa -- f )    send byte(a) to register(x) on yamaha 2151: f = true if it went
def_word "ywrite", "ywrite", 0
    jsr spull_0
    lda TEMP1+1       ; the register, then the value: a pair for /dev/snd
    sta TEMP3
    lda TEMP1
    sta TEMP3+1
    jsr SNDOPEN
    sta TEMP1
    lda #<TEMP3
    sta ZP_IO_BUF
    stz ZP_IO_BUF+1
    lda #2
    sta ZP_IO_CNT
    stz ZP_IO_CNT+1
    lda TEMP1
    jsr IO_WRITE
    php
    lda TEMP1
    jsr IO_CLOSE
    plp
    bcs YMBAD
    jmp PUSHTRUE
YMBAD:
    jmp PUSHFALSE
.endif
;
;
;----------------------------------------------------------------------------
HYWORDS_END:
;  end hywords.s
;----------------------------------------------------------------------------