;----------------------------------------------------------------------
;
;   A MilliForth for 6502 
;
;   original for the 6502, by Alvaro G. S. Barcellos, 2023
;
;   https://github.com/agsb 
;   see the disclaimer file in this repo for more information.
;
;   SectorForth and MilliForth was made for x86 arch 
;   and uses full 16-bit registers 
;
;   The way at 6502 is use page zero and lots of lda/sta
;
;   Focus in size not performance.
;
;   why ? For understand better my skills, 6502 code and thread codes
;
;   how ? Programming a new Forth for old 8-bit cpu emulator
;
;   what ? Design the best minimal Forth engine and vocabulary
;
;----------------------------------------------------------------------
;   Changes:
;
;   all data (36 cells) and return (36 cells) stacks, TIB (80 bytes) 
;       and PIC (32 bytes) are in same page $400, 256 bytes; 
;
;   TIB and PIC grows forward, stacks grows backwards;
;
;   no overflow or underflow checks;
;
;   the header order is LINK, SIZE+FLAG, NAME.
;
;   only IMMEDIATE flag used as $80, no hide, no compile;
;
;   As ANSI Forth 1994: FALSE is $0000 ; TRUE is $FFFF ;
;
;----------------------------------------------------------------------
;   Remarks:
;
;       this code uses Minimal Thread Code, aka MTC.
;
;       use a classic cell with 16-bits. 
;
;       no TOS register, all values keeped at stacks;
;
;       TIB (terminal input buffer) is like a stream;
;
;       Chuck Moore uses 64 columns, be wise, obey rule 72 CPL; 
;
;       words must be between spaces, before and after;
;
;       no line wrap, do not break words between lines;
;
;       only 7-bit ASCII characters, plus \n, no controls;
;           ( later maybe \b backspace and \u cancel )
;
;       words are case-sensitivy and less than 16 characters;
;
;       no need named-'pad' at end of even names;
;
;       no multiuser, no multitask, no checks, not faster;
;
;----------------------------------------------------------------------
;   For 6502:
;
;       a 8-bit processor with 16-bit address space;
;
;       the most significant byte is the page count;
;
;       page zero and page one hardware reserved;
;
;       hardware stack not used for this Forth;
;
;       page zero is used as pseudo registers;
;
;----------------------------------------------------------------------
;   For stacks:
;
;   "when the heap moves forward, move the stack backward" 
;
;   as hardware stacks do: 
;      push is 'store and decrease', pull is 'increase and fetch',
;
;   but see the notes for Devs.
;
;   common memory model organization of Forth: 
;   [tib->...<-spt: user forth dictionary :here->pad...<-rpt]
;   then backward stacks allow to use the slack space ... 
;
;   this 6502 Forth memory model blocked in pages of 256 bytes:
;   [page0][page1][page2][core ... forth dictionary ...here...]
;   
;   At page2: 
;
;   |$00 tib> .. $50| <spt..sp0 $98| <rpt..rp0 $E0|pic> ..$FF|
;
;   From page 3 onwards:
;
;   |$0300 cold:, warm:, forth code, init: here> heap ... tail| 
;
;   PIC is a transient area of 32 bytes 
;   PAD could be allocated from here
;
;----------------------------------------------------------------------
;   For Devs:
;
;   the hello_world.forth file states that stacks works
;       to allow : dup sp@ @ ; so sp must point to actual TOS.
;   
;   The movement will be:
;       pull is 'fetch and increase'
;       push is 'decrease and store'
;
;   Never mess with two underscore variables;
;
;   Not using smudge, 
;       colon saves "here" into "back" and 
;       semis loads "lastest" from "back";
;
;   Do not risk to put stacks with $FF.
;
;   Also carefull inspect if any label ends with $FF and move it;
;
;   This source is hacked for use with Ca65.
;
;----------------------------------------------------------------------
;
;   Stacks represented as (standart)
;       S:(w1 w2 w3 -- u1 u2)  R:(w1 w2 w3 -- u1 u2)
;       before -- after, top at left.
;
;----------------------------------------------------------------------
;
; Stuff for ca65 compiler
;
.pc02
.feature c_comments
.feature string_escapes
.feature org_per_seg
.feature dollar_is_pc
.feature pc_assignment

;---------------------------------------------------------------------
; macros for dictionary, makes:
;
;   h_name:
;   .word  link_to_previous_entry
;   .byte  strlen(name) + flags
;   .byte  name
;   name:
;
; label for primitives
.macro makelabel arg1, arg2
.ident (.concat (arg1, arg2)):
.endmacro

; header for primitives
; the entry point for dictionary is h_~name~
; the entry point for code is ~name~
.macro def_word name, label, flag
makelabel "forth_p_", label
.ident(.sprintf("H%04X", hcount + 1)) = *
.word .ident (.sprintf ("H%04X", hcount))
hcount .set hcount + 1
.byte .strlen(name) + flag ; nice trick !
.byte name
makelabel "", label
.endmacro

.macro COPYFROM zpfrom, zpto
    ldx     #zpfrom
    ldy     #zpto
    jsr     forth_copy_from
.endmacro

.macro COPYINTO zpinto, zpfrom
    ldx     #zpinto
    ldy     #zpfrom
    jsr     forth_copy_into
.endmacro

;---------------------------------------------------------------------
; variables for macros

hcount .set 0

H0000 = 0

; uncomment to include the extras (sic)
 use_extras = 1 

; uncomment to include the extensions (sic)
 use_extensions = 1 

;---------------------------------------------------------------------
/*
NOTES:

    not finished yet :)

*/
;----------------------------------------------------------------------
;
; alias

; Flags
FLAG_IMM  = $80         ; immediate
FLAG_CODE = $40         ; assembly, not word list
FLAG_LIT  = $20         ; literal

; "all in" page $400

; terminal input buffer, forward
; getline, token, skip, scan, depends on page boundary
.segment "BUFFERS"
F_TIB:      .res    224
F_TIB_END:
F_PIC:      .res    32
XF_SP0:     .res    128
F_SP0:                      ; stacks go down
XF_RP0:     .res    128
F_RP0:                      ; stacks go down
start_of_user_words:

.zeropage
; FORTH ZP
; * = $TAKE_ME
; stacks
ZP_F_START:
; as user variables
; sure, order matters for hello_world.forth !

; internal Forth 
ZP_F_STAT: .word   0  ; state at lsb, ZP_F_LAST size+flag at msb
ZP_F_TIN:  .word   0  ; ZP_F_TIN next free byte in F_TIB
ZP_F_LAST: .word   0  ; ZP_F_LAST link cell
ZP_F_HERE: .word   0  ; next free cell in heap dictionary, aka dpt

; pointers registers
ZP_F_SPI:  .word   0
ZP_F_RPI:  .word   0
ZP_F_IPT:  .word   0  ; instruction pointer
ZP_F_WRD:  .word   0  ; word pointer

; classic, free for use
ZP_F_1ST:  .res F_CELL  ; first
ZP_F_2ND:  .res F_CELL  ; second
ZP_F_3RD:  .res F_CELL  ; third
ZP_F_4TH:  .res F_CELL  ; fourth

; used, reserved
ZP_F_TOUT: .word   0  ; next token in F_TIB
ZP_F_BACK: .word   0  ; hold 'here while compile

;----------------------------------------------------------------------
; "What happens in Vegas, stays in Vegas", like in page zero
; but no values here or must be at BSS

;----------------------------------------------------------------------
;.segment "ONCE" 
; no rom code

;----------------------------------------------------------------------
;.segment "VECTORS" 
; no boot code

;----------------------------------------------------------------------

.segment "FORTH"
; ram code here
;
; leave space for page zero, hard stack, 
; and buffer, locals, forth stacks
;
forth_main:
forth_cold_start:
; copy primitives to RAM
    jsr         CLEAR_SCR
    cld

;----------------------------------------------------------------------
forth_warm_start:
; link list of headers
    LOAD_ADDR   forth_p_exit, ZP_F_LAST

; next heap free cell, same as init:
    LOAD_ADDR   start_of_user_words, ZP_F_HERE

;---------------------------------------------------------------------
forth_reset:
    ldy         #>F_TIB
    sty         ZP_F_TIN + 1
    sty         ZP_F_TOUT + 1

forth_abort:
    LOAD_ADDR   F_SP0, ZP_F_SPI

forth_quit:
    LOAD_ADDR   F_RP0, ZP_F_RPI

forth_clean:
; reset F_TIB
    stz         F_TIB
; clear cursor
    stz         ZP_F_TIN
    stz         ZP_F_TOUT
; ZP_F_STAT is 'interpret' == \0
    stz         ZP_F_STAT
    SKIPNEXT2

;---------------------------------------------------------------------
; the outer loop

;---------------------------------------------------------------------
resolvept:
    .word       forth_okay

forth_okay:
    lda         ZP_F_STAT
    bne         forth_resolve
    PRINT_CHAR  #ASCII_O, #ASCII_K
    PRINT_CRLF

forth_resolve:
; get a token
    jsr         forth_token
    ;PRINT_CHAR  #ASCII_P

forth_find:
; load ZP_F_LAST
    WORDCOPY    ZP_F_LAST, ZP_F_2ND

@find_loop:
; lsb linked list
    BYTECOPY    ZP_F_2ND, ZP_F_WRD

; verify \0x0
    ora         ZP_F_2ND + 1
    bne         @find_each

; maybe to place a code for forth_number? 
; but not for now.

; uncomment for feedback, comment out "beq abort" above
    PRINT_CHAR  #ASCII_QUESTION
    PRINT_CHAR               ; another '?'
    PRINT_CRLF
    jmp         forth_abort  ; end of dictionary, no more words to search, abort

@find_each:    
; msb linked list
    BYTECOPY    ZP_F_2ND + 1, ZP_F_WRD + 1

; update next link 
    COPYFROM    ZP_F_WRD, ZP_F_2ND

; save the flag, first byte is (size and flag) 
    BYTECOPY    {(ZP_F_WRD)}, ZP_F_STAT + 1

; compare words
    ldy         #0

; compare chars
@find_equal:
    lda         (ZP_F_TOUT), y
; space ends
    cmp         #ASCII_SPACE  
    beq         @find_done
; verify 
    sec
    sbc         (ZP_F_WRD), y
; clean 7-bit ascii
    asl
    bne         @find_loop

; next char
    iny
    bne         @find_equal

@find_done:
; update ZP_F_WRD
    tya
    ;; ldx #ZP_F_WRD ; set already
    ;; forth_addwx also clear carry
    jsr         forth_addwx
    
forth_eval:
; executing ? if == \0
    lda         ZP_F_STAT   
    beq         forth_execute

; immediate ? if < \0
    lda         ZP_F_STAT + 1
    bmi         forth_immediate

forth_compile:
    ;PRINT_CHAR  #ASCII_C

    jsr         forth_wcomma
    bra         forth_resolve

forth_immediate:
forth_execute:
    ;PRINT_CHAR  #ASCII_E

    LOAD_ADDR   resolvept, ZP_F_IPT
    jmp         forth_pick

;---------------------------------------------------------------------
forth_try:
    lda         F_TIB, y
    beq         forth_getline    ; if \0 
    iny
    eor         #ASCII_SPACE
    rts

;---------------------------------------------------------------------
forth_getline:
; drop rts of forth_try
    ply
    ply

; leave the first
    ldy         #0
    lda         #ASCII_SPACE

@getline_loop:
; is valid
    sta         F_TIB, y
    iny
; would be better with 
; end of buffer ?
;    cpy #F_TIB_END
;    beq @getline_end
; then 
@getline_again:
    jsr         forth_getchar
; would be better with 
; 7-bit ascii only
;    and #$7F
; compare with LF
    cmp         #ASCII_CR
    beq         @getline_end
; handle backspace
    cmp         #ASCII_BACKSPACE
    bne         @getline_loop
    dey
    bne         @getline_again          ; make sure we don't underflow
    iny
    bra         @getline_again
; clear all if y eq \0
@getline_end:
; eat the LF?
    PRINT_CRLF
; would be better with 
; no controls
;    cmp #' '
;    bmi @loop
    BYTECOPY    F_TIB, {F_TIB, y}      ; starts and ends with space
; mark eol with \0
    BYTECOPY    #0, {F_TIB + 1, y}
; start it
    sta         ZP_F_TIN

;---------------------------------------------------------------------
; in place every token,
; the counter is placed at last space before word
; no rewinds
forth_token:
; last position on F_TIB
    ;PRINT_CHAR  #ASCII_DOT
    ldy         ZP_F_TIN

@getline_skip:
; skip spaces
    jsr         forth_try
    beq         @getline_skip
; y == start + 1
    dey
    sty         ZP_F_TOUT

@getline_scan:
; scan for trailing space
    jsr         forth_try
    bne         @getline_scan

; y == end of token + 1  
    dey
    sty         ZP_F_TIN 

@getline_done:
; sizeof
    tya
    sec
    sbc         ZP_F_TOUT

; keep it
    ldy         ZP_F_TOUT
    dey
    sta         F_TIB, y  ; store size for counted string 
    sty         ZP_F_TOUT

; setup token
    clc

; exit for emulator  
forth_byes:
    rts
;
;---------------------------------------------------------------------
; classic heap moves always forward
;
forth_wcomma:
    ldy         #ZP_F_WRD

forth_comma: 
    ldx         #ZP_F_HERE
    ;PRINT_CHAR  #ASCII_COMMA
    ; fall through

;---------------------------------------------------------------------
; from a page zero address indexed by Y
; into a page zero indirect address indexed by X
forth_copy_into:
    BYTECOPY    {0, y}, {(0, x)}
    jsr         forth_incwx
    BYTECOPY    {1, y}, {(0, x)}
    jmp         forth_incwx

;---------------------------------------------------------------------
;
; generics 
;
;---------------------------------------------------------------------
; push a cell to .S
forth_s_push_1:
    ldy         #ZP_F_1ST
; bounds check SPI
forth_s_push:
    ldx         #ZP_F_SPI
    bra         forth_push

; push a cell to .R
; classic stack backwards
forth_r_push_ipt:
    ldy         #ZP_F_IPT

forth_r_push:
    ldx         #ZP_F_RPI

forth_push:
; stack overflow check
    lda         0, x
    and         #$7f
    beq         :+
    cmp         #2
    bmi         forth_stack_overflow
:
    jsr         forth_decwx
    BYTECOPY    {1, y}, {(0, x)}
    jsr         forth_decwx
    BYTECOPY    {0, y}, {(0, x)}
    rts

forth_decwx:
    lda         0, x
    bne         :+
    dec         1, x
:
    dec         0, x
    rts

forth_stack_overflow:
    sec

forth_stack_underflow:
    PRINT_CHAR  #ASCII_DOT
    lda         #ASCII_R
    cpx         #ZP_F_RPI
    beq         :+
    inc
:
    PRINT_CHAR
    lda         #ASCII_LT
    bcc         :+
    adc         #1
:
    PRINT_CHAR
    PRINT_CHAR  #ASCII_BANG
    PRINT_CHAR
    PRINT_CRLF_JMP

;---------------------------------------------------------------------
forth_s_pull_2:
    jsr         forth_s_pull_1
    ldy         #ZP_F_2ND
    jmp         forth_s_pull
    ; fall through

;---------------------------------------------------------------------
forth_s_pull_1:
    ldy         #ZP_F_1ST
    ; fall through

;---------------------------------------------------------------------
; pull a cell from .S
forth_s_pull:
; bounds check SPI
    ldx         #ZP_F_SPI
    bra         forth_pull

; pull a cell from .R
; bounds check RPI
forth_r_pull_ipt:
    ldy         #ZP_F_IPT

forth_r_pull:
    ldx         #ZP_F_RPI

;---------------------------------------------------------------------
; from a page zero address indexed by X
; into a page zero address indexed by y
forth_pull:
    lda         0, x
    and         #$7f
    beq         forth_stack_underflow

forth_copy_from:
; bounds check 0, x
    BYTECOPY    {(0, x)}, {0, y}
    jsr         forth_incwx
    BYTECOPY    {(0, x)}, {1, y}
    ; fall through

;---------------------------------------------------------------------
; increment a word in page zero. offset by X
forth_incwx:
    lda         #1
;---------------------------------------------------------------------
; add a byte to a word in page zero. offset by X
forth_addwx:
    clc
    adc         0, x
    sta         0, x
    bcc         :+
    inc         1, x
    clc ; keep carry clean
:
    rts

forth_getchar:
    jsr         READ_CHAR
    bcc         forth_getchar
    rts

.include "forth_primitives.s"

;---------------------------------------------------------------------
def_word ";", "semis",  FLAG_IMM
; update ZP_F_LAST, panic if colon not lead elsewhere 
    WORDCOPY    ZP_F_BACK, ZP_F_LAST

; ZP_F_STAT of 0 is 'interpret'
    stz         ZP_F_STAT

; compound words must ends with exit
semis_finish:
    LOAD_ADDR   exit, ZP_F_WRD
    jsr         forth_wcomma
    jmp         forth_next

;---------------------------------------------------------------------
def_word ":", "colon", 0
; save ZP_F_HERE, panic if semis not follow elsewhere
    WORDCOPY    ZP_F_HERE, ZP_F_BACK

; ZP_F_STAT of 1 is 'compile'
    BYTECOPY    #1, ZP_F_STAT

@colon_header:
; copy ZP_F_LAST into (ZP_F_HERE)
    ldy         #ZP_F_LAST
    jsr         forth_comma

; get following token
    jsr         forth_token

; copy it
    ldy         #0

@colon_loop:
    lda         (ZP_F_TOUT), y
    cmp         #ASCII_SPACE    ; stops at space
    beq         @colon_end
    sta         (ZP_F_HERE), y
    iny
    bne         @colon_loop

@colon_end:
; update ZP_F_HERE 
    tya
    ldx         #ZP_F_HERE
    jsr         forth_addwx
    bra         forth_next

;---------------------------------------------------------------------
; Thread Code Engine
;
;   ZP_F_IPT is IP, ZP_F_WRD is W
;
; for reference: 
;
;   forth_nest aka enter or docol, 
;   unnest aka exit or semis;
;
;---------------------------------------------------------------------
; ( -- ) 
def_word "exit", "exit", 0
unnest: ; exit
; forth_pull, ZP_F_IPT = (ZP_F_RPI), Z_F_RPI += 2 
    jsr         forth_r_pull_ipt

forth_next:
; ZP_F_WRD = (ZP_F_IPT) ; ZP_F_IPT += 2
    COPYFROM    ZP_F_IPT, ZP_F_WRD

forth_pick:
; compare pages (MSBs)
    lda         ZP_F_WRD + 1
    bpl         forth_nest
    jmp         (ZP_F_WRD)

forth_nest:   ; enter
; forth_push, (ZP_F_RPI) = ZP_F_IPT, ZP_F_RPI -=2
    jsr         forth_r_push_ipt

    WORDCOPY    ZP_F_WRD, ZP_F_IPT
    bra         forth_next
;~~~~~~~~

end_of_primitives:
