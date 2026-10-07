; ****************************************************************************
; blocks.s - the text's blocks, text.c's fast part (edit.h): the file shown's blocks (a bank each, its gap's start
; and end, its LFs: tables of BLK_MAX, each 16-bit one as a low and a high table), the cursor (a block, an offset in
; its text, its place and line), and the iterator (a block and an offset); moving them a byte at a time, putting a
; place to them, the gap moved to the cursor, bytes in and out there, a block split or given back, and a byte looked
; for along the text.  A block's text is its bank's bytes from 0 to gs, then from ge to BLK; the bank is at $8000.
; The block state (bstate) is saved and put back whole as the file shown changes.

            .export     _bstate, _nblk, _bkb, _gsl, _gsh, _gel, _geh, _nll, _nlh, _cb, _co, _cpos, _cline
            .export     _blen, _t_get, _t_next, _t_prev, _it_set, _it_next, _it_prev, _it_pos, _t_goto, _lfs
            .export     _gapcur, _ins1, _del1, _newblk, _delblk, _split, _fwd1, _quiet, _setbank, _uncr
            .import     popax, pushax, _memmove, _bounce

            .include    "zeropage.inc"
            .include    "hydra.inc"

BLK_MAX         = 48            ; edit.h's
BLK             = $2000
WINDOW          = $8000
BANKREG         = $00           ; The task's RAM bank register
NL              = $0A
CR              = $0D
CHUNK           = 256

            .zeropage
pb:         .res        1                                   ; The iterator: its block ...
po:         .res        2                                   ;   and offset
len:        .res        2                                   ; blen's, kept

            .bss
_bstate:
_nblk:      .res        1
_bkb:       .res        BLK_MAX
_gsl:       .res        BLK_MAX
_gsh:       .res        BLK_MAX
_gel:       .res        BLK_MAX
_geh:       .res        BLK_MAX
_nll:       .res        BLK_MAX
_nlh:       .res        BLK_MAX
_cb:        .res        1
_co:        .res        2
_cpos:      .res        4
_cline:     .res        2
.assert     * - _bstate = 1 + BLK_MAX * 7 + 9, error, "edit.h's BSTATE"
pp:         .res        4                                   ; A place being found
sbuf:       .res        CHUNK                               ; split's bytes, between banks
fg:         .res        2                                   ; fwd1's bytes

            .code

; Block X's text's length (BLK - (ge - gs)) in len, and .A (low), .Y (high).  Keeps .X
blen_x:
            sec
            lda         _gsl,X
            sbc         _gel,X
            sta         len
            lda         _gsh,X
            sbc         _geh,X
            clc
            adc         #>BLK
            sta         len + 1
            tay
            lda         len
            rts

; unsigned __fastcall__ blen (unsigned char b)
_blen:
            tax
            jsr         blen_x
            ldx         len + 1
            rts

; The byte at offset ptr1 of block X's text (its bank selected), in .A; ptr1 left its address.  Keeps .X
at_x:
            lda         _bkb,X
            sta         BANKREG
            lda         ptr1                                ; Past the gap's start: + (ge - gs)
            cmp         _gsl,X
            lda         ptr1 + 1
            sbc         _gsh,X
            bcc         :+
            clc
            lda         ptr1
            adc         _gel,X
            sta         ptr1
            lda         ptr1 + 1
            adc         _geh,X
            sta         ptr1 + 1
            sec
            lda         ptr1
            sbc         _gsl,X
            sta         ptr1
            lda         ptr1 + 1
            sbc         _gsh,X
            sta         ptr1 + 1
:
            lda         ptr1 + 1
            ora         #>WINDOW
            sta         ptr1 + 1
            lda         (ptr1)
            rts

; ---- A place (pb, po) moved a byte: .A = the byte passed (.X = 0), or .A = .X = $FF (none: at the end, the start)

pnext:
            ldx         pb
@again:
            jsr         blen_x
            lda         po                                  ; Past its block's text: the next block's start
            cmp         len
            lda         po + 1
            sbc         len + 1
            bcc         @in
            inx
            cpx         _nblk
            bcs         @none
            stx         pb
            stz         po
            stz         po + 1
            bra         @again
@none:
            lda         #$FF
            tax
            rts
@in:
            lda         po
            sta         ptr1
            lda         po + 1
            sta         ptr1 + 1
            jsr         at_x
            inc         po
            bne         :+
            inc         po + 1
:
            ldx         #0
            rts

pprev:
            ldx         pb
@again:
            lda         po                                  ; At its block's start: the last block's end
            ora         po + 1
            bne         @in
            txa
            beq         @none
            dex
            stx         pb
            jsr         blen_x
            sta         po
            sty         po + 1
            bra         @again
@none:
            lda         #$FF
            tax
            rts
@in:
            lda         po
            bne         :+
            dec         po + 1
:
            dec         po
            lda         po
            sta         ptr1
            lda         po + 1
            sta         ptr1 + 1
            jsr         at_x
            ldx         #0
            rts

; ---- The iterator

; int it_next (void), it_prev (void)
_it_next    = pnext
_it_prev    = pprev

; void __fastcall__ it_set (lpos p)
_it_set:
            jsr         walk
            stx         pb
            rts

; The place pp found: .X = its block, po = its offset (past the text's end: the end)
walk:
            sta         pp
            stx         pp + 1
            lda         sreg
            sta         pp + 2
            lda         sreg + 1
            sta         pp + 3
            ldx         #0
@block:
            jsr         blen_x
            lda         pp + 2                              ; In this block (pp <= its length), or the last?
            ora         pp + 3
            bne         :+
            lda         len
            cmp         pp
            lda         len + 1
            sbc         pp + 1
            bcs         @here
:
            inx
            cpx         _nblk
            beq         @last
            sec                                             ; pp -= its length
            lda         pp
            sbc         len
            sta         pp
            lda         pp + 1
            sbc         len + 1
            sta         pp + 1
            bcs         @block
            lda         pp + 2
            bne         :+
            dec         pp + 3
:
            dec         pp + 2
            bra         @block
@last:
            dex                                             ; (Past the end: the last block's end)
            jsr         blen_x
            sta         po
            sty         po + 1
            rts
@here:
            lda         pp
            sta         po
            lda         pp + 1
            sta         po + 1
            rts

; lpos it_pos (void): the iterator's place
_it_pos:
            ldx         pb
            lda         po
            ldy         po + 1
            ; (on into place)

; .X = a block, .A/.Y = an offset in it: its place in the text, in .A/.X/sreg
place:
            sta         pp
            sty         pp + 1
            stz         pp + 2
            stz         pp + 3
@block:
            dex
            bmi         @done
            jsr         blen_x
            clc
            lda         pp
            adc         len
            sta         pp
            lda         pp + 1
            adc         len + 1
            sta         pp + 1
            bcc         @block
            inc         pp + 2
            bne         @block
            inc         pp + 3
            bra         @block
@done:
            lda         pp + 2
            sta         sreg
            lda         pp + 3
            sta         sreg + 1
            lda         pp
            ldx         pp + 1
            rts

; ---- The cursor

; int t_get (void): the byte at the cursor (-1: at the end)
_t_get:
            jsr         cur_it
            jmp         pnext

; The iterator at the cursor
cur_it:
            lda         _cb
            sta         pb
            lda         _co
            sta         po
            lda         _co + 1
            sta         po + 1
            rts

; The cursor at the iterator
it_cur:
            lda         pb
            sta         _cb
            lda         po
            sta         _co
            lda         po + 1
            sta         _co + 1
            rts

; int t_next (void): the cursor past a byte: it, or -1 at the end
_t_next:
            jsr         cur_it
            jsr         pnext
            cpx         #0
            bne         @end
            pha
            jsr         it_cur
            inc         _cpos
            bne         :+
            inc         _cpos + 1
            bne         :+
            inc         _cpos + 2
            bne         :+
            inc         _cpos + 3
:
            pla
            cmp         #NL
            bne         :+
            inc         _cline
            bne         :+
            inc         _cline + 1
:
            ldx         #0
@end:
            rts

; int t_prev (void): the cursor back over a byte: it, or -1 at the start
_t_prev:
            jsr         cur_it
            jsr         pprev
            cpx         #0
            bne         @end
            pha
            jsr         it_cur
            lda         _cpos
            bne         @d0
            lda         _cpos + 1
            bne         @d1
            lda         _cpos + 2
            bne         @d2
            dec         _cpos + 3
@d2:
            dec         _cpos + 2
@d1:
            dec         _cpos + 1
@d0:
            dec         _cpos
            pla
            cmp         #NL
            bne         :+
            ldx         _cline
            bne         @l0
            dec         _cline + 1
@l0:
            dec         _cline
:
            ldx         #0
@end:
            rts

; void __fastcall__ t_goto (lpos p): the cursor to p, its place and line found
_t_goto:
            jsr         walk
            stx         pb
            jsr         it_cur
            lda         po                                  ; Its place
            ldy         po + 1
            jsr         place
            sta         _cpos
            stx         _cpos + 1
            lda         sreg
            sta         _cpos + 2
            lda         sreg + 1
            sta         _cpos + 3
            stz         _cline                              ; Its line: the LFs of the blocks before it ...
            stz         _cline + 1
            ldx         #0
@sum:
            cpx         _cb
            beq         @mine
            clc
            lda         _cline
            adc         _nll,X
            sta         _cline
            lda         _cline + 1
            adc         _nlh,X
            sta         _cline + 1
            inx
            bra         @sum
@mine:
            lda         _bkb,X                              ; ... and of its own before it: its text up to
            sta         BANKREG                             ;   the gap, and after it
            stz         ptr1
            lda         #>WINDOW
            sta         ptr1 + 1
            lda         _co
            cmp         _gsl,X
            lda         _co + 1
            sbc         _gsh,X
            bcc         @one
            lda         _gsl,X                              ; (Past the gap: all before it ...)
            sta         ptr2
            lda         _gsh,X
            sta         ptr2 + 1
            jsr         addlfs
            ldx         _cb
            lda         _gel,X                              ; (... and after it, co - gs)
            sta         ptr1
            lda         _geh,X
            ora         #>WINDOW
            sta         ptr1 + 1
            sec
            lda         _co
            sbc         _gsl,X
            sta         ptr2
            lda         _co + 1
            sbc         _gsh,X
            sta         ptr2 + 1
            jmp         addlfs
@one:
            lda         _co
            sta         ptr2
            lda         _co + 1
            sta         ptr2 + 1
            ; (on into addlfs)

; cline += the LFs of ptr2 bytes at ptr1
addlfs:
            jsr         lfs_p
            clc
            adc         _cline
            sta         _cline
            txa
            adc         _cline + 1
            sta         _cline + 1
            rts

; unsigned __fastcall__ lfs (const unsigned char* s, unsigned n): the LFs in n bytes at s
_lfs:
            sta         ptr2
            stx         ptr2 + 1
            jsr         popax
            sta         ptr1
            stx         ptr1 + 1
            ; (on into lfs_p)

; The LFs of ptr2 bytes at ptr1, in .A/.X
lfs_p:
            stz         tmp1
            stz         tmp2
            ldy         #0
            ldx         ptr2 + 1                            ; Whole pages first
            beq         @part
@page:
            lda         (ptr1),Y
            cmp         #NL
            bne         :+
            inc         tmp1
            bne         :+
            inc         tmp2
:
            iny
            bne         @page
            inc         ptr1 + 1
            dex
            bne         @page
@part:
            ldx         ptr2                                ; Then the rest
            beq         @done
@byte:
            lda         (ptr1),Y
            cmp         #NL
            bne         :+
            inc         tmp1
            bne         :+
            inc         tmp2
:
            iny
            dex
            bne         @byte
@done:
            lda         tmp1
            ldx         tmp2
            rts

; ---- Changes, at the cursor

; void gapcur (void): the cursor's block's gap moved to the cursor
_gapcur:
            ldx         _cb
            lda         _bkb,X
            sta         BANKREG
            lda         _co                                 ; co < gs: gs - co bytes up, to end at ge
            cmp         _gsl,X
            lda         _co + 1
            sbc         _gsh,X
            bcs         @up
            sec
            lda         _gsl,X                              ; n = gs - co; ge -= n
            sbc         _co
            sta         ptr2
            lda         _gsh,X
            sbc         _co + 1
            sta         ptr2 + 1
            sec
            lda         _gel,X
            sbc         ptr2
            sta         _gel,X
            lda         _geh,X
            sbc         ptr2 + 1
            sta         _geh,X
            lda         _gel,X                              ; memmove (W + ge, W + co, n)
            ldy         _geh,X
            jsr         pushw
            lda         _co
            ldy         _co + 1
            jsr         pushw
            bra         @move
@up:
            lda         _co                                 ; co == gs: nothing to move
            cmp         _gsl,X
            bne         :+
            lda         _co + 1
            cmp         _gsh,X
            beq         @done
:
            sec                                             ; n = co - gs: down from ge to gs
            lda         _co
            sbc         _gsl,X
            sta         ptr2
            lda         _co + 1
            sbc         _gsh,X
            sta         ptr2 + 1
            lda         _gsl,X                              ; memmove (W + gs, W + ge, n)
            ldy         _gsh,X
            jsr         pushw
            ldx         _cb
            lda         _gel,X
            ldy         _geh,X
            jsr         pushw
            ldx         _cb
            clc                                             ; ge += n
            lda         _gel,X
            adc         ptr2
            sta         _gel,X
            lda         _geh,X
            adc         ptr2 + 1
            sta         _geh,X
@move:
            lda         ptr2
            ldx         ptr2 + 1
            jsr         _memmove
            ldx         _cb
@done:
            lda         _co                                 ; gs = co
            sta         _gsl,X
            lda         _co + 1
            sta         _gsh,X
            rts

; An offset in the window (.A low, .Y high) pushed, for memmove.  Keeps ptr2
pushw:
            pha
            tya
            ora         #>WINDOW
            tax
            pla
            jmp         pushax

; unsigned __fastcall__ ins1 (const unsigned char* s, unsigned n): as many of n bytes at s as the gap (at the
; cursor: gapcur) has room for put in, the cursor after them: their count.  gs, co, cpos, nl and cline follow
_ins1:
            sta         ptr2
            stx         ptr2 + 1
            jsr         popax
            sta         ptr3                                ; (s)
            stx         ptr3 + 1
            ldx         _cb
            sec                                             ; The gap's room
            lda         _gel,X
            sbc         _gsl,X
            sta         tmp3
            lda         _geh,X
            sbc         _gsh,X
            sta         tmp4
            cmp         ptr2 + 1                            ; n = min (n, room)
            bcc         @room
            bne         @n
            lda         tmp3
            cmp         ptr2
            bcs         @n
@room:
            lda         tmp3
            sta         ptr2
            lda         tmp4
            sta         ptr2 + 1
@n:
            lda         _bkb,X
            sta         BANKREG
            lda         _gsl,X                              ; ptr1 = W + gs
            sta         ptr1
            lda         _gsh,X
            ora         #>WINDOW
            sta         ptr1 + 1
            lda         ptr2                                ; (The count, kept)
            sta         tmp3
            lda         ptr2 + 1
            sta         tmp4
            ldy         #0                                  ; The bytes, a page at a time
            ldx         ptr2 + 1
            beq         @part
@page:
            lda         (ptr3),Y
            sta         (ptr1),Y
            iny
            bne         @page
            inc         ptr1 + 1
            inc         ptr3 + 1
            dex
            bne         @page
@part:
            ldx         ptr2
            beq         @copied
@byte:
            lda         (ptr3),Y
            sta         (ptr1),Y
            iny
            dex
            bne         @byte
@copied:
            ldx         _cb                                 ; Their LFs: counted where they are now
            lda         _gsl,X
            sta         ptr1
            lda         _gsh,X
            ora         #>WINDOW
            sta         ptr1 + 1
            lda         tmp3
            sta         ptr2
            lda         tmp4
            sta         ptr2 + 1
            jsr         lfs_p
            sta         tmp1
            stx         tmp2
            ldx         _cb
            clc
            lda         _nll,X
            adc         tmp1
            sta         _nll,X
            lda         _nlh,X
            adc         tmp2
            sta         _nlh,X
            clc
            lda         _cline
            adc         tmp1
            sta         _cline
            lda         _cline + 1
            adc         tmp2
            sta         _cline + 1
            clc                                             ; gs, co += n
            lda         _gsl,X
            adc         tmp3
            sta         _gsl,X
            lda         _gsh,X
            adc         tmp4
            sta         _gsh,X
            clc
            lda         _co
            adc         tmp3
            sta         _co
            lda         _co + 1
            adc         tmp4
            sta         _co + 1
            clc                                             ; cpos += n
            lda         _cpos
            adc         tmp3
            sta         _cpos
            lda         _cpos + 1
            adc         tmp4
            sta         _cpos + 1
            bcc         :+
            inc         _cpos + 2
            bne         :+
            inc         _cpos + 3
:
            lda         tmp3
            ldx         tmp4
            rts

; unsigned __fastcall__ del1 (unsigned n): up to n bytes (CHUNK at most, and no more than the cursor's block has
; after it) after the cursor (its gap there: gapcur) out, into bounce: their count.  ge and nl follow
_del1:
            sta         ptr2
            stx         ptr2 + 1
            ldx         _cb
            jsr         blen_x                              ; What its block has after the cursor
            sec
            lda         len
            sbc         _co
            sta         tmp3
            lda         len + 1
            sbc         _co + 1
            sta         tmp4
            lda         ptr2 + 1                            ; n = min (n, that, CHUNK)
            cmp         tmp4
            bcc         :+
            bne         @that
            lda         ptr2
            cmp         tmp3
            bcc         :+
@that:
            lda         tmp3
            sta         ptr2
            lda         tmp4
            sta         ptr2 + 1
:
            lda         ptr2 + 1
            beq         :+
            stz         ptr2
            lda         #>CHUNK
            sta         ptr2 + 1
:
            lda         _bkb,X                              ; From W + ge, into bounce
            sta         BANKREG
            lda         _gel,X
            sta         ptr1
            lda         _geh,X
            ora         #>WINDOW
            sta         ptr1 + 1
            lda         ptr2 + 1
            beq         @part
            ldy         #0                                  ; (A whole CHUNK)
:
            lda         (ptr1),Y
            sta         _bounce,Y
            iny
            bne         :-
            bra         @copied
@part:
            ldy         ptr2
            beq         @copied
            ldy         #0
:
            lda         (ptr1),Y
            sta         _bounce,Y
            iny
            cpy         ptr2
            bne         :-
@copied:
            lda         ptr2                                ; (The count, kept: lfs_p uses ptr2)
            sta         tmp3
            lda         ptr2 + 1
            sta         tmp4
            lda         #<_bounce
            sta         ptr1
            lda         #>_bounce
            sta         ptr1 + 1
            jsr         lfs_p
            sta         tmp1
            stx         tmp2
            ldx         _cb
            sec
            lda         _nll,X
            sbc         tmp1
            sta         _nll,X
            lda         _nlh,X
            sbc         tmp2
            sta         _nlh,X
            clc                                             ; ge += n
            lda         _gel,X
            adc         tmp3
            sta         _gel,X
            lda         _geh,X
            adc         tmp4
            sta         _geh,X
            lda         tmp3
            ldx         tmp4
            rts

; ---- Blocks

; The 7 tables' entries from .X up moved up one (to make room at .X), or down one onto .X (shift_dn)
shift_up:
            stx         tmp1
            ldy         _nblk
@entry:
            cpy         tmp1
            beq         @done
            dey
            .repeat     7, T
            lda         _bkb + T * BLK_MAX,Y
            sta         _bkb + T * BLK_MAX + 1,Y
            .endrepeat
            bra         @entry
@done:
            inc         _nblk
            rts

shift_dn:
            txa
            tay
            dec         _nblk
@entry:
            cpy         _nblk
            beq         @done
            .repeat     7, T
            lda         _bkb + T * BLK_MAX + 1,Y
            sta         _bkb + T * BLK_MAX,Y
            .endrepeat
            iny
            bra         @entry
@done:
            rts

; unsigned char __fastcall__ newblk (unsigned char b): an empty block after block b.  0; or 1: no room (no bank,
; or the file's blocks all used)
_newblk:
            sta         tmp2
            ldx         _nblk
            cpx         #BLK_MAX
            bcs         @full
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @full
            sta         tmp3
            ldx         tmp2
            inx
            jsr         shift_up
            ldx         tmp2
            inx
            lda         tmp3
            sta         _bkb,X
            stz         _gsl,X
            stz         _gsh,X
            stz         _gel,X
            lda         #>BLK
            sta         _geh,X
            stz         _nll,X
            stz         _nlh,X
            lda         #0
            tax
            rts
@full:
            lda         #1
            ldx         #0
            rts

; void __fastcall__ delblk (unsigned char b): block b (empty) given back
_delblk:
            pha
            tax
            lda         _bkb,X
            ldx         #1
            jsr         BANKS_FREE
            plx
            jmp         shift_dn

; unsigned char split (void): the cursor's block, full (its gap empty: its text's bytes at their own offsets), its
; second half into a new block after it, at the same offsets.  0; or 1: no room
_split:
            lda         _cb
            jsr         _newblk
            cmp         #0
            beq         :+
            ldx         #0                                  ; (No room: 1)
            rts
:
            ldx         _cb
            lda         #>(WINDOW + BLK / 2)                ; The half, a page at a time
            sta         ptr1 + 1
            stz         ptr1
@page:
            lda         _bkb,X
            sta         BANKREG
            ldy         #0
:
            lda         (ptr1),Y
            sta         sbuf,Y
            iny
            bne         :-
            lda         _bkb + 1,X
            sta         BANKREG
:
            lda         sbuf,Y
            sta         (ptr1),Y
            iny
            bne         :-
            inc         ptr1 + 1
            lda         ptr1 + 1
            cmp         #>(WINDOW + BLK)
            bne         @page
            stz         ptr1                                ; Its LFs (the new bank selected)
            lda         #>(WINDOW + BLK / 2)
            sta         ptr1 + 1
            stz         ptr2
            lda         #>(BLK / 2)
            sta         ptr2 + 1
            jsr         lfs_p
            sta         tmp1
            stx         tmp2
            ldx         _cb
            lda         tmp1
            sta         _nll + 1,X
            lda         tmp2
            sta         _nlh + 1,X
            sec
            lda         _nll,X
            sbc         tmp1
            sta         _nll,X
            lda         _nlh,X
            sbc         tmp2
            sta         _nlh,X
            lda         #>(BLK / 2)                         ; The new: its gap 0 to BLK / 2; the old: BLK / 2
            sta         _geh + 1,X                          ;   to BLK
            sta         _gsh,X
            stz         _gel + 1,X
            stz         _gsl,X
            lda         #>BLK
            sta         _geh,X
            stz         _gel,X
            lda         _co + 1                             ; The cursor in the new, if it's past the half
            cmp         #>(BLK / 2)
            bcc         @ok
            bne         :+
            lda         _co
            beq         @ok
:
            inc         _cb
            sec
            lda         _co + 1
            sbc         #>(BLK / 2)
            sta         _co + 1
@ok:
            lda         #0
            tax
            rts

; ---- Finding

; unsigned char __fastcall__ fwd1 (unsigned fg): the iterator on to the next byte that's f or g (the low byte and
; the high: a letter's two cases), and past it.  0; or 1: none (it's at the end)
_fwd1:
            sta         fg
            stx         fg + 1
@block:
            ldx         pb
            jsr         blen_x
            lda         po                                  ; At its block's end: the next
            cmp         len
            lda         po + 1
            sbc         len + 1
            bcc         @in
            inx
            cpx         _nblk
            bcs         @none
            stx         pb
            stz         po
            stz         po + 1
            bra         @block
@none:
            lda         #1
            ldx         #0
            rts
@in:
            lda         _bkb,X
            sta         BANKREG
            stz         tmp3                                ; tmp3/4: the gap's length, if it's the second run
            stz         tmp4
            lda         po                                  ; Its run: to the gap, or from it to the end
            cmp         _gsl,X
            lda         po + 1
            sbc         _gsh,X
            bcs         @run2
            sec                                             ; ptr2 = gs - po: the bytes to look at
            lda         _gsl,X
            sbc         po
            sta         ptr2
            lda         _gsh,X
            sbc         po + 1
            sta         ptr2 + 1
            lda         po
            sta         ptr1
            lda         po + 1
            bra         @look
@run2:
            sec
            lda         _gel,X
            sbc         _gsl,X
            sta         tmp3
            lda         _geh,X
            sbc         _gsh,X
            sta         tmp4
            sec                                             ; ptr2 = len - po
            lda         len
            sbc         po
            sta         ptr2
            lda         len + 1
            sbc         po + 1
            sta         ptr2 + 1
            clc                                             ; ptr1 = po + the gap
            lda         po
            adc         tmp3
            sta         ptr1
            lda         po + 1
            adc         tmp4
@look:
            ora         #>WINDOW
            sta         ptr1 + 1
            ldy         #0
            ldx         ptr2 + 1                            ; Whole pages ...
            beq         @part
@page:
            lda         (ptr1),Y
            cmp         fg
            beq         @hit
            cmp         fg + 1
            beq         @hit
            iny
            bne         @page
            inc         ptr1 + 1
            dex
            bne         @page
@part:
            ldx         ptr2                                ; ... then the rest
            beq         @past
@byte:
            lda         (ptr1),Y
            cmp         fg
            beq         @hit
            cmp         fg + 1
            beq         @hit
            iny
            dex
            bne         @byte
@past:
            tya                                             ; None in this run: po on past it
            clc
            adc         ptr1
            sta         ptr1
            lda         ptr1 + 1
            adc         #0
            bra         @at
@hit:
            iny                                             ; Found: po just past it
            tya
            clc
            adc         ptr1
            sta         ptr1
            lda         ptr1 + 1
            adc         #0
            stz         ptr2                                ; (Found)
            bra         @set
@at:
            ldx         #1
            stx         ptr2
@set:
            and         #>(BLK - 1)                         ; po = the address less the window, less the gap
            sta         ptr1 + 1
            sec
            lda         ptr1
            sbc         tmp3
            sta         po
            lda         ptr1 + 1
            sbc         tmp4
            sta         po + 1
            lda         ptr2
            beq         :+
            jmp         @block
:
            tax
            rts

; unsigned __fastcall__ uncr (unsigned char* s, unsigned n): n bytes at s, each CR LF made LF: their count now
_uncr:
            sta         ptr2                                ; (n)
            stx         ptr2 + 1
            jsr         popax
            sta         ptr1                                ; From ...
            stx         ptr1 + 1
            sta         ptr3                                ;   to
            stx         ptr3 + 1
            sta         tmp3                                ; (s, for the count)
            stx         tmp4
@byte:
            lda         ptr2
            ora         ptr2 + 1
            beq         @done
            lda         ptr2
            bne         :+
            dec         ptr2 + 1
:
            dec         ptr2
            lda         (ptr1)
            inc         ptr1
            bne         :+
            inc         ptr1 + 1
:
            cmp         #CR
            bne         @keep
            ldx         ptr2                                ; (The last byte: kept)
            bne         :+
            ldx         ptr2 + 1
            beq         @keep
:
            pha                                             ; A CR before an LF: dropped
            lda         (ptr1)
            cmp         #NL
            beq         @drop
            pla
@keep:
            sta         (ptr3)
            inc         ptr3
            bne         @byte
            inc         ptr3 + 1
            bra         @byte
@drop:
            pla
            bra         @byte
@done:
            sec
            lda         ptr3
            sbc         tmp3
            pha
            lda         ptr3 + 1
            sbc         tmp4
            tax
            pla
            rts

; void __fastcall__ setbank (unsigned char b): the bank at $8000
_setbank:
            sta         BANKREG
            rts

; ---- Notes

; The note handler edit sets (NOTIFY): every note it can take, ignored (C = 0: it goes on).  (Ctrl-C does nothing)
_quiet:
            clc
            rts
