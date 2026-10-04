; ****************************************************************************
; ffile.inc - forth's File Access words, and including files (included by forth.s).  A fileid is the system's fd (0,
; 1 and 2 stdin, stdout and stderr); an ior is 0, or -512 less the system's error code (THROW shows its text).  A
; file being included is read a line at a time into its own buffer (lbufs: as deep as it's nested), the way READ-LINE
; reads one: a line and 2 bytes read, and the fd's offset moved back to the line's end, so the file's offset is
; always where its next line starts (SAVE-INPUT keeps a line's place; RESTORE-INPUT seeks there and reads it again).

; ---- Lines

; A line from fd .A into (w2), tmp2 bytes at most (a longer one is cut there: the rest is the next): tmp2 + 2 bytes
; read, and the fd's offset moved back to the line's end.  OUT: C = 0, .A/.Y = its length, tmp3 = the bytes it took
; (its end too: LF, CR LF or CR); or C = 1, .A = 0 at the file's end, or the system's error.  Keeps .X
read_line_fd:
            sta         rl_fd
            stx         xsave
            lda         w2
            sta         r0
            lda         w2 + 1
            sta         r0 + 1
            clc
            lda         tmp2
            adc         #2
            sta         r1
            lda         tmp2 + 1
            adc         #0
            sta         r1 + 1
            lda         rl_fd
            jsr         READ
            bcc         :+
            ldx         xsave                               ; (An error)
            rts
:
            sta         rl_n
            stx         rl_n + 1
            ldx         xsave
            ora         rl_n + 1
            bne         :+
            sec                                             ; (Nothing: the end)
            rts
:
            stz         tmp3                                ; (tmp3: the char looked at)
            stz         tmp3 + 1
@scan:
            lda         tmp3                                ; All of it, and no end: the file's last line
            cmp         rl_n
            lda         tmp3 + 1
            sbc         rl_n + 1
            bcs         @cut
            clc
            lda         w2
            adc         tmp3
            sta         w3
            lda         w2 + 1
            adc         tmp3 + 1
            sta         w3 + 1
            lda         (w3)
            cmp         #LF
            beq         @lf
            cmp         #$0D
            beq         @cr
            lda         tmp3                                ; tmp2 chars and no end: cut there
            cmp         tmp2
            bne         :+
            lda         tmp3 + 1
            cmp         tmp2 + 1
            beq         @cut
:
            inc         tmp3
            bne         @scan
            inc         tmp3 + 1
            bra         @scan

@cut:
            lda         tmp3                                ; The line: tmp3 chars, all it took
            sta         rl_len
            lda         tmp3 + 1
            sta         rl_len + 1
            bra         @back

@cr:
            jsr         @len                                ; A CR: an LF after it, the line's too
            lda         tmp3
            cmp         rl_n
            lda         tmp3 + 1
            sbc         rl_n + 1
            bcs         @back
            ldy         #1
            lda         (w3),y
            cmp         #LF
            bne         @back
            jsr         @took
            bra         @back

@lf:
            jsr         @len

@back:
            sec                                             ; The rest read: the offset back before it
            lda         rl_n
            sbc         tmp3
            sta         r0
            lda         rl_n + 1
            sbc         tmp3 + 1
            sta         r0 + 1
            ora         r0
            beq         @done
            sec
            lda         #0
            sbc         r0
            sta         r0
            lda         #0
            sbc         r0 + 1
            sta         r0 + 1
            lda         #$FF
            sta         r1
            sta         r1 + 1
            stx         xsave
            lda         rl_fd
            ldx         #1
            jsr         SEEK
            ldx         xsave
@done:
            lda         rl_len
            ldy         rl_len + 1
            clc
            rts

@len:                                                       ; rl_len = tmp3 (the end's place), and tmp3 past it
            lda         tmp3
            sta         rl_len
            lda         tmp3 + 1
            sta         rl_len + 1
@took:
            inc         tmp3
            bne         :+
            inc         tmp3 + 1
:
            rts

; REFILL for the file being included: its next line in its buffer (SOURCE), >IN 0, the line's place and number.
; OUT: C = 1 at its end (or an error)
refill_file:
            clc                                             ; Its place: past the line before
            lda         src_pos
            adc         src_cons
            sta         src_pos
            lda         src_pos + 1
            adc         src_cons + 1
            sta         src_pos + 1
            bcc         :+
            inc         src_pos + 2
            bne         :+
            inc         src_pos + 3
:
            stz         src_cons
            stz         src_cons + 1
            lda         src_fdep
            jsr         lbuf_w2
            LDR         tmp2, LINE_MAX
            lda         src_id
            jsr         read_line_fd
            bcs         @done
            sta         src_len
            sty         src_len + 1
            lda         w2
            sta         src_addr
            lda         w2 + 1
            sta         src_addr + 1
            lda         tmp3
            sta         src_cons
            lda         tmp3 + 1
            sta         src_cons + 1
            stz         to_in
            stz         to_in + 1
            inc         src_line
            bne         :+
            inc         src_line + 1
:
            clc
@done:
            rts

; w2 = the line buffer of the file being included at depth .A (1 ...)
lbuf_w2:
            tay
            lda         #<lbufs
            sta         w2
            lda         #>lbufs
            sta         w2 + 1
@next:
            dey
            beq         @done
            clc
            lda         w2
            adc         #<LINE_BUF
            sta         w2
            lda         w2 + 1
            adc         #>LINE_BUF
            sta         w2 + 1
            bra         @next
@done:
            rts

; ---- Names and results

; ( c-addr u -- ): the name into pathbuf, zero-terminated (PATH_SIZE - 1 chars at most)
to_path:
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #PATH_SIZE - 1                      ; (Its length: that at most)
            lda         dhi,x
            bne         :+
            lda         dlo,x
            cmp         #PATH_SIZE
            bcs         :+
            tay
:
            lda         #0
            sta         pathbuf,y
@copy:
            dey
            bmi         @done
            lda         (w),y
            sta         pathbuf,y
            bra         @copy
@done:
            inx
            inx
            rts

; Push OPEN's or CREATE's fileid and ior (C, .A)
fid_ior:
            bcc         :+
            pha
            dex
            jsr         zero_tos
            pla
            sec
            bra         push_ior
:
            ldy         #0
            PUSHAY
            clc
; Push a system call's ior: C = 0, 0; or C = 1 (.A the error), -512 less it
push_ior:
            bcs         :+
            dex
            jmp         zero_tos
:
            eor         #$FF
            inc
            ldy         #$FD
            PUSHAY
            rts

; ( x -- 0 0 ior ): a failed call's (.A the error), its double 0
ud_err:
            pha
            jsr         zero_tos
            dex
            jsr         zero_tos
            pla
            sec
            bra         push_ior

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

            HEADER      "R/O", 0
ro:
            lda         #O_READ
fam:
            ldy         #0
            PUSHAY
            rts

            HEADER      "W/O", 0
wo:
            lda         #O_WRITE
            bra         fam

            HEADER      "R/W", 0
rw:
            lda         #O_RDWR
            bra         fam

            HEADER      "BIN", 0
bin:
            rts

            HEADER      "OPEN-FILE", 0
openfile:                                                   ; ( c-addr u fam -- fileid ior )
            lda         dlo,x
            pha
            inx
            jsr         to_path
            LDR         r0, pathbuf
            pla
            stx         xsave
            jsr         OPEN
            ldx         xsave
            jmp         fid_ior

            HEADER      "CREATE-FILE", 0
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

            HEADER      "CLOSE-FILE", 0
closefile:                                                  ; ( fileid -- ior )
            lda         dlo,x
            inx
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            jmp         push_ior

            HEADER      "DELETE-FILE", 0
deletefile:                                                 ; ( c-addr u -- ior )
            jsr         to_path
            LDR         r0, pathbuf
            stx         xsave
            jsr         REMOVE
            ldx         xsave
            jmp         push_ior

            HEADER      "READ-FILE", 0
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

            HEADER      "READ-LINE", 0
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

            HEADER      "WRITE-FILE", 0
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

            HEADER      "WRITE-LINE", 0
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

            HEADER      "FILE-POSITION", 0
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

            HEADER      "REPOSITION-FILE", 0
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

            HEADER      "FILE-SIZE", 0
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

            HEADER      "RESIZE-FILE", 0
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

            HEADER      "RENAME-FILE", 0
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

            HEADER      "FILE-STATUS", 0
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

            HEADER      "FLUSH-FILE", 0
flushfile:                                                  ; ( fileid -- ior ): nothing's kept but TYPE's
            jsr         flush
            jmp         zero_tos

; ---- Including files

            HEADER      "INCLUDE-FILE", 0
includefile:                                                ; ( i*x fileid -- j*x ): its lines, from its offset, as
            lda         src_fdep                            ;   the source; closed at its end
            cmp         #INC_MAX
            bcc         :+
            jmp         throw_deep
:
            jsr         src_push
            inc         src_fdep
            lda         inc_named                           ; (Its name: INCLUDED's, or none)
            bne         :+
            lda         src_fdep
            jsr         lname_w
            lda         #0
            sta         (w)
:
            stz         inc_named
            lda         dlo,x
            sta         src_id
            stz         src_id + 1
            inx
            lda         #1
            sta         src_close
            stz         src_cons
            stz         src_cons + 1
            stz         src_line
            stz         src_line + 1
            stz         src_len
            stz         src_len + 1
            stz         to_in
            stz         to_in + 1
            stz         r0                                  ; Where it is: its first line's place
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            stx         xsave
            lda         src_id
            ldx         #1
            jsr         SEEK
            ldx         xsave
            bcc         :+
            stz         r0
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
:
            ldy         #3
:
            lda         r0,y
            sta         src_pos,y
            dey
            bpl         :-
@line:
            jsr         refill_file
            bcs         @end
            jsr         interpret
            bra         @line
@end:
            jmp         src_pop

            HEADER      "INCLUDED", 0
included:                                                   ; ( i*x c-addr u -- j*x ): the file named, opened and
            jsr         inc_find                            ;   included (and noted, for REQUIRED)
            bcc         :+
            jsr         inc_add
:
            lda         src_fdep                            ; Its name, for an error's place (the last 31 chars)
            cmp         #INC_MAX
            bcs         @open
            inc
            jsr         lname_w
            ldy         #LNAME_SIZE - 1
            lda         dhi,x
            bne         :+
            lda         dlo,x
            cmp         #LNAME_SIZE
            bcs         :+
            tay
:
            tya
            sta         (w)
            clc                                             ; (w3: its first char kept)
            lda         dlo + 1,x
            adc         dlo,x
            sta         w3
            lda         dhi + 1,x
            adc         dhi,x
            sta         w3 + 1
            sec
            lda         w3
            sbc         (w)
            sta         w3
            bcs         :+
            dec         w3 + 1
:
            cpy         #0
            beq         :+
            dey
            lda         (w3),y
            iny
            sta         (w),y
            dey
            bra         :-
:
            lda         #1
            sta         inc_named
@open:
            lda         dlo + 1,x                           ; (Its name for an error opening it)
            sta         throw_name
            lda         dhi + 1,x
            sta         throw_name + 1
            lda         dlo,x
            sta         throw_nlen
            dex
            lda         #O_READ
            sta         dlo,x
            stz         dhi,x
            jsr         openfile
            lda         dlo,x
            ora         dhi,x
            beq         :+
            stz         inc_named
            lda         #1
            sta         throw_named
            jmp         throw
:
            inx
            jmp         includefile

            HEADER      "REQUIRED", 0
required:                                                   ; ( i*x c-addr u -- j*x ): INCLUDED, unless it has been
            jsr         inc_find
            bcc         :+
            jmp         included
:
            inx
            inx
            rts

            HEADER      "INCLUDE", 0
include:                                                    ; ( i*x "name" -- j*x )
            jsr         parse_name
            jmp         included

            HEADER      "REQUIRE", 0
require:                                                    ; ( i*x "name" -- j*x )
            jsr         parse_name
            bra         required

; Is the name ( c-addr u, kept) one of the files INCLUDED (incn)?  OUT: C = 0 yes; or C = 1, w3 = incn's end
inc_find:
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            LDR         w3, incn
@rec:
            sec                                             ; (Past the names: not there)
            lda         w3
            sbc         #<incn
            sta         tmp3
            lda         w3 + 1
            sbc         #>incn
            sta         tmp3 + 1
            lda         tmp3
            cmp         incn_len
            lda         tmp3 + 1
            sbc         incn_len + 1
            bcs         @no
            lda         dhi,x
            bne         @next
            lda         (w3)
            cmp         dlo,x
            bne         @next
            tay
@cmp:
            cpy         #0
            beq         @yes
            lda         (w3),y
            dey
            cmp         (w),y
            bne         @next
            bra         @cmp
@yes:
            clc
            rts
@next:
            sec
            lda         (w3)
            adc         w3
            sta         w3
            bcc         @rec
            inc         w3 + 1
            bra         @rec
@no:
            rts

; The name ( c-addr u, kept) added to incn at w3, its end (inc_find's), if there's room
inc_add:
            lda         dhi,x
            bne         @full
            sec                                             ; (incn_len + u + 1, no more than its size)
            lda         incn_len
            adc         dlo,x
            sta         tmp3
            lda         incn_len + 1
            adc         #0
            sta         tmp3 + 1
            lda         #<INCN_SIZE
            cmp         tmp3
            lda         #>INCN_SIZE
            sbc         tmp3 + 1
            bcc         @full
            lda         tmp3
            sta         incn_len
            lda         tmp3 + 1
            sta         incn_len + 1
            lda         dlo,x
            sta         (w3)
            tay
@copy:
            beq         @full
            dey
            lda         (w),y
            iny
            sta         (w3),y
            dey
            bra         @copy
@full:
            rts
