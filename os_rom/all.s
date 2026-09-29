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

; BIOS ROM page 5 (W = 5): far pointers and references.  First: the other pages' gates refer to PAGE5::
.scope PAGE5
.include "kernel/page5.s"       ; must be first in the scope
.include "kernel/fp.s"          ; Far pointers and references
.endscope

; BIOS ROM page 2 (W = 2): the IO layer.  Its own scope, so page 2 code binds to the page 2 gates in
; page2.s.  Before PAGE1, whose gates refer to PAGE2::
.scope PAGE2
.include "io/page2.s"           ; must be first in the scope
.include "io/io.s"
.include "io/ns.s"              ; Per-task namespaces (IO_MOUNT, IO_BIND)
.include "io/ser_srv.s"         ; The serial driver's file server (/dev/cons, /dev/ser)
.include "io/serctl.s"          ; Its settings: /dev/ser/ctl, the rate and format IO_CTLs
.include "io/serfast.s"         ; Its fast paths: the ACIA's interrupt, console output and input
.include "io/snd_srv.s"         ; The sound driver's file server (/dev/snd)
.include "drivers/snd_test.s"   ; The sound driver's test tune
.include "io/pipe_srv.s"        ; The pipe server (/dev/pipe)
.include "io/proc_srv.s"        ; The tasks (/dev/proc)
.endscope
IRQ_FAST_P2     = PAGE2::IRQ_FAST_P2    ; (For the COMMON block's fast IRQ stubs, assembled before page 2)

; BIOS ROM page 3 (W = 3): storage.  Its own scope, so page 3 code binds to the page 3 gates in page3.s
.scope PAGE3
.include "drivers/page3.s"      ; must be first in the scope
.include "drivers/spi.s"        ; SPI (bit-banged on the VIA's port B)
.include "drivers/sd.s"         ; The SD card (blocks)
.include "io/sd_srv.s"          ; /dev/sd, and the storage task's init
.endscope

; BIOS ROM page 6 (W = 6): the HydraFS server, in the storage task, on page 3's block layer.  After PAGE3,
; whose names its gates use; page 3 reaches it through the aliases after the scope.
.scope PAGE6
.include "io/page6.s"           ; must be first in the scope
.include "io/hfs_srv.s"         ; The HydraFS server (/sd/N/..., the files on the cards): requests, reading
.include "io/hfs_write.s"       ;   and writing: allocating, create, remove, wstat, format
.include "io/hfs_check.s"       ;   its check, and a card's details for its ctl file
.endscope
HFS_FORGET_P6   = PAGE6::HFS_FORGET     ; (For page 3's gates: PAGE3 is assembled before PAGE6)
HFS_FORMAT_P6   = PAGE6::HFS_FORMAT
HFS_LABEL_P6    = PAGE6::HFS_LABEL
HFS_CHECK_P6    = PAGE6::HFS_CHECK
HFS_CTL_LINES_P6 = PAGE6::HFS_CTL_LINES

; BIOS ROM page 4 (W = 4): the self tests.  Its own scope, so page 4 code binds to the page 4 gates in
; page4.s
.scope PAGE4
.include "tests/page4.s"        ; must be first in the scope
.include "tests/mmu_test.s"
.include "tests/sched_test.s"
.include "tests/io_test.s"
.include "tests/post.s"         ; POST (the power-on self test)
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
