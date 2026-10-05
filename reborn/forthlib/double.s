; ****************************************************************************
; double.s - HyForth's Double-Number library (/lib/forth/double.fl), as yet only 2CONSTANT, 2VARIABLE, 2LITERAL (the
; String tests use them), DNEGATE and DABS (the core's code, which . and the rest use: their headers here jump to it).

.include "forthlib.inc"

            HEADER      "2constant", 0
twoconstant:                                                ; ( x1 x2 "name" -- ): two literals and rts
            lda         #0
            jsr         make_hdr
            jsr         twoliteral
            lda         #RTS_OP
            jmp         ccomma_a

            HEADER      "2variable", 0
twovariable:
            jsr         variable
            lda         #0
            tay
            jmp         comma_ay

            HEADER      "2literal", F_IMMEDIATE
twoliteral:                                                 ; ( x1 x2 -- ): compiled, x1 pushed first
            jsr         swap
            jsr         literal
            jmp         literal

            HEADER      "dnegate", 0
dnegate_w:                                                  ; ( d -- -d )
            jmp         dnegate

            HEADER      "dabs", 0
dabs_w:                                                     ; ( d -- |d| )
            jmp         dabs
