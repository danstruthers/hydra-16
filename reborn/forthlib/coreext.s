; ****************************************************************************
; coreext.s - HyForth's Core Extension library (/lib/forth/coreext.fl): the Core Extension word set's words (but \,
; the core's, as startup.fs and every library's source use it).  Some are the core's code, which it uses too
; (PARSE-NAME, PICK, ROLL, AGAIN, COMPILE,): their headers here jump to it; and a compiled ?DO's, C"'s, VALUE's, DEFER's
; and MARKER's code calls the core's (xqdo, xcquote, dovalue, defer_none, domarker), so a definition outlives this.

.include "forthlib.inc"

            HEADER      "tuck", 0
tuck:                                                       ; ( a b -- b a b )
            jsr         swap
            jmp         over

            HEADER      "<>", 0
notequal:
            jsr         equal
            jmp         invert

            HEADER      "u>", 0
ugreater:
            jsr         swap
            jmp         uless

            HEADER      "0<>", 0
znotequal:
            lda         dlo,x
            ora         dhi,x
            beq         :+
            jmp         true_tos
:
            jmp         zero_tos

            HEADER      "0>", 0
zgreater:
            lda         dhi,x
            bmi         @no
            ora         dlo,x
            beq         @no
            jmp         true_tos
@no:
            jmp         zero_tos

            HEADER      "within", 0
within:                                                     ; ( n lo hi -- flag ): n - lo U< hi - lo
            sec
            lda         dlo,x
            sbc         dlo + 1,x
            sta         tmp
            lda         dhi,x
            sbc         dhi + 1,x
            sta         tmp + 1
            sec
            lda         dlo + 2,x
            sbc         dlo + 1,x
            sta         dlo + 2,x
            lda         dhi + 2,x
            sbc         dhi + 1,x
            sta         dhi + 2,x
            inx
            lda         tmp
            sta         dlo,x
            lda         tmp + 1
            sta         dhi,x
            jmp         uless

            HEADER      "true", 0
true:
            CONSTCODE   $FFFF

            HEADER      "false", 0
false:
            CONSTCODE   0

            HEADERI     "2>r", twotor, F_COMPILE
twotor:
            lda         dhi + 1,x
            pha
            lda         dlo + 1,x
            pha
            lda         dhi,x
            pha
            lda         dlo,x
            pha
            inx
            inx
twotor_end:
            rts

            HEADERI     "2r>", tworfrom, F_COMPILE
tworfrom:
            dex
            dex
            pla
            sta         dlo,x
            pla
            sta         dhi,x
            pla
            sta         dlo + 1,x
            pla
            sta         dhi + 1,x
tworfrom_end:
            rts

            HEADERI     "2r@", tworfetch, F_COMPILE
tworfetch:
            stx         xsave
            tsx
            lda         $0101,x
            sta         tmp
            lda         $0102,x
            sta         tmp + 1
            lda         $0103,x
            sta         tmp2
            lda         $0104,x
            ldx         xsave
            dex
            dex
            sta         dhi + 1,x
            lda         tmp2
            sta         dlo + 1,x
            lda         tmp
            sta         dlo,x
            lda         tmp + 1
            sta         dhi,x
tworfetch_end:
            rts

            HEADER      "erase", 0
erase:
            dex
            jsr         zero_tos
            jmp         fill

            HEADER      "unused", 0
unused:                                                     ; (To the heap's page: ALLOCATE's, memory.fl's)
            sec
            lda         #0
            sbc         here
            pha
            lda         heap_lo + 1
            sbc         here + 1
            tay
            pla
            PUSHAY
            rts

            HEADER      "pad", 0
pad_:
            lda         #<pad
            ldy         #>pad
            PUSHAY
            rts

            HEADER      "hex", 0
hex:
            lda         #16
            jmp         set_base

            HEADERI     "nip", nip_l
nip_l:
            lda         dlo,x
            sta         dlo + 1,x
            lda         dhi,x
            sta         dhi + 1,x
            inx
nip_l_end:
            rts

            HEADER      "pick", 0
pick_w:                                                     ; ( xu ... x0 u -- xu ... x0 xu )
            jmp         pick

            HEADER      "roll", 0
roll_w:                                                     ; ( xu xu-1 ... x0 u -- xu-1 ... x0 xu )
            jmp         roll

            HEADER      ":noname", 0
noname:                                                     ; ( -- xt )
            stz         lasthdr
            stz         lasthdr + 1
            lda         here
            sta         lastxt
            ldy         here + 1
            sty         lastxt + 1
            PUSHAY
            jsr         code_begin
            jmp         rbracket

            HEADER      "compile,", 0
compilecomma_w:                                             ; ( xt -- ): its header's way, if it has one
            jmp         compilecomma

            HEADERC     "[compile]", F_IMMEDIATE
bracketcompile:
            jsr         name_hdr
            jmp         comp_hdr

            HEADERC     "again", F_IMMEDIATE
again_w:                                                    ; ( dest -- )
            jmp         again

            HEADERC     "?do", F_IMMEDIATE
qdo:
            lda         leaves                              ; (Its skip: a LEAVE of this loop's)
            ldy         leaves + 1
            PUSHAY
            stz         leaves
            stz         leaves + 1
            lda         #<xqdo
            ldy         #>xqdo
            jsr         comp_jsr
            lda         #OP_BNE
            jsr         ccomma_a
            lda         #3
            jsr         ccomma_a
            jsr         leave_jmp
            jmp         here_

            HEADERC     "case", F_IMMEDIATE
case:
            dex                                             ; (0: the ENDOFs' end)
            jmp         zero_tos

            HEADERC     "of", F_IMMEDIATE
of:                                                         ; OVER = IF DROP
            lda         #<over
            ldy         #>over
            jsr         comp_jsr
            lda         #<equal
            ldy         #>equal
            jsr         comp_jsr
            jsr         comp_test
            lda         #<drop
            ldy         #>drop
            jmp         comp_jsr

            HEADERC     "endof", F_IMMEDIATE
endof:
            jmp         else_

            HEADERC     "endcase", F_IMMEDIATE
endcase:                                                    ; DROP, and each ENDOF's jmp here
            lda         #<drop
            ldy         #>drop
            jsr         comp_jsr
:
            lda         dlo,x
            ora         dhi,x
            beq         :+
            jsr         resolve
            bra         :-
:
            inx
            rts

            HEADER      "value", 0
value:                                                      ; ( x "name" -- )
            lda         #<dovalue
            ldy         #>dovalue
            jsr         make_word
            jmp         comma

            HEADER      "to", F_IMMEDIATE
to:                                                         ; ( x "name" -- ): the VALUE's cell; or ( x1 x2 "name"
            jsr         parse_name                          ;   -- ), a 2VALUE's two (its jsr do2value: the
            lda         state                               ;   Double-Number library's), as 2! stores them; or,
            beq         :+                                  ;   compiling, a local (the locals library's: first)
            lda         #3
            jsr         loc_call
            bcs         :+
            rts
:
            jsr         find_name
            bcc         :+
            jmp         throw_undef
:
            inx
            inx
            jsr         hdr_xt
            lda         w2
            ldy         w2 + 1
            PUSHAY
            sta         w
            sty         w + 1
            jsr         body_
            ldy         #1
            lda         (w),y
            cmp         #<do2value
            bne         @one
            iny
            lda         (w),y
            cmp         #>do2value
            bne         @one
            lda         #<twostore
            ldy         #>twostore
            bra         :+
@one:
            lda         #<store
            ldy         #>store
:
            pha
            phy
            lda         state
            beq         @now
            jsr         literal
            ply
            pla
            jmp         comp_jsr
@now:
            ply
            pla
            sta         w
            sty         w + 1
            jmp         (w)

            HEADER      "buffer:", 0
bufferc:                                                    ; ( u "name" -- )
            jsr         create
            jmp         allot

            HEADER      "defer", 0
defer:                                                      ; jmp to its xt (none yet: THROW -256)
            lda         #0
            jsr         make_hdr
            lda         #<defer_none
            ldy         #>defer_none
            jmp         comp_jmp

            HEADER      "defer!", 0
deferstore:                                                 ; ( xt2 xt1 -- )
            jsr         oneplus
            jmp         store

            HEADER      "defer@", 0
deferfetch:
            jsr         oneplus
            jmp         fetch

            HEADER      "is", F_IMMEDIATE
is:
            jsr         tick
            lda         state
            beq         deferstore
            jsr         literal
            lda         #<deferstore
            ldy         #>deferstore
            jmp         comp_jsr

            HEADER      "action-of", F_IMMEDIATE
actionof:
            jsr         tick
            lda         state
            beq         deferfetch
            jsr         literal
            lda         #<deferfetch
            ldy         #>deferfetch
            jmp         comp_jsr

            HEADER      "marker", 0
marker:                                                     ; Its word: HERE, the compilation word list, the search
            lda         here                                ;   order, the files INCLUDED and the code banks' (the
                                                            ;   code's next byte, the banks taken) as before it
            pha
            lda         here + 1
            pha
            lda         #<domarker
            ldy         #>domarker
            jsr         make_word
            pla
            tay
            pla
            jsr         comma_ay                            ; (lo, hi swapped back: comma_ay is .A low)
            lda         current
            ldy         current + 1
            jsr         comma_ay
            ldy         #0
:
            lda         order_n,y                           ; (order_n, then order)
            jsr         ccomma_a
            iny
            cpy         #ORDER_MAX * 2 + 1
            bne         :-
            lda         incn_len                            ; (So REQUIRE loads again what it takes out)
            ldy         incn_len + 1
            jsr         comma_ay
            lda         chere
            ldy         chere + 1
            jsr         comma_ay
            lda         cbanks
            jmp         ccomma_a

            HEADER      "refill", 0
refill:                                                     ; ( -- flag ): a block's, the next block (BLK + 1, the
            lda         src_blk                             ;   Block library's); a string's (EVALUATE) can't
            ora         src_blk + 1
            beq         @src
            inc         src_blk
            bne         :+
            inc         src_blk + 1
:
            stz         to_in
            stz         to_in + 1
            lda         #0
            jsr         blk_call
            bcs         @false
            dex
            jmp         true_tos
@src:
            lda         src_id + 1
            bmi         @false
            jsr         refill_src
            bcs         @false
            dex
            jmp         true_tos
@false:
            dex
            jmp         zero_tos

            HEADER      "parse", 0
parse:                                                      ; ( char "ccc<char>" -- c-addr u )
            lda         dlo,x
            inx
            sta         cnt
            jmp         parse_to

            HEADER      "parse-name", 0
parse_name_w:                                               ; ( "<spaces>name<space>" -- c-addr u )
            jmp         parse_name

            HEADER      "source-id", 0
sourceid:
            lda         src_id
            ldy         src_id + 1
            PUSHAY
            rts

            HEADER      "save-input", 0
saveinput:                                                  ; ( -- blk pos pos-hi line >in id 6 )
            lda         src_blk
            ldy         src_blk + 1
            PUSHAY
            lda         src_pos
            ldy         src_pos + 1
            PUSHAY
            lda         src_pos + 2
            ldy         src_pos + 3
            PUSHAY
            lda         src_line
            ldy         src_line + 1
            PUSHAY
            lda         to_in
            ldy         to_in + 1
            PUSHAY
            lda         src_id
            ldy         src_id + 1
            PUSHAY
            lda         #6
            ldy         #0
            PUSHAY
            rts

            HEADER      "restore-input", 0
restoreinput:                                               ; ( blk pos pos-hi line >in id 6 -- flag ): false if it
            lda         dlo,x                               ;   could: the same source, and its line still in the
            cmp         #6                                  ;   buffer (a file's: read again, from where it was; a
            bne         @fail_n                             ;   block's: that block, as BLOCK has it)
            lda         dhi,x
            bne         @fail_n
            lda         dlo + 1,x
            cmp         src_id
            bne         @fail
            lda         dhi + 1,x
            cmp         src_id + 1
            bne         @fail
            lda         dlo + 6,x                           ; (A block?)
            ora         dhi + 6,x
            beq         @noblk
            lda         src_blk
            ora         src_blk + 1
            beq         @fail
            lda         dlo + 6,x
            sta         src_blk
            lda         dhi + 6,x
            sta         src_blk + 1
            lda         #0
            jsr         blk_call
            bcs         @fail
            bra         @set
@noblk:
            lda         src_blk
            ora         src_blk + 1
            bne         @fail
            lda         src_id                              ; (A file?)
            ora         src_id + 1
            beq         @line
            cmp         #$FF
            beq         @line
            jsr         ri_file
            bcs         @fail
            bra         @set
@line:
            lda         dlo + 3,x
            cmp         src_line
            bne         @fail
            lda         dhi + 3,x
            cmp         src_line + 1
            bne         @fail
@set:
            lda         dlo + 3,x
            sta         src_line
            lda         dhi + 3,x
            sta         src_line + 1
            lda         dlo + 2,x
            sta         to_in
            lda         dhi + 2,x
            sta         to_in + 1
            txa
            clc
            adc         #6
            tax
            jmp         zero_tos
@fail_n:
            lda         dlo,x
            bra         :+
@fail:
            lda         #6
:
            stx         xsave
            clc
            adc         xsave
            tax
            jmp         true_tos

; RESTORE-INPUT's, for a file: its line, unless it's the one SAVE-INPUT's place says, read again from there.
; OUT: C = 1 if it can't be
ri_file:
            lda         dlo + 5,x
            cmp         src_pos
            bne         @seek
            lda         dhi + 5,x
            cmp         src_pos + 1
            bne         @seek
            lda         dlo + 4,x
            cmp         src_pos + 2
            bne         @seek
            lda         dhi + 4,x
            cmp         src_pos + 3
            bne         @seek
            clc
            rts
@seek:
            lda         dlo + 5,x
            sta         src_pos
            sta         r0
            lda         dhi + 5,x
            sta         src_pos + 1
            sta         r0 + 1
            lda         dlo + 4,x
            sta         src_pos + 2
            sta         r1
            lda         dhi + 4,x
            sta         src_pos + 3
            sta         r1 + 1
            stz         src_cons
            stz         src_cons + 1
            stx         xsave
            lda         src_id
            ldx         #0
            jsr         SEEK
            ldx         xsave
            bcs         @done
            jmp         refill_file
@done:
            rts

            HEADER      ".(", F_IMMEDIATE
dotparen:
            lda         #')'
            sta         cnt
            jsr         parse_to
            jmp         type

            HEADER      "holds", 0
holds:                                                      ; ( addr u -- )
:
            lda         dlo,x
            ora         dhi,x
            beq         @done
            jsr         oneminus
            clc
            lda         dlo + 1,x
            adc         dlo,x
            sta         w
            lda         dhi + 1,x
            adc         dhi,x
            sta         w + 1
            lda         (w)
            jsr         hold_a
            bra         :-
@done:
            inx
            inx
            rts

; ( addr u w -- ): spaces to w, then the text
right:
            sec
            lda         dlo,x
            sbc         dlo + 1,x
            sta         dlo,x
            lda         dhi,x
            sbc         dhi + 1,x
            sta         dhi,x
            jsr         spaces
            jmp         type

            HEADER      ".r", 0
dotr:                                                       ; ( n w -- )
            jsr         save_top
            jsr         n_text
            jsr         push_tmp3
            bra         right

            HEADER      "u.r", 0
udotr:
            jsr         save_top
            jsr         u_text
            jsr         push_tmp3
            bra         right

            HEADERQC    "c", F_IMMEDIATE
cquote:
            lda         #'"'
            sta         cnt
            jsr         parse_to
            lda         #<xcquote
            ldy         #>xcquote
            jmp         comp_str

            HEADERQ     "s\", F_IMMEDIATE
sbquote:                                                    ; S" with escapes: \a \b \e \f \l \m \n \q \r \t \v \z
            jsr         src_rest                            ;   \" \\ \xHH (into wbuf, then as S")
            lda         #<wbuf
            sta         w2
            lda         #>wbuf
            sta         w2 + 1
            stz         cnt                                 ; (cnt: the bytes in wbuf)
@char:
            jsr         @get
            bcs         @end
            cmp         #'"'
            beq         @end
            cmp         #'\'
            bne         @put
            jsr         @get
            bcs         @end
            ldy         #ESC_N - 1
:
            cmp         esc_from,y
            beq         @esc
            dey
            bpl         :-
            cmp         #'x'
            beq         @hex
            cmp         #'m'
            bne         @put
            lda         #$0D                                ; (\m: CR LF)
            jsr         @store
            lda         #LF
            bra         @put
@esc:
            lda         esc_to,y
@put:
            jsr         @store
            bra         @char
@hex:
            jsr         @get
            jsr         @nibble
            asl
            asl
            asl
            asl
            sta         tmp2
            jsr         @get
            jsr         @nibble
            ora         tmp2
            bra         @put
@end:
            dex
            lda         #<wbuf
            sta         dlo,x
            lda         #>wbuf
            sta         dhi,x
            dex
            lda         cnt
            sta         dlo,x
            stz         dhi,x
            jmp         str_comp

@get:                                                       ; The source's next char (C = 1: none), >IN past it
            lda         tmp
            ora         tmp + 1
            beq         @none
            lda         (w)
            pha
            jsr         in_next
            inc         w
            bne         :+
            inc         w + 1
:
            lda         tmp
            bne         :+
            dec         tmp + 1
:
            dec         tmp
            pla
            clc
            rts
@none:
            sec
            rts

@store:
            ldy         cnt
            cpy         #255
            bcs         :+
            sta         (w2),y
            inc         cnt
:
            rts

@nibble:
            phx                                             ; (A hex digit, any base)
            ldx         base
            ldy         #16
            sty         base
            jsr         digit
            stx         base
            plx
            bcc         :+
            lda         #0
:
            rts

esc_from:   .byte       "abefnqrtvz", $22, $5C, "l"
ESC_N       = * - esc_from
esc_to:     .byte       7, 8, 27, 12, LF, $22, $0D, 9, 11, 0, $22, $5C, LF
