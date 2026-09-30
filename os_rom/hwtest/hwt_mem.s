.debuginfo

; ****************************************************************************
; The hardware test's memory tests: task RAM (U7: every task's $0000-$7FFF), shared RAM (U25, U27-U29: 256
; banks of 8K) and the task RAM modules (memory daughter cards).  (Inside `.scope HWTEST`: hwtest.s.)
;
; Each writes a pattern that depends on the address (so a stuck, crossed or missing address line shows as
; well as a data line), reads it all back, then does the same with its complement.  Every bank also gets a
; mark of its own in its first and last bytes, all of them written before any is read back, so two banks
; that are one (a bank line, U line, task line or chip select at fault) show.  A quick run tests every byte
; of task RAM, and every byte of one bank of each chip; a full one (HWT_FULL) every byte of every bank, and
; task RAM with a second pattern.

.segment "HWT_A"

; ****************************************************************************
; Pages .A to .X - 1 of the current task's RAM (or of the bank at $8000), with the pattern (offset ^ page ^
; .Y), then its complement.  It may run in any task: its zero page is HWT_S* ($F0-$FF), which it doesn't test.
; HWT_PAGES_TO: the last page only up to offset HWT_SL (0: all of it).
; OUT: C = 0; or C = 1: .A = the page, .X = the offset, .Y = the bad bits.  Modifies: .A, .X, .Y
HWT_PAGES:
            stz         HWT_SL
HWT_PAGES_TO:
            sta         HWT_SN                              ; (The first page)
            stx         HWT_SN + 1                          ;   and the one after the last
            sty         HWT_SW                              ; (The pattern's seed)
            stz         HWT_SE                              ; (0: the pattern; $FF: its complement)

@pass:
            lda         HWT_SN                              ; Write it
            sta         HWT_SP + 1
            stz         HWT_SP

@wpage:
            jsr         @end
            lda         HWT_SP + 1
            eor         HWT_SW
            eor         HWT_SE
            sta         HWT_SV
            ldy         #0
:
            tya
            eor         HWT_SV
            sta         (HWT_SP),Y
            iny
            cpy         HWT_SX
            bne         :-
            inc         HWT_SP + 1
            lda         HWT_SP + 1
            cmp         HWT_SN + 1
            bne         @wpage
            lda         HWT_SN                              ; Then read it all back
            sta         HWT_SP + 1

@rpage:
            jsr         @end
            lda         HWT_SP + 1
            eor         HWT_SW
            eor         HWT_SE
            sta         HWT_SV
            ldy         #0
:
            tya
            eor         HWT_SV
            eor         (HWT_SP),Y
            bne         @bad
            iny
            cpy         HWT_SX
            bne         :-
            inc         HWT_SP + 1
            lda         HWT_SP + 1
            cmp         HWT_SN + 1
            bne         @rpage
            lda         HWT_SE                              ; The complement next
            eor         #$FF
            sta         HWT_SE
            bne         @pass
            clc
            rts

@bad:                                                       ; (.A = the bits, .Y = the offset)
            tax
            phx
            tya
            tax
            ply
            lda         HWT_SP + 1
            sec
            rts

@end:                                                       ; HWT_SX = this page's end: 0 (all of it), or
            stz         HWT_SX                              ;   HWT_SL on the last page
            lda         HWT_SP + 1
            inc
            cmp         HWT_SN + 1
            bne         :+
            lda         HWT_SL
            sta         HWT_SX
:
            rts

; A memory fault (.A = the page, .X = the offset, .Y = the bad bits), after what the test printed:
; "AAAA bits BB".  Modifies: .A, .Y
HWT_PAGE_FAULT:
            sty         HWT_T7
            jsr         HWT_HEX2
            txa
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       " bits ", 0
            lda         HWT_T7
            jmp         HWT_HEX2

; Test page `base` from its offset `first` with .A and .X alone (no stack, no zero page): the pattern
; (offset ^ $A5), then its complement.  A bad byte: to `fail`, .A = the bad bits, .X = the offset
.macro HWT_REG_PAGE base, first, fail
            ldx         #first
:
            txa
            eor         #$A5
            sta         base,X
            inx
            bne         :-
            ldx         #first
:
            txa
            eor         #$A5
            eor         base,X
            bne         fail
            inx
            bne         :-
            ldx         #first
:
            txa
            eor         #$5A
            sta         base,X
            inx
            bne         :-
            ldx         #first
:
            txa
            eor         #$5A
            eor         base,X
            bne         fail
            inx
            bne         :-
.endmacro

; ****************************************************************************
; Task RAM: first, each task's RAM is its own (HWT_TASK_MARKS: a T line to U7 at fault makes two tasks one,
; which the patterns, the same in every task, don't show).  Then each task's zero page (not $00-$01: the
; bank registers), its stack page, and $0200-$7FFF (task RTC_TASK's to RTC_REGS: a DS1747's clock registers
; may be there).  A task's zero page and stack page are tested with no stack and no zero page
; (HWT_REG_PAGE), then the rest with HWT_PAGES, run in the task (its stack and zero page just tested).  A
; fault: FAIL, the task, where and the bad bits; or the task, whose mark it has, and U7's lines that differ.
HWT_T_TASKRAM:
            jsr         HWT_TASK_MARKS
            bcs         @marks_bad
            ldx         #15

@task:
            stx         HWT_T0
            jsr         HWT_TASK_ONE
            bcs         @fault
            ldx         HWT_T0
            dex
            bpl         @task
            rts

@fault:                                                     ; (.A = the page, .X = the offset, .Y = the bits)
            pha
            jsr         HWT_FAIL
            .byte       "task ", 0
            lda         HWT_T0
            jsr         HWT_HEX1
            lda         #' '
            jsr         HWT_PUTC
            pla
            jmp         HWT_PAGE_FAULT

@marks_bad:                                                 ; (.X = the task, .Y = whose mark it has: $FF none)
            phy
            phx
            jsr         HWT_FAIL
            .byte       "task ", 0
            pla
            sta         HWT_T2
            jsr         HWT_HEX1
            pla
            bmi         @no_mark
            sta         HWT_T3
            jsr         HWT_PRINT
            .byte       " has task ", 0
            lda         HWT_T3
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       "'s mark: U7", 0
            lda         HWT_T2                              ; The T lines they differ by
            eor         HWT_T3
            sta         HWT_T2
            ldx         #0                                  ; .X = the T line

@line:
            lsr         HWT_T2
            bcc         @no_line
            phx
            lda         #' '
            jsr         HWT_PUTC
            txa
            asl
            tax
            lda         HWT_TL_NAMES,X
            ldy         HWT_TL_NAMES + 1,X
            jsr         HWT_PUTS_AY
            plx
@no_line:
            inx
            cpx         #4
            bne         @line
            rts

@no_mark:
            jsr         HWT_PRINT
            .byte       " no mark (U7's data lines, or U7)", 0
            rts

; The tasks' marks: each task's RAM gets a mark of its own at HWT_TMARK_AT, all of them written before any
; is read back, so two tasks that are one show.  Leaves T at 0.
; OUT: C = 0; or C = 1: .X = the task, .Y = the task whose mark it has ($FF: none of them).
; Modifies: .A, .X, .Y
HWT_TASK_MARKS:
            ldx         #15
:
            lda         HWT_TMARKS,X
            stx         T_REGISTER
            sta         HWT_TMARK_AT
            dex
            bpl         :-                                  ; (Ends in task 0)
            ldx         #15

@check:
            stx         T_REGISTER
            lda         HWT_TMARK_AT
            stz         T_REGISTER
            cmp         HWT_TMARKS,X
            bne         @bad
            dex
            bpl         @check
            clc
            rts

@bad:
            ldy         #15                                 ; Whose mark?
:
            cmp         HWT_TMARKS,Y
            beq         :+
            dey
            bpl         :-
:
            sec
            rts

HWT_TMARK_AT    = $4000
HWT_TMARKS:     .byte   $0F, $1E, $2D, $3C, $4B, $5A, $69, $78, $87, $96, $A5, $B4, $C3, $D2, $E1, $F0
HWT_TL_NAMES:   .word   HWT_SH_N6, HWT_SH_N7, HWT_SH_N0, HWT_SH_N1  ; T0-T3: U7's A15-A18

; Task .X's RAM.  OUT: C = 0; or C = 1: .A = the page, .X = the offset, .Y = the bad bits.
; Modifies: .A, .X, .Y
HWT_TASK_ONE:
            txa
            bne         @far1
            jmp         HWT_TASK_ZERO
@far1:
            ldy         HWT_MODE                            ; (The mode goes along in .Y)
            sta         T_REGISTER                          ; In task .X: no stack or zero page until tested
            HWT_REG_PAGE $00, 2, @zp_bad
            HWT_REG_PAGE $0100, 0, @st_bad
            sty         HWT_SA                              ; (The mode, in its zero page now)
            stz         HWT_SL                              ; (All of it; task RTC_TASK: up to RTC_REGS)
            lda         T_REGISTER
            cmp         #RTC_TASK
            bne         :+
            lda         #<RTC_REGS
            sta         HWT_SL
:
            lda         #$02                                ; Now they're good: the rest (the return address
            ldx         #$80                                ;   goes on its stack, the pointers in its zero
            ldy         #$A5                                ;   page)
            jsr         HWT_PAGES_TO
            bcs         @out
            bit         HWT_SA
            bpl         @out
            lda         #$02                                ; Full: a second pattern
            ldx         #$80
            ldy         #$3C
            jsr         HWT_PAGES_TO

@out:
            stz         T_REGISTER                          ; (Keeps .A, .X, .Y and C)
            rts

@zp_bad:
            ldy         #$00
            bra         @bad

@st_bad:
            ldy         #$01

@bad:                                                       ; (.A = the bits, .X = the offset, .Y = the page)
            stz         T_REGISTER
            sta         HWT_T7
            tya
            ldy         HWT_T7
            sec
            rts

; Task 0's: its zero page and stack are kept in shared bank $F0 ($8000-$81FF: $00 is $F0) while they're
; tested, and put back before anything that needs them (the fault, if there is one, kept at $8200)
HWT_TASK_ZERO:
            ldx         #0
:
            lda         $00,X
            sta         $8000,X
            lda         $0100,X
            sta         $8100,X
            inx
            bne         :-
            stz         $8200                               ; (No fault yet)
            HWT_REG_PAGE $00, 2, @zp_bad
            HWT_REG_PAGE $0100, 0, @st_bad
            bra         @back

@zp_bad:
            ldy         #$00
            bra         @bad

@st_bad:
            ldy         #$01

@bad:                                                       ; (.A = the bits, .X = the offset, .Y = the page)
            sta         $8203
            stx         $8202
            sty         $8201
            lda         #1
            sta         $8200

@back:
            ldx         #2                                  ; Both back
:
            lda         $8000,X
            sta         $00,X
            inx
            bne         :-
:
            lda         $8100,X
            sta         $0100,X
            inx
            bne         :-
            lda         $8200                               ; A fault?
            beq         :+
            lda         $8201
            ldx         $8202
            ldy         $8203
            sec
            rts
:
            lda         #$02                                ; The rest
            ldx         #$80
            ldy         #$A5
            jsr         HWT_PAGES
            bcs         :+
            bit         HWT_MODE
            bpl         :+
            lda         #$02
            ldx         #$80
            ldy         #$3C
            jsr         HWT_PAGES
:
            rts

; ****************************************************************************
; Shared RAM: each of its 256 banks (U = 0-F, bank IDs $F0-$FF) gets its mark; then every byte of one bank
; of each chip ($F0, $F4, $F8, $FC with U = 0), or of all 256 (full).  A fault: FAIL, the shared bank ID
; (U and the bank), where, and the bad bits.
HWT_T_SHARED:
            ldx         #0                                  ; The marks: shared bank ID (U << 4 | bank), and
:                                                           ;   its complement at the end
            jsr         HWT_SHARED_AT
            stx         $8000
            txa
            eor         #$FF
            sta         $9FFF
            inx
            bne         :-
            ldy         #7                                  ; Each read back: a wrong one's chip gets the
:                                                           ;   bits its ID and the mark it has differ by:
            lda         #0                                  ;   the address lines it doesn't see
            sta         HWT_SH_LINES,Y                      ;   (HWT_SH_LINES; no mark at all: HWT_SH_JUNK)
            dey
            bpl         :-
@check:
            jsr         HWT_SHARED_AT
            cpx         $8000
            bne         @mark_bad
            txa
            eor         #$FF
            cmp         $9FFF
            bne         @mark_bad
@check_next:
            inx
            bne         @check
            jsr         HWT_SHARED_BACK
            ldy         #7
:
            lda         HWT_SH_LINES,Y
            bne         @lines_bad
            dey
            bpl         :-
            ldx         #0                                  ; Then the banks' bytes: every bank (full), or
@bank:                                                      ;   a bank of each chip
            bit         HWT_MODE
            bmi         :+
            txa
            and         #$F3
            bne         @next
:
            stx         HWT_T0
            jsr         HWT_SHARED_AT
            lda         #$80
            ldx         #$A0
            ldy         HWT_T0
            jsr         HWT_PAGES
            bcs         @fault
            ldx         HWT_T0

@next:
            inx
            bne         @bank
            bra         @done

@mark_bad:                                                  ; (ID .X read wrong: its chip, .Y)
            txa
            lsr
            lsr
            and         #3
            tay
            lda         $8000
            sta         HWT_T1
            eor         #$FF
            cmp         $9FFF
            bne         @junk
            txa                                             ; Another bank's mark: the lines that differ
            eor         HWT_T1
            ora         HWT_SH_LINES,Y
            sta         HWT_SH_LINES,Y
            jmp         @check_next
@junk:
            lda         #1                                  ; No mark (data lines, or no chip)
            sta         HWT_SH_JUNK,Y
            jmp         @check_next

@lines_bad:
            jmp         HWT_SH_REPORT

@fault:
            pha
            jsr         HWT_SHARED_BACK
            jsr         HWT_FAIL
            .byte       "bank ", 0
            lda         HWT_T0
            jsr         HWT_HEX2
            lda         #' '
            jsr         HWT_PUTC
            pla
            jmp         HWT_PAGE_FAULT

@done:

; U = 0, and shared bank $F0 at $8000.  Modifies: nothing but the registers named
HWT_SHARED_BACK:
            stz         U_REGISTER
            pha
            lda         #$F0
            sta         RAM_BANK_REG
            pla
            rts

HWT_SH_LINES        = HWT_KEEP      ; (4) By chip: the ID bits its wrong marks differed by
HWT_SH_JUNK         = HWT_KEEP + 4  ; (4)   and <> 0: it had no mark at all somewhere

; The shared RAM's address faults, from the marks: FAIL, then each chip with faults, and the lines it doesn't
; see (the bits the marks read differed by): "U25: A15 (pin 31)".  The U lines (A13-A16) go to all four chips:
; the same one on every chip is the U register's line itself.  Modifies: .A, .X, .Y
HWT_SH_REPORT:
            ldx         #0                                  ; .X = the chip (ID bits 2-3)
@chip:
            lda         HWT_SH_LINES,X
            ora         HWT_SH_JUNK,X
            beq         @next_chip
            stx         HWT_T3
            jsr         HWT_FAIL
            .byte       0
            txa                                             ; Its name: HWT_SH_CHIPS + .X x 4
            asl
            asl
            clc
            adc         #<HWT_SH_CHIPS
            pha
            lda         #>HWT_SH_CHIPS
            adc         #0
            tay
            pla
            jsr         HWT_PUTS_AY
            lda         #':'
            jsr         HWT_PUTC
            ldx         HWT_T3
            lda         HWT_SH_LINES,X
            sta         HWT_T2
            ldy         #0                                  ; .Y = the ID bit
@line:
            lsr         HWT_T2
            bcc         @no_line
            phy
            lda         #' '
            jsr         HWT_PUTC
            tya
            asl
            tax
            lda         HWT_SH_NAMES,X
            ldy         HWT_SH_NAMES + 1,X
            jsr         HWT_PUTS_AY
            ply
@no_line:
            iny
            cpy         #8
            bne         @line
            ldx         HWT_T3
            lda         HWT_SH_JUNK,X
            beq         @next_chip
            jsr         HWT_PRINT
            .byte       " no mark (data or chip)", 0
@next_chip:
            inx
            cpx         #4
            bne         @chip
            rts

HWT_SH_CHIPS:   .byte   "U25", 0, "U28", 0, "U27", 0, "U29", 0     ; (By ID bits 2-3: V1's are swapped)
HWT_SH_NAMES:   .word   HWT_SH_N0, HWT_SH_N1, HWT_SH_N2, HWT_SH_N3, HWT_SH_N4, HWT_SH_N5, HWT_SH_N6, HWT_SH_N7
HWT_SH_N0:      .byte   "A17 (pin 30)", 0               ; (Bank bit 0)
HWT_SH_N1:      .byte   "A18 (pin 1)", 0                ; (Bank bit 1)
HWT_SH_N2:      .byte   "select (bank bit 2)", 0        ; (Another chip's mark)
HWT_SH_N3:      .byte   "select (bank bit 3)", 0
HWT_SH_N4:      .byte   "A13 (pin 28)", 0               ; (U0)
HWT_SH_N5:      .byte   "A14 (pin 3)", 0                ; (U1)
HWT_SH_N6:      .byte   "A15 (pin 31)", 0               ; (U2)
HWT_SH_N7:      .byte   "A16 (pin 2)", 0                ; (U3)

; Shared bank ID .X (U << 4 | bank) at $8000.  Preserves .X, .Y
HWT_SHARED_AT:
            txa
            lsr
            lsr
            lsr
            lsr
            sta         U_REGISTER
            txa
            ora         #$F0
            sta         RAM_BANK_REG
            rts

; ****************************************************************************
; The task RAM modules: the ones there are (a byte of bank $m0 keeps what's written); each bank of each
; module, in each task, gets its mark (the task and the bank); then every byte of each module's first bank
; (task 0), or of all its banks in all tasks (full).  It says which modules it found.  A fault: FAIL, the
; module's bank, the task, where, and the bad bits.
HWT_T_MODULES:
            jsr         HWT_MODS_FIND
            lda         HWT_MODS
            ora         HWT_MODS + 1
            bne         :+
            jsr         HWT_PRINT
            .byte       "none ", 0
            rts
:
            ldx         #0                                  ; Each module found
            stx         HWT_T2

@module:
            jsr         HWT_MOD_THERE
            bcc         @next_module
            lda         HWT_T2
            jsr         HWT_HEX1
            lda         #' '
            jsr         HWT_PUTC
            jsr         HWT_MOD_MARKS
            bcs         @mark_bad
            jsr         HWT_MOD_BYTES
            bcs         @fault

@next_module:
            inc         HWT_T2
            ldx         HWT_T2
            cpx         #15
            bne         @module
            rts

@mark_bad:                                                  ; (.A = the bank, .X = the task, .Y = what it read)
            sta         HWT_T0
            stx         HWT_T1
            sty         HWT_T3
            jsr         HWT_FAIL
            .byte       "bank ", 0
            lda         HWT_T0
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       " task ", 0
            lda         HWT_T1
            jsr         HWT_HEX1
            jsr         HWT_PRINT
            .byte       " read ", 0
            lda         HWT_T3
            jmp         HWT_HEX2

@fault:                                                     ; (HWT_T0 = the bank, HWT_T1 = the task)
            pha
            jsr         HWT_FAIL
            .byte       "bank ", 0
            lda         HWT_T0
            jsr         HWT_HEX2
            jsr         HWT_PRINT
            .byte       " task ", 0
            lda         HWT_T1
            jsr         HWT_HEX1
            lda         #' '
            jsr         HWT_PUTC
            pla
            jmp         HWT_PAGE_FAULT

; The modules there are: HWT_MODS bit m (a byte of bank $m0, in task 0, keeps $55 and $AA).
; Modifies: .A, .X, .Y
HWT_MODS_FIND:
            stz         HWT_MODS
            stz         HWT_MODS + 1
            ldx         #0

@module:
            txa
            asl
            asl
            asl
            asl
            sta         RAM_BANK_REG
            lda         #$55
            sta         $8000
            cmp         $8000
            bne         @next
            lda         #$AA
            sta         $8000
            cmp         $8000
            bne         @next
            txa                                             ; Module .X is there
            lsr
            lsr
            lsr
            tay
            txa
            and         #7
            phx
            tax
            lda         HWT_BITS,X
            plx
            ora         HWT_MODS,Y
            sta         HWT_MODS,Y

@next:
            inx
            cpx         #15
            bne         @module
            lda         #$F0
            sta         RAM_BANK_REG
            rts

HWT_BITS:       .byte   $01, $02, $04, $08, $10, $20, $40, $80

; Is module .X there (HWT_MODS)?  OUT: C = 1: it is.  Preserves .X
HWT_MOD_THERE:
            txa
            and         #7
            tay
            lda         HWT_BITS,Y
            cpx         #8
            bcs         :+
            and         HWT_MODS
            bra         :++
:
            and         HWT_MODS + 1
:
            cmp         #1                                  ; (C = 1: a bit)
            rts

; Module HWT_T2's marks: in each task, each of its 16 banks gets (the task << 4 ^ the bank ID) at its start
; and the complement at its end, all written before any is read.  (In task t, its zero page's HWT_SV holds
; the bank ID: no other memory is used with T switched.)
; OUT: C = 0; or C = 1: .A = the bank, .X = the task, .Y = what it read.  Modifies: .A, .X, .Y
HWT_MOD_MARKS:
            lda         HWT_T2
            asl
            asl
            asl
            asl
            sta         HWT_T3                              ; (Its first bank)
            ldx         #15

@wtask:
            ldy         HWT_T3
            stx         T_REGISTER
:
            sty         RAM_BANK_REG
            sty         HWT_SV
            txa
            asl
            asl
            asl
            asl
            eor         HWT_SV
            sta         $8000
            eor         #$FF
            sta         $9FFF
            iny
            tya
            and         #$0F
            bne         :-
            stz         T_REGISTER
            dex
            bpl         @wtask
            ldx         #15

@rtask:
            ldy         HWT_T3
            stx         T_REGISTER
:
            sty         RAM_BANK_REG
            sty         HWT_SV
            txa
            asl
            asl
            asl
            asl
            eor         HWT_SV
            cmp         $8000
            bne         @bad
            eor         #$FF
            cmp         $9FFF
            bne         @bad
            iny
            tya
            and         #$0F
            bne         :-
            lda         #$F0
            sta         RAM_BANK_REG
            stz         T_REGISTER
            dex
            bpl         @rtask
            clc
            rts

@bad:                                                       ; (In task .X, bank .Y)
            lda         $8000
            sta         HWT_SW                              ; (In task .X's zero page, until .A is free)
            lda         #$F0
            sta         RAM_BANK_REG
            tya
            ldy         HWT_SW
            stz         T_REGISTER
            sec
            rts

; Module HWT_T2's bytes: its first bank, in task 0; or (full) all its banks, in each task (HWT_PAGES run in
; the task).  OUT: C = 0; or C = 1: as HWT_PAGES's, HWT_T0 = the bank, HWT_T1 = the task
; Modifies: .A, .X, .Y
HWT_MOD_BYTES:
            lda         HWT_T2
            asl
            asl
            asl
            asl
            sta         HWT_T0
            stz         HWT_T1

@bank:
            ldy         HWT_T0                              ; (Task 0's: read before T changes)
            ldx         HWT_T1
            stx         T_REGISTER                          ; The task, and the bank in its bank register
            sty         RAM_BANK_REG
            lda         #$80
            ldx         #$A0
            jsr         HWT_PAGES                           ; (Seed: the bank ID, .Y)
            pha
            lda         #$F0
            sta         RAM_BANK_REG
            pla
            stz         T_REGISTER
            bcs         @done
            bit         HWT_MODE
            bpl         @ok                                 ; (Quick: the first bank, in task 0)
            inc         HWT_T0                              ; Full: the next bank; then the next task
            lda         HWT_T0
            and         #$0F
            bne         @bank
            lda         HWT_T0
            sec
            sbc         #16
            sta         HWT_T0
            inc         HWT_T1
            lda         HWT_T1
            cmp         #16
            bne         @bank

@ok:
            clc

@done:
            rts
