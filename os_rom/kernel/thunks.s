.debuginfo

.segment "BIOS_THUNKS"
; Page 0's copy: TH_name, a jmp to the call (include/thunks.inc)
.macro THUNK name, addr
.assert     * = addr, lderror, .sprintf("The thunk %s has moved", .string(name))
.ident(.concat("TH_", .string(name))):
                jmp             name
.endmacro
.macro THUNK_P0 name, addr
            THUNK       name, addr
.endmacro

            THUNK_TABLE
.delmacro   THUNK
.delmacro   THUNK_P0
; (SHELL_CMD, the last, is a task's entry point (TASK_RUN, page 0): the command shell, HyForth running its
; stdin: Plan 9's rc -c; system())

; Gates for the calls above on other pages, and the kernel's to them (GATES_P0, before the thunks, is full)
FAR_GATE_INLINE TASK_EXITS,     PAGE5::TASK_EXITS,      5
FAR_GATE_INLINE TASK_JOIN,      PAGE5::TASK_JOIN,       5
FAR_GATE_INLINE EXIT_NOTE,      PAGE5::EXIT_NOTE,       5   ; (TASK_EXIT: 0)
FAR_GATE_INLINE EXIT_SIGNALLED, PAGE5::EXIT_SIGNALLED,  5   ; (BREAK_ENTRY: interrupt, killed)
FAR_GATE_INLINE TASK_ORPHANS,   PAGE5::TASK_ORPHANS,    5   ; (TASK_EXIT: its children's new owner, its area)
FAR_GATE_INLINE HFS_AREA_END_G, PAGE6::HFS_AREA_END,    6   ; (TASK_AREA_END's, run in the storage task)
FAR_GATE_INLINE RAM_SERVE,      ::RAM_SERVE_P9,         9   ; (/dev/ram: ram_srv.s; the shell registers it)
FAR_GATE_INLINE GPIO_SERVE,     ::GPIO_SERVE_PD,        $D  ; (/dev/gpio: gpio_srv.s; the shell registers it)
FAR_GATE_INLINE IT_RAW_T0,      PAGE4::IT_RAW_T0,       4   ; (The IO self test's step in task 0: io_test.s)
FAR_GATE_INLINE SHELL_CMD,      ::SH_CMDSHELL_P7,       7
FAR_GATE_INLINE YM_BEEP,        PAGEB::YM_BEEP,         $B  ; (The console bell, page B: beep.s, for the serial driver)

