; ****************************************************************************
; tee [-a] [file ...] - fd 0 copied to fd 1 and to each file (made, or emptied: CREATE; -a: added to its end), 512
; bytes at a time.  A file that can't be opened or written is said ("tee: name: why"), and tee goes on without it,
; to end with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "tee", main

F_A             = $01           ; -a
FILES           = 8             ; Files at most

.zeropage
names:      .res        2 * FILES                           ; Each file's name ...

.bss
fds:        .res        FILES                               ;   and its fd ($FF: none)
nfiles:     .res        1
count:      .res        2
buf:        .res        512

.code
main:
            jsr         tl_start
            stz         nfiles
@arg:
            lda         (tl_arg)                            ; Each file opened
            beq         @copy
            ldx         nfiles
            cpx         #FILES
            bcs         @copy
            txa
            asl
            tax
            lda         tl_arg
            sta         names,X
            lda         tl_arg + 1
            sta         names + 1,X
            jsr         open
            ldx         nfiles
            sta         fds,X
            inc         nfiles
            jsr         tl_next
            bra         @arg

@copy:
            LDR         r0, buf                             ; fd 0, to its end
            LDR         r1, 512
            lda         #0
            jsr         READ
            bcs         @end
            sta         count
            stx         count + 1
            ora         count + 1
            beq         @end
            LDR         r0, buf                             ; To fd 1 ...
            MOVR        r1, count
            lda         #1
            jsr         WRITE
            bcc         :+
            pha
            LDR         r0, s_out
            pla
            jsr         tl_err
            jmp         tl_end

:
            ldx         #0                                  ;   and each file still open
@file:
            cpx         nfiles
            bcs         @copy
            lda         fds,X
            bmi         @next
            phx
            pha
            LDR         r0, buf
            MOVR        r1, count
            pla
            jsr         WRITE
            plx
            bcc         @next
            jsr         lost
@next:
            inx
            bra         @file

@end:
            ldx         #0
:
            cpx         nfiles
            bcs         :+
            lda         fds,X
            bmi         @shut
            phx
            jsr         CLOSE
            plx
@shut:
            inx
            bra         :-
:
            jmp         tl_end

; The file at tl_arg opened: made or emptied, or (-a) at its end.  OUT: .A = its fd, or $FF (said)
open:
            lda         tl_flags
            and         #F_A
            beq         @create
            MOVR        r0, tl_arg                          ; -a: there already, at its end
            lda         #O_WRITE
            jsr         OPEN
            bcs         @create
            pha
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #2
            jsr         SEEK
            pla
            rts

@create:
            MOVR        r0, tl_arg
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcc         @done
            pha
            MOVR        r0, tl_arg
            pla
            jsr         tl_err
            lda         #$FF
@done:
            rts

; File .X can't be written: said, closed, and no more written to it.  Keeps .X
lost:
            phx
            pha
            txa
            asl
            tax
            lda         names,X
            sta         r0
            lda         names + 1,X
            sta         r0 + 1
            pla
            jsr         tl_err
            plx
            lda         fds,X
            phx
            jsr         CLOSE
            plx
            lda         #$FF
            sta         fds,X
            rts

.rodata
s_out:      .byte       "fd 1", 0
tl_name:    .byte       "tee", 0
tl_flagset: .byte       "a", 0
tl_usage:   .byte       "tee [-a] [file ...]", 0

.include "toollib.s"
