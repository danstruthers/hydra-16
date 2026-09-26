.debuginfo
.macpack        cpu
.pc02

.include "defines.s"
.include "zero.s"       ; this should always be after defines.s
.include "common.s"     ; COMMON block (every ROM page) and gate macros

; BIOS ROM page 1 (W = 1).  Its own scope, so page 1 code binds to the page 1 gates in page1.s
.scope PAGE1
.include "page1.s"      ; must be first in the scope
.include "mmu_test.s"
.include "disasm.s"
.include "hyforth/hyforth.s"
.endscope

; BIOS ROM page 0 (W = 0)
.include "bios.s"
.include "thunks.s"
.include "math.s"
.include "tasks.s"
.include "wozmon.s"
.include "mmu.s"
.include "irq.s"
.include "msg.s"
.include "shared.s"
.include "shell.s"
.include "sound.s"
.include "os_main.s"
.include "page0_gates.s"
