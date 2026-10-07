; ****************************************************************************
; locals.s - HyForth's Locals word set (/lib/forth/locals.fl: lib locals): {: args | vals -- comment :}, (local),
; and the extension's locals| .  A definition's locals are a frame on the 6502's stack, made where they're declared
; (loc_enter) and let go at its end, EXIT and DOES> (loc_leave); lp (the core's) is its first cell, so DO's loop
; and >R above it don't move them, and the frame before is kept under it (a definition with locals calling another;
; CATCH keeps lp too).  A local compiles as code that reads (or, after TO, sets) its cell through lp.  The core asks
; this library about each name it compiles, before the search order and numbers (loc_vec: loc_hook), so a local's
; name hides a word's or a number's while its definition is compiled, and is gone at its end.  16 locals a
; definition (#LOCALS); {:'s args take the stack's cells, the deepest first; (local)s each the top in turn.

.include "forthlib.inc"

LOC_MAX     = 16                                            ; Locals a definition, at most (ENVIRONMENT? #LOCALS)
LOC_NAMES   = 192                                           ; Their names' bytes (counted), at most

OP_LDY_IMM  = $A0                                           ; (The code compiled)
OP_LDA_INDY = $B1
OP_STA_INDY = $91
OP_INY      = $C8

.bss
loc_owner:  .res        2                                   ; The definition the locals are (its xt: lastxt) ...
loc_n:      .res        1                                   ;   how many ...
loc_args:   .res        1                                   ;   of them the stack's ...
loc_live:   .res        1                                   ;   <> 0: their frame compiled (their names found) ...
loc_rev:    .res        1                                   ;   <> 0: (local)'s, the first the top ...
loc_mode:   .res        1                                   ;   {:'s: 0 args, 1 vals, 2 the comment ...
loc_len:    .res        1                                   ;   and their names (counted), in order
loc_names:  .res        LOC_NAMES
loc_what:   .res        1                                   ; The core's call (loc_hook's .A)
.code

            HEADERC     "{:", F_IMMEDIATE
bracecolon:                                                 ; ( "args | vals -- comment :}" -- ): the definition's
            jsr         loc_begin                           ;   locals (args from the stack, its top the last)
            stz         loc_mode
@word:
            jsr         parse_name
            lda         dlo,x
            ora         dhi,x
            bne         :+
            inx                                             ; (The line's end: the next line; none, THROW -22)
            inx
            lda         src_id + 1
            bmi         @none
            jsr         refill_src
            bcc         @word
@none:
            lda         #<-22
            jmp         throw_a
:
            LDR         w2, s_end                           ; :} the end
            jsr         loc_tok_is
            bcc         @done
            lda         loc_mode                            ; (The comment: to :})
            cmp         #2
            bcs         @drop
            LDR         w2, s_bar                           ; | the vals
            jsr         loc_tok_is
            bcs         :+
            lda         #1
            bra         @mode
:
            LDR         w2, s_dashes                        ; -- the comment
            jsr         loc_tok_is
            bcs         :+
            lda         #2
@mode:
            sta         loc_mode
@drop:
            inx
            inx
            bra         @word
:
            lda         loc_mode                            ; A name: an arg's, or a val's
            bne         :+
            inc         loc_args
:
            jsr         loc_add
            bra         @word
@done:
            inx
            inx
            jmp         loc_enter_c

            HEADER      "(local)", 0
parenlocal:                                                 ; ( c-addr u -- ): compiling, a local, the stack's top
            lda         dlo,x                               ;   (the one after it, the cell under it); 0 0, the end
            ora         dhi,x
            bne         @name
            inx
            inx
            lda         loc_n
            sta         loc_args
            jmp         loc_enter_c
@name:
            lda         loc_rev                             ; (The definition's first: its locals start)
            beq         @begin
            lda         loc_live
            bne         @begin
            lda         loc_owner
            cmp         lastxt
            bne         @begin
            lda         loc_owner + 1
            cmp         lastxt + 1
            beq         :+
@begin:
            jsr         loc_begin
            inc         loc_rev
:
            jmp         loc_add

            HEADERC     "locals|", F_IMMEDIATE
localsbar:                                                  ; ( "name ... |" -- ): (local) each, then 0 0 (local)
            jsr         parse_name
            lda         dlo,x
            ora         dhi,x
            beq         @end
            LDR         w2, s_bar
            jsr         loc_tok_is
            bcc         @end
            jsr         parenlocal
            bra         localsbar
@end:
            stz         dlo,x
            stz         dhi,x
            stz         dlo + 1,x
            stz         dhi + 1,x
            jmp         parenlocal

s_end:      .byte       2, ":}"
s_bar:      .byte       1, "|"
s_dashes:   .byte       2, "--"
s_nlocals:  .byte       7, "#LOCALS"

; The core's calls (loc_call's: .A what), about the locals of the definition being compiled
loc_hook:
            cmp         #4
            beq         loc_env
            cmp         #1
            beq         loc_end
            sta         loc_what
            jsr         loc_here
            bcs         @no
            lda         loc_live
            beq         @no
            lda         loc_what
            cmp         #2
            beq         loc_exit
            jsr         loc_find                            ; 0 and 3: a name, a local's?  (.Y its cell's offset)
            bcs         @no
            sty         tmp3
            ldy         #0                                  ; (Its code: read, or set: TO's)
            lda         loc_what
            cmp         #3
            bne         :+
            ldy         #store_code - loc_code
:
            jsr         loc_code_c
            inx
            inx
            clc
            rts
@no:
            sec
            rts

; The definition's end (; DOES>): its frame let go, its locals forgotten
loc_end:
            jsr         loc_here
            bcs         :+
            jsr         loc_leave_c
:
            stz         loc_n
            stz         loc_live
            stz         loc_rev
            stz         loc_owner
            stz         loc_owner + 1
            sec
            rts

; EXIT: the frame let go
loc_exit:
            jsr         loc_leave_c
            sec
            rts

; ENVIRONMENT? ( c-addr u ): #LOCALS ( -- n true ), C = 0; else C = 1
loc_env:
            LDR         w2, s_nlocals
            jsr         loc_tok_is
            bcs         @done
            lda         #LOC_MAX
            sta         dlo + 1,x
            stz         dhi + 1,x
            lda         #$FF
            sta         dlo,x
            sta         dhi,x
            clc
@done:
            rts

; Are there locals, and are they this definition's (loc_owner lastxt)?  C = 0 yes
loc_here:
            lda         loc_n
            beq         @no
            lda         loc_owner
            cmp         lastxt
            bne         @no
            lda         loc_owner + 1
            cmp         lastxt + 1
            bne         @no
            clc
            rts
@no:
            sec
            rts

; This definition's locals: none yet
loc_begin:
            lda         lastxt
            sta         loc_owner
            lda         lastxt + 1
            sta         loc_owner + 1
            stz         loc_n
            stz         loc_args
            stz         loc_live
            stz         loc_rev
            stz         loc_len
            rts

; ( c-addr u -- ): a local named that (31 characters at most), in its definition's next cell.  Too many: THROW -8
loc_add:
            lda         loc_n
            cmp         #LOC_MAX
            bcs         @full
            lda         dhi,x
            bne         @long
            lda         dlo,x
            cmp         #LEN_MASK + 1
            bcc         :+
@long:
            lda         #LEN_MASK
            sta         dlo,x
:
            sta         cnt
            sec                                             ; (Room for it, counted?)
            adc         loc_len
            bcs         @full
            cmp         #LOC_NAMES + 1
            bcs         @full
            ldy         loc_len                             ; Its count, then its characters (w2)
            sta         loc_len
            lda         cnt
            sta         loc_names,y
            tya
            sec
            adc         #<loc_names
            sta         w2
            lda         #>loc_names
            adc         #0
            sta         w2 + 1
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0
:
            cpy         cnt
            beq         :+
            lda         (w),y
            sta         (w2),y
            iny
            bra         :-
:
            inc         loc_n
            inx
            inx
            rts
@full:
            lda         #<-8
            jmp         throw_a

; Is the name ( c-addr u, kept) a local's (in either case)?  C = 0, .Y its cell's offset in the frame; or C = 1
loc_find:
            lda         dhi,x
            bne         @no
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            stz         tmp                                 ; (tmp: the name's place in loc_names, tmp + 1 its index)
            stz         tmp + 1
@name:
            lda         tmp + 1
            cmp         loc_n
            bcs         @no
            ldy         tmp
            lda         loc_names,y
            cmp         dlo,x
            bne         @next
            sta         cnt
            tya                                             ; (Its characters: w2)
            sec
            adc         #<loc_names
            sta         w2
            lda         #>loc_names
            adc         #0
            sta         w2 + 1
            ldy         #0
:
            cpy         cnt
            beq         @found
            lda         (w),y
            jsr         upper
            sta         tmp3
            lda         (w2),y
            jsr         upper
            cmp         tmp3
            bne         @next
            iny
            bra         :-
@next:
            ldy         tmp
            lda         loc_names,y
            sec
            adc         tmp
            sta         tmp
            inc         tmp + 1
            bra         @name
@found:
            lda         tmp + 1                             ; Its cell: its index, or (local)'s the other way
            ldy         loc_rev
            beq         :+
            eor         #$FF
            sec
            adc         loc_n
            dec
:
            asl
            tay
            clc
            rts
@no:
            sec
            rts

; Is the name ( c-addr u, kept) the counted string at w2 (in either case)?  C = 0 yes
loc_tok_is:
            lda         dhi,x
            bne         @no
            lda         (w2)
            cmp         dlo,x
            bne         @no
            sta         cnt
            lda         dlo + 1,x
            sta         w
            lda         dhi + 1,x
            sta         w + 1
            ldy         #0
:
            cpy         cnt
            beq         @yes
            lda         (w),y
            jsr         upper
            sta         tmp3
            iny
            lda         (w2),y
            jsr         upper
            cmp         tmp3
            bne         @no
            bra         :-
@yes:
            clc
            rts
@no:
            sec
            rts

; The frame's making compiled (lda #cells; ldy #args; jsr loc_enter), if there are locals: their names found from
; here on
loc_enter_c:
            lda         loc_n
            beq         @done
            lda         #OP_LDA_IMM
            jsr         ccomma_a
            lda         loc_n
            jsr         ccomma_a
            lda         #OP_LDY_IMM
            jsr         ccomma_a
            lda         loc_args
            jsr         ccomma_a
            lda         #<loc_enter
            ldy         #>loc_enter
            jsr         comp_jsr
            inc         loc_live
@done:
            rts

; Its letting go compiled (lda #cells; jsr loc_leave), if the frame was made
loc_leave_c:
            lda         loc_live
            beq         @done
            lda         #OP_LDA_IMM
            jsr         ccomma_a
            lda         loc_n
            jsr         ccomma_a
            lda         #<loc_leave
            ldy         #>loc_leave
            jmp         comp_jsr
@done:
            rts

; loc_code's 12 bytes at .Y compiled, the second the cell's offset (tmp3)
loc_code_c:
            lda         #12
            sta         cnt
:
            lda         loc_code,y
            cpy         #1
            beq         @off
            cpy         #store_code - loc_code + 1
            bne         @byte
@off:
            lda         tmp3
@byte:
            jsr         ccomma_a
            iny
            dec         cnt
            bne         :-
            rts

loc_code:                                                   ; (A local read: ldy #offset; lda (lp),y; dex ...)
fetch_code: .byte       OP_LDY_IMM, 0, OP_LDA_INDY, lp, OP_DEX, OP_STA_ZPX, dlo, OP_INY, OP_LDA_INDY, lp, OP_STA_ZPX, dhi
store_code: .byte       OP_LDY_IMM, 0, OP_LDA_ZPX, dlo, OP_STA_INDY, lp, OP_INY, OP_LDA_ZPX, dhi, OP_STA_INDY, lp, OP_INX

; ---- What a definition with locals runs

; The frame made: .A its cells, .Y how many of them take the stack's top cells (the first the deepest; dropped);
; lp's frame before kept under it
loc_enter:
            sta         tmp
            sty         tmp3
            sty         tmp3 + 1
            pla                                             ; (Its return address, put back after)
            sta         w
            pla
            sta         w + 1
            lda         lp
            pha
            stx         xsave
            lda         tmp
            asl
            sta         tmp2
            tsx
            txa
            sec
            sbc         tmp2
            tax
            txs
            inx
            stx         lp
            ldx         xsave
            lda         tmp3
            beq         @back
            txa                                             ; (.X: the deepest)
            clc
            adc         tmp3
            tax
            dex
            ldy         #0
:
            lda         dlo,x
            sta         (lp),y
            iny
            lda         dhi,x
            sta         (lp),y
            iny
            dex
            dec         tmp3
            bne         :-
            lda         xsave                               ; (Past them)
            clc
            adc         tmp3 + 1
            tax
@back:
            lda         w + 1
            pha
            lda         w
            pha
            rts

; The frame let go: .A its cells; lp's before it back
loc_leave:
            asl
            sta         tmp2
            pla
            sta         w
            pla
            sta         w + 1
            stx         xsave
            tsx
            txa
            clc
            adc         tmp2
            tax
            txs
            ldx         xsave
            pla
            sta         lp
            lda         w + 1
            pha
            lda         w
            pha
            rts

; lib_init: the core's compiler asks this library
lib_init:
            lda         #<loc_hook
            sta         loc_vec
            lda         #>loc_hook
            sta         loc_vec + 1
            rts
