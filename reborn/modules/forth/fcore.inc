; ****************************************************************************
; fcore.inc - forth's words for the stacks, arithmetic, logic, comparisons and memory (included by forth.s)

; ---- The data stack

            HEADERI     "dup", dup
dup:
            dex
            lda         dlo + 1,x
            sta         dlo,x
            lda         dhi + 1,x
            sta         dhi,x
dup_end:
            rts

            HEADERI     "drop", drop
drop:
            inx
drop_end:
            rts

            HEADER      "swap", 0
swap:
            lda         dlo,x
            ldy         dlo + 1,x
            sta         dlo + 1,x
            sty         dlo,x
            lda         dhi,x
            ldy         dhi + 1,x
            sta         dhi + 1,x
            sty         dhi,x
            rts

            HEADERI     "over", over
over:
            dex
            lda         dlo + 2,x
            sta         dlo,x
            lda         dhi + 2,x
            sta         dhi,x
over_end:
            rts

nip:                                                        ; ( a b -- b ) (the Core Extension library's NIP: a copy)
            lda         dlo,x
            sta         dlo + 1,x
            lda         dhi,x
            sta         dhi + 1,x
            inx
nip_end:
            rts

            HEADER      "rot", 0
rot:                                                        ; ( a b c -- b c a )
            ldy         dlo + 2,x
            lda         dlo + 1,x
            sta         dlo + 2,x
            lda         dlo,x
            sta         dlo + 1,x
            sty         dlo,x
            ldy         dhi + 2,x
            lda         dhi + 1,x
            sta         dhi + 2,x
            lda         dhi,x
            sta         dhi + 1,x
            sty         dhi,x
            rts

            HEADER      "?dup", 0
qdup:
            lda         dlo,x
            ora         dhi,x
            beq         :+
            jmp         dup
:
            rts

            HEADER      "2dup", 0
twodup:
            jsr         over
            jmp         over

            HEADER      "2drop", 0
twodrop:
            inx
            inx
            rts

            HEADER      "2swap", 0
twoswap:                                                    ; ( a b c d -- c d a b ): cells 0 and 2, 1 and 3
            lda         dlo,x
            ldy         dlo + 2,x
            sta         dlo + 2,x
            sty         dlo,x
            lda         dhi,x
            ldy         dhi + 2,x
            sta         dhi + 2,x
            sty         dhi,x
            lda         dlo + 1,x
            ldy         dlo + 3,x
            sta         dlo + 3,x
            sty         dlo + 1,x
            lda         dhi + 1,x
            ldy         dhi + 3,x
            sta         dhi + 3,x
            sty         dhi + 1,x
            rts

            HEADER      "2over", 0
twoover:                                                    ; ( a b c d -- a b c d a b )
            dex
            dex
            lda         dlo + 5,x
            sta         dlo + 1,x
            lda         dhi + 5,x
            sta         dhi + 1,x
            lda         dlo + 4,x
            sta         dlo,x
            lda         dhi + 4,x
            sta         dhi,x
            rts

            HEADER      "depth", 0
depth:
            stx         tmp
            sec
            lda         #DS_N
            sbc         tmp
            ldy         #0
            PUSHAY
            rts

; PICK's (the Core Extension library's header)
pick:                                                       ; ( xu ... x0 u -- xu ... x0 xu )
            txa
            clc
            adc         dlo,x
            tay
            lda         dlo + 1,y
            sta         dlo,x
            lda         dhi + 1,y
            sta         dhi,x
            rts

; ROLL's (the Core Extension library's header)
roll:                                                       ; ( xu xu-1 ... x0 u -- xu-1 ... x0 xu )
            lda         dlo,x
            inx
            cmp         #0
            beq         @done
            sta         cnt
            txa                                             ; xu's index
            clc
            adc         cnt
            tay
            lda         dlo,y                               ; It, kept ...
            sta         tmp
            lda         dhi,y
            sta         tmp + 1
:
            lda         dlo - 1,y                           ;   the rest up one ...
            sta         dlo,y
            lda         dhi - 1,y
            sta         dhi,y
            dey
            dec         cnt
            bne         :-
            lda         tmp                                 ;   and it on top
            sta         dlo,x
            lda         tmp + 1
            sta         dhi,x
@done:
            rts

; ---- The return stack (copied into definitions: they work on the definition's own return stack)

            HEADERI     ">r", tor, F_COMPILE
tor:
            lda         dhi,x
            pha
            lda         dlo,x
            pha
            inx
tor_end:
            rts

            HEADERI     "r>", rfrom, F_COMPILE
rfrom:
            dex
            pla
            sta         dlo,x
            pla
            sta         dhi,x
rfrom_end:
            rts

            HEADERI     "r@", rfetch, F_COMPILE
rfetch:
            stx         xsave
            tsx
            lda         $0101,x
            ldy         $0102,x
            ldx         xsave
            dex
            sta         dlo,x
            sty         dhi,x
rfetch_end:
            rts

            HEADERI     "unloop", unloop, F_COMPILE
unloop:
            pla
            pla
            pla
            pla
unloop_end:
            rts

            HEADERI     "exit", exit, F_COMPILE
exit:
            rts
exit_end:
            rts

; ---- Arithmetic and logic

            HEADERI     "+", plus
plus:
            clc
            lda         dlo,x
            adc         dlo + 1,x
            sta         dlo + 1,x
            lda         dhi,x
            adc         dhi + 1,x
            sta         dhi + 1,x
            inx
plus_end:
            rts

            HEADERI     "-", minus
minus:
            sec
            lda         dlo + 1,x
            sbc         dlo,x
            sta         dlo + 1,x
            lda         dhi + 1,x
            sbc         dhi,x
            sta         dhi + 1,x
            inx
minus_end:
            rts

            HEADERI     "1+", oneplus
oneplus:
            inc         dlo,x
            bne         :+
            inc         dhi,x
:
oneplus_end:
            rts

            HEADERI     "1-", oneminus
oneminus:
            lda         dlo,x
            bne         :+
            dec         dhi,x
:
            dec         dlo,x
oneminus_end:
            rts

            HEADERI     "2*", twostar
twostar:
            asl         dlo,x
            rol         dhi,x
twostar_end:
            rts

            HEADER      "2/", 0
twoslash:
            lda         dhi,x
            cmp         #$80
            ror         dhi,x
            ror         dlo,x
            rts

            HEADER      "negate", 0
negate:
            sec
            lda         #0
            sbc         dlo,x
            sta         dlo,x
            lda         #0
            sbc         dhi,x
            sta         dhi,x
            rts

            HEADER      "abs", 0
abs:
            lda         dhi,x
            bmi         negate
            rts

            HEADERI     "invert", invert
invert:
            lda         dlo,x
            eor         #$FF
            sta         dlo,x
            lda         dhi,x
            eor         #$FF
            sta         dhi,x
invert_end:
            rts

            HEADERI     "and", and_
and_:
            lda         dlo,x
            and         dlo + 1,x
            sta         dlo + 1,x
            lda         dhi,x
            and         dhi + 1,x
            sta         dhi + 1,x
            inx
and__end:
            rts

            HEADERI     "or", or_
or_:
            lda         dlo,x
            ora         dlo + 1,x
            sta         dlo + 1,x
            lda         dhi,x
            ora         dhi + 1,x
            sta         dhi + 1,x
            inx
or__end:
            rts

            HEADERI     "xor", xor_
xor_:
            lda         dlo,x
            eor         dlo + 1,x
            sta         dlo + 1,x
            lda         dhi,x
            eor         dhi + 1,x
            sta         dhi + 1,x
            inx
xor__end:
            rts

            HEADER      "lshift", 0
lshift:                                                     ; ( x u -- x << u )
            ldy         dlo,x
            inx
            lda         dhi,x                               ; (16 or more: 0)
            cpy         #16
            bcc         :+
            jmp         zero_tos
:
            tya
            beq         @done
:
            asl         dlo,x
            rol         dhi,x
            dey
            bne         :-
@done:
            rts

            HEADER      "rshift", 0
rshift:
            ldy         dlo,x
            inx
            cpy         #16
            bcc         :+
            jmp         zero_tos
:
            tya
            beq         @done
:
            lsr         dhi,x
            ror         dlo,x
            dey
            bne         :-
@done:
            rts

            HEADER      "min", 0
min:
            jsr         lt_flag                             ; (C = 1: a < b, and a stays)
            bcc         nip_call
            inx
            rts

            HEADER      "max", 0
max:
            jsr         lt_flag
            bcs         nip_call
            inx
            rts

nip_call:
            jmp         nip

; C = 1 if a < b, signed (the two on top: a under b).  Keeps the stack
lt_flag:
            lda         dlo + 1,x
            cmp         dlo,x
            lda         dhi + 1,x
            sbc         dhi,x
            bvc         :+
            eor         #$80
:
            asl                                             ; (N to C)
            rts

; C = 1 if a < b, unsigned
ult_flag:
            lda         dlo + 1,x
            cmp         dlo,x
            lda         dhi + 1,x
            sbc         dhi,x
            bcs         :+
            sec
            rts
:
            clc
            rts

; ---- Comparisons: true -1, false 0

; The top 0 (a false flag)
zero_tos:
            stz         dlo,x
            stz         dhi,x
            rts

; The top -1 (a true flag)
true_tos:
            lda         #$FF
            sta         dlo,x
            sta         dhi,x
            rts

            HEADER      "=", 0
equal:
            lda         dlo,x
            cmp         dlo + 1,x
            bne         :+
            lda         dhi,x
            cmp         dhi + 1,x
            bne         :+
            inx
            bra         true_tos
:
            inx
            bra         zero_tos

            HEADER      "<", 0
less:
            jsr         lt_flag
flag_c:                                                     ; Two dropped for a flag: C's
            inx
            bcs         true_tos
            bra         zero_tos

            HEADER      ">", 0
greater:
            jsr         swap
            jmp         less

            HEADER      "u<", 0
uless:
            jsr         ult_flag
            bra         flag_c

            HEADER      "0=", 0
zequal:
            lda         dlo,x
            ora         dhi,x
            beq         true_tos
            bra         zero_tos

            HEADER      "0<", 0
zless:
            lda         dhi,x
            bmi         true_tos
            bra         zero_tos

; ---- Memory

            HEADERI     "@", fetch
fetch:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            lda         (w)
            sta         dlo,x
            ldy         #1
            lda         (w),y
            sta         dhi,x
fetch_end:
            rts

            HEADERI     "!", store
store:                                                      ; ( x addr -- )
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
store_end:
            rts

            HEADERI     "c@", cfetch
cfetch:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            lda         (w)
            sta         dlo,x
            stz         dhi,x
cfetch_end:
            rts

            HEADERI     "c!", cstore
cstore:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            lda         dlo + 1,x
            sta         (w)
            inx
            inx
cstore_end:
            rts

            HEADER      "+!", 0
plusstore:                                                  ; ( n addr -- )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            clc
            lda         (w)
            adc         dlo + 1,x
            sta         (w)
            ldy         #1
            lda         (w),y
            adc         dhi + 1,x
            sta         (w),y
            inx
            inx
            rts

            HEADER      "2@", 0
twofetch:                                                   ; ( addr -- x1 x2 ): x2 at addr, x1 at addr + 2
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            dex
            ldy         #0
            lda         (w),y
            sta         dlo,x
            iny
            lda         (w),y
            sta         dhi,x
            iny
            lda         (w),y
            sta         dlo + 1,x
            iny
            lda         (w),y
            sta         dhi + 1,x
            rts

            HEADER      "2!", 0
twostore:                                                   ; ( x1 x2 addr -- )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            ldy         #0
            lda         dlo + 1,x
            sta         (w),y
            iny
            lda         dhi + 1,x
            sta         (w),y
            iny
            lda         dlo + 2,x
            sta         (w),y
            iny
            lda         dhi + 2,x
            sta         (w),y
            inx
            inx
            inx
            rts

            HEADER      "move", 0
move:                                                       ; ( from to u -- ): as if through a buffer
            lda         dlo + 2,x                           ; From below to: from the end down; else up
            cmp         dlo + 1,x
            lda         dhi + 2,x
            sbc         dhi + 1,x
            bcc         cmove_up
            jmp         cmove

cmove:                                                      ; ( from to u -- ): a byte at a time, up
            jsr         move_args
@page:
            lda         tmp + 1
            beq         @part
            ldy         #0
:
            lda         (w),y
            sta         (w2),y
            iny
            bne         :-
            inc         w + 1
            inc         w2 + 1
            dec         tmp + 1
            bra         @page

@part:
            ldy         #0
:
            cpy         tmp
            beq         @done
            lda         (w),y
            sta         (w2),y
            iny
            bra         :-
@done:
            rts

cmove_up:                                                   ; ( from to u -- ): a byte at a time, from the end down
            jsr         move_args
            clc                                             ; (w, w2: their last pages)
            lda         w + 1
            adc         tmp + 1
            sta         w + 1
            clc
            lda         w2 + 1
            adc         tmp + 1
            sta         w2 + 1
            ldy         tmp                                 ; The part, first
:
            cpy         #0
            beq         @pages
            dey
            lda         (w),y
            sta         (w2),y
            bra         :-
@pages:
            lda         tmp + 1
            beq         @done
            dec         w + 1
            dec         w2 + 1
            ldy         #0
:
            dey
            lda         (w),y
            sta         (w2),y
            cpy         #0
            bne         :-
            dec         tmp + 1
            bra         @pages
@done:
            rts

; MOVE's and CMOVE's ( from to u -- ): w from, w2 to, tmp u
move_args:
            lda         dlo,x
            sta         tmp
            lda         dhi,x
            sta         tmp + 1
            lda         dlo + 1,x
            sta         w2
            lda         dhi + 1,x
            sta         w2 + 1
            lda         dlo + 2,x
            sta         w
            lda         dhi + 2,x
            sta         w + 1
            inx
            inx
            inx
            rts

            HEADER      "fill", 0
fill:                                                       ; ( addr u char -- )
            lda         dlo + 2,x
            sta         w
            lda         dhi + 2,x
            sta         w + 1
            lda         dlo + 1,x
            sta         tmp
            lda         dhi + 1,x
            sta         tmp + 1
            lda         dlo,x
            inx
            inx
            inx
fill_a:                                                     ; tmp bytes of .A at w
            ldy         tmp + 1
            beq         @part
            ldy         #0
:
            sta         (w),y
            iny
            bne         :-
            inc         w + 1
            dec         tmp + 1
            bne         :-
@part:
            ldy         tmp
            beq         @done
            ldy         #0
:
            sta         (w),y
            iny
            cpy         tmp
            bne         :-
@done:
            rts

            HEADER      "count", 0
count:                                                      ; ( c-addr -- addr u )
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            lda         (w)
            inc         dlo,x
            bne         :+
            inc         dhi,x
:
            ldy         #0
            PUSHAY
            rts

            HEADER      "align", 0
align:
            rts

            HEADER      "aligned", 0
aligned:
            rts

            HEADER      "cells", 0
cells:
            jmp         twostar

            HEADER      "cell+", 0
cellplus:
            jsr         oneplus
            jmp         oneplus

            HEADER      "chars", 0
chars:
            rts

            HEADER      "char+", 0
charplus:
            jmp         oneplus

            HEADER      "here", 0
here_:
            lda         here
            ldy         here + 1
            PUSHAY
            rts

            HEADER      "allot", 0
allot:                                                      ; ( n -- ): no further than the dictionary's end (the
            clc                                             ;   heap's page; the code's: its bank's)
            lda         here
            adc         dlo,x
            tay
            lda         here + 1
            adc         dhi,x
            inx
            bit         cmode
            bmi         @code
            cmp         heap_lo + 1
            bcc         @set
@full:
            lda         #<-8                                ; (Dictionary overflow)
            jmp         throw_a
@code:
            cmp         #>(BANK_WINDOW + BANK_SIZE)
            bcs         @full
@set:
            sty         here
            sta         here + 1
            rts

            HEADER      ",", 0
comma:
            lda         dlo,x
            ldy         dhi,x
            inx
; .A/.Y into the dictionary, a cell
comma_ay:
            jsr         ccomma_a
            tya
; .A into the dictionary, a byte (a definition's code, CM_CODE: into its code bank, selected meanwhile, as an
; immediate word in another may be running).  Keeps .X, .Y
ccomma_a:
            phy
            ldy         here + 1
            bit         cmode
            bmi         @code
            cpy         heap_lo + 1                         ; (Its end: the heap's page)
            bcs         @full
            sta         (here)
@next:
            inc         here
            bne         :+
            inc         here + 1
:
            ply
            rts
@code:
            cpy         #>(BANK_WINDOW + BANK_SIZE)         ; (The bank's end)
            bcs         @full
            ldy         RAM_BANK
            phy
            ldy         cbank
            sty         RAM_BANK
            sta         (here)
            ply
            sty         RAM_BANK
            bra         @next
@full:
            lda         #<-8
            jmp         throw_a

            HEADER      "c,", 0
ccomma:
            lda         dlo,x
            inx
            jmp         ccomma_a

; ---- Variables and constants of the system

            HEADER      "base", 0
base_:
            lda         #<base
            ldy         #>base
            PUSHAY
            rts

            HEADER      "decimal", 0
decimal:
            lda         #10
            bra         set_base

; BASE .A (DECIMAL's; and HEX's: the Core Extension library's), the numbers library's base set from it again (as
; its own may be one of the same radix: set-base's "<d")
set_base:
            sta         base
            stz         base + 1
            stz         num_base
            stz         num_base + 1
            rts

            HEADER      "state", 0
state_:
            lda         #<state
            ldy         #>state
            PUSHAY
            rts

            HEADER      ">in", 0
toin:
            lda         #<to_in
            ldy         #>to_in
            PUSHAY
            rts

            HEADER      "bl", 0
bl:
            CONSTCODE   BL_CHAR

            HEADER      "execute", 0
execute:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            jmp         (w)
