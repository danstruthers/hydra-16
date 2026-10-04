; ****************************************************************************
; hylang - danlang on the Hydra-16 (docs/hylang.md; the plan's §17, phase 7).  As yet its heap (heap.inc: spike
; S5, step 7.1, tested by tests/mod/t_heap); the reader, printer and evaluator come with step 7.2.

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "hylang", main

main:
            jsr         heap_init
            bcs         @fail
            PRINT       "hylang: its heap is ready; the language comes with step 7.2"
            PRINT       s_crlf
            lda         #0
            rts
@fail:
            PRINT       "hylang: no room for its heap"
            PRINT       s_crlf
            lda         #1
            rts

s_crlf:     .byte       CR, LF, 0

.include "heap.inc"
