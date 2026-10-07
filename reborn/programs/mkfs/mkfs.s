; ****************************************************************************
; mkfs [-fp] disk [label ...] - a new, empty HydraFS on the disk (its ctl's "format", #d/disk/ctl: the storage
; driver does the work): -f a full format (its whole free map written); -p in a partition of its own, after a
; card's others; the label, the words after the disk.  What can't be done is said ("mkfs: disk: why").  A RAM
; program on the ROM disk (/rom/bin).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "mkfs", main

F_F             = $01           ; -f
F_P             = $02           ; -p

.code
main:
            jsr         tl_start
            lda         (tl_arg)
            bne         :+
            jmp         tl_badusage

:
            jsr         dc_path
            bcs         @failed
            stz         dc_len                              ; "format [-f] [-p] [label]"
            LDR         r0, s_format
            jsr         dc_word
            lda         tl_flags
            and         #F_F
            beq         :+
            LDR         r0, s_f
            jsr         dc_word
:
            lda         tl_flags
            and         #F_P
            beq         :+
            LDR         r0, s_p
            jsr         dc_word
:
            lda         tl_arg                              ; (The disk's name, for its errors)
            pha
            lda         tl_arg + 1
            pha
@label:
            jsr         tl_next
            beq         :+
            MOVR        r0, tl_arg
            jsr         dc_word
            bra         @label

:
            pla
            sta         tl_arg + 1
            pla
            sta         tl_arg
            jsr         dc_send
            bcc         @end
@failed:
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
@end:
            jmp         tl_end

.rodata
s_format:   .byte       "format", 0
s_f:        .byte       "-f", 0
s_p:        .byte       "-p", 0
tl_name:    .byte       "mkfs", 0
tl_flagset: .byte       "fp", 0
tl_usage:   .byte       "mkfs [-fp] disk [label ...]", 0

.include "toollib.s"
.include "diskctl.s"
