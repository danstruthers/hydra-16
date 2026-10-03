; ****************************************************************************
; ns [task] - a task's namespace (none: this one's), as the binds and mounts that make it: its /proc's ns
; (#p/N/ns, kdev's), copied to fd 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "ns", main

.bss
path:       .res        16                                  ; "#p/N/ns"
buf:        .res        256
fd:         .res        1

.code
main:
            jsr         tl_start
            lda         (tl_arg)
            beq         @self
            MOVR        r0, tl_arg                          ; A task: 0-15
            jsr         tl_atoi
            bcs         @usage
            lda         tl_num + 1
            ora         tl_num + 2
            ora         tl_num + 3
            bne         @usage
            lda         tl_num
            cmp         #16
            bcc         @task
@usage:
            jmp         tl_badusage

@self:
            jsr         GETPID
@task:
            ldy         #'#'                                ; "#p/N/ns"
            sty         path
            ldy         #'p'
            sty         path + 1
            ldy         #'/'
            sty         path + 2
            ldx         #3
            cmp         #10
            bcc         :+
            pha
            lda         #'1'
            sta         path,X
            pla
            inx
            sec
            sbc         #10
:
            ora         #'0'
            sta         path,X
            inx
            ldy         #0
:
            lda         s_ns,Y
            sta         path,X
            inx
            iny
            cmp         #0
            bne         :-
            LDR         r0, path
            lda         #O_READ
            jsr         OPEN
            bcs         @failed
            sta         fd
@read:
            LDR         r0, buf
            LDR         r1, 255                             ; (Its count: .A alone)
            lda         fd
            jsr         READ
            bcs         @failed
            tay
            beq         @end
            ldx         #0
:
            lda         buf,X
            jsr         tl_putc
            inx
            dey
            bne         :-
            bra         @read

@failed:
            pha
            LDR         r0, path
            pla
            jsr         tl_err
@end:
            jmp         tl_end

.rodata
s_ns:       .byte       "/ns", 0
tl_name:    .byte       "ns", 0
tl_flagset: .byte       0
tl_usage:   .byte       "ns [task]", 0

.include "toollib.s"
