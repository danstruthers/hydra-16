; ****************************************************************************
; ffile.inc - forth's files: including them (INCLUDED, INCLUDE, REQUIRED, REQUIRE) and loading a library (included by
; forth.s; the other File Access words are a library, forthlib/file.s).  A fileid is the system's fd (0, 1 and 2
; stdin, stdout and stderr); an ior is 0, or -512 less the system's error code (THROW shows its text).  A
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
            ldx         xsave                               ; (An error; Ctrl-C's: THROW -28)
            cmp         #E_INTR
            bne         @error
            jmp         intr_throw
@error:
            sec
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
            lda         src_line                            ; Its first line #!...: none (a script's: #!/bin/forth)
            cmp         #1
            bne         @line
            lda         src_line + 1
            ora         src_len + 1
            bne         @line
            lda         src_len
            cmp         #2
            bcc         @line
            lda         (w2)
            cmp         #'#'
            bne         @line
            ldy         #1
            lda         (w2),y
            cmp         #'!'
            bne         @line
            stz         src_len
@line:
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
            lda         #<pathbuf
            sta         w2
            lda         #>pathbuf
            sta         w2 + 1
; ( c-addr u -- ): the string into (w2), zero-terminated (PATH_SIZE - 1 chars at most)
to_z:
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
            sta         (w2),y
@copy:
            dey
            bmi         @done
            lda         (w),y
            sta         (w2),y
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

; ---- Opening

; OPEN-FILE's ( c-addr u fam -- fileid ior ) (the File Access library's header: file.fl), INCLUDED's
openfile:
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

; ---- Including files

; INCLUDE-FILE's (the File Access library's header: file.fl)
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
            jsr         twodup
            jsr         open_ro
            lda         dlo,x
            ora         dhi,x
            beq         @found
            lda         dlo,x                               ; Not there, and a bare name (no /): /lib/forth's
            cmp         #<(-512 - E_NOENT)
            bne         @fail
            lda         dhi,x
            cmp         #>(-512 - E_NOENT)
            bne         @fail
            inx
            inx
            jsr         lib_name
            bcs         @none
            jsr         open_ro
            lda         dlo,x
            ora         dhi,x
            bne         @fail
            inx
            jmp         inc_or_load
@none:
            dex                                             ; (Its ior again)
            lda         #<(-512 - E_NOENT)
            sta         dlo,x
            lda         #>(-512 - E_NOENT)
            sta         dhi,x
@fail:
            stz         inc_named
            lda         #1
            sta         throw_named
            jmp         throw
@found:
            inx                                             ; ( c-addr u fileid -- fileid )
            lda         dlo,x
            sta         dlo + 2,x
            lda         dhi,x
            sta         dhi + 2,x
            inx
            inx
            jmp         inc_or_load

; INCLUDED's file ( fileid ): a library (it starts "HYFL": a word set's, made at the build) loaded into the
; dictionary; or else included, from its start
inc_or_load:
            lda         dlo,x
            sta         rl_fd
            LDR         r0, numtmp
            LDR         r1, 4
            lda         rl_fd
            stx         xsave
            jsr         READ
            ldx         xsave
            bcs         @source
            cmp         #4
            bne         @source
            ldy         #3
:
            lda         numtmp,y
            cmp         s_hyfl,y
            bne         @source
            dey
            bpl         :-
            jmp         lib_load
@source:
            stz         r0                                  ; (Back to its start)
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            lda         rl_fd
            stx         xsave
            ldx         #0
            jsr         SEEK
            ldx         xsave
            jmp         includefile

s_hyfl:     .byte       "HYFL"

LH_VERSION      = 0             ; A library file's header, after its magic (tools/forthlib.js): its form's version ...
LH_CORE         = 1             ;   the core's id (2) ...
LH_IMAGE        = 3             ;   its image's length (2) ...
LH_BSS          = 5             ;   its BSS's (2) ...
LH_FIRST        = 7             ;   its first header's link, an offset ($FFFF: none) ...
LH_LAST         = 9             ;   its last header ...
LH_INIT         = 11            ;   its init ($FFFF: none) ...
LH_RELOCS       = 13            ;   its relocations' count
LH_SIZE         = 15

; lib_load's ends: no room (THROW -8), another core's (THROW -259), the file closed first
lib_full:
            jsr         lib_close
            lda         #<-8
            jmp         throw_a
lib_bad:
            jsr         lib_close
            lda         #<-259
            sta         tmp
            lda         #>-259
            sta         tmp + 1
            jmp         throw_tmp
lib_close:
            lda         rl_fd
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            rts

; A library from fileid ( fileid -- ), its magic read (tools/forthlib.js has its form): its image read to HERE, its
; BSS zeros after it, its relocations (read after that) applied (HERE added to each), its headers linked into the
; compilation word list, HERE past it, the file closed, and its init run.  Another core's: THROW -259
lib_load:
            lda         dlo,x
            sta         rl_fd
            inx
            LDR         r0, statbuf                         ; Its header
            LDR         r1, LH_SIZE
            jsr         lib_read
            cmp         #LH_SIZE
            bne         lib_bad
            lda         statbuf + LH_VERSION
            cmp         #1
            bne         lib_bad
            lda         statbuf + LH_CORE
            cmp         core_id
            bne         lib_bad
            lda         statbuf + LH_CORE + 1
            cmp         core_id + 1
            bne         lib_bad
            clc                                             ; w: its image's end; w2: its BSS's (no further than
            lda         here                                ;   the dictionary's end)
            adc         statbuf + LH_IMAGE
            sta         w
            lda         here + 1
            adc         statbuf + LH_IMAGE + 1
            sta         w + 1
            clc
            lda         w
            adc         statbuf + LH_BSS
            sta         w2
            lda         w + 1
            adc         statbuf + LH_BSS + 1
            sta         w2 + 1
            bcs         lib_full
            cmp         #>DICT_END
            bcs         lib_full
            lda         here                                ; Its image
            sta         r0
            lda         here + 1
            sta         r0 + 1
            lda         statbuf + LH_IMAGE
            sta         r1
            lda         statbuf + LH_IMAGE + 1
            sta         r1 + 1
            jsr         lib_read
            cmp         statbuf + LH_IMAGE
            bne         :+
            cpy         statbuf + LH_IMAGE + 1
            beq         :++
:
            jmp         lib_bad
:
            lda         w2                                  ; Its relocations, after its BSS (what's left of the file)
            sta         r0
            lda         w2 + 1
            sta         r0 + 1
            sec
            lda         #<DICT_END
            sbc         w2
            sta         r1
            lda         #>DICT_END
            sbc         w2 + 1
            sta         r1 + 1
            jsr         lib_read
            jsr         lib_close
            lda         w                                   ; Its BSS: zeros
            sta         w3
            lda         w + 1
            sta         w3 + 1
:
            lda         w3
            cmp         w2
            lda         w3 + 1
            sbc         w2 + 1
            bcs         :+
            lda         #0
            sta         (w3)
            inc         w3
            bne         :-
            inc         w3 + 1
            bra         :-
:
            lda         w2                                  ; Its relocations (p1, tmp2 of them)
            sta         p1
            lda         w2 + 1
            sta         p1 + 1
            lda         statbuf + LH_RELOCS
            sta         tmp2
            lda         statbuf + LH_RELOCS + 1
            sta         tmp2 + 1
@reloc:
            lda         tmp2
            ora         tmp2 + 1
            beq         @link
            ldy         #1
            lda         (p1),y                              ; (p2: where, here + its offset; .Y: its kind)
            and         #$3F
            clc
            adc         here + 1
            sta         p2 + 1
            lda         (p1)
            clc
            adc         here
            sta         p2
            bcc         :+
            inc         p2 + 1
:
            lda         (p1),y
            rol
            rol
            rol
            and         #3
            tay
            beq         @word
            dey
            beq         @low
            ldy         #2                                  ; A high byte: its address's low byte (after it) +
            lda         (p1),y                              ;   HERE's, for the carry
            clc
            adc         here
            lda         (p2)
            adc         here + 1
            sta         (p2)
            inc         p1                                  ; (Its 3rd byte)
            bne         @next
            inc         p1 + 1
            bra         @next
@word:
            clc
            lda         (p2)
            adc         here
            sta         (p2)
            ldy         #1
            lda         (p2),y
            adc         here + 1
            sta         (p2),y
            bra         @next
@low:
            clc
            lda         (p2)
            adc         here
            sta         (p2)
@next:
            clc
            lda         p1
            adc         #2
            sta         p1
            bcc         :+
            inc         p1 + 1
:
            lda         tmp2
            bne         :+
            dec         tmp2 + 1
:
            dec         tmp2
            bra         @reloc
@link:
            lda         statbuf + LH_FIRST + 1              ; Its headers, in the compilation word list
            cmp         #$FF
            beq         @here
            clc                                             ; (Its first header's link: the word list's last)
            lda         here
            adc         statbuf + LH_FIRST
            sta         p2
            lda         here + 1
            adc         statbuf + LH_FIRST + 1
            sta         p2 + 1
            jsr         current_w3
            lda         (w3)
            sta         (p2)
            ldy         #1
            lda         (w3),y
            sta         (p2),y
            clc                                             ; (The word list's last: its last header)
            lda         here
            adc         statbuf + LH_LAST
            sta         (w3)
            lda         here + 1
            adc         statbuf + LH_LAST + 1
            sta         (w3),y
@here:
            clc                                             ; Its init's address (before HERE moves on)
            lda         here
            adc         statbuf + LH_INIT
            sta         w
            lda         here + 1
            adc         statbuf + LH_INIT + 1
            sta         w + 1
            lda         w2                                  ; HERE past it
            sta         here
            lda         w2 + 1
            sta         here + 1
            lda         statbuf + LH_INIT + 1               ; Its init
            cmp         #$FF
            beq         :+
            jmp         exec_w
:
            rts
; READ rl_fd into r0, r1 bytes.  OUT: .A/.Y the count (0 after an error).  Keeps .X
lib_read:
            lda         rl_fd
            stx         xsave
            jsr         READ
            bcc         :+
            lda         #0
            tax
:
            phx
            ply
            ldx         xsave
            rts

; ( c-addr u -- fileid ior ): OPEN-FILE, read only
open_ro:
            dex
            lda         #O_READ
            sta         dlo,x
            stz         dhi,x
            jmp         openfile

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

; ( c-addr u -- c-addr' u' ): a bare name (no /) as /lib/forth's, in pathbuf (INCLUDED's).  OUT: C = 1, the name left
; as it was, if it isn't bare (or is too long)
lib_name:
            lda         dhi,x
            bne         @not
            lda         dlo,x
            beq         @not
            cmp         #PATH_SIZE - LIB_LEN - 1
            bcs         @not
            sta         cnt
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0                                  ; (A / in it: not bare)
:
            lda         (w),y
            cmp         #'/'
            beq         @not
            iny
            cpy         cnt
            bne         :-
            ldy         #LIB_LEN - 1                        ; /lib/forth/, then it
:
            lda         s_lib,y
            sta         pathbuf,y
            dey
            bpl         :-
            ldy         #0
:
            lda         (w),y
            sta         pathbuf + LIB_LEN,y
            iny
            cpy         cnt
            bne         :-
            clc
            lda         dlo,x
            adc         #LIB_LEN
            sta         dlo,x
            lda         #<pathbuf
            sta         dlo + 1,x
            lda         #>pathbuf
            sta         dhi + 1,x
            clc
            rts
@not:
            sec
            rts

s_lib:      .byte       "/lib/forth/"
LIB_LEN     = * - s_lib
