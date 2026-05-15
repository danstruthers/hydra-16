.debuginfo

.zeropage
RAM_BANK_REG:
    .res  1
ROM_BANK_REG:
    .res  1
STACK_SAVE_REG:
    .res  1
TASK_STATUS_REG:
    .res  1
TASK_PARENT:
    .res  1
TASK_SAVE_REG:
    .res  1
ZP_READ_PTR:
    .res  1
ZP_WRITE_PTR:
    .res  1
ZP_SER_SEND_STATUS:
    .res  1
ZP_TEMP:
    .res  1
ZP_TEMP_2:
    .res  1
ZP_SPI_DATA_IN:
    .res  1
ZP_SPI_DATA_OUT:
    .res  1
ZP_TEMP_VEC:
    .res  2
ZP_TEMP_VEC2:
    .res  2
ZP_TEMP_VEC3:
    .res  2
ZP_TEMP_VEC4:
    .res  2
ZP_A_SAVE:
    .res  1
ZP_X_SAVE:
    .res  1
ZP_Y_SAVE:
    .res  1
ZP_T_SAVE:
    .res  1
ZP_U_SAVE:
    .res  1
ZP_V_SAVE:
    .res  1
ZP_W_SAVE:
    .res  1

;  BIOS
ZP_HS_TEMP:
    .res  2

;  WAZMON
ZP_WM_ST:
    .res  2      ; STore address
ZP_WM_XAM:
    .res  2
ZP_WM_HVP:
    .res  2      ; Hex Value Parsing
ZP_WM_MODE:
    .res  1      ; $00=ZP_D_XAM, $7F=STOR, $AE=BLOCK ZP_D_XAM

; MMU
ZP_M_BI_START:
    .res  2
ZP_M_SP1:
ZP_M_SP1_L:
    .res  1
ZP_M_SP1_H:
    .res  1
ZP_M_SP2:
ZP_M_SP2_L:
    .res  1
ZP_M_SP2_H:
    .res  1
ZP_M_SZ1:
    .res  1
ZP_M_TEMP:
    .res  1
ZP_M_TEMP2:
    .res  1
ZP_M_SV:
    .res  1

; MATH
ZP_MATH_TEMP:           ; temp space, parse output base
    .res  4
;ZP_MATH_TEMP2:          ; temp space, parse output base
;    .res  4
;ZP_MATH_PST:            ; parse state
;    .res  1
;ZP_MATH_PB:             ; parse base
;    .res  1
;ZP_MATH_PNS:            ; parse number size
;    .res  1
;ZP_MATH_OA:             ; parse output address
;    .res  2

; DISASM
ZP_D_STATE:
    .res    1
ZP_D_EXBYTES:
    .res    1
ZP_D_INST:
    .res    3
ZP_D_MODE:
    .res   1
ZP_D_XAM:
    .res    2       ; eXAMine address
ZP_D_ICOUNT:
    .res    1

.feature org_per_seg
.segment "STACK"