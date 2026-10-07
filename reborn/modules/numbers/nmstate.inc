; ****************************************************************************
; nmstate.inc - the libraries' state, in their bank: INIT, which makes a bank theirs; the base (SET_BASE, GET_BASE);
; the random generator's seed (SEED).

; INIT: the bank made the libraries' (r13): the base decimal, the precision 12 digits, the random generator seeded
nm_init:
            jsr         nm_begin0
            lda         #'d'
            sta         st_base
            stz         st_base + 1
            lda         #DIGITS_START
            sta         st_digits
            jsr         rn_clock
            lda         #NUMBERS_MAGIC
            sta         st_magic
            clc
            jmp         nm_end

; SET_BASE: r0 = a base string (its bytes and a 0), the base from now on.  NE_BASE: one too long, or empty
nm_set_base:
            jsr         nm_begin
            MOVR        np, r0
            ldx         #0
@char:
            jsr         c_get
            sta         text,x                              ; (Kept aside till it's whole)
            cmp         #0
            beq         @end
            inx
            cpx         #BASE_MAX
            bcc         @char
@bad:
            lda         #NE_BASE
            jmp         nm_fail
@end:
            txa
            beq         @bad
:
            lda         text,x
            sta         st_base,x
            dex
            bpl         :-
            clc
            jmp         nm_end

; GET_BASE: the base's string at r2 (room r3), its 0 too.  OUT: .A/.X = its length.  NE_ROOM
nm_get_base:
            jsr         nm_begin
            ldx         #0
:
            lda         st_base,x
            beq         :+
            inx
            bra         :-
:
            stx         v_len                               ; (Its length; the room it needs one more)
            stz         v_len + 1
            inx
            lda         r3 + 1
            bne         @room
            cpx         r3
            beq         @room
            bcs         @short
@room:
            MOVR        np, r2
            ldx         #0
:
            lda         st_base,x
            jsr         c_put
            inx
            cmp         #0
            bne         :-
            lda         v_len
            ldx         #0
            clc
            jmp         nm_end
@short:
            lda         #NE_ROOM
            jmp         nm_fail

; SEED: r0 = a seed, the random generator's state (0: from the clock and the ticks)
nm_seed:
            jsr         nm_begin
            lda         r0
            ora         r0 + 1
            beq         @clock
            MOVR        st_rand, r0
            clc
            jmp         nm_end
@clock:
            jsr         rn_clock
            clc
            jmp         nm_end

; The random generator's state from the tick count and the clock (as hylang's was, with the clock too), never 0.
; Modifies .A, .X, .Y (the caller's r0-r3 kept: TIME's)
rn_clock:
            ldx         #7
:
            lda         r0,x
            sta         v_regs,x
            dex
            bpl         :-
            jsr         TICKS
            eor         #$5A
            sta         st_rand
            txa
            eor         #$A5
            sta         st_rand + 1
            jsr         TIME
            lda         st_rand
            eor         r0
            sta         st_rand
            lda         st_rand + 1
            eor         r0 + 1
            sta         st_rand + 1
            ora         st_rand
            bne         :+
            inc         st_rand
:
            ldx         #7
:
            lda         v_regs,x
            sta         r0,x
            dex
            bpl         :-
            rts
