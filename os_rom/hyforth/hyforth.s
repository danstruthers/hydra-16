;----------------------------------------------------------------------
;
;  Patrick Struthers - March 2026
;      - THANKS to AGSB for the starting point here....
;
;  The HyForth project starts here with AGSB's core Forth engine;
;  the engine will be moved to ROM and the heap will be copied to RAM
;  on cold start.
;
;  The ulitmate purpose of working this port through is to develop a
;  flexible and powerful operating system for the Hydra-16 of reasonable
;  efficiency and minimal memory footprint.  HyForth will grow and
;  shrink in RAM footprint according to context.
;
; ---------------------------------------------------------------------
;
.debuginfo
.setcpu "65C02"

;---------------------------------------------------------------------
; macros for dictionary, creates code as follows:
;
;   h_name:
;   .word  link_to_previous_entry
;   .byte  strlen(name) + flags
;   .byte  name
;   name:
;
; label for primitives
;
.macro makelabel arg1, arg2
.ident (.concat (arg1, arg2)):
.endmacro
;
; header for primitives
; the entry point for dictionary is h_~name~
; the entry point for code is ~name~
;
.macro def_word name, label, flag
makelabel "h_", label
.ident(.sprintf("H%04X", hcount + 1)):
  .word .ident (.sprintf ("H%04X", hcount))
hcount .set hcount + 1
  .byte .strlen(name) + flag + 0 ; nice trick !
  .byte name
makelabel "", label
.endmacro
;---------------------------------------------------------------------
;  macros for PGS stuff
;
.macro WCRLF_np                ; no push of A
    PRINT_CRLF
.endmacro

.macro WCRLF
    pha
    WCRLF_np
    pla
.endmacro

.macro  WSEQ_np  strlbl
    phy
    ldy #0
:
    lda strlbl, y
    beq :+
    iny
    PRINT_CHAR
    bra :-
:
    WCRLF_np
    ply
.endmacro


.macro  WSEQ_raw  strlbl
    phy
    ldy #0
:
    lda strlbl, y
    beq :+
    iny
    PRINT_CHAR
    bra :-
:
    ply
.endmacro

.macro WSEQ  strlbl
    pha
    WSEQ_np  strlbl
    pla
.endmacro

;
;   ANSI screen stuff
; <rr>;<cc>f move cursor to rr,cc
; <cc>m for color/attributes
; 2J for clear screen
; H for 'home'
; <cc>[ABCD] screen moves
;
.macro ANSI b1,b2,b3,b4,b5,b6,b7
    pha
    PRINT_ANSI_ESC_SEQ b1, b2, b3, b4, b5, b6, b7
    pla
.endmacro

;---------------------------------------------------------------------
;  for error messages
;
.macro ERR_entry errmsg
    .byte <errmsg
    .byte >errmsg
    emcount .set emcount + 1
.endmacro

.macro WERR errptr
;.macro WERR
    .local werrloop, werrend
    WCRLF_np
    ldy #0
    lda (errptr),y
    sta TEMP0
    iny
    lda (errptr),y
    sta TEMP0+1
    ldy #0
werrloop:
    lda (TEMP0),y
    beq werrend
    iny
    PRINT_CHAR
    bra werrloop
werrend:
    WCRLF_np
    rts
.endmacro

;---------------------------------------------------------------------
; variables for macros

hcount .set 0
emcount .set 0             ; # of error messages set up

H0000 = 0

;---------------------------------------------------------------------
;               CONFIGURATION OPTIONS
;---------------------------------------------------------------------

; number conversion
numbers := 1      ; include DEC/BIN/HEX conversion

SINGLE := 1     ; single digits hard coded?

DEBUG := 1        ; enable inclusion of debug code

HYWORDS := 1      ; add in additional hardcoded words / logic

ANSIOK := 1         ; add ANSI screen stuff

YSOUND := 1        ; add sound support

;TXT2STACK := 1     ; use old TXTGET instead of new

;---------------------------------------------------------------------
;              for PGS hyforth stuff
;
; error codes
;
ERR_RPTR := $01    ; return stack pointer error
ERR_SPTR := $02    ; data stack pointer error
ERR_DIV0 := $03    ; divide by zero
ERR_MEM := $04     ; memory not-avail
ERR_UKW := $05     ; unknown word
ERR_SEC := $06     ; security error, ie dangerous address write
ERR_SYS := $07     ; return from system call error
  ; warnings
WRN_SEC := $86     ; security
WRN_MEM := $84     ; memory alloc
;
;---------------------------------------------------------------------
;                 CORE ENGINE CONSTANTS
;
CELL = 2         ; cell size, two bytes, 16-bit
FLAG_IMM = 1<<7  ; immediate flag
FLAG_COM = 1<<6  ; compiled flag
MAXSTR = 100

; terminal input buffer, forward
; getline, token, skip, scan, depends on page boundary
; INBUF = $0400  (see segment STACKS below)
; moves forwards
INBUF_end = $FD

; data stacks
; moves backwards, push decreases before copy
DSEND = $7E

; return stack
; moves backwards, push decreases before copy
RTEND = $FE

; malloc stack
; moves backwards
MEMEND = $01FE
MEM_SZ = $04

;
; spi
;F9E8 SPI_INIT_DELAY
;F9CC SPI_RECV
;F9AD SPI_SEND
;F9A3 SPI_OPERATION_DONE
;F974 SPI_TRANSCEIVE

ALTBUF = $6000
ALTBUF_end = $6FFF
MEMTOP = $8000            ; malloc allocates DOWN from MEMTOP

;----------------------------------------------------------------------
;       Look closely at hyforth.cfg and the output of ca65/ld65 after
;  a build; the RAM and ROM code is carefully arranged when the binary
;  is created, to make initialization easier and optimize RAM use.
;  The main program engine starts at $A000 in ROM and is only about 500 bytes
;  long.  Additional functions, then initialization and debug code,
;  then the dictionary follow in the binary, and these stay in ROM.
;
;       The dictionary has a particular structure, with 'bye' and 'abort'
;  at the beginning; there are some core words,
;  then contents of 'primitives.s', then 'hywords.s', followed by
;  the end of the dictionary with critical items such as 'fetch', 'store',
;  'immediate', 'compile, 'semis', 'exit, and ancillary stuff.
;
;       When $A003 is run from WozMon, a jump instruction to 'main' is
;  copied to $0600, followed by the dictionary, with 'exit' at the end.
;  HyForth is start by running '600R' or 'A000R'.
;
;----------------------------------------------------------------------
;                   ZERO PAGE USAGE
;----------------------------------------------------------------------
.segment "ZEROPAGE"
.org $C8
ZPSTART:
;
;                   HyForth setup stuff
;
TEMP0:                        ;  DUMPREG         $C8
   .res 2
TEMP8:                        ;  Hstring macro   $CA
   .res 2
TEMP9:                        ;   Imm            $CC
   .res 2
MEMPTR:                       ;   malloc         $CE
   .res 2
MEMLAST:
   .res 2                      ;  malloc         $D0
TIB:
   .res 2          ; pointer to input buffer     $D2
TIBEND:
   .res 2          ; pointer to end of TIB       $D4
DFLAG:
   .res 1          ; debug flag                  $D6
ERRFLAG:
   .res 1          ; error type, 0 = none        $D7
ERRPTR:
   .res 2          ; ptr to mitigation/message   $D8
DIGBASE:
   .res 1          ; base for number conversion  $DA
RSEED:
   .res 4          ; random # seed               $DB
ALFLAG:            ; autoload flag               $DF
   .res 1
;
;                   internal Forth
;
STATUS:     .word $0   ; state at lsb, last size+flag at msb   $E0
CURBUF:     .word $0   ; CURBUF next free byte in TIB          $E2
LASTHEAP:   .word $0   ; last link cell                        $E4
NEXTHEAP:   .word $0   ; next free cell in heap dictionary     $E6
;
;                   pointer registers
;
DSPTR:      .word $0   ; data stack pointer                    $E8
RTPTR:      .word $0   ; return stack pointer                  $EA
INSTPTR:    .word $0   ; instruction pointer                   $EC
WORKREG:    .word $0   ; working register                      $EE
;
;                    TEMP1 - 4
;
mainoff:               ; used for COPYTORAM, MEMCPY
TEMP1:    .word $0     ; first                                 $F0
endsoff:               ; used for COPYTORAM, MEMCPY
TEMP2:    .word $0     ; second                                $F2
ramstart:              ; used for COPYTORAM, MEMCPY
TEMP3:    .word $0     ; third  (two bytes)                    $F4
TEMP4:    .word $0     ; fourth  (two bytes)                   $F6
;
;          NXTTOK, BACKHEAP, TEMP5 - 7
;
NXTTOK:     .word $0   ; next token in tib (INBUF)             $F8
BACKHEAP:   .word $0   ; hold 'here while compile              $FA
supprint:              ; suppress printing in MEMCPY
FFLAG:                 ; 'find flag' for wfind entry point
TEMP5:   .res 1        ;    WORDS, WFIND                       $FC
TEMP6:   .res 1        ;    WORDS, DIGCONT, TEXTGET            $FD
TEMP7:   .res 2        ;    AUTOLOAD                           $FE
;
; *** $DO-$FF total usage in ZP, including TEMP vars ***
;
;   HYDRA-16 serial/read buffers at $200 and $300 so skip those
;
;----------------------------------------------------------------------
;                   FORTH STACKS
;----------------------------------------------------------------------
.segment "BUFFERS"
INBUF:
      .res 256
DS:                          ; data stack (S)
      .res 126
      .res 2
RT:                          ; return stack (R)
      .res 126
      .res 2
MEMSTK:                      ; memory manager
      .res 510
      .res 2
;
;
.segment "FORTH_ROM"              ; regular core
;
;
; ************ the real deal...
;

forth_main:
    jmp cold
    jmp COPYTORAM

HYPROMPT:
    .byte $0D, $0A
    .byte "HF>"
    .byte 0
;
.ifdef DEBUG
WDISP:
    .byte $0D, $0A
    .byte "W="
    .byte 0
WATDISP:
    .byte "  [W]="
    .byte 0
.endif
;
;
;
cold:
    cld
    jsr CLEAR          ; zero out zero page, INBUF, DS, and RT

warm:
; link list of headers
    lda #>h_exit               ; initialize HEAP pointers
    sta LASTHEAP + 1
    lda #<h_exit
    sta LASTHEAP

; next heap free cell
    lda #>ends + 1
    sta NEXTHEAP + 1
    stz NEXTHEAP
    stz ERRFLAG                ; clear ERROR flag


    ldy #>(MEMSTK+MEMEND)       ; initialize memory manager area
    sty MEMPTR + 1
    ldy #<(MEMSTK+MEMEND)
    sty MEMPTR
    ldy #<MEMTOP
    sty MEMLAST
    ldy #>MEMTOP
    sty MEMLAST+1

;---------------------------------------------------------------------
; various reinitialization points
;
reset:
    ldy #INBUF_end
    sty TIBEND
    ldy #<INBUF
    sty TIB
    ldy #>INBUF
    sty TIB+1
    sty TIBEND+1
    sty CURBUF+1
    sty NXTTOK+1

    ldy #>DS                     ; DS and RT are now half page each
    sty DSPTR + 1
    ldy #>RT
    sty RTPTR + 1

    lda #1                       ; DEBUG OFF by default
    sta DFLAG
    stz ALFLAG                   ; autoload flag OFF

abort:                            ; clear DS
    ldy #<DSEND
    sty DSPTR

errrtn:                          ; return from error
quit:                             ; clear RT
    ldy #<RTEND
    sty RTPTR

    jsr wrterror                 ; print any error messages
    ldy #0          ; reset INBUF
    lda #0
    sta (TIB),y     ; clear INBUF stuff
    stz CURBUF    ; clear cursor  (pointer into INBUF)
    stz STATUS    ; status is 'interpret' == \0

    .byte $2c       ; mask next two bytes, nice trick !
;---------------------------------------------------------------------
; the outer loop

resolvept:
    .word okey
;---------------------------------------------------------------------
okey:               ; well shit, I hope this is easy....
resolve:           ; get a token
    jsr token      ; then just process the regular way
.ifdef DEBUG
    lda DFLAG                 ; DEBUG
    bne RVPSKIP
    WCRLF_np
    lda #'P'
    PRINT_CHAR            ; DEBUG
.endif

RVPSKIP:

RESFIND:                ; load last or 'latest' word on heap
    lda LASTHEAP + 1
    sta TEMP2 + 1
    lda LASTHEAP
    sta TEMP2

RESLOOP:              ; lsb linked list
    lda TEMP2
    sta WORKREG             ; so 'last' -> W
    ora TEMP2+1             ; only zero if both are zero
    bne RESEACH              ; PGS - did he forget this?

WORDNOTFOUND:
    lda #ERR_UKW           ; UNKNOWN WORD error
    sta ERRFLAG
    jmp errrtn ; end of dictionary, no more words to search, abort

RESEACH:                        ; msb linked list
    lda TEMP2 + 1
    sta WORKREG + 1           ; update next link

    ldx #WORKREG
    ldy #TEMP2
    jsr copyfrom                  ; W += 2, now pointing at size/flag byte from 'here'
    ldy #0              ; compare words
    lda (WORKREG), y    ; save the flag, first byte is (size and flag)
    sta STATUS + 1
            ;; *** start of mod for bit check
    and #$3F            ; mask off flags
    sec
    sbc (NXTTOK), y    ; compare lengths
    bne RESLOOP
    iny
; compare chars
RESEQUAL:
    lda (NXTTOK), y
    cmp #ASCII_SPACE            ; space ends
    beq RESDONE
    sec                 ; verify
    sbc (WORKREG), y
    asl                 ; clean 7-bit ascii
    bne RESLOOP
    iny                 ; get next char
    bne RESEQUAL

RESDONE:
    tya                ; increment W by y, W will point at CFA?
    jsr addwx

eval:
; executing ? if status = 0
    lda STATUS
    beq execute
;
; falls thru on compile, but...
; immediate ? if status+1 < 0 (bit seven set)
    lda STATUS + 1
    bmi immediate

compile:          ; otherwise compile
.ifdef DEBUG
    lda DFLAG          ; DEBUG, print C if here
    bne CMPSKIP
    WCRLF_np
    lda #'C'
    PRINT_CHAR
CMPSKIP:
.endif
    jsr wcomma          ; copy W into NEXTHEAP ('here'), increment NEXTHEAP
    bcs immediate
    jmp resolve         ; if not 'immediate' go on to next token
;
immediate:
execute:

.ifdef DEBUG
    lda DFLAG         ; DEBUG, print E if here
    bne EXESKIP
    WCRLF_np
    lda #'E'
    PRINT_CHAR
EXESKIP:
.endif
    lda #>resolvept     ; set up INSTPTR to run,
    sta INSTPTR + 1     ; or return to interpreter.
    lda #<resolvept
    sta INSTPTR
    jmp pick             ; almost done, 'next' and either ENTER or EXEC

;-----------------------START PROCESSING INPUT-----------------------
try:
    lda (TIB), y                   ; index is in y
    beq getline    ; if \0  - get a line if pointing at 0
    iny
    eor #ASCII_SPACE    ; return 0 in  A if a space
    rts

;--------------------GET AN INPUT LINE ENDING WITH CR/LF ------------
getline:   ; drop rts of try, fall through to 'token'
    pla
    pla
;
;   DO AUTOLOAD HERE
;      load a space, then copy next line to buffer
;      calc y (length + 1) jump to GETLNEND
;
    lda ALFLAG
    beq GLNORMAL
    jsr ALOADTIB
    jmp GETLNSKIPCRLF

GLNORMAL:
    WSEQ_raw HYPROMPT    ; print prompt
;
    ldy #0   ; leave the first
GETLOOP:
    sta (TIB), y  ; dummy store on first pass, overwritten
    iny
    cpy TIBEND
    beq GETLNEND
    cpy #$FF
    bne GETREADLOOP
    ldy #1
GETREADLOOP:
    jsr READ_CHAR
    bcc GETREADLOOP
    cmp #ASCII_CR
    beq GETLNEND
    cmp #ASCII_BACKSPACE         ; handle backspace
    bne GETLOOP
    dey
    dey
    lda (TIB), y      ; make sure prev char not overwritten
    bra GETLOOP
GETLNEND:                ; clear all if y eq \0
    PRINT_CRLF
GETLNSKIPCRLF:          ; SKIP to here if don't want CRLF
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

;---------------------------------------------------------------------
; in place every token,
; the counter is placed at last space before word
; no rewinds
token:
    ldy CURBUF   ; last position on INBUF

TOKENSKIP:   ; skip spaces
    jsr try
    beq TOKENSKIP
    dey   ; keep y == <start of input word> + 1
    sty NXTTOK

TOKENSCAN:  ; scan spaces
    jsr try
    bne TOKENSCAN
    dey   ; keep y == <end of input word> + 1
    sty CURBUF

TOKENDONE:  ; find size and store it;
    tya
    sec
    sbc NXTTOK
    ldy NXTTOK    ; keep it
    dey
    sta (TIB), y  ; store size for counted string
    sty NXTTOK
    ;
    ;  During interpretive mode at least...do number and string capture here, before
    ;     looking at word list; will be pushed on stack.
    ;  This SHOULD have a general digit converter (any base up to 16); return C = 1 if no conversion
    ;   if SINGLE is defined, this will skip single digit numbers and use hardcoded ones.
    ;
    ;  Following check for #'s, check for quoted inline txt a la 'q^....^'
    ;
.ifdef numbers
    ldy #0
    lda (NXTTOK),y
    tax                ; store size in X, pass to conversion
    jsr DIGCONVT
    bcs CHKFERTXT       ; if some error in conversion, skip and continue processing
    bra TOKCLR0
.endif  ; 'numbers'
CHKFERTXT:
    jsr TEXTGET         ; returns length +4 in X
    bcs TOKENEND
    ldy #0
    lda #ASCII_SPACE
    jmp TOKCLR
TOKCLR0:
    ldy #0
    lda (NXTTOK),y     ; load length again
    tax
    inx
    lda #ASCII_SPACE          ; copy spaces over entire converted string
TOKCLR:
    sta (NXTTOK),y
    iny
    dex
    bne TOKCLR
TOKNEXT:
    jmp token               ; and use 'token' to rebuild input buffer w/o converted #


TOKENEND:
    clc     ; clean - setup token
    rts
;
;--------------------UTILITIES----------------------------------------
;    A whole bunch of helper functions;
;  wcomma / comma increment WORKREG (or another reg) and NEXTHEAP ('here')
;  while copying into NEXTHEAP, uses incwx.
;
;  copyinto / copyfrom copy AND increment/decrement
;  spush/rpush - pushes something onto stacks indexed on ZP by Y (increment using addwx/incwx)
;  spull/rpull - pulls from stacks, copies into ZP indexed by Y (decrement builtin)
;
;  addwx/incwx/decwx do some of the inc/dec-rementing duties
;
;---------------------------------------------------------------------
;
;   COMMA allocates memory at top of heap for 'other stuff'
;   and makes sure heap and working reg pointers are updated.
;
; heap linked list (moves forward)
;
wcomma:
    ldy #WORKREG                  ; copy addr at WORKREG, change addr fld NEXTHEAP points to
comma:
    ldx #NEXTHEAP                 ; Y has source of address, change addr fld NEXTHEAP points to
    ; FALL THROUGH - copyinto, then rts after second incwx
;---------------------------------------------------------------------
; from a page zero address indexed by Y
; into a page zero indirect address indexed by X
;
copyinto:
    lda 0, y
    sta (0, x)
    jsr incwx
    lda 1, y
    sta (0, x)
    jmp incwx                        ; incwx ends with rts!
;---------------------------------------------------------------------
;
; generics - PUSH, PULL, incwx/addwx, copyfrom
;
;------------------------PUSH a cell--------------------------------
spush_2:
    ldy #TEMP3       ; push TEMP3 on top
    jmp spush
spush_1:
    ldy #TEMP2       ; push TEMP2 on top
    jmp spush
spush_0:             ; push TEMP1 to stack, probably top of stack
    ldy #TEMP1
     ; FALL THROUGH
;---------------------------------------------------------------------
; PUSH a cell
; from a page zero address indexed by Y
; into a page zero indirect address indexed by X
spush:
    ldx #DSPTR
    lda DSPTR
    cmp #<DS                ; ditto
    beq ptrerr_s              ; ditto
    jmp push
rpush:
    ldx #RTPTR
    lda RTPTR               ; ditto
    cmp #<RT                ; ditto
    beq ptrerr_r

    ; FALL THROUGH
;---------------------------------------------------------------------
; classic stack backwards
push:
    jsr decwx
    lda 1, y
    sta (0, x)
    jsr decwx
    lda 0, y
    sta (0, x)
    rts
;
;                      pointer error (DS or RT)
ptrerr_r:                      ; pop jsr off stack, throw error
    lda #ERR_RPTR
    bra ptrerr_cont
ptrerr_s:
    lda #ERR_SPTR
ptrerr_cont:
    sta ERRFLAG
    pla
    pla
    jmp errrtn
;
;---------------- PULL a cell, with convenience for TEMP 1/2/3 -----
;
spull_2:
    ldy #TEMP3              ; pull TEMP3 from top
    jmp spull

spull_1:                    ; pull TEMP2 from top
    ldy #TEMP2
    jmp spull

spull_0:
    ldy #TEMP1             ; pull TEMP1 from top of DS
;
;  FALL THROUGH
;
; PULL a cell
; from a page zero indirect address indexed by X
; into a page zero address indexed by y
;
spull:
    ldx #DSPTR
    lda DSPTR         ; pointer bounds checking
    cmp #DSEND        ; ditto
    beq ptrerr_s      ; ditto
    jmp pull          ; pull includes rts from incwx so....
rpull:                ;
    ldx #RTPTR
    lda RTPTR        ; pointer bounds checking
    cmp #RTEND        ; ditto
    beq ptrerr_r        ; ditto
;
;  FALL THROUGH
;---------------------------------------------------------------------
;
; from a page zero indirect address indexed by X
; into a page zero address indexed by y
pull:
copyfrom:
    lda (0, x)
    sta 0, y
    jsr incwx      ; NOTE:  not a jmp, returns here.
    lda (0, x)
    sta 1, y
 ;
 ;  FALL THROUGH
;---------------------------------------------------------------------
; increment a word in page zero. offset by X
;
;   Usage: ldx #<pointer name> and then jsr incwx/addwx
;
;   THESE functions are SOLELY intended to increment/decrement
;   zeropage pointers, and nothing else.  Offsets indexed by
;   X are INTO the zeropage space, not relative to a specific
;   pointer location.
;
incwx:
    lda #01
;---------------------------------------------------------------------
; add a byte in A to a word in page zero. offset by X
addwx:
    clc
    adc 0, x
    sta 0, x
    bcc addwx_end
    inc 1, x
    clc      ; keep carry clean.
             ; our convention is that functions SET the carry for positive results,
             ; clear carry for negative or neutral ones.
addwx_end:
    rts

;---------------------------------------------------------------------
;          decwx, heap moves
; decrement a word in page zero. offset by X
;
;   Usage: ldx #<pointer name> and then jsr decwx
;
;   THESE functions are SOLELY intended to increment/decrement
;   zeropage pointers, and nothing else.  Offsets indexed by
;   X are INTO the zeropage space, not relative to a specific
;   pointer location.
;
decwx:
    lda 0, x
    bne decwx_end
    dec 1, x
decwx_end:
    dec 0, x
    rts
;
;
;
ENGINEEND:
;                           END OF CORE ENGINE
;----------------------------------------------------------------------
;
;                        DICTIONARY and ADDITIONS
;
;    upper.s -- error messaging, CLEAR on start, debug, utilities
;------------------------------------------------------------------------
;
upper_ram:
   .include "upper.s"
UPPER_END:
;
;
YSOUND_START:
;.ifdef YSOUND
;   .include "sound_os.s"
;.endif
YSOUND_END:
;
;
ROMCODEEND:                         ; end of all code
;
;  end of hyforth.s
;
;-----------------------------------------------------------------------
    .res 16                    ; just a visible buffer in binary
                               ; to make easier to identify different
                               ; code segments.
;-----------------------------------------------------------------------
;                            BELOW ENDS UP IN RAM
;-----------------------------------------------------------------------
.segment "FORTH_PAGED_ROM"
;------------------------------------------------------------------------
;
;    hyf_rom.s -- COPYTORAM
;
.include "hyf_rom.s"
;
;
COPYSTART := $A100              ; marks beginning of copy in ROM space
.segment "FORTH_CODE"
RAMSTART:
    jmp forth_main             ; MAIN program start
    jmp COPYTORAM
;---------------------------------------------------------------------
;        primitives.s -- original AGSB hardcoded dictionary
;
primitives:
.include "primitives.s"
;
;
;------------ CRITICAL CORE PRIMITIVES (AGSB and PGS) ----------------
;
;   COMPILE (:), FINISH (;), FETCH (@), STORE (!)
;      including:  keeps, this, next, finish
;       ...and other stuff.
;
;---------------------------------------------------------------------
; ( a -- ) execute a jump to a reference at top of data stack
def_word "exec", "exec", 0
    jsr spull_0
    jmp (TEMP1)        ; assumes an address on top of DS

;---------------------------------------------------------------------
; ( -- ) execute a jump to a reference at IP
def_word ":$", "docode", 0
    jmp (INSTPTR)                 ;  DOCODE (thus the ':')

;---------------------------------------------------------------------
; ( -- ) execute a jump to next
def_word ";$", "donext", 0        ;  'next' (thus the ';')
    jmp next
;---------------------------------------------------------------------
;
; ( w a -- ) ; [a] = w    (w is word, a is an address)
def_word "!", "store", 0
storew:
    jsr spull_1             ; get address, store in TEMP2
    jsr spull_0             ; get data, store in TEMP1
    ldx #TEMP2              ;  [a]
    ldy #TEMP1              ;   w
    jsr copyinto            ; copy TEMP2 stuff to addr in TEMP1 (opposite of @)..
    jmp next                ;            ...(see below)
;
;--------------------------------------------FETCH--------------------
; ( a -- w ) ; w = [a]
def_word "@", "fetch", 0      ; replace addr of data on top of DS, with data pointed to
fetchw:
    jsr spull_0             ; get addr from DS
    ldx #TEMP1
    ldy #TEMP2
    jsr copyfrom            ; copies data from [TEMP1] => TEMP2
;---------------------------------------------------------------------
;            NEXT entry point for many AGSB primitives
;---------------------------------------------------------------------
copys:                      ; copy from cell at y (zp) to TEMP1
    lda 0, y
    sta TEMP1
    lda 1, y
keeps:                      ; saves bytes since have to get here anyway
    sta TEMP1+1
this:                       ; same as above
    jsr spush_0             ; then push back on stack
    jmp next
;
;-----------------------IMMEDIATE, '[', ']', ','-----------------------
def_word "I", "Imm", 0
wimm:                        ; jmp here if proccessing a compiled
    lda LASTHEAP+1           ;   word that needs to run 'immediate'.
    sta TEMP9+1
    lda LASTHEAP             ; get addr of 'last' compiled word, copy to TEMP4, add 2
    clc
    adc #2                   ; ..to find where length byte is...
    sta TEMP9
    bcc IMMSKIP
    inc TEMP9+1
IMMSKIP:
    ldy #0
    lda (TEMP9),y
    ora #$80                 ; ...set bit 7 and store
    sta (TEMP9),y
    jmp next
;
def_word "[", "leftbrack", FLAG_IMM       ; switch to 'interpret'
    stz STATUS
    jmp next
;
def_word "]", "rtbrack", 0               ; switch back to 'compile'
    lda #1
    sta STATUS
    jmp next
;
;
                                         ; NOTE: 'lit' or similar will have pushed a value
                                         ; or address onto stack; 'comma' stores it inline in word
                                         ;
def_word ",", "xcomma", 0                ; pull data from top of stack, store at 'here', adjust
    jsr spull_0                          ; 'here' to point at next cell
    ldy #0
    lda TEMP1
    sta (NEXTHEAP),y                     ; POP DS TO TEMP1, [here] = TEMP1
    iny
    lda TEMP1+1
    sta (NEXTHEAP),y
    ldx #NEXTHEAP                        ; here += 2
    lda #2
    jsr addwx
    jmp next

;--------------------------------------------SEMIS-----------------
def_word ";", "semis",  FLAG_IMM
    lda BACKHEAP
    sta LASTHEAP                ; bring back BACKHEAP to LASTHEAP
    lda BACKHEAP + 1
    sta LASTHEAP + 1

    stz STATUS                  ; set status to 'interpret' (presumably from 'compile')

finish:                         ; compiled words must end with exit
    lda #<exit
    sta WORKREG
    lda #>exit
    sta WORKREG + 1
    jsr wcomma                  ; change NEXTHEAP to point to addr of 'exit',
                                ; and make sure is last entry in code table for word...
                                ; as all good Forth compiled words should do.
    jmp next
;
;
;------------------------------------------COMPILE------------------
def_word ":", "colon", 0
    lda NEXTHEAP
    sta BACKHEAP                ; backup NEXTHEAP to BACKHEAP
    lda NEXTHEAP + 1
    sta BACKHEAP + 1

    lda #1                      ; set status to 'compile'
    sta STATUS

COMPHEADER:
; copy LASTHEAP into (NEXTHEAP)
    ldy #LASTHEAP
    jsr comma                    ; change NEXTHEAP to point to LASTHEAP  ('here' <= 'last')
    jsr token                    ; get first token, the name of new word
    ldy #0                       ; copy it to heap: length and name
                                 ; code field comes with later proc
COMPLOOP:
    lda (NXTTOK), y
    cmp #ASCII_SPACE
    beq COMPEND
    cpy #0                       ; if this is the length field...
    bne COMCOPY
    ora #$40                     ; set bit six for 'compiled' word
COMCOPY:
    sta (NEXTHEAP), y
    iny
    bne COMPLOOP
COMPEND:
    tya                          ; and update NEXTHEAP  :  'here' incremented by length
    ldx #(NEXTHEAP)
    jsr addwx                   ; 'here' now at CFA

;~~~~~~~~ all done....
    jmp next                     ; and then see below; compiled word will continue until ';'
;
;---------------------ADD IN EXTRA HARDCODED LOGIC / NUMERALS / ETC--
.ifdef HYWORDS
   .include "hywords.s"
.endif
;---------------------------------------------------------------------
; Thread Code Engine
;
;   INSTPTR is IP, WORKREG is W
;
;     unnest, next, pick, nest, and jump
;
;   nest aka ENTER or DOCOL  (do colon?)
;   unnest aka EXIT or semis?
;
;---------------------------------------------------------------------
; ( -- )
def_word "exit", "exit", 0
    jmp EXIT

; mark end of ROM IMAGE
;
;.byte "\CODEEND/"
;.byte 0
;-----------------------------------------------------------------------
; BEWARE, MUST BE AT END! MINIMAL THREAD CODE DEPENDS ON IT!
;
ends:                            ; end marker of hardcoded primitives
;
;-----------------------------------------------------------------------
;
;

;-----------------------------------------------------------------------
;                            TRAINING DATA
;
; include training data

.byte "***HEAPEND***"
;.org $C000
    .byte "FTRAIN"
    .byte 0,0
.include "ftrain.s"
;
;                            BINARY LOAD
;
    .res 16
    .byte "BLOAD"
    .byte 0,0
.include "bload.s"
;
;  end of hyforth.s
;
