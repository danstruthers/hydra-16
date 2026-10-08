; ****************************************************************************
; nbasic - the Hydra's BASIC (docs/basic.md; the plan, docs/design/plans/BASIC.md): a structured BASIC in
; QuickBASIC's way, on the Hydra's numbers.  A program is compiled whole (its text read, its names made slots, its
; labels addresses, its blocks linked) into the code of a stack machine, then run by the interpreter.  `nbasic` at
; rc's prompt starts it (its prompt: a line run at once, or a line of the program with its number first); `nbasic
; file args` runs a file.
;   A module of eight banks: the interpreter and the top (run.inc, main.inc, fn.inc) in the first; the compiler in the
; second (lex.inc, comp.inc, expr.inc, stmt.inc) and the third (stmt3.inc); the numbers in the fourth (num.inc); the
; heap and strings in the fifth (heap.inc); input and output in the sixth (io.inc); the system, the errors' messages
; and the program's text in the seventh (sys.inc, prog.inc).  What every bank calls is in the task's RAM (ram.inc).

.setcpu "65C02"

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "numbers.inc"
.include "nbasic.inc"
.include "zp.inc"

            HYX2_PROGRAM "nbasic", main, 8

.include "ram.inc"
.include "fn.inc"
.include "run.inc"
.include "main.inc"
.include "lex.inc"
.include "comp.inc"
.include "expr.inc"
.include "stmt.inc"
.include "procs.inc"
.include "stmt3.inc"
.include "num.inc"
.include "heap.inc"
.include "io.inc"
.include "sys.inc"
.include "prog.inc"

; (The other banks' segments, so each is linked even before it has code)
.segment "CODE8"
            .byte       0
