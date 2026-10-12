; ****************************************************************************
; rmdir dir ... - each empty directory removed (REMOVE: a file isn't, E_NOTDIR).  One that can't be is said
; ("rmdir: dir: why"), and rmdir ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "rmdir", main

.bss
st:         .res        SR_SIZE

.code
main:
            jsr         tl_start
@arg:
            lda         (tl_arg)
            beq         @end
            MOVR        r0, tl_arg
            LDR         r1, st
            jsr         STAT
            bcs         @failed
            lda         st + SR_QTYPE
            and         #QT_DIR
            bne         :+
            lda         #E_NOTDIR
            bra         @failed

:
            MOVR        r0, tl_arg
            jsr         REMOVE
            bcc         @next
@failed:
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
@next:
            jsr         tl_next
            bra         @arg

@end:
            jmp         tl_end

.rodata
tl_name:    .byte       "rmdir", 0
tl_flagset: .byte       0
tl_usage:   .byte       "rmdir dir ...", 0

.include "toollib.s"
