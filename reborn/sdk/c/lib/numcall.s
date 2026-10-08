; ****************************************************************************
; numcall.s - a call into the number libraries (num.h; spec/numbers.def), for num.s's functions and printf's and
; scanf's numbers: the libraries found and readied at the first call, then an XCALL.
;   int num_init (void)     the libraries readied, if they aren't: the modules numbers and math found (MODINFO), two
;                           RAM banks of the program's taken (BANKS_ALLOC), the first the libraries' (NUM_INIT: the
;                           base decimal, 12 digits), the second printf's (its numbers' text).  0, or -1 (num_error
;                           NE_INIT: no numbers library, or no banks)
;   __num_ready             the same, from assembly.  OUT: C = 0; or C = 1, .A = NE_INIT.  Keeps r0-r6
;   __num_xcall             a call: __num_entry the entry (its offset in its library's table, from $A030; bit 7 the
;                           math library's), .A, .X, .Y and r0-r6 as it takes them.  OUT: as the entry gives back: C
;                           = 0, .A/.X its result's length; or C = 1, .A its error (num_error too).  Keeps r0-r3
;   __num_tbank, __num_mmod printf's bank, and the math library's module (0: none; after __num_ready)

            .export     _num_init, _num_error
            .export     __num_ready, __num_xcall, __num_entry, __num_tbank, __num_mmod

            .include    "zeropage.inc"
            .include    "hydra.inc"
            .include    "numbers.inc"

            .bss
_num_error: .res        1                                   ; The last failure's error (NE_*)
__num_entry: .res       1                                   ; A call's entry
__num_tbank: .res       1                                   ; printf's bank
__num_mmod: .res        1                                   ; The math library's module (0: none)
nmod:       .res        1                                   ; The numbers library's (0: not readied yet)
lbank:      .res        1                                   ; The libraries' bank (r13)
info:       .res        ME_SIZE                             ; (A module's MODINFO ...
idx:        .res        1                                   ;   its entry ...
save:       .res        14                                  ;   r0-r6 kept while the libraries are readied)
in_a:       .res        1                                   ; A call's .A, .X, .Y
in_x:       .res        1
in_y:       .res        1

            .code

; int num_init (void)
_num_init:
            jsr         __num_ready
            lda         #0
            tax
            bcc         :+
            dex
            txa
:
            rts

__num_ready:
            lda         nmod                                ; (Ready: the numbers library's module found, and
            beq         :+                                  ;   its bank taken)
            clc
            rts
:
            ldx         #13                                 ; (r0-r6 kept)
:
            lda         r0,x
            sta         save,x
            dex
            bpl         :-
            stz         nmod
            stz         __num_mmod
            stz         idx
@find:
            lda         #<info
            sta         r0
            lda         #>info
            sta         r0 + 1
            lda         idx
            jsr         MODINFO
            bcs         @found                              ; (Past the last)
            lda         info + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @next
            ldx         #<(s_numbers - names)
            jsr         is_name
            bne         :+
            lda         info + ME_BANK
            sta         nmod
:
            ldx         #<(s_math - names)
            jsr         is_name
            bne         @next
            lda         info + ME_BANK
            sta         __num_mmod
@next:
            inc         idx
            bra         @find
@found:
            lda         nmod
            beq         @none
            lda         #2                                  ; Two banks: the libraries', printf's
            jsr         BANKS_ALLOC
            bcs         @none
            sta         lbank
            sta         r13
            inc         a
            sta         __num_tbank
            lda         nmod
            sta         r14
            lda         #<NUM_INIT
            sta         r15
            lda         #>NUM_INIT
            sta         r15 + 1
            jsr         XCALL
            jsr         restore
            clc
            rts
@none:
            stz         nmod
            jsr         restore
            lda         #NE_INIT
            sta         _num_error
            sec
            rts

restore:
            ldx         #13
:
            lda         save,x
            sta         r0,x
            dex
            bpl         :-
            rts

; Is info's name the one at names,x (a 0 after it)?  OUT: Z = 1 yes
is_name:
            ldy         #0
:
            lda         names,x
            cmp         info + ME_NAME,y
            bne         :+
            inx
            iny
            cmp         #0
            bne         :-
:
            rts

__num_xcall:
            sta         in_a
            stx         in_x
            sty         in_y
            jsr         __num_ready
            bcs         @rts
            lda         __num_entry
            and         #$7F
            clc
            adc         #<NUM_INIT                          ; (Each library's table at $A030: NUM_INIT's,
            sta         r15                                 ;   MATH_DIGITS's)
            lda         #>NUM_INIT
            adc         #0
            sta         r15 + 1
            lda         nmod
            bit         __num_entry
            bpl         :+
            lda         __num_mmod                          ; (No math library: NE_INIT)
            bne         :+
            lda         #NE_INIT
            bra         @fail
:
            sta         r14
            lda         lbank
            sta         r13
            lda         in_a
            ldx         in_x
            ldy         in_y
            jsr         XCALL
            bcc         @rts
@fail:
            sta         _num_error
            sec
@rts:
            rts

            .rodata
names:
s_numbers:  .byte       "numbers", 0
s_math:     .byte       "math", 0
