.debuginfo

; ****************************************************************************
; Gates from BIOS ROM page 0 to page 1 routines (HyForth and the disassembler, see page1.s), and to the
; self tests on page 4 (see page4.s)
; .A, .X, .Y and C pass through to the routine and back (see FAR_CALL_A in common.s).  Compact
; (FAR_GATE_INLINE) gates, to save page 0 space.

.segment "GATES_P0"

FAR_GATE_INLINE forth_main,     PAGE1::forth_main,      1
FAR_GATE_INLINE COPYTORAM,      PAGE1::COPYTORAM,       1
FAR_GATE_INLINE DISASM,         PAGE1::DISASM,          1
FAR_GATE_INLINE DISASM_AY,      PAGE1::DISASM_AY,       1
FAR_GATE_INLINE DISASM_WM,      PAGE1::DISASM_WM,       1
FAR_GATE_INLINE MMU_TEST,       PAGE4::MMU_TEST,        4
FAR_GATE_INLINE SCHED_TEST,     PAGE4::SCHED_TEST,      4
FAR_GATE_INLINE POST,           PAGE4::POST,            4

; Far pointers and references (page 5): the calls, and the MMU's and shared memory's hooks for references
FAR_GATE_INLINE FP_MAKE,        PAGE5::FP_MAKE,         5
FAR_GATE_INLINE FP_READ,        PAGE5::FP_READ,         5
FAR_GATE_INLINE FP_WRITE,       PAGE5::FP_WRITE,        5
FAR_GATE_INLINE FP_COPY,        PAGE5::FP_COPY,         5
FAR_GATE_INLINE MM_REF,         PAGE5::MM_REF,          5
FAR_GATE_INLINE MM_FP,          PAGE5::MM_FP,           5
FAR_GATE_INLINE SH_REF,         PAGE5::SH_REF,          5
FAR_GATE_INLINE SH_FP,          PAGE5::SH_FP,           5
FAR_GATE_INLINE MM_ENTRY_FP,    PAGE5::MM_ENTRY_FP,     5
FAR_GATE_INLINE MM_REF_LOCK,    PAGE5::MM_REF_LOCK,     5
FAR_GATE_INLINE SH_REF_LOAD,    PAGE5::SH_REF_LOAD,     5
