; ****************************************************************************
; xorshift.s - dice.h's random numbers: hylang's and HyForth's generator (forthlib/random.s), xorshift32 (x ^= x
; << 13, x ^= x >> 17, x ^= x << 5: every 32-bit value but 0, in turn).  rand16 and rand32 the next number; rseed
; the generator started from a number; random (n) the high 16 bits of rand16 () * n.  The first number, if rseed
; hasn't been called, seeds it from the clock and the ticks first (dice.c's _rseed_clock).

            .export     _rand16, _rand32, _rseed, _random, _xs_seed
            .import     __rseed_clock
            .importzp   sreg, tmp1, tmp2, tmp3, tmp4, ptr1

            .bss
_xs_seed:   .res        4                       ; The state, low byte first (never 0 once seeded; 0: not yet)

            .code

; unsigned rand16 (void)
_rand16:
            jsr         next
            lda         _xs_seed
            ldx         _xs_seed + 1
            rts

; unsigned long rand32 (void)
_rand32:
            jsr         next
            lda         _xs_seed + 2
            sta         sreg
            lda         _xs_seed + 3
            sta         sreg + 1
            lda         _xs_seed
            ldx         _xs_seed + 1
            rts

; void __fastcall__ rseed (unsigned long seed): (.A, .X, sreg)
_rseed:
            sta         _xs_seed
            stx         _xs_seed + 1
            lda         sreg
            sta         _xs_seed + 2
            lda         sreg + 1
            sta         _xs_seed + 3
            ora         _xs_seed + 2                ; (0: 1, xorshift's one value it can't leave)
            ora         _xs_seed + 1
            ora         _xs_seed
            bne         :+
            inc         _xs_seed
:           rts

; unsigned __fastcall__ random (unsigned n): the high 16 bits of rand16 () * n
_random:
            sta         ptr1                        ; n
            stx         ptr1 + 1
            jsr         next
            lda         #0                          ; tmp1-tmp4: the product, low byte first
            sta         tmp3
            sta         tmp4
            lda         _xs_seed                    ; tmp1, tmp2: the multiplier, shifted out as the product
            sta         tmp1                        ;   comes in above it (tmp4 ... tmp1 shifted right 16
            lda         _xs_seed + 1                ;   times, n added to its high half first each time the
            sta         tmp2                        ;   bit going out is 1)
            ldy         #16
@bit:       lda         tmp1
            lsr
            bcc         @no                         ; (C: 0, or the add's carry, goes in at the top)
            clc
            lda         tmp3
            adc         ptr1
            sta         tmp3
            lda         tmp4
            adc         ptr1 + 1
            sta         tmp4
@no:        ror         tmp4
            ror         tmp3
            ror         tmp2
            ror         tmp1
            dey
            bne         @bit
            lda         tmp3
            ldx         tmp4
            rts

; The state, the next: x ^= x << 13, x ^= x >> 17, x ^= x << 5 (seeded from the clock first if it's 0)
next:
            lda         _xs_seed
            ora         _xs_seed + 1
            ora         _xs_seed + 2
            ora         _xs_seed + 3
            bne         :+
            jsr         __rseed_clock
:           lda         _xs_seed                    ; x ^= x << 13: x << 8 (its bytes 0-2 as bytes 1-3: tmp1,
            sta         tmp1                        ;   tmp2, tmp3; byte 0 is 0), then << 5
            lda         _xs_seed + 1
            sta         tmp2
            lda         _xs_seed + 2
            sta         tmp3
            ldy         #5
:           asl         tmp1
            rol         tmp2
            rol         tmp3
            dey
            bne         :-
            lda         _xs_seed + 1
            eor         tmp1
            sta         _xs_seed + 1
            lda         _xs_seed + 2
            eor         tmp2
            sta         _xs_seed + 2
            lda         _xs_seed + 3
            eor         tmp3
            sta         _xs_seed + 3
            lda         _xs_seed + 3                ; x ^= x >> 17: x >> 16 (bytes 2-3 to 0-1), then >> 1
            lsr
            sta         tmp2
            lda         _xs_seed + 2
            ror
            eor         _xs_seed
            sta         _xs_seed
            lda         tmp2
            eor         _xs_seed + 1
            sta         _xs_seed + 1
            lda         _xs_seed                    ; x ^= x << 5
            sta         tmp1
            lda         _xs_seed + 1
            sta         tmp2
            lda         _xs_seed + 2
            sta         tmp3
            lda         _xs_seed + 3
            sta         tmp4
            ldy         #5
:           asl         tmp1
            rol         tmp2
            rol         tmp3
            rol         tmp4
            dey
            bne         :-
            lda         _xs_seed
            eor         tmp1
            sta         _xs_seed
            lda         _xs_seed + 1
            eor         tmp2
            sta         _xs_seed + 1
            lda         _xs_seed + 2
            eor         tmp3
            sta         _xs_seed + 2
            lda         _xs_seed + 3
            eor         tmp4
            sta         _xs_seed + 3
            rts
