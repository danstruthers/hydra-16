; ****************************************************************************
; label disk [text ...] - the disk's HydraFS label: set to the words after the disk, a space between (its ctl's
; "label", #d/disk/ctl); none, said (its ctl's "hydrafs label=" line).  What can't be done is said ("label: disk:
; why").  A RAM program on the ROM disk (/rom/bin).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "label", main

.code
main:
            jsr         tl_start
            lda         (tl_arg)
            bne         :+
            jmp         tl_badusage

:
            jsr         dc_path
            bcs         @failed
            lda         tl_arg                              ; (The disk's name, for its errors)
            pha
            lda         tl_arg + 1
            pha
            jsr         tl_next
            beq         @show
            stz         dc_len                              ; "label TEXT"
            LDR         r0, s_label
            jsr         dc_word
@word:
            MOVR        r0, tl_arg
            jsr         dc_word
            jsr         tl_next
            bne         @word
            pla
            sta         tl_arg + 1
            pla
            sta         tl_arg
            jsr         dc_send
            bcs         @failed
            jmp         tl_end

@show:
            pla
            sta         tl_arg + 1
            pla
            sta         tl_arg
            jsr         dc_read                             ; Its label: "hydrafs label=" ...
            bcs         @failed
            ldx         #0
@line:
            ldy         #0
:
            lda         s_has,Y
            beq         @found
            cmp         dc_text,X
            bne         @skip
            inx
            iny
            bra         :-

@skip:
            lda         dc_text,X                           ; (The next line)
            beq         @end
            inx
            cmp         #LF
            bne         @skip
            bra         @line

@found:
            lda         dc_text,X                           ; ... the rest of its line
            beq         :+
            cmp         #LF
            beq         :+
            jsr         tl_putc
            inx
            bne         @found
:
            jsr         tl_nl
            bra         @end

@failed:
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
@end:
            jmp         tl_end

.rodata
s_label:    .byte       "label", 0
s_has:      .byte       "hydrafs label=", 0
tl_name:    .byte       "label", 0
tl_flagset: .byte       0
tl_usage:   .byte       "label disk [text ...]", 0

.include "toollib.s"
.include "diskctl.s"
