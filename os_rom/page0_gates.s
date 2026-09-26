.debuginfo

; ****************************************************************************
; Gates from BIOS ROM page 0 to page 1 routines (HyForth and the disassembler, see page1.s), and to the
; POST RAM tests on page 2 (post_ram.s)
; .A, .X, .Y and C pass through to the routine and back (see FAR_CALL_A in common.s).  Compact
; (FAR_GATE_INLINE) gates, to save page 0 space.

.segment "GATES_P0"

FAR_GATE_INLINE forth_main,     PAGE1::forth_main,      1
FAR_GATE_INLINE COPYTORAM,      PAGE1::COPYTORAM,       1
FAR_GATE_INLINE DISASM,         PAGE1::DISASM,          1
FAR_GATE_INLINE DISASM_AY,      PAGE1::DISASM_AY,       1
FAR_GATE_INLINE DISASM_WM,      PAGE1::DISASM_WM,       1
FAR_GATE_INLINE MMU_TEST,       PAGE1::MMU_TEST,        1
FAR_GATE_INLINE SCHED_TEST,     PAGE1::SCHED_TEST,      1
FAR_GATE_INLINE POST_RAM_TEST,  PAGE2::POST_RAM_TEST,   2