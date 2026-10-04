; ****************************************************************************
; fhydra.inc - forth's Hydra words (included by forth.s): the sys- words' call (their headers are made in RAM as forth
; starts: fsys.inc, in the second bank, from the specification's table), zero-terminated strings, SH and RUN, the
; bank and segment words, and Ctrl-C.
;   A sys- word is a system call with its registers as stack items, in the specification's order (spec/api.def;
; /rom/doc/api.md has each one's), the first deepest: its inputs, then its outputs and, if the call can fail, an ior
; (0, or -512 less the error code; its outputs 0 then).  A register is a cell (rN, .A/.X: 16 bits; .A, .X, .Y: a
; byte), or a double (rN and the next: r0/r1).  A name is a zero-terminated string's address (>Z).

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

            HEADER      ">Z", 0
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

            HEADER      "ZCOUNT", 0
zcount:                                                     ; ( z-addr -- c-addr u )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            stz         tmp
            stz         tmp + 1
:
            lda         (w)
            beq         :+
            jsr         w_inc
            inc         tmp
            bne         :-
            inc         tmp + 1
            bra         :-
:
            lda         tmp
            ldy         tmp + 1
            PUSHAY
            rts

; ---- Programs

            HEADER      "SH", 0
sh:                                                         ; ( c-addr u -- status ): the command line, as rc runs
            LDR         w2, argbuf                          ;   one (rc -c: /bin/rc, else the ROM's): its exit code,
            LDR         p2, argbuf + ARGS_MAX - 2           ;   or its status if that's a number
            lda         #'-'
            jsr         arg_put
            lda         #'c'
            jsr         arg_put
            lda         #0
            jsr         arg_put
            lda         dlo + 1,x                           ; The line, whole
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            inx
:
            lda         tmp
            ora         tmp + 1
            beq         :+
            lda         (w)
            jsr         arg_put
            jsr         arg_next
            bra         :-
:
            lda         #0                                  ; (Its 0, and the empty one after it)
            sta         (w2)
            ldy         #1
            sta         (w2),y
            LDR         r0, s_binrc
            jsr         spawn
            bcs         :+
            jmp         wait_task
:
            cmp         #E_NOENT
            bne         :+
            LDR         r0, s_mrc
            jsr         spawn
            bcs         :+
            jmp         wait_task
:
            jmp         throw_os

            HEADER      "RUN", 0
run:                                                        ; ( c-addr u -- status ): a program and its arguments,
            lda         dlo + 1,x                           ;   split at blanks (a name with no / in it is /bin's):
            sta         w                                   ;   its exit code
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            inx
            LDR         w2, pathbuf + 5                     ; Its name (room for /bin/ before it)
            LDR         p2, pathbuf + PATH_SIZE - 1
            jsr         next_arg
            bcc         :+
            lda         #<-16                               ; (None: no name)
            jmp         throw_a
:
            LDR         r0, pathbuf + 5
            lda         pathbuf + 5
            cmp         #'#'
            beq         @args
            ldy         #0
:
            lda         pathbuf + 5,y
            beq         @bin
            iny
            cmp         #'/'
            bne         :-
            bra         @args
@bin:
            ldy         #4                                  ; (/bin/ before it)
:
            lda         s_binrc,y
            sta         pathbuf,y
            dey
            bpl         :-
            LDR         r0, pathbuf
@args:
            LDR         w2, argbuf                          ; Its arguments, each zero-terminated, then an empty one
            LDR         p2, argbuf + ARGS_MAX - 1
:
            jsr         next_arg
            bcc         :-
            lda         #0
            sta         (w2)
            jsr         spawn
            bcc         wait_task
            jmp         throw_os

; ( -- status ): task .A waited for (a note ending the wait: waited for again): its exit code, or, if that's 1 and
; its message is a number (rc's $status), the number
wait_task:
            sta         tmp
@wait:
            LDR         r0, statbuf
            lda         tmp
            stx         xsave
            jsr         WAIT
            stx         tmp2
            ldx         xsave
            bcc         @ended
            cmp         #E_INTR
            beq         @wait
            jmp         throw_os
@ended:
            stz         tmp2 + 1
            lda         tmp2
            cmp         #1
            bne         @push
            lda         statbuf                             ; (A number?)
            sec
            sbc         #'0'
            cmp         #10
            bcs         @push
            stz         tmp2
            ldy         #0
@digit:
            lda         statbuf,y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @push
            pha
            asl         tmp2                                ; (* 10: * 2, kept, * 4, and the kept added)
            rol         tmp2 + 1
            lda         tmp2
            sta         numtmp
            lda         tmp2 + 1
            sta         numtmp + 1
            asl         tmp2
            rol         tmp2 + 1
            asl         tmp2
            rol         tmp2 + 1
            clc
            lda         tmp2
            adc         numtmp
            sta         tmp2
            lda         tmp2 + 1
            adc         numtmp + 1
            sta         tmp2 + 1
            clc
            pla
            adc         tmp2
            sta         tmp2
            bcc         :+
            inc         tmp2 + 1
:
            iny
            bra         @digit
@push:
            lda         tmp2
            ldy         tmp2 + 1
            PUSHAY
            rts

; SPAWN r0's program with argbuf's arguments (the output waiting out first: r0 kept over its WRITE).  OUT: C, .A:
; SPAWN's
spawn:
            lda         r0
            pha
            lda         r0 + 1
            pha
            jsr         flush
            pla
            sta         r0 + 1
            pla
            sta         r0
            LDR         r1, argbuf
            stx         xsave
            lda         #0
            jsr         SPAWN
            ldx         xsave
            rts

; The next word of the string at w (tmp chars left) into (w2), zero-terminated, w2 past its 0.  OUT: C = 1 if there
; was none (nothing stored)
next_arg:
@skip:
            lda         tmp
            ora         tmp + 1
            beq         @none
            lda         (w)
            cmp         #' ' + 1
            bcs         @word
            jsr         arg_next
            bra         @skip
@word:
            lda         tmp
            ora         tmp + 1
            beq         @end
            lda         (w)
            cmp         #' ' + 1
            bcc         @end
            jsr         arg_put
            jsr         arg_next
            bra         @word
@end:
            lda         #0
            jsr         arg_put
            clc
            rts
@none:
            sec
            rts

; w on a char, tmp one less
arg_next:
            jsr         w_inc
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            rts

; .A into (w2), w2 on; at p2 (the buffer's end): THROW the ior of E_NAMETOOLONG.  Keeps .Y
arg_put:
            pha
            lda         w2
            cmp         p2
            lda         w2 + 1
            sbc         p2 + 1
            pla
            bcc         :+
            lda         #E_NAMETOOLONG
            jmp         throw_os
:
            sta         (w2)
            inc         w2
            bne         :+
            inc         w2 + 1
:
            rts

s_binrc:    .byte       "/bin/rc", 0
s_mrc:      .byte       "#m/rc", 0

; ---- Banks and shared segments: a bank at $8000-$9FFF (BANK-WINDOW), the task's own (sys-banks-alloc gives them)
; or a shared segment's (sys-seg-create, sys-seg-attach)

            HEADER      "BANK-WINDOW", 0
bankwindow:                                                 ; ( -- addr ): $8000, where the bank selected is
            lda         #<BANK_WINDOW
            ldy         #>BANK_WINDOW
            PUSHAY
            rts

            HEADER      "BANK!", 0
bankstore:                                                  ; ( bank -- ): one of the task's at BANK-WINDOW
            lda         dlo,x
            sta         RAM_BANK
            inx
            rts

            HEADER      "BANK@", 0
bankfetch:                                                  ; ( -- bank ): the bank at BANK-WINDOW
            lda         RAM_BANK
            ldy         #0
            PUSHAY
            rts

            HEADER      "SEG-BANK!", 0
segbankstore:                                               ; ( seg n -- ior ): bank n of shared segment seg (attached)
            lda         dlo + 1,x                           ;   at BANK-WINDOW (SEG_MAP: U and its bank register)
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

; ---- Ctrl-C: the note handler (forth.s's notes) sets intr; a word interpreted, a loop (compiled: comp_poll; DO's:
; xloop, xploop) and a wait (KEY, MS, a file's line) THROW -28 then

; THROW -28, intr cleared
intr_throw:
            stz         intr
            lda         #<-28
            jmp         throw_a

; ---- The second bank's words: each one's code (its header in RAM) is jsr far_word, then its code's address there

; A second bank's word: its code, through FAR2 (the return address is its caller's)
far_word:
            pla
            sta         p1
            pla
            sta         p1 + 1
            ldy         #1
            lda         (p1),y
            sta         hyx2_to
            iny
            lda         (p1),y
            sta         hyx2_to + 1
            jmp         hyx2_far2

; ---- Scripts and libraries

; forth file.fs [argument ...]: the file INCLUDED (a name with no / not there: /lib/forth's), then the end: code 0,
; or 1 after an error's message (its file and line)
run_script:
            lda         argp
            ldy         argp + 1
            PUSHAY
            jsr         zcount
            lda         #<included
            ldy         #>included
            PUSHAY
            jsr         catch
            lda         dlo,x
            ora         dhi,x
            beq         :+
            jsr         show_error
            lda         #1
:
            pha
            jsr         flush
            stz         r0
            stz         r0 + 1
            pla
            jmp         EXITS

            HEADER      "LIBRARY", 0
library:                                                    ; ( "name" -- wid ): a library's start: name a CONSTANT,
            jsr         getcurrent                          ;   a new word list, which goes first in the search order
            jsr         wordlist                            ;   and takes the definitions; wid the compilation word
            jsr         dup                                 ;   list before (END-LIBRARY's)
            jsr         constant
            jsr         dup
            jsr         setcurrent
            lda         order_n                             ; (First in the order, before the rest)
            bne         :+
            inc         order_n
            bra         @first
:
            jsr         also
@first:
            lda         dlo,x
            sta         order
            lda         dhi,x
            sta         order + 1
            inx
            rts

            HEADER      "END-LIBRARY", 0
endlibrary:                                                 ; ( wid -- ): a library's end: definitions where they were,
            jmp         setcurrent                          ;   its word list kept in the order
