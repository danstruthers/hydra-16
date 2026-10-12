; ****************************************************************************
; asm - the asm library (spec/asm.def): the W65C02S's instructions as the assembler as writes them, for every tool
; that reads or writes them.  Its assembler (core.s, expr.s, stmt.s: phase 9's as) is the as program's and BASIC's
; ASM blocks'; its disassembler (dis.inc) is db's d's, HyForth's disasm's and dis's; one table of the instructions
; (w65c02.inc) for both.  A library module (HT_LIBRARY) of one bank of the paged ROM, run in its caller's task through
; XCALL: the jump table, the assembler, and the disassembler with its copy of the table (a second bank's once: the
; two together leave 870 bytes of the bank).
; It has no RAM of its own: the assembler's state is in RAM its caller lends it for a call (ASM_RAM: asm.cfg's RAM),
; its zero page the caller's, kept and put back; the disassembler's scratch is the call registers (r5-r9).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "asmlib.inc"

.import     asm_file, asm_begin, asm_pass, asm_define, asm_line, asm_end, asm_symbol, asm_image, asm_done, asm_error

            HYX2_LIBRARY "asm", "HEADER", "ROM"
.include "asm_jt.inc"

; ---- The disassembler, and its copy of the table (its own scope, as when it was a second bank's)
.code
.scope b2
.include "w65c02.inc"
.include "dis.inc"
.endscope

.code
; DIS: the disassembler's (b2::as_dis)
as_dis:
            jmp         b2::as_dis
