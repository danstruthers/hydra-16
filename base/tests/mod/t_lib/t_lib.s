; ****************************************************************************
; t_lib - a library module for the xcall test (HT_LIBRARY: code alone, run in its caller's task through XCALL).  Its
; routines are a jump table after its header, at $A030 on:
;   $A030 lib_add       .A = .A + .X; r0 = r0 + r1; C = 1 (so the test sees the flags come back); .Y kept
;   $A033 lib_bank      .A = the bank register ($01) as it runs: its own bank
;   $A036 lib_say       PUTS "t_lib: a system call from a library", and C = 0

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_LIBRARY "t_lib", "HEADER", "ROM"

.code
            jmp         lib_add
            jmp         lib_bank
            jmp         lib_say

lib_add:
            stx         r2
            clc
            adc         r2
            pha
            clc
            lda         r0
            adc         r1
            sta         r0
            lda         r0 + 1
            adc         r1 + 1
            sta         r0 + 1
            pla
            sec
            rts

lib_bank:
            lda         ROM_BANK
            rts

lib_say:
            LDR         r0, s_say
            jsr         PUTS
            clc
            rts

.rodata
s_say:      .byte       "t_lib: a system call from a library", $0A, 0

.data
.bss
