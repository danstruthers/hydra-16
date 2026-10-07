.debuginfo

; ****************************************************************************
; The hardware test's device tests: the interrupt lines and the vector RAM, the VIA, the YM2151 and the CPU
; clock, the ACIA, the SPI devices, the I2C bus and the slots.  (Inside `.scope HWTEST`: hwtest.s.)
;
; The interrupt tests point every vector RAM entry at a stub of its own (HWT_IRQ_STUBS), which notes the
; entry and returns with IRQs off (the I flag set in the P it pulls), so each CLI lets one interrupt in.

.segment "HWT_C"

; ****************************************************************************
; Interrupts

; A stub for each vector RAM entry: its number, to HWT_IRQ
HWT_IRQ_STUBS:
.repeat 16, i
            pha
            lda         #i
            bra         HWT_IRQ
.endrepeat
HWT_IRQ_STUB_SIZE   = 5         ; (HWT_VECTORS_SET: entry x 5)
.assert     * - HWT_IRQ_STUBS = 16 * HWT_IRQ_STUB_SIZE, error, "HWT_IRQ_STUBS: 5 bytes each"

; An interrupt (or a BRK): its entry (HWT_IRQ_E), the P it pushed (HWT_IRQ_P), counted (HWT_IRQ_N); it
; returns with the I flag set
HWT_IRQ:
            sta         HWT_IRQ_E
            inc         HWT_IRQ_N
            phx
            tsx
            lda         $0103,X                             ; (Its P: above .X and .A)
            sta         HWT_IRQ_P
            ora         #$04
            sta         $0103,X
            plx
            pla
            rti

; Every vector RAM entry to its stub (with no IRQ line active, entry V is written).  Modifies: .A, .X
HWT_VECTORS_SET:
            ldx         #15
@entry:
            stx         V_REGISTER
            txa
            asl
            asl
            sta         HWT_T7
            txa
            clc
            adc         HWT_T7                              ; (Entry x 5)
            adc         #<HWT_IRQ_STUBS
            sta         $FFFE
            lda         #>HWT_IRQ_STUBS
            adc         #0
            sta         $FFFF
            dex
            bpl         @entry
            rts

; Let one interrupt in: CLI, then wait for it (about 20,000 cycles at most).  OUT: C = 1: none came; C = 0:
; .A = its entry.  Modifies: .A, .X, .Y
HWT_IRQ_WAIT:
            stz         HWT_IRQ_N
            ldx         #0
            ldy         #0
            cli
@wait:
            lda         HWT_IRQ_N
            bne         @came
            dey
            bne         @wait
            inx
            cpx         #8
            bne         @wait
            sei
            sec
            rts
@came:
            sei
            lda         HWT_IRQ_E
            clc
            rts

; Let an interrupt in, and check it's the one expected: entry .A, from the device .X names (an offset in
; HWT_IRQ_NAMES).  A fault: FAIL, the device, and what came: no IRQ, or the line one came on.
; Modifies: .A, .X, .Y
HWT_IRQ_CHECK:
            sta         HWT_T4
            stx         HWT_T5
            jsr         HWT_IRQ_WAIT
            bcs         @none
            cmp         HWT_T4
            bne         @wrong
            rts
@none:
            jsr         HWT_IRQ_WHO
            jsr         HWT_PRINT
            .byte       " no IRQ", 0
            rts
@wrong:
            jsr         HWT_IRQ_WHO
            jmp         HWT_IRQ_LINE

; No interrupt may come now (nothing's been asked to interrupt): one that does is FAIL "unasked", its line
; (and the YM2151's status, if it's its line).  Modifies: .A, .X, .Y
HWT_IRQ_NONE:
            jsr         HWT_IRQ_WAIT
            bcs         :+
            jsr         HWT_FAIL
            .byte       "unasked:", 0
            jmp         HWT_IRQ_LINE
:
            rts

; " IRQ on line N" for the interrupt that came (HWT_IRQ_E), and if it's the YM2151's line (4), its status
; (bit 0: timer A's flag, bit 1: timer B's, bit 7: busy).  Modifies: .A
HWT_IRQ_LINE:
            jsr         HWT_PRINT
            .byte       " IRQ on line ", 0
            lda         HWT_IRQ_E
            eor         #7                                  ; (The line: the entry ^ 7)
            pha
            jsr         HWT_HEX1
            pla
            cmp         #4
            bne         :+
            jsr         HWT_PRINT
            .byte       " (YM2151 status ", 0
            lda         YM_DATA
            jsr         HWT_HEX2
            lda         #')'
            jmp         HWT_PUTC
:
            rts

; FAIL, then "all at once:" if HWT_T6 <> 0, and the name of device HWT_T5.  Modifies: .A, .X, .Y
HWT_IRQ_WHO:
            jsr         HWT_FAIL
            .byte       0
            lda         HWT_T6
            beq         :+
            jsr         HWT_PRINT
            .byte       "all at once: ", 0
:
            ldx         HWT_T5
            lda         HWT_IRQ_NAMES,X
            ldy         HWT_IRQ_NAMES + 1,X
            jmp         HWT_PUTS_AY

HWT_IRQ_NAMES:  .word   HWT_IN_VIA, HWT_IN_ACIA, HWT_IN_YM
HWT_IN_VIA:     .byte   "VIA", 0
HWT_IN_ACIA:    .byte   "ACIA", 0
HWT_IN_YM:      .byte   "YM2151", 0

; The VIA's timer 1 asks for an interrupt ($40 cycles on, one-shot); and the VIA quiet again
HWT_IRQ_VIA:
            lda         #$40
            sta         VIA_R_T1C_L
            stz         VIA_R_T1C_H                         ; (Starts it)
            lda         #VIA_INT_ENABLE | VIA_T1_INT_BIT
            sta         VIA_R_INT_ENABLE
            rts
HWT_VIA_OFF:
            lda         #$7F
            sta         VIA_R_INT_ENABLE
            sta         VIA_R_INT_FLAGS
            rts

; The ACIA asks for an interrupt: its transmitter's.  With the transmitter idle, its interrupt on, then a
; space sent: TDRE goes on again as the space moves to the shift register, a bit's time later (the
; Rockwell 65C51 interrupts as TDRE goes on: turning its interrupt on while it's already on isn't enough).
; And not again.  Don't read its status after this: that clears its interrupt
HWT_IRQ_ACIA:
            jsr         HWT_TX_IDLE
            lda         #ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLIE | ACIA_CMD_BIT_RID
            sta         ACIA_R_CMD
            lda         #' '
            sta         ACIA_R_DATA
            inc         HWT_COL
            rts
HWT_ACIA_OFF:
            lda         #HWT_ACIA_CMD
            sta         ACIA_R_CMD
            lda         ACIA_R_STATUS                       ; (Clears its IRQ flag)
            rts

; The YM2151 asks for an interrupt: its timer A, every 64 of its clocks; and its timers stopped
HWT_IRQ_YM:
            lda         #$10
            ldx         #$FF
            jsr         HWT_YM_SET
            lda         #$11
            ldx         #$03
            jsr         HWT_YM_SET
            lda         #$14                                ; Timer A: loaded, its flag on, reset
            ldx         #$15
            jmp         HWT_YM_SET
HWT_YM_OFF:
            lda         #$14
            ldx         #$30
            jmp         HWT_YM_SET

; ****************************************************************************
; Interrupts: no IRQ line active with the devices quiet; the vector RAM (each entry, by V); then BRK (entry
; V), and the VIA's, the ACIA's and the YM2151's interrupts, each on its own line, first one at a time and
; then all at once (they must come in their lines' order).  The slots' lines can't be tested without cards.
HWT_T_IRQS:
            jsr         HWT_VECTORS_SET                     ; Nothing asks for an interrupt: none comes
            jsr         HWT_IRQ_WAIT
            bcs         @vectors
            jsr         HWT_FAIL
            .byte       "an IRQ line is held active", 0
            rts

@vectors:
            stz         HWT_T3                              ; Each entry: (entry x $11) ^ HWT_T3, and that ^ $A5; then again
@pass:
            ldx         #15                                 ;   with HWT_T3 = $FF
:
            stx         V_REGISTER
            jsr         HWT_VEC_VALUE
            sta         $FFFE
            eor         #$A5
            sta         $FFFF
            dex
            bpl         :-
            ldx         #15
@check:
            stx         V_REGISTER
            jsr         HWT_VEC_VALUE
            cmp         $FFFE
            bne         @bad
            eor         #$A5
            cmp         $FFFF
            bne         @bad
            dex
            bpl         @check
            lda         HWT_T3
            eor         #$FF
            sta         HWT_T3
            bne         @pass
            bra         @sources

@bad:
            jsr         HWT_FAIL
            .byte       "vector entry ", 0
            txa
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       " wrote ", 0
            jsr         HWT_VEC_VALUE
            pha
            eor         #$A5
            jsr         HWT_HEX2
            pla
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       " read ", 0
            lda         $FFFF
            jsr         HWT_HEX2
            lda         $FFFE
            jmp         HWT_HEX2

@sources:
            jsr         HWT_VECTORS_SET
            stz         HWT_T6                              ; (One at a time)
            lda         #IRQ_NUMBER_SW                      ; BRK: the entry V selects
            sta         V_REGISTER
            stz         HWT_IRQ_N
            brk
            .byte       0                                   ; (Its signature byte: RTI returns after it)
            lda         HWT_IRQ_N
            beq         @brk_bad
            lda         HWT_IRQ_E
            cmp         #IRQ_NUMBER_SW
            bne         @brk_bad
            lda         HWT_IRQ_P
            and         #$10                                ; (B: set in the P a BRK pushes)
            bne         @each
@brk_bad:
            jsr         HWT_FAIL
            .byte       "BRK", 0

@each:
            jsr         HWT_IRQ_VIA                         ; Each on its own
            lda         #IRQ_NUMBER_ONBOARD_VIA
            ldx         #0
            jsr         HWT_IRQ_CHECK
            jsr         HWT_VIA_OFF
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL                        ; (The WDC 65C51's transmitter can't interrupt)
            jsr         HWT_IRQ_NONE                        ; (Nothing asking before it's asked)
            jsr         HWT_IRQ_ACIA
            lda         #IRQ_NUMBER_ONBOARD_SERIAL
            ldx         #2
            jsr         HWT_IRQ_CHECK
            jsr         HWT_ACIA_OFF
.endif
            jsr         HWT_IRQ_NONE
            jsr         HWT_IRQ_YM
            lda         #IRQ_NUMBER_ONBOARD_SOUND
            ldx         #4
            jsr         HWT_IRQ_CHECK
            jsr         HWT_YM_OFF

            inc         HWT_T6                              ; All at once: the lowest line first
            jsr         HWT_IRQ_VIA
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL
            jsr         HWT_IRQ_ACIA
.endif
            jsr         HWT_IRQ_YM
            ldx         #(2 * SER_CHAR_CYCLES + 1279) / 1280  ; (Till they're all asking: the ACIA's space
            ldy         #0                                  ;   can take a character's time, twice as
                                                            ;   many cycles with the CPU at 7.16 MHz)
:
            dey
            bne         :-
            dex
            bne         :-
            lda         #IRQ_NUMBER_ONBOARD_VIA
            ldx         #0
            jsr         HWT_IRQ_CHECK
            jsr         HWT_VIA_OFF
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL
            lda         #IRQ_NUMBER_ONBOARD_SERIAL
            ldx         #2
            jsr         HWT_IRQ_CHECK
            jsr         HWT_ACIA_OFF
.endif
            lda         #IRQ_NUMBER_ONBOARD_SOUND
            ldx         #4
            jsr         HWT_IRQ_CHECK
            jmp         HWT_YM_OFF

; The vector RAM test's value for entry .X: its low byte, (.X x $11) ^ HWT_T3.  Preserves .X, .Y
HWT_VEC_VALUE:
            txa
            asl
            asl
            asl
            asl
            stx         HWT_T7
            ora         HWT_T7
            eor         HWT_T3
            rts

; ****************************************************************************
; The VIA: its registers (timer 1's latches, the shift register, IER); timer 1 counting at the CPU's clock,
; its flag one-shot and free-running (and IFR bit 7); timer 2's flag; the shift register's (mode 6: out at
; the CPU's clock).  Port A is tested as the I2C bus, port B as SPI.
HWT_T_VIA:
            jsr         HWT_VIA_REGS
            jsr         HWT_VIA_IER
            jsr         HWT_VIA_T1
            jsr         HWT_VIA_T2
            jmp         HWT_VIA_SR

HWT_VIA_REGS:
            ldx         #HWT_PATTERNS_N - 1                 ; (ACR 0: the timers one-shot, the shift register off)
@reg:
            lda         HWT_PATTERNS,X
            sta         VIA_R_T1L_L
            eor         #$FF
            sta         VIA_R_T1L_H
            eor         #$5A
            sta         VIA_R_SHIFT_REG
            lda         HWT_PATTERNS,X
            cmp         VIA_R_T1L_L
            bne         @latch
            eor         #$FF
            cmp         VIA_R_T1L_H
            bne         @latch
            eor         #$5A
            cmp         VIA_R_SHIFT_REG
            bne         @sr
            dex
            bpl         @reg
            rts
@latch:
            jsr         HWT_FAIL
            .byte       "T1 latch", 0
            rts
@sr:
            jsr         HWT_FAIL
            .byte       "shift register", 0
            rts

HWT_VIA_IER:
            lda         #$7F
            sta         VIA_R_INT_FLAGS
            ldx         #0                                  ; (Pairs: what's written, what it reads as)
:
            lda         HWT_IER_TRIES,X
            sta         VIA_R_INT_ENABLE
            lda         VIA_R_INT_ENABLE
            cmp         HWT_IER_TRIES + 1,X
            bne         @bad
            inx
            inx
            cpx         #HWT_IER_TRIES_N
            bne         :-
            rts
@bad:
            pha
            jsr         HWT_FAIL
            .byte       "IER wrote ", 0
            lda         HWT_IER_TRIES,X
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       " read ", 0
            pla
            jsr         HWT_HEX2
            jmp         HWT_VIA_OFF

HWT_IER_TRIES:  .byte   $FF, $FF, $55, $AA, $7F, $80, $D5, $D5, $2A, $D5, $7F, $80
HWT_IER_TRIES_N = * - HWT_IER_TRIES

HWT_VIA_T1:
            lda         #$FF                                ; Counting: from $FFFF, for about 890 cycles
            sta         VIA_R_T1L_L
            sta         VIA_R_T1C_H                         ; (Starts it)
            ldx         #177
:
            dex
            bne         :-
            lda         VIA_R_T1C_L
            ldy         VIA_R_T1C_H
            eor         #$FF                                ; (Counted: $FFFF - it)
            sta         HWT_NUM
            tya
            eor         #$FF
            sta         HWT_NUM + 1
            lda         HWT_NUM                             ; 860 to 1,100 (a branch across a page adds 177)
            cmp         #<860
            lda         HWT_NUM + 1
            sbc         #>860
            bcc         @count_bad
            lda         HWT_NUM
            cmp         #<1101
            lda         HWT_NUM + 1
            sbc         #>1101
            bcc         @one_shot
@count_bad:
            jsr         HWT_FAIL
            .byte       "T1 counted ", 0
            lda         HWT_NUM
            ldy         HWT_NUM + 1
            jsr         HWT_DEC_AY
            jsr         HWT_PRINT
            .byte       " in 890 cycles", 0

@one_shot:
            lda         #$7F                                ; One-shot: its flag, once
            sta         VIA_R_INT_FLAGS
            lda         #$20
            sta         VIA_R_T1C_L
            stz         VIA_R_T1C_H
            lda         #VIA_T1_INT_BIT
            jsr         HWT_VIA_FLAG
            bcc         :+
            jsr         HWT_FAIL
            .byte       "T1 no flag", 0
            rts
:
            lda         VIA_R_T1C_L                         ; (Reading T1C-L clears it)
            lda         #VIA_T1_INT_BIT
            bit         VIA_R_INT_FLAGS
            beq         :+
            jsr         HWT_FAIL
            .byte       "T1 flag stays set", 0
            rts
:
            lda         #$40                                ; Free-running: its flag again and again, and (IER
            sta         VIA_R_AUX_CTRL                      ;   on) IFR bit 7
            lda         #$FF
            sta         VIA_R_T1C_L
            stz         VIA_R_T1C_H
            lda         #VIA_INT_ENABLE | VIA_T1_INT_BIT
            sta         VIA_R_INT_ENABLE
            lda         #3
            sta         HWT_T3
@again:
            lda         #VIA_T1_INT_BIT
            jsr         HWT_VIA_FLAG
            bcs         @free_bad
            lda         VIA_R_INT_FLAGS
            bpl         @free_bad
            lda         VIA_R_T1C_L
            dec         HWT_T3
            bne         @again
            bra         @free_end
@free_bad:
            jsr         HWT_FAIL
            .byte       "T1 free-running", 0
@free_end:
            stz         VIA_R_AUX_CTRL
            jmp         HWT_VIA_OFF

HWT_VIA_T2:
            lda         #$20                                ; One-shot: its flag
            sta         VIA_R_T2C_L
            stz         VIA_R_T2C_H                         ; (Starts it)
            lda         #VIA_T2_INT_BIT
            jsr         HWT_VIA_FLAG
            bcc         :+
            jsr         HWT_FAIL
            .byte       "T2 no flag", 0
            rts
:
            lda         VIA_R_T2C_L                         ; (Reading T2C-L clears it)
            lda         #VIA_T2_INT_BIT
            bit         VIA_R_INT_FLAGS
            beq         :+
            jsr         HWT_FAIL
            .byte       "T2 flag stays set", 0
:
            rts

HWT_VIA_SR:
            lda         #$18                                ; Mode 6: 8 bits out at the CPU's clock
            sta         VIA_R_AUX_CTRL
            lda         #$A5
            sta         VIA_R_SHIFT_REG                     ; (Starts it)
            lda         #$04
            jsr         HWT_VIA_FLAG
            stz         VIA_R_AUX_CTRL
            lda         VIA_R_SHIFT_REG                     ; (Clears its flag)
            bcc         :+
            jsr         HWT_FAIL
            .byte       "shift register no flag", 0
:
            rts

; Wait for a VIA flag (.A, in IFR): about 11,000 cycles at most.  OUT: C = 0: it's set; C = 1: it never was.
; Preserves .A.  Modifies: .X, .Y
HWT_VIA_FLAG:
            ldx         #0
            ldy         #4
:
            bit         VIA_R_INT_FLAGS
            bne         @set
            dex
            bne         :-
            dey
            bne         :-
            sec
            rts
@set:
            clc
            rts

; ****************************************************************************
; The YM2151: its busy flag (clear, set by a write, clear again), and its timers' flags.  (Its IRQ: the
; interrupts test.  Its sound can't be tested here: listen to it with HyForth's sound words.)
HWT_T_YM:
            jsr         HWT_YM_WAIT
            bcc         :+
            jsr         HWT_FAIL
            .byte       "always busy (no chip?)", 0
            rts
:
            lda         #$14
            sta         YM_REG
            jsr         HWT_YM_WAIT
            lda         #$30
            sta         YM_DATA
            lda         YM_DATA                             ; (Busy for 64 of its clocks after a write)
            bmi         :+
            jsr         HWT_FAIL
            .byte       "never busy", 0
            rts
:
            jsr         HWT_YM_WAIT
            bcc         :+
            jsr         HWT_FAIL
            .byte       "stays busy", 0
            rts
:
            lda         #$10                                ; Timer A: 64 x (1024 - $300) = 16,384 of its clocks
            ldx         #$C0
            jsr         HWT_YM_SET
            lda         #$11
            ldx         #$00
            jsr         HWT_YM_SET
            lda         #$14                                ; (Loaded, its flag on, reset)
            ldx         #$15
            jsr         HWT_YM_SET
            lda         #$01
            jsr         HWT_YM_FLAG
            bcc         :+
            jsr         HWT_FAIL
            .byte       "timer A", 0
            bra         @timer_b
:
            lda         #$14                                ; (Its flag reset: done once the chip isn't busy, or
            ldx         #$15                                ;   a while after, on some boards: then its IRQ
            jsr         HWT_YM_SET                          ;   line let go too)
            jsr         HWT_YM_WAIT
            ldy         #0
:
            lda         YM_DATA
            and         #$01
            beq         @timer_b
            dey
            bne         :-
            jsr         HWT_FAIL
            .byte       "timer A flag stays set", 0

@timer_b:
            lda         #$12                                ; Timer B: 1024 x (256 - $F0) = 16,384 of its clocks
            ldx         #$F0
            jsr         HWT_YM_SET
            lda         #$14                                ; (Loaded, its flag on, reset; timer A stopped)
            ldx         #$2A
            jsr         HWT_YM_SET
            lda         #$02
            jsr         HWT_YM_FLAG
            bcc         :+
            jsr         HWT_FAIL
            .byte       "timer B", 0
:
            jmp         HWT_YM_OFF

; Wait for a YM2151 status flag (.A): about 90,000 cycles at most.  OUT: C = 0: it's set; C = 1: it never
; was.  Preserves .A.  Modifies: .X, .Y
HWT_YM_FLAG:
            ldx         #0
            ldy         #32
:
            bit         YM_DATA
            bne         @set
            dex
            bne         :-
            dey
            bne         :-
            sec
            rts
@set:
            clc
            rts

; ****************************************************************************
; The CPU's clock: the VIA's timer 1 (the CPU's clock) over a period of the YM2151's timer A (SND_CLK,
; 3.58 MHz: 16,384 of its clocks).  4,096 cycles is 0.89 MHz, 8,192 1.79 MHz, 16,384 3.58 MHz and 32,768
; 7.16 MHz (the jumpers J6-J7); it must be the clock the ROM is built for (CPU_CLOCK_MULT).
HWT_CLOCK_NONE:                                                 ; (No timer: here, in its branches' range)
            jsr         HWT_YM_OFF
            jsr         HWT_FAIL
            .byte       "no YM2151 timer", 0
            rts

HWT_T_CLOCK:
            lda         #$14                                ; Timer A stopped, its flag reset
            ldx         #$30
            jsr         HWT_YM_SET
            lda         #$10
            ldx         #$C0
            jsr         HWT_YM_SET
            lda         #$11
            ldx         #$00
            jsr         HWT_YM_SET
            jsr         HWT_YM_WAIT                         ; Then started (loaded, its flag on), with VIA
            lda         #$14                                ;   timer 1 from $FFFF, and timed to its first
            sta         YM_REG                              ;   overflow.  (Nothing's written to the chip
            jsr         HWT_YM_WAIT                         ;   in between: on the board, writing $14 again
            ldx         #$FF                                ;   seems to restart the timer)
            stx         VIA_R_T1L_L
            lda         #$15
            sta         YM_DATA
            stx         VIA_R_T1C_H
            lda         #$01
            jsr         HWT_YM_FLAG
            lda         VIA_R_T1C_L
            ldy         VIA_R_T1C_H
            ldx         #$FF                                ; (Timer 1 again at once, for the next period)
            stx         VIA_R_T1L_L
            stx         VIA_R_T1C_H
            bcs         HWT_CLOCK_NONE
            eor         #$FF                                ; (Counted: $FFFF - it)
            sta         HWT_NUM
            tya
            eor         #$FF
            sta         HWT_NUM + 1
            lda         #$14                                ; The next period (shown, not judged), with its
            ldx         #$15                                ;   flag reset by writing $14 again: if that
            jsr         HWT_YM_SET                          ;   restarts the timer, it's longer than the
            lda         #$01                                ;   first by the write's time
            jsr         HWT_YM_FLAG
            lda         VIA_R_T1C_L
            ldy         VIA_R_T1C_H
            eor         #$FF
            sta         HWT_Q
            tya
            eor         #$FF
            sta         HWT_Q + 1
            jsr         HWT_YM_OFF
            lda         HWT_NUM + 1                         ; The count / 4,096, to the nearest; within 512
            clc                                             ;   cycles of it (3% at 3.58 MHz; the clocks
            adc         #$08                                ;   are a factor of 2 apart): (the count + $800)
            sta         HWT_T0                              ;   & $FFF = $600-$9FF
            and         #$0F
            sec
            sbc         #$06
            cmp         #$04
            bcs         @odd
@near:
            lda         HWT_T0
            lsr
            lsr
            lsr
            lsr
            ldx         #3
:
            cmp         HWT_CLOCK_Q,X
            beq         @clock
            dex
            bpl         :-
            bra         @odd
@clock:
            pha
            txa                                             ; Its name: HWT_CLOCK_MHZ + .X x 5
            asl
            asl
            stx         HWT_T1
            adc         HWT_T1
            adc         #<HWT_CLOCK_MHZ
            pha
            lda         #>HWT_CLOCK_MHZ
            adc         #0
            tay
            pla
            jsr         HWT_PUTS_AY
            jsr         HWT_PRINT
            .byte       " MHz ", 0
            jsr         HWT_CLOCK_COUNTS
            pla
            cmp         #4 * CPU_CLOCK_MULT
            beq         :+
            jsr         HWT_FAIL
.if ::CPU_CLOCK_MULT = 1
            .byte       "(the ROM's built for 3.58 MHz)", 0
.else
            .byte       "(the ROM's built for 7.16 MHz)", 0
.endif
:
            rts

@odd:
            jsr         HWT_FAIL
            .byte       "not a clock the board has: 16,384 SND_CLK cycles took ", 0

; The counts: "(first N, next M) ": the CPU cycles from timer A's start to its first overflow (HWT_NUM), and
; from there to the next, with its flag reset by writing $14 again (HWT_Q).  16,384 at 3.58 MHz; if the
; next is longer by about the write's time, writing $14 restarts the timer.  Modifies: .A, .Y, HWT_NUM
HWT_CLOCK_COUNTS:
            jsr         HWT_PRINT
            .byte       "(first ", 0
            lda         HWT_NUM
            ldy         HWT_NUM + 1
            jsr         HWT_DEC_AY
            jsr         HWT_PRINT
            .byte       ", next ", 0
            lda         HWT_Q
            ldy         HWT_Q + 1
            jsr         HWT_DEC_AY
            jsr         HWT_PRINT
            .byte       ") ", 0
            rts

HWT_CLOCK_Q:    .byte   1, 2, 4, 8
HWT_CLOCK_MHZ:  .byte   "0.89", 0, "1.79", 0, "3.58", 0, "7.16", 0

; ****************************************************************************
; The ACIA: DCD and DSR (tied active), its control and command registers, a programmed reset, and (Rockwell)
; a character's time at 9600 baud: 3,840 CPU cycles at 3.58 MHz (its clock, SER_CLK, is the CPU's / 2).
HWT_T_ACIA:
            lda         ACIA_R_STATUS
            and         #ACIA_STATUS_BIT_DCD | ACIA_STATUS_BIT_DSRB
            beq         :+
            jsr         HWT_FAIL
            .byte       "DCD or DSR not active", 0
:
            jsr         HWT_TX_IDLE                         ; (Nothing being sent while they change)
            ldx         #HWT_CTRLS_N - 1
@ctrl:
            lda         HWT_CTRLS,X
            sta         ACIA_R_CTRL
            cmp         ACIA_R_CTRL
            bne         @ctrl_bad
            dex
            bpl         @ctrl
            bra         @cmds
@ctrl_bad:
            pha
            lda         #$10 | SR_SELECT
            sta         ACIA_R_CTRL
            jsr         HWT_FAIL
            .byte       "control wrote ", 0
            pla
            jsr         HWT_HEX2

@cmds:
            lda         #$10 | SR_SELECT
            sta         ACIA_R_CTRL
            ldx         #HWT_CMDS_N - 1
@cmd:
            lda         HWT_CMDS,X
            sta         ACIA_R_CMD
            cmp         ACIA_R_CMD
            bne         @cmd_bad
            dex
            bpl         @cmd
            bra         @reset
@cmd_bad:
            pha
            jsr         HWT_ACIA_OFF
            jsr         HWT_FAIL
            .byte       "command wrote ", 0
            pla
            jsr         HWT_HEX2

@reset:
            lda         #$EB                                ; A programmed reset (a status write) clears the
            sta         ACIA_R_CMD                          ;   command register's bits 0-4
            sta         ACIA_R_STATUS
            ldy         ACIA_R_CMD
            jsr         HWT_ACIA_OFF
            cpy         #$E0
            beq         :+
            jsr         HWT_FAIL
            .byte       "programmed reset: command reads ", 0
            tya
            jsr         HWT_HEX2
:
.if ::SER_ACIA = ::SER_ACIA_ROCKWELL
            lda         #' '                                ; A character's time: two out (the first goes
            jsr         HWT_PUTC                            ;   straight to the transmitter, the second waits
            lda         #' '                                ;   for it), then a third when the second's
            jsr         HWT_PUTC                            ;   started, timed till TDRE again
            jsr         HWT_TDRE_WAIT
            lda         #' '
            sta         ACIA_R_DATA
            ldx         #$FF
            stx         VIA_R_T1L_L
            stx         VIA_R_T1C_H
            inc         HWT_COL
            jsr         HWT_TDRE_WAIT
            lda         VIA_R_T1C_L
            ldy         VIA_R_T1C_H
            bcs         @stuck
            eor         #$FF
            sta         HWT_NUM
            tya
            eor         #$FF
            sta         HWT_NUM + 1
            lda         HWT_NUM                             ; Within 2%
            cmp         #<(HWT_CHAR_CYCLES - HWT_CHAR_CYCLES / 50)
            lda         HWT_NUM + 1
            sbc         #>(HWT_CHAR_CYCLES - HWT_CHAR_CYCLES / 50)
            bcc         @slow
            lda         HWT_NUM
            cmp         #<(HWT_CHAR_CYCLES + HWT_CHAR_CYCLES / 50)
            lda         HWT_NUM + 1
            sbc         #>(HWT_CHAR_CYCLES + HWT_CHAR_CYCLES / 50)
            bcs         @slow
            jsr         HWT_PRINT
            .byte       "9600 baud ", 0
            rts
@slow:
            jsr         HWT_FAIL
            .byte       "a character took ", 0
            lda         HWT_NUM
            ldy         HWT_NUM + 1
            jsr         HWT_DEC_AY
            jsr         HWT_PRINT
            .byte       " cycles", 0
            rts
@stuck:
            jsr         HWT_FAIL
            .byte       "TDRE stays 0 (CTS off?)", 0
.endif
            rts

HWT_CHAR_CYCLES     = 3840 * CPU_CLOCK_MULT     ; 9600 8N1: 10 x 16 x 12 ACIA clocks (SER_CLK: the CPU's / 2)

HWT_CTRLS:      .byte   $1E, $3C, $5A, $96, $F0, $0F, $FF, $00
HWT_CTRLS_N     = * - HWT_CTRLS
HWT_CMDS:       .byte   $0B, $0A, $03, $07, $1B, $2B, $4B, $8B, $F1, $E9
HWT_CMDS_N      = * - HWT_CMDS

; Wait till the transmitter's idle: TDRE, then a character's time.  Modifies: .A, .X, .Y
HWT_TX_IDLE:
            jsr         HWT_TDRE_WAIT
            ldx         #(SER_CHAR_CYCLES + 1279) / 1280
            ldy         #0
:
            dey
            bne         :-
            dex
            bne         :-
            rts

; ****************************************************************************
; The SPI devices 0-7 (information): each is sent CMD0 (an SD card's reset); an SD card answers $01.  Its
; number is shown; another answer is shown as (device: answer).
HWT_T_SPI:
            stz         HWT_T3                              ; The device
            stz         HWT_T2                              ; (Found: 1 = SD cards shown, 2 = anything)
@device:
            lda         #HWT_SPI_IDLE                       ; 80 clocks, deselected (a card's power-up)
            sta         HWT_T5
            sta         VIA_R_PORTB
            ldy         #10
:
            lda         #$FF
            jsr         HWT_SPI_BYTE
            dey
            bne         :-
            lda         HWT_T3                              ; Selected: /CS low, the device on PB3-PB5
            asl
            asl
            asl
            sta         HWT_T5
            sta         VIA_R_PORTB
            ldy         #0
:
            lda         HWT_CMD0,Y
            jsr         HWT_SPI_BYTE
            iny
            cpy         #6
            bne         :-
            ldy         #16                                 ; Its answer: the first byte that isn't $FF
:
            lda         #$FF
            jsr         HWT_SPI_BYTE
            cmp         #$FF
            bne         :+
            dey
            bne         :-
:
            sta         HWT_T4
            lda         #HWT_SPI_IDLE                       ; Deselected, and a byte more
            sta         HWT_T5
            sta         VIA_R_PORTB
            lda         #$FF
            jsr         HWT_SPI_BYTE
            lda         HWT_T4
            cmp         #$FF
            beq         @next
            cmp         #$01
            bne         @other
            lda         HWT_T2
            and         #1
            bne         :+
            jsr         HWT_PRINT
            .byte       "SD cards ", 0
:
            lda         #3
            tsb         HWT_T2
            lda         HWT_T3
            jsr         HWT_HEX1
            lda         #' '
            jsr         HWT_PUTC
            bra         @next
@other:
            lda         #2
            tsb         HWT_T2
            jsr         HWT_PRINT
            .byte       "(", 0
            lda         HWT_T3
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       ": ", 0
            lda         HWT_T4
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       ") ", 0
@next:
            inc         HWT_T3
            lda         HWT_T3
            cmp         #8
            beq         :+
            jmp         @device
:
            lda         HWT_T2
            bne         :+
            jsr         HWT_PRINT
            .byte       "no SD cards ", 0
:
            rts

HWT_CMD0:       .byte   $40, $00, $00, $00, $00, $95

; .A out on SPI (mode 0), and the byte in (.A); port B's other bits from HWT_T5.  Modifies: .X
HWT_SPI_BYTE:
            sta         HWT_T6
            ldx         #8
@bit:
            lda         HWT_T5
            asl         HWT_T6                              ; (MOSI: PB2)
            bcc         :+
            ora         #$04
:
            sta         VIA_R_PORTB
            inc         VIA_R_PORTB                         ; SCLK up: the device takes MOSI, and MISO
            lda         VIA_R_PORTB                         ;   (PB7) is read
            asl
            rol         HWT_T7
            dec         VIA_R_PORTB                         ; SCLK down
            dex
            bne         @bit
            lda         HWT_T7
            rts

; ****************************************************************************
; The I2C bus (VIA port A: PA0 SCL, PA1 SDA, pulled up; a line is pulled low by making its bit an output):
; both lines high when released, each low alone when pulled low, then a scan of addresses $08-$77, which
; shows the devices that answer (information).
HWT_T_I2C:
            stz         VIA_R_PORTA_NOHS                    ; (Its outputs: low)
            stz         VIA_R_DDRA
            jsr         HWT_I2C_LINES
            cmp         #3
            beq         :+
            jsr         HWT_FAIL
            .byte       "released:", 0
            jmp         HWT_I2C_SHOW
:
            jsr         HWT_SCL_LO
            jsr         HWT_I2C_LINES
            jsr         HWT_SCL_HI
            lda         HWT_T4
            cmp         #2
            beq         :+
            jsr         HWT_FAIL
            .byte       "SCL low:", 0
            jmp         HWT_I2C_SHOW
:
            jsr         HWT_SDA_LO                          ; (A START, then a STOP)
            jsr         HWT_I2C_LINES
            jsr         HWT_SDA_HI
            lda         HWT_T4
            cmp         #1
            beq         @scan
            jsr         HWT_FAIL
            .byte       "SDA low:", 0
            jmp         HWT_I2C_SHOW

@scan:
            stz         HWT_T2                              ; Found
            lda         #$08                                ; Each address: a START, it (to write), its
            sta         HWT_T3                              ;   ACK, a STOP
@address:
            jsr         HWT_SDA_LO
            jsr         HWT_SCL_LO
            lda         HWT_T3
            asl
            jsr         HWT_I2C_BYTE
            php
            jsr         HWT_SDA_LO
            jsr         HWT_SCL_HI
            jsr         HWT_SDA_HI
            plp
            bcs         @next
            lda         HWT_T2
            bne         :+
            jsr         HWT_PRINT
            .byte       "devices ", 0
:
            inc         HWT_T2
            lda         HWT_T3
            jsr         HWT_HEX2
            lda         #' '
            jsr         HWT_PUTC
@next:
            inc         HWT_T3
            lda         HWT_T3
            cmp         #$78
            bne         @address
            lda         HWT_T2
            bne         :+
            jsr         HWT_PRINT
            .byte       "no devices ", 0
:
            stz         VIA_R_DDRA
            rts

; The lines, after a moment for the pull-ups: .A and HWT_T4 = SDA x 2 + SCL.  Modifies: .X
HWT_I2C_LINES:
            ldx         #20
:
            dex
            bne         :-
            lda         VIA_R_PORTA_NOHS
            and         #3
            sta         HWT_T4
            rts

; " SDA n SCL n" from HWT_T4, and the lines released.  Modifies: .A
HWT_I2C_SHOW:
            jsr         HWT_PRINT
            .byte       " SDA ", 0
            lda         HWT_T4
            lsr
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       " SCL ", 0
            lda         HWT_T4
            and         #1
            jsr         HWT_HEX1
            stz         VIA_R_DDRA
            rts

; A line low, or released (SCL: then high, or a device holding it low for a while).  Modifies: .A (SCL_HI: .X)
HWT_SCL_LO:
            lda         #$01
            tsb         VIA_R_DDRA
            rts
HWT_SCL_HI:
            lda         #$01
            trb         VIA_R_DDRA
            ldx         #0
:
            bit         VIA_R_PORTA_NOHS
            bne         :+
            dex
            bne         :-
:
            rts
HWT_SDA_LO:
            lda         #$02
            tsb         VIA_R_DDRA
            rts
HWT_SDA_HI:
            lda         #$02
            trb         VIA_R_DDRA
            rts

; .A out on I2C (SCL low before and after), and the ACK.  OUT: C = 0: acknowledged.  Modifies: .A, .X, .Y
HWT_I2C_BYTE:
            sta         HWT_T6
            ldy         #8
@bit:
            asl         HWT_T6
            bcc         :+
            jsr         HWT_SDA_HI
            bra         @clock
:
            jsr         HWT_SDA_LO
@clock:
            jsr         HWT_SCL_HI
            jsr         HWT_SCL_LO
            dey
            bne         @bit
            jsr         HWT_SDA_HI                          ; The ACK: the device's, on SDA
            jsr         HWT_SCL_HI
            lda         VIA_R_PORTA_NOHS
            lsr
            lsr
            php
            jsr         HWT_SCL_LO
            plp
            rts

; ****************************************************************************
; The slots (information): each slot port ($FF20-$FF3F, $FF50-$FFEF) whose 16 bytes don't all read $FF
; (an empty port's floating bus) is shown as its slot and select (A or B).
HWT_T_SLOTS:
            stz         HWT_T2                              ; Found
            ldx         #0
@port:
            lda         HWT_SLOT_PORTS,X
            sta         HWT_P
            lda         #$FF
            sta         HWT_P + 1
            ldy         #15
:
            lda         (HWT_P),Y
            cmp         #$FF
            bne         @card
            dey
            bpl         :-
            bra         @next
@card:
            lda         HWT_T2
            bne         :+
            jsr         HWT_PRINT
            .byte       "cards at ", 0
:
            inc         HWT_T2
            lda         HWT_SLOT_NUMS,X
            jsr         HWT_PUTC
            lda         HWT_SLOT_SELS,X
            jsr         HWT_PUTC
            lda         #' '
            jsr         HWT_PUTC
@next:
            inx
            cpx         #12
            bne         @port
            lda         HWT_T2
            bne         :+
            jsr         HWT_PRINT
            .byte       "all empty ", 0
:
            rts

HWT_SLOT_PORTS: .byte   $20, $30, $50, $60, $70, $80, $90, $A0, $B0, $C0, $D0, $E0
HWT_SLOT_NUMS:  .byte   "001234512345"
HWT_SLOT_SELS:  .byte   "ABAAAAABBBBB"
