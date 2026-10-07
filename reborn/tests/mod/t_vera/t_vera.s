; ****************************************************************************
; t_vera - the emulator's Vera X (sim/lib/vera.js: the VERA in slot 0, the X16's gateware v47.0.2), its chip as a
; program sees it, run as init without the driver (vid): the version register; ADDR0 and ADDR1, their steps (1,
; 40, DECR), the byte a data port fetches ahead; DC_VIDEO and the scales as the gateware starts; the scan's
; interrupts on IRQ line 2 (a second's VSYNCs, 59.5 a second; LINE at line 100, SCANLINE read at it; at line 300,
; IEN's bit 8); two sprites colliding (SPRCOL, ISR's collision bits); the PCM FIFO (empty, full, AFLOW's level, and
; its interrupt as 48828 samples a second drain it); a PSG voice (sim/test.js looks for it); the SPI port with no
; card; CTRL's reset (no answer while the FPGA configures itself, then the registers as it starts them).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_HEADER "t_vera", HT_PROGRAM, 0, main, 0, irq, 0, 0

.zeropage
vsyncs:     .res        2                                   ; (The irq entry's: VSYNCs counted ...
lines:      .res        1                                   ;   LINEs ...
scan_at:    .res        1                                   ;   SCANLINE as the last LINE came ...
scan_hi:    .res        1                                   ;   and IEN then (bit 6: SCANLINE's bit 8) ...
isr_or:     .res        1                                   ;   ISR, every bit it's had ...
aflowed:    .res        1                                   ;   <> 0: AFLOW came (turned off)
t0:         .res        1
n:          .res        2

.code
main:
            stz         T_FAILS
            stz         vsyncs
            stz         vsyncs + 1
            stz         lines
            stz         isr_or
            stz         aflowed

; ---- The card: its version (DCSEL 63)
            lda         #VERA_DCSEL_VER
            sta         VERA_CTRL
            lda         VERA_DC_VER0
            EXPECT_A    VERA_VER_ID, "DC_VER0 (DCSEL 63): V, a version follows"
            lda         VERA_DC_VER1
            EXPECT_A    47, "the version: 47 ..."
            lda         VERA_DC_VER2
            EXPECT_A    0, "... .0 ..."
            lda         VERA_DC_VER3
            EXPECT_A    2, "... .2"
            lda         VERA_CTRL
            EXPECT_A    VERA_DCSEL_VER, "CTRL reads back DCSEL 63"
            stz         VERA_CTRL

; ---- ADDR0 and DATA0, step 1: four bytes at $01000
            lda         #$00
            sta         VERA_ADDR_L
            lda         #$10
            sta         VERA_ADDR_M
            lda         #VERA_INC_1
            sta         VERA_ADDR_H
            ldx         #0
:
            txa
            ora         #$A0
            sta         VERA_DATA0
            inx
            cpx         #4
            bcc         :-
            lda         VERA_ADDR_L
            EXPECT_A    4, "ADDR0 stepped by each write (+1)"
            lda         VERA_ADDR_H
            EXPECT_A    VERA_INC_1, "ADDR0_H reads back its step"
            lda         #VERA_CTRL_ADDRSEL                  ; ADDR1 at $01000: read through DATA1
            sta         VERA_CTRL
            stz         VERA_ADDR_L
            lda         #$10
            sta         VERA_ADDR_M
            lda         #VERA_INC_1
            sta         VERA_ADDR_H
            stz         VERA_CTRL
            lda         VERA_DATA1
            EXPECT_A    $A0, "DATA1: the first byte"
            lda         VERA_DATA1
            EXPECT_A    $A1, "DATA1 stepped: the second"
            lda         VERA_ADDR_L                         ; (ADDRSEL 0: ADDR0's)
            EXPECT_A    4, "ADDR0 as it was (ADDRSEL picks which ADDRx the registers are)"
            lda         #$03                                ; DECR: from $01003 down
            sta         VERA_ADDR_L
            lda         #VERA_INC_1 | VERA_DECR
            sta         VERA_ADDR_H
            lda         VERA_DATA0
            EXPECT_A    $A3, "DECR: $01003 ..."
            lda         VERA_DATA0
            EXPECT_A    $A2, "... then $01002"
            lda         VERA_ADDR_L
            EXPECT_A    $01, "ADDR0 stepped down"

; ---- The byte a port fetches ahead: DATA1's at $01000 (step 0), then $55 written there through DATA0
            lda         #VERA_CTRL_ADDRSEL
            sta         VERA_CTRL
            stz         VERA_ADDR_L
            lda         #$10
            sta         VERA_ADDR_M
            stz         VERA_ADDR_H
            stz         VERA_CTRL
            stz         VERA_ADDR_L
            lda         #$10
            sta         VERA_ADDR_M
            stz         VERA_ADDR_H
            lda         #$55
            sta         VERA_DATA0
            lda         VERA_DATA1
            EXPECT_A    $A0, "DATA1: the byte it fetched before DATA0's write"
            lda         VERA_DATA1
            EXPECT_A    $55, "then (step 0: the same address, fetched again) the new one"

; ---- Step 40 (index 11): $02000, $02028
            stz         VERA_ADDR_L
            lda         #$20
            sta         VERA_ADDR_M
            lda         #11 << 4
            sta         VERA_ADDR_H
            lda         #$11
            sta         VERA_DATA0
            lda         #$22
            sta         VERA_DATA0
            lda         VERA_ADDR_L
            EXPECT_A    80, "step 40: two writes, ADDR0 80 on"
            lda         #$28
            sta         VERA_ADDR_L
            stz         VERA_ADDR_H
            lda         VERA_DATA0
            EXPECT_A    $22, "the second byte at $02028"

; ---- The display's registers as the gateware starts them
            lda         VERA_DC_VIDEO
            and         #$7F
            EXPECT_A    0, "DC_VIDEO 0: the screen off"
            lda         VERA_DC_HSCALE
            EXPECT_A    128, "DC_HSCALE 128 (1 to 1)"
            lda         #1 << 1
            sta         VERA_CTRL
            lda         VERA_DC_HSTOP
            EXPECT_A    640 >> 2, "DC_HSTOP (DCSEL 1): 640"
            lda         VERA_DC_VSTOP
            EXPECT_A    480 >> 1, "DC_VSTOP: 480"
            stz         VERA_CTRL

; ---- VSYNC: a second's, on IRQ line 2
            lda         #LINE_SLOT0A
            jsr         IRQ_OWN
            EXPECT_OK   "IRQ_OWN of slot 0's line A (the VERA's IRQ#)"
            lda         #VERA_IRQ_VSYNC | VERA_IRQ_LINE | VERA_IRQ_SPRCOL
            sta         VERA_ISR
            lda         #VERA_IRQ_VSYNC
            sta         VERA_IEN
            lda         #<TICK_HZ
            ldx         #>TICK_HZ
            jsr         SLEEP
            stz         VERA_IEN
            lda         vsyncs + 1
            EXPECT_A    0, "VSYNCs in a second (high byte)"
            lda         vsyncs
            cmp         #59
            beq         :+
            cmp         #60
            beq         :+
            NOTOK       "59 or 60 VSYNCs in a second (59.5 Hz)"
            bra         @line
:
            OK          "59 or 60 VSYNCs in a second (59.5 Hz)"

; ---- LINE at line 100, SCANLINE read in the irq entry; at 300, IEN's bit 8 (IRQ_LINE's, and SCANLINE's)
@line:
            lda         #100
            sta         VERA_IRQ_LINE_L
            lda         #VERA_IRQ_VSYNC | VERA_IRQ_LINE | VERA_IRQ_SPRCOL
            sta         VERA_ISR
            lda         #VERA_IRQ_LINE
            sta         VERA_IEN
            lda         #10
            ldx         #0
            jsr         SLEEP
            stz         VERA_IEN
            lda         lines
            cmp         #2
            bcs         :+
            NOTOK       "LINE: an interrupt a frame (100)"
            bra         :++
:
            OK          "LINE: an interrupt a frame (100)"
:
            lda         scan_at                             ; (The entry runs a few dozen cycles after the line
            sec                                             ;   starts, or a line on if the tick's came first)
            sbc         #100
            cmp         #2
            bcc         :+
            lda         scan_at
            NOTOK       "SCANLINE read at line 100's interrupt: 100"
            bra         :++
:
            OK          "SCANLINE read at line 100's interrupt: 100"
:
            lda         scan_hi
            and         #$40
            EXPECT_A    0, "IEN's bit 6 (SCANLINE's bit 8): 0"
            lda         #<300
            sta         VERA_IRQ_LINE_L
            stz         lines
            lda         #VERA_IRQ_VSYNC | VERA_IRQ_LINE | VERA_IRQ_SPRCOL
            sta         VERA_ISR
            lda         #$80 | VERA_IRQ_LINE                ; (Bit 7: IRQ_LINE's bit 8)
            sta         VERA_IEN
            lda         #10
            ldx         #0
            jsr         SLEEP
            stz         VERA_IEN
            lda         lines
            beq         :+
            OK          "LINE at line 300 (IEN's bit 7: its bit 8)"
            bra         :++
:
            NOTOK       "LINE at line 300 (IEN's bit 7: its bit 8)"
:
            lda         scan_at
            sec
            sbc         #<300
            cmp         #2
            bcc         :+
            lda         scan_at
            NOTOK       "SCANLINE's low byte at line 300's interrupt"
            bra         :++
:
            OK          "SCANLINE's low byte at line 300's interrupt"
:
            lda         scan_hi
            and         #$40
            EXPECT_A    $40, "IEN's bit 6: SCANLINE's bit 8, 1"

; ---- Sprites: 0 and 1 on each other (collision masks $10 and $30): SPRCOL, ISR's collision bits $10
            lda         #$00                                ; Their image: 8 x 8, 4 bits, colour 1, at $03000
            sta         VERA_ADDR_L
            lda         #$30
            sta         VERA_ADDR_M
            lda         #VERA_INC_1
            sta         VERA_ADDR_H
            ldx         #32
            lda         #$11
:
            sta         VERA_DATA0
            dex
            bne         :-
            lda         #$00                                ; Their attributes at $1FC00
            sta         VERA_ADDR_L
            lda         #$FC
            sta         VERA_ADDR_M
            lda         #VERA_INC_1 | 1
            sta         VERA_ADDR_H
            ldx         #0
:
            lda         sprites,X
            sta         VERA_DATA0
            inx
            cpx         #16
            bcc         :-
            lda         #VERA_DC_OUT_VGA | VERA_DC_SPRITES
            sta         VERA_DC_VIDEO
            lda         #VERA_IRQ_VSYNC | VERA_IRQ_LINE | VERA_IRQ_SPRCOL
            sta         VERA_ISR
            stz         isr_or
            lda         #VERA_IRQ_SPRCOL
            sta         VERA_IEN
            lda         #10
            ldx         #0
            jsr         SLEEP
            stz         VERA_IEN
            stz         VERA_DC_VIDEO
            lda         isr_or
            and         #$F4
            EXPECT_A    $10 | VERA_IRQ_SPRCOL, "SPRCOL, and the collision bits: masks $10 and $30 met in $10"

; ---- The PCM FIFO: empty, AFLOW's level, full, reset
            stz         VERA_AUDIO_RATE
            lda         #$80 | $0F                          ; (Reset; 8 bits, mono, volume 15)
            sta         VERA_AUDIO_CTRL
            lda         VERA_AUDIO_CTRL
            and         #$C0
            EXPECT_A    $40, "AUDIO_CTRL: the FIFO empty"
            lda         VERA_ISR
            and         #VERA_IRQ_AFLOW
            EXPECT_A    VERA_IRQ_AFLOW, "ISR's AFLOW: the FIFO under a quarter full"
            LDR         n, 2000
            jsr         fill
            lda         VERA_ISR
            and         #VERA_IRQ_AFLOW
            EXPECT_A    0, "AFLOW off with 2000 bytes in"
            LDR         n, 2100
            jsr         fill
            lda         VERA_AUDIO_CTRL
            and         #$C0
            EXPECT_A    $80, "AUDIO_CTRL: the FIFO full (4095 bytes)"
            lda         #$80 | $0F
            sta         VERA_AUDIO_CTRL
            lda         VERA_AUDIO_CTRL
            and         #$C0
            EXPECT_A    $40, "the FIFO reset: empty"

; ---- AFLOW's interrupt: 2000 bytes, 48828 a second, under 1024 in 977 samples (20 ms: 4 ticks)
            LDR         n, 2000
            jsr         fill
            lda         #VERA_IRQ_AFLOW
            sta         VERA_IEN
            jsr         TICKS
            sta         t0
            lda         #128
            sta         VERA_AUDIO_RATE
@aflow:
            lda         aflowed
            bne         :+
            jsr         YIELD
            jsr         TICKS
            sec
            sbc         t0
            cmp         #TICK_HZ / 4
            bcc         @aflow
:
            jsr         TICKS
            sec
            sbc         t0
            sta         t0
            stz         VERA_AUDIO_RATE
            lda         aflowed
            EXPECT_A    1, "AFLOW's interrupt as the FIFO drained"
            lda         t0
            cmp         #3
            bcc         :+
            cmp         #6
            bcs         :+
            OK          "... in 20 ms (3-5 ticks)"
            bra         :++
:
            NOTOK       "... in 20 ms (3-5 ticks)"
:

; ---- The PSG: voice 0 at 440 Hz, both sides, loudest, a square wave (sim/test.js sees it come on); then quiet
            lda         #$C0
            sta         VERA_ADDR_L
            lda         #$F9
            sta         VERA_ADDR_M
            lda         #VERA_INC_1 | 1
            sta         VERA_ADDR_H
            lda         #<1181
            sta         VERA_DATA0
            lda         #>1181
            sta         VERA_DATA0
            lda         #$C0 | 63
            sta         VERA_DATA0
            lda         #63
            sta         VERA_DATA0
            lda         #$C2                                ; (Its volume: 0)
            sta         VERA_ADDR_L
            stz         VERA_DATA0
            OK          "a PSG voice keyed on and off"

; ---- The SPI port: no card on it, so $FF
            lda         #1                                  ; (Selected; fast)
            sta         VERA_SPI_CTRL
            lda         #$40
            sta         VERA_SPI_DATA
            ldx         #20
:
            lda         VERA_SPI_CTRL
            bpl         :+
            dex
            bne         :-
:
            lda         VERA_SPI_CTRL
            and         #$80
            EXPECT_A    0, "SPI: the byte sent (busy no more)"
            lda         VERA_SPI_DATA
            EXPECT_A    $FF, "SPI: $FF back (no card)"
            stz         VERA_SPI_CTRL

; ---- CTRL's reset: the FPGA configures itself again (0.1 s): the bus floats meanwhile, then the gateware's start
            lda         #$5A
            sta         VERA_ADDR_L
            lda         #VERA_CTRL_RESET
            sta         VERA_CTRL
            lda         VERA_ADDR_L
            EXPECT_A    $FF, "reset: no answer while the FPGA configures itself"
            lda         #TICK_HZ / 5
            ldx         #0
            jsr         SLEEP
            lda         VERA_ADDR_L
            EXPECT_A    0, "then ADDR0 0, as the gateware starts it"
            lda         VERA_DC_HSCALE
            EXPECT_A    128, "DC_HSCALE 128 again"

            lda         #LINE_SLOT0A
            jsr         IRQ_RELEASE
            EXPECT_OK   "IRQ_RELEASE"
            DONE        "t_vera"

; n bytes of 0 into the PCM FIFO.  Modifies .A, n
fill:
            lda         n
            ora         n + 1
            beq         @done
            stz         VERA_AUDIO_DATA
            lda         n
            bne         :+
            dec         n + 1
:
            dec         n
            bra         fill

@done:
            rts

; The irq entry (line 2): the interrupts that came and are on, cleared and counted; a LINE's SCANLINE kept; AFLOW
; turned off
irq:
            lda         VERA_ISR
            tsb         isr_or
            and         VERA_IEN
            pha
            and         #VERA_IRQ_VSYNC | VERA_IRQ_LINE | VERA_IRQ_SPRCOL
            sta         VERA_ISR
            pla
            lsr
            bcc         :+
            inc         vsyncs
            bne         :+
            inc         vsyncs + 1
:
            lsr
            bcc         :+
            ldx         VERA_SCANLINE_L
            stx         scan_at
            ldx         VERA_IEN
            stx         scan_hi
            inc         lines
:
            lsr
            lsr
            bcc         :+
            lda         #VERA_IRQ_AFLOW
            trb         VERA_IEN
            lda         #1
            sta         aflowed
:
            lda         #0
            rts

.rodata
; Sprites 0 and 1: the image at $03000 (4 bits a pixel), at (100, 100), z 3, collision masks $10 and $30, 8 x 8
sprites:
            .byte       $80, $01, 100, 0, 100, 0, $10 | $0C, $00
            .byte       $80, $01, 100, 0, 100, 0, $30 | $0C, $00
