.debuginfo

; ****************************************************************************
; BIOS ROM page 5 (W = 5): far pointers and references (fp.s), semaphores (sem.s) and exit statuses (exits.s).
;
;   This file is included inside `.scope PAGE5` (see all.s), before fp.s, so the gate labels below take
;   precedence over the page 0 routines of the same name for all page 5 code.  Page 5 code runs with
;   W = 5; it's entered through the compact gates on pages 0, 1, 2 and 4.

.segment "GATES_P5"

; Gates from page 5 to page 0 routines (the MMU's and the shared memory's handle tables; IRQs off)
FAR_GATE_INLINE     MM_NEW_HANDLE,  ::MM_NEW_HANDLE,        0
FAR_GATE_INLINE     MM_HANDLE_PTR,  ::MM_HANDLE_PTR,        0
FAR_GATE_INLINE     SH_NEW_HANDLE,  ::SH_NEW_HANDLE,        0
FAR_GATE_INLINE     SH_HANDLE_PTR,  ::SH_HANDLE_PTR,        0
FAR_GATE_INLINE     SH_CHECK_REF,   ::SH_CHECK_REF,         0
FAR_GATE_INLINE     SH_SET_TASK_BIT, ::SH_SET_TASK_BIT,     0

; ... and for the semaphores (sem.s)
FAR_GATE_INLINE     YIELD,          ::YIELD,                0
FAR_GATE_INLINE     TASK_WAKE_MASK, ::TASK_WAKE_MASK,       0

; ... and for the exit statuses (exits.s)
FAR_GATE_INLINE     CONS_SET_FG,    ::CONS_SET_FG,          0
FAR_JMP_GATE        TASK_EXIT_NOTED, ::TASK_EXIT_NOTED,     0
