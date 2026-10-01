.debuginfo

; ****************************************************************************
; BIOS ROM page 9 (W = 9): the system's file servers that run in their client's task: /dev/proc (the tasks,
; proc_srv.s) and /env (each task's environment, env_srv.s).  The IO layer reaches them through page 0's
; gates (their serve routines: io_p0.s).
;
;   This file is included inside `.scope PAGE9` (see all.s), before the rest of page 9, so the gate labels
;   below take precedence over the routines of the same name on other pages.

.segment "GATES_P9"

; Gates to page 0
FAR_GATE_INLINE     IO_SRV_MAP,     ::IO_SRV_MAP,           0
FAR_GATE_INLINE     IO_SRV_UNMAP,   ::IO_SRV_UNMAP,         0
FAR_GATE_INLINE     CONS_SET_FG,    ::CONS_SET_FG,          0
FAR_GATE_INLINE     TASK_SIGNAL,    ::TASK_SIGNAL,          0
FAR_GATE_INLINE     TASK_CALL,      ::TASK_CALL,            0
FAR_GATE_INLINE     NO_PREEMPT,     ::NO_PREEMPT,           0
FAR_GATE_INLINE     PREEMPT,        ::PREEMPT,              0
FAR_GATE_INLINE     TASK_SLEEP,     ::TASK_SLEEP,           0   ; (The clock chip's probe: rtc.s)

; ... and to the IO layer (page 2)
FAR_GATE_INLINE     IO_SRV_COUNT,   PAGE2::IO_SRV_COUNT,    2
FAR_GATE_INLINE     STAT_ZERO,      PAGE2::STAT_ZERO,       2
