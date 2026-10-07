; ****************************************************************************
; tools.s - HyForth's Programming-Tools library (/lib/forth/tools.fl: require tools.fl): .s ? words dump see, ahead,
; [if] [else] [then] and their kind, the control-flow stack's, the return stack's, synonym, and the name tokens' (an
; nt is a header's address); and the libraries' (the core's libtab): libs, lib, -lib.  bye is the core's.

.include "forthlib.inc"

WD_COL      = 25                                            ; WORDS: a word's place on the line, at least

.bss
wd_pos:     .res        1                                   ; WORDS: where the line has got to ...
wd_next:    .res        1                                   ;   where the next word goes on it ...
wd_width:   .res        1                                   ;   and its width ($COLUMNS, else 80)
wd_i:       .res        1                                   ; A libtab record's number (the next one's)
wd_buf:     .res        4                                   ; $COLUMNS's value
lib_nm:     .res        LR_NAME_MAX + 4                     ; LIB's file's name: the name, and .fl or .fs
lib_inc:    .res        2                                   ; LIB: INCLUDED's names' bytes, before it tried NAME.fl
lib_last:   .res        2                                   ; LIB, a .fs: the compilation word list's last before ...
lib_wid:    .res        2                                   ;   and it
.code

            HEADERC     "ahead", F_IMMEDIATE
ahead:
            jmp         comp_fwd

            HEADER      "?", 0
question:
            jsr         fetch
            jmp         dot

            HEADER      ".s", 0
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

; ---- WORDS

            HEADER      "words", 0
words:                                                      ; The first word list in the order, newest first: each
            jsr         lib_prune                           ;   word's xt in hex, its kind (l a literal, i immediate,
            jsr         columns                             ;   a or f assembly or Forth) and its name, as many to a
            stz         wd_pos                              ;   line as the screen's width has room for
            stz         wd_next
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
            bit         intr                                ; (Ctrl-C)
            bvc         :+
            jmp         intr_throw
:
            ldy         #2
            lda         (w),y
            and         #F_HIDDEN
            bne         @next
            jsr         word_out
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

; Header w out: " xxxx lif name" (its xt, its kind), at wd_next (spaces to it), or on the next line if it wouldn't
; fit on this one
word_out:
            ldy         #2                                  ; (tmp3: its width, " xxxx lif " and its name)
            lda         (w),y
            and         #LEN_MASK
            clc
            adc         #10
            sta         tmp3
            lda         wd_pos                              ; Room on the line?
            beq         @out
            clc
            lda         wd_next
            adc         tmp3
            bcs         @line
            cmp         wd_width
            bcc         @pad
@line:
            jsr         cr
            stz         wd_pos
            stz         wd_next
            bra         @out
@pad:
            lda         wd_pos
            cmp         wd_next
            bcs         @out
            jsr         space
            inc         wd_pos
            bra         @pad
@out:
            jsr         hdr_xt                              ; Its xt (w2; cnt its flags)
            jsr         space
            lda         w2 + 1
            jsr         hex2
            lda         w2
            jsr         hex2
            jsr         space
            jsr         word_lit                            ; l: a literal
            lda         #'-'
            bcs         :+
            lda         #'l'
:
            jsr         emit_a
            lda         #'-'                                ; i: immediate
            bit         cnt
            bpl         :+
            lda         #'i'
:
            jsr         emit_a
            jsr         hdr_asm                             ; a or f: assembly or Forth
            lda         #'f'
            bcc         :+
            lda         #'a'
:
            jsr         emit_a
            jsr         space
            jsr         hdr_out                             ; Its name (and a space)
            sec                                             ; Where the line's got to, and where the next goes: at a
            lda         wd_next                             ;   multiple of WD_COL
            adc         tmp3
            sta         wd_pos
            sta         tmp3
            lda         #0
:
            cmp         tmp3
            bcs         :+
            adc         #WD_COL
            bcc         :-
            lda         #$FF
:
            sta         wd_next
            rts

; Is the word whose xt is w2 a literal: its code a literal (lit_at) and rts, or two (a 2CONSTANT's) and rts?  OUT:
; C = 0 yes
word_lit:
            lda         w2
            sta         w3
            lda         w2 + 1
            sta         w3 + 1
            jsr         lit_at
            bcs         @no
            jsr         @past
            lda         (w3)
            cmp         #RTS_OP
            beq         @yes
            jsr         lit_at
            bcs         @no
            jsr         @past
            lda         (w3)
            cmp         #RTS_OP
            beq         @yes
@no:
            sec
            rts
@yes:
            clc
            rts
@past:
            clc
            lda         w3
            adc         #9
            sta         w3
            bcc         :+
            inc         w3 + 1
:
            rts

; Is the code at w3 a literal's (dex, lda #lo, sta dlo,x, lda #hi, sta dhi,x: see_lit, its bytes 2 and 6 the
; number's)?  OUT: C = 0 yes
lit_at:
            ldy         #8
@byte:
            cpy         #2
            beq         @next
            cpy         #6
            beq         @next
            lda         (w3),y
            cmp         see_lit,y
            bne         @no
@next:
            dey
            bpl         @byte
            clc
            rts
@no:
            sec
            rts

; Is header w assembly: the core's (in its ROM, from $A000), or in a library's image (not a .fs's: libtab's)?  OUT:
; C = 1 yes.  Keeps .X
hdr_asm:
            lda         w + 1
            cmp         #$A0
            bcs         @yes
            stz         wd_i
@rec:
            lda         wd_i
            cmp         libs_n
            bcs         @no
            jsr         lib_rec
            inc         wd_i
            ldy         #LR_FLAGS
            lda         (w3),y
            and         #LRF_SOURCE
            bne         @rec
            lda         w                                   ; (Its image's start <= w ...
            cmp         (w3)
            ldy         #LR_START + 1
            lda         w + 1
            sbc         (w3),y
            bcc         @rec
            ldy         #LR_END                             ;   < its end)
            lda         w
            cmp         (w3),y
            iny
            lda         w + 1
            sbc         (w3),y
            bcs         @rec
@yes:
            sec
            rts
@no:
            clc
            rts

; wd_width: the screen's width, $COLUMNS, else 80
columns:
            lda         #80
            sta         wd_width
            LDR         r0, s_columns
            LDR         r1, wd_buf
            LDR         r2, 4
            stz         r3
            stz         r3 + 1
            lda         #$FF
            stx         xsave
            jsr         ENV_GET
            sta         tmp                                 ; (Its length)
            ldx         xsave
            bcs         @done
            stz         tmp + 1                             ; (Its number, in decimal: 3 digits at most)
            ldy         #0
@digit:
            cpy         tmp
            beq         @got
            cpy         #3
            beq         @got
            lda         wd_buf,y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @got
            pha
            lda         tmp + 1
            asl
            asl
            clc
            adc         tmp + 1
            asl
            sta         tmp + 1
            pla
            clc
            adc         tmp + 1
            sta         tmp + 1
            iny
            bra         @digit
@got:
            lda         tmp + 1
            beq         @done
            sta         wd_width
@done:
            rts

s_columns:  .byte       "COLUMNS", 0

; ---- The libraries (libtab, the core's: each library loaded, as INCLUDED or LIB loaded it)

            HEADER      "libs", 0
libs:                                                       ; ( -- ): the libraries loaded, the core (forth) first, then
            jsr         lib_prune                           ;   the oldest first; (name): one not searched (-lib)
            LDR         w, s_core
            jsr         type_z
            stz         wd_i
@rec:
            lda         wd_i
            cmp         libs_n
            bcs         @done
            jsr         lib_rec
            inc         wd_i
            jsr         space
            ldy         #LR_FLAGS
            lda         (w3),y
            bpl         :+
            lda         #'('
            jsr         emit_a
:
            ldy         #LR_NAME
            lda         (w3),y
            sta         tmp
@char:
            lda         tmp
            beq         :+
            dec         tmp
            iny
            lda         (w3),y
            jsr         emit_a
            bra         @char
:
            ldy         #LR_FLAGS
            lda         (w3),y
            bpl         @rec
            lda         #')'
            jsr         emit_a
            bra         @rec
@done:
            jmp         cr

s_core:     .byte       "forth", 0

            HEADER      "lib", 0
lib:                                                        ; ( "name" -- ): library name searched again (after
            jsr         parse_name                          ;   -lib), or else loaded: name.fl, or if there's none,
            jsr         lib_prune                           ;   name.fs, by REQUIRED (so /lib/forth's)
            jsr         lib_find
            bcs         @load
            inx
            inx
            ldy         #LR_FLAGS
            lda         (w3),y
            bpl         @done
            jmp         lib_relink
@done:
            rts
@load:
            lda         dhi,x                               ; (Too long a name: none of its)
            bne         @long
            lda         dlo,x
            beq         @long
            cmp         #LR_NAME_MAX + 1
            bcc         :+
@long:
            jmp         lib_unknown
:
            sta         lib_nm                              ; Its file's name, in lib_nm: name.fl
            lda         dlo + 1,x
            sta         p1
            lda         dhi + 1,x
            sta         p1 + 1
            inx
            inx
            ldy         #0
:
            lda         (p1),y
            sta         lib_nm + 1,y
            iny
            cpy         lib_nm
            bne         :-
            lda         #'.'
            sta         lib_nm + 1,y
            lda         #'f'
            sta         lib_nm + 2,y
            lda         #'l'
            sta         lib_nm + 3,y
            lda         incn_len                            ; REQUIRED name.fl, caught
            sta         lib_inc
            lda         incn_len + 1
            sta         lib_inc + 1
            jsr         lib_file
            lda         #<required
            ldy         #>required
            PUSHAY
            jsr         catch
            lda         dlo,x
            ora         dhi,x
            bne         :+
            inx
            rts
:
            lda         dlo,x                               ; Not there: name.fs, not name.fl noted as INCLUDED
            cmp         #<(-512 - E_NOENT)
            bne         @throw
            lda         dhi,x
            cmp         #>(-512 - E_NOENT)
            beq         :+
@throw:
            jmp         throw
:
            inx
            inx
            inx
            lda         lib_inc
            sta         incn_len
            lda         lib_inc + 1
            sta         incn_len + 1
            ldy         lib_nm
            lda         #'s'
            sta         lib_nm + 3,y
            jsr         current_w3                          ; (The compilation word list and its last, before)
            lda         w3
            sta         lib_wid
            lda         w3 + 1
            sta         lib_wid + 1
            lda         (w3)
            sta         lib_last
            ldy         #1
            lda         (w3),y
            sta         lib_last + 1
            lda         here                                ; (Its image's start)
            pha
            lda         here + 1
            pha
            jsr         lib_file
            jsr         required
            pla                                             ; Noted: what it compiled, its headers (if they're in the
            sta         tmp2 + 1                            ;   word list they started in)
            pla
            sta         tmp2
            lda         here
            sta         w2
            lda         here + 1
            sta         w2 + 1
            stz         p1
            stz         p1 + 1
            jsr         current_w3
            lda         w3
            cmp         lib_wid
            bne         @note
            lda         w3 + 1
            cmp         lib_wid + 1
            bne         @note
            lda         (w3)                                ; (p2 its last header; p1 its first, the one whose link is
            sta         p2                                  ;   the word list's last before: none if it's that)
            ldy         #1
            lda         (w3),y
            sta         p2 + 1
            lda         p2
            sta         w3
            lda         p2 + 1
            sta         w3 + 1
@first:
            lda         w3
            cmp         lib_last
            bne         :+
            lda         w3 + 1
            cmp         lib_last + 1
            beq         @note
:
            lda         w3
            ora         w3 + 1
            beq         @note
            lda         w3
            sta         p1
            lda         w3 + 1
            sta         p1 + 1
            ldy         #1
            lda         (w3),y
            pha
            lda         (w3)
            sta         w3
            pla
            sta         w3 + 1
            bra         @first
@note:
            LDR         w, lib_nm
            lda         #LRF_SOURCE
            jmp         lib_add

; ( -- c-addr u ): lib_nm, the file LIB loads
lib_file:
            lda         #<(lib_nm + 1)
            ldy         #>(lib_nm + 1)
            PUSHAY
            clc
            lda         lib_nm
            adc         #3
            ldy         #0
            PUSHAY
            rts

            HEADER      "-lib", 0
unlib:                                                      ; ( "name" -- ): library name's words not searched: its
            jsr         parse_name                          ;   headers out of their word list (lib puts them back),
            jsr         lib_prune                           ;   its code kept (what uses it still runs); this library
            jsr         lib_find                            ;   can't be (-21: LIB would be gone)
            bcs         lib_unknown
            inx
            inx
            ldy         #LR_FLAGS
            lda         (w3),y
            bmi         @done
            lda         #<unlib                             ; (This library's)
            cmp         (w3)
            ldy         #LR_START + 1
            lda         #>unlib
            sbc         (w3),y
            bcc         @other
            ldy         #LR_END
            lda         #<unlib
            cmp         (w3),y
            iny
            lda         #>unlib
            sbc         (w3),y
            bcs         @other
            lda         #<-21
            jmp         throw_a
@other:
            ldy         #LR_FIRST + 1                       ; (No headers: only noted)
            lda         (w3),y
            beq         @hide
            jsr         lib_slot
            bcs         @hide
            ldy         #LR_FIRST                           ; The place that held its last: its first's link
            lda         (w3),y
            sta         p1
            iny
            lda         (w3),y
            sta         p1 + 1
            lda         (p1)
            sta         (w2)
            ldy         #1
            lda         (p1),y
            sta         (w2),y
@hide:
            jsr         idx_drop                            ; (FORTH's index made again: its list changed)
            ldy         #LR_FLAGS
            lda         (w3),y
            ora         #LRF_HIDDEN
            sta         (w3),y
@done:
            rts

; A library LIB or -lib can't find ( c-addr u ): THROW -13, its name's
lib_unknown:
            jmp         throw_undef

; The record of the library named c-addr u (the top two, kept; either case)?  OUT: C = 0, w3 it; or C = 1
lib_find:
            stz         wd_i
            lda         dlo + 1,x
            sta         p1
            lda         dhi + 1,x
            sta         p1 + 1
@rec:
            lda         wd_i
            cmp         libs_n
            bcs         @no
            jsr         lib_rec
            inc         wd_i
            lda         dhi,x
            bne         @no
            ldy         #LR_NAME
            lda         (w3),y
            cmp         dlo,x
            bne         @rec
            sta         tmp + 1
            ldy         #0
@char:
            cpy         tmp + 1
            beq         @yes
            lda         (p1),y
            jsr         upper
            sta         tmp
            iny
            phy
            tya
            clc
            adc         #LR_NAME
            tay
            lda         (w3),y
            jsr         upper
            ply
            cmp         tmp
            beq         @char
            bra         @rec
@yes:
            clc
            rts
@no:
            sec
            rts

; w2 = the place (its word list's last, or a header's link) that holds library w3's last header.  OUT: C = 1: none
lib_slot:
            ldy         #LR_WID
            lda         (w3),y
            sta         w2
            iny
            lda         (w3),y
            sta         w2 + 1
@look:
            lda         (w2)
            ldy         #LR_LAST
            cmp         (w3),y
            bne         @next
            ldy         #1
            lda         (w2),y
            ldy         #LR_LAST + 1
            cmp         (w3),y
            beq         @found
@next:
            ldy         #1
            lda         (w2),y
            pha
            lda         (w2)
            sta         w2
            pla
            sta         w2 + 1
            ora         w2
            bne         @look
            sec
            rts
@found:
            clc
            rts

; Library w3's headers back in their word list, where they were: after the newest header older than they (a word
; list's headers in RAM are newest first, then the core's in ROM, as a MARKER takes them)
lib_relink:
            ldy         #LR_FIRST + 1                       ; (No headers: only noted)
            lda         (w3),y
            beq         @shown
            ldy         #LR_WID
            lda         (w3),y
            sta         w2
            iny
            lda         (w3),y
            sta         w2 + 1
@look:
            ldy         #1                                  ; The next one newer (in RAM, after its last)?
            lda         (w2),y
            cmp         #$A0
            bcs         @here
            ldy         #LR_LAST + 1
            cmp         (w3),y
            bcc         @here
            bne         @next
            lda         (w2)
            ldy         #LR_LAST
            cmp         (w3),y
            bcc         @here
@next:
            ldy         #1
            lda         (w2),y
            pha
            lda         (w2)
            sta         w2
            pla
            sta         w2 + 1
            bra         @look
@here:
            ldy         #LR_FIRST                           ; Its first's link: what's there; there: its last
            lda         (w3),y
            sta         p1
            iny
            lda         (w3),y
            sta         p1 + 1
            lda         (w2)
            sta         (p1)
            ldy         #1
            lda         (w2),y
            sta         (p1),y
            ldy         #LR_LAST
            lda         (w3),y
            sta         (w2)
            iny
            lda         (w3),y
            ldy         #1
            sta         (w2),y
@shown:
            jsr         idx_drop                            ; (FORTH's index made again)
            ldy         #LR_FLAGS
            lda         (w3),y
            and         #<~LRF_HIDDEN
            sta         (w3),y
            rts

            HEADER      "dump", 0
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

; ---- SEE

; The assembly word w shown by disasm.fl's (see-code) ( nt -- ), if it's loaded (a library calls only the core, so
; it's found by its name).  OUT: C = 0, shown; or C = 1, w as it was (it isn't loaded)
see_code:
            lda         w
            pha
            lda         w + 1
            pha
            lda         #<s_seecode
            ldy         #>s_seecode
            PUSHAY
            lda         #S_SEECODE_LEN
            ldy         #0
            PUSHAY
            jsr         find_name
            inx
            inx
            pla                                             ; (The word's nt: .A/.Y)
            tay
            pla
            bcs         @none
            PUSHAY
            jsr         hdr_xt
            jsr         exec_w2
            clc
            rts
@none:
            sta         w
            sty         w + 1
            rts

s_seecode:  .byte       "(see-code)"
S_SEECODE_LEN = * - s_seecode

            HEADER      "see", 0
see:                                                        ; ( "name" -- ): its definition: a word's name for a call
            jsr         name_hdr                            ;   (or the address), a literal's number, a branch's and
            jsr         hdr_asm                             ;   IF's address, a string's text, an inline word's name
            bcc         :+                                  ;   for its code (or the bytes); to the rts past every
            jsr         see_code                            ;   branch's address.  An assembly word, with disasm.fl
            bcs         :+                                  ;   loaded: its instructions
            rts
:
            lda         #':'
            jsr         emit_a
            jsr         space
            jsr         hdr_out
            jsr         hdr_xt
            lda         RAM_BANK                            ; (A definition's in a code bank: its code, the bank
            pha                                             ;   selected till the end)
            jsr         see_stub
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
            pla
            sta         RAM_BANK
            jmp         cr

@jsr:
            jsr         @target
            ldy         #0                                  ; One of the routines compiled code calls?
@special:
            lda         see_xt,y
            ora         see_xt + 1,y
            bne         :+
            jmp         @call
:
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
            pla
            sta         RAM_BANK
            jmp         cr
@string:
            cmp         #$81
            beq         @pstring
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
@pstring:
            ldy         #3                                  ; (In a code bank: its address, the text there)
            lda         (w3),y
            sta         w
            iny
            lda         (w3),y
            sta         w + 1
            lda         (w)
            sta         cnt
            ldy         #0
:
            cpy         cnt
            beq         :+
            iny
            lda         (w),y
            jsr         emit_a
            bra         :-
:
            lda         #'"'
            jsr         emit_a
            jsr         space
            lda         #5
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
            jsr         lit_at
            bcs         @code_j
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

; Is the xt w2 a code bank's stub (jsr far_enter, the bank, the code)?  Its bank selected, w2 its code
see_stub:
            lda         (w2)
            cmp         #JSR_OP
            bne         @done
            ldy         #1
            lda         (w2),y
            cmp         #<far_enter
            bne         @done
            iny
            lda         (w2),y
            cmp         #>far_enter
            bne         @done
            iny
            lda         (w2),y
            sta         RAM_BANK
            iny
            lda         (w2),y
            pha
            iny
            lda         (w2),y
            sta         w2 + 1
            pla
            sta         w2
@done:
            rts

; Is the xt w2 the stub of the code at p1 in the bank selected (a call in a code bank to a word in it)?  C = 0 yes
stub_of:
            ldy         #5
            lda         (w2),y
            cmp         p1 + 1
            bne         @no
            dey
            lda         (w2),y
            cmp         p1
            bne         @no
            dey
            lda         (w2),y
            cmp         RAM_BANK
            bne         @no
            dey
            lda         (w2),y
            cmp         #>far_enter
            bne         @no
            dey
            lda         (w2),y
            cmp         #<far_enter
            bne         @no
            lda         (w2)
            cmp         #JSR_OP
            bne         @no
            clc
            rts
@no:
            sec
            rts

; The word whose xt is p1 (or whose stub's code it is, in the bank selected), in any word list: C = 0, w its header;
; or C = 1.  Keeps w3
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
            lda         p1 + 1                              ; (Code at BANK-WINDOW: a stub's?)
            cmp         #>BANK_WINDOW
            bcc         :+
            cmp         #>(BANK_WINDOW + BANK_SIZE)
            bcs         :+
            jsr         stub_of
            bcc         @found
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
            ldy         tmp                                 ; (0: called, a compile-only word)
            beq         @next
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
            .word       domarker, xsquote_p, xcquote_p, xabortq_p, do_does_far, 0
see_text:   .word       s_squote, s_dotq, s_cquote, s_abortq, s_does, s_do, s_qdo, s_loop, s_ploop, s_create
            .word       s_value, s_marker, s_squote, s_cquote, s_abortq, s_does
see_after:  .byte       $80, $80, $80, $80, 3, 0, 5, 9, 9, $7F, $7F, $7F, $81, $81, $81, 2
see_lit:    .byte       OP_DEX, OP_LDA_IMM, 0, OP_STA_ZPX, dlo, OP_LDA_IMM, 0, OP_STA_ZPX, dhi
see_if:     .byte       OP_INX, OP_LDA_ZPX, dlo - 1, OP_ORA_ZPX, dhi - 1, OP_BNE, 3
s_squote:   .byte       "s", $22, 0
s_dotq:     .byte       ".", $22, 0
s_cquote:   .byte       "c", $22, 0
s_abortq:   .byte       "abort", $22, 0
s_does:     .byte       "does>", 0
s_do:       .byte       "do", 0
s_qdo:      .byte       "?do", 0
s_loop:     .byte       "loop", 0
s_ploop:    .byte       "+loop", 0
s_create:   .byte       "create", 0
s_value:    .byte       "value", 0
s_marker:   .byte       "marker", 0
s_exit:     .byte       "exit ", 0
s_immed:    .byte       " immediate", 0
s_jmp:      .byte       "jmp ", 0
s_branch:   .byte       "branch ", 0
s_qbranch:  .byte       "?branch ", 0

; ---- Conditional compiling: the words skipped are parsed (refilling, from a file), so a [THEN] in a comment counts

            HEADER      "[if]", F_IMMEDIATE
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

            HEADER      "[else]", F_IMMEDIATE
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

            HEADER      "[then]", F_IMMEDIATE
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

            HEADER      "[defined]", F_IMMEDIATE
bdefined:                                                   ; ( "name" -- flag ): in the search order
            jsr         parse_name
            jsr         find_name
            inx
            bcc         :+
            jmp         zero_tos
:
            jmp         true_tos

            HEADER      "[undefined]", F_IMMEDIATE
bundefined:
            jsr         bdefined
            jmp         zequal

; ---- The control-flow stack (the data stack: an orig or a dest a cell) and the return stack

            HEADER      "cs-pick", 0
cspick:
            jmp         pick

            HEADER      "cs-roll", 0
csroll:
            jmp         roll

            HEADER      "n>r", 0
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

            HEADER      "nr>", 0
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

            HEADER      "synonym", 0
synonym:                                                    ; ( "newname" "oldname" -- ): newname as oldname is (its
            lda         #F_HIDDEN                           ;   code copied if it's inline; else a jmp to it), and
            jsr         make_hdr                            ;   compile-only if it is
            jsr         name_hdr
            jsr         hdr_xt
            dey                                             ; (F_INLINE: its byte, F_COMPILE too: tmp2)
            lda         (w),y
            sta         tmp2
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
            lda         tmp2                                ; Its byte (its code's length), its code and the rts
            jsr         ccomma_a                            ;   after it (none: a jmp to it)
            lda         tmp
            beq         @jmp
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

            HEADER      "traverse-wordlist", 0
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

            HEADER      "name>string", 0
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

            HEADER      "name>interpret", 0
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

            HEADER      "name>compile", 0
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
