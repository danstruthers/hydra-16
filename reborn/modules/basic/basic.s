; ****************************************************************************
; basic - the Hydra's BASIC (docs/design/basic.md; the plan, docs/design/plans/BASIC.md): a structured BASIC in
; QuickBASIC's way, on the Hydra's numbers.  A program is compiled whole (its text read, its names made slots, its
; labels addresses, its blocks linked) into the code of a stack machine, then run by the interpreter.  `basic` at
; rc's prompt starts it (its prompt: a line run at once, or a line of the program with its number first); `basic
; file args` runs a file.
;   A module of five banks: the interpreter and the top (run.inc, main.inc, fn.inc) in the first; the compiler in the
; second (lex.inc, comp.inc, expr.inc, stmt.inc) and the third (stmt3.inc; ASM's blocks, asm.inc: the asm library's
; assembler), with input and output (io.inc); the numbers in the fourth (num.inc), with the shell (basic -l: shell.inc),
; the machine's statements and graphics (machine.inc, gfx.inc); the heap and strings in the fifth (heap.inc), with the
; system, the errors' messages and the program's text (sys.inc, prog.inc); nslib's newns in the first.  (Eight banks
; once, filled a half on average: a bank's code calls another's with FARN, so whole banks were merged.)  What every bank calls is in the task's RAM
; (ram.inc).

.setcpu "W65C02"

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "numbers.inc"
.include "asmlib.inc"
.include "basic.inc"
.include "zp.inc"

            HYX2_PROGRAM "basic", main, 5

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
.include "rec.inc"
.include "asm.inc"
.include "num.inc"
.include "fns4.inc"
.include "heap.inc"
.include "gc.inc"
.include "fns5.inc"
.include "io.inc"
.include "sys.inc"
.include "prog.inc"
.segment "CODE5"                                            ; (The last bank named in a .s: build.js links five)
.segment "CODE4"                                            ; (The fourth bank's too: the shell, the machine, graphics)
.include "shell.inc"
.include "machine.inc"
.include "gfx.inc"

; nslib (the SDK's: newns, basic -l's), its code in the first bank, its buffers at $8000 in a bank of their own while
; it runs (newns_do)
NS_BSS      = $8000
.include "nslib.s"
.assert NS_BSS_SIZE <= $2000, error, "nslib's buffers: a bank"
