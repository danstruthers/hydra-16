; ****************************************************************************
; ovunpack.inc - a Dingus Quest Zero task's overlays read into its banks (each task's bank.s includes it): the unpack
; library found (MODINFO: the module named unpack) and called (XCALL its entry, $A030 in its bank), the task's
; overlays' file packed by ovlpack.js read and unpacked into the banks _ovbank names, from the first on (the library's
; UNPACK_BANKS: modules/unpack).  The module's directory entry is looked at in the first bank (its window: the
; library fills it after).  2026-10-31: the game's start, the card's read the fewer bytes.
;   unsigned char __fastcall__ ovl_unpack (unsigned fb)    fb: the file's fd (low byte), the banks at most (high):
;                                                           0, or not 0 (no library, or the file not as it should be).
;                                                           _ovbank[0] the first bank (the caller's): the rest of
;                                                           them, so many, set here, a run after it

            .export     _ovl_unpack

.scope ovu
MODINFO     = $F857                                         ; (hydra.inc's)
XCALL       = $F80C
r0          = $02
r14         = $1E
r15         = $20
ME_BANK     = 0
ME_NAME     = 4
ME_PAGE     = 15
BUF         = $9FE0                                         ; (MODINFO's answer, ME_SIZE bytes; then the entry
                                                            ;   looked at)
.endscope

.pushseg
.code
.proc _ovl_unpack
            pha                                             ; (The fd, and the banks)
            phx
            lda         _ovbank                             ; (_ovbank: the first bank, then the rest after it)
            ldy         #0
fill:
            sta         _ovbank,y
            ina
            iny
            dex
            bne         fill
            ldy         _ovbank                             ; (The first bank in the window: the buffer's)
            sty         $00
            stz         ovu::BUF + 18
find:
            lda         #<ovu::BUF
            sta         ovu::r0
            lda         #>ovu::BUF
            sta         ovu::r0 + 1
            lda         ovu::BUF + 18
            inc         ovu::BUF + 18
            jsr         ovu::MODINFO
            bcs         none                                ; (Past the last: no library, C = 1)
            ldx         #6
name:
            lda         unpack,x
            cmp         ovu::BUF + ovu::ME_NAME,x
            bne         find
            dex
            bpl         name
            lda         ovu::BUF + ovu::ME_BANK             ; Its bank, and its entry: $A030 in it (at its page)
            sta         ovu::r14
            lda         #$30
            sta         ovu::r15
            lda         ovu::BUF + ovu::ME_PAGE
            clc
            adc         #$A0
            sta         ovu::r15 + 1
            plx
            pla
            ldy         _ovbank
            jsr         ovu::XCALL
done:
            lda         #0
            tax
            rol                                             ; (C: 1, it failed)
            rts
none:
            plx
            pla
            bra         done
unpack:     .byte       "unpack", 0
.endproc
.popseg
