; ****************************************************************************
; basic - EhyBASIC: Microsoft BASIC 2A for the 6502 (Michael Steil's mist64/msbasic, from Ben Eater's port, and
; EhyBASIC's for the Hydra-16, burntcouch/ehybasic), a program run in place from its paged ROM bank.  `basic` at rc's
; prompt starts it; `basic <file` runs a file's lines as typed ones.  Microsoft's code is as it was, its conditional
; assembly settled for this build (the other machines' code taken out), but where the system wanted it changed:
; its zero page into the program's ($22-$7F: zeropage.inc), its line buffer out of it (a page in RAM), CHRGET in
; ROM, the keywords and error messages in full again (EhyBASIC's short forms aliases: token.inc), letters in either
; case, and its I/O the system's (hyio.inc: fd 1 buffered, stdin a line at a time, GET's raw keys, Ctrl-C a note).
; Its memory is the task's RAM after the BSS, to MEM_END.
;   The parts, in Microsoft's order: token.inc (the keywords), error.inc and message.inc, memory.inc (the stack's
; frames, block moves), program.inc (errors, the warm start, lines entered, the tokenizer, LIST), flow1.inc and
; flow2.inc (statements: FOR, NEWSTT, GOTO, GOSUB, IF ...), misc1.inc (LET), print.inc, input.inc (GET, INPUT, READ),
; eval.inc (NEXT, expressions), var.inc and array.inc (variables), misc2.inc (FRE, DEF FN), string.inc, poke.inc
; (PEEK, POKE, WAIT), float.inc, chrget.inc, rnd.inc, trig.inc; then hyio.inc, the system's.

.feature force_range
.setcpu "65C02"
.macpack longbranch

.include "hydra.inc"
.include "hyx2.inc"

            HYX2_PROGRAM "basic", main

.include "defines.inc"
.include "bmacros.inc"
.include "zeropage.inc"

.include "token.inc"
.include "error.inc"
.include "message.inc"
.include "memory.inc"
.include "program.inc"
.include "flow1.inc"
.include "flow2.inc"
.include "misc1.inc"
.include "print.inc"
.include "input.inc"
.include "eval.inc"
.include "var.inc"
.include "array.inc"
.include "misc2.inc"
.include "string.inc"
.include "poke.inc"
.include "float.inc"
.include "chrget.inc"
.include "rnd.inc"
.include "trig.inc"
.include "hyio.inc"
