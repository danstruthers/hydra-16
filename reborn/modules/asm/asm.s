; ****************************************************************************
; asm - the asm library (spec/asm.def): the W65C02S's instructions as the assembler as writes them, for every tool
; that reads or writes them.  Its assembler (core.s, expr.s, stmt.s: phase 9's as) is the as program's and BASIC's
; ASM blocks'; its disassembler (dis.inc) is db's d's, HyForth's disasm's and dis's; one table of the instructions
; (w65c02.inc) for both.  A library module (HT_LIBRARY) of two banks of the paged ROM, run in its caller's task through
; XCALL: the assembler and the jump table in the first, the disassembler (and its copy of the table) in the second.
; It has no RAM of its own: the assembler's state is in RAM its caller lends it for a call (ASM_RAM: asm.cfg's RAM),
; its zero page the caller's, kept and put back; the disassembler's scratch is the call registers (r5-r9).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "asmlib.inc"

.import     asm_file, asm_begin, asm_pass, asm_define, asm_line, asm_end, asm_symbol, asm_image, asm_done, asm_error

            HYX2_LIBRARY "asm", "HEADER", "ROM2", 2
.include "asm_jt.inc"

; ---- The second bank: the disassembler, and its copy of the table
.segment "CODE2"
.scope b2
W65_BANK2       = 1
.include "w65c02.inc"
.include "dis.inc"
.endscope

.code
; DIS: the second bank's (b2::as_dis), through XCALL
as_dis:
            pha
            lda         #<b2::as_dis
            sta         r15
            lda         #>b2::as_dis
            sta         r15 + 1
            lda         ROM_BANK
            inc         a
            sta         r14
            pla
            jmp         XCALL
