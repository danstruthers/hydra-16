.debuginfo

; ****************************************************************************
; The hardware test: a program of its own in paged ROM bank HWT_BANK ($A000-$DFFF), included inside
; `.scope HWTEST` (see all.s).  It takes the machine over: the OS is gone (IRQs off; no BIOS routines are
; called: its own polled serial I/O at 9600 8N1), and it tests as much of the hardware as it can, from a
; menu.  The memory tests overwrite everything, so it ends by resetting the machine (R).
;   Entered from BIOS ROM code with _M_HWT_ENTER (include/hwtest.inc): HyForth's hwtest, or a T typed
; during POST.  Every task's paged ROM bank is then this bank, so it can switch T as it runs.
;
; Its memory: task 0's zero page ($40-$7F; the tests that use a task's RAM keep to $F0-$FF there, HWT_S*),
; its stack, and task 0's RAM from $0400 (HWT_RAM: routines that switch the paged ROM bank run from there).
; The task RAM test keeps task 0's zero page and stack in shared bank $F0 while it tests them.
;
; The tests (hwt_regs.s, hwt_mem.s, hwt_rom.s, hwt_dev.s) each print what they found after their name, and
; call HWT_FAIL for a fault (it prints FAIL and the text after the call); the runner prints ok, or counts
; the failure.  HWT_MODE says how thorough the memory tests are (HWT_FULL).

.segment "HWT_A"

; Zero page (task 0)
HWT_FAILS           = $40       ; Tests failed, this run (2)
HWT_MODE            = $42       ; HWT_FULL: the whole of each memory; HWT_LOOP: again until a key
HWT_FULL            = $80
HWT_LOOP            = $40
HWT_PASSES          = $43       ; Loop passes (2)
HWT_STR             = $45       ; HWT_PRINT's text (2)
HWT_NUM             = $47       ; A number to print (4)
HWT_FAIL1           = $4B       ; <> 0: the test running has failed
HWT_SAVY            = $4C       ; (HWT_PRINT keeps .Y here)
HWT_IDX             = $4D       ; The test running: its place in HWT_TESTS
HWT_COL             = $4E       ; The output's column (for the dots after a name)
HWT_P               = $50       ; Pointers (2 each)
HWT_Q               = $52
HWT_T0              = $54       ; Temporaries (HWT_T0-HWT_T7)
HWT_T1              = $55
HWT_T2              = $56
HWT_T3              = $57
HWT_T4              = $58
HWT_T5              = $59
HWT_T6              = $5A
HWT_T7              = $5B
HWT_CRC             = $5C       ; A CRC (2)
HWT_ERRS            = $5E       ; Faults counted by a test (2)
HWT_WROTE           = $60       ; A memory fault: what was written, what was read, where (2), which bank
HWT_READ            = $61
HWT_AT              = $62
HWT_ATB             = $64
HWT_MODS            = $65       ; The RAM modules found (2: bit m = module m)
HWT_IRQ_N           = $67       ; Interrupts taken (hwt_dev.s's handler)
HWT_IRQ_E           = $68       ;   the last one's vector RAM entry
HWT_IRQ_P           = $69       ;   and the P it pushed
HWT_KEEP            = $70       ; ($70-$7F: kept by a test across its calls)
; Zero page scratch, in whichever task a test runs in (T switched)
HWT_SP              = $F0       ; A pointer (2)
HWT_SV              = $F2       ; A value, and another
HWT_SW              = $F3
HWT_SN              = $F4       ; A count (2)
HWT_SE              = $F6       ; A fault found: wrote, read, where (2)
HWT_SR              = $F7
HWT_SA              = $F8
HWT_SL              = $F9       ; HWT_PAGES_TO: the last page's end (0: all of it)
HWT_SX              = $FA       ;   and this page's

HWT_RAM             = $0400     ; Routines that switch the paged ROM bank run in task 0's RAM here

HWT_NAME_COL        = 22        ; The column results start at

; ****************************************************************************
; The entry (_M_HWT_ENTER jumps here, in task 0, with IRQs off)
HWT_ENTRY:
            sei
            cld
            ldx         #$FF
            txs
            stz         U_REGISTER
            jsr         HWT_QUIET
            stz         HWT_MODE
            stz         HWT_COL
            jsr         HWT_PRINT
            .byte       ASCII_CR, ASCII_LF, ASCII_CR, ASCII_LF, "Hydra-16 hardware test", ASCII_CR, ASCII_LF
            .byte       "It overwrites all of memory, and ends with a reset (R).", 0

HWT_MENU:
            ldx         #$FF
            txs
            jsr         HWT_QUIET
            jsr         HWT_PRINT
            .byte       ASCII_CR, ASCII_LF
            .byte       "A  all tests (quick)   F  all, full memory   L  all, until a key   R  reset", ASCII_CR, ASCII_LF, 0
            ldx         #0                                  ; Then each test's key and name, 3 a line
            stx         HWT_T0

@list:
            lda         HWT_TESTS,X
            beq         @listed
            jsr         HWT_PUTC
            jsr         HWT_PRINT
            .byte       "  ", 0
            lda         HWT_TESTS + 3,X
            ldy         HWT_TESTS + 4,X
            jsr         HWT_PUTS_AY
            ldy         HWT_T0                              ; (Padded to a third of the line)
            lda         HWT_THIRDS,Y
            sta         HWT_T1
            lda         #' '
:
            ldy         HWT_COL
            cpy         HWT_T1
            bcs         :+
            jsr         HWT_PUTC
            bra         :-
:
            inc         HWT_T0
            lda         HWT_T0
            cmp         #3
            bne         :+
            stz         HWT_T0
            jsr         HWT_CRLF
:
            txa
            clc
            adc         #HWT_TEST_SIZE
            tax
            bra         @list

@listed:
            jsr         HWT_CRLF
            jsr         HWT_PRINT
            .byte       "> ", 0
:
            jsr         HWT_GETC
            bcc         :-
            cmp         #'a'                                ; (Upper case)
            bcc         :+
            and         #$DF
:
            pha
            jsr         HWT_PUTC
            jsr         HWT_CRLF
            pla
            cmp         #'R'
            bne         @far1
            jmp         HWT_RESET
@far1:
            ldx         #0
            cmp         #'A'
            beq         @all
            ldx         #HWT_FULL
            cmp         #'F'
            beq         @all
            ldx         #HWT_LOOP
            cmp         #'L'
            beq         @all
            sta         HWT_T0                              ; One test
            ldx         #0
:
            lda         HWT_TESTS,X
            beq         @menu                               ; (Not a key it knows)
            cmp         HWT_T0
            beq         :+
            txa
            clc
            adc         #HWT_TEST_SIZE
            tax
            bra         :-
:
            stz         HWT_MODE
            stz         HWT_FAILS
            stz         HWT_FAILS + 1
            jsr         HWT_RUN_ONE
            jsr         HWT_SUMMARY

@menu:
            jmp         HWT_MENU

@all:
            stx         HWT_MODE
            stz         HWT_PASSES
            stz         HWT_PASSES + 1
            stz         HWT_FAILS
            stz         HWT_FAILS + 1

@pass:
            ldx         #0

@each:
            lda         HWT_TESTS,X
            beq         @passed
            lda         HWT_TESTS + 5,X                     ; (Flags: not in the all-tests run)
            bmi         @next
            phx
            jsr         HWT_RUN_ONE
            plx
            bit         HWT_MODE                            ; Looping: a key stops it (after this test)
            bvc         @next
            jsr         HWT_GETC
            bcs         @stopped

@next:
            txa
            clc
            adc         #HWT_TEST_SIZE
            tax
            bra         @each

@passed:
            inc         HWT_PASSES
            bne         :+
            inc         HWT_PASSES + 1
:
            bit         HWT_MODE
            bvc         @done
            jsr         HWT_PRINT                           ; Looping: the pass, and on
            .byte       "pass ", 0
            lda         HWT_PASSES
            ldy         HWT_PASSES + 1
            jsr         HWT_DEC_AY
            jsr         HWT_PRINT
            .byte       ", failures ", 0
            lda         HWT_FAILS
            ldy         HWT_FAILS + 1
            jsr         HWT_DEC_AY
            jsr         HWT_CRLF
            jsr         HWT_GETC
            bcc         @pass

@stopped:
@done:
            jsr         HWT_SUMMARY
            jmp         HWT_MENU

HWT_THIRDS:     .byte   26, 52, 0

; Reset the machine: BIOS ROM page 0's reset code
HWT_RESET:
            jsr         HWT_PRINT
            .byte       "Resetting", ASCII_CR, ASCII_LF, 0
            ldx         #0                                  ; (Its last byte out first)
:
            dex
            bne         :-
            stz         W_REGISTER
            jmp         ($FFFC)

; "all passed", or how many failed
HWT_SUMMARY:
            lda         HWT_FAILS
            ora         HWT_FAILS + 1
            bne         :+
            jsr         HWT_PRINT
            .byte       "hwtest: all passed", ASCII_CR, ASCII_LF, 0
            rts
:
            jsr         HWT_PRINT
            .byte       "hwtest: failed: ", 0
            lda         HWT_FAILS
            ldy         HWT_FAILS + 1
            jsr         HWT_DEC_AY
            jmp         HWT_CRLF

; Run the test at HWT_TESTS + .X: its name, then it (which prints what it finds), then ok if it didn't
; fail.  Modifies: .A, .X, .Y
HWT_RUN_ONE:
            stx         HWT_IDX
            jsr         HWT_QUIET
            ldx         HWT_IDX
            lda         HWT_TESTS + 3,X
            ldy         HWT_TESTS + 4,X
            jsr         HWT_PUTS_AY
            lda         #' '
            jsr         HWT_PUTC
            lda         #'.'
:
            jsr         HWT_PUTC
            ldy         HWT_COL
            cpy         #HWT_NAME_COL - 1
            bcc         :-
            lda         #' '
            jsr         HWT_PUTC
            stz         HWT_FAIL1
            ldx         HWT_IDX
            lda         HWT_TESTS + 1,X
            sta         HWT_P
            lda         HWT_TESTS + 2,X
            sta         HWT_P + 1
            jsr         @call
            lda         HWT_FAIL1
            bne         :+
            jsr         HWT_PRINT
            .byte       "ok", 0
            bra         @end
:
            inc         HWT_FAILS
            bne         @end
            inc         HWT_FAILS + 1

@end:
            jmp         HWT_CRLF

@call:
            jmp         (HWT_P)

; ****************************************************************************
; Reporting

; A fault: "FAIL " (once in a test), then the text after the call.  Preserves .X, .Y
HWT_FAIL:
            pha
            lda         HWT_FAIL1
            bne         :+
            inc         HWT_FAIL1
            pla
            pha
            lda         #'F'                                ; ("FAIL ", without a call that would move the
            jsr         HWT_PUTC                            ;   return address HWT_PRINT reads)
            lda         #'A'
            jsr         HWT_PUTC
            lda         #'I'
            jsr         HWT_PUTC
            lda         #'L'
            jsr         HWT_PUTC
            lda         #' '
            jsr         HWT_PUTC
            bra         :++
:
            lda         #' '
            jsr         HWT_PUTC
:
            pla
            jmp         HWT_PRINT                           ; (Its text: the caller's)

; A memory fault (HWT_WROTE, HWT_READ, HWT_AT): " at AAAA wrote WW read RR".  Preserves .X, .Y
HWT_MEMERR:
            jsr         HWT_PRINT
            .byte       "at ", 0
            phx
            phy
            lda         HWT_AT + 1
            jsr         HWT_HEX2
            lda         HWT_AT
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       " wrote ", 0
            lda         HWT_WROTE
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       " read ", 0
            lda         HWT_READ
            jsr         HWT_HEX2
            ply
            plx
            rts

; ****************************************************************************
; Serial I/O: the ACIA, polled

; .A out.  (Rockwell 65C51: wait for TDRE, with a timeout, in case CTS is off; WDC 65C51: its TDRE always
; says empty, so wait a character's time after the byte.)  Counts the column.  Preserves .A, .X, .Y
HWT_PUTC:
            phx
            phy
            pha
            jsr         HWT_TDRE_WAIT
            pla
            sta         ACIA_R_DATA
.if ::SER_ACIA = ::SER_ACIA_WDC
            ldx         #(SER_CHAR_CYCLES + 1279) / 1280
            ldy         #0
:
            dey
            bne         :-
            dex
            bne         :-
.endif
            inc         HWT_COL
            cmp         #ASCII_CR
            bne         :+
            stz         HWT_COL
:
            ply
            plx
            rts

; Wait for the ACIA's TDRE: about 26,000 cycles at most (more than a character's time at 9600 baud and
; 7.16 MHz).  OUT: C = 0: it's set; C = 1: it never was (CTS off?).  Modifies: .A, .X, .Y
HWT_TDRE_WAIT:
            ldx         #0
            ldy         #8
:
            lda         ACIA_R_STATUS
            and         #ACIA_STATUS_BIT_TDRE
            bne         :+
            dex
            bne         :-
            dey
            bne         :-
            sec
            rts
:
            clc
            rts

; A key?  OUT: C = 1: .A = it; C = 0: none.  Preserves .X, .Y
HWT_GETC:
            lda         ACIA_R_STATUS
            and         #ACIA_STATUS_BIT_RDRF
            clc
            beq         :+
            lda         ACIA_R_DATA
            sec
:
            rts

; CR LF.  Preserves .X, .Y
HWT_CRLF:
            lda         #ASCII_CR
            jsr         HWT_PUTC
            lda         #ASCII_LF
            jmp         HWT_PUTC

; The text after the call (zero-terminated), then on after it.  Preserves .X, .Y
HWT_PRINT:
            pla
            sta         HWT_STR
            pla
            sta         HWT_STR + 1
            sty         HWT_SAVY
            ldy         #1                                  ; (The return address is the call's last byte)
:
            lda         (HWT_STR),Y
            beq         :+
            jsr         HWT_PUTC
            iny
            bne         :-
:
            tya                                             ; On after the 0 (rts adds 1)
            clc
            adc         HWT_STR
            tay
            lda         HWT_STR + 1
            adc         #0
            pha
            phy
            ldy         HWT_SAVY
            rts

; The text at .A.Y (zero-terminated).  Preserves .X
HWT_PUTS_AY:
            sta         HWT_STR
            sty         HWT_STR + 1
            ldy         #0
:
            lda         (HWT_STR),Y
            beq         :+
            jsr         HWT_PUTC
            iny
            bne         :-
:
            rts

; .A as two hex digits.  Preserves .A, .X, .Y
HWT_HEX2:
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         HWT_HEX1
            pla
            pha
            jsr         HWT_HEX1
            pla
            rts

; .A's low 4 bits as a hex digit.  Preserves .X, .Y
HWT_HEX1:
            and         #$0F
            cmp         #10
            bcc         :+
            adc         #'A' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            jmp         HWT_PUTC

; .A.Y (16 bits) in decimal.  Modifies: .A, .Y, HWT_NUM
HWT_DEC_AY:
            sta         HWT_NUM
            sty         HWT_NUM + 1
            stz         HWT_NUM + 2
            stz         HWT_NUM + 3

; HWT_NUM (32 bits) in decimal (HWT_NUM ends as 0).  Modifies: .A, .Y
HWT_DEC:
            phx
            lda         #$FF                                ; (The digits on the stack, the last first)
            pha
:
            lda         #0                                  ; HWT_NUM / 10: .A = the remainder
            ldx         #32
@bit:
            asl         HWT_NUM
            rol         HWT_NUM + 1
            rol         HWT_NUM + 2
            rol         HWT_NUM + 3
            rol
            cmp         #10
            bcc         @no
            sbc         #10
            inc         HWT_NUM
@no:
            dex
            bne         @bit
            pha
            lda         HWT_NUM
            ora         HWT_NUM + 1
            ora         HWT_NUM + 2
            ora         HWT_NUM + 3
            bne         :-
:
            pla
            bmi         :+
            ora         #'0'
            jsr         HWT_PUTC
            bra         :-
:
            plx
            rts

; ****************************************************************************
; The devices, quiet: no interrupts from the VIA, the ACIA or the YM2151; the VIA's timers stopped; the
; ACIA at 9600 8N1, polled; no SPI device selected.  Modifies: .A, .X, .Y
HWT_QUIET:
            lda         #$7F
            sta         VIA_R_INT_ENABLE                    ; (All of them off)
            sta         VIA_R_INT_FLAGS
            stz         VIA_R_AUX_CTRL
            stz         VIA_R_PER_CTRL
            lda         #HWT_SPI_IDLE
            sta         VIA_R_PORTB
            lda         #HWT_SPI_DDR
            sta         VIA_R_DDRB
            stz         VIA_R_DDRA                          ; (Port A: inputs; I2C released)
            lda         #$10 | SR_SELECT                    ; 9600 8N1, the internal baud rate generator
            sta         ACIA_R_CTRL
            lda         #HWT_ACIA_CMD
            sta         ACIA_R_CMD
            lda         #$01                                ; YM2151: its test/LFO register clear (as it
            ldx         #0                                  ;   should power up, but may not) ...
            jsr         HWT_YM_SET
            lda         #$14                                ;   its timers stopped, their flags reset,
            ldx         #$30                                ;   no IRQ
            jmp         HWT_YM_SET

HWT_ACIA_CMD        = ACIA_CMD_BIT_DTRL | ACIA_CMD_BIT_TLID | ACIA_CMD_BIT_RID    ; No IRQs
HWT_SPI_IDLE        = $02       ; Port B: SCLK low, /CS high (nothing selected)
HWT_SPI_DDR         = $7F       ;   PB0-PB6 out, PB7 (MISO) in

; YM2151 register .A = .X, after its busy flag clears (with a timeout: a missing chip).
; OUT: C = 0; or C = 1: it stayed busy.  Preserves .X, .Y
HWT_YM_SET:
            phy
            pha
            jsr         HWT_YM_WAIT
            pla
            sta         YM_REG
            jsr         HWT_YM_WAIT                         ; (Between the two writes too)
            stx         YM_DATA
            ply
            rts

; Wait while the YM2151 is busy (up to about 3,000 cycles).  OUT: C = 0; or C = 1: it stayed busy.
; Modifies: .A, .Y
HWT_YM_WAIT:
            ldy         #0
:
            lda         YM_DATA                             ; (The status: bit 7 = busy)
            bpl         :+
            dey
            bne         :-
            sec
            rts
:
            clc
            rts

.include "hwt_tests.s"          ; The tests' list (HWT_TESTS)
