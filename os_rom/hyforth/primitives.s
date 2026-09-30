;
; primitives.s
;
;
;----------------------------------------------------------------------
; ( -- ) ae exit forth
def_word "bye", "bye", 0
    lda BATCH                   ; a copy of the shell (run's, a pipeline's): end its task
    beq BYEMON
    ldx CHILDSP
    txs
    rts
BYEMON:
    jmp MON_START               ; jump to WOZMON

;----------------------------------------------------------------------
; ( -- ) ae abort
def_word "abort", "abort_", 0
    jmp abort

; ( -- ) resets buffer to INBUF
def_word "reset", "reset_", 0
    jmp reset

;----------------------------------------------------------------------
; ( -- ) ae list of data stack
def_far ".S", "splist"          ; changed from %S

; ( -- ) data stack empty?
def_word "?S", "spp", 0
    stz TEMP1+1
    stz TEMP1
    lda DSPTR
    cmp #DSEND
    bne EMPEND
EMPCONT:
    lda #$FF
    sta TEMP1
    sta TEMP1+1
EMPEND:
    jsr spush_0
    jmp next

; ( -- ) return stack empty?
def_word "?R", "rtp", 0
    stz TEMP1+1
    stz TEMP1
    lda RTPTR
    cmp #RTEND
    bne EMPEND
    jmp EMPCONT

;----------------------------------------------------------------------
; ( -- ) list of return stack
def_far ".R", "rplist"           ; changed from %R

;------------------------------- ODUMP AND DUMP ---------------------------------------
; ( -- ) dumps the user dictionary
;    ( REMOVED as of 4/19/26 )

; ( addr pages -- ) dump memory (the tools library)
lib_begin LIBN_TOOLS
def_word "dump", "dump", 0
    jsr spull_1             ; TEMP2 LSB is # of pages
    jsr spull_0             ; TEMP1 has starting addr
    lda TEMP1+1
    sta TEMP0+1
    ldx TEMP2
    bne FDUMPLOOP
    ldx #1                  ; always print at least one page
FDUMPLOOP:
    jsr DUMPPAGE
    inc TEMP0+1
FDUMPGET:
    jsr GET_CHAR            ; wait for char to continue to next page
    bcc FDUMPGET
    dex
    bne FDUMPLOOP
    clc
    jmp next
lib_end

;------------------------------ WLIST -----------------------------------
;
; ( -- ) clean word list,
def_word "words", "words", 0

; load LASTHEAP
    lda LASTHEAP + 1
    sta TEMP2 + 1
    lda LASTHEAP
    sta TEMP2
    lda LIBSET          ; (then the RAM libraries, the base, and the libraries: LIB_NEXT)
    sta LIBLEFT
    lda LIBSET2
    sta LIBLEFT2

; load NEXTHEAP
    lda NEXTHEAP + 1
    sta TEMP3 + 1
    lda NEXTHEAP
    sta TEMP3
    stz TEMP5
    dec TEMP5

WORD_LOOP:
    inc TEMP5       ; increment 'words per line' count
    lda TEMP2       ; lsb linked list
    sta TEMP1
    ora TEMP2 + 1    ; check if TEMP2 = $0000, end if so
    beq WORD_END
    lda TEMP2 + 1    ; msb linked list
    sta TEMP1 + 1

    lda TEMP5        ; check word count, CRLF if 4th one
    cmp #4
    bne WORD_SKIP
    WCRLF_np
    stz TEMP5

WORD_SKIP:               ; TEMP1 has address of work-record,
; put address            ; (TEMP1) is link to previous record
    PRINT_SPACE
    lda TEMP1 + 1
    PRINT_BYTE
    lda TEMP1
    PRINT_BYTE

    ldx #TEMP1          ; advance TEMP1 to size + flag, name
    lda #2
    jsr addwx
    ldy #0
    jsr WRDSHOWNAME        ; put size + flag, name

    lda #10
    sec
    sbc TEMP6
    tax
SPCLOOP:
    PRINT_SPACE
    dex
    bne SPCLOOP
    PRINT_CHAR #ASCII_PIPE
    iny                  ; update TEMP1 again, point at CFA
    tya
    ldx #TEMP1
    jsr addwx
    lda TEMP3           ; instead of printing refs, just advance TEMP1 to TEMP3
    sta TEMP1
    lda TEMP3+1
    sta TEMP1+1

WORD_CONT:
    lda TEMP2            ; update TEMP2 and TEMP3, advance in linked list
    sta TEMP3
    lda TEMP2 + 1
    sta TEMP3 + 1
    ldy #0
    lda (TEMP3), y
    sta TEMP2
    iny
    lda (TEMP3), y
    sta TEMP2 + 1
    ldx #(TEMP3)
    lda #2
    jsr addwx
    jmp WORD_LOOP
WORD_END:
    jsr LIB_NEXT     ; the end of a chain: the next library's
    bcs WORD_DONE
    dec TEMP5        ; (the count per line goes on)
    jmp WORD_LOOP
WORD_DONE:
    clc              ; clean return
    jmp next
;
;  routines needed for 'words'
;
; print size and name
WRDSHOWNAME:
    PRINT_CHAR #ASCII_COLON
;    lda (TEMP1), y
;    PRINT_BYTE       ; size

    PRINT_SPACE
    lda (TEMP1), y
    and #$3F           ; mask off top two bits
    tax
    sta TEMP6          ; save length of name

NAMELOOP:              ; name
    iny
    PRINT_CHAR {(TEMP1), y}
    dex
    bne NAMELOOP
    PRINT_SPACE
    rts

;----------------------------------------------------------------------
;
;  holdover from original 'words' but keep for now
;
show_refer:
; print references (PFA - parameter field addresses)
;    ldx #(TEMP1)
;
;SHWREFLOOP:
;    PRINT_SPACE
;    lda TEMP1 + 1
;    PRINT_BYTE
;    lda TEMP1
;    PRINT_BYTE
;    PRINT_CHAR #ASCII_COLON
;    iny
;    PRINT_BYTE {(TEMP1), y}
;    dey
;    PRINT_BYTE {(TEMP1), y}
;    lda #2
;    jsr addwx
;
; check if at the end
;    lda TEMP1
;    cmp TEMP3
;    bne SHWREFLOOP
;    lda TEMP1 + 1
;    cmp TEMP3 + 1
;    bne SHWREFLOOP
;    rts
;
;----------------------------------------------------------------------
;  seek for addr of 'exit' at end of sequence of references
;  max of 254 references in list
;
;  removed 'seek', was not used in original code

;----------------------------------------------------------------------
; ( u -- ) print top of DS in hexadecimal in MSB:LSB form, and drop it
def_far ".", "dot"

; ( u -- ) print top of DS in ascii, two bytes, msb first, and drop it
def_far ".C", "cdot"
;
;
def_word "ord", "ord", 0
    jsr token                  ; get first token
    ldy #0
    lda (NXTTOK),y
    clc
    adc #ASCII_0
    PRINT_CHAR
    ldy #1                     ; skip len, first time
ORDLOOP:
    lda (NXTTOK), y
    beq  ORDNONE
    cmp #ASCII_SPACE
    beq  ORDNONE
    sta TEMP1
    stz TEMP1+1
    lda #ASCII_SPACE
    ldy #0
    sta (NXTTOK), y           ; clear it (may be unnecc.)
    iny
    sta (NXTTOK), y
    jsr spush_0
ORDNONE:
    jmp next
;
; (addr -- )  -------  print sz string using new allocated RAM space
;
def_far ".sz", "szdot"
;
; ( addr n -- w0 w1 ... w(n-1) )  push n words from memory to stack
;
def_far "dsgetn@", "dsgetn"
;
; ( addr -- w w ... w )  push words from memory to stack
;
def_far "dsget@", "dsget"
;
; ( w w w..len addr -- addr)    stores len BYTES (words * 2) from stack
;
def_far "dwstk!", "dwstkstore"
;
;
; ( 0c 0c 0c...len addr -- addr)    stores len chars from stack
;
def_far "dcstk!", "dcstkstore"
;
; ( 0c 0c 0c...len addr -- addr)  stores len chars from stack, reverse order
;
def_far "rdcstk!", "rdstkstore"
;
;
;
def_far "decs!", "decstore"
;
;
extensions:
;---------------------------------------------------------------------
; ( w n -- w >> n ) -- shift right
def_word ">>", "shr", 0
    jsr spull_1
    lda TEMP2
    beq SRZERO
    jsr spull_0
SRLOOP:
    lsr TEMP1 + 1
    ror TEMP1
    dec TEMP2
    bne SRLOOP
    jmp this         ; 'this' includes jsr spush_0 and next
SRZERO:
    jmp next

; ( w n -- w << n ) -- shift left
def_word "<<", "shl", 0
    jsr spull_1
    lda TEMP2
    beq SLZERO
    jsr spull_0
SLLOOP:
    asl TEMP1
    rol TEMP1 + 1
    dec TEMP2
    bne SLLOOP
    jmp this         ; 'this' includes jsr spush_0 and next
SLZERO:
    jmp next

;--------------- bit test/set/clear ----------------------------------
; ( n b -- t? )
def_far "tbit", "tbit"                ; nondestructive test bit

; ( n b -- ns )
def_far "sbit", "sbit"                 ; set bit

; ( n b -- nc )
def_far "cbit", "cbit"
;---------------------------------------------------------------------
; start of dictionary
;---------------------------------------------------------------------
core_dict:
;---------------------------------------------------------------------
; ( -- u ) ; tos + 1 unchanged
def_word "key", "key", 0
KEYRDLP:
    jsr GET_CHAR            ; from fd 0 (sleeps until a key comes in)
    stz TEMP1+1
    bcs KEYGOT
    lda #$FF                ; end of file (stdin from a pipe): -1
    sta TEMP1+1
KEYGOT:
    sta TEMP1
    jmp this         ; 'this' includes jsr spush_0 and next

;---------------------------------------------------------------------
; ( u -- ) ; tos + 1 unchanged
def_word "emit", "emit", 0
    jsr spull_0
    lda TEMP1
    PRINT_CHAR
    jmp next

;---------------------------------------------------------------------
; ( w1 w2 -- NOT(w1 AND w2) )
def_word "nand", "nand", 0
    jsr spull_1             ; load TEMP2, TEMP1 from stack
    jsr spull_0
    lda TEMP2
    and TEMP1
    eor #$FF            ; toggles FIRST byte okay, but...
    sta TEMP1
    lda TEMP2 + 1
    and TEMP1 + 1
    eor #$FF           ; and then second.
                       ; sta TEMP1+1 at 'keeps', then jsr spush_0 and 'next'
    jmp keeps

;---------------------PLUS and MINUS----------------------------------
; ( w1 w2 -- w1+w2 )
def_word "+", "plus", 0
    jsr spull_1        ; load TEMP2 from stack
    jsr spull_0        ; then TEMP1
    clc
    lda TEMP2
    adc TEMP1
    sta TEMP1
    lda TEMP2+1
    adc TEMP1+1
                    ; 'keeps' = sta TEMP1+1; jsr spush_0; jmp next
    jmp keeps

; ( w1 w2 -- w1-w2 )
def_word "-", "minus", 0
    jsr spull_1        ; get TEMP 2 from stack
    jsr spull_0        ; then TEMP 1
    sec
    lda TEMP1
    sbc TEMP2
    sta TEMP1
    lda TEMP1+1
    sbc TEMP2+1
                   ; 'keeps' = sta TEMP1+1; jsr spush_0; jmp next
    clc            ; not sure why, but just in case?
    jmp keeps

;---------------------------------------------------------------------
; ( 0 -- $0000) | ( n -- $FFFF)  normalize boolean - change TOS to $0000 if false, $FFFF if TRUE
def_word "bool", "normbool", 0
    jsr spull_0
    lda TEMP1+1
    ora TEMP1    ; only zero if both are zero
    beq PUSHFALSE
    jmp PUSHTRUE
;
; ( -- $FFFF )
def_word "TRUE", "istrue", 0
PUSHTRUE:
    lda #$FF
    sta TEMP1
    jmp keeps
;
; ( -- $0000 )
def_word "FALSE", "isfalse", 0
PUSHFALSE:
    stz TEMP1
    stz TEMP1+1
    jmp this
;---------------------------------------------------------------------
; ( -- state ) pushes addr of status word on stack
def_word "s@", "state", 0
    lda #<STATUS
    sta TEMP1
    lda #>STATUS
    jmp keeps   ; pushes addr of status word on stack?  keeps includes sta TEMP+1 etc.

.ifdef DEBUG
;------------------------ADDED for HyForth debugging-------
; ( -- ) toggle debug
def_word "debug", "debug", 0
    lda DFLAG
    cmp #1
    beq @make0
    lda #1
    bra @store
@make0:
    lda #0
@store:
    sta DFLAG
    jmp next
.endif
;
;------------------------END of primitives.s
;