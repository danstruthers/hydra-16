; ****************************************************************************
; mkdir [-p] dir ... - each directory made (CREATE, DM_DIR).  -p: the directories it's in too, as needed, and one
; that's there already isn't an error.  One that can't be made is said ("mkdir: dir: why"), and mkdir ends with
; code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "mkdir", main

F_P             = $01           ; -p

.bss
path:       .res        PATH_MAX + 1                        ; (-p: the directories it's in, in turn)

.code
main:
            jsr         tl_start
@arg:
            lda         (tl_arg)
            beq         @end
            lda         tl_flags
            and         #F_P
            beq         @one
            jsr         parents
            bcc         @next
            bra         @failed

@one:
            MOVR        r0, tl_arg
            jsr         make
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

; The directory at tl_arg, and each it's in (one there already: as it is).  OUT: C = 0; or C = 1, .A = the error
parents:
            ldy         #0
@byte:
            lda         (tl_arg),Y
            beq         @whole
            cpy         #PATH_MAX
            bcs         @long
            sta         path,Y
            cmp         #'/'                                ; Up to a / (not the first): a directory it's in
            bne         @on
            cpy         #0
            beq         @on
            lda         #0
            sta         path,Y
            phy
            LDR         r0, path
            jsr         make
            ply
            jsr         there
            bcs         @done
            lda         #'/'
            sta         path,Y
@on:
            iny
            bra         @byte

@whole:
            sta         path,Y
            LDR         r0, path
            jsr         make
            jmp         there

@done:
            rts

@long:
            lda         #E_NAMETOOLONG
            sec
            rts

; make's C and .A, with a directory there already as good as made (E_EXIST: C = 0)
there:
            bcc         @done
            cmp         #E_EXIST
            sec
            bne         @done
            clc
@done:
            rts

; The directory r0 made.  OUT: C = 0; or C = 1, .A = the error
make:
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcs         :+
            jsr         CLOSE
            clc
:
            rts

.rodata
tl_name:    .byte       "mkdir", 0
tl_flagset: .byte       "p", 0
tl_usage:   .byte       "mkdir [-p] dir ...", 0

.include "toollib.s"
