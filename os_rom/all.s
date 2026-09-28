.debuginfo
.macpack        cpu
.pc02

.include "defines.s"
.include "zero.s"       ; this should always be after defines.s
.include "common.s"     ; COMMON block (every ROM page) and gate macros

; BIOS ROM page 2 (W = 2): the IO layer.  Its own scope, so page 2 code binds to the page 2 gates in
; page2.s.  Before PAGE1, whose gates refer to PAGE2::
.scope PAGE2
.include "page2.s"      ; must be first in the scope
.include "io.s"
.include "ns.s"         ; Per-task namespaces (IO_MOUNT, IO_BIND)
.include "ser_srv.s"    ; The serial driver's file server (/dev/cons, /dev/ser)
.include "snd_srv.s"    ; The sound driver's file server (/dev/snd)
.include "snd_test.s"   ; The sound driver's test tune
.include "pipe_srv.s"   ; The pipe server (/dev/pipe)
.include "io_test.s"
.include "post_ram.s"   ; POST paged RAM line tests
.endscope

; BIOS ROM page 3 (W = 3): storage.  Its own scope, so page 3 code binds to the page 3 gates in page3.s
.scope PAGE3
.include "page3.s"      ; must be first in the scope
.include "spi.s"        ; SPI (bit-banged on the VIA's port B)
.include "sd.s"         ; The SD card (blocks)
.include "sd_srv.s"     ; /dev/sd, and the storage task's init
.endscope

; BIOS ROM page 1 (W = 1).  Its own scope, so page 1 code binds to the page 1 gates in page1.s
.scope PAGE1
.include "page1.s"      ; must be first in the scope
.include "mmu_test.s"
.include "sched_test.s"
.include "disasm.s"
.include "hyforth/hyforth.s"
.endscope

; BIOS ROM page 0 (W = 0)
.include "bios.s"
.include "thunks.s"
.include "tasks.s"
.include "wozmon.s"
.include "mmu.s"
.include "irq.s"
.include "shared.s"
.include "io_p0.s"
.include "storage.s"    ; The storage task (page 0 part)
.include "shell.s"
.include "sound.s"
.include "os_main.s"
.include "page0_gates.s"
