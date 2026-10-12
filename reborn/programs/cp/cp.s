; ****************************************************************************
; cp [-r] from to; cp [-r] from ... dir - each file copied, 512 bytes at a time, to a file made (CREATE: one there
; already is emptied first), or, if the last name is a directory, into it by its own name.  -r: a directory's
; tree too, its directories made and its files copied (toollib's tl_walk); without it a directory isn't copied
; (E_ISDIR).  A file isn't copied onto itself.  What can't be copied is said ("cp: name: why"), and cp ends with
; code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "cp", main

F_R             = $01           ; -r

.zeropage
dst:        .res        2                                   ; The last name

.bss
spath:      .res        PATH_MAX + 1                        ; What's copied (the tree walked) ...
tpath:      .res        PATH_MAX + 1                        ;   and where to
tplen:      .res        TL_DEPTH + 1                        ; (tpath's length before each directory's name)
st:         .res        SR_SIZE                             ; From's record ...
st2:        .res        SR_SIZE                             ;   and to's
todir:      .res        1                                   ; <> 0: the last name is a directory
left:       .res        1                                   ; The names still to copy
fdin:       .res        1
fdout:      .res        1
buf:        .res        512

.code
main:
            jsr         tl_start
            LDR         tl_vdir, dir
            LDR         tl_vpost, post
            LDR         tl_vfile, file
            jsr         tl_count
            cmp         #2
            bcs         :+
            jmp         tl_badusage

:
            dec         a
            sta         left
            MOVR        dst, tl_arg                         ; The last name: a directory?
            ldx         left
@past:
            lda         (dst)                               ; (Past the others)
            inc         dst
            bne         :+
            inc         dst + 1
:
            cmp         #0
            bne         @past
            dex
            bne         @past
            stz         todir
            MOVR        r0, dst
            LDR         r1, st2
            jsr         STAT
            bcs         :+
            lda         st2 + SR_QTYPE
            and         #QT_DIR
            sta         todir
:
            lda         left                                ; Several: into a directory only
            cmp         #2
            bcc         @arg
            lda         todir
            bne         @arg
            MOVR        r0, dst
            lda         #E_NOTDIR
            jsr         tl_err
            jmp         tl_end

@arg:
            lda         left
            beq         @end
            jsr         one
            jsr         tl_next
            dec         left
            bra         @arg

@end:
            jmp         tl_end

; The name at tl_arg copied to dst (or into it)
one:
            MOVR        r0, tl_arg                          ; From: spath
            LDR         r1, spath
            jsr         put
            bcs         @long
            MOVR        r0, dst                             ; To: dst, or dst/its name
            LDR         r1, tpath
            jsr         put
            bcs         @long
            lda         todir
            beq         :+
            MOVR        r0, tl_arg
            jsr         tl_base
            jsr         tcat
            bcs         @long
:
            LDR         r0, spath
            LDR         r1, st
            jsr         STAT
            bcs         @failed
            lda         st + SR_QTYPE
            and         #QT_DIR
            bne         @dir
            LDR         r0, st                              ; A file: copied (not onto itself)
            jmp         copy

@dir:
            lda         tl_flags                            ; A directory: with -r, its tree
            and         #F_R
            bne         :+
            lda         #E_ISDIR
            bra         @failed

:
            jsr         mkdir
            bcs         @done
            LDR         tl_pp, spath
            jmp         tl_walk

@long:
            lda         #E_NAMETOOLONG
@failed:
            pha
            MOVR        r0, tl_arg
            pla
            jmp         tl_err

@done:
            rts

; The string at r0 into the path buffer at r1.  OUT: C = 1 if it's too long
put:
            ldy         #$FF
:
            iny
            cpy         #PATH_MAX + 1
            bcs         @done
            lda         (r0),Y
            sta         (r1),Y
            bne         :-
            clc
@done:
            rts

; A directory in the tree (its record at r0): made where it goes, then what's in it
dir:
            jsr         tcat
            bcs         :+
            ldx         tl_depth
            sta         tplen,X
            jsr         mkdir
            bcc         @done                               ; (C = 0: what's in it, next)
            ldx         tl_depth
            lda         tplen,X
            jsr         tcut
            sec                                             ; (Not made: skipped)
@done:
            rts

:
            jsr         toolong
            sec
            rts

; After what's in it: back out of it
post:
            ldx         tl_depth
            lda         tplen,X
            jmp         tcut

; A file in the tree (its record at r0): copied
file:
            jsr         tcat
            bcs         toolong
            pha
            jsr         copy
            pla
            jmp         tcut

; spath's place in to too long: said
toolong:
            LDR         r0, spath
            lda         #E_NAMETOOLONG
            jmp         tl_err

; The directory at tpath made (one there already will do).  OUT: C = 0; or C = 1 (said)
mkdir:
            LDR         r0, tpath
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcs         :+
            jsr         CLOSE
            clc
            rts

:
            cmp         #E_EXIST
            clc
            beq         @done
            pha
            LDR         r0, tpath
            pla
            jsr         tl_err
            sec
@done:
            rts

; The name at r0 onto tpath (a / first).  OUT: C = 0, .A = its old length; or C = 1, .A = E_NAMETOOLONG
tcat:
            LDR         tl_pp, tpath
            jsr         tl_pcat
            php
            pha
            LDR         tl_pp, spath
            pla
            plp
            rts

; tpath cut to .A bytes
tcut:
            pha
            LDR         tl_pp, tpath
            pla
            jsr         tl_pcut
            LDR         tl_pp, spath
            rts

; The file spath copied to tpath (its record at r0: not onto itself)
copy:
            MOVR        tl_q, r0
            LDR         r0, tpath                           ; The same file?  (Its device, instance and qid)
            LDR         r1, st2
            jsr         STAT
            bcs         @open
            ldx         #0
:
            lda         same,X
            tay
            lda         (tl_q),Y
            cmp         st2,Y
            bne         @open
            inx
            cpx         #SAME_N
            bne         :-
            LDR         r0, tpath
            jsr         tl_prefix
            LDR         r0, s_same
            jsr         tl_puts2
            lda         #1
            sta         tl_code
            rts

@open:
            LDR         r0, spath
            lda         #O_READ
            jsr         OPEN
            bcs         @sfail
            sta         fdin
            LDR         r0, tpath
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcs         @tfail
            sta         fdout
@read:
            LDR         r0, buf
            LDR         r1, 512
            lda         fdin
            jsr         READ
            bcs         @rfail
            sta         r1
            stx         r1 + 1
            ora         r1 + 1
            beq         @close
            LDR         r0, buf
            lda         fdout
            jsr         WRITE
            bcc         @read
            pha                                             ; (To: it can't be written)
            jsr         @close
            LDR         r0, tpath
            pla
            jmp         tl_err

@rfail:
            pha
            jsr         @close
            bra         @sname

@tfail:
            pha
            lda         fdin
            jsr         CLOSE
            LDR         r0, tpath
            pla
            jmp         tl_err

@sfail:
            pha
@sname:
            LDR         r0, spath
            pla
            jmp         tl_err

@close:
            lda         fdout
            jsr         CLOSE
            lda         fdin
            jmp         CLOSE

.rodata
same:       .byte       SR_DEV, SR_INST, SR_QPATH, SR_QPATH + 1, SR_QPATH + 2, SR_QPATH + 3
SAME_N      = * - same
s_same:     .byte       ": the same file", LF, 0
tl_name:    .byte       "cp", 0
tl_flagset: .byte       "r", 0
tl_usage:   .byte       "cp [-r] from to, or cp [-r] from ... dir", 0

.include "toollib.s"

