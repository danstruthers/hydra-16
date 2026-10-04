; ****************************************************************************
; fbank2.inc - forth's words in its second bank (included by forth.s, in CODE2): each has its header in RAM, made as
; forth starts (FARWORD's entries in far_table: fsys.inc's sys_build), and its code here, run through FAR2 (far_word),
; calling the first bank's routines through FAR1 (a THROW then goes back to its CATCH, in the first bank, with that
; bank there).  What it reads it reads in RAM (or this bank).
;   REPLACES's substitutions are in substs, each its name (counted) then its text (counted): SUBSTITUTE finds a name
; in either case.  forth's arguments (forth file.fs a b) are the list main was given (argp): ARGC, ARG.

; .A in upper case (the first bank's upper)
upper2:
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            rts

; ---- Strings

            FARWORD     "UNESCAPE", unescape, 0
unescape:                                                   ; ( c-addr1 u1 c-addr2 -- c-addr2 u2 ): each % doubled
            lda         dlo,x
            sta         w2
            sta         w3
            lda         dhi,x
            sta         w2 + 1
            sta         w3 + 1
            lda         dlo + 1,x
            sta         tmp
            lda         dhi + 1,x
            sta         tmp + 1
            lda         dlo + 2,x
            sta         w
            lda         dhi + 2,x
            sta         w + 1
            inx
@char:
            lda         tmp
            ora         tmp + 1
            beq         @done
            lda         (w)
            cmp         #'%'
            bne         :+
            jsr         @put
            lda         #'%'
:
            jsr         @put
            inc         w
            bne         :+
            inc         w + 1
:
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            bra         @char
@done:
            lda         w2                                  ; ( c-addr2 u2 ): u2 = w3 - c-addr2
            sta         dlo + 1,x
            lda         w2 + 1
            sta         dhi + 1,x
            sec
            lda         w3
            sbc         w2
            sta         dlo,x
            lda         w3 + 1
            sbc         w2 + 1
            sta         dhi,x
            rts
@put:
            sta         (w3)
            inc         w3
            bne         :+
            inc         w3 + 1
:
            rts

            FARWORD     "REPLACES", replaces, 0
replaces:                                                   ; ( c-addr1 u1 c-addr2 u2 -- ): SUBSTITUTE's %c-addr2%
            lda         dlo + 1,x                           ;   (its name) c-addr1 u1, copied (no room: THROW -79)
            sta         p2
            lda         dhi + 1,x
            sta         p2 + 1
            lda         dlo,x
            jsr         subst_find
            bcs         @add
            sec                                             ; One there by its name: out (those after it down over
            lda         (p2)                                ;   it)
            adc         p2
            sta         w3
            lda         p2 + 1
            adc         #0
            sta         w3 + 1
            phx
            sec
            lda         w3
            sbc         #<substs
            tax
            sec
            lda         p1
            sbc         #<substs
            tay
:
            cpx         subst_len
            bcs         :+
            lda         substs,x
            sta         substs,y
            inx
            iny
            bra         :-
:
            sty         subst_len
            plx
@add:
            lda         dhi,x                               ; Room: its name and text, counted?
            ora         dhi + 2,x
            bne         @full
            clc
            lda         subst_len
            adc         dlo,x
            bcs         @full
            adc         dlo + 2,x
            bcs         @full
            adc         #2                                  ; (SUBST_SIZE is 255: a carry is too many)
            bcs         @full
            lda         dlo + 1,x                           ; Its name, then its text
            sta         p2
            lda         dhi + 1,x
            sta         p2 + 1
            ldy         subst_len
            lda         dlo,x
            jsr         @copy
            lda         dlo + 3,x
            sta         p2
            lda         dhi + 3,x
            sta         p2 + 1
            lda         dlo + 2,x
            jsr         @copy
            sty         subst_len
            txa
            clc
            adc         #4
            tax
            rts
@full:
            lda         #<-79
            FAR1        throw_a
@copy:                                                      ; .A bytes from (p2), counted, into substs at .Y
            sta         cnt
            sta         substs,y
            iny
:
            lda         cnt
            beq         :+
            lda         (p2)
            sta         substs,y
            iny
            inc         p2
            bne         @n
            inc         p2 + 1
@n:
            dec         cnt
            bra         :-
:
            rts

; The substitution named (p2, .A chars), in either case: C = 0, p1 its record, p2 its text (counted); or C = 1
subst_find:
            sta         cnt
            LDR         p1, substs
@rec:
            sec                                             ; (Past the last: none)
            lda         p1
            sbc         #<substs
            cmp         subst_len
            bcs         @none
            lda         (p1)
            cmp         cnt
            bne         @next
            tay
@char:
            cpy         #0
            beq         @found
            lda         (p1),y
            jsr         upper2
            sta         numtmp
            dey
            lda         (p2),y
            jsr         upper2
            cmp         numtmp
            bne         @next
            bra         @char
@found:
            sec                                             ; (Its text: after its name)
            lda         (p1)
            adc         p1
            sta         p2
            lda         p1 + 1
            adc         #0
            sta         p2 + 1
            clc
            rts
@next:
            sec                                             ; Past its name, then its text
            lda         (p1)
            adc         p1
            sta         p1
            bcc         :+
            inc         p1 + 1
:
            sec
            lda         (p1)
            adc         p1
            sta         p1
            bcc         @rec
            inc         p1 + 1
            bra         @rec
@none:
            sec
            rts

            FARWORD     "SUBSTITUTE", substitute, 0
substitute:                                                 ; ( c-addr1 u1 c-addr2 u2 -- c-addr2 u3 n ): c-addr1 u1
            lda         dlo,x                               ;   into c-addr2, each %name% REPLACES's text (n of
            sta         tmp2                                ;   them), %% a %; n -78: no room, or the two overlap
            lda         dhi,x
            sta         tmp2 + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            lda         dlo + 2,x
            sta         tmp
            lda         dhi + 2,x
            sta         tmp + 1
            lda         dlo + 3,x
            sta         w
            lda         dhi + 3,x
            sta         w + 1
            stz         tmp3                                ; (tmp3: the chars out; numacc: n, then no room)
            stz         tmp3 + 1
            stz         numacc
            stz         numacc + 1
            stz         numacc + 2
            clc                                             ; Overlapping: c-addr1 < c-addr2 + u2 and c-addr2 <
            lda         w2                                  ;   c-addr1 + u1
            adc         tmp2
            sta         p1
            lda         w2 + 1
            adc         tmp2 + 1
            sta         p1 + 1
            lda         w
            cmp         p1
            lda         w + 1
            sbc         p1 + 1
            bcs         @char
            clc
            lda         w
            adc         tmp
            sta         p1
            lda         w + 1
            adc         tmp + 1
            sta         p1 + 1
            lda         w2
            cmp         p1
            lda         w2 + 1
            sbc         p1 + 1
            bcs         @char
            inc         numacc + 2
            jmp         @end
@char:
            lda         tmp
            ora         tmp + 1
            bne         :+
            jmp         @end
:
            lda         (w)
            cmp         #'%'
            beq         @pct
@lit:
            jsr         @out
            lda         #1
@adv:                                                       ; On .A chars
            sta         cnt
            clc
            adc         w
            sta         w
            bcc         :+
            inc         w + 1
:
            sec
            lda         tmp
            sbc         cnt
            sta         tmp
            bcs         @char
            dec         tmp + 1
            bra         @char
@pct:
            lda         tmp + 1                             ; %%: a %
            bne         :+
            lda         tmp
            cmp         #2
            bcc         @lone
:
            ldy         #1
            lda         (w),y
            cmp         #'%'
            bne         @name
            jsr         @out
            lda         #2
            bra         @adv
@name:
            ldy         #1                                  ; The % that ends the name (within 255 chars)
@scan:
            iny
            beq         @lone
            lda         tmp + 1
            bne         :+
            cpy         tmp
            bcs         @lone
:
            lda         (w),y
            cmp         #'%'
            bne         @scan
            phy
            clc
            lda         w
            adc         #1
            sta         p2
            lda         w + 1
            adc         #0
            sta         p2 + 1
            tya
            dec
            jsr         subst_find
            ply
            bcs         @lone
            phy                                             ; Its text out
            lda         (p2)
            sta         cnt
            ldy         #0
:
            cpy         cnt
            beq         :+
            iny
            lda         (p2),y
            jsr         @out
            bra         :-
:
            inc         numacc
            pla
            inc
            bra         @adv
@lone:
            lda         #'%'                                ; (No name ends: the % as it is)
            bra         @lit
@end:
            lda         w2                                  ; ( c-addr2 u3 n )
            sta         dlo + 3,x
            lda         w2 + 1
            sta         dhi + 3,x
            lda         tmp3
            sta         dlo + 2,x
            lda         tmp3 + 1
            sta         dhi + 2,x
            lda         numacc
            sta         dlo + 1,x
            stz         dhi + 1,x
            lda         numacc + 2
            beq         :+
            lda         #<-78
            sta         dlo + 1,x
            lda         #$FF
            sta         dhi + 1,x
:
            inx
            rts
@out:                                                       ; .A out (no room: noted).  Keeps .Y
            pha
            lda         tmp3
            cmp         tmp2
            lda         tmp3 + 1
            sbc         tmp2 + 1
            bcs         @over
            clc
            lda         w2
            adc         tmp3
            sta         p1
            lda         w2 + 1
            adc         tmp3 + 1
            sta         p1 + 1
            pla
            sta         (p1)
            inc         tmp3
            bne         :+
            inc         tmp3 + 1
:
            rts
@over:
            pla
            lda         #1
            sta         numacc + 2
            rts

; ---- forth's arguments

            FARWORD     "ARGC", argc, 0
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

            FARWORD     "ARG", arg, 0
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

; w = the first argument.  OUT: C = 1 if there's none
argl_first:
            lda         argp
            sta         w
            lda         argp + 1
            sta         w + 1
            ora         w
            bne         argl_is
            sec
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

; ---- Libraries: INCLUDED's name, not found as it is

; ( c-addr u -- c-addr' u' ): a bare name (no /) as /lib/forth's, in argbuf (INCLUDED's, through FAR2).  OUT: C = 1,
; the name left as it was, if it isn't bare (or is too long)
lib_name:
            lda         dhi,x
            bne         @not
            lda         dlo,x
            beq         @not
            cmp         #ARGS_MAX - LIB_LEN - 1
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
            sta         argbuf,y
            dey
            bpl         :-
            ldy         #0
:
            lda         (w),y
            sta         argbuf + LIB_LEN,y
            iny
            cpy         cnt
            bne         :-
            clc
            lda         dlo,x
            adc         #LIB_LEN
            sta         dlo,x
            lda         #<argbuf
            sta         dlo + 1,x
            lda         #>argbuf
            sta         dhi + 1,x
            clc
            rts
@not:
            sec
            rts

s_lib:      .byte       "/lib/forth/"
LIB_LEN     = * - s_lib
