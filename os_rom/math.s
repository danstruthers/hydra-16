.debuginfo

.zeropage
ZP_MATH_TEMP:           ; temp space, parse output base
            .res 4      
ZP_MATH_TEMP2:          ; temp space, parse output base
            .res 4      
ZP_MATH_PST:            ; parse state
            .byte 0
ZP_MATH_PB:             ; parse base
            .byte 0
ZP_MATH_PNS:            ; parse number size
            .byte 0
ZP_MATH_OA:             ; parse output address
            .word 0

.segment "BIOS"
; MATH
MOD_10:
            ldx             #0
            cmp             #0
            bmi             @negative
            cmp             #100
            bmi             @pos_loop
            ldx             #10
            sec
            sbc             #100

@pos_loop:
            cmp             #10
            bmi             @pos_end
            inx
            sec
            sbc             #10
            bpl             @pos_loop

@pos_end:
            rts

@negative:
            cmp             #$A6            ; -90
            bpl             @neg_start
            adc             #$A6

@neg_start:
            clc

@neg_loop:
            adc             #10
            bmi             @neg_loop
            rts

REM_10:
            ldx             #0
            cmp             #0
            bmi             @negative
            cmp             #100
            bmi             @positive
            sbc             #100
            ldx             #10

@positive:
            sec

@pos_loop:
            inx
            sbc             #10
            beq             @pos_end
            bpl             @pos_loop
            dex
            adc             #10

@pos_end:
            rts

@negative:
            cmp             #$9B            ; -99
            bpl             @neg_start
            adc             #$9B
            ldx             #$F6            ; -10

@neg_start:
            clc

@neg_loop:
            dex
            adc             #10
            bmi             @neg_loop
            beq             @neg_end
            inx
            sec
            sbc             #10

@neg_end:
            rts

DIV_10:
            jsr             REM_10
            SWAP_AX
            rts

ABS:
            cmp             #0
            bcs             NEG_DONE

.macro INVERT_A
            eor             #$FF
.endmacro

NEGATE:
            INVERT_A
            inc

NEG_DONE:
            rts

; .A, .Y hold ADDR of LOB to increment, .X holds the length of the value in bytes
; Incremets an x-byte value at addr, addr + 1 and sets Z, N properly based on HOB
.macro _M_ADD_BYTE_X           byteVal
            sta             ZP_MATH_TEMP
            sty             ZP_MATH_TEMP + 1
.ifblank    byteVal
            pha
.else
            lda             #byteVal
.endif
            ldy             #0
            dex
            phx
            bra             ADD_X_LOOP
.endmacro

ADD_X_LOOP:
            adc             (ZP_MATH_TEMP),Y
            sta             (ZP_MATH_TEMP),Y
            dex
            bmi             :+
            ldx             #0
            iny
            bcs             ADD_X_LOOP
:
            ply
            lda             (ZP_MATH_TEMP),Y  ; set Z and N from HOB
            bcc             :+
            ora             #1                ; reset Z since LOB is not zero, but preserve other flags
:
            rts

;  Adds a byte passed on the stack to a .X-byte long integer where the LOB address is in .A, .Y
ADD_STACK_BYTE_X:
            _M_ADD_BYTE_X

INC_X:
            _M_ADD_BYTE_X   1
.macro  _M_INCX_C  xval
            phx
            ldx             #xval
.endmacro

.macro  _M_INCX  xval
            _M_INCX_C       {xval}
            bra             INCX_CONTINUE
.endmacro

INC_8:
            _M_INCX         1

INC_16:
            _M_INCX         2

INC_24:
            _M_INCX         3

INC_32:
            _M_INCX_C       4

INCX_CONTINUE:
            jsr             INC_X
            php                             ; save the flags
            pla                             ; move them to .A for restore later
            plx                             ; retrieve saved .X (which modifies N, Z)
            pha                             ; push old flags to stack
            plp                             ;    and restore them to the flags register
            rts

INC_40:
            _M_INCX         5

INC_48:
            _M_INCX         6

INC_56:
            _M_INCX         7

INC_64:
            _M_INCX         8

.feature c_comments
/*
MATH_PS_ACCEPT_BYTES    := 0
MATH_PS_BASE_NEG        := 1
MATH_PS_BASE_CHANGE     := 2
MATH_PS_CAN_CHANGE_BASE := 3
MATH_PS_VAL_NEG         := $80

ValidBases:
            HString         "bdhtwx"
BaseSize:
            .byte           2, 10, 16, 3, 12, 36
BasePlaceValues:
            .byte           "0123456789ABCDEFGHIJKLMNOPQRSTUVWZYZ"
BalanceTerneryValues:
            .byte           "-0+"

; Number parsing

MATH_PARSE_DEC_TO_WORD:
            ldy             #2

MATH_PARSE_DEC:
            phx
            lda             #10

; .A = initial base, .Y = size in bytes
MATH_PARSE_INIT:
            sta             ZP_MATH_PB
            sty             ZP_MATH_PNS
            lda             #MATH_PS_CAN_CHANGE_BASE
            stz             ZP_MATH_PST
            rts

; .A = byte to parse
MATH_PARSE_BYTE:
; if state = INIT, can get % or digit
            pha
            lda             #$7f
            and             ZP_MATH_PST
            beq             @parse_byte
            cmp             #MATH_PS_CAN_CHANGE_BASE

@parse_byte:
            jsr             MATH_PARSE_VALID_BYTE     ; leaves place value in .A
            bcc             @base_mult
            rts

@base_mult:
            ldx             0
; save the place value in y
            tay
@copy_loop:
            lda             ZP_MATH_TEMP, x
            sta             ZP_MATH_TEMP2, x
            inx
            cpx             ZP_MATH_PNS
            bmi             @copy_loop
            jsr             @shift_one
            jsr             @shift_one
            ldx             0
            clc
            php
@add_loop:
            plp
            lda             ZP_MATH_TEMP, x
            adc             ZP_MATH_TEMP2, x
            php
            inx
            cpx             ZP_MATH_PNS
            bpl             @shift_one
            plp
            ;bcs             @err_overflow

@shift_one:
            ldx             0
            clc
            php

@shift_loop:
            php
            rol             ZP_MATH_TEMP, x
            inx
            cpx             ZP_MATH_PNS
            bmi             @shift_loop

@add_one:

MATH_PARSE_VALID_BYTE:
            cmp             #ASCII_0
            bmi             @ret_invalid_byte_err
            sec
            sbc             #ASCII_0
            cmp             #10
            bmi             :+
            sec
            sbc             #(ASCII_A - ASCII_0)
:
            cmp             ZP_MATH_PB
            bpl             @ret_invalid_byte_err
            clc
            rts

@ret_invalid_byte_err:
            sec
            lda             ERR_MATH_INVD
            rts

; .A.Y: output address
MATH_PARSE_END:
*/