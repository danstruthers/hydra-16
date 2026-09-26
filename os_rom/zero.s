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
ZP_SER_SEND_STATUS:     ; serial driver: TX ready (valid in the serial task's ZP)
    .res  1
ZP_SER_CAPTURE:         ; serial driver: task that receives serial input (valid in the serial task's ZP)
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

; FAR CALLS (cross-ROM-page calls, see common.s)
ZP_FAR_A:               ; .A passed to / returned from the far routine
    .res  1
ZP_FAR_VEC:             ; far routine address
    .res  2
ZP_FAR_PAGE:            ; ROM page (W) of the far routine
    .res  1

; TASK_CALL (run a routine in another task, see tasks.s)
ZP_TC_VEC:              ; routine address
    .res  2
ZP_TC_TASK:             ; task to run it in
    .res  1
ZP_TC_A:                ; register / flag transfer between tasks
    .res  1
ZP_TC_X:
    .res  1
ZP_TC_Y:
    .res  1
ZP_TC_P:
    .res  1
ZP_TC_FROM:             ; calling task
    .res  1

; IRQ DISPATCH / REGISTRATION (see irq.s)
ZP_IRQ_NUM:             ; logical IRQ# (or S/W interrupt #) being dispatched
    .res  1
ZP_IRQ_TMP:             ; table offset scratch
    .res  1
ZP_IRQ_CNT:             ; registration: slots to search
    .res  1
ZP_IRQ_HOME:            ; replication: home task (kept in task 0's ZP)
    .res  1
ZP_IRQ_H:               ; registration: handler address
    .res  2

; DRIVERS
ZP_DRV_PTR:             ; DriverInfo pointer
    .res  2

; SCHEDULER (see tasks.s)
ZP_TASK_ENTRY:          ; task entry point (TASK_TRAMPOLINE)
    .res  2
ZP_TASK_PAGE:           ; ROM page of the entry point
    .res  1
ZP_NO_PREEMPT:          ; NO_PREEMPT nesting count
    .res  1
ZP_PREEMPT_DUE:         ; a task switch came due while NO_PREEMPT was held
    .res  1
ZP_TC_GUEST:            ; > 0 while running a TASK_CALL routine for another task
    .res  1
ZP_IRQ_RESCHED:         ; an IRQ handler asked for a task switch
    .res  1
ZP_SCHED_CNT:           ; SCHED_PICK loop count
    .res  1

; IO (see io.s)
ZP_IO_BUF:              ; caller's buffer / name (IO_OPEN, IO_READ, IO_WRITE, IO_STAT)
    .res  2
ZP_IO_CNT:              ; byte count: requested (in), done (out)
    .res  2
ZP_IO_OFS:              ; offset (IO_SEEK)
    .res  4
ZP_IO_FD:               ; fd being worked on
    .res  1
ZP_IO_MODE:             ; open mode
    .res  1
ZP_IO_XFER:             ; the current task's transfer area (request block)
    .res  2
ZP_IO_DATA:             ; the current task's transfer area data
    .res  2
ZP_IO_LEFT:             ; bytes still to transfer
    .res  2
ZP_IO_CHUNK:            ; this transfer's size (<= IO_UNIT)
    .res  2
ZP_IO_BYTE:             ; one-byte buffer (IO_GETC / IO_PUTC)
    .res  1
ZP_IO_TMP:              ; scratch
    .res  1
ZP_IO_REQ:              ; server side: the client's request block (IO_SRV_MAP)
    .res  2
ZP_IO_SAVEB:            ; server side: RAM bank / U before IO_SRV_MAP
    .res  1
ZP_IO_SAVEU:
    .res  1

; MESSAGES (see msg.s)
ZP_MSG_PTR:             ; ring byte pointer
    .res  2
ZP_MSG_IDX:             ; ring index: receiver * 16 + sender
    .res  1
ZP_MSG_BYTE:            ; byte being sent / received
    .res  1

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
ZP_M_BM:                ; bitmap pointer (allocation map)
    .res  2
ZP_M_BE:                ; bitmap pointer (run-end map)
    .res  2
ZP_M_CNT:               ; bitmap run length wanted
    .res  1
ZP_M_RUN:               ; bitmap run length found / scratch
    .res  1
ZP_M_LO:                ; bitmap lowest / highest bit bound / scratch
    .res  1
ZP_M_MODS:              ; installed RAM modules, bit m = module m (banks m*16 - m*16+15)
    .res  2
ZP_M_HP:                ; handle table entry pointer
    .res  2
ZP_M_HANDLE:            ; handle being worked on
    .res  1
ZP_M_CP:                ; chunk page pointer (low byte always 0)
    .res  2
ZP_M_CPREV:             ; chunk page pointer: previous page in a class list (low byte always 0)
    .res  2
ZP_M_CLS:               ; chunk size class index
    .res  1
ZP_M_CSZ:               ; chunk size
    .res  1
ZP_M_COFS:              ; chunk offset in its page
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
ZP_D_PAGE:              ; ROM page (W) the disassembler reads $E000-$FDFF from (0 = BIOS, set by TASKS_INIT)
    .res    1

.feature org_per_seg
.segment "STACK"