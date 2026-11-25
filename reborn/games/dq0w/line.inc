; ****************************************************************************
; line.inc - (the world's line.s, the senses' dq0p/line.s) one line between two squares, line.h's: the game's
; library's (the paged ROM's, modules/dq0lib/line.s, 2026-11-14; this file was the line itself, assembled into each,
; from 2026-11-08), each call an XCALL of it, its arguments taken off the C stack into the zero page's block
; (../dq0/lnzp.inc's LZ_*), where it keeps its work and the task's C reads its answers (line.h's ln_n ...: at.s's
; names).  Its squares read by the library's own at () (the task's chunk state, at.s's), the task's ln_way called back.
; The line's stub in the segment its includer sets (the world's part missiles: its line of fire's, wshoot.c's lof, no other
; part draws one), ln_pass's and ln_portal's in the main RAM (the world's: a missile's flight on, part missile_fall's; a smell's
; way, wtrace.c's).
;
;   unsigned char __fastcall__ line (unsigned char sn, unsigned x, unsigned y, unsigned tx, unsigned ty,
;       unsigned char mode);            0 clear, LN_STOP a square stops it, LN_WAY ln_way's said so (mode LN_CACHED
;                                       ;   too: a chunk not in the cache stops it, none made)
;   unsigned char __fastcall__ ln_pass (unsigned char sn, unsigned x, unsigned y, unsigned char mode);
;                                       a square: 0 it stops a line, 1 it lets it through, 2 narrowly (a slit, a window)
;   unsigned char __fastcall__ ln_portal (const struct portal* o, unsigned char mode);     a portal: the same

            .export     _line, _ln_pass, _ln_portal
            .import     incsp2, addysp, dq0x
            .importzp   sp
            .include    "../dq0/lnzp.inc"

; line (sn, x, y, tx, ty, mode): mode in .A, ty, tx, y, x and sn on the C stack (in the segment the includer set)
_line:
            sta         LZ_MODE
            ldy         #8
            lda         (sp),y
            sta         LZ_SN
            dey
:
            lda         (sp),y
            sta         LZ_XY,y
            dey
            bpl         :-
            ldy         #9
            jsr         addysp
            lda         #LN_LINE
            jmp         dq0x

            .code
; ln_pass (sn, x, y, mode): mode in .A, y, x and sn on the C stack
_ln_pass:
            sta         LZ_MODE
            ldy         #4
            lda         (sp),y
            sta         LZ_SN
            dey
:
            lda         (sp),y
            sta         LZ_LY,y
            dey
            bpl         :-
            ldy         #5
            jsr         addysp
            lda         #LN_PASS
            jmp         dq0x

; ln_portal (o, mode): mode in .A, o on the C stack
_ln_portal:
            sta         LZ_MODE
            ldy         #1
            lda         (sp),y
            sta         $03                 ; (r0)
            dey
            lda         (sp),y
            sta         $02
            jsr         incsp2
            lda         #LN_PORTAL
            jmp         dq0x
