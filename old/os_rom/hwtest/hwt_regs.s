.debuginfo

; ****************************************************************************
; The hardware test's CPU and register tests: the W65C02S's instructions, the pseudo-registers T, U, V
; and W, and each task's RAM bank register ($00).  (Inside `.scope HWTEST`: hwtest.s.)

.segment "HWT_A"

; ****************************************************************************
; The CPU: a sample of the W65C02S's instructions and modes, with their flags (binary and decimal
; arithmetic, the 65C02's own: STZ, TSB/TRB, BRA, PHX..., RMB/SMB, BBR/BBS, (zp)).  A wrong one: FAIL and
; its number.
HWT_T_CPU:
            ldx         #1                                  ; 1: ADC, binary, with carry and overflow
            clc
            lda         #$7F
            adc         #$01
            bvs         @far9
            jmp         @fail
@far9:
            bcc         @far8
            jmp         @fail
@far8:
            cmp         #$80
            beq         @far7
            jmp         @fail
@far7:
            inx                                             ; 2: SBC, binary, borrow
            sec
            lda         #$10
            sbc         #$20
            bcc         @far6
            jmp         @fail
@far6:
            cmp         #$F0
            beq         @far5
            jmp         @fail
@far5:
            inx                                             ; 3: decimal mode ADC and SBC
            sed
            clc
            lda         #$19
            adc         #$28
            cld
            cmp         #$47
            beq         @far4
            jmp         @fail
@far4:
            sed
            sec
            lda         #$50
            sbc         #$01
            cld
            cmp         #$49
            beq         @far3
            jmp         @fail
@far3:
            inx                                             ; 4: STZ, TSB, TRB
            lda         #$FF
            sta         HWT_T0
            stz         HWT_T0
            lda         HWT_T0
            beq         @far2
            jmp         @fail
@far2:
            lda         #$0F
            tsb         HWT_T0
            lda         #$03
            trb         HWT_T0
            lda         HWT_T0
            cmp         #$0C
            beq         @far1
            jmp         @fail
@far1:
            inx                                             ; 5: RMB, SMB, BBR, BBS
            lda         #$00
            sta         HWT_T1
            smb3        HWT_T1
            smb7        HWT_T1
            rmb3        HWT_T1
            bbs3        HWT_T1, @fail
            bbr7        HWT_T1, @fail
            lda         HWT_T1
            cmp         #$80
            bne         @fail
            inx                                             ; 6: PHX, PHY, PLA, PLX, INC A, DEC A
            phx
            ldy         #$12
            phy
            pla
            inc
            inc
            dec
            cmp         #$13
            bne         @fail2
            plx
            inx                                             ; 7: (zp) and (zp),Y and (zp,X)
            lda         #<HWT_CPU_DATA
            sta         HWT_P
            lda         #>HWT_CPU_DATA
            sta         HWT_P + 1
            lda         (HWT_P)
            cmp         #$A5
            bne         @fail
            ldy         #1
            lda         (HWT_P),Y
            cmp         #$5A
            bne         @fail
            phx
            ldx         #0
            lda         (HWT_P,X)
            plx
            cmp         #$A5
            bne         @fail
            inx                                             ; 8: shifts and rotates
            lda         #$81
            asl                                             ; ($02, C = 1)
            bcc         @fail
            rol                                             ; ($05, C = 0)
            cmp         #$05
            bne         @fail
            lsr                                             ; ($02, C = 1)
            bcc         @fail
            ror                                             ; ($81: the C in at bit 7; C = 0)
            bcs         @fail
            cmp         #$81
            bne         @fail
            inx                                             ; 9: BIT immediate (Z only) and BIT zp (N, V)
            lda         #$C0
            sta         HWT_T2
            lda         #$01
            bit         HWT_T2
            bpl         @fail
            bvc         @fail
            bne         @fail
            bit         #$01
            beq         @fail
            inx                                             ; 10: the stack: 128 bytes down and back
            ldy         #0
:
            tya
            pha
            iny
            bpl         :-
:
            dey
            tya
            sta         HWT_T3
            pla
            cmp         HWT_T3
            bne         @fail3
            cpy         #0
            bne         :-
            rts

@fail3:                                                     ; (What's left of the 128 bytes: .Y of them)
            cpy         #0
            beq         @fail
            pla
            dey
            bra         @fail3

@fail2:
            plx

@fail:
            cld
            stx         HWT_T0
            jsr         HWT_FAIL
            .byte       "check ", 0
            lda         HWT_T0
            ldy         #0
            jmp         HWT_DEC_AY

HWT_CPU_DATA:   .byte   $A5, $5A

; ****************************************************************************
; The pseudo-registers: each of T, U, V and W stores all 8 bits, and reads them back (74F573 latches read
; through 74F541s).  Writing T switches the task, so its test uses no stack or zero page until T is back to
; 0; writing W switches the BIOS ROM page, which this doesn't run from.  A bad bit: FAIL and the register.
HWT_T_REGS:
            ldx         #HWT_PATTERNS_N - 1                 ; T
:
            lda         HWT_PATTERNS,X
            sta         T_REGISTER
            ldy         T_REGISTER
            stz         T_REGISTER                          ; (Task 0 again)
            sta         HWT_T0
            tya
            eor         HWT_T0
            bne         @t
            dex
            bpl         :-
            ldx         #HWT_PATTERNS_N - 1                 ; U
:
            lda         HWT_PATTERNS,X
            sta         U_REGISTER
            eor         U_REGISTER
            bne         @u
            dex
            bpl         :-
            stz         U_REGISTER
            ldx         #HWT_PATTERNS_N - 1                 ; V
:
            lda         HWT_PATTERNS,X
            sta         V_REGISTER
            eor         V_REGISTER
            bne         @v
            dex
            bpl         :-
            ldx         #HWT_PATTERNS_N - 1                 ; W
:
            lda         HWT_PATTERNS,X
            sta         W_REGISTER
            eor         W_REGISTER
            bne         @w
            dex
            bpl         :-
            stz         W_REGISTER
            rts

@t:
            ldx         #'T'
            bra         @bad

@u:
            ldx         #'U'
            bra         @bad

@v:
            ldx         #'V'
            bra         @bad

@w:
            ldx         #'W'

@bad:
            stz         W_REGISTER
            stz         U_REGISTER
            sta         HWT_T0                              ; (The bits that differ)
            jsr         HWT_FAIL
            .byte       0
            txa
            jsr         HWT_PUTC
            jsr         HWT_PRINT
            .byte       " bits ", 0
            lda         HWT_T0
            jmp         HWT_HEX2

; Walking ones and zeros, all 0 and all 1
HWT_PATTERNS:   .byte   $00, $FF, $55, $AA, $01, $02, $04, $08, $10, $20, $40, $80
                .byte   $FE, $FD, $FB, $F7, $EF, $DF, $BF, $7F
HWT_PATTERNS_N = * - HWT_PATTERNS

; ****************************************************************************
; The RAM bank registers ($00: 74LS219s, one 4-bit word a task in each).  Each task's register holds its
; own bank: task t selects shared bank $F0 + t (U = 0), which has t's mark.  Then its lines 4-7: $F0 with
; one of them cleared (a RAM module's bank, or nothing) mustn't be $F0.  A fault: FAIL, the task and what
; it read.  (Shared RAM must work for this: its own test says so if it doesn't.)
HWT_BAD             = HWT_KEEP  ; (2) The shared banks this test skips: bit v, bank $F0 + v

HWT_T_BANKREG:
            stz         U_REGISTER
            stz         T_REGISTER
            ldx         #15                                 ; Each shared bank's mark ($F0 + v: v ^ $A5),
@mark:                                                      ;   by task 0
            txa
            ora         #$F0
            sta         RAM_BANK_REG
            txa
            eor         #$A5
            sta         $8000
            dex
            bpl         @mark
            stz         HWT_BAD                             ;   and read back (task 0's register, with each
            stz         HWT_BAD + 1                         ;   value).  A bank that reads wrong is the
            ldx         #15                                 ;   shared RAM's fault (test 3), not a
@marked:                                                    ;   register's: it's skipped from here on
            txa
            ora         #$F0
            sta         RAM_BANK_REG
            txa
            eor         #$A5
            cmp         $8000
            beq         :+
            txa
            tay
            jsr         HWT_BANK_SKIP
:
            dex
            bpl         @marked
            lda         HWT_BAD
            ora         HWT_BAD + 1
            beq         @at_once
            jsr         HWT_PRINT
            .byte       "skipping banks", 0
            ldx         #0
:
            txa
            tay
            jsr         HWT_BANK_IS_BAD
            beq         :+
            jsr         HWT_PRINT
            .byte       " F", 0
            txa
            jsr         HWT_HEX1
:
            inx
            cpx         #16
            bne         :--
            jsr         HWT_PRINT
            .byte       " (task 0 reads them wrong too: the shared RAM, test 3) ", 0
            lda         HWT_BAD
            and         HWT_BAD + 1
            cmp         #$FF
            bne         @at_once
            rts                                             ; (None left to test with)

@at_once:                                                   ; Each task, each value in bits 0-3: set, and
            stz         HWT_T2                              ;   its bank read at once ("at once: task F:
            lda         #$FF                                ;   D>F" = set to $FD, it read bank $FF's mark)
            sta         HWT_T4                              ; (The task last shown)
            ldx         #15                                 ; .X = the task
@task_v:
            ldy         #15                                 ; .Y = the value
@value:
            stz         T_REGISTER                          ; (A bank task 0 reads wrong: skipped)
            jsr         HWT_BANK_IS_BAD
            bne         @next_v
            stx         T_REGISTER
            tya
            ora         #$F0
            sta         RAM_BANK_REG
            tya
            eor         #$A5
            cmp         $8000
            bne         @wrong
@next_v:
            dey
            bpl         @value
            dex
            bpl         @task_v
            stz         T_REGISTER
            jmp         @all_set

@wrong:
            lda         $8000
            stz         T_REGISTER                          ; (Task 0's stack and zero page, to print)
            sta         HWT_T1
            stx         HWT_T0
            sty         HWT_T3
            inc         HWT_T2
            lda         HWT_T2
            cmp         #1
            bne         :+
            jsr         HWT_FAIL
            .byte       "at once:", 0
:
            lda         HWT_T0
            cmp         HWT_T4
            beq         :+
            sta         HWT_T4
            jsr         HWT_PRINT
            .byte       " task ", 0
            lda         HWT_T0
            jsr         HWT_HEX1
            lda         #':'
            jsr         HWT_PUTC
:
            lda         #' '
            jsr         HWT_PUTC
            lda         HWT_T3
            jsr         HWT_HEX1
            lda         #'>'
            jsr         HWT_PUTC
            lda         HWT_T1
            eor         #$A5
            cmp         #$10
            bcc         :+
            lda         #'?'                                ; (Not a shared bank's mark: what it read)
            jsr         HWT_PUTC
            lda         HWT_T1
            jsr         HWT_HEX2
            bra         :++
:
            jsr         HWT_HEX1
:
            lda         HWT_T3                              ; (Task 0 reading it wrong too: the RAM)
            jsr         HWT_BANK_TASK0
            ldx         HWT_T0
            ldy         HWT_T3
            jmp         @next_v

@all_set:
            ldx         #15                                 ; All set first: each task its own bank, and
                                                            ;   the bank's mark; then each read (a task
@set:                                                       ;   whose bank is skipped: not tested here)
            stz         T_REGISTER
            txa
            tay
            jsr         HWT_BANK_IS_BAD
            bne         @set_next
            stx         T_REGISTER
            txa
            ora         #$F0
            sta         RAM_BANK_REG
            txa
            eor         #$A5
            sta         $8000
@set_next:
            dex
            bpl         @set                                ; (Task 0 at the end)
            stz         T_REGISTER
            stz         HWT_T5                              ; (Tasks wrong: all of them are shown)
            ldx         #15
@check:
            stz         T_REGISTER
            txa
            tay
            jsr         HWT_BANK_IS_BAD
            bne         @checked
            stx         T_REGISTER
            txa
            eor         #$A5
            cmp         $8000
            bne         @mixed
@checked:
            dex
            bpl         @check
            stz         T_REGISTER
            lda         HWT_T2                              ; (Either way wrong: not lines 4-7 too)
            ora         HWT_T5
            beq         :+
            jmp         HWT_BANKS_F0
:
            ldx         #15                                 ; Each task: lines 4-7
@task:
            stx         T_REGISTER
            lda         #$F0
            sta         RAM_BANK_REG
            lda         #$5A
            sta         $8000                               ; (The mark in $F0)
            ldy         #3
@line:
            lda         HWT_HIGH_BITS,Y
            sta         RAM_BANK_REG                        ; $F0 without that line
            lda         #$C3
            sta         $8000                               ; (Into another bank: not $F0's mark)
            lda         #$F0
            sta         RAM_BANK_REG
            lda         $8000
            cmp         #$5A
            beq         :+
            jmp         @high
:
            dey
            bpl         @line
            dex
            bpl         @task
            jmp         HWT_BANKS_F0                        ; (Every task's bank: $F0 again)

@mixed:                                                     ; (In task .X: its bank has another's mark)
            ldy         $8000
            stz         T_REGISTER                          ; (Task 0's stack again, for the calls)
            stx         HWT_T0
            sty         HWT_T1
            inc         HWT_T5
            lda         HWT_T5
            cmp         #1
            bne         :+
            jsr         HWT_FAIL
            .byte       "all set, then read:", 0
:
            jsr         HWT_PRINT
            .byte       " task ", 0
            lda         HWT_T0
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       " bank F", 0
            lda         HWT_T0
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       " read ", 0
            lda         HWT_T1
            jsr         HWT_HEX2
            lda         HWT_T1                              ; Another task's mark: its bank, and the bits
            eor         #$A5                                ;   that differ ("bank FD's mark, bits 02")
            cmp         #$10
            bcs         @next_task
            sta         HWT_T3
            jsr         HWT_PRINT
            .byte       " (bank F", 0
            lda         HWT_T3
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       "'s mark, bits ", 0
            lda         HWT_T3
            eor         HWT_T0
            jsr         HWT_HEX2
            lda         #')'
            jsr         HWT_PUTC
@next_task:
            lda         HWT_T0                              ; (Task 0 reading it wrong too: the RAM)
            jsr         HWT_BANK_TASK0
            ldx         HWT_T0
            jmp         @checked

@high:                                                      ; (In task .X: line .Y + 4 doesn't change the bank)
            stz         T_REGISTER
            stx         HWT_T0
            tya
            clc
            adc         #4
            sta         HWT_T1
            jsr         HWT_BANKS_F0
            jsr         HWT_FAIL
            .byte       "task ", 0
            lda         HWT_T0
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       " bank line ", 0
            lda         HWT_T1
            jmp         HWT_HEX1

HWT_HIGH_BITS:  .byte   $E0, $D0, $B0, $70

; Shared bank $F0 + .Y (0-F): skipped by the bank register test (task 0 reads it wrong).  In task 0.
; Preserves .X, .Y.  Modifies: .A
HWT_BANK_SKIP:
            cpy         #8
            bcs         :+
            lda         HWT_BITS,Y
            tsb         HWT_BAD
            rts
:
            lda         HWT_BITS - 8,Y
            tsb         HWT_BAD + 1
            rts

; Is shared bank $F0 + .Y (0-F) skipped?  In task 0.  OUT: Z = 0: it is.  Preserves .X, .Y.  Modifies: .A
HWT_BANK_IS_BAD:
            cpy         #8
            bcs         :+
            lda         HWT_BITS,Y
            and         HWT_BAD
            rts
:
            lda         HWT_BITS - 8,Y
            and         HWT_BAD + 1
            rts

; A task read shared bank $F0 + .A (0-F) wrong: does task 0 read it wrong too?  Then it's the RAM (or its
; bank lines: test 3), not that task's register: " (task 0 too)".  In task 0; its RAM bank register ends as
; $F0 + 0, as the test left it.  Modifies: .A
HWT_BANK_TASK0:
            and         #$0F
            pha
            ora         #$F0
            sta         RAM_BANK_REG
            pla
            eor         #$A5                                ; (Its mark)
            cmp         $8000
            php
            lda         #$F0
            sta         RAM_BANK_REG
            plp
            beq         :+
            jsr         HWT_PRINT
            .byte       " (task 0 too)", 0
:
            rts

; ****************************************************************************
; Hold a task, for probing (not in the all-tests run): every task's RAM bank register set to $F0 + its task
; (as the bank register test sets them), then T set to the task typed (0-F) and held there, its bank at
; $8000 read over and over, until a key.  Its lines can be measured meanwhile: T0-T3 (IC1-IC4 pins 1, 15,
; 14, 13) and its RAM bank register's outputs, RAMB0-7 (IC1 and IC2 pins 5, 7, 9, 11: $F0 + the task).
HWT_T_HOLD:
            jsr         HWT_PRINT
            .byte       "task (0-F)? ", 0
:
            jsr         HWT_GETC
            bcc         :-
            jsr         HWT_PUTC
            cmp         #'a'                                ; (Upper case)
            bcc         :+
            and         #$DF
:
            sec                                             ; The digit's value: 0-9, A-F
            sbc         #'0'
            cmp         #10
            bcc         @digit
            sbc         #'A' - '0' - 10                     ; (C = 1)
            cmp         #10
            bcc         @bad
            cmp         #16
            bcs         @bad
@digit:
            sta         HWT_T0
            jsr         HWT_PRINT
            .byte       " held (a key ends it) ", 0
            jsr         HWT_TX_IDLE                         ; (All of that sent first)
            stz         U_REGISTER
            ldx         #15                                 ; Every task's bank register: $F0 + the task
:
            stx         T_REGISTER
            txa
            ora         #$F0
            sta         RAM_BANK_REG
            dex
            bpl         :-
            ldx         HWT_T0
            stx         T_REGISTER                          ; (No stack or zero page from here: that task's)
@hold:
            lda         $8000
            lda         ACIA_R_STATUS
            and         #ACIA_STATUS_BIT_RDRF
            beq         @hold
            stz         T_REGISTER
            lda         ACIA_R_DATA                         ; (The key: dropped)
            jsr         HWT_BANKS_F0
            jsr         HWT_PRINT
            .byte       "released ", 0
            rts
@bad:
            lda         #' '                                ; (After the key's echo)
            jsr         HWT_PUTC
            jsr         HWT_FAIL
            .byte       "not a task", 0
            rts

; Every task's RAM bank: shared bank $F0 (U = 0), as at the test's start.  Modifies: .A, .X
HWT_BANKS_F0:
            stz         U_REGISTER
            ldx         #15
            lda         #$F0
:
            stx         T_REGISTER
            sta         RAM_BANK_REG
            dex
            bpl         :-
            rts
