;
;HyForth extra commands
;
; hywords.s
;
; (A word made with def_far has its code on BIOS ROM page A, in farwords.s: only its header is here)
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
; ( -- )   ANSI clear screen (the term library)
.ifdef ANSIOK
lib_begin LIBN_TERM
def_far "Acls", "Acls"
;
; (c r -- )      ANSI screen position ESC[<r>;<c>f
def_far "Ascr", "Ascr"
;
; (c -- )      ANSI attributes ESC[<c>m
def_far "Acol", "Acol"
lib_end
.endif   ; ANSIOK

;
;--------------------------RANDOM #'s
; ( s s -- )   random #
def_far "rseed", "rseed"
; ( -- r)          16 bit rand -> stack
;
def_far "rand", "rand"
;
; ( -- r r)          32 bit rand -> stack
def_far "rand32", "rand32"
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
def_far "*", "mult16"

;
; (ux um -- r m)      16x16 divide, result + MOD
;
def_far "/", "div16"
;
; ( hex -- d1 d2 d3 d4 d5 )
def_far "xdrv", "xdrv"
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
;-------- The tools library: disasm and syscall here; dump (primitives.s), hwtest and mmtest below
lib_begin LIBN_TOOLS
;
; (daddr n -- ) start disassembly from daddr, do it
; $F600 is entry point --A+Y for starting address, C=1 for multiple opcodes, X for # of codes
def_far "disasm", "disasm"

; (jsaddr a y -- x) call machine code (a thunk, or code in RAM), on BIOS ROM page 0, with .A and .Y set
def_far "syscall", "syscall"
; (jsaddr a x y -- a x y p) the same, with every register, in and out, and the flags (p: C is bit 0)
def_far "sys", "sys"
lib_end
;
;
; ( paddr -- )
def_far "mktemp", "mktemp"
;
;   ( -- )  --- 'deallocate' last memory item in stack if marked 'temp'
;
def_far "purge0", "purge0"
;
;
; ( bytes type -- staddr )
def_far "malloc", "malloc"

;
; ( maddr -- len )
def_far "mlen", "mlen"
;
;-------- MMU handles (see docs/plans/MMU_PLAN.md): the mem library
lib_begin LIBN_MEM
;
; ( bytes flags -- h )  allocate MMU memory; flags 0 = task RAM, 1 = 8K RAM banks (AI_PAGED)
def_far "halloc", "halloc"
;
; ( h -- )  free MMU memory
def_far "hfree", "hfree"
;
; ( h -- addr )  raw address of MMU memory (8K RAM bank allocations: selects the bank at $8000);
;               can't be freed until hunlock.  One hlock at a time.
def_far "hlock", "hlock"
;
; ( h -- )  undo hlock (restores the RAM bank)
def_far "hunlock", "hunlock"
lib_end
lib_begin LIBN_TOOLS
;
; ( -- )  the hardware test: a program in the paged ROM that takes the machine over and tests what it can
;          (RAM, ROMs, registers, IRQs, the VIA, ACIA, YM2151, SPI, I2C ...) from a menu.  It overwrites
;          memory, and ends with a reset
def_far "hwtest", "hwtest"
;
; ( -- )  run the MMU self test
def_word "mmtest", "mmtest", 0
    jsr MMU_TEST
    jmp next
lib_end
;
;-------- Libraries: the word sets beyond this base (LIBN_IO ...; hyforth.s), each a chain of headers of its
;         own, searched when it's loaded (LIBSET).  'cold' loads them all (LIB_BOOT); the code is on page A
;
.pushseg
.segment "FORTH_HIGH"   ; (Page 1's room: after its gates)
; ( -- n )  the exit status: the last program's (or script's, or error's) code, 0 for success.  $status
;           (/env/status) has it as text: the program's message, or the code, or "" for 0
def_far "status", "status"
;
; ( n -- )  end this task (a script run by run, a pipeline's stage, a command shell) with exit status n.  The
;           boot shell can't end: there it only sets the status
def_far "exits", "exits"
.popseg
;
; libs  list the libraries: forth (the base, always there), then each one, in parentheses if it isn't loaded
def_far "libs", "libs"
;
; lib name  load a library (and the ones it needs), e.g. lib sound: its words are found again
def_far "lib", "lib"
;
; -lib name  unload a library: its words aren't found (the ones compiled into definitions still run).
;            -lib shell: a plain prompt, and no pipelines, redirection, or programs run by name
def_far "-lib", "unlib"
;
;-------- IO: files (see docs/plans/IO_PLAN.md): the io library (with mount ... ioerr below).  A failed call gives !IO ERR!, and 'ioerr' the IO layer's
;         error code ($70 not found, $71 bad fd, $72 wrong mode, $73 would block, $75 no fds, ...).
;         fds 0, 1, 2 are the console (key, emit); buffers must be in task RAM ($0000-$7FFF).
;
; ( sz mode -- fd )  open a file: sz = a q^...^ string, e.g. q^/dev/cons^; mode 1 = read, 2 = write,
;                   3 = both, + $80 = don't wait (reads and writes give ioerr $73 instead)
lib_begin LIBN_IO
def_far "open", "open"
;
; ( fd -- )  close a file
def_far "close", "close"
;
; ( fd addr n -- n' )  read up to n bytes into addr; n' = bytes read (0 = end of file).  Waits for data
;                     (the console: at least one key), unless the fd was opened with $80.
def_far "read", "read"
;
; ( fd addr n -- n' )  write n bytes from addr; n' = bytes written
def_far "write", "write"
;
; ( fd lo hi -- )  set fd's offset for the next read or write (32 bits: hi * 65536 + lo), e.g. in /dev/sd
def_far "seek", "seek"
;
; ( fd code arg -- )  device control, e.g. fd 1 task ioctl: make task the foreground task (the
;                    console's input goes to it)
def_far "ioctl", "ioctl"
;
; ( fd newfd -- )  make newfd refer to the same file as fd (closing newfd first), e.g. fd 1 fdup2
;                  sends emit's output to fd's file
def_far "fdup2", "fdup2"
;
; ( -- rfd wfd )  make a pipe: what's written to wfd can be read from rfd
def_far "pipe", "pipe"
;
;-------- Files on the SD cards (HydraFS, at /sd/N: e.g. q^/sd/0/games^ ls)
;
; ( sz mode -- fd )  create a file (mode 0; $40 append-only, $01 read-only) or a directory ($80), and open
;                   it: a file for reading and writing, a directory for reading.  A file that's there already
;                   is emptied (e.g. q^/sd/0/notes^ 0 create)
def_far "create", "create"
lib_end
;
;-------- The shell: the current directory, and commands that take their arguments from the line
;   A parsing word (cd games) takes the words after it on the line (ARGGET); its stack form, for
;   definitions, takes q^...^ strings: (cd).  The work is done on BIOS page 7 (shell/shell.s).
;
; cd [dir]  change directory (relative, or not; ".." understood); cd alone: the current card's root
lib_begin LIBN_FILES
def_far "cd", "cd"
;
; ( sz -- )  change directory
def_far "(cd)", "pcd"
;
; pwd  show the current directory
def_far "pwd", "pwd"
lib_end
lib_begin LIBN_SHELL
;
; ( sz -- )  set the prompt's format: %v the volume ("0:"), %d the directory on the card (or the whole
;           path off the cards), %p the whole path, %l the card's label, %t the task, %% a %
;           (default: "%v%d> " prompt)
def_far "prompt", "prompt"
;
; include file  read a HyForth script (.hys) into this shell, as if it were typed: its definitions stay.
;               An error, or Ctrl-C, stops it (and the scripts that include it), with its line number
def_far "include", "include"
;
; ( sz -- )  read a script, as include does
def_far "(include)", "pinclude"
;
; run file  run a program in a task of its own, and wait for it to end: a Hydra executable (its .hyx
;           header says so), or else a HyForth script (.hys), read by a copy of this shell (which starts
;           with its stack and definitions, and takes its own away with it).  It has the console while it
;           runs: Ctrl-C stops it.  A word HyForth doesn't know runs the program of that name (RUNNAME)
def_far "run", "run"
;
; ( sz -- )  run a program, as run does (with no arguments)
def_far "(run)", "prun"
;
; ( -- sz )  a program's arguments: the rest of the line after a program's name (run go.hys a b, or go a b),
;           as a string (e.g. args .sz; args (cat)).  In a script run with them: its own
def_far "args", "args"
;
; edit [file]  edit a text file (a new one, if it isn't there): a line editor, in a task of its own (its h:
;              its commands)
def_far "edit", "edit"
;
; echo text  print the rest of the line, and a new line (without its "s): echo hello > greeting.txt
def_far "echo", "echo"
.pushseg
.segment "FORTH_TOP"    ; (Page 1's room above COMMON, $FE00)
;
; send N line  shell N (task N, one this shell started) runs the line, as if typed at its prompt: now, if it's
;              waiting there, or when it's next there (/proc/N/cmd)
def_far "send", "send"
.popseg
lib_end
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
;-------- Shell commands: files and the cards (the files library).  The work is done on BIOS page 7
;         (shell/files.s: SH_CMD).
;   Each has a parsing form, which takes its arguments from the words after it on the line (ls games),
;   and a stack form in parentheses for definitions, which takes q^...^ strings ((ls))
;
; ls [-l] [dir]  list a directory: a line per entry, "name size" or "name/" (ls alone: the current one); or
;                show any file's text (ls /dev/sd/0/ctl).  -l: with each one's date and time (its last change)
lib_begin LIBN_FILES
def_far "ls", "ls"
def_far "(ls)", "pls"
;
; rm file  remove a file (a directory: rmdir)
def_far "rm", "rm"
def_far "(rm)", "prm"
;
; rmdir dir  remove an empty directory
def_far "rmdir", "rmdir"
def_far "(rmdir)", "prmdir"
;
; mkdir dir  make a directory
def_far "mkdir", "mkdir"
def_far "(mkdir)", "pmkdir"
;
; cp from to  copy a file: to a new name, or into a directory with the same name
def_far "cp", "cp"
def_far "(cp)", "pcp"
;
; mv from to  rename a file or directory (to: a name, in the same directory); or move a file (to: a path,
;             or a directory to move it into)
def_far "mv", "mv"
def_far "(mv)", "pmv"
;
; ( -- )  the cards: for each of 0-7, what it is (or none), and its HydraFS: label, free space, last check
def_far "vols", "vols"
;
; ( n sz-label -- )  make an empty HydraFS on card n (0-7), with that label, and show the card: everything
;                   that was on it is lost (e.g. 0 "GAMES" mkfs).  A quick format: the free map is written
;                   as the card fills, so it takes a moment whatever the card's size.  A card with a HydraFS
;                   partition has it made in the partition (the others are kept)
def_far "mkfs", "mkfs"
;
; ( n sz-label -- )  mkfs, with the whole free map written now (a version 1 HydraFS, as older ROMs read);
;                   a big card takes minutes, and shows its progress (10% 20% ...)
def_far "mkfs-full", "mkfsfull"
;
; ( n sz-label mb -- )  mkfs, making the HydraFS mb megabytes (up to 65535: $FFFF), if the card is bigger:
;                      the rest of the card isn't used (e.g. 0 "SMALL" 4096 mkfs-size)
def_far "mkfs-size", "mkfssize"
;
; ( n sz-label -- )  mkfs, in a HydraFS partition: one is made after the card's other partitions (e.g. a FAT
;                   one, for a PC), or with a new partition table, if it hasn't one (e.g. 0 "GAMES" mkfs-part)
def_far "mkfs-part", "mkfspart"
;
; ( n sz-label -- )  give card n's HydraFS a new label (e.g. 0 q^TOYS^ relabel)
def_far "relabel", "relabel"
;
; ( n -- )  check card n's HydraFS, and show what it found: clusters lost (marked in use, but nothing uses
;          them), unmarked (in use, but marked free) and used twice
def_far "fsck", "fsck"
;
; ( n -- )  check it as fsck does, and repair its free map (lost clusters freed, unmarked ones marked; a
;          cluster used twice needs a person, so it's only shown)
def_far "fsfix", "fsfix"
lib_end
;
SDCMD_SIZE = 64
SHOWBUF_SIZE = 256
ARGBUF_SIZE = 64
ARGLINE_SIZE = 64
PROMPTFMT_SIZE = 32
; The shell's RAM that starts with a value (its buffers are after 'ends': hyforth.s).  The shell's routines,
; on BIOS page 7, use it too (shell/shell.s)
.segment "FORTH_DATA"
PROMPTFMT:              ; the prompt's format (prompt)
    .byte "%v%d> ", 0
    .res PROMPTFMT_SIZE - 7
PROMPTLAST:             ; the prompt's last character (put back when the console's echo erases it)
    .byte '>'
REDOUT:                 ; the line's > or >> redirection: stdout, kept ($FF: none; shell/redir.s)
    .byte $FF
REDIN:                  ;   and its <: stdin
    .byte $FF
OUTBASE:                ; The base '.' and 'u.' print in: 16 (hex, 4 digits) or 10 (decimal, signed for '.')
    .byte 16
LECTL:                  ; The console's line editor (LINE_EDIT, farwords.s): its fd on /dev/cons/ctl ($FF: none)
    .byte $FF
LELEN:                  ;   the line's length (in TIB from 1), the cursor (before character LEPOS + 1) ...
    .byte 0
LEPOS:
    .byte 0
LEHPOS:                 ;   the line from HIST being shown (HISTLEN: the new one)
    .byte 0
LEOLD:                  ;   scratch: an old length, spaces to write, a count
    .byte 0
LEPAD:
    .byte 0
LECNT:
    .byte 0
LE_RAWON:               ;   what it writes to /dev/cons/ctl
    .byte "rawon"
HISTLEN:                ; The lines typed (LINE_EDIT's history): HISTLEN bytes in HIST, each line and a 0, the oldest
    .byte 0             ;   first
HIST:
    .res HIST_SIZE
BOOTFLAG:               ; Before the first prompt (the boot shell: page 7's SH_BOOT): 1, run boot.hys (a card's);
                        ;   2, /rom/boot.hys (no card); 0, neither
    .byte 0
BAREFLAG:               ; <> 0: a bare Forth ('forth': forth_bare_main): 'cold' loads no libraries
    .byte 0
CMDFLAG:                ; <> 0: a command shell (SHELL_CMD: page 7's SH_CMDSHELL): no banner, and the end of
    .byte 0             ;   its input ends it (LINE_EOF)
HYSTAT:                 ; The exit status (status, $status): the last program's, script's or error's code ...
    .byte 0
HYSTATMSG:              ;   and its message ("": none; page 7's SH_WAIT, SH_STATUS_OUT)
    .res ::EXIT_MSG_MAX + 2
SHNAMEP:                ; The program being run: its name as run, for its argv[0] (page 7: SH_ARGS_OUT)
    .word 0
SHBG:                   ; <> 0: it was started with & (ARGREST): not waited for (page 7: SH_WAIT)
    .byte 0
LIB_HEADS2:             ; The chains LIBSET2 selects (LIB_NEXT): the RAM libraries' (by slot, 0 till
    .word 0, 0, 0, 0    ;   loaded), and the base's (entry 7: its last header, 'exit')
    .word 0, 0, 0, h_exit
RLIBNAME:               ; Each RAM library slot: its name (0: a free slot; lib, -lib, libs) ...
    .res RLIB_MAX * RLIB_NAMELEN
RLIBSAVE:               ;   while it's loading: the words in RAM's chain (LASTHEAP), put back at its end
    .res RLIB_MAX * 2
RLIBDEPTH:              ;   and the depth of its file (INCDEPTH, with it open), $FF: not loading
    .byte $FF, $FF, $FF, $FF
RLIBFAIL:               ; <> 0: the scripts being read are being stopped (an error): a library being
    .byte 0             ;   loaded is dropped (INCABORT, INCEND)
RLIBHAVE:               ; The slots loaded (bit s: slot s; lib all searches them all again)
    .byte 0
RLIBSLOT:               ; (lib's: the slot it's on)
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
SHFIND:                 ; What SH_EXEC's search looks for: SH_FIND_PROG (a program) or SH_FIND_LIB (lib's)
    .byte 0
INCDEPTH:               ; include: how many scripts deep it is (their fds and lines: INCFD, INCLINE)
    .byte 0
ECHOCR:                 ; <> 0: this line is from the console (a CR LF after it, as it's typed)
    .byte 0
LASTCR:                 ; <> 0: the last line read ended with a CR (so an LF after it is nothing: CR LF)
    .byte 0
FWSP:                   ; a far word's stack pointer, on page A (farwords.s: FW_ENTRY)
    .byte 0
S_BOOTHYS:              ; (getline's, for INCOPEN on page A: so it's in RAM, not on page 1)
    .byte "boot.hys", 0
S_ROMBOOT:              ; (... and the ROM's, when the current directory has none)
    .byte "/rom/boot.hys", 0
.segment "FORTH_CORE"
lib_begin LIBN_IO
;
; Namespaces, as Plan 9's (docs/io.md).  Each has a parsing form, which takes its arguments from the words
; after it on the line, and a stack form in parentheses, which takes q^...^ strings and no flags.
; mount [-abc] device path  attach a device at a path in this task's namespace: names under the path go to
;                           the device (e.g. mount zero /z  then  q^/z^ 1 open).  With none of -a -b,
;                           it replaces the path's entries; -b puts it before them in the path's union,
;                           -a after; -c: a file made in the union is made in it
def_far "mount", "mount"
;
; bind [-abc] new old  names under old stand for names under new (e.g. bind /dev/cons /tty); -a -b -c: as
;                      mount's
def_far "bind", "bind"
;
; unmount [new] old  remove old's entries; or just one member of its union: the bind of new, or the mount
;                    of the device new
def_far "unmount", "unmount"
.pushseg
.segment "FORTH_TOP"    ; (Page 1's room above COMMON, $FE00)
;
; hide path  nothing under the path is found (in this task, and the tasks it starts)
def_far "hide", "hide"
;
; ( sz-dev sz-path -- )  (mount)
def_far "(mount)", "pmount"
;
; ( sz-new sz-old -- )  (bind)
def_far "(bind)", "pbind"
;
; ( sz-old -- )  (unmount)
def_far "(unmount)", "punmount"
.popseg
;
; ( -- )  list this task's namespace
def_far "ns", "ns"
;
; ( sz -- )  change the serial port's settings: commands for /dev/ser/ctl, e.g. q^b19200^ stty, or
;            q^l7 pe s1^ stty (b = baud rate, l = data bits, p = parity n/o/e/m/s, s = stop bits).  It
;            waits until the output so far has gone; then switch the terminal to match
def_far "stty", "stty"
;
; ( -- )  show the serial port's settings (/dev/ser/ctl), e.g. b9600 l8 pn s1
def_far "stty?", "sttyq"
;
; ( sz-file sz-text -- )  write a line of text to a file: a command to a ctl file, e.g.
;                        q^/dev/sd/0/ctl^ q^check^ ctl  (then q^/dev/sd/0/ctl^ ls shows what it found)
def_far "ctl", "ctl"
;
; ( -- n )  the last IO error code
def_far "ioerr", "ioerr"
lib_end
lib_begin LIBN_FILES
;
; cat [file]  show a file; or with none, copy stdin to stdout, to the end of the file (e.g. the right side
;             of a pipeline:  words | cat)
def_far "cat", "cat"
;
; ( -- lines words chars )  count stdin's lines (ended by CR, LF or CR LF), words (runs of characters
;                           other than space, tab, CR and LF) and characters, to the end of the file
;                           (e.g.  words | wc .S; or typed, ended with Ctrl-D)
def_far "wc", "wc"
lib_end
;
;-------- Tasks: another shell, the foreground, kill, the task list (/dev/proc), and semaphores: the tasks
;         library
lib_begin LIBN_TASKS
;
; ( -- n )  start another shell (HyForth, in a task of its own); n = its task.  It prints its banner and
;           waits for input until it's brought to the front (fg, or Ctrl-] then n)
def_far "shell", "shell"
;
; ( -- n )  start a bare Forth: HyForth with only its base loaded (no libraries: lib loads them), in a task
;           of its own; n = its task.  As shell's, it waits for input until it's brought to the front
def_far "forth", "forth"
;
; ( n -- )  bring task n to the front: the console reads for it, and only it (and the tasks it started)
;           write to it; the others wait.  Ctrl-] then n does the same
def_far "fg", "fg"
;
; ( n -- )  kill task n, and the tasks it started (as Ctrl-\ does to the foreground task)
def_far "kill", "kill"
;
; ( n -- )  sleep for n ticks (200 a second: 200 sleep is 1 second, up to 32767): the other tasks run
;           meanwhile, or the system idles.  Ctrl-C ends it
def_far "sleep", "sleep"
;
; ( -- )  list the tasks (/dev/proc): the task, its state (R runnable, W waiting (IO or sleep), P paused, D a
;         driver) and the task that started it; * = the foreground task
def_far "ps", "ps"
;
.pushseg
.segment "FORTH_HIGH"
; ( n -- )  wait for task n (one this shell started with &) to end, with the console meanwhile: its exit
;           status is the status then
def_far "wait", "wait"
.popseg
;
; ( n -- s )  a semaphore of n: n takes (acquire) before a task has to wait; s = its number (1-16)
def_far "sem", "sem"
;
; ( -- s )  a mutex: a semaphore of 1 that only the task that took it can release (released if it ends)
def_far "mutex", "mutex"
;
; ( s -- )  take one of semaphore s: if there's none, wait (using no CPU) until there is.  Ctrl-C ends it
def_far "acquire", "acquire"
;
; ( s -- f )  take one if there is one (true); else false, at once
def_far "acquire?", "acquireq"
;
; ( s -- )  give one back to semaphore s (a mutex: only its holder can): a task waiting for it goes on
def_far "release", "release"
;
; ( s -- )  free semaphore s: its number can be made again, and the tasks waiting for it get an error
def_far "-sem", "unsem"
lib_end
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
;
;
.ifdef YSOUND
;
;                        word definitions for Yamaha 2151 sound chip
;
; They go through the sound driver's file, /dev/snd: IO_CTL codes, and register/value pairs to write (the
; sound library)
lib_begin LIBN_SOUND
;
; ( -- )  clear the YM2151 (and stop the test tune)
def_far "sndinit", "sndinit"
;
; ( -- )  play the test tune, in the background: it goes on while you do other things (sndstop ends it)
def_far "sndtest", "sndtest"
;
; ( -- )  stop the test tune
def_far "sndstop", "sndstop"
;
; ( xxaa -- f )    send byte(a) to register(x) on yamaha 2151: f = true if it went
def_far "ywrite", "ywrite"
.pushseg
.segment "FORTH_TOP"    ; (Page 1's room above COMMON, $FE00)
; ( p ch -- )  load patch p (0-127: General MIDI's instruments; 128-162: drum sounds) into channel ch (0-7)
def_far "patch", "patch"
;
; ( n ch -- )  play MIDI note n (60: middle C) on channel ch
def_far "note", "note"
;
; ( ch -- )  key channel ch off
def_far "noteoff", "noteoff"
;
; play song [n] [&]  play a ZSM song (and its loop n more times; 0: forever), in a task of its own
def_far "play", "play"
.popseg
lib_end
.endif
;
;
;----------------------------------------------------------------------------
HYWORDS_END:
;  end hywords.s
;----------------------------------------------------------------------------