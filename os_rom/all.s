.debuginfo
.macpack        cpu
.setcpu     "65C02"

.include "defines.s"
.include "zero.s"       ; this should always be after defines.s
.include "bios.s"
.include "thunks.s"
.include "math.s"
.include "tasks.s"
.include "disasm.s"
.include "wozmon.s"
.include "mmu.s"
.include "shell.s"
.include "sound.s"
.include "os_main.s"
