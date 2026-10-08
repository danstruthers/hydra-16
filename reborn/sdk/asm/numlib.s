; ****************************************************************************
; numlib.s - the number libraries, for a program in assembly: included at the end of its source (as toollib.s is),
; with numbers.inc at its top.  The libraries (spec/numbers.def; the plan docs/design/plans/NUMBERS.md) are modules
; of the paged ROM, numbers (the number system: arithmetic, conversions, text in every base, bits) and math (its
; functions); a call is an XCALL, which numbers.inc's macros make.
;   num_open    the libraries found (MODINFO: the modules named numbers and math) and readied: a RAM bank of the
;               program's (BANKS_ALLOC) made theirs (NUM_INIT: the base decimal, 12 digits).  OUT: C = 0; or C = 1
;               and .A the error: E_NOENT (no numbers library), or BANKS_ALLOC's
; Then NUMCALL NUM_ADD (an entry of the numbers library's) and MATHCALL MATH_SQRT (the math library's), .A, .X, .Y
; and r0-r6 as the entry takes them, with num_bank, num_mod and math_mod, the bytes num_open sets (math_mod 0: there's
; no math library).  A call keeps r0-r3; its result is a number in the stored format at r2, its length .A/.X.
; It uses: r0 (num_open's), and its bytes below.

.pushseg

.bss
num_bank:   .res        1                                   ; The libraries' RAM bank (r13: NUM_INIT's) ...
num_mod:    .res        1                                   ;   the numbers library's module (its paged ROM bank:
math_mod:   .res        1                                   ;   r14), and the math library's (0: none)
nl_info:    .res        ME_SIZE                             ; (num_open's: a module's MODINFO ...
nl_i:       .res        1                                   ;   and its entry)

.code

num_open:
            stz         num_mod
            stz         math_mod
            stz         nl_i
@find:
            LDR         r0, nl_info
            lda         nl_i
            jsr         MODINFO
            bcs         @found                              ; (Past the last: C = 1)
            lda         nl_info + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @next
            ldx         #<(nl_snum - nl_names)
            jsr         nl_is
            bne         :+
            lda         nl_info + ME_BANK
            sta         num_mod
:
            ldx         #<(nl_smath - nl_names)
            jsr         nl_is
            bne         @next
            lda         nl_info + ME_BANK
            sta         math_mod
@next:
            inc         nl_i
            bra         @find
@found:
            lda         num_mod
            bne         :+
            lda         #E_NOENT
            sec
            rts
:
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            sta         num_bank
            NUMCALL     NUM_INIT
            clc
@rts:
            rts

; Is nl_info's name the one at nl_names,x (a 0 after it)?  OUT: Z = 1 yes
nl_is:
            ldy         #0
:
            lda         nl_names,x
            cmp         nl_info + ME_NAME,y
            bne         :+
            inx
            iny
            cmp         #0
            bne         :-
:
            rts

.rodata
nl_names:
nl_snum:    .byte       "numbers", 0
nl_smath:   .byte       "math", 0

.popseg
