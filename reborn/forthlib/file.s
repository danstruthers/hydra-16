; ****************************************************************************
; file.s - HyForth's File Access library (/lib/forth/file.fl): files opened, made, read and written, their places,
; sizes, names and states.  A fileid is the system's fd; an ior is 0, or -512 less the system's error code.
; INCLUDE-FILE, INCLUDED, REQUIRE and the rest that load files are the core's (OPEN-FILE's and INCLUDE-FILE's code
; too: INCLUDED's).

.include "forthlib.inc"

; ( x -- 0 0 ior ): a failed call's (.A the error), its double 0
ud_err:
            pha
            jsr         zero_tos
            dex
            jsr         zero_tos
            pla
            sec
            jmp         push_ior

; statbuf: a record that changes nothing (Plan 9's: its name empty, the rest $FF).  Keeps .X
stat_none:
            ldy         #SR_SIZE - 1
            lda         #$FF
:
            sta         statbuf,y
            dey
            bpl         :-
            stz         statbuf + SR_NAME
            rts

; ---- The words

            HEADER      "r/o", 0
ro:
            CONSTCODE   O_READ

            HEADER      "w/o", 0
wo:
            CONSTCODE   O_WRITE

            HEADER      "r/w", 0
rw:
            CONSTCODE   O_RDWR

            HEADER      "bin", 0
bin:
            rts

            HEADER      "open-file", 0
openfile_w:                                                 ; ( c-addr u fam -- fileid ior )
            jmp         openfile

            HEADER      "create-file", 0
createfile:                                                 ; ( c-addr u fam -- fileid ior ): one that's there is
            lda         dlo,x                               ;   emptied
            pha
            inx
            jsr         to_path
            LDR         r0, pathbuf
            pla
            stx         xsave
            ldx         #0
            jsr         CREATE
            ldx         xsave
            jmp         fid_ior

            HEADER      "close-file", 0
closefile:                                                  ; ( fileid -- ior )
            lda         dlo,x
            inx
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            jmp         push_ior

            HEADER      "delete-file", 0
deletefile:                                                 ; ( c-addr u -- ior )
            jsr         to_path
            LDR         r0, pathbuf
            stx         xsave
            jsr         REMOVE
            ldx         xsave
            jmp         push_ior

            HEADER      "read-file", 0
readfile:                                                   ; ( c-addr u1 fileid -- u2 ior ): u2 < u1 at the end
            lda         dlo,x                               ;   (stdin's read ahead first: the interpreter's)
            bne         @read
            lda         ipos
            cmp         ilen
            lda         ipos + 1
            sbc         ilen + 1
            bcs         @read
            lda         dlo + 2,x
            sta         w2
            lda         dhi + 2,x
            sta         w2 + 1
            lda         dlo + 1,x
            sta         tmp2
            lda         dhi + 1,x
            sta         tmp2 + 1
            inx
            inx
            stz         tmp3
            stz         tmp3 + 1
@byte:
            lda         tmp3                                ; (As many as asked for, or as there are)
            cmp         tmp2
            lda         tmp3 + 1
            sbc         tmp2 + 1
            bcs         @got
            lda         ipos
            cmp         ilen
            lda         ipos + 1
            sbc         ilen + 1
            bcs         @got
            jsr         getc_in
            sta         (w2)
            inc         w2
            bne         :+
            inc         w2 + 1
:
            inc         tmp3
            bne         @byte
            inc         tmp3 + 1
            bra         @byte
@got:
            lda         tmp3
            sta         dlo,x
            lda         tmp3 + 1
            sta         dhi,x
            clc
            jmp         push_ior

@read:
            sta         rl_fd
            lda         dlo + 1,x
            sta         r1
            lda         dhi + 1,x
            sta         r1 + 1
            lda         dlo + 2,x
            sta         r0
            lda         dhi + 2,x
            sta         r0 + 1
            inx
            inx
            lda         rl_fd
            stx         xsave
            jsr         READ
            bcs         @error
            sta         tmp
            stx         tmp + 1
            ldx         xsave
            lda         tmp
            sta         dlo,x
            lda         tmp + 1
            sta         dhi,x
            clc
            jmp         push_ior
@error:
            ldx         xsave
            pha
            jsr         zero_tos
            pla
            sec
            jmp         push_ior

            HEADER      "read-line", 0
readline:                                                   ; ( c-addr u1 fileid -- u2 flag ior ): the buffer u1 + 2
            lda         dlo + 2,x                           ;   long; flag false at the end
            sta         w2
            lda         dhi + 2,x
            sta         w2 + 1
            lda         dlo + 1,x
            sta         tmp2
            lda         dhi + 1,x
            sta         tmp2 + 1
            lda         dlo,x
            inx
            cmp         #0
            bne         @file
            jsr         read_line                           ; (stdin's: as the interpreter reads it)
            bcc         @line
            lda         #0
            bra         @end
@file:
            jsr         read_line_fd
            bcs         @end
@line:
            sta         dlo + 1,x
            sty         dhi + 1,x
            jsr         true_tos
            clc
            jmp         push_ior
@end:
            pha                                             ; (The end, or an error: 0 false ior)
            jsr         zero_tos
            stz         dlo + 1,x
            stz         dhi + 1,x
            pla
            cmp         #1                                  ; (C = 1 for an error)
            jmp         push_ior

            HEADER      "write-file", 0
writefile:                                                  ; ( c-addr u fileid -- ior )
            jsr         flush                               ; (TYPE's first, for stdout's order)
            lda         dlo,x
            sta         rl_fd
            lda         dlo + 1,x
            sta         r1
            lda         dhi + 1,x
            sta         r1 + 1
            lda         dlo + 2,x
            sta         r0
            lda         dhi + 2,x
            sta         r0 + 1
            inx
            inx
            inx
            lda         rl_fd
write_fd:
            stx         xsave
            jsr         WRITE
            ldx         xsave
            jmp         push_ior

            HEADER      "write-line", 0
writeline:                                                  ; ( c-addr u fileid -- ior ): and an LF
            lda         dlo,x
            pha
            jsr         writefile
            pla
            ldy         dlo,x
            bne         :+
            ldy         dhi,x
            bne         :+
            inx
            pha
            lda         #LF
            sta         numtmp
            LDR         r0, numtmp
            LDR         r1, 1
            pla
            bra         write_fd
:
            rts

            HEADER      "file-position", 0
fileposition:                                               ; ( fileid -- ud ior )
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         dlo,x
            stx         xsave
            ldx         #1
            jsr         SEEK
            ldx         xsave
            bcc         :+
            jmp         ud_err
:
            lda         r0
            sta         dlo,x
            lda         r0 + 1
            sta         dhi,x
            lda         r1
            ldy         r1 + 1
            PUSHAY
            clc
            jmp         push_ior

            HEADER      "reposition-file", 0
repositionfile:                                             ; ( ud fileid -- ior )
            lda         dlo + 2,x
            sta         r0
            lda         dhi + 2,x
            sta         r0 + 1
            lda         dlo + 1,x
            sta         r1
            lda         dhi + 1,x
            sta         r1 + 1
            lda         dlo,x
            inx
            inx
            inx
            stx         xsave
            ldx         #0
            jsr         SEEK
            ldx         xsave
            jmp         push_ior

            HEADER      "file-size", 0
filesize:                                                   ; ( fileid -- ud ior )
            LDR         r0, statbuf
            lda         dlo,x
            stx         xsave
            jsr         FSTAT
            ldx         xsave
            bcc         :+
            jmp         ud_err
:
            lda         statbuf + SR_LENGTH
            sta         dlo,x
            lda         statbuf + SR_LENGTH + 1
            sta         dhi,x
            lda         statbuf + SR_LENGTH + 2
            ldy         statbuf + SR_LENGTH + 3
            PUSHAY
            clc
            jmp         push_ior

            HEADER      "resize-file", 0
resizefile:                                                 ; ( ud fileid -- ior ): longer with zeros, or cut short
            jsr         stat_none                           ;   (a WSTAT of its length)
            lda         dlo + 2,x
            sta         statbuf + SR_LENGTH
            lda         dhi + 2,x
            sta         statbuf + SR_LENGTH + 1
            lda         dlo + 1,x
            sta         statbuf + SR_LENGTH + 2
            lda         dhi + 1,x
            sta         statbuf + SR_LENGTH + 3
            LDR         r0, statbuf
            lda         dlo,x
            inx
            inx
            inx
            stx         xsave
            jsr         FWSTAT
            ldx         xsave
            jmp         push_ior

            HEADER      "rename-file", 0
renamefile:                                                 ; ( c-addr1 u1 c-addr2 u2 -- ior ): in its directory, the
            jsr         stat_none                           ;   new name's last part its name
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            ldy         #0                                  ; (cnt: past its last /)
            stz         cnt
@scan:
            cpy         tmp
            beq         @copy
            lda         (w),y
            iny
            cmp         #'/'
            bne         @scan
            sty         cnt
            bra         @scan
@copy:
            ldy         cnt
            phx
            ldx         #0
:
            cpy         tmp
            beq         :+
            cpx         #LNAME_SIZE - 1
            beq         :+
            lda         (w),y
            sta         statbuf + SR_NAME,x
            iny
            inx
            bra         :-
:
            stz         statbuf + SR_NAME,x
            plx
            inx
            inx
            jsr         to_path
            LDR         r0, pathbuf
            LDR         r1, statbuf
            stx         xsave
            jsr         WSTAT
            ldx         xsave
            jmp         push_ior

            HEADER      "file-status", 0
filestatus:                                                 ; ( c-addr u -- x ior ): x its mode
            jsr         to_path
            LDR         r0, pathbuf
            LDR         r1, statbuf
            stx         xsave
            jsr         STAT
            ldx         xsave
            pha
            lda         statbuf + SR_MODE
            ldy         statbuf + SR_MODE + 1
            PUSHAY
            pla
            jmp         push_ior

            HEADER      "flush-file", 0
flushfile:                                                  ; ( fileid -- ior ): nothing's kept but TYPE's
            jsr         flush
            jmp         zero_tos

            HEADER      "include-file", 0
includefile_w:                                              ; ( i*x fileid -- j*x ): its lines, from its offset, as
            jmp         includefile                         ;   the source; closed at its end
