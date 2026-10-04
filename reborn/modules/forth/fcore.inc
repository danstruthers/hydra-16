; ****************************************************************************
; fcore.inc - forth's words for the stacks, arithmetic, logic, comparisons and memory (included by forth.s)

; ---- The data stack

            HEADERI     "DUP", dup
dup:
            dex
            lda         dlo + 1,x
            sta         dlo,x
            lda         dhi + 1,x
            sta         dhi,x
dup_end:
            rts

            HEADERI     "DROP", drop
drop:
            inx
drop_end:
            rts

            HEADER      "SWAP", 0
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

            HEADERI     "OVER", over
over:
            dex
            lda         dlo + 2,x
            sta         dlo,x
            lda         dhi + 2,x
            sta         dhi,x
over_end:
            rts

            HEADERI     "NIP", nip
nip:
            lda         dlo,x
            sta         dlo + 1,x
            lda         dhi,x
            sta         dhi + 1,x
            inx
nip_end:
            rts

            HEADER      "TUCK", 0
tuck:                                                       ; ( a b -- b a b )
            jsr         swap
            jmp         over

            HEADER      "ROT", 0
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

            HEADER      "?DUP", 0
qdup:
            lda         dlo,x
            ora         dhi,x
            beq         :+
            jmp         dup
:
            rts

            HEADER      "2DUP", 0
twodup:
            jsr         over
            jmp         over

            HEADER      "2DROP", 0
twodrop:
            inx
            inx
            rts

            HEADER      "2SWAP", 0
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

            HEADER      "2OVER", 0
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

            HEADER      "DEPTH", 0
depth:
            stx         tmp
            sec
            lda         #DS_N
            sbc         tmp
            ldy         #0
            PUSHAY
            rts

            HEADER      "PICK", 0
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

            HEADER      "ROLL", 0
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

            HEADERI     ">R", tor
tor:
            lda         dhi,x
            pha
            lda         dlo,x
            pha
            inx
tor_end:
            rts

            HEADERI     "R>", rfrom
rfrom:
            dex
            pla
            sta         dlo,x
            pla
            sta         dhi,x
rfrom_end:
            rts

            HEADERI     "R@", rfetch
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

            HEADERI     "2>R", twotor
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

            HEADERI     "2R>", tworfrom
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

            HEADERI     "2R@", tworfetch
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

            HEADERI     "UNLOOP", unloop
unloop:
            pla
            pla
            pla
            pla
unloop_end:
            rts

            HEADERI     "EXIT", exit
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

            HEADER      "NEGATE", 0
negate:
            sec
            lda         #0
            sbc         dlo,x
            sta         dlo,x
            lda         #0
            sbc         dhi,x
            sta         dhi,x
            rts

            HEADER      "ABS", 0
abs:
            lda         dhi,x
            bmi         negate
            rts

            HEADERI     "INVERT", invert
invert:
            lda         dlo,x
            eor         #$FF
            sta         dlo,x
            lda         dhi,x
            eor         #$FF
            sta         dhi,x
invert_end:
            rts

            HEADERI     "AND", and_
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

            HEADERI     "OR", or_
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

            HEADERI     "XOR", xor_
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

            HEADER      "LSHIFT", 0
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

            HEADER      "RSHIFT", 0
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

            HEADER      "MIN", 0
min:
            jsr         lt_flag                             ; (C = 1: a < b, and a stays)
            bcc         nip_call
            inx
            rts

            HEADER      "MAX", 0
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

            HEADER      "<>", 0
notequal:
            jsr         equal
            jmp         invert

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

            HEADER      "U<", 0
uless:
            jsr         ult_flag
            bra         flag_c

            HEADER      "U>", 0
ugreater:
            jsr         swap
            jmp         uless

            HEADER      "0=", 0
zequal:
            lda         dlo,x
            ora         dhi,x
            beq         true_tos
            bra         zero_tos

            HEADER      "0<>", 0
znotequal:
            lda         dlo,x
            ora         dhi,x
            bne         true_tos
            bra         zero_tos

            HEADER      "0<", 0
zless:
            lda         dhi,x
            bmi         true_tos
            bra         zero_tos

            HEADER      "0>", 0
zgreater:
            lda         dhi,x
            bmi         @no
            ora         dlo,x
            beq         @no
            jmp         true_tos
@no:
            jmp         zero_tos

            HEADER      "WITHIN", 0
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

            HEADER      "TRUE", 0
true:
            dex
            jmp         true_tos

            HEADER      "FALSE", 0
false:
            dex
            jmp         zero_tos

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

            HEADERI     "C@", cfetch
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

            HEADERI     "C!", cstore
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

            HEADER      "MOVE", 0
move:                                                       ; ( from to u -- ): as if through a buffer
            lda         dlo + 2,x                           ; From below to: from the end down; else up
            cmp         dlo + 1,x
            lda         dhi + 2,x
            sbc         dhi + 1,x
            bcc         cmove_up
            jmp         cmove

            HEADER      "CMOVE", 0
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

            HEADER      "CMOVE>", 0
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

            HEADER      "FILL", 0
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

            HEADER      "ERASE", 0
erase:
            dex
            jsr         zero_tos
            jmp         fill

            HEADER      "COUNT", 0
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

            HEADER      "ALIGN", 0
align:
            rts

            HEADER      "ALIGNED", 0
aligned:
            rts

            HEADER      "CELLS", 0
cells:
            jmp         twostar

            HEADER      "CELL+", 0
cellplus:
            jsr         oneplus
            jmp         oneplus

            HEADER      "CHARS", 0
chars:
            rts

            HEADER      "CHAR+", 0
charplus:
            jmp         oneplus

            HEADER      "HERE", 0
here_:
            lda         here
            ldy         here + 1
            PUSHAY
            rts

            HEADER      "ALLOT", 0
allot:                                                      ; ( n -- ): no further than the dictionary's end
            clc
            lda         here
            adc         dlo,x
            tay
            lda         here + 1
            adc         dhi,x
            inx
            cmp         #>DICT_END
            bcc         :+
            lda         #<-8                                ; (Dictionary overflow)
            jmp         throw_a
:
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
; .A into the dictionary, a byte.  Keeps .X, .Y
ccomma_a:
            phy
            ldy         here + 1                            ; (Its end)
            cpy         #>DICT_END
            bcc         :+
            lda         #<-8
            jmp         throw_a
:
            sta         (here)
            inc         here
            bne         :+
            inc         here + 1
:
            ply
            rts

            HEADER      "C,", 0
ccomma:
            lda         dlo,x
            inx
            jmp         ccomma_a

            HEADER      "UNUSED", 0
unused:
            sec
            lda         #<DICT_END
            sbc         here
            pha
            lda         #>DICT_END
            sbc         here + 1
            tay
            pla
            PUSHAY
            rts

; ---- Variables and constants of the system

            HEADER      "BASE", 0
base_:
            lda         #<base
            ldy         #>base
            PUSHAY
            rts

            HEADER      "DECIMAL", 0
decimal:
            lda         #10
            bra         set_base

            HEADER      "HEX", 0
hex:
            lda         #16
set_base:
            sta         base
            stz         base + 1
            rts

            HEADER      "STATE", 0
state_:
            lda         #<state
            ldy         #>state
            PUSHAY
            rts

            HEADER      ">IN", 0
toin:
            lda         #<to_in
            ldy         #>to_in
            PUSHAY
            rts

            HEADER      "PAD", 0
pad_:
            lda         #<pad
            ldy         #>pad
            PUSHAY
            rts

            HEADER      "BL", 0
bl:
            lda         #BL_CHAR
            ldy         #0
            PUSHAY
            rts

            HEADER      "EXECUTE", 0
execute:
            lda         dlo,x
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            jmp         (w)
