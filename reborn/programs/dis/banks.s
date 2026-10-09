; ****************************************************************************
; banks.s - dis's image and its flags, each in a run of the task's RAM banks (8K each, at $8000 when selected): the
; image's byte at an address, a flag byte's, and bits set in one.  cc65's calls: the address in .A/.X.

            .export     _img, _img_ptr, _flg, _flg_or, _bk_base, _bk_img, _bk_flg, _bk_bits

            .include    "zeropage.inc"

            .bss
_bk_base:   .res        2                                   ; The image's first address
_bk_img:    .res        1                                   ; Its first bank ...
_bk_flg:    .res        1                                   ;   and its flags'
_bk_bits:   .res        1                                   ; (flg_or's bits)

            .code

; unsigned char __fastcall__ img (unsigned a): the image's byte at a
_img:
            ldy         _bk_img
            jsr         at
            lda         (ptr1)
            ldx         #0
            rts

; unsigned char* __fastcall__ img_ptr (unsigned a): the image's byte at a, its bank selected: its place in the window
; (cc65's optimizer drops a % 0x2000's high byte: this does it)
_img_ptr:
            ldy         _bk_img
            jsr         at
            lda         ptr1
            ldx         ptr1 + 1
            rts

; unsigned char __fastcall__ flg (unsigned a): a's flags
_flg:
            ldy         _bk_flg
            jsr         at
            lda         (ptr1)
            ldx         #0
            rts

; void __fastcall__ flg_or (unsigned a): bk_bits set in a's flags
_flg_or:
            ldy         _bk_flg
            jsr         at
            lda         (ptr1)
            ora         _bk_bits
            sta         (ptr1)
            rts

; ptr1 = the address .A/.X's place in the window, its bank (.Y, the run's first, + its 8K's) selected
at:
            sec
            sbc         _bk_base
            sta         ptr1
            txa
            sbc         _bk_base + 1
            pha
            and         #$1F
            ora         #$80
            sta         ptr1 + 1
            pla
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            sty         tmp1
            clc
            adc         tmp1
            sta         $00
            rts
