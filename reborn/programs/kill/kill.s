; ****************************************************************************
; kill [-i] task ... - each task (its number) ended: the note NOTE_KILL, which no handler catches; -i: interrupted
; instead (NOTE_INTERRUPT, Ctrl-C's).  A task that isn't there is said ("kill: task: why"), and kill ends with
; code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "kill", main

F_I             = $01           ; -i

.code
main:
            jsr         tl_start
            lda         (tl_arg)
            bne         @arg
            jmp         tl_badusage

@arg:
            MOVR        r0, tl_arg
            jsr         tl_atoi
            lda         #E_INVAL
            bcs         @failed
            lda         tl_num + 1                          ; (0-15)
            ora         tl_num + 2
            ora         tl_num + 3
            bne         @srch
            lda         tl_num
            cmp         #16
            bcs         @srch
            ldx         #NOTE_KILL
            lda         tl_flags
            and         #F_I
            beq         :+
            ldx         #NOTE_INTERRUPT
:
            lda         tl_num
            jsr         NOTE
            bcc         @next
            bra         @failed

@srch:
            lda         #E_SRCH
@failed:
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
@next:
            jsr         tl_next
            bne         @arg
            jmp         tl_end

.rodata
tl_name:    .byte       "kill", 0
tl_flagset: .byte       "i", 0
tl_usage:   .byte       "kill [-i] task ...", 0

.include "toollib.s"
