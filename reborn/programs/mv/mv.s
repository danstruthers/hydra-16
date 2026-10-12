; ****************************************************************************
; mv from to; mv from ... dir - each file or directory moved to to, or, if the last name is a directory, into it
; by its own name.  In its own directory (to's has the same device, instance and qid) it's renamed (WSTAT: its new
; name, nothing else; a file there by that name removed first); to another, a file is copied and then removed, and
; a directory can't be moved (E_ISDIR), as in Plan 9.  What can't be moved is said ("mv: name: why"), and mv ends
; with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "mv", main

.zeropage
dst:        .res        2                                   ; The last name

.bss
tpath:      .res        PATH_MAX + 1                        ; Where it goes
dpath:      .res        PATH_MAX + 1                        ; A directory a name is in
st:         .res        SR_SIZE                             ; From's record ...
st2:        .res        SR_SIZE                             ;   to's ...
sd:         .res        SR_SIZE                             ;   from's directory's ...
td:         .res        SR_SIZE                             ;   and to's
todir:      .res        1                                   ; <> 0: the last name is a directory
left:       .res        1                                   ; The names still to move
fdin:       .res        1
fdout:      .res        1
buf:        .res        512

.code
main:
            jsr         tl_start
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
            bcc         :+
            tax                                             ; (0: said already)
            beq         :+
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
:
            jsr         tl_next
            dec         left
            bra         @arg

@end:
            jmp         tl_end

; The name at tl_arg moved.  OUT: C = 0; or C = 1, .A = the error (about it; 0: one about to, said)
one:
            jsr         where
            bcc         :+
            rts

:
            LDR         r0, sd                              ; The directories: the same one?
            LDR         r1, td
            jsr         same
            bne         @other
            LDR         r0, tpath                           ; The same: a file there by its new name, gone first
            LDR         r1, st2
            jsr         STAT
            bcs         @rename
            LDR         r0, st
            LDR         r1, st2
            jsr         same
            beq         @ok                                 ; (Itself: as it is)
            lda         st2 + SR_QTYPE
            and         #QT_DIR
            beq         :+
            lda         #E_ISDIR
            sec
            rts

:
            LDR         r0, tpath
            jsr         REMOVE
            bcs         @to
@rename:
            ldx         #SR_SIZE - 1                        ; Its new name, nothing else
            lda         #$FF
:
            sta         st2,X
            dex
            bpl         :-
            LDR         r0, tpath
            jsr         tl_base
            ldy         #$FF
:
            iny
            lda         (r0),Y
            sta         st2 + SR_NAME,Y
            bne         :-
            MOVR        r0, tl_arg
            LDR         r1, st2
            jmp         WSTAT

@other:
            lda         st + SR_QTYPE                       ; Another directory: a file copied, then removed
            and         #QT_DIR
            beq         :+
            lda         #E_ISDIR
            sec
            rts

:
            jsr         copy
            bcs         @done
            MOVR        r0, tl_arg
            jmp         REMOVE

@ok:
            clc
@done:
            rts

@to:                                                        ; (An error about to: said here)
            pha
            LDR         r0, tpath
            pla
            jsr         tl_err
            lda         #0
            sec
            rts

; The name at tl_arg: its record (st), where it goes (tpath), and the directories both are in (sd, td).  OUT:
; C = 0; or C = 1, .A = the error
where:
            MOVR        r0, tl_arg
            LDR         r1, st
            jsr         STAT
            bcs         @done
            MOVR        r0, dst                             ; Where to: dst, or dst/its name
            LDR         r1, tpath
            jsr         put
            bcs         @long
            lda         todir
            beq         :+
            LDR         tl_pp, tpath
            MOVR        r0, tl_arg
            jsr         tl_base
            jsr         tl_pcat
            bcs         @done
:
            MOVR        r0, tl_arg
            LDR         r1, sd
            jsr         dirstat
            bcs         @done
            LDR         r0, tpath
            LDR         r1, td
            jmp         dirstat

@long:
            lda         #E_NAMETOOLONG
@done:
            rts

; The stat record of the directory the name at r0 is in, into r1's buffer.  OUT: C = 0; or C = 1, .A
dirstat:
            phx
            lda         r1
            pha
            lda         r1 + 1
            pha
            LDR         r1, dpath
            jsr         put
            pla
            sta         r1 + 1
            pla
            sta         r1
            plx
            bcs         @long
            ldy         #0                                  ; Its last /
            ldx         #$FF
:
            lda         dpath,Y
            beq         :++
            cmp         #'/'
            bne         :+
            tya
            tax
:
            iny
            bra         :--
:
            cpx         #$FF
            bne         :+
            lda         #'.'                                ; (None: .)
            sta         dpath
            ldx         #1
            bra         @cut

:
            cpx         #0                                  ; (The first: /)
            bne         @cut
            inx
@cut:
            stz         dpath,X
            LDR         r0, dpath
            jmp         STAT

@long:
            lda         #E_NAMETOOLONG
            rts

; The records at r0 and r1 for the same file (their device, instance, qid)?  OUT: Z = 1 if so
same:
            ldx         #0
:
            ldy         fields,X
            lda         (r0),Y
            cmp         (r1),Y
            bne         @done
            inx
            cpx         #FIELDS_N
            bne         :-
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

; The file at tl_arg copied to tpath.  OUT: C = 0; or C = 1, .A (0: an error about to, said here)
copy:
            MOVR        r0, tl_arg
            lda         #O_READ
            jsr         OPEN
            bcs         @done
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
            pla
            bra         @tsay

@rfail:
            pha
            jsr         @close
            pla
            sec
@done:
            rts

@tfail:
            pha
            lda         fdin
            jsr         CLOSE
            pla
@tsay:
            pha
            LDR         r0, tpath
            pla
            jsr         tl_err
            lda         #0                                  ; (Said: from stays)
            sec
            rts

@close:
            lda         fdout
            jsr         CLOSE
            lda         fdin
            jsr         CLOSE
            clc
            rts

.rodata
fields:     .byte       SR_DEV, SR_INST, SR_QPATH, SR_QPATH + 1, SR_QPATH + 2, SR_QPATH + 3
FIELDS_N    = * - fields
tl_name:    .byte       "mv", 0
tl_flagset: .byte       0
tl_usage:   .byte       "mv from to, or mv from ... dir", 0

.include "toollib.s"
