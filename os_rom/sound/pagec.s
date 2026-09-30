.debuginfo

; ****************************************************************************
; BIOS ROM page C (W = $C): the song player (player.s), a ROM program the shell starts in a task of its own
; (run.s: SH_SONG).  It's a client of /dev/snd, like any program, so its gates are the IO calls.
;
;   This file is included inside `.scope PAGEC` (see all.s), before the rest of page C, so the gate labels
;   below take precedence over the routines of the same name on other pages.

.segment "GATES_PC"

; Gates to page 0
FAR_GATE_INLINE     MM_SET_FLOOR,   ::MM_SET_FLOOR,         0
FAR_GATE_INLINE     TICKS_GET,      ::TICKS_GET,            0
FAR_GATE_INLINE     TASK_SLEEP_UNTIL, ::TASK_SLEEP_UNTIL,   0
FAR_GATE_INLINE     YIELD,          ::YIELD,                0   ; (Waiting for the sound clock)

; ... to the IO layer (page 2)
FAR_GATE_INLINE     IO_OPEN,        PAGE2::IO_OPEN,         2
FAR_GATE_INLINE     IO_CLOSE,       PAGE2::IO_CLOSE,        2
FAR_GATE_INLINE     IO_READ,        PAGE2::IO_READ,         2
FAR_GATE_INLINE     IO_WRITE,       PAGE2::IO_WRITE,        2
FAR_GATE_INLINE     IO_SEEK,        PAGE2::IO_SEEK,         2
FAR_GATE_INLINE     IO_CTL,         PAGE2::IO_CTL,          2

; ... and to the exit statuses (page 5)
FAR_GATE_INLINE     TASK_EXITS,     PAGE5::TASK_EXITS,      5
