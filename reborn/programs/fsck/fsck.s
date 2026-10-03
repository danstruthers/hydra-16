; ****************************************************************************
; fsck [-f] disk - the disk's HydraFS checked (its ctl's "check", #d/disk/ctl: the storage driver does the work;
; -f: "check fix", its faults fixed), then the ctl's lines after its first (its label, the space free, what the
; check found).  What can't be done is said ("fsck: disk: why").  A RAM program on the ROM disk (/rom/bin).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "fsck", main

F_F             = $01           ; -f

.code
main:
            jsr         tl_start
            lda         (tl_arg)
            bne         :+
            jmp         tl_badusage

:
            jsr         dc_path
            bcs         @failed
            stz         dc_len                              ; "check [fix]"
            LDR         r0, s_check
            jsr         dc_word
            lda         tl_flags
            and         #F_F
            beq         :+
            LDR         r0, s_fix
            jsr         dc_word
:
            jsr         dc_send
            bcs         @failed
            jsr         dc_read                             ; What it found: the lines after the first
            bcs         @failed
            ldx         #0
:
            lda         dc_text,X
            beq         @end
            inx
            cmp         #LF
            bne         :-
:
            lda         dc_text,X
            beq         @end
            jsr         tl_putc
            inx
            bne         :-
            bra         @end

@failed:
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
@end:
            jmp         tl_end

.rodata
s_check:    .byte       "check", 0
s_fix:      .byte       "fix", 0
tl_name:    .byte       "fsck", 0
tl_flagset: .byte       "f", 0
tl_usage:   .byte       "fsck [-f] disk", 0

.include "toollib.s"
.include "diskctl.s"
