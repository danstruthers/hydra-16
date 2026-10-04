; ****************************************************************************
; fcomp.inc - forth's compiler: headers, definitions, control flow and the defining words (included by forth.s).
;   A definition is 65C02 code: `jsr xt` for each word (or an F_INLINE word's code itself), a literal as
; `dex; lda #lo; sta dlo,x; lda #hi; sta dhi,x`, IF and UNTIL as `inx; lda dlo-1,x; ora dhi-1,x; bne +3; jmp target`,
; ELSE, AGAIN and REPEAT as `jmp`, ; as `rts`.  DO's parameters are on the return stack: the limit, then the index
; less it, with its sign bit flipped (so +LOOP's crossing of the limit is an overflow); LOOP is `jsr xloop; bvs +3;
; jmp body` and the four `pla`s of UNLOOP after it, where LEAVE goes.  The control-flow stack is the data stack.

; ---- Laying code down

; jsr .A/.Y
comp_jsr:
            pha
            lda         #JSR_OP
            jsr         ccomma_a
            pla
            jmp         comma_ay

; jmp .A/.Y
comp_jmp:
            pha
            lda         #JMP_OP
            jsr         ccomma_a
            pla
            jmp         comma_ay

; A literal: .A/.Y pushed
comp_lit:
            pha
            phy
            lda         #OP_DEX
            jsr         ccomma_a
            lda         #OP_LDA_IMM
            jsr         ccomma_a
            ply
            pla
            phy
            jsr         ccomma_a
            lda         #OP_STA_ZPX
            jsr         ccomma_a
            lda         #dlo
            jsr         ccomma_a
            lda         #OP_LDA_IMM
            jsr         ccomma_a
            pla
            jsr         ccomma_a
            lda         #OP_STA_ZPX
            jsr         ccomma_a
            lda         #dhi
            jmp         ccomma_a

; The top's test, then jmp: the jmp's address pushed (an orig, or for a dest: resolve it)
comp_test:
            lda         #OP_INX
            jsr         ccomma_a
            lda         #OP_LDA_ZPX
            jsr         ccomma_a
            lda         #dlo - 1
            jsr         ccomma_a
            lda         #OP_ORA_ZPX
            jsr         ccomma_a
            lda         #dhi - 1
            jsr         ccomma_a
            lda         #OP_BNE
            jsr         ccomma_a
            lda         #3
            jsr         ccomma_a
; jmp, its address to come: pushes the address's place (an orig)
comp_fwd:
            lda         #JMP_OP
            jsr         ccomma_a
            jsr         here_
            lda         #0
            tay
            jmp         comma_ay

; Resolve an orig (on top): its jmp to HERE
resolve:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            lda         here
            sta         (w)
            ldy         #1
            lda         here + 1
            sta         (w),y
            rts

; The word whose header is at w, compiled: its code copied (F_INLINE), or a jsr to it
comp_hdr:
            jsr         hdr_xt
            bit         cnt                                 ; (hdr_xt: cnt its flags)
            bvs         @call                               ; (F_HIDDEN: as called)
            lda         cnt
            and         #F_INLINE
            beq         @call
            ldy         #0                                  ; Its code, but its rts
:
            cpy         tmp
            beq         @done
            lda         (w2),y
            jsr         ccomma_a
            iny
            bra         :-
@done:
            rts
@call:
            lda         w2
            ldy         w2 + 1
            jmp         comp_jsr

; The header at w: w2 its xt, cnt its flags and length, tmp its code's length (F_INLINE), w kept
hdr_xt:
            ldy         #2
            lda         (w),y
            sta         cnt
            and         #LEN_MASK
            clc
            adc         #3
            tay
            lda         cnt
            and         #F_INLINE
            beq         :+
            lda         (w),y                               ; (Its code's length)
            sta         tmp
            iny
:
            tya
            clc
            adc         w
            sta         w2
            lda         w + 1
            adc         #0
            sta         w2 + 1
            rts

; ---- Headers and definitions

; A header for the next name in the input, linked into the compilation word list (hidden with .A = F_HIDDEN):
; lasthdr, lastxt.  An empty name: THROW -16
make_hdr:
            sta         tmp3
            jsr         parse_name
            lda         dlo,x
            ora         dhi,x
            bne         :+
            lda         #<-16
            jmp         throw_a
:
            lda         dlo,x                               ; (31 chars at most)
            ldy         dhi,x
            bne         :+
            cmp         #LEN_MASK + 1
            bcc         :++
:
            lda         #LEN_MASK
            sta         dlo,x
:
            lda         here                                ; Its header
            sta         lasthdr
            lda         here + 1
            sta         lasthdr + 1
            jsr         current_w3                          ; (Its link: the word list's last)
            ldy         #1
            lda         (w3),y
            tay
            lda         (w3)
            jsr         comma_ay
            lda         dlo,x
            ora         tmp3
            jsr         ccomma_a
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            lda         dlo,x
            sta         cnt
            ldy         #0
:
            cpy         cnt
            beq         :+
            lda         (w),y
            jsr         ccomma_a
            iny
            bra         :-
:
            inx
            inx
            jsr         current_w3                          ; In the list
            lda         lasthdr
            sta         (w3)
            ldy         #1
            lda         lasthdr + 1
            sta         (w3),y
            lda         here
            sta         lastxt
            lda         here + 1
            sta         lastxt + 1
            rts

; w3 = the compilation word list
current_w3:
            lda         current
            sta         w3
            lda         current + 1
            sta         w3 + 1
            rts

; lasthdr shown (F_HIDDEN off)
reveal:
            lda         lasthdr
            ora         lasthdr + 1
            beq         @done
            lda         lasthdr
            sta         w
            lda         lasthdr + 1
            sta         w + 1
            ldy         #2
            lda         (w),y
            and         #<~F_HIDDEN
            sta         (w),y
@done:
            rts

            HEADER      ":", 0
colon:
            lda         #F_HIDDEN
            jsr         make_hdr
            jmp         rbracket

            HEADER      ";", F_IMMEDIATE
semicolon:
            lda         #RTS_OP
            jsr         ccomma_a
            jsr         reveal
            jmp         lbracket

            HEADER      "IMMEDIATE", 0
immediate:
            lda         lasthdr
            sta         w
            lda         lasthdr + 1
            sta         w + 1
            ldy         #2
            lda         (w),y
            ora         #F_IMMEDIATE
            sta         (w),y
            rts

            HEADER      "RECURSE", F_IMMEDIATE
recurse:
            lda         lastxt
            ldy         lastxt + 1
            jmp         comp_jsr

            HEADER      "[", F_IMMEDIATE
lbracket:
            stz         state
            stz         state + 1
            rts

            HEADER      "]", 0
rbracket:
            lda         #$FF
            sta         state
            sta         state + 1
            rts

            HEADER      "LITERAL", F_IMMEDIATE
literal:
            lda         dlo,x
            ldy         dhi,x
            inx
            jmp         comp_lit

; COMPILE,'s (the Core Extension library's header)
compilecomma:                                               ; ( xt -- ): its header's way, if it has one
            jsr         hdr_of_xt
            bcs         :+
            inx
            jmp         comp_hdr
:
            lda         dlo,x
            ldy         dhi,x
            inx
            jmp         comp_jsr

; The header in FORTH whose xt is the top: C = 0, w it; or C = 1 (a :NONAME's, another word list's)
hdr_of_xt:
            lda         forth_wl
            sta         w
            lda         forth_wl + 1
            sta         w + 1
@hdr:
            lda         w
            ora         w + 1
            beq         @none
            jsr         hdr_xt
            lda         w2
            cmp         dlo,x
            bne         @next
            lda         w2 + 1
            cmp         dhi,x
            bne         @next
            clc
            rts
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

; The next name's header: w (not found: THROW -13)
name_hdr:
            jsr         parse_name
            jsr         find_name
            bcc         :+
            jmp         throw_undef
:
            inx
            inx
            rts

            HEADER      "'", 0
tick:                                                       ; ( "name" -- xt )
            jsr         name_hdr
            jsr         hdr_xt
            lda         w2
            ldy         w2 + 1
            PUSHAY
            rts

            HEADER      "[']", F_IMMEDIATE
brackettick:
            jsr         tick
            jmp         literal

            HEADER      "[CHAR]", F_IMMEDIATE
bracketchar:
            jsr         char
            jmp         literal

            HEADER      "POSTPONE", F_IMMEDIATE
postpone:                                                   ; Immediate: compiled; else, code that compiles it
            jsr         name_hdr
            ldy         #2
            lda         (w),y
            bmi         @now                                ; (F_IMMEDIATE)
            lda         w
            ldy         w + 1
            jsr         comp_lit
            lda         #<comp_hdr_rt
            ldy         #>comp_hdr_rt
            jmp         comp_jsr
@now:
            jmp         comp_hdr

; ( hdr -- ): POSTPONE's, as the definition runs: the word compiled
comp_hdr_rt:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            jmp         comp_hdr

; ---- Control flow

            HEADER      "IF", F_IMMEDIATE
if_:
            jmp         comp_test

            HEADER      "THEN", F_IMMEDIATE
then:
            jmp         resolve

            HEADER      "ELSE", F_IMMEDIATE
else_:                                                      ; ( orig1 -- orig2 )
            jsr         comp_fwd
            jsr         swap
            jmp         resolve

            HEADER      "BEGIN", F_IMMEDIATE
begin:
            jmp         here_

            HEADER      "UNTIL", F_IMMEDIATE
until:                                                      ; ( dest -- )
            jsr         comp_poll
            jsr         comp_test
            jmp         resolve_to                          ; (Its jmp: to dest)

; AGAIN's (the Core Extension library's header), REPEAT's
again:
            jsr         comp_poll
again_jmp:                                                  ; ( dest -- ): jmp dest (LOOP's: xloop polls)
            lda         dlo,x
            ldy         dhi,x
            inx
            jmp         comp_jmp

            HEADER      "WHILE", F_IMMEDIATE
while:                                                      ; ( dest -- orig dest )
            jsr         comp_test
            jmp         swap

            HEADER      "REPEAT", F_IMMEDIATE
repeat:                                                     ; ( orig dest -- )
            jsr         again
            jmp         resolve

; Ctrl-C's poll, in a loop before its branch back: bit intr; bpl +3; jmp intr_throw
comp_poll:
            lda         #OP_BIT_ZP
            jsr         ccomma_a
            lda         #intr
            jsr         ccomma_a
            lda         #OP_BPL
            jsr         ccomma_a
            lda         #3
            jsr         ccomma_a
            lda         #<intr_throw
            ldy         #>intr_throw
            jmp         comp_jmp

; ( dest orig -- ): orig's jmp to dest
resolve_to:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            lda         dlo + 1,x
            sta         (w)
            ldy         #1
            lda         dhi + 1,x
            sta         (w),y
            inx
            inx
            rts

            HEADER      "DO", F_IMMEDIATE
do:
            lda         #<xdo
            ldy         #>xdo
            jsr         comp_jsr
do_rest:                                                    ; ( -- leaves dest ): this loop's LEAVEs from none
            lda         leaves
            ldy         leaves + 1
            PUSHAY
            stz         leaves
            stz         leaves + 1
            jmp         here_

            HEADER      "LEAVE", F_IMMEDIATE
leave:
leave_jmp:                                                  ; jmp, to the loop's end: in the chain of LEAVEs
            lda         #JMP_OP
            jsr         ccomma_a
            lda         here                                ; (Its address's place: the chain's next, then it)
            pha
            lda         here + 1
            pha
            lda         leaves
            ldy         leaves + 1
            jsr         comma_ay
            pla
            sta         leaves + 1
            pla
            sta         leaves
            rts

            HEADER      "LOOP", F_IMMEDIATE
loop:
            lda         #<xloop
            ldy         #>xloop
            bra         loop_end

            HEADER      "+LOOP", F_IMMEDIATE
plusloop:
            lda         #<xploop
            ldy         #>xploop
loop_end:                                                   ; ( leaves dest -- ): jsr .A/.Y; bvs +3; jmp dest;
            jsr         comp_jsr                            ;   the LEAVEs here; UNLOOP
            lda         #OP_BVS
            jsr         ccomma_a
            lda         #3
            jsr         ccomma_a
            jsr         again_jmp
:
            lda         leaves                              ; Each LEAVE's jmp: here
            ora         leaves + 1
            beq         :+
            lda         leaves
            sta         w
            lda         leaves + 1
            sta         w + 1
            ldy         #1
            lda         (w),y
            sta         leaves + 1
            lda         (w)
            sta         leaves
            lda         here
            sta         (w)
            lda         here + 1
            sta         (w),y
            bra         :-
:
            ldy         #4                                  ; UNLOOP
:
            lda         #OP_PLA
            jsr         ccomma_a
            dey
            bne         :-
            lda         dlo,x                               ; The outer loop's LEAVEs again
            sta         leaves
            lda         dhi,x
            sta         leaves + 1
            inx
            rts

; ?DO's: as DO's; then Z = 1 if they're equal (the loop skipped: its LEAVE)
xqdo:
            lda         #1
            sta         cnt
            lda         dlo,x
            cmp         dlo + 1,x
            bne         xdo
            lda         dhi,x
            cmp         dhi + 1,x
            bne         xdo
            stz         cnt
; DO's: ( limit index -- ) R: ( -- limit index-limit^$8000 )
xdo:
            pla
            sta         tmp
            pla
            sta         tmp + 1
            lda         dhi + 1,x
            pha
            lda         dlo + 1,x
            pha
            sec
            lda         dlo,x
            sbc         dlo + 1,x
            tay
            lda         dhi,x
            sbc         dhi + 1,x
            eor         #$80
            pha
            tya
            pha
            inx
            inx
            lda         tmp + 1
            pha
            lda         tmp
            pha
            lda         cnt                                 ; (?DO's flag)
            rts

; LOOP's: the index on one (V = 1: its limit crossed); Ctrl-C first
xloop:
            bit         intr
            bmi         intr_j
            stx         xsave
            tsx
            clc
            lda         $0103,x
            adc         #1
            sta         $0103,x
            lda         $0104,x
            adc         #0
            sta         $0104,x
            ldx         xsave
            rts

; +LOOP's: ( n -- ): the index on n (V = 1: crossed); Ctrl-C first
xploop:
            bit         intr
            bmi         intr_j
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            inx
            stx         xsave
            tsx
            clc
            lda         $0103,x
            adc         tmp
            sta         $0103,x
            lda         $0104,x
            adc         tmp + 1
            sta         $0104,x
            ldx         xsave
            rts

intr_j:
            jmp         intr_throw

            HEADER      "I", 0
i:                                                          ; ( -- index ): the parameters under the return address
            stx         xsave
            tsx
            clc
            lda         $0103,x
            adc         $0105,x
            sta         tmp
            lda         $0104,x
            eor         #$80
            adc         $0106,x
            ldx         xsave
            dex
            sta         dhi,x
            lda         tmp
            sta         dlo,x
            rts

            HEADER      "J", 0
j:                                                          ; ( -- index ): the outer loop's
            stx         xsave
            tsx
            clc
            lda         $0107,x
            adc         $0109,x
            sta         tmp
            lda         $0108,x
            eor         #$80
            adc         $010A,x
            ldx         xsave
            dex
            sta         dhi,x
            lda         tmp
            sta         dlo,x
            rts

; ---- Defining words

; A header for the next name, shown, and jsr .A/.Y after it (CREATE's, VALUE's ...)
make_word:
            pha
            phy
            lda         #0
            jsr         make_hdr
            ply
            pla
            jmp         comp_jsr

            HEADER      "CREATE", 0
create:
            lda         #<dovar
            ldy         #>dovar
            jmp         make_word

; A CREATEd word's: its body's address (its return address + 1)
dovar:
            pla
            clc
            adc         #1
            sta         tmp
            pla
            adc         #0
            tay
            lda         tmp
            PUSHAY
            rts

            HEADER      "VARIABLE", 0
variable:
            jsr         create
            lda         #0
            tay
            jmp         comma_ay

            HEADER      "CONSTANT", 0
constant:                                                   ; ( x "name" -- ): a literal and rts
            lda         #0
            jsr         make_hdr
            jsr         literal
            lda         #RTS_OP
            jmp         ccomma_a

; A VALUE's: the cell after its jsr
dovalue:
            pla
            sta         w
            pla
            sta         w + 1
            ldy         #1
            lda         (w),y
            pha
            iny
            lda         (w),y
            tay
            pla
            PUSHAY
            rts

            HEADER      ">BODY", 0
body_:
            clc
            lda         dlo,x
            adc         #3
            sta         dlo,x
            bcc         :+
            inc         dhi,x
:
            rts

            HEADER      "DOES>", F_IMMEDIATE
does:                                                       ; jsr do_does, then the child's code: jsr dodoes ...
            lda         #<do_does
            ldy         #>do_does
            jsr         comp_jsr
            lda         #<dodoes
            ldy         #>dodoes
            jmp         comp_jsr

; DOES>'s, in the defining word: the last word's jsr to the code after this one's (the child's code's jsr dodoes),
; and the defining word ends
do_does:
            pla
            clc
            adc         #1
            sta         tmp
            pla
            adc         #0
            sta         tmp + 1
            lda         lastxt
            sta         w
            lda         lastxt + 1
            sta         w + 1
            ldy         #1
            lda         tmp
            sta         (w),y
            iny
            lda         tmp + 1
            sta         (w),y
            rts

; The child's: its body pushed (its return address + 1), then the code after the jsr here (this one's + 1)
dodoes:
            pla
            clc
            adc         #1
            sta         w
            pla
            adc         #0
            sta         w + 1
            pla
            clc
            adc         #1
            sta         tmp
            pla
            adc         #0
            tay
            lda         tmp
            PUSHAY
            jmp         (w)

defer_none:
            lda         #<-256
            jmp         throw_a

; A MARKER's: all four as they were, and each word list without the words (and word lists) made since: a header or
; word list in RAM at HERE or after
domarker:
            stx         xsave
            pla
            sta         w
            pla
            sta         w + 1
            ldy         #1
            lda         (w),y
            sta         here
            iny
            lda         (w),y
            sta         here + 1
            iny
            lda         (w),y
            sta         current
            iny
            lda         (w),y
            sta         current + 1
            ldx         #0
:
            iny
            lda         (w),y
            sta         order_n,x
            inx
            cpx         #ORDER_MAX * 2 + 1
            bne         :-
            iny                                             ; The files INCLUDED since: not noted
            lda         (w),y
            sta         incn_len
            iny
            lda         (w),y
            sta         incn_len + 1
            lda         wl_last                             ; The word lists made since: gone
            ldy         wl_last + 1
@wl:
            jsr         since
            bcs         :+
            sta         w
            sty         w + 1
            ldy         #3
            lda         (w),y
            pha
            dey
            lda         (w),y
            ply
            bra         @wl
:
            sta         wl_last
            sty         wl_last + 1
@list:                                                      ; Each one's words made since: gone
            sta         w2
            sty         w2 + 1
            ora         w2 + 1
            beq         @done
            ldy         #1
            lda         (w2),y
            tay
            lda         (w2)
@hdr:
            jsr         since
            bcs         :+
            sta         w
            sty         w + 1
            ldy         #1
            lda         (w),y
            tay
            lda         (w)
            bra         @hdr
:
            sta         (w2)
            tya
            ldy         #1
            sta         (w2),y
            ldy         #3                                  ; The next word list
            lda         (w2),y
            pha
            dey
            lda         (w2),y
            ply
            bra         @list
@done:
            ldx         xsave
            rts

; Is .A/.Y (an address) in RAM at HERE or after (made since)?  OUT: C = 0 yes; or C = 1.  Keeps .A, .Y
since:
            cpy         #>DICT_END                          ; (ROM's, at $8000 and up; or 0)
            bcs         @no
            pha
            cmp         here
            tya
            sbc         here + 1
            pla
            bcc         @no
            clc
            rts
@no:
            sec
            rts
