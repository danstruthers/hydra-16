; ****************************************************************************
; at.s - (the world's; the denizens' and the senses' include it) world.h's at (): a square's chunk in the window and
; the square in it.  The quick way here, in assembly: a square of the chunk at () found last (its stratum, its bounds,
; the chunk's first square, its bank, its squares), worked out without multiplying, shifting or the C stack (cc65's
; at () took some 1,500 cycles a square, and a look about asks for thousands; an XCALL each would double it); any
; other, the game's library's AT_FIND (the paged ROM's, modules/dq0lib/sq.s: the hub's cache, or the maker), which sets
; the quick way for the next.  The task's chunk state is here (../dq0/lnzp.inc's struct ats: the quick way's, the long
; way's, world.h's C, cx_, cy_), with what the library calls back (ask_chunk: ask_maker's) and the zero page's block
; the library's squares' code keeps (lnzp.inc's LZ: line.h's ln_eye ..., walk.h's wk_* named for the C here).

            .macpack    longbranch
            .export     _at, _C, _cx_, _cy_, _at_me, _lz_init
            .export     _at_sn, _at_x0, _at_y0, _at_w, _at_h, _at_ox, _at_oy, _at_bank, _at_chunk, _at_tile
            .exportzp   _lr0, _lr1, _lr2, _ln_eye, _ln_n, _ln_first, _ln_last, _ln_way, _ln_x, _ln_y
            .exportzp   _wk_cl, _wk_el, _wk_ring, _wk_rules, _wk_x0, _wk_y0, _wk_w, _wk_h, _wk_qh, _wk_qt, _wk_stop
            .exportzp   _wk_reach, _wk_u, _wk_v, _wk_e, _wk_dz, _wk_v8, _wk_rt, _wk_rd, _wk_rn
            .import     _hub_map, _ask_maker, _strata, _t_kinds, _cornk, _dir_x, _dir_y, dq0x
            .import     incsp3, pusha, pushax
            .importzp   sp, tmp1
            .include    "../dq0/lnzp.inc"
            .include    "dq0hub_at.inc"             ; (world.h's CW, M_CHUNK: the build's, ../dq0/hubgen.js)

r0          = $02                           ; (The call registers: the library's arguments)
r1          = $04
r2          = $06
_lr0        = r0                            ; (lnzp.h's: C's names for them)
_lr1        = r1
_lr2        = r2
_ln_eye     = LZ_EYE                        ; (line.h's)
_ln_n       = LZ_N
_ln_first   = LZ_FIRST
_ln_last    = LZ_LAST
_ln_way     = LZ_WAY
_ln_x       = LZ_LX
_ln_y       = LZ_LY
_wk_cl      = LZ_CL                         ; (walk.h's)
_wk_el      = LZ_EL
_wk_ring    = LZ_RING
_wk_rules   = LZ_RULES
_wk_x0      = LZ_X0
_wk_y0      = LZ_Y0
_wk_w       = LZ_W
_wk_h       = LZ_H
_wk_qh      = LZ_QH
_wk_qt      = LZ_QT
_wk_stop    = LZ_STOP
_wk_reach   = LZ_REACH
_wk_u       = LZ_U
_wk_v       = LZ_V
_wk_e       = LZ_DIR
_wk_dz      = LZ_DZ
_wk_v8      = LZ_V8
_wk_rt      = LZ_U
_wk_rd      = LZ_V
_wk_rn      = LZ_DIR

            .data
ats:        .word       ask_chunk, _strata, _t_kinds, _cornk, _dir_x, _dir_y
_at_me:     .byte       0                   ; (ASK_*: chunks_init's)
_at_sn:     .byte       $FF                 ; at ()'s quick way: the stratum found last ($FF: none), its bounds; the
_at_x0:     .word       0                   ;   chunk, its first square in the stratum's own, its bank, its squares
_at_y0:     .word       0
_at_w:      .word       0
_at_h:      .word       0
_at_ox:     .word       0
_at_oy:     .word       0
_at_bank:   .byte       0
_at_chunk:  .word       0
_at_tile:   .word       0
_C:         .word       0                   ; world.h's C, cx_, cy_
_cx_:       .byte       0
_cy_:       .byte       0
            .byte       $FF                 ; (The long way's: the slot found last, none yet; what it holds, its bank
            .res        ATS_SIZE - ATS_LASTK - 1    ;   and chunk)
.assert     _at_me - ats = ATS_ME && _at_sn - ats = ATS_SN && _at_x0 - ats = ATS_X0 && _at_tile - ats = ATS_TILE, error, "at.s: struct ats"
.assert     _C - ats = ATS_C && _cy_ - ats = ATS_CY && _cy_ + 1 - ats = ATS_LASTK, error, "at.s: struct ats"

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
            cpy         #CW
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
            cpy         #CW
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

@long:      lda         qy                  ; Any other: the long way, the library's (sn, x, y: r0, r1, r2)
            sta         r2
            lda         qy + 1
            sta         r2 + 1
            ldy         #2
            lda         (sp),y
            sta         r0
            dey
            lda         (sp),y
            sta         r1 + 1
            dey
            lda         (sp),y
            sta         r1
            lda         #AT_FIND
            jsr         dq0x
            jmp         incsp3

; The library's: the maker asked for chunk r1, r2 of stratum r0 (chunk.c's ask_maker (M_CHUNK, sn, cx, cy))
ask_chunk:
            lda         #M_CHUNK
            jsr         pusha
            lda         r0
            jsr         pusha
            lda         r1
            ldx         r1 + 1
            jsr         pushax
            lda         r2
            ldx         r2 + 1
            jmp         _ask_maker

; void lz_init (void): the zero page's block made ready (chunks_init's, as the task starts): the task's chunk state,
; its hub_map; no ln_way, the eyes standing (line.h's LN_EYE)
_lz_init:
            lda         #<ats
            sta         LZ_ATS
            lda         #>ats
            sta         LZ_ATS + 1
            lda         #<_hub_map
            sta         LZ_MAP
            lda         #>_hub_map
            sta         LZ_MAP + 1
            stz         LZ_WAY
            stz         LZ_WAY + 1
            lda         #5
            sta         LZ_EYE
            rts
