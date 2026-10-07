; ****************************************************************************
; random.s - HyForth's random numbers (/lib/forth/random.fl: lib random): the old HyForth's rand, rand32 and rseed,
; and hylang's random.  hylang's generator too: xorshift32 (x ^= x << 13, x ^= x >> 17, x ^= x << 5: every 32-bit
; value but 0, in turn), seeded as it loads from the clock and the ticks.

.include "forthlib.inc"

.bss
seed:       .res        4                                   ; The generator's state, low byte first (never 0)
.code

; Its start: the seed, the clock (seconds) and the ticks
lib_init:
            stx         xsave
            jsr         TIME
            jsr         TICKS
            eor         r0
            sta         seed
            txa
            eor         r0 + 1
            sta         seed + 1
            lda         r1
            sta         seed + 2
            lda         r1 + 1
            sta         seed + 3
            ldx         xsave
            jmp         seed_nz

            HEADER      "rand", 0
rand:                                                       ; ( -- u ): the next number, 16 bits
            jsr         next
            dex
            lda         seed
            sta         dlo,x
            lda         seed + 1
            sta         dhi,x
            rts

            HEADER      "rand32", 0
rand32:                                                     ; ( -- ud ): the next number, 32 bits
            jsr         rand
            dex
            lda         seed + 2
            sta         dlo,x
            lda         seed + 3
            sta         dhi,x
            rts

            HEADER      "rseed", 0
rseed:                                                      ; ( ud -- ): the generator started from ud (0: 1)
            lda         dlo + 1,x
            sta         seed
            lda         dhi + 1,x
            sta         seed + 1
            lda         dlo,x
            sta         seed + 2
            lda         dhi,x
            sta         seed + 3
            inx
            inx
            jmp         seed_nz

            HEADER      "random", 0
random:                                                     ; ( u -- u' ): from 0 to u - 1 (u 0: 0), as hylang's:
            jsr         rand                                ;   the high cell of rand * u
            jsr         umstar
            lda         dlo,x
            sta         dlo + 1,x
            lda         dhi,x
            sta         dhi + 1,x
            inx
            rts

; The seed made 1 if it's 0 (xorshift's one value it can't leave)
seed_nz:
            lda         seed
            ora         seed + 1
            ora         seed + 2
            ora         seed + 3
            bne         :+
            inc         seed
:
            rts

; The seed, the next: x ^= x << 13, x ^= x >> 17, x ^= x << 5.  Keeps .X
next:
            lda         seed                                ; x ^= x << 13: x << 8 (its bytes 0-2 as bytes 1-3: tmp2,
            sta         tmp2                                ;   tmp, tmp + 1; byte 0 is 0), then << 5
            lda         seed + 1
            sta         tmp
            lda         seed + 2
            sta         tmp + 1
            ldy         #5
:
            asl         tmp2
            rol         tmp
            rol         tmp + 1
            dey
            bne         :-
            lda         seed + 1
            eor         tmp2
            sta         seed + 1
            lda         seed + 2
            eor         tmp
            sta         seed + 2
            lda         seed + 3
            eor         tmp + 1
            sta         seed + 3
            lda         seed + 3                            ; x ^= x >> 17: x >> 16 (bytes 2-3 to 0-1), then >> 1
            lsr
            sta         tmp + 1
            lda         seed + 2
            ror
            eor         seed
            sta         seed
            lda         tmp + 1
            eor         seed + 1
            sta         seed + 1
            lda         seed                                ; x ^= x << 5
            sta         tmp
            lda         seed + 1
            sta         tmp + 1
            lda         seed + 2
            sta         tmp2
            lda         seed + 3
            sta         tmp2 + 1
            ldy         #5
:
            asl         tmp
            rol         tmp + 1
            rol         tmp2
            rol         tmp2 + 1
            dey
            bne         :-
            lda         seed
            eor         tmp
            sta         seed
            lda         seed + 1
            eor         tmp + 1
            sta         seed + 1
            lda         seed + 2
            eor         tmp2
            sta         seed + 2
            lda         seed + 3
            eor         tmp2 + 1
            sta         seed + 3
            rts
