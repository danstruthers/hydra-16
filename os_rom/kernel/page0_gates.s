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
FAR_GATE_INLINE POST_RAM_TEST,  PAGE4::POST_RAM_TEST,   4