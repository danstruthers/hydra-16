; ****************************************************************************
; asm - the asm library (spec/asm.def): the W65C02S's instructions as the assembler as writes them, from one table
; (w65c02.inc) for every tool that reads or writes them: db's d, HyForth's disasm and dis call it to disassemble,
; as makes its encoding table from the same one.  A library module (HT_LIBRARY) in its own bank of the paged ROM,
; run in its caller's task through XCALL; it reads and writes the caller's memory where it's told to, and keeps
; nothing of its own between calls (its scratch: the call registers r5-r9).

.include "hydra.inc"
.include "hyx2.inc"
.include "asmlib.inc"

            HYX2_LIBRARY "asm", "HEADER", "ROM"
.include "asm_jt.inc"

.code
.include "w65c02.inc"
.include "dis.inc"
