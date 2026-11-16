; ****************************************************************************
; oserror.s - int __fastcall__ __osmaperrno (unsigned char oserror): the kernel's error codes as errno's (the map:
; oserrmap.inc (beside this: made from spec/errors.def's last column); EUNKNOWN for a code it hasn't).  _oserror keeps
; the code itself (hydra.h: HY_E_*).

            .export     ___osmaperrno

            .include    "errno.inc"
            .include    "hydra.inc"

            .code

___osmaperrno:
            ldx         #0                  ; (From the first: E_AGAIN, a poll's, the commonest, its fourth.  From the
@look:                                      ;   last, a poll's every failed call went through the map whole, some 400
            cmp         map,X               ;   cycles: a twentieth of the game's world and UI as a step goes,
            beq         @found              ;   2026-11-16)
            inx
            inx
            cpx         #MAP_SIZE
            bcc         @look
            lda         #<EUNKNOWN
            ldx         #>EUNKNOWN
            rts

@found:
            lda         map + 1,X
            ldx         #0
            rts

            .rodata
map:
            .include    "oserrmap.inc"
MAP_SIZE        = * - map
.assert     MAP_SIZE    < 256, error, "oserror.s: .X counts the map's bytes"
