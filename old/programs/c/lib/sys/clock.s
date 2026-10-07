; ****************************************************************************
; clock.s - clock_t clock (void): the time since the program started, in the scheduler's ticks (CLOCKS_PER_SEC,
; hydra.h: 200 a second).  The tick count (TICKS_GET) wraps after about 5.5 minutes; clock counts on past that as
; long as it's called at least that often.

        .export     _clock
        .constructor initclock

        .include    "zeropage.inc"
        .include    "hydra.inc"

        .segment    "ONCE"

initclock:
        jsr         TICKS_GET                           ; (The start: clock is 0 here)
        sta         last
        sty         last + 1
        rts

        .code

_clock:
        jsr         TICKS_GET                           ; The ticks since the last look ...
        pha
        sec
        sbc         last
        sta         tmp1
        tya
        sbc         last + 1
        sta         tmp2
        sty         last + 1
        pla
        sta         last
        clc                                             ; ... added to the count
        lda         count
        adc         tmp1
        sta         count
        lda         count + 1
        adc         tmp2
        sta         count + 1
        bcc         :+
        inc         count + 2
        bne         :+
        inc         count + 3
:
        lda         count + 2
        sta         sreg
        lda         count + 3
        sta         sreg + 1
        lda         count
        ldx         count + 1
        rts

        .bss
last:       .res    2
count:      .res    4
