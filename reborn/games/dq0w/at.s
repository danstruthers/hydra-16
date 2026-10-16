; ****************************************************************************
; at.s - (the world's; the denizens' and the senses' include it) world.h's at (): a square's chunk in the window and
; the square in it, the quick way, in assembly: a square of the chunk at () found last (its stratum, its bounds, the
; chunk's first square, its bank, its squares: chunk.c's at_sn ...), worked out without multiplying, shifting or the
; C stack (cc65's at () took some 1,500 cycles a square, and a look about asks for thousands); any other, at_find
; (chunk.c: the hub's cache, or the maker), which sets the quick way for the next.

            .macpack    longbranch
            .export     _at
            .import     _at_find, _hub_map, incsp3
            .import     _at_sn, _at_x0, _at_y0, _at_w, _at_h, _at_ox, _at_oy, _at_bank, _at_chunk, _at_tile
            .import     _C, _cx_, _cy_
            .importzp   sp, tmp1

            .bss
qy:         .res        2                   ; (y, kept)
qd:         .res        2                   ; (A square's place in its stratum, x or y)
qx:         .res        1                   ; (Its x in the chunk)

            .code

; unsigned char* __fastcall__ at (unsigned char sn, unsigned x, unsigned y): y in .A .X, x and sn on the C stack
_at:
            sta         qy
            stx         qy + 1
            ldy         #2                  ; The stratum found last?
            lda         (sp),y
            cmp         _at_sn
            jne         @long
            ldy         #0                  ; x - x0, in the bounds,
            lda         (sp),y
            sec
            sbc         _at_x0
            sta         qd
            iny
            lda         (sp),y
            sbc         _at_x0 + 1
            sta         qd + 1
            lda         qd
            cmp         _at_w
            lda         qd + 1
            sbc         _at_w + 1
            jcs         @long
            lda         qd                  ;   and in the chunk
            sec
            sbc         _at_ox
            tay
            lda         qd + 1
            sbc         _at_ox + 1
            jne         @long
            cpy         #32                 ; (CW)
            jcs         @long
            sty         qx
            lda         qy                  ; y - y0, the same
            sec
            sbc         _at_y0
            sta         qd
            lda         qy + 1
            sbc         _at_y0 + 1
            sta         qd + 1
            lda         qd
            cmp         _at_h
            lda         qd + 1
            sbc         _at_h + 1
            jcs         @long
            lda         qd
            sec
            sbc         _at_oy
            tay
            lda         qd + 1
            sbc         _at_oy + 1
            jne         @long
            cpy         #32
            jcs         @long
            sty         _cy_
            lda         qx
            sta         _cx_
            lda         _at_bank            ; Its chunk in the window
            jsr         _hub_map
            lda         _at_chunk
            sta         _C
            lda         _at_chunk + 1
            sta         _C + 1
            lda         _cy_                ; The square: at_tile + cy_ * 32 + cx_
            lsr         a
            lsr         a
            lsr         a
            sta         tmp1                ; (cy_ * 32's high byte)
            lda         _cy_
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            ora         _cx_
            clc
            adc         _at_tile
            pha
            lda         tmp1
            adc         _at_tile + 1
            tax
            pla
            jmp         incsp3

@long:      lda         qy                  ; Any other: the long way, its arguments as they were
            ldx         qy + 1
            jmp         _at_find
