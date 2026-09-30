.debuginfo

; ****************************************************************************
; BIOS ROM page 7 (W = 7): the shell (shell.s): the boot shell's start (the volumes found, one selected),
; the prompt, the file commands HyForth's shell words call, and running programs.  It runs in the shell's
; task (HyForth's), called through page 1's gates; its RAM is HyForth's (PAGE1::SHBUF ...).
;
;   This file is included inside `.scope PAGE7` (see all.s), before the rest of page 7, so the gate labels
;   below take precedence over the routines of the same name on other pages.  Page 1 reaches page 7
;   through the global aliases after the scope (SH_CD_P7, ...), as PAGE1 is assembled first.

.segment "GATES_P7"

; Gates to the IO layer (page 2)
FAR_GATE_INLINE     IO_OPEN,        PAGE2::IO_OPEN,         2
FAR_GATE_INLINE     IO_CLOSE,       PAGE2::IO_CLOSE,        2
FAR_GATE_INLINE     IO_READ,        PAGE2::IO_READ,         2
FAR_GATE_INLINE     IO_WRITE,       PAGE2::IO_WRITE,        2
FAR_GATE_INLINE     IO_STAT,        PAGE2::IO_STAT,         2
FAR_GATE_INLINE     IO_CREATE,      PAGE2::IO_CREATE,       2
FAR_GATE_INLINE     IO_REMOVE,      PAGE2::IO_REMOVE,       2
FAR_GATE_INLINE     IO_WSTAT,       PAGE2::IO_WSTAT,        2
FAR_GATE_INLINE     IO_CHDIR,       PAGE2::IO_CHDIR,        2
FAR_GATE_INLINE     IO_GETCWD,      PAGE2::IO_GETCWD,       2
FAR_GATE_INLINE     IO_SEEK,        PAGE2::IO_SEEK,         2
FAR_GATE_INLINE     IO_DUP,         PAGE2::IO_DUP,          2
FAR_GATE_INLINE     IO_DUP2,        PAGE2::IO_DUP2,         2
FAR_GATE_INLINE     IO_PIPE,        PAGE2::IO_PIPE,         2
FAR_GATE_INLINE     IO_STD_OPEN,    PAGE2::IO_STD_OPEN,     2
FAR_GATE_INLINE     IO_MOUNT,       PAGE2::IO_MOUNT,        2

; ... to page 0
FAR_GATE_INLINE     WRITE_CHAR,     ::WRITE_CHAR,           0
FAR_GATE_INLINE     TASK_RUN,       ::TASK_RUN,             0
FAR_GATE_INLINE     YIELD,          ::YIELD,                0
FAR_GATE_INLINE     CONS_SET_FG,    ::CONS_SET_FG,          0
FAR_GATE_INLINE     MM_SET_FLOOR,   ::MM_SET_FLOOR,         0
FAR_GATE_INLINE     DEV_REGISTER,   ::DEV_REGISTER_FAR,     0   ; (It reads the name on the caller's page)

; ... and to page 9 (the system's servers)
FAR_GATE_INLINE     ENV_INIT,       ::ENV_INIT_P9,          9
FAR_GATE_INLINE     CLOCK_TEXT,     ::CLOCK_TEXT_P9,        9   ; (ls -l: a stamp as a date and time)
FAR_GATE_INLINE     RTC_BOOT,       ::RTC_BOOT_P9,          9   ; (SH_BOOT: the clock chip)
FAR_GATE_INLINE     TIME_DIV8,      ::TIME_DIV8_P9,         9
FAR_JMP_GATE        MON_START,      ::MON_START,            0

; ... and to HyForth (page 1)
FAR_GATE_INLINE     COPYTORAM,      PAGE1::COPYTORAM,       1
FAR_GATE_INLINE     forth_main,     PAGE1::forth_main,      1
