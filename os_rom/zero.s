.debuginfo
.zeropage
RAM_BANK_REG:
    .res 1
ROM_BANK_REG:
    .res 1
STACK_SAVE_REG:
    .res 1
TASK_STATUS_REG:
    .res 1
TASK_PARENT:
    .res 1
TASK_SAVE_REG:
    .res 1
    .res 10
ZP_READ_PTR:
    .res 1
ZP_WRITE_PTR:
    .res 1
ZP_SER_SEND_STATUS:
    .res 1
ZP_TEMP:
    .res 1
ZP_TEMP_2:
    .res 1
ZP_SPI_DATA_IN:
    .res 1
ZP_SPI_DATA_OUT:
    .res 1
ZP_TEMP_VEC:
    .res 2
ZP_A_SAVE:
    .res 1
ZP_X_SAVE:
    .res 1
ZP_Y_SAVE:
    .res 1
ZP_T_SAVE:
    .res 1
ZP_U_SAVE:
    .res 1
ZP_V_SAVE:
    .res 1
ZP_W_SAVE:
    .res 1

.segment "STACK"
