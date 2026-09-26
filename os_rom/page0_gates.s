.debuginfo

; ****************************************************************************
; Gates from BIOS ROM page 0 to page 1 routines (HyForth and the disassembler, see page1.s)
; .A, .X, .Y and C pass through to the routine and back (see FAR_CALL_A in common.s).

.segment "GATES_P0"

FAR_GATE        forth_main,     PAGE1::forth_main,      1
FAR_GATE        COPYTORAM,      PAGE1::COPYTORAM,       1
FAR_GATE        DISASM,         PAGE1::DISASM,          1
FAR_GATE        DISASM_AY,      PAGE1::DISASM_AY,       1
FAR_GATE        DISASM_WM,      PAGE1::DISASM_WM,       1
FAR_GATE        MMU_TEST,       PAGE1::MMU_TEST,        1