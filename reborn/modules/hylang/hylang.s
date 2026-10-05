; ****************************************************************************
; hylang - danlang on the Hydra-16 (docs/hylang.md), written again from scratch.  As yet (phase 1) its runtime: the
; values, the heap and the collector (heap.inc), which it makes, then says how it stands.  The REPL comes with
; phase 2.
;   A program of four banks: the evaluator, the dispatch and the hot built-ins in the first; the reader, the printer
; and the list built-ins in the second; the numbers in the third; strings, hashes, streams and the system in the
; fourth.  heap.inc is in the task's RAM, where each bank calls it; a bank calls another through FARN.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "hylang.inc"

            HYX2_PROGRAM "hylang", main, 4

.include "heap.inc"

.code

main:
            HYX2_BANKS_INIT
            jsr         heap_init
            bcc         :+
            PRINT       "hylang: no room for its heap"
            bra         @fail
:
            FARN        2, bank_two                         ; (Each bank answers: its number)
            cmp         #2
            bne         @banks
            FARN        3, bank_three
            cmp         #3
            bne         @banks
            FARN        4, bank_four
            cmp         #4
            beq         @ready
@banks:
            PRINT       "hylang: a bank doesn't answer"
@fail:
            PRINT       s_crlf
            LDR         r0, 0
            lda         #1
            jmp         EXITS
@ready:
            PRINT       "hylang (danlang on the Hydra-16): as yet its runtime"
            PRINT       s_crlf
            LDR         r0, 0
            lda         #0
            jmp         EXITS

.rodata
s_crlf:     .byte       CR, LF, 0

.segment "CODE2"                                            ; (The reader, the printer, the list built-ins: phase 2)
bank_two:
            lda         #2
            rts

.segment "CODE3"                                            ; (The numbers: phase 5)
bank_three:
            lda         #3
            rts

.segment "CODE4"                                            ; (Strings, hashes, streams, the system: phases 6, 7)
bank_four:
            lda         #4
            rts
