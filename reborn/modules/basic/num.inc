; ****************************************************************************
; num.inc - BASIC's numbers (the fourth bank), on the number libraries (the modules numbers and math: NUMCALL and
; MATHCALL, numbers.inc's).  A value is an integer of 32 bits (VT_INT), worked by the interpreter itself while it
; fits (run.inc), or a number in the stored format (VT_NUM: its length and bytes, in the heap or the code).  Here
; the rest: an integer's operand made its bytes in the RAM (nia, nib), a VT_NUM's copied into the scratch bank (NSB:
; NS_A, NS_B), the library called with that bank at $8000, its result (NS_R) a VT_INT again if it fits, else a
; number in the heap.  A number's text is DISPLAY's, into the text bank (TXB).

NS_A            = $8000         ; The scratch bank: an operand ...
NS_B            = $8410         ;   another ...
NS_R            = $8820         ;   the result (NUM_MAX each)
TX_NUM          = $9BE0         ; The text bank: a number to be written (NUM_MAX + 2: to $9FF2) ...
TX_ROOM         = TX_NUM - $8000 ;   and its text, from $8000
NB_ROOM         = 64            ; An operand's bytes below $8000 (nba, nbb), if it's this long or less


.bss
num_bank:   .res        1                                   ; The libraries' bank (r13), the numbers library's
num_mod:    .res        1                                   ;   module and the math library's (0: none): NUMCALL's
math_mod:   .res        1
nsb:        .res        1                                   ; The scratch bank and the text bank (physical)
txb:        .res        1
nia:        .res        6                                   ; An integer's bytes, as an operand: the first ...
nib:        .res        6                                   ;   and the second
nba:        .res        NB_ROOM                             ; A VT_NUM's bytes as an operand, the first's and the
nbb:        .res        NB_ROOM                             ;   second's, if they fit (else NS_A, NS_B)
nmag:       .res        5                                   ; (int_sf's: a negative integer's magnitude, from nmag + 1)
n_op:        .res        1                                   ; (num_op's op)
nlen:       .res        2                                   ; (A result's length)
nsign:      .res        1                                   ; (A number's sign: $FF, 0, 1)
digits_set: .res        1                                   ; (The math library's precision, as set: DIGITS's)
mi_buf:     .res        ME_SIZE                             ; (num_init's: a module's entry)
rr3:        .res        2                                   ; (round_r0's: r3 kept)
rsrc:       .res        2                                   ; (result_vt's: the result's place)
nt_base:    .res        2                                   ; num_text's base (0: the base)
nolen:      .res        1                                   ; (operand's: an integer's or a VT_FIX's bytes' length)
rv_tag:     .res        1                                   ; (result_vt's: a fixed decimal's tag)

.segment "CODE4"

; ---- The libraries readied (main's start): found, a bank of the task's theirs, the scratch and text banks.  OUT:
; C = 1 if there's no numbers library (or no bank)
num_init:
            stz         num_mod
            stz         math_mod
            stz         t0
@find:
            LDR         r0, mi_buf                          ; (A module's entry: MODINFO's)
            lda         t0
            jsr         MODINFO
            bcs         @found
            lda         mi_buf + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @next
            ldx         #0
            jsr         mi_is
            bne         :+
            lda         mi_buf + ME_BANK
            sta         num_mod
:
            ldx         #s_math - s_numbers
            jsr         mi_is
            bne         @next
            lda         mi_buf + ME_BANK
            sta         math_mod
@next:
            inc         t0
            bra         @find
@found:
            lda         num_mod
            beq         @none
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @none
            sta         num_bank
            NUMCALL     NUM_INIT
            lda         #1                                  ; ($70-$7F not BASIC's: not kept over a call)
            NUMCALL     NUM_OPTIONS
            lda         #12
            sta         digits_set
            ldx         #LB_NSB                             ; (The scratch and text banks: taken now)
            jsr         lb_take
            sta         nsb
            ldx         #LB_TXB
            jsr         lb_take
            sta         txb
            clc
            rts
@none:
            sec
            rts

; Is the module entry's name the one at s_numbers,x?  OUT: Z = 1 yes
mi_is:
            ldy         #0
:
            lda         s_numbers,x
            cmp         mi_buf + ME_NAME,y
            bne         :+
            inx
            iny
            cmp         #0
            bne         :-
:
            rts
s_numbers:  .byte       "numbers", 0
s_math:     .byte       "math", 0

; ---- Text to a number, a number's integer

; PARSE (r0 text, r1 its length, r4 a base string, r2 the place, r3 its room, .Y the flags), its place in the RAM.
; OUT: C = 0, .A/.X its length (r5 the characters it took); C = 1 not one
num_parse_ram:
            NUMCALL     NUM_PARSE
            php
            pha
            jsr         code_sel
            pla
            plp
            rts

; tisint and tint: is the number at tnum (tnl bytes) an integer of 32 bits, signed?
num_isint32:
            LDW         np, tnum
            jsr         sf_int32
            lda         #0
            bcs         @set
            ldx         #3
:
            lda         t0,x
            sta         tint,x
            dex
            bpl         :-
            lda         #1
@set:
            sta         tisint
            rts

; Is the number at (np) (its bytes in the RAM, or in the bank at $8000) an integer of 32 bits, signed?  OUT: C = 0
; yes, t0-t3 it; C = 1 no
sf_int32:
            lda         (np)
            bmi         @tagged
            stz         t0 + 1                              ; (-64 to 63: the tag itself, 7-bit two's complement)
            stz         t0 + 2
            stz         t0 + 3
            cmp         #$40
            bcc         :+
            ora         #$80
            ldx         #$FF
            stx         t0 + 1
            stx         t0 + 2
            stx         t0 + 3
:
            sta         t0
            clc
            rts
@tagged:
            cmp         #$84                                ; ($80-$83: 1-4 bytes, positive)
            bcc         @pos
            cmp         #$90
            bcc         @no
            cmp         #$94                                ; ($90-$93: negative)
            bcs         @no
            and         #$0F
            jsr         @mag
            lda         t0 + 3                              ; (Below -2^31: no; -2^31 itself fits)
            bpl         @neg
            cmp         #$80
            bne         @no
            lda         t0
            ora         t0 + 1
            ora         t0 + 2
            bne         @no
@neg:
            sec                                             ; (Negated)
            ldx         #0
            ldy         #4
:
            lda         #0
            sbc         t0,x
            sta         t0,x
            inx
            dey
            bne         :-
            clc
            rts
@pos:
            and         #$0F
            jsr         @mag
            lda         t0 + 3
            bmi         @no
            clc
            rts
@no:
            sec
            rts
@mag:
            tax                                             ; (Its n bytes into t0, the rest 0)
            inx
            stz         t0
            stz         t0 + 1
            stz         t0 + 2
            stz         t0 + 3
            ldy         #1
:
            lda         (np),y
            sta         t0 - 1,y
            iny
            dex
            bne         :-
            rts

; The integer t0-t3 as its bytes in the stored format at (np).  OUT: .A = their count
int_sf:
            lda         t0 + 3                              ; (-64 to 63: one byte)
            bmi         @negs
            ora         t0 + 2
            ora         t0 + 1
            bne         @pos
            lda         t0
            cmp         #$40
            bcs         @pos
            sta         (np)
            lda         #1
            rts
@negs:
            and         t0 + 2
            and         t0 + 1
            cmp         #$FF
            bne         @neg
            lda         t0
            cmp         #$C0
            bcc         @neg
            and         #$7F
            sta         (np)
            lda         #1
            rts
@pos:
            lda         #$80
            sta         t4
            ldx         #0
@bytes:
            ldy         #1                                  ; (Its magnitude, its bytes but the top 0s)
:
            lda         t0,x
            sta         (np),y
            inx
            iny
            cpx         #4
            bne         :-
            dey
:
            cpy         #1
            beq         :+
            lda         (np),y
            bne         :+
            dey
            bra         :-
:
            tya                                             ; (Its tag: n - 1)
            dec         a
            ora         t4
            sta         (np)
            tya
            inc         a
            rts
@neg:
            sec                                             ; (The magnitude: -t0 (t0 kept), from nmag + 1)
            ldx         #0
            ldy         #4
            lda         #0
            sbc         t0
            sta         nmag + 1
            lda         #0
            sbc         t0 + 1
            sta         nmag + 2
            lda         #0
            sbc         t0 + 2
            sta         nmag + 3
            lda         #0
            sbc         t0 + 3
            sta         nmag + 4
            lda         #$90
            sta         t4
            ldy         #1
:
            lda         nmag,y
            sta         (np),y
            iny
            cpy         #5
            bne         :-
            dey
:
            cpy         #1
            beq         :+
            lda         (np),y
            bne         :+
            dey
            bra         :-
:
            tya
            dec         a
            ora         t4
            sta         (np)
            tya
            inc         a
            rts

; ---- An operand made ready: the value at vt as a number's bytes, its address in r0 (or r1: .X = 2)

; .Y = which (0: the first, r0, NS_A or nia; 1: the second, r1, NS_B or nib).  The scratch bank at $8000 after
operand:
            sty         t4 + 3
            lda         (vt)
            bne         :+
            jmp         @int
:
            cmp         #VT_FIX
            bcc         :+
            jmp         @fix
:
            cmp         #VT_NUM
            beq         :+
            lda         #ER_TYPE
            ldx         #0
            jmp         b_error
:
            ldy         #1                                  ; A VT_NUM: its bytes copied to NS_A or NS_B
            lda         (vt),y
            sta         ca
            iny
            lda         (vt),y
            sta         ca + 1
            iny
            lda         (vt),y
            sta         cal
            jsr         ca_sel
            jsr         ca_rd                               ; (Its length)
            sta         cn
            jsr         ca_rd
            sta         cn + 1
            lda         cn + 1                              ; (Below $8000 if it fits: nba, nbb)
            bne         @bank
            lda         cn
            cmp         #NB_ROOM + 1
            bcs         @bank
            LDW         cb, nba
            ldx         t4 + 3
            beq         @to
            LDW         cb, nbb
            bra         @to
@bank:
            LDW         cb, NS_A
            ldx         t4 + 3
            beq         @to
            LDW         cb, NS_B
@to:
            lda         #LB_NSB
            sta         cbl
            MOVW        t4, cb
            jsr         far_copy
            lda         nsb
            sta         RAM_BANK
            lda         t4 + 3
            asl         a
            tax
            lda         t4
            sta         r0,x
            lda         t4 + 1
            sta         r0 + 1,x
            rts
@int:
            ldy         #4                                  ; A VT_INT: its bytes in nia or nib
:
            lda         (vt),y
            sta         t0 - 1,y
            dey
            bne         :-
            LDW         np, nia
            ldx         t4 + 3
            beq         :+
            LDW         np, nib
:
            jsr         int_sf
            sta         nolen
@at:
            ldx         t4 + 3                              ; (r0 or r1: 2 bytes each)
            txa
            asl         a
            tax
            lda         np
            sta         r0,x
            lda         np + 1
            sta         r0 + 1,x
            lda         nsb
            sta         RAM_BANK
            rts
@fix:
            ldy         #4                                  ; A VT_FIX: its tag, then its digits' bytes, in nba or nbb
:
            lda         (vt),y                              ;   (their room: ROUND's over them)
            sta         t0 - 1,y
            dey
            bne         :-
            LDW         np, nba + 1
            ldx         t4 + 3
            beq         :+
            LDW         np, nbb + 1
:
            jsr         int_sf
            inc         a
            sta         nolen
            lda         np
            bne         :+
            dec         np + 1
:
            dec         np
            lda         (vt)
            sta         (np)
            bra         @at

; ---- The ops: two values (or one) worked by the libraries, the result in the first's place

; .A = the op (run.inc's OP_*): its operands the stack's top two (or one)
num_op:
            sta         n_op
            ldx         #0                                  ; (One operand: NEG, NOT)
            cmp         #OP_NEG
            beq         @one
            cmp         #OP_NOT
            beq         @one
            jsr         two_vals                            ; (vt the first, vu the second)
            ldy         #0
            jsr         operand                             ; (r0)
            MOVW        vt, vu
            ldy         #1
            jsr         operand                             ; (r1)
            jsr         two_vals
            bra         @call
@one:
            jsr         top_vt
            ldy         #0
            jsr         operand
@call:
            LDW         r2, NS_R
            LDW         r3, NUM_MAX
            lda         n_op
            sec
            sbc         #OP_ADD
            asl         a
            tax
            jmp         (nop_tab,x)
nop_tab:
            .word       n_add, n_sub, n_mul, n_div, n_idiv, n_mod, n_pow, n_neg
            .word       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            .word       n_not, n_and, n_or, n_xor, n_eqv, n_imp
n_add:
            NUMCALL     NUM_ADD
            jmp         n_result
n_sub:
            NUMCALL     NUM_SUB
            jmp         n_result
n_mul:
            NUMCALL     NUM_MUL
            jmp         n_result
n_div:
            NUMCALL     NUM_DIV
            jmp         n_result
n_neg:
            NUMCALL     NUM_NEG
            jmp         n_result1
; \ and MOD: each operand rounded to an integer first (QuickBASIC's), then the quotient (toward 0) or remainder
n_idiv:
n_mod:
            jsr         round_ops
            LDW         r5, NS_B                            ; (The remainder: over the second operand's copy)
            LDW         r6, NUM_MAX
            NUMCALL     NUM_IDIV
            bcc         @lb904
            jmp         n_fail
@lb904:
            lda         n_op
            cmp         #OP_MOD
            beq         @lb903
            jmp         n_result
@lb903:
            MOVW        r2, r5                              ; (MOD: the remainder, its length r6)
            lda         r6
            ldx         r6 + 1
            clc
            jmp         n_result_at
n_pow:
            lda         math_mod
            beq         :+
            MATHCALL    MATH_RPOW
            jmp         n_result
:
            NUMCALL     NUM_POW
            jmp         n_result
n_not:
            jsr         round_one
            ldy         #NBIT_NOT
            NUMCALL     NUM_BITS
            jmp         n_result1
n_and:
            ldy         #NBIT_AND
            bra         n_bits
n_or:
            ldy         #NBIT_OR
            bra         n_bits
n_xor:
            ldy         #NBIT_XOR
n_bits:
            phy
            jsr         round_ops
            ply
            NUMCALL     NUM_BITS
            jmp         n_result
; EQV: NOT (a XOR b); IMP: (NOT a) OR b
n_eqv:
            jsr         round_ops
            ldy         #NBIT_XOR
            NUMCALL     NUM_BITS
            bcc         @lb902
            jmp         n_fail
@lb902:
            MOVW        r0, r2
            ldy         #NBIT_NOT
            LDW         r2, NS_B
            NUMCALL     NUM_BITS
            jmp         n_result
n_imp:
            jsr         round_ops
            MOVW        t4, r1                              ; (b kept)
            ldy         #NBIT_NOT
            NUMCALL     NUM_BITS                            ; (NOT a, at NS_R)
            bcc         @lb901
            jmp         n_fail
@lb901:
            MOVW        r0, r2
            MOVW        r1, t4
            LDW         r2, NS_A
            ldy         #NBIT_OR
            NUMCALL     NUM_BITS
            jmp         n_result

; The operands made integers (ROUND: a half to the even), each over itself in the scratch bank (an integer's bytes
; in the RAM: as they are)
round_ops:
            jsr         round_one
            MOVW        t4, r0
            MOVW        r0, r1
            jsr         round_r0
            MOVW        r1, r0
            MOVW        r0, t4
            rts
round_one:
round_r0:
            lda         (r0)                                ; (An integer already: as it is; its bank's selected)
            cmp         #NT_FIXED
            bcc         @done
            MOVW        t4 + 2, r2
            MOVW        rr3, r3
            MOVW        r2, r0
            LDW         r3, NB_ROOM                         ; (Over itself: its room, below $8000 or in the bank)
            lda         r0 + 1
            bpl         :+
            LDW         r3, NUM_MAX
:
            NUMCALL     NUM_ROUND
            bcs         n_fail
            MOVW        r2, t4 + 2
            MOVW        r3, rr3
@done:
            rts

; A failure: its error (NE_*) as BASIC's
n_fail:
            jsr         code_sel
            tax
            lda         ne_err,x
            ldx         #0
            jmp         b_error
ne_err:     .byte       ER_FUNC, ER_OVERFLOW, ER_DIV0, ER_TYPE, ER_OVERFLOW, ER_FUNC, ER_FUNC, ER_FUNC, ER_FUNC
            .byte       ER_FUNC, ER_FUNC

; The result (C = 0: at r2, its length .A/.X) made a value in the first operand's place, the second dropped
n_result:
            bcs         n_fail
n_result_at:
            sta         nlen
            stx         nlen + 1
            jsr         two_vals
            jsr         result_vt
            MOVW        vsp, vu
            jmp         code_sel
n_result1:
            bcs         n_fail
            sta         nlen
            stx         nlen + 1
            jsr         top_vt
            jsr         result_vt
            jmp         code_sel

; The number at r2 (nlen bytes, in the scratch bank) put at vt: a VT_INT if it fits, a VT_FIX if it's a fixed
; decimal whose digits do, else a number in the heap
result_vt:
            MOVW        np, r2
            lda         nsb
            sta         RAM_BANK
            jsr         sf_int32
            bcs         @fix
            lda         #VT_INT
@value:
            sta         (vt)
            ldy         #4
:
            lda         t0 - 1,y
            sta         (vt),y
            dey
            bne         :-
            rts
@fix:
            lda         (np)
            cmp         #NT_FIXED
            bcc         rv_heap
            cmp         #NT_FIXED + 16
            bcs         rv_heap
            sta         rv_tag
            inc         np
            bne         :+
            inc         np + 1
:
            jsr         sf_int32
            bcs         rv_heap
            lda         rv_tag
            bra         @value
rv_heap:
            MOVW        rsrc, r2                            ; (The heap's room for it: then copied; not t4, which a
                                                            ;   collection changes)
            lda         vt
            pha
            lda         vt + 1
            pha
            lda         nlen
            ldx         nlen + 1
            FARN        5, heap_num                         ; (A block for nlen bytes: hres its far address)
            pla
            sta         vt + 1
            pla
            sta         vt
            lda         #VT_NUM
            sta         (vt)
            ldy         #1
            lda         hres
            sta         (vt),y
            iny
            lda         hres + 1
            sta         (vt),y
            iny
            lda         hres + 2
            sta         (vt),y
            lda         hres                                ; (Its bytes: after its length)
            clc
            adc         #2
            sta         cb
            lda         hres + 1
            adc         #0
            sta         cb + 1
            lda         hres + 2
            sta         cbl
            lda         cb + 1                              ; (Past the window's end: the next bank)
            cmp         #WIN_END
            bcc         :+
            lda         #WIN
            sta         cb + 1
            inc         cbl
:
            MOVW        ca, rsrc
            lda         #LB_NSB
            sta         cal
            MOVW        cn, nlen
            jmp         far_copy

; ---- Comparisons, tests

; t4 = the order of the values at vt and vu ($FF, 0, 1): CMP
num_cmp2:
            ldy         #0
            jsr         operand
            MOVW        t4 + 2, vt                          ; (vt kept: operand moves it)
            MOVW        vt, vu
            ldy         #1
            jsr         operand
            MOVW        vt, t4 + 2
            NUMCALL     NUM_CMP
            bcs         :+
            sta         t4
            jmp         code_sel
:
            jmp         n_fail

; Is the value at vt 0?  OUT: Z = 1 yes
num_is_zero:
            lda         (vt)
            cmp         #VT_FIX
            bcs         @fix
            cmp         #VT_NUM
            bne         @no                                 ; (Not a number: not 0)
            ldy         #0
            jsr         operand
            NUMCALL     NUM_KIND
            jsr         code_sel
            txa                                             ; (Z: .A's, through FARN)
            rts
@no:
            lda         #1
            rts
@fix:
            ldy         #4                                  ; (A VT_FIX: its digits)
            lda         (vt),y
            dey
            ora         (vt),y
            dey
            ora         (vt),y
            dey
            ora         (vt),y
            rts

; The VT_FIX at vt made a VT_NUM in the heap, the same number (CONST's: its bytes)
fix_heap:
            ldy         #0
            jsr         operand                             ; (Its bytes at nba: nolen of them)
            MOVW        r2, r0
            lda         nolen
            sta         nlen
            stz         nlen + 1
            jsr         rv_heap
            jmp         code_sel

; Is the number at tnum a fixed decimal of 0-15 places whose digits fit in 32 bits (signed)?  OUT: C = 0 yes (tint its
; digits); C = 1 no
num_isfix32:
            lda         tnum
            cmp         #NT_FIXED
            bcc         @no
            cmp         #NT_FIXED + 16
            bcs         @no
            LDW         np, tnum + 1
            jsr         sf_int32
            bcs         @no
            ldx         #3
:
            lda         t0,x
            sta         tint,x
            dex
            bpl         :-
            clc
            rts
@no:
            sec
            rts

; The value at vt made an integer in place (VT_INT), rounded if it isn't one (a half to the even); past 32 bits:
; Illegal function call
num_to_int:
            ldy         #0
            jsr         operand
            jsr         round_r0
            NUMCALL     NUM_TO_INT
            bcc         @lb900
            jmp         n_fail
@lb900:
            cmp         #0
            bne         @big
            lda         #VT_INT
            sta         (vt)
            ldy         #1
            lda         r4
            sta         (vt),y
            iny
            lda         r4 + 1
            sta         (vt),y
            iny
            lda         r5
            sta         (vt),y
            iny
            lda         r5 + 1
            sta         (vt),y
            jmp         code_sel
@big:
            jsr         code_sel
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; MUL of two integers of 16 bits each (vt, vu): their product, a VT_INT in vt's place, the top dropped
mul16:
            ldy         #1                                  ; (Their signs: the magnitudes multiplied)
            lda         (vt),y
            sta         t0
            iny
            lda         (vt),y
            sta         t0 + 1
            ldy         #1
            lda         (vu),y
            sta         t0 + 2
            iny
            lda         (vu),y
            sta         t0 + 3
            lda         t0 + 1
            eor         t0 + 3
            sta         nsign
            lda         t0 + 1
            bpl         :+
            sec
            lda         #0
            sbc         t0
            sta         t0
            lda         #0
            sbc         t0 + 1
            sta         t0 + 1
:
            lda         t0 + 3
            bpl         :+
            sec
            lda         #0
            sbc         t0 + 2
            sta         t0 + 2
            lda         #0
            sbc         t0 + 3
            sta         t0 + 3
:
            stz         t4                                  ; (t4: the product, 32 bits)
            stz         t4 + 1
            stz         t4 + 2
            stz         t4 + 3
            ldx         #16
@bit:
            lsr         t0 + 1
            ror         t0
            bcc         :+
            clc
            lda         t4 + 2
            adc         t0 + 2
            sta         t4 + 2
            lda         t4 + 3
            adc         t0 + 3
            sta         t4 + 3
:
            ror         t4 + 3
            ror         t4 + 2
            ror         t4 + 1
            ror         t4
            dex
            bne         @bit
            lda         nsign
            bpl         :+
            sec
            ldx         #0
            ldy         #4
@neg:
            lda         #0
            sbc         t4,x
            sta         t4,x
            inx
            dey
            bne         @neg
:
            ldy         #4
:
            lda         t4 - 1,y
            sta         (vt),y
            dey
            bne         :-
            MOVW        vsp, vu
            rts

; ---- A number's text: DISPLAY into the text bank ($8000 on), in the base (r4: 0) or another.  The value at vt.
; OUT: .A/.X its length; nsign its sign
num_text:
            stz         nsign
            lda         (vt)
            beq         @int
            cmp         #VT_FIX
            bcc         @num
            ldy         #0                                  ; (A VT_FIX: its bytes at nba, its sign its digits')
            jsr         operand
            ldy         #4
            lda         (vt),y
            bpl         :+
            dec         nsign
:
            jmp         @call
@int:
            ldy         #4                                  ; (An integer's sign)
            lda         (vt),y
            bpl         :+
            dec         nsign
:
            ldy         #4
:
            lda         (vt),y
            sta         t0 - 1,y
            dey
            bne         :-
            LDW         np, nia
            jsr         int_sf
            LDW         r0, nia
            bra         @call
@num:
            ldy         #1                                  ; (A VT_NUM: its bytes copied to the text bank's end)
            lda         (vt),y
            sta         ca
            iny
            lda         (vt),y
            sta         ca + 1
            iny
            lda         (vt),y
            sta         cal
            jsr         ca_sel
            jsr         ca_rd
            sta         cn
            jsr         ca_rd
            sta         cn + 1
            LDW         cb, TX_NUM
            lda         #LB_TXB
            sta         cbl
            jsr         far_copy
            LDW         r0, TX_NUM
            lda         txb
            sta         RAM_BANK
            NUMCALL     NUM_KIND                            ; (Its sign: its real part's)
            stx         nsign
@call:
            lda         txb
            sta         RAM_BANK
            MOVW        r4, nt_base
            LDW         r2, $8000
            LDW         r3, TX_ROOM
            NUMCALL     NUM_DISPLAY
            bcc         :+
            jmp         n_fail
:
            jmp         code_sel

; .A = the sign of the number at vt ($FF, 0, 1: a complex number's real part's)
num_sign_vt:
            lda         (vt)
            beq         @int
            cmp         #VT_FIX                             ; (A VT_FIX: its digits', as an integer's)
            bcc         @num
@int:
            ldy         #4
            lda         (vt),y
            bmi         @neg
            dey
            ora         (vt),y
            dey
            ora         (vt),y
            dey
            ora         (vt),y
            beq         :+
            lda         #1
:
            rts
@neg:
            lda         #$FF
            rts
@num:
            ldy         #0
            jsr         operand
            NUMCALL     NUM_KIND
            jsr         code_sel
            txa
            rts

; ---- The fourth bank's functions (fn.inc: F_B4 on)
fn_disp4:
            tax
            jmp         (fn_jt4,x)

rt_todo4:
            lda         #73                                 ; (Advanced feature unavailable: not yet)
            ldx         #0
            jmp         b_error
