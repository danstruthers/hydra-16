.debuginfo
.macpack        cpu
.pc02

; The OS ROM, as one assembly.  Folders: include/ (constants and macros), kernel/, io/ (the IO layer and
; the file servers), drivers/, tests/, monitor/ (WOZMON, the disassembler), hyforth/.  Build: makeC02.bat.

.include "include/hw.inc"       ; The hardware: ports, chip registers, IRQ numbers
.include "include/kernel.inc"   ; Task numbers, error codes, task ZP
.include "include/io.inc"       ; IO, namespaces, the drivers' and servers' constants
.include "include/ascii.inc"
.include "include/macros.inc"
.include "include/zero.s"       ; The OS ZP (after the constants)
.include "kernel/common.s"      ; COMMON block (every ROM page) and gate macros

; BIOS ROM page 2 (W = 2): the IO layer.  Its own scope, so page 2 code binds to the page 2 gates in
; page2.s.  Before PAGE1, whose gates refer to PAGE2::
.scope PAGE2
.include "io/page2.s"           ; must be first in the scope
.include "io/io.s"
.include "io/ns.s"              ; Per-task namespaces (IO_MOUNT, IO_BIND)
.include "io/ser_srv.s"         ; The serial driver's file server (/dev/cons, /dev/ser)
.include "io/snd_srv.s"         ; The sound driver's file server (/dev/snd)
.include "drivers/snd_test.s"   ; The sound driver's test tune
.include "io/pipe_srv.s"        ; The pipe server (/dev/pipe)
.include "io/proc_srv.s"        ; The tasks (/dev/proc)
.endscope

; BIOS ROM page 3 (W = 3): storage.  Its own scope, so page 3 code binds to the page 3 gates in page3.s
.scope PAGE3
.include "drivers/page3.s"      ; must be first in the scope
.include "drivers/spi.s"        ; SPI (bit-banged on the VIA's port B)
.include "drivers/sd.s"         ; The SD card (blocks)
.include "io/sd_srv.s"          ; /dev/sd, and the storage task's init
.endscope

; BIOS ROM page 4 (W = 4): the self tests.  Its own scope, so page 4 code binds to the page 4 gates in
; page4.s
.scope PAGE4
.include "tests/page4.s"        ; must be first in the scope
.include "tests/mmu_test.s"
.include "tests/sched_test.s"
.include "tests/io_test.s"
.include "tests/post_ram.s"     ; POST paged RAM line tests
.endscope

; BIOS ROM page 1 (W = 1).  Its own scope, so page 1 code binds to the page 1 gates in page1.s
.scope PAGE1
.include "hyforth/page1.s"      ; must be first in the scope
.include "monitor/disasm.s"
.include "hyforth/hyforth.s"
.endscope

; BIOS ROM page 0 (W = 0)
.include "kernel/print.s"
.include "drivers/serial.s"
.include "drivers/via.s"
.include "kernel/vectors.s"
.include "kernel/thunks.s"
.include "kernel/tasks.s"
.include "monitor/wozmon.s"
.include "kernel/mmu.s"
.include "kernel/irq.s"
.include "kernel/shared.s"
.include "io/io_p0.s"
.include "drivers/storage.s"    ; The storage task (page 0 part)
.include "drivers/sound.s"
.include "kernel/os_main.s"
.include "kernel/page0_gates.s"
