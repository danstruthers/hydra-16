;---------------------------------------------------------------------
;
; the primitives, 
; for stacks uses
; a address, c byte ascii, w signed word, u unsigned word 
; cs counted string < 256, sz string with nul ends
; 
;----------------------------------------------------------------------

;----------------------------------------------------------------------
; extras
;----------------------------------------------------------------------
; ( -- ) ae exit forth
def_word "bye", "bye", 0
    jmp         forth_byes

;----------------------------------------------------------------------
; ( -- ) ae abort
def_word "abort", "abort", 0
    jmp         forth_abort

;----------------------------------------------------------------------
; ( -- ) ae list of data stack
def_word ".S", "splist", 0
    WORDCOPY    ZP_F_SPI, ZP_F_1ST
    lda         #<F_SP0
    jsr         forth_list
    PRINT_CRLF
    jmp         forth_next

;----------------------------------------------------------------------
; ( -- ) ae list of return stack
def_word ".R", "rplist", 0
    WORDCOPY    ZP_F_RPI, ZP_F_1ST
    lda         #<F_RP0
    jsr         forth_list
    PRINT_CRLF
    jmp         forth_next

_f_print_fst:
    PRINT_SPACE
    PRINT_BYTE_JMP  ZP_F_1ST + 1, ZP_F_1ST

_f_print_fst_val:
    PRINT_SPACE
_f_print_fst_val_ns:
    iny
    PRINT_BYTE      {(ZP_F_1ST),y}
    dey
    PRINT_BYTE_JMP  {(ZP_F_1ST),y}

;----------------------------------------------------------------------
;  ae list a sequence of references
forth_list:
    sec
    sbc         ZP_F_1ST
    lsr

    tax
    jsr         _f_print_fst

    PRINT_SPACE

    txa
    PRINT_BYTE
    PRINT_SPACE

    txa
    beq         @list_end
    ldy         #0

@list_loop:
    jsr         _f_print_fst_val
    iny
    iny
    dex
    bne         @list_loop

@list_end:
    rts

;----------------------------------------------------------------------
; ( -- ) dumps the user dictionary
def_word "dump", "dump", 0
    LOAD_ADDR   start_of_user_words, ZP_F_1ST
    ldx         #ZP_F_1ST

@dump_loop:
    lda         ZP_F_1ST
    cmp         ZP_F_HERE
    bne         :+

    lda         ZP_F_1ST + 1
    cmp         ZP_F_HERE + 1
    bne         :+

    clc
    PRINT_CRLF
    jmp         forth_next 

:
    PRINT_BYTE  {(ZP_F_1ST)}
    jsr         forth_incwx
    bra         @dump_loop

;----------------------------------------------------------------------
; ( -- ) words in dictionary, 
def_word "words", "words", 0
; load ZP_F_LAST
    WORDCOPY    ZP_F_LAST, ZP_F_2ND

; load ZP_F_HERE
    WORDCOPY    ZP_F_HERE, ZP_F_3RD

@words_loop:
; lsb linked list
    BYTECOPY    ZP_F_2ND, ZP_F_1ST

; verify \0x0
    ora         ZP_F_2ND + 1
    beq         @words_end

; msb linked list
    BYTECOPY    ZP_F_2ND + 1, ZP_F_1ST + 1
    PRINT_CRLF

; put address
    jsr         _f_print_fst

; put link
    ldy         #0
    jsr         _f_print_fst_val

    ldx         #ZP_F_1ST
    lda         #2
    jsr         forth_addwx

; put size + flag, name
    jsr         forth_show_name

; update
    iny
    tya
    ldx         #ZP_F_1ST
    jsr         forth_addwx

; show CFA
    jsr         _f_print_fst

; check if is a primitive
    lda         ZP_F_1ST + 1
    bmi         @words_continue

; list references
    ldy         #0

; show_refer:
; ae put references PFA ... 
    ldx         #ZP_F_1ST

@refer_loop:
    jsr         _f_print_fst
    PRINT_CHAR  #ASCII_COLON
    jsr         _f_print_fst_val_ns

    lda         #2
    jsr         forth_addwx

; check if ends
    lda         ZP_F_1ST
    cmp         ZP_F_3RD
    bne         @refer_loop
    lda         ZP_F_1ST + 1
    cmp         ZP_F_3RD + 1
    bne         @refer_loop

@words_continue:
    WORDCOPY    ZP_F_2ND, ZP_F_3RD
    COPYFROM    ZP_F_3RD, ZP_F_2ND
    bra         @words_loop 

@words_end:
    PRINT_CRLF
    clc
    jmp         forth_next

;----------------------------------------------------------------------
; ae put size and name 
forth_show_name:
    PRINT_SPACE
    PRINT_BYTE  {(ZP_F_1ST), y}
    PRINT_SPACE
    lda         (ZP_F_1ST), y
; strip flags, if present, leaving max length of 31
    and         #$1F
    tax

 @show_name_loop:
    iny
    PRINT_CHAR  {(ZP_F_1ST), y}
    dex
    bne         @show_name_loop

@show_name_end:
    rts

;----------------------------------------------------------------------
; ( u -- u ) print tos in hexadecimal, swaps order
def_word ".", "dot", 0
    PRINT_SPACE
    ldy         #1
    PRINT_BYTE {(ZP_F_SPI)}, {(ZP_F_SPI), y}
    jmp         forth_next

;---------------------------------------------------------------------
;
; extensions
;
;---------------------------------------------------------------------

;---------------------------------------------------------------------
; ( -- w ) ; push $1
def_word "1", "one", 0
    BYTECOPY    #1, ZP_F_1ST
forth_do_one:
    stz         ZP_F_1ST + 1
    jmp         this

; ( -- w ) ; push $FFFF (-1)
def_word "-1", "negative_one", 0
    BYTECOPY    #$FF, ZP_F_1ST
    jmp         keeps

; ( -- w ) ; push $0
def_word "0", "zero", 0
    stz         ZP_F_1ST
    bra         forth_do_one

;---------------------------------------------------------------------
; ( w -- w/2 ) ; shift right
def_word "2/", "shr", 0
    ldy         #1
    lda         (ZP_F_SPI), y
    lsr
    sta         (ZP_F_SPI), y
    lda         (ZP_F_SPI)
    ror
    sta         (ZP_F_SPI)
    jmp         forth_next

;---------------------------------------------------------------------
; ( w -- w*2 ) ; shift left
def_word "2*", "shl", 0
    lda         (ZP_F_SPI)
    asl
    sta         (ZP_F_SPI)
    ldy         #1
    lda         (ZP_F_SPI), y
    rol
    sta         (ZP_F_SPI), y
    ; bcs       ERROR_OVERFLOW          ; if carry is set, you get an overflow
    jmp         forth_next

;---------------------------------------------------------------------
; Take next word value and push it on DS, and increment RPI
def_word "[']", "quot_lit", 0
    bra         forth_p_lit

def_word "lit", "lit", 0
; ZP_F_1ST = (ZP_F_IPT) ; ZP_F_IPT += 2
    COPYFROM    ZP_F_IPT, ZP_F_1ST
    jmp         this

;---------------------------------------------------------------------
; Take last word and set the immediate flag
def_word "I", "imm", 0
    ldy         #2
    lda         (ZP_F_LAST), y
    ora         #FLAG_IMM
    sta         (ZP_F_LAST), y
    jmp         forth_next

; switch to exec/imm/interpret mode
def_word "[", "lbracket", FLAG_IMM
    stz         ZP_F_STAT
    jmp         forth_next

; switch to compile mode
def_word "]", "rbracket", 0
    lda         #1
    sta         ZP_F_STAT
    jmp         forth_next

;---------------------------------------------------------------------
; ( a -- ) execute a jump to a reference at top of data stack
def_word "exec", "exec", 0 
    jsr         forth_s_pull_1
    jmp         (ZP_F_1ST)

;---------------------------------------------------------------------
; ( -- ) execute a jump to a reference at IP
def_word ":$", "docode", 0 
    jmp         (ZP_F_IPT)

;---------------------------------------------------------------------
; ( -- ) execute a jump to forth_next
def_word ";$", "donext", 0 
    jmp         forth_next

;---------------------------------------------------------------------
; core primitives minimal 
; start of dictionary
;---------------------------------------------------------------------
; ( -- u ) ; tos + 1 unchanged
def_word "key", "key", 0
    jsr         forth_getchar
    sta         ZP_F_1ST
    jmp         forth_do_one

;---------------------------------------------------------------------
; ( u -- ) ; tos + 1 unchanged
def_word "emit", "emit", 0
    jsr         forth_s_pull_1
    PRINT_CHAR  ZP_F_1ST
    jmp         forth_next

;---------------------------------------------------------------------
; ( w a -- ) ; [a] = w
def_word "!", "store", 0
    jsr         forth_s_pull_2
    COPYINTO    ZP_F_1ST, ZP_F_2ND
    jmp         forth_next

;---------------------------------------------------------------------
; (w -- | -- w) Top of DS moved to top of RS
def_word ">r", "to_rs", 0
    jsr         forth_s_pull_1
    ldy         #ZP_F_1ST
    jsr         forth_r_push
    jmp         forth_next

;---------------------------------------------------------------------
; ( -- w | w -- ) Top of RS moved to top of DS
def_word "<r", "from_rs", 0
    ldy         #ZP_F_1ST
    jsr         forth_r_pull
    jmp         this

;---------------------------------------------------------------------
; ( w -- INVERT(w) )
def_word "~", "invert", 0
; TODO: stack item count > 0 check
    lda         (ZP_F_SPI)
    eor         #$ff
    sta         (ZP_F_SPI)
    ldy         #1
    lda         (ZP_F_SPI), y
    eor         #$ff
    sta         (ZP_F_SPI), y
    jmp         forth_next

forth_and:
    jsr         forth_s_pull_2
    lda         ZP_F_2ND
    and         ZP_F_1ST
    sta         ZP_F_1ST
    lda         ZP_F_2ND + 1
    and         ZP_F_1ST + 1
    rts

; ( w1 w2 -- INVERT(w1 BAND w2) )
def_word "nand", "nand", 0
    jsr         forth_and
    eor         #$ff
    sta         ZP_F_1ST + 1
    lda         ZP_F_1ST
    eor         #$ff
    sta         ZP_F_1ST
    jmp         this

;---------------------------------------------------------------------
; ( w1 w2 -- w1 BAND w2 )
def_word "and", "band", 0
    jsr         forth_and
    bra         keeps

;---------------------------------------------------------------------
; ( w1 w2 -- w1 BOR w2 )
def_word "or", "bor", 0
    jsr         forth_s_pull_2
    lda         ZP_F_2ND
    ora         ZP_F_1ST
    sta         ZP_F_1ST
    lda         ZP_F_2ND + 1
    ora         ZP_F_1ST + 1
    bra         keeps

;---------------------------------------------------------------------
; ( w1 w2 -- w1 BXOR w2 )
def_word "xor", "bxor", 0
    jsr         forth_s_pull_2
    lda         ZP_F_2ND
    eor         ZP_F_1ST
    sta         ZP_F_1ST
    lda         ZP_F_2ND + 1
    eor         ZP_F_1ST + 1
    bra         keeps

;---------------------------------------------------------------------
; ( w1 w2 -- w1+w2 )
def_word "+", "plus", 0
    jsr         forth_s_pull_2
    clc
    lda         ZP_F_2ND
    adc         ZP_F_1ST
    sta         ZP_F_1ST
    lda         ZP_F_2ND + 1
    adc         ZP_F_1ST + 1
    ; bvs err_overflow?
    clc
    bra         keeps

; ( w1 w2 -- w1-w2 )
def_word "-", "minus", 0
    jsr         forth_s_pull_2
    sec
    lda         ZP_F_2ND
    sbc         ZP_F_1ST
    sta         ZP_F_1ST
    lda         ZP_F_2ND + 1
    sbc         ZP_F_1ST + 1
    clc
    bra         keeps

;---------------------------------------------------------------------
; ( a -- w ) ; TOS gets value at a
def_word "@", "fetch", 0
    jsr         forth_s_pull_1
; ZP_F_2ND = (ZP_F_1ST) ; ZP_F_1ST += 2
    COPYFROM    ZP_F_1ST, ZP_F_2ND
    ; fall through

;---------------------------------------------------------------------
copys:
    BYTECOPY    {0, y}, ZP_F_1ST
    lda         1, y

keeps:
    sta         ZP_F_1ST + 1

this:
    jsr         forth_s_push_1

jmpnext:
    jmp         forth_next

;---------------------------------------------------------------------
; ( 0 -- $0000) | ( n -- $FFFF) not zero at top ?
;def_word "0=", "eq0", 0
; TODO: stack item count > 0 check
;    ldy         #1
;    lda         (ZP_F_SPI)
;    ora         (ZP_F_SPI), y
;    beq         istrue  ; is \0 ?
;    BYTECOPY    #0, {(ZP_F_SPI)}
;    sta         (ZP_F_SPI), y
;    bra         jmpnext

;---------------------------------------------------------------------
; ( 0 -- $0000) | ( n -- $FFFF) normalize bool (same as 0#)
def_word "nb", "norm_bool", 0
    bra         forth_p_neq0

;---------------------------------------------------------------------
; ( 0 -- $0000) | ( n -- $FFFF) not zero at top ?
def_word "0#", "neq0", 0
; TODO: stack item count > 0 check
    ldy         #1
    lda         (ZP_F_SPI)
    ora         (ZP_F_SPI), y
    beq         isfalse  ; is \0 ?

istrue:
    BYTECOPY    #$ff, {(ZP_F_SPI)}
    sta         (ZP_F_SPI), y

isfalse:
    bra         jmpnext

.macro ZPA_PUSH zpa
    lda         #zpa
    sta         ZP_F_1ST
    jmp         forth_do_one
.endmacro

;---------------------------------------------------------------------
; ( -- a ) put state address on the stack
def_word "s@", "state", 0 
    ZPA_PUSH    ZP_F_STAT

; ( -- a ) put input ptr on the stack
def_word ">in", "input_ptr", 0
    ZPA_PUSH    ZP_F_TIN

; ( -- a) put latest ptr on the stack
def_word "last", "last_ptr", 0
    ZPA_PUSH    ZP_F_LAST

; ( -- a) put here ptr on the stack
def_word "here", "here_ptr", 0
    ZPA_PUSH    ZP_F_HERE

; ( -- a) put stack pointer address on the stack
def_word "sp", "stack_ptr", 0
    ZPA_PUSH    ZP_F_SPI

; ( -- a) put return stack pointer address on the stack
def_word "rp", "rstack_ptr", 0
    ZPA_PUSH    ZP_F_RPI

forth_zpv_push:
    BYTECOPY    {0, x}, ZP_F_1ST
    lda         1, x
    jmp         keeps

.macro ZPV_PUSH zpa
    ldx         #zpa
    bra         forth_zpv_push
.endmacro

; ( -- w) put value at ZP_F_HERE on DS
def_word "here@", "here_val", 0
    ZPV_PUSH    ZP_F_HERE

; ( -- w) put value at ZP_F_LAST on DS
;def_word "latest@", "latest_val", 0
;    ZPV_PUSH    ZP_F_LAST

; ( -- w) put value at top of RS on DS
def_word "rp@", "rstack_val", 0
    ZPV_PUSH    ZP_F_RPI

; (w -- w w) duplicate the TOS item
def_word "dup", "duplicate", 0
; TODO: stack item count > 0 check
; TODO: stack ptr overflow check
    BYTECOPY    {(ZP_F_SPI)}, ZP_F_1ST
    ldy         #1
    lda         (ZP_F_SPI), y
    jmp         keeps

; (w1 w2 -- w2 w1 w2) duplicate the second item on the stack
def_word "over", "over", 0
; TODO: stack ptr overflow check
; TODO: stack item count > 1 check
    ldy         #2
    BYTECOPY    {(ZP_F_SPI), y}, ZP_F_1ST
    iny
    lda         (ZP_F_SPI), y
    jmp         keeps

; (w -- ) drop the TOS item
def_word "drop", "drop", 0
; TODO: stack item count > 0 check
    clc
    lda         #2
    adc         ZP_F_SPI
    sta         ZP_F_SPI
    bcc         :+
    inc         ZP_F_SPI + 1
:
    jmp         forth_next

; (w1 w2 -- w2 w1) drop the TOS item
def_word "swap", "swap", 0
; TODO: stack item count > 1 check
    ; load 2nd item on stack
    ; push to HW stack
    ; move TOS to 2nd place
    ; pull from HW stack into TOS
    jsr         forth_s_pull_2
    jsr         forth_s_push_1
    ldy         #ZP_F_2ND
    jsr         forth_s_push
    jmp         forth_next

