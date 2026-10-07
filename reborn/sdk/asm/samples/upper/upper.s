; ****************************************************************************
; upper [file ...] - a sample tool (the assembly SDK's: sdk/asm/README.md), built on toollib.s as the system's
; tools are: each file (none: fd 0's) to fd 1 in upper case.  toollib does the rest: the flags (it takes none: a -x
; is "usage: upper [file ...]"), a file that can't be read ("upper: name: why", and code 1), the output a buffer at a
; time, a write that fails.
;   % echo Hello | upper
;   HELLO

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"                                      ; (Its zero page, so the tool's code reaches it so)

            HYX2_PROGRAM "upper", main

.code
main:
            jsr         tl_start                            ; Its flags and arguments
            LDR         tl_ivec, file                       ; Each file, or fd 0, to file
            jsr         tl_eachin
            jmp         tl_end                              ; The output written; its code

; The input (tl_getc), in upper case
file:
            jsr         tl_getc
            bcs         @done                               ; (Its end)
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            jsr         tl_putc
            bra         file

@done:
            rts

.rodata
tl_name:    .byte       "upper", 0                          ; (toollib's: its name, flags and usage)
tl_flagset: .byte       0
tl_usage:   .byte       "upper [file ...]", 0

.include "toollib.s"
