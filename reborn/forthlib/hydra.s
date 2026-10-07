; ****************************************************************************
; hydra.s - HyForth's Hydra library (/lib/forth/hydra.fl): a sys- word for each system call a program makes (their
; headers forthsys.inc's, tools/apigen.js's, from the specification), zero-terminated strings, sh and run, the bank and
; segment words (and code-banks), forth's arguments (argc, arg), ctl, hylang's Hydra built-ins as Forth names them
; (a directory's names: Gforth's open-dir read-dir close-dir, =mkdir, get-dir, set-dir; note, note-group, on-note;
; pause; ior>text), and sys (machine code, called with its registers).
;   A sys- word is a system call with its registers as stack items, in the specification's order (spec/api.def;
; /rom/doc/api.md has each one's), the first deepest: its inputs, then its outputs and, if the call can fail, an ior
; (0, or -512 less the error code; its outputs 0 then).  A register is a cell (rN, .A/.X: 16 bits; .A, .X, .Y: a
; byte), or a double (rN and the next: r0/r1).  A name is a zero-terminated string's address (>Z).

.include "forthlib.inc"

.bss
zbufs:      .res        PATH_SIZE * 2                       ; >Z's two buffers, in turn ...
zbuf_n:     .res        1                                   ;   the one last used
errbuf:     .res        32                                  ; IOR>TEXT's text (ERRSTR's)
sys_a:      .res        1                                   ; A sys- word's call: .A, .X and .Y, in and out ...
sys_x:      .res        1
sys_y:      .res        1
sys_p:      .res        1                                   ;   its flags (C: it failed) ...
sys_f:      .res        1                                   ;   its descriptor's flags ($80: an ior) ...
sys_to:     .res        2                                   ;   and its address
.assert     sys_y = sys_a + 2 .and sys_x = sys_a + 1, error, "sys_a, sys_x, sys_y: in that order (sys_pop, sys_push)"
.code

; ---- The sys- words

; A sys- word's: its code is jsr here, then its descriptor (forthsys.inc's entry, but its name): the call's address,
; flags ($80: an ior), its inputs (a count, then their registers, the top's first) and outputs (a count, then theirs).
; The output waiting goes out first (the call may write)
sys_call:
            pla
            sta         p1
            pla
            sta         p1 + 1
            jsr         flush
            stz         sys_a
            stz         sys_x
            stz         sys_y
            ldy         #1
            lda         (p1),y
            sta         sys_to
            iny
            lda         (p1),y
            sta         sys_to + 1
            iny
            lda         (p1),y
            sta         sys_f
            iny
            lda         (p1),y
            sta         cnt
@in:
            lda         cnt
            beq         @call
            dec         cnt
            iny
            lda         (p1),y
            sty         tmp3
            jsr         sys_pop
            ldy         tmp3
            bra         @in
@call:
            sty         tmp3
            stx         xsave
            ldy         sys_y
            ldx         sys_x
            lda         sys_a
            jsr         @go
            php
            sta         sys_a
            stx         sys_x
            sty         sys_y
            pla
            sta         sys_p
            ldx         xsave
            ldy         tmp3
            iny
            lda         (p1),y
            sta         cnt
@out:
            lda         cnt
            beq         @ior
            dec         cnt
            iny
            lda         (p1),y
            sty         tmp3
            jsr         sys_push
            ldy         tmp3
            bra         @out
@ior:
            bit         sys_f
            bpl         @done
            lda         sys_p
            lsr                                             ; (C: the call's)
            lda         sys_a
            jmp         push_ior
@done:
            rts
@go:
            jmp         (sys_to)

; The top into the register .A names: rN ($0N), rN and the next ($1N: a double, its high cell on top), .A, .X, .Y
; ($20-$22: the cell's low byte), .A/.X ($23)
sys_pop:
            cmp         #$20
            bcs         @reg
            cmp         #$10
            bcc         @r
            and         #$0F                                ; (A double: its high cell, then its low)
            inc
            pha
            jsr         @r
            pla
            dec
@r:
            asl                                             ; (C = 0 after)
            adc         #r0
            sta         p2
            stz         p2 + 1
            lda         dlo,x
            sta         (p2)
            ldy         #1
            lda         dhi,x
            sta         (p2),y
            inx
            rts
@reg:
            cmp         #$23
            beq         @ax
            and         #3
            tay
            lda         dlo,x
            sta         sys_a,y
            inx
            rts
@ax:
            lda         dlo,x
            sta         sys_a
            lda         dhi,x
            sta         sys_x
            inx
            rts

; The register .A names pushed (as sys_pop's); or 0, if the call failed (a double: 0 0)
sys_push:
            tay
            lda         sys_p
            lsr
            tya
            bcs         @zero
            cmp         #$20
            bcs         @reg
            cmp         #$10
            bcc         @r
            and         #$0F                                ; (A double: its low cell, then its high)
            pha
            jsr         @r
            pla
            inc
@r:
            asl                                             ; (C = 0 after)
            adc         #r0
            sta         p2
            stz         p2 + 1
            ldy         #1
            lda         (p2),y
            tay
            lda         (p2)
            PUSHAY
            rts
@reg:
            cmp         #$23
            beq         @ax
            and         #3
            tay
            lda         sys_a,y
            ldy         #0
            PUSHAY
            rts
@ax:
            lda         sys_a
            ldy         sys_x
            PUSHAY
            rts
@zero:
            and         #$F0
            cmp         #$10
            bne         :+
            dex
            jsr         zero_tos
:
            dex
            jmp         zero_tos

; ---- Zero-terminated strings

            HEADER      ">z", 0
toz:                                                        ; ( c-addr u -- z-addr ): a copy, zero-terminated (127
            lda         zbuf_n                              ;   chars at most), in one of two buffers in turn
            eor         #1
            sta         zbuf_n
            beq         :+
            lda         #<(zbufs + PATH_SIZE)
            ldy         #>(zbufs + PATH_SIZE)
            bra         :++
:
            lda         #<zbufs
            ldy         #>zbufs
:
            sta         w2
            sty         w2 + 1
            phy
            pha
            jsr         to_z
            pla
            ply
            PUSHAY
            rts

            HEADER      "zcount", 0
zcount_w:                                                   ; ( z-addr -- c-addr u )
            jmp         zcount

            HEADER      "ctl", 0
ctl:                                                        ; ( c-addr1 u1 c-addr2 u2 -- ): the text c-addr2 u2
            lda         dlo + 1,x                           ;   written to the file c-addr1 u1 in one write, as rc's
            sta         p1                                  ;   echo -n text >file writes it (a device's ctl: s"
            lda         dhi + 1,x                           ;   /dev/sd/0/ctl" s" check" ctl); a failure THROWs,
            sta         p1 + 1                              ;   named by the file
            lda         dlo,x
            sta         p2
            lda         dhi,x
            sta         p2 + 1
            inx
            inx
            lda         dlo + 1,x                           ; (The file's name, for an error)
            sta         throw_name
            lda         dhi + 1,x
            sta         throw_name + 1
            lda         dlo,x
            sta         throw_nlen
            jsr         toz
            lda         dlo,x
            sta         r0
            lda         dhi,x
            sta         r0 + 1
            inx
            lda         #O_WRITE
            stx         xsave
            jsr         OPEN
            ldx         xsave
            bcs         @fail
            sta         cnt
            lda         p1
            sta         r0
            lda         p1 + 1
            sta         r0 + 1
            lda         p2
            sta         r1
            lda         p2 + 1
            sta         r1 + 1
            lda         cnt
            stx         xsave
            jsr         WRITE
            php
            pha
            lda         cnt
            jsr         CLOSE
            pla
            plp
            ldx         xsave
            bcs         @fail
            rts
@fail:
            pha
            lda         #1
            sta         throw_named
            pla
            jmp         throw_os

; ---- Programs

            HEADER      "sh", 0
sh:                                                         ; ( c-addr u -- status ): the command line, as rc runs
            jsr         prog_rc                             ;   one (rc -c: /bin/rc, else the ROM's): its exit code,
            lda         #0                                  ;   or its status if that's a number
            jsr         rc_spawn
            bra         ran

            HEADER      "run", 0
run:                                                        ; ( c-addr u -- status ): a program and its arguments,
            jsr         prog_args                           ;   split at blanks (a name with no / in it is /bin's):
            lda         #0                                  ;   its exit code
            jsr         prog_spawn
ran:                                                        ; (Started, or not: waited for, its code pushed)
            bcc         :+
            jmp         throw_os
:
            jsr         prog_wait
            lda         tmp2
            ldy         tmp2 + 1
            PUSHAY
            rts

; ---- Banks and shared segments: a bank at $8000-$9FFF (BANK-WINDOW), the task's own (sys-banks-alloc gives them)
; or a shared segment's (sys-seg-create, sys-seg-attach).  A colon definition's code is in a bank there too (the
; core's code banks), so BANK! and SEG-BANK! in one THROW -21: false code-banks before it's compiled

            HEADER      "bank-window", 0
bankwindow:                                                 ; ( -- addr ): $8000, where the bank selected is
            CONSTCODE   BANK_WINDOW

            HEADER      "bank!", 0
bankstore:                                                  ; ( bank -- ): one of the task's at BANK-WINDOW
            jsr         bank_guard
            lda         dlo,x
            sta         RAM_BANK
            inx
            rts

; THROW -21 if this one's caller was called from code in a code bank (BANK!'s, SEG-BANK!'s: it would be gone from
; under it).  Keeps .X
bank_guard:
            stx         xsave
            tsx
            lda         $0104,x                             ; (The caller's return address's high byte)
            ldx         xsave
            cmp         #>BANK_WINDOW
            bcc         @ok
            cmp         #>(BANK_WINDOW + BANK_SIZE)
            bcs         @ok
            lda         #<-21
            jmp         throw_a
@ok:
            rts

            HEADER      "code-banks", 0
codebanks:                                                  ; ( flag -- ): false, colon definitions from now on
            ldy         #0                                  ;   compiled into the dictionary (as one with BANK! in
            lda         dlo,x                               ;   it needs); true, into the task's banks (as at
            ora         dhi,x                               ;   the start)
            bne         :+
            iny
:
            sty         cb_off
            inx
            rts

            HEADER      "bank@", 0
bankfetch:                                                  ; ( -- bank ): the bank at BANK-WINDOW
            lda         RAM_BANK
            ldy         #0
            PUSHAY
            rts

            HEADER      "seg-bank!", 0
segbankstore:                                               ; ( seg n -- ior ): bank n of shared segment seg (attached)
            jsr         bank_guard                          ;   at BANK-WINDOW (SEG_MAP: U and its bank register)
            lda         dlo + 1,x
            ldy         dlo,x
            inx
            inx
            stx         xsave
            phy
            plx
            jsr         SEG_MAP
            bcs         :+
            sta         U_REGISTER
            stx         RAM_BANK
:
            ldx         xsave
            jmp         push_ior

; ---- forth's arguments

            HEADER      "argc", 0
argc:                                                       ; ( -- n ): forth's arguments (forth file.fs a b: 3, the
            jsr         argl_first                           ;   file's name the first); at the console, 0
            stz         cnt
@arg:
            bcs         @push
            inc         cnt
            jsr         argl_next
            bra         @arg
@push:
            lda         cnt
            ldy         #0
            PUSHAY
            rts

            HEADER      "arg", 0
arg:                                                        ; ( n -- c-addr u ): argument n (0: the file's name); past
            lda         dhi,x                               ;   the last, 0 0
            bne         @none
            lda         dlo,x
            sta         cnt
            jsr         argl_first
@find:
            bcs         @none
            lda         cnt
            beq         @found
            dec         cnt
            jsr         argl_next
            bra         @find
@found:
            lda         w
            sta         dlo,x
            lda         w + 1
            sta         dhi,x
            ldy         #0                                  ; (Its length)
:
            lda         (w),y
            beq         :+
            iny
            bne         :-
:
            tya
            ldy         #0
            PUSHAY
            rts
@none:
            stz         dlo,x
            stz         dhi,x
            dex
            stz         dlo,x
            stz         dhi,x
            rts

; w = the first argument.  OUT: C = 1 if there's none (forth -l's -l isn't one: none, as at the console)
argl_first:
            sec
            lda         login
            bne         @none
            lda         argp
            sta         w
            lda         argp + 1
            sta         w + 1
            ora         w
            bne         argl_is
            sec
@none:
            rts

; w past its argument, to the next.  OUT: C = 1 if there's none (the empty one after the last)
argl_next:
            lda         (w)
            pha
            inc         w
            bne         :+
            inc         w + 1
:
            pla
            bne         argl_next
argl_is:
            lda         (w)
            bne         :+
            sec
            rts
:
            clc
            rts

; ---- Directories (Gforth's words), notes, a task's turn, errors' texts: hylang's Hydra built-ins, as Forth names
; them.  (mkdir is =mkdir, Gforth's name, so the shell's prompt still runs the mkdir program)

            HEADER      "get-dir", 0
getdir:                                                     ; ( c-addr1 u1 -- c-addr2 u2 ): the current directory, in
            LDR         r0, pathbuf                         ;   the buffer c-addr1 u1 (as much of it as fits)
            stx         xsave
            jsr         GETCWD
            ldx         xsave
            bcc         :+
            lda         #0
:
            sta         tmp                                 ; (Its length, the buffer's at most)
            lda         dhi,x
            bne         :+
            lda         dlo,x
            cmp         tmp
            bcs         :+
            sta         tmp
:
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0
:
            cpy         tmp
            beq         :+
            lda         pathbuf,y
            sta         (w),y
            iny
            bra         :-
:
            lda         tmp
            sta         dlo,x
            stz         dhi,x
            rts

            HEADER      "set-dir", 0
setdir:                                                     ; ( c-addr u -- wior ): the current directory c-addr u
            jsr         z_r0                                ;   (the shell's cd, in a definition)
            stx         xsave
            jsr         CHDIR
            ldx         xsave
            jmp         push_ior

            HEADER      "open-dir", 0
opendir:                                                    ; ( c-addr u -- wdirid wior ): a directory, for read-dir
            jsr         z_r0
            lda         #O_READ
            stx         xsave
            jsr         OPEN
            ldx         xsave
            jmp         fid_ior

            HEADER      "read-dir", 0
readdir:                                                    ; ( c-addr u1 wdirid -- u2 flag wior ): its next name (a
            LDR         r0, statbuf                         ;   stat record's) in the buffer c-addr u1, u2 long (as
            LDR         r1, SR_SIZE                         ;   much of it as fits); flag false at its end
            lda         dlo,x
            stx         xsave
            jsr         READ
            ldx         xsave
            bcc         :+
            pha                                             ; (An error: 0 false ior)
            jsr         rd_none
            pla
            sec
            bra         rd_ior
:
            cmp         #SR_SIZE                            ; (Its end: 0 false 0)
            bcs         :+
            jsr         rd_none
            clc
            bra         rd_ior
:
            ldy         #0                                  ; Its name's length (the buffer's at most)
:
            lda         statbuf + SR_NAME,y
            beq         :+
            iny
            cpy         #SR_QTYPE
            bne         :-
:
            sty         tmp
            lda         dhi + 1,x
            bne         :+
            lda         dlo + 1,x
            cmp         tmp
            bcs         :+
            sta         tmp
:
            lda         dlo + 2,x
            sta         w
            lda         dhi + 2,x
            sta         w + 1
            ldy         #0
:
            cpy         tmp
            beq         :+
            lda         statbuf + SR_NAME,y
            sta         (w),y
            iny
            bra         :-
:
            lda         tmp                                 ; ( u2 true 0 )
            sta         dlo + 2,x
            stz         dhi + 2,x
            lda         #$FF
            sta         dlo + 1,x
            sta         dhi + 1,x
            clc
rd_ior:                                                     ; (The top: the ior, C and .A's)
            inx
            jmp         push_ior
rd_none:                                                    ; (0 false under the top)
            stz         dlo + 2,x
            stz         dhi + 2,x
            stz         dlo + 1,x
            stz         dhi + 1,x
            rts

            HEADER      "close-dir", 0
closedir:                                                   ; ( wdirid -- wior )
            lda         dlo,x
            inx
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            jmp         push_ior

            HEADER      "=mkdir", 0
mkdir:                                                      ; ( c-addr u wmode -- wior ): a directory made (wmode: as
            inx                                             ;   it comes, the system's own)
            jsr         z_r0
            lda         #O_READ
            phx
            ldx         #DM_DIR
            jsr         CREATE
            plx
            bcs         :+
            stx         xsave
            jsr         CLOSE
            ldx         xsave
            clc
:
            jmp         push_ior

; ( c-addr u -- ): r0 the name, zero-terminated (>Z's buffers)
z_r0:
            jsr         toz
            lda         dlo,x
            sta         r0
            lda         dhi,x
            sta         r0 + 1
            inx
            rts

            HEADER      "note", 0
note:                                                       ; ( task n -- ): note n to the task (Plan 9's postnote: 1
            lda         dlo + 1,x                           ;   interrupt, 3 hangup, 4 alarm, 16-31 a program's own);
note_a:                                                     ;   a failure THROWs
            sta         tmp
            lda         dlo,x
            inx
            inx
            stx         xsave
            tax
            lda         tmp
            jsr         NOTE
            ldx         xsave
            bcc         :+
            jmp         throw_os
:
            rts

            HEADER      "note-group", 0
notegroup:                                                  ; ( group n -- ): the note to a note group's every task
            lda         dlo + 1,x
            ora         #NOTE_GROUP
            bra         note_a

            HEADER      "on-note", 0
onnote:                                                     ; ( xt -- ): xt ( n -- flag ) takes the notes that come
            lda         dlo,x                               ;   (but Ctrl-C's and kill's): at the next word interpreted
            sta         note_xt                             ;   or loop step, given the note; true, forth goes on;
            lda         dhi,x                               ;   false, as Ctrl-C (THROW -28).  0: none (a note's
            sta         note_xt + 1                         ;   default, the end)
            inx
            rts

            HEADER      "pause", 0
pause:                                                      ; ( -- ): the other tasks' turn (YIELD)
            stx         xsave
            jsr         YIELD
            ldx         xsave
            rts

            HEADER      "ior>text", 0
iortext:                                                    ; ( ior -- c-addr u ): a system error's text (an ior of a
            lda         dhi,x                               ;   file word's or a sys- word's: -512 less the error,
            cmp         #$FD                                ;   $FDxx); another, ""
            bne         @none
            lda         dlo,x                               ; (The error: -512 - ior)
            eor         #$FF
            inc
            sta         tmp
            LDR         r0, errbuf
            lda         tmp
            stx         xsave
            jsr         ERRSTR
            ldx         xsave
            inx
            lda         #<errbuf
            ldy         #>errbuf
            PUSHAY
            jmp         zcount
@none:
            dex
            jsr         zero_tos
            lda         #<errbuf
            sta         dlo + 1,x
            lda         #>errbuf
            sta         dhi + 1,x
            rts

; ---- Machine code: SYS, the old HyForth's

            HEADER      "sys", 0
sys:                                                        ; ( addr a x y -- a x y p ): the machine code at addr
            lda         dlo,x                               ;   called (jsr) with .A, .X and .Y (each cell's low
            sta         sys_y                               ;   byte), and what they were after it, with its flags
            lda         dlo + 1,x                           ;   (P: C is bit 0).  forth's data stack is .X and the
            sta         sys_x                               ;   zero page from $22: the code must leave them be
            lda         dlo + 2,x
            sta         sys_a
            lda         dlo + 3,x
            sta         sys_to
            lda         dhi + 3,x
            sta         sys_to + 1
            phx
            lda         sys_a
            ldx         sys_x
            ldy         sys_y
            jsr         @call
            php
            sta         sys_a
            stx         sys_x
            sty         sys_y
            pla
            plx
            sta         dlo,x
            lda         sys_y
            sta         dlo + 1,x
            lda         sys_x
            sta         dlo + 2,x
            lda         sys_a
            sta         dlo + 3,x
            stz         dhi,x
            stz         dhi + 1,x
            stz         dhi + 2,x
            stz         dhi + 3,x
            rts
@call:
            jmp         (sys_to)

; ---- The sys- words' headers

.include "forthsys.inc"
