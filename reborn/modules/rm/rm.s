; ****************************************************************************
; rm [-rf] name ... - each file removed (REMOVE), or empty directory.  -r: a directory's tree first, depth first
; (toollib's tl_walk); -f: quiet about what can't be removed.  What can't be removed is said ("rm: name: why"), and
; rm ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "rm", main

F_R             = $01           ; -r
F_F             = $02           ; -f

.bss
path:       .res        PATH_MAX + 1                        ; What's being removed
st:         .res        SR_SIZE

.code
main:
            jsr         tl_start
            LDR         tl_pp, path
            LDR         tl_vdir, keep                       ; (-r: what's in a directory, then it)
            LDR         tl_vpost, remove
            LDR         tl_vfile, remove
@arg:
            lda         (tl_arg)
            beq         @end
            ldy         #$FF                                ; Its path
:
            iny
            lda         (tl_arg),Y
            sta         path,Y
            beq         :+
            cpy         #PATH_MAX
            bcc         :-
            MOVR        r0, tl_arg
            lda         #E_NAMETOOLONG
            jsr         tl_err
            bra         @next

:
            lda         tl_flags                            ; -r, and a directory: its tree first
            and         #F_R
            beq         @one
            LDR         r0, path
            LDR         r1, st
            jsr         STAT
            bcs         @one
            lda         st + SR_QTYPE
            and         #QT_DIR
            beq         @one
            jsr         tl_walk
@one:
            jsr         remove
@next:
            jsr         tl_next
            bra         @arg

@end:
            jmp         tl_end

; A directory, before what's in it: kept for after
keep:
            clc
            rts

; The path removed (it can't: said, unless -f)
remove:
            LDR         r0, path
            jsr         REMOVE
            bcc         keep
; Error .A about the path: said, unless -f
failed:
            tax
            lda         tl_flags
            and         #F_F
            bne         keep
            LDR         r0, path
            txa
            jmp         tl_err

.rodata
tl_name:    .byte       "rm", 0
tl_flagset: .byte       "rf", 0
tl_usage:   .byte       "rm [-rf] name ...", 0

.include "toollib.s"
