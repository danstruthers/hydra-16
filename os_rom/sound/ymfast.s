.debuginfo

; ****************************************************************************
; The YM2151's interrupt: the sound clock (BIOS ROM page 2, included inside `.scope PAGE2`, see all.s: the fast
; interrupt handlers are there, serfast.s).  Timer B, run by the sound driver for a song player (SND_CTL_CLOCK:
; snd_lib.s's SND_CLOCK_SET), interrupts at the song's tick rate: this counts the ticks (SND_CLK) and wakes the
; player when its time (its ZSM_AT) has come, asking for a task switch, as the system's tick does its sleepers.
;   The rate is exact on average: a period is K or K + 1 units of 1024 of the chip's clocks (286 us), as a
; 16-bit fraction carries (SND_CLK_F, SND_CLK_ACC); timer B reloads $12 at each overflow, so each interrupt sets
; the period after the one that's begun.
;   Like the VIA's and the ACIA's, its vector is a fast stub (COMMON: YM_IRQ_STUB), with no dispatcher: 150-300
; cycles with interrupts off, two register writes to the chip among them (the flag's reset, and $12 when the
; period changes; the chip may be busy for up to 64 cycles from the sound driver's last write).
; IN (from IRQ_FAST_P2): .X = the interrupted ROM page; the interrupted .A, .X and .Y on the stack.

.segment "IO_P2"

YM_FAST_WAIT = 64 * CPU_CLOCK_MULT                          ; (The chip's busy time, and some: then on anyway)

; Wait for the chip, then register .X = .A.  Modifies .Y
.macro YM_FAST_WRITE
            ldy         #YM_FAST_WAIT                       ; (Anonymous labels: the handler's @ ones stay in scope)
:
            dey
            bmi         :+
            bit         YM_DATA
            bmi         :-
:
            stx         YM_REG
            sta         YM_DATA
.endmacro

YM_IRQ_FAST:
            ldy         T_REGISTER                          ; The interrupted task
            lda         #SOUND_TASK_NUM
            sta         T_REGISTER                          ; Quick switch to the sound task (no stack use!)
            sty         SND_IRQ_T
            stx         SND_IRQ_W
            lda         SND_CLK_OWNER
            beq         @stray                              ; (No clock: a timer the clients started)
            clc                                             ; The next period: long when the fraction carries
            lda         SND_CLK_ACC
            adc         SND_CLK_F
            sta         SND_CLK_ACC
            lda         SND_CLK_ACC + 1
            adc         SND_CLK_F + 1
            sta         SND_CLK_ACC + 1
            lda         SND_CLK_NB1
            bcs         :+
            inc                                             ; (Short: one unit less)
:
            cmp         SND_CLK_LAST
            beq         @same
            sta         SND_CLK_LAST
            ldx         #$12
            YM_FAST_WRITE

@same:
            lda         SND_R14                             ; Timer B's flag reset (it runs on, its interrupt on)
            ora         #$2A
            ldx         #$14
            YM_FAST_WRITE
            inc         SND_CLK                             ; A tick
            bne         :+
            inc         SND_CLK + 1
:
            ldx         SND_CLK_WAIT                        ; The player's time come?
            bmi         @done
            lda         SND_CLK
            ldy         SND_CLK + 1
            stx         T_REGISTER                          ; Quick look at it (no stack use!)
            sec                                             ; Now - its time: negative, not yet
            sbc         ZSM_AT
            tya
            sbc         ZSM_AT + 1
            bmi         @not_yet
            rmb2        TASK_STATUS_REG                     ; Wake it (as IO_WAKE: TASK_WAITING_FLAG)
            lda         #SOUND_TASK_NUM
            sta         T_REGISTER
            lda         #$FF
            sta         SND_CLK_WAIT
            ldx         SND_IRQ_W
            ldy         SND_IRQ_T
            sty         T_REGISTER                          ; Back to the interrupted task (and its stack)
            ply
            lda         #IRQ_TICK                           ; The dispatcher: a task switch, so it runs now
            jmp         IRQ_FAST_SLOW

@stray:                                                     ; Both flags reset (a client's timer: its interrupt
            lda         SND_R14                             ;   isn't on, but in case)
            ora         #$30
            ldx         #$14
            YM_FAST_WRITE
            bra         @done

@not_yet:
            lda         #SOUND_TASK_NUM
            sta         T_REGISTER

@done:
            ldx         SND_IRQ_W
            ldy         SND_IRQ_T
            sty         T_REGISTER                          ; Back to the interrupted task (and its stack)
            ply
            txa
            jmp         IRQ_EXIT
