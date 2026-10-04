; ****************************************************************************
; tools.s - HyForth's Programming-Tools library (/lib/forth/tools.fl: REQUIRE tools.fl): .S ? WORDS DUMP SEE, AHEAD,
; [IF] [ELSE] [THEN] and their kind, the control-flow stack's, the return stack's, SYNONYM, and the name tokens' (an
; nt is a header's address).  BYE is the core's.

.include "forthlib.inc"

            HEADER      "AHEAD", F_IMMEDIATE
ahead:
            jmp         comp_fwd

            HEADER      "?", 0
question:
            jsr         fetch
            jmp         dot

            HEADER      ".S", 0
dots:                                                       ; ( -- ): "<depth> items", the top last
            lda         #'<'
            jsr         emit_a
            jsr         depth
            jsr         u_text
            jsr         type
            lda         #'>'
            jsr         emit_a
            jsr         space
            stx         tmp3                                ; (The top's index; tmp3 + 1: the next item's, + 1)
            lda         #DS_N
            sta         tmp3 + 1
@item:
            dec         tmp3 + 1                            ; The deepest first, to the top
            lda         tmp3 + 1
            cmp         tmp3
            bcc         @done
            tay
            dex
            lda         dlo,y
            sta         dlo,x
            lda         dhi,y
            sta         dhi,x
            jsr         dot
            bra         @item
@done:
            rts

            HEADER      "WORDS", 0
words:                                                      ; The first word list in the order: its names, newest first
            lda         order_n
            beq         @done
            lda         order
            sta         w
            lda         order + 1
            sta         w + 1
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
@hdr:
            lda         w
            ora         w + 1
            beq         @done
            ldy         #2
            lda         (w),y
            and         #F_HIDDEN
            bne         @next
            lda         (w),y
            and         #LEN_MASK
            sta         cnt
            ldy         #3
:
            lda         (w),y
            jsr         emit_a
            iny
            dec         cnt
            bne         :-
            jsr         space
@next:
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
            bra         @hdr
@done:
            jmp         cr

            HEADER      "DUMP", 0
dump:                                                       ; ( addr u -- ): 8 bytes a line, in hex and as text
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            inx
@line:
            lda         tmp
            ora         tmp + 1
            beq         @done
            lda         w + 1
            jsr         hex2
            lda         w
            jsr         hex2
            lda         #':'
            jsr         emit_a
            ldy         #0
@hex:
            jsr         space
            jsr         @have
            bcc         :+
            lda         (w),y
            jsr         hex2
            bra         :++
:
            jsr         space
            jsr         space
:
            iny
            cpy         #8
            bne         @hex
            jsr         space
            jsr         space
            ldy         #0
@text:
            jsr         @have
            bcc         @end
            lda         (w),y
            cmp         #' '
            bcc         :+
            cmp         #$7F
            bcc         :++
:
            lda         #'.'
:
            jsr         emit_a
            iny
            cpy         #8
            bne         @text
@end:
            jsr         cr
            clc
            lda         w
            adc         #8
            sta         w
            bcc         :+
            inc         w + 1
:
            lda         tmp + 1                             ; (u less 8, or 0)
            bne         :+
            lda         tmp
            cmp         #8
            bcs         :+
            stz         tmp
            bra         @line
:
            sec
            lda         tmp
            sbc         #8
            sta         tmp
            bcs         @line
            dec         tmp + 1
            bra         @line
@done:
            rts
@have:                                                      ; C = 1: byte .Y is the dump's (.Y < u)
            lda         tmp + 1
            bne         :+
            cpy         tmp
            bcs         :++
:
            sec
            rts
:
            clc
            rts

; .A out as two hex digits.  Keeps .X, .Y
hex2:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         @digit
            pla
            and         #$0F
@digit:
            cmp         #10
            bcc         :+
            adc         #6                                  ; (C = 1: 'A' on)
:
            adc         #'0'
            jmp         emit_a

; ---- SEE

            HEADER      "SEE", 0
see:                                                        ; ( "name" -- ): its definition: a word's name for a call
            jsr         name_hdr                            ;   (or the address), a literal's number, a branch's and
            lda         #':'                                ;   IF's address, a string's text, an inline word's name
            jsr         emit_a                              ;   for its code (or the bytes); to the rts past every
            jsr         space                               ;   branch's address
            jsr         hdr_out
            jsr         hdr_xt
            lda         cnt
            pha
            lda         w2
            sta         w3
            lda         w2 + 1
            sta         w3 + 1
            stz         p2                                  ; (p2: the furthest branch's address)
            stz         p2 + 1
@op:
            lda         (w3)
            beq         @zero
            cmp         #RTS_OP
            beq         @rts
            cmp         #JSR_OP
            beq         @jsr
            cmp         #JMP_OP
            bne         :+
            jmp         @jmp
:
            cmp         #OP_DEX
            bne         :+
            jmp         @lit
:
            cmp         #OP_INX
            bne         :+
            jmp         @if
:
            cmp         #OP_BIT_ZP                          ; (A loop's Ctrl-C poll: not shown)
            bne         @code
            ldy         #1
            lda         (w3),y
            cmp         #intr
            bne         @code
            lda         #7
            bra         @adv
@code:
            jsr         inline_at                           ; An inline word's code?  Or a byte
            bcc         @adv
            lda         #'$'
            jsr         emit_a
            lda         (w3)
            jsr         hex2
            jsr         space
            lda         #1
@adv:
            clc
            adc         w3
            sta         w3
            bcc         @op
            inc         w3 + 1
            bra         @op
@zero:
            jsr         @past                               ; (A 0 past every branch's address: the end, as
            bcs         @end                                ;   compiled code has none; else a byte)
            jmp         @code
@rts:
            jsr         @past                               ; Past every branch's address: the end
            bcs         @end
            LDR         w, s_exit
            jsr         type_z
            lda         #1
            bra         @adv
@end:
            lda         #';'
            jsr         emit_a
            pla
            bpl         :+
            LDR         w, s_immed
            jsr         type_z
:
            jmp         cr

@jsr:
            jsr         @target
            ldy         #0                                  ; One of the routines compiled code calls?
@special:
            lda         see_xt,y
            ora         see_xt + 1,y
            beq         @call
            lda         see_xt,y
            cmp         p1
            bne         :+
            lda         see_xt + 1,y
            cmp         p1 + 1
            beq         @known
:
            iny
            iny
            bra         @special
@known:
            lda         see_text,y                          ; Its text, then what's after the call: a string; a
            sta         w                                   ;   number of bytes; or nothing more (CREATE's and the
            lda         see_text + 1,y                      ;   like: their data)
            sta         w + 1
            tya
            lsr
            tay
            lda         see_after,y
            pha
            jsr         type_z
            jsr         space
            pla
            bmi         @string
            beq         :+
            cmp         #$7F
            beq         @data
:
            clc
            adc         #3
            jmp         @adv
@data:
            pla
            jmp         cr
@string:
            ldy         #3                                  ; The text, and "
            lda         (w3),y
            sta         cnt
:
            lda         cnt
            beq         :+
            iny
            lda         (w3),y
            jsr         emit_a
            dec         cnt
            bra         :-
:
            lda         #'"'
            jsr         emit_a
            jsr         space
            iny
            tya
            jmp         @adv
@call:
            jsr         xt_out
            lda         #3
            jmp         @adv

@jmp:
            jsr         @target                             ; To a word (a DEFER's, a SYNONYM's), or elsewhere: out,
            jsr         xt_name                             ;   and the end if no branch goes past it
            bcs         @where
            LDR         w2, s_jmp
            jsr         @named
            bra         @out
@where:
            sec                                             ; (A branch: back, or less than 1K on)
            lda         p1
            sbc         w3
            lda         p1 + 1
            sbc         w3 + 1
            bcc         @branch
            cmp         #4
            bcc         @branch
            LDR         w, s_jmp
            jsr         type_z
            jsr         @hex
@out:
            jsr         @past
            bcc         :+
            jmp         @end
:
            lda         #3
            jmp         @adv
@branch:
            LDR         w, s_branch
            jsr         type_z
            jsr         @to
            lda         #3
            jmp         @adv

@lit:
            ldy         #8                                  ; dex, lda #lo, sta dlo,x, lda #hi, sta dhi,x?
:
            lda         (w3),y
            cmp         see_lit,y
            bne         :+
            dey
            bpl         :-
            bra         @number
:
            cpy         #2                                  ; (Its bytes 2 and 6 are the number's)
            beq         :+
            cpy         #6
            bne         @code_j
:
            dey
            bpl         :---
@number:
            ldy         #6
            lda         (w3),y
            dex
            sta         dhi,x
            ldy         #2
            lda         (w3),y
            sta         dlo,x
            lda         w3                                  ; (HOLD's use w3)
            pha
            lda         w3 + 1
            pha
            jsr         dot
            pla
            sta         w3 + 1
            pla
            sta         w3
            lda         #9
            jmp         @adv
@code_j:
            jmp         @code

@if:
            ldy         #6                                  ; inx, lda dlo-1,x, ora dhi-1,x, bne +3, then jmp
:
            lda         (w3),y
            cmp         see_if,y
            bne         @code_j
            dey
            bpl         :-
            lda         w3                                  ; (Its jmp: as if it were here)
            clc
            adc         #7
            sta         w3
            bcc         :+
            inc         w3 + 1
:
            jsr         @target
            LDR         w, s_qbranch
            jsr         type_z
            jsr         @to
            lda         #3
            jmp         @adv

@past:                                                      ; C = 1: w3 is past every branch's address (p2)
            lda         w3
            cmp         p2
            lda         w3 + 1
            sbc         p2 + 1
            rts
@target:                                                    ; p1 = the address after the opcode
            ldy         #1
            lda         (w3),y
            sta         p1
            iny
            lda         (w3),y
            sta         p1 + 1
            rts
@to:                                                        ; p1 out ($hex), and noted if it's past the others
            jsr         @hex
            lda         p2
            cmp         p1
            lda         p2 + 1
            sbc         p1 + 1
            bcs         :+
            lda         p1
            sta         p2
            lda         p1 + 1
            sta         p2 + 1
:
            rts
@hex:
            jmp         p1_out
@named:                                                     ; The text at w2, then w's name
            phy
            lda         w
            pha
            lda         w + 1
            pha
            lda         w2
            sta         w
            lda         w2 + 1
            sta         w + 1
            jsr         type_z
            pla
            sta         w + 1
            pla
            sta         w
            ply
            jmp         hdr_out

; The xt p1 out: its word's name, or $ and the address
xt_out:
            jsr         xt_name
            bcs         p1_out
            jmp         hdr_out

; p1 out: $ and it in hex, and a space
p1_out:
            lda         #'$'
            jsr         emit_a
            lda         p1 + 1
            jsr         hex2
            lda         p1
            jsr         hex2
            jmp         space

; The header w's name out, and a space
hdr_out:
            ldy         #2
            lda         (w),y
            and         #LEN_MASK
            sta         cnt
            ldy         #3
:
            lda         cnt
            beq         :+
            lda         (w),y
            jsr         emit_a
            iny
            dec         cnt
            bra         :-
:
            jmp         space

; The word whose xt is p1, in any word list: C = 0, w its header; or C = 1.  Keeps w3
xt_name:
            lda         wl_last
            sta         tmp3
            lda         wl_last + 1
            sta         tmp3 + 1
@wl:
            lda         tmp3
            ora         tmp3 + 1
            beq         @none
            ldy         #1
            lda         (tmp3),y
            sta         w + 1
            lda         (tmp3)
            sta         w
@hdr:
            lda         w
            ora         w + 1
            beq         @next
            jsr         hdr_xt
            lda         w2
            cmp         p1
            bne         :+
            lda         w2 + 1
            cmp         p1 + 1
            beq         @found
:
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
            bra         @hdr
@next:
            ldy         #3
            lda         (tmp3),y
            pha
            dey
            lda         (tmp3),y
            sta         tmp3
            pla
            sta         tmp3 + 1
            bra         @wl
@none:
            sec
            rts
@found:
            clc
            rts

; Is the code at w3 an inline word's (FORTH's, in ROM)?  C = 0: its name out, .A its length; or C = 1
inline_at:
            lda         #<forth_last
            sta         w
            lda         #>forth_last
            sta         w + 1
@hdr:
            lda         w
            ora         w + 1
            beq         @none
            ldy         #2
            lda         (w),y
            and         #F_INLINE
            beq         @next
            lda         w3                                  ; (hdr_xt keeps w3)
            pha
            lda         w3 + 1
            pha
            jsr         hdr_xt                              ; (tmp: its code's length)
            pla
            sta         w3 + 1
            pla
            sta         w3
            ldy         tmp
:
            dey
            bmi         @found
            lda         (w2),y
            cmp         (w3),y
            beq         :-
@next:
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            sta         w
            pla
            sta         w + 1
            bra         @hdr
@none:
            sec
            rts
@found:
            jsr         hdr_out
            lda         tmp
            clc
            rts

see_xt:     .word       xsquote, xdotq, xcquote, xabortq, do_does, xdo, xqdo, xloop, xploop, dovar, dovalue
            .word       domarker, 0
see_text:   .word       s_squote, s_dotq, s_cquote, s_abortq, s_does, s_do, s_qdo, s_loop, s_ploop, s_create
            .word       s_value, s_marker
see_after:  .byte       $80, $80, $80, $80, 3, 0, 5, 9, 9, $7F, $7F, $7F
see_lit:    .byte       OP_DEX, OP_LDA_IMM, 0, OP_STA_ZPX, dlo, OP_LDA_IMM, 0, OP_STA_ZPX, dhi
see_if:     .byte       OP_INX, OP_LDA_ZPX, dlo - 1, OP_ORA_ZPX, dhi - 1, OP_BNE, 3
s_squote:   .byte       "S", $22, 0
s_dotq:     .byte       ".", $22, 0
s_cquote:   .byte       "C", $22, 0
s_abortq:   .byte       "ABORT", $22, 0
s_does:     .byte       "DOES>", 0
s_do:       .byte       "DO", 0
s_qdo:      .byte       "?DO", 0
s_loop:     .byte       "LOOP", 0
s_ploop:    .byte       "+LOOP", 0
s_create:   .byte       "CREATE", 0
s_value:    .byte       "VALUE", 0
s_marker:   .byte       "MARKER", 0
s_exit:     .byte       "EXIT ", 0
s_immed:    .byte       " IMMEDIATE", 0
s_jmp:      .byte       "jmp ", 0
s_branch:   .byte       "branch ", 0
s_qbranch:  .byte       "?branch ", 0

; ---- Conditional compiling: the words skipped are parsed (refilling, from a file), so a [THEN] in a comment counts

            HEADER      "[IF]", F_IMMEDIATE
bif:                                                        ; ( flag -- ): false, the words skipped to its [ELSE]
            lda         dlo,x                               ;   or [THEN]
            ora         dhi,x
            inx
            cmp         #0
            bne         @done
            lda         #1
            bra         skip_cond
@done:
            rts

            HEADER      "[ELSE]", F_IMMEDIATE
belse:                                                      ; The words skipped to its [THEN]
            lda         #0

; The words skipped to the [THEN] (or with .A = 1, the [ELSE]) at this depth
skip_cond:
            sta         cond_else
            lda         #1
            sta         cond_lvl
@word:
            jsr         parse_name
            lda         dlo,x
            ora         dhi,x
            bne         @have
            inx                                             ; (The line's end: the next, if there is one)
            inx
            lda         src_id + 1
            bmi         @done
            jsr         refill_src
            bcc         @word
@done:
            rts
@have:
            LDR         w3, s_bif
            jsr         name_eq
            bne         :+
            inc         cond_lvl
            bra         @next
:
            LDR         w3, s_belse
            jsr         name_eq
            bne         :+
            lda         cond_lvl
            cmp         #1
            bne         @next
            lda         cond_else
            bne         @end
            bra         @next
:
            LDR         w3, s_bthen
            jsr         name_eq
            bne         @next
            dec         cond_lvl
            beq         @end
@next:
            inx
            inx
            bra         @word
@end:
            inx
            inx
            rts

            HEADER      "[THEN]", F_IMMEDIATE
bthen:
            rts

; Is the name ( c-addr u, kept) the counted string at w3 (upper case), in either case?  OUT: Z = 1 it is
name_eq:
            lda         dhi,x
            bne         @no
            lda         dlo,x
            cmp         (w3)
            bne         @no
            sta         cnt
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0
@char:
            cpy         cnt
            beq         @yes
            lda         (w),y
            jsr         upper
            iny
            cmp         (w3),y
            bne         @no
            bra         @char
@yes:
            lda         #0
            rts
@no:
            lda         #1
            rts

s_bif:      .byte       4, "[IF]"
s_belse:    .byte       6, "[ELSE]"
s_bthen:    .byte       6, "[THEN]"

            HEADER      "[DEFINED]", F_IMMEDIATE
bdefined:                                                   ; ( "name" -- flag ): in the search order
            jsr         parse_name
            jsr         find_name
            inx
            bcc         :+
            jmp         zero_tos
:
            jmp         true_tos

            HEADER      "[UNDEFINED]", F_IMMEDIATE
bundefined:
            jsr         bdefined
            jmp         zequal

; ---- The control-flow stack (the data stack: an orig or a dest a cell) and the return stack

            HEADER      "CS-PICK", 0
cspick:
            jmp         pick

            HEADER      "CS-ROLL", 0
csroll:
            jmp         roll

            HEADER      "N>R", 0
ntor:                                                       ; ( i*x n -- ) R: ( -- i*x n )
            pla
            sta         tmp
            pla
            sta         tmp + 1
            lda         dlo,x
            sta         tmp2
            sta         cnt
            lda         dhi,x
            sta         tmp2 + 1
            inx
:
            lda         cnt
            beq         :+
            lda         dhi,x
            pha
            lda         dlo,x
            pha
            inx
            dec         cnt
            bra         :-
:
            lda         tmp2 + 1
            pha
            lda         tmp2
            pha
            lda         tmp + 1
            pha
            lda         tmp
            pha
            rts

            HEADER      "NR>", 0
nrfrom:                                                     ; ( -- i*x n ) R: ( i*x n -- )
            pla
            sta         tmp
            pla
            sta         tmp + 1
            pla
            sta         tmp2
            sta         cnt
            pla
            sta         tmp2 + 1
:
            lda         cnt
            beq         :+
            dex
            pla
            sta         dlo,x
            pla
            sta         dhi,x
            dec         cnt
            bra         :-
:
            lda         tmp2
            ldy         tmp2 + 1
            PUSHAY
            lda         tmp + 1
            pha
            lda         tmp
            pha
            rts

; ---- Names

            HEADER      "SYNONYM", 0
synonym:                                                    ; ( "newname" "oldname" -- ): newname as oldname is (its
            lda         #F_HIDDEN                           ;   code copied if it's inline; else a jmp to it)
            jsr         make_hdr
            jsr         name_hdr
            jsr         hdr_xt
            lda         lasthdr
            sta         w
            lda         lasthdr + 1
            sta         w + 1
            ldy         #2
            lda         cnt
            and         #F_IMMEDIATE | F_INLINE
            ora         (w),y
            sta         (w),y
            lda         cnt
            and         #F_INLINE
            beq         @jmp
            lda         tmp                                 ; Its code's length, its code and the rts after it
            jsr         ccomma_a
            ldy         #0
:
            lda         (w2),y
            jsr         ccomma_a
            iny
            cpy         tmp
            bne         :-
            lda         #RTS_OP
            jsr         ccomma_a
            jmp         reveal
@jmp:
            lda         w2
            ldy         w2 + 1
            jsr         comp_jmp
            jmp         reveal

            HEADER      "TRAVERSE-WORDLIST", 0
traversewordlist:                                           ; ( i*x xt wid -- j*x ): xt ( k*x nt -- l*x flag ) for
            lda         dlo,x                               ;   each word in it, newest first, till a false flag
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            lda         dhi,x                               ; (The xt, and the next nt: on the return stack while
            pha                                             ;   the xt runs)
            lda         dlo,x
            pha
            inx
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            pha
@nt:
            pla
            sta         w
            pla
            sta         w + 1
            ora         w
            beq         @done
            ldy         #1
            lda         (w),y
            pha
            lda         (w)
            pha
            ldy         #2
            lda         (w),y
            and         #F_HIDDEN
            bne         @nt
            lda         w
            ldy         w + 1
            PUSHAY
            stx         xsave
            tsx
            lda         $0103,x
            sta         w
            lda         $0104,x
            sta         w + 1
            ldx         xsave
            jsr         exec_w
            lda         dlo,x
            ora         dhi,x
            inx
            cmp         #0
            bne         @nt
            pla
            pla
@done:
            pla
            pla
            rts

            HEADER      "NAME>STRING", 0
nametostring:                                               ; ( nt -- c-addr u )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            ldy         #2
            lda         (w),y
            and         #LEN_MASK
            pha
            clc
            lda         w
            adc         #3
            sta         dlo,x
            bcc         :+
            inc         dhi,x
:
            pla
            ldy         #0
            PUSHAY
            rts

            HEADER      "NAME>INTERPRET", 0
nametointerpret:                                            ; ( nt -- xt )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            jsr         hdr_xt
            lda         w2
            sta         dlo,x
            lda         w2 + 1
            sta         dhi,x
            rts

            HEADER      "NAME>COMPILE", 0
nametocompile:                                              ; ( nt -- xt xt2 ): xt2 COMPILE, (or, immediate, EXECUTE)
            jsr         nametointerpret
            lda         #<compilecomma
            ldy         #>compilecomma
            bit         cnt
            bpl         :+
            lda         #<execute
            ldy         #>execute
:
            PUSHAY
            rts
