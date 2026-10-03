; ****************************************************************************
; oserror.s - int __fastcall__ __osmaperrno (unsigned char oserror): the kernel's error codes as errno's (the map:
; obj/sdk/c/oserrmap.inc, made from spec/errors.def's last column; EUNKNOWN for a code it hasn't).  _oserror keeps
; the code itself (hydra.h: HY_E_*).

            .export     ___osmaperrno

            .include    "errno.inc"
            .include    "hydra.inc"

            .code

___osmaperrno:
            ldx         #MAP_SIZE - 2
@look:
            cmp         map,X
            beq         @found
            dex
            dex
            bpl         @look
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
.assert     MAP_SIZE    < 128, error, "oserror.s: .X counts down with bpl"
