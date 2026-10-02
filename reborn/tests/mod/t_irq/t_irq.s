; ****************************************************************************
; t_irq - an interrupt handler in a program's task (phase 1, spike S1), run as init: the serial port at 115200,
; received by this task's irq entry while two children spin (the tick switching tasks), N bytes from the PC
; (sim/test.js sends them when it sees "ready>": byte i is 3 + 7 * i), none lost, all in order.  sim/test.js
; also measures each byte's time from arriving to being read, and the ACIA's lost bytes.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_HEADER "t_irq", HT_PROGRAM, 0, main, 0, irq, 0, 0

N               = 2000

.zeropage
count:      .res        2                                   ; (The irq entry's: bytes received ...
expect:     .res        1                                   ;   the next one expected ...
wrong:      .res        1                                   ;   bytes not as expected ...
overruns:   .res        1                                   ;   overruns the ACIA saw)
t0:         .res        1

.code
main:
            stz         T_FAILS
            stz         count
            stz         count + 1
            stz         wrong
            stz         overruns
            lda         #3
            sta         expect
            LDR         r0, s_child                         ; Two children that never yield
            LDR         r1, s_sff
            lda         #0
            jsr         SPAWN
            LDR         r0, s_child
            LDR         r1, s_sff
            lda         #0
            jsr         SPAWN
            lda         #LINE_ACIA
            jsr         IRQ_OWN
            EXPECT_OK   "IRQ_OWN of the serial port's line"
            sei                                             ; 115200, its receive interrupt on
            lda         #ACIA_CTRL_BRG | ACIA_RATE_115200 | ACIA_CTRL_8N1
            sta         ACIA_CTRL
            lda         #ACIA_CMD_DTR | ACIA_CMD_TX_ON
            sta         ACIA_CMD
            lda         ACIA_DATA
            cli
            MARK        "ready>"
            jsr         TICKS
            sta         t0
@wait:                                                      ; Till all N are in, or a second
            lda         #1
            ldx         #0
            jsr         SLEEP
            sei
            lda         count
            ldx         count + 1
            cli
            cmp         #<N
            bne         :+
            cpx         #>N
            beq         @in
:
            jsr         TICKS
            sec
            sbc         t0
            cmp         #TICK_HZ
            bcc         @wait
@in:
            sei                                             ; The console, polled again
            lda         #ACIA_CMD_DTR | ACIA_CMD_NO_RXIRQ | ACIA_CMD_TX_ON
            sta         ACIA_CMD
            cli
            lda         #LINE_ACIA
            jsr         IRQ_RELEASE
            jsr         t_crlf
            EXPECT_OK   "IRQ_RELEASE"
            lda         count + 1
            EXPECT_A    >N, "all the bytes (high byte of the count)"
            lda         count
            EXPECT_A    <N, "all the bytes (low byte)"
            lda         wrong
            EXPECT_A    0, "each in order"
            lda         overruns
            EXPECT_A    0, "no overruns"
            DONE        "t_irq"

; The irq entry: in this task, IRQs off.  .A = the line.  OUT: .A = 0 (no task to wake)
irq:
            lda         ACIA_STATUS                         ; (Reading it clears its interrupt)
            tax
            and         #ACIA_ST_RDRF
            beq         @none
            txa
            and         #ACIA_ST_OVR
            beq         :+
            inc         overruns
:
            lda         ACIA_DATA
            cmp         expect
            beq         :+
            inc         wrong
:
            clc                                             ; (In step with what came)
            adc         #7
            sta         expect
            inc         count
            bne         @none
            inc         count + 1
@none:
            lda         #0
            rts

.rodata
s_child:    .byte       "#m/t_child", 0
s_sff:      .byte       "sff", 0
