; ****************************************************************************
; t_asall.s - the assembler's test (tests.js: as): every W65C02S opcode in each of its modes, as ca65 writes them,
; then directives, expressions, labels (cheap locals, unnamed ones), macros, conditionals, segments, .include and
; .incbin.  The build makes it with ca65 and ld65 (obj/tests/t_asall.hyx), and the test with as on the Hydra: the
; same bytes.  It's never run.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "t_asall", start

.zeropage
zp1:        .res        2
zp2:        .res        1

.code
start:
            brk
            ora         ($34,x)
            tsb         $34
            ora         $34
            asl         $34
            rmb0        $34
            php
            ora         #$12
            asl         a
            tsb         $1234
            ora         $1234
            asl         $1234
            bbr0        $34, * - 1
            bpl         * + 2
            ora         ($34),y
            ora         ($34)
            trb         $34
            ora         $34,x
            asl         $34,x
            rmb1        $34
            clc
            ora         $1234,y
            inc         a
            trb         $1234
            ora         $1234,x
            asl         $1234,x
            bbr1        $34, * - 2
            jsr         $1234
            and         ($34,x)
            bit         $34
            and         $34
            rol         $34
            rmb2        $34
            plp
            and         #$12
            rol         a
            bit         $1234
            and         $1234
            rol         $1234
            bbr2        $34, * - 3
            bmi         * + 2
            and         ($34),y
            and         ($34)
            bit         $34,x
            and         $34,x
            rol         $34,x
            rmb3        $34
            sec
            and         $1234,y
            dec         a
            bit         $1234,x
            and         $1234,x
            rol         $1234,x
            bbr3        $34, * - 4
            rti
            eor         ($34,x)
            eor         $34
            lsr         $34
            rmb4        $34
            pha
            eor         #$12
            lsr         a
            jmp         $1234
            eor         $1234
            lsr         $1234
            bbr4        $34, * - 5
            bvc         * + 2
            eor         ($34),y
            eor         ($34)
            eor         $34,x
            lsr         $34,x
            rmb5        $34
            cli
            eor         $1234,y
            phy
            eor         $1234,x
            lsr         $1234,x
            bbr5        $34, * - 6
            rts
            adc         ($34,x)
            stz         $34
            adc         $34
            ror         $34
            rmb6        $34
            pla
            adc         #$12
            ror         a
            jmp         ($1234)
            adc         $1234
            ror         $1234
            bbr6        $34, * - 7
            bvs         * + 2
            adc         ($34),y
            adc         ($34)
            stz         $34,x
            adc         $34,x
            ror         $34,x
            rmb7        $34
            sei
            adc         $1234,y
            ply
            jmp         ($1234,x)
            adc         $1234,x
            ror         $1234,x
            bbr7        $34, * - 8
            bra         * + 2
            sta         ($34,x)
            sty         $34
            sta         $34
            stx         $34
            smb0        $34
            dey
            bit         #$12
            txa
            sty         $1234
            sta         $1234
            stx         $1234
            bbs0        $34, * - 9
            bcc         * + 2
            sta         ($34),y
            sta         ($34)
            sty         $34,x
            sta         $34,x
            stx         $34,y
            smb1        $34
            tya
            sta         $1234,y
            txs
            stz         $1234
            sta         $1234,x
            stz         $1234,x
            bbs1        $34, * - 10
            ldy         #$12
            lda         ($34,x)
            ldx         #$12
            ldy         $34
            lda         $34
            ldx         $34
            smb2        $34
            tay
            lda         #$12
            tax
            ldy         $1234
            lda         $1234
            ldx         $1234
            bbs2        $34, * - 11
            bcs         * + 2
            lda         ($34),y
            lda         ($34)
            ldy         $34,x
            lda         $34,x
            ldx         $34,y
            smb3        $34
            clv
            lda         $1234,y
            tsx
            ldy         $1234,x
            lda         $1234,x
            ldx         $1234,y
            bbs3        $34, * - 12
            cpy         #$12
            cmp         ($34,x)
            cpy         $34
            cmp         $34
            dec         $34
            smb4        $34
            iny
            cmp         #$12
            dex
            wai
            cpy         $1234
            cmp         $1234
            dec         $1234
            bbs4        $34, * - 13
            bne         * + 2
            cmp         ($34),y
            cmp         ($34)
            cmp         $34,x
            dec         $34,x
            smb5        $34
            cld
            cmp         $1234,y
            phx
            stp
            cmp         $1234,x
            dec         $1234,x
            bbs5        $34, * - 14
            cpx         #$12
            sbc         ($34,x)
            cpx         $34
            sbc         $34
            inc         $34
            smb6        $34
            inx
            sbc         #$12
            nop
            cpx         $1234
            sbc         $1234
            inc         $1234
            bbs6        $34, * - 15
            beq         * + 2
            sbc         ($34),y
            sbc         ($34)
            sbc         $34,x
            inc         $34,x
            smb7        $34
            sed
            sbc         $1234,y
            plx
            sbc         $1234,x
            inc         $1234,x
            bbs7        $34, * - 16
; Implied accumulator, labels as operands, forcing, forward and backward
            asl
            inc
            lda         zp1
            lda         zp1 + 1,x
            sta         (zp1),y
            lda         a:zp1
            lda         later
            lda         later,y
            jmp         (vec)
            jmp         (vec,x)
            bne         :+
            nop
:
            beq         :++
            nop
:
            bra         :-
:
            bbs3        zp2, :-
            bbr7        zp2, :+
            rmb1        zp2
            smb6        zp2
:
lbl1:
            ldx         #<msg
            ldy         #>msg
@loop:
            dex
            bne         @loop
lbl2:
@loop:
            dey
            bne         @loop
            lda         #'A'
            lda         #'a' + 1
            cmp         #';'                                ; (a ; in quotes)
            ldx         #%1010
            ldy         #12
            jsr         sub
            rts
sub:
            .byte       $EA
vec:        .word       start, sub, later

; Constants and expressions
ONE         = 1
TWO         := ONE + 1
BIG         = $12345678
            .word       1 + 2 * 3, (1 + 2) * 3, 10 / 3, 10 .mod 3, 1 << 4, $100 >> 4
            .word       $FF & $0F, $F0 | $0F, $FF ^ $0F, -1 & $FFFF, ~0 & $FFFF, <$1234, >$1234, ^$123456
            .word       !0, !5, 3 = 3, 3 <> 4, 2 < 3, 3 <= 3, 4 > 3, 4 >= 5, 1 && 0, 1 || 0, 0 || 0
            .word       .lobyte($1234), .hibyte($1234), .bankbyte($123456), .loword(BIG), .hiword(BIG)
            .word       .strlen("hello"), 'A', %1010, 12345, TWO, -TWO & $FFFF, 1 .and 2, 0 .or 3, 1 .xor 1
            .word       12 .bitand 10, 12 .bitor 3, 6 .bitxor 3, .bitnot(0) & $FF, 1 .shl 3, 16 .shr 2
            .word       *, * - start, later - start
            .dword      BIG, -2, BIG >> 8
            .byte       "str", 0, 'x', "it's", "a;b"
            .asciiz     "abc"
            .res        3
            .res        2, $EE
            .addr       sub
            .byt        .defined(ONE), .defined(nothing), .def(TWO)
            .byte       .match(1, 1), .blank(), .blank(x)

; Macros
.macro      PAIR        a1, a2
            .byte       a1, a2
.endmacro
.macro      MAYBE       a1, a2
.ifblank a2
            .byte       a1
.else
            .byte       a1, a2
.endif
.endmacro
.macro      DOUBLE      v
            PAIR        v, v
.endmacro
.macro      LEAVE       v
            .byte       v
.if v = 1
.exitmacro
.endif
            .byte       v + 10
.endmacro
            PAIR        1, 2
            PAIR        {3, 4}, 5
            MAYBE       6
            MAYBE       7, 8
            DOUBLE      9
            LEAVE       1
            LEAVE       2
            PAIR        "x", 'y'

; Conditionals
.if 0
            .byte       $01
.elseif 1
            .byte       $02
.else
            .byte       $03
.endif
.ifdef ONE
            .byte       $04
.endif
.ifndef ONE
            .byte       $05
.else
.if 1
            .byte       $06
.endif
.endif
.if 0
.if 1
            .byte       $07
.endif
.else
            .byte       $08
.endif
.ifnblank x
            .byte       $09
.endif

; Segments
.rodata
msg:        .byte       "hello", 0
.data
counter:    .word       msg, counter
.bss
buf:        .res        16
.code
later:      lda         buf
            lda         counter
            lda         msg,x
.pushseg
.rodata
more:       .word       buf, more
.popseg
            .word       more
.segment "RODATA"
            .byte       $AA
.code
            .byte       $BB
.assert     * > start, error, "the PC goes on"

; Files
.include "inc1.inc"
            .incbin     "data.bin"
            .incbin     "data.bin", 2
            .incbin     "data.bin", 1, 3
            .byte       FROM_INC

