.debuginfo

; ****************************************************************************
; BIOS ROM page 8 (W = 8): the text editor (edit.s), a ROM program: the shell starts it in a task of its own
; (edit file: page 7's SH_EDIT), and waits for it.
;
;   This file is included inside `.scope PAGE8` (see all.s), before the rest of page 8, so the gate labels
;   below take precedence over the routines of the same name on other pages.

.segment "GATES_P8"

; Gates to the IO layer (page 2)
FAR_GATE_INLINE     IO_OPEN,        PAGE2::IO_OPEN,         2
FAR_GATE_INLINE     IO_CLOSE,       PAGE2::IO_CLOSE,        2
FAR_GATE_INLINE     IO_READ,        PAGE2::IO_READ,         2
FAR_GATE_INLINE     IO_WRITE,       PAGE2::IO_WRITE,        2
FAR_GATE_INLINE     IO_CREATE,      PAGE2::IO_CREATE,       2

; ... and to page 0
FAR_GATE_INLINE     WRITE_CHAR,     ::WRITE_CHAR,           0
FAR_GATE_INLINE     WRITE_BYTE,     ::WRITE_BYTE,           0
FAR_GATE_INLINE     GET_CHAR,       ::GET_CHAR,             0
FAR_GATE_INLINE     MM_SET_FLOOR,   ::MM_SET_FLOOR,         0
FAR_GATE_INLINE     TASK_SET_BREAK, ::TASK_SET_BREAK,       0
