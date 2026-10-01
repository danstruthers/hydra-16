.debuginfo
.macpack        cpu
.pc02

; The OS ROM, as one assembly.  Folders: include/ (constants and macros), kernel/, io/ (the IO layer:
; fds, namespaces, pipes), servers/ (the devices' file servers), fs/ (HydraFS), drivers/, sound/, shell/,
; tests/, monitor/ (WOZMON, the disassembler), hyforth/.  Build: build.js (makeC02.bat runs it).

.include "include/hw.inc"       ; The hardware: ports, chip registers, IRQ numbers
.include "include/kernel.inc"   ; Task numbers, error codes, task ZP
.include "include/io.inc"       ; IO, namespaces, the drivers' and servers' constants
.include "include/ascii.inc"
.include "include/macros.inc"
.include "obj/version.inc"      ; HY_VERSION (build.js makes it from VERSION)
.include "include/thunks.inc"     ; The thunk table's list (kernel/thunks.s, hyforth/page1.s)
.include "include/shell.inc"    ; The shell's commands (page 7)
.include "include/hwtest.inc"   ; The hardware test (paged ROM bank 1)
.include "include/zero.s"       ; The OS ZP (after the constants)
.include "kernel/high.s"        ; Each page's room at $FE00 (its segment)
.include "kernel/common.s"      ; COMMON block (every ROM page) and gate macros

; BIOS ROM page 5 (W = 5): far pointers and references.  First: the other pages' gates refer to PAGE5::
.scope PAGE5
.include "kernel/page5.s"       ; must be first in the scope
.include "kernel/fp.s"          ; Far pointers and references
.include "kernel/sem.s"         ; Semaphores
.include "kernel/exits.s"       ; Exit statuses (TASK_EXITS, TASK_JOIN)
.endscope

; BIOS ROM page 2 (W = 2): the IO layer.  Its own scope, so page 2 code binds to the page 2 gates in
; page2.s.  Before PAGE1, whose gates refer to PAGE2::
.scope PAGE2
.include "io/page2.s"           ; must be first in the scope
.include "io/io.s"
.include "io/ns.s"              ; Per-task namespaces (IO_MOUNT, IO_BIND)
.include "servers/ser_srv.s"         ; The serial driver's file server (/dev/cons, /dev/ser)
.include "servers/serctl.s"          ; Its settings: /dev/ser/ctl, the rate and format IO_CTLs
.include "servers/serfast.s"         ; Its fast paths: the ACIA's interrupt, console output and input
.include "io/pipe_srv.s"        ; The pipe server (/dev/pipe)
.include "sound/ymfast.s"       ; The YM2151's interrupt: the sound clock (a fast handler, as serfast.s's)
.endscope
IRQ_FAST_P2     = PAGE2::IRQ_FAST_P2    ; (For the COMMON block's fast IRQ stubs, assembled before page 2)

; BIOS ROM page 3 (W = 3): storage.  Its own scope, so page 3 code binds to the page 3 gates in page3.s
.scope PAGE3
.include "drivers/page3.s"      ; must be first in the scope
.include "drivers/spi.s"        ; SPI (bit-banged on the VIA's port B)
.include "drivers/sd.s"         ; The SD card (blocks)
.include "servers/sd_srv.s"          ; /dev/sd, and the storage task's init
.include "servers/ramdisk.s"         ; The RAM disks: started, stopped, their ctl lines
.include "fs/hfs_format.s"      ; HydraFS: format and label (the rest of it is on page 6) ...
.include "fs/hfs_check.s"       ;   its check, and a card's details for its ctl file
.endscope

; BIOS ROM page 6 (W = 6): the HydraFS server, in the storage task, on page 3's block layer.  After PAGE3,
; whose names its gates use; page 3 reaches it through the aliases after the scope.
.scope PAGE6
.include "fs/page6.s"           ; must be first in the scope
.include "fs/hfs_srv.s"         ; The HydraFS server (/sd/N/..., the files on the cards): requests, reading
.include "fs/hfs_write.s"       ;   and writing: allocating, create, remove, wstat
.include "fs/hfs_sparse.s"      ;   and sparse files: holes, and writes past a file's end
.endscope
HFS_FORGET_P6   = PAGE6::HFS_FORGET     ; (For page 3's gates: PAGE3 is assembled before PAGE6)
HFS_META_NEW_P6 = PAGE6::HFS_META_NEW    ; (Format and label: fs/hfs_format.s)
HFS_META_AT_P6  = PAGE6::HFS_META_AT
HFS_META_CHANGED_P6 = PAGE6::HFS_META_CHANGED
HFS_FINISH_P6   = PAGE6::HFS_FINISH
HFS_SHR_P6      = PAGE6::HFS_SHR
HFS_VOLUME_P6   = PAGE6::HFS_VOLUME
HFS_SB_GET_P6   = PAGE6::HFS_SB_GET
HFS_AT_P6       = PAGE6::HFS_AT          ; (The check: fs/hfs_check.s)
HFS_AT_END_P6   = PAGE6::HFS_AT_END
HFS_CARD_X_P6   = PAGE6::HFS_CARD_X
HFS_EACH_RUN_P6 = PAGE6::HFS_EACH_RUN
HFS_ENT_READ_P6 = PAGE6::HFS_ENT_READ
HFS_FILE_BLOCK_P6 = PAGE6::HFS_FILE_BLOCK
HFS_LOAD_P6     = PAGE6::HFS_LOAD
HFS_MAP_BIT_P6  = PAGE6::HFS_MAP_BIT
HFS_MAP_CHANGED_P6 = PAGE6::HFS_MAP_CHANGED
HFS_PUT_P6      = PAGE6::HFS_PUT
HFS_PUT_DEC_P6  = PAGE6::HFS_PUT_DEC
HFS_CK_RUN_P6   = PAGE6::HFS_CK_RUN      ; (Page 6's gate to the check's HFS_CK_RUN)

; BIOS ROM page 4 (W = 4): the self tests.  Its own scope, so page 4 code binds to the page 4 gates in
; page4.s
.scope PAGE4
.include "tests/page4.s"        ; must be first in the scope
.include "monitor/wozmon.s"     ; WOZMON (MON_START: page 0 and the others reach it through gates)
.include "tests/mmu_test.s"
.include "tests/sched_test.s"
.include "tests/io_test.s"
.include "tests/post.s"         ; POST (the power-on self test)
.include "tests/post_ram.s"     ; POST paged RAM line tests
.endscope
MON_START_P4    = PAGE4::MON_START      ; (WOZMON: page 0's, 1's and 7's gates)

; BIOS ROM page 1 (W = 1): HyForth.  Its own scope, so page 1 code binds to the page 1 gates in page1.s.
; Inside it, scope FAR is BIOS ROM page A (W = $A): HyForth's far words and the disassembler
; (hyforth/farwords.s); page 0 and page 1 reach it through the aliases after the scope.
.scope PAGE1
.include "hyforth/page1.s"      ; must be first in the scope
.include "hyforth/hyforth.s"
.endscope
FW_ENTRY_PA     = PAGE1::FAR::FW_ENTRY  ; (For page 1's gates: page1.s)
LINE_START_PA   = PAGE1::FAR::LINE_START
LINE_PROMPT_PA  = PAGE1::FAR::LINE_PROMPT
DISASM_WM_PA    = PAGE1::FAR::DISASM_WM   ; (WOZMON's, page 4)
LINE_READ_PA    = PAGE1::FAR::LINE_READ
LINE_EOF_PA     = PAGE1::FAR::LINE_EOF
LINE_EXITS_PA   = PAGE1::FAR::LINE_EXITS
INCOPEN_PA      = PAGE1::FAR::INCOPEN
INCOPENFD_PA    = PAGE1::FAR::INCOPENFD
INCEND_PA       = PAGE1::FAR::INCEND
INCCOUNT_PA     = PAGE1::FAR::INCCOUNT
INCABORT_PA     = PAGE1::FAR::INCABORT
RUNNAME_PA      = PAGE1::FAR::RUNNAME
wrterror_PA     = PAGE1::FAR::wrterror
MALLOC_PA       = PAGE1::FAR::MALLOC
DISASM_PA       = PAGE1::FAR::DISASM
DISASM_AY_PA    = PAGE1::FAR::DISASM_AY

; BIOS ROM page 7 (W = 7): the shell: its boot, the prompt, file commands, running programs.  After PAGE1,
; whose RAM (HyForth's) it uses; page 1 reaches it through the aliases after the scope.
.scope PAGE7
.include "shell/page7.s"        ; must be first in the scope
.include "shell/shell.s"
.include "shell/files.s"        ; Its file and card commands (SH_CMD)
.include "shell/run.s"          ; Running programs: run, a program's name, the loader
.include "shell/redir.s"        ; Redirection: >, >> and <
.endscope
SH_BOOT_P7      = PAGE7::SH_BOOT        ; (For page 0 and page 1's gates: PAGE1 is assembled before PAGE7)
SH_CMDSHELL_P7  = PAGE7::SH_CMDSHELL    ; (The SHELL_CMD thunk's gate)
SH_PROMPT_P7    = PAGE7::SH_PROMPT
SH_CD_P7        = PAGE7::SH_CD
SH_PWD_P7       = PAGE7::SH_PWD
SH_CMD_P7       = PAGE7::SH_CMD
SH_REDIR_P7     = PAGE7::SH_REDIR
SH_UNREDIR_P7   = PAGE7::SH_UNREDIR
SH_INSAVE_P7    = PAGE7::SH_INSAVE

; BIOS ROM page 8 (W = 8): the text editor, a ROM program the shell starts in a task of its own (edit)
.scope PAGE8
.include "shell/page8.s"        ; must be first in the scope
.include "shell/edit.s"
.endscope
ED_MAIN_P8      = PAGE8::ED_MAIN        ; (For page 7: SH_EDIT)

; BIOS ROM page 9 (W = 9): the system's servers that run in their client's task (/dev/proc, /env)
.scope PAGE9
.include "servers/page9.s"           ; must be first in the scope
.include "servers/proc_srv.s"        ; The tasks (/dev/proc)
.include "servers/env_srv.s"         ; Each task's environment (/env)
.include "servers/time_srv.s"        ; The clock (/dev/time)
.include "servers/ram_srv.s"         ; The RAM itself, for task 0 (/dev/ram)
.include "drivers/rtc.s"             ; The clock chip (a DS1747 in U7)
.endscope
PROC_SERVE_P9   = PAGE9::PROC_SERVE     ; (For page 0's gates: io_p0.s)
ENV_SERVE_P9    = PAGE9::ENV_SERVE
PROC_MEM_COUNT_P9 = PAGE9::PROC_MEM_COUNT
ENV_COPY_P9     = PAGE9::ENV_COPY       ; (For page 2: IO_INHERIT)
ENV_INIT_P9     = PAGE9::ENV_INIT       ; (For page 7: SH_BOOT)
TIME_SERVE_P9   = PAGE9::TIME_SERVE     ; (For page 0's gates: io_p0.s)
RAM_SERVE_P9    = PAGE9::RAM_SERVE      ; (... and thunks.s)
CLOCK_GET_P9    = PAGE9::CLOCK_GET      ; (For page 6: HydraFS's stamps)
CLOCK_TEXT_P9   = PAGE9::TIME_TEXT      ; (For page 7: ls -l)
RTC_BOOT_P9     = PAGE9::RTC_BOOT       ; (For page 7: SH_BOOT)
TIME_DIV8_P9    = PAGE9::TIME_DIV8

; BIOS ROM page B (W = $B): sound (the YM2151: its library, /dev/snd, the test tune, the bell)
.scope PAGEB
.include "sound/pageb.s"         ; must be first in the scope
.include "sound/ym.s"           ; The YM2151's set-up and register writes
.include "sound/snd_lib.s"      ; Its library: the registers' shadow, volumes, notes, patches, commands
.include "sound/patches.s"      ;   and its data: the patches, the drum map, the volume curve
.include "sound/snd_srv.s"      ; The sound driver's file server (/dev/snd)
.include "sound/beep.s"         ; The console bell
.endscope

; BIOS ROM page C (W = $C): the song player (ZSM), a ROM program the shell starts in a task of its own (play)
.scope PAGEC
.include "sound/pagec.s"         ; must be first in the scope
.include "sound/player.s"       ; The song player
.endscope
ZSM_PLAY_PC     = PAGEC::ZSM_PLAY       ; (For page 7: SH_SONG)
ZSM_PLAY_TEST_PC = PAGEC::ZSM_PLAY_TEST ; (For page B: SND_CTL_TEST)

; BIOS ROM page 0 (W = 0)
.include "kernel/print.s"
.include "drivers/serial.s"
.include "drivers/via.s"
.include "kernel/vectors.s"
.include "kernel/thunks.s"
.include "kernel/tasks.s"
.include "kernel/mmu.s"
.include "kernel/irq.s"
.include "kernel/shared.s"
.include "io/io_p0.s"
.include "drivers/storage.s"    ; The storage task (page 0 part)
.include "drivers/sound.s"
.include "kernel/os_main.s"
.include "kernel/page0_gates.s"

; Paged ROM bank 1: the hardware test, a program of its own (it takes the machine over).  Its own scope: it
; calls nothing in the BIOS ROM.
.scope HWTEST
.include "hwtest/hwtest.s"
.endscope
HWT_ENTRY       = HWTEST::HWT_ENTRY     ; (_M_HWT_ENTER's jump: include/hwtest.inc)

