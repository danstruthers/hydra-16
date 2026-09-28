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
ZP_TICK_T:              ; the fast tick handler (VIA_IRQ_FAST, in the system task's ZP): the interrupted
    .res  1             ;   task and ROM page
ZP_TICK_W:
    .res  1
ZP_TC_HOLD:             ; the calling task holds NO_PREEMPT: so does the call (TC_GO)
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
ZP_TASK_OWNER:          ; the task that started this one ($FF: none; a break kills the foreground task's)
    .res  1
ZP_BREAK_VEC:           ; break handler (TASK_SET_BREAK; high byte 0: none), its ROM page and stack pointer
    .res  2
ZP_BREAK_PAGE:
    .res  1
ZP_BREAK_SP:
    .res  1
ZP_NO_PREEMPT:          ; NO_PREEMPT nesting count
    .res  1
ZP_PREEMPT_DUE:         ; a task switch came due while NO_PREEMPT was held
    .res  1
ZP_TC_GUEST:            ; > 0 while running a TASK_CALL routine for another task
    .res  1
ZP_TC_WAITERS:          ; tasks waiting to TASK_CALL this one while it's busy with another's call (bit = task)
    .res  2
ZP_SLEEP_UNTIL:         ; TASK_SLEEP: the tick count to wake at
    .res  2
ZP_SLEEPERS:            ; tasks in TASK_SLEEP (bit = task; valid in the system task: its tick handler
    .res  2             ;   wakes them, SLEEP_CHECK)
ZP_SLEEP_SCAN:          ; SLEEP_CHECK: the sleepers still to look at
    .res  2

; FAR POINTERS (see fp.s)
ZP_FP:                  ; the far pointer register (FarPtr): FP_MAKE fills it, FP_READ / FP_WRITE / FP_COPY use it
    .tag  FarPtr
ZP_FP_DST:              ; FP_COPY: the destination, bytes to copy, string mode (bit 0)
    .res  2
ZP_FP_N:
    .res  1
ZP_FP_MODE:
    .res  1
ZP_FP_SAVE:             ; FP_MAP: the RAM bank, paged ROM bank and U it replaced
    .res  3
ZP_IRQ_RESCHED:         ; an IRQ handler asked for a task switch
    .res  1
ZP_IN_SCHED:            ; non-zero: this task is in SCHED_SWITCH (IRQs can come in during SCHED_PICK, but
    .res  1             ;   they mustn't start another switch: SCHED_CAN_PREEMPT)
ZP_SCHED_CNT:           ; SCHED_PICK loop count
    .res  1
ZP_TICKS:               ; the tick count (valid in the system task: VIA_IRQ_HANDLER; TICKS_GET)
    .res  2
ZP_SIG_TARGET:          ; TASK_SIGNAL (IRQs off): the task signalled, the task being checked, and the
    .res  1             ;   owner links still to follow
ZP_SIG_TASK:
    .res  1
ZP_SIG_CNT:
    .res  1
ZP_PROC_IDX:            ; /dev/proc's server (in the client's task): the text's length, a task's owner,
    .res  1             ;   the foreground task, and the request's count
ZP_PROC_OWN:
    .res  1
ZP_PROC_FG:
    .res  1
ZP_PROC_LEN:
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
ZP_OUT_CNT:             ; bytes in the task's stdout buffer (STDOUT_BUF; STDOUT_PUT)
    .res  1
ZP_OUT_LINE:            ; bit 7: stdout is the console, so the buffer is written out at each LF too
    .res  1
ZP_IN_POS:              ; the task's stdin read-ahead (STDIN_BUF; STDIN_GET): the next byte, and the
    .res  1             ;   bytes in it
ZP_IN_CNT:
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
ZP_M_BAD_MODS:          ; RAM modules that failed the POST line tests (post_ram.s; task 0): not used
    .res  2
ZP_M_BAD_SH:            ; shared RAM chips that failed the POST (task 0), bit c = bank IDs 4c - 4c+3 of every U
    .res  1
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

; Serial driver task ZP (valid in the serial task; see drivers/serial.s).  Here, so page 2 (ser_srv.s) sees them
; as zero page addresses.  The rings (SER_RX_BUF, SER_TX_BUF) are empty when head = tail.
TASK_ZP_BEGIN
TASK_ZP     SER_RX_HEAD, 1          ; RX ring: next byte in (the IRQ handler)
TASK_ZP     SER_RX_TAIL, 1          ;   next byte out (reads)
TASK_ZP     SER_TX_HEAD, 1          ; TX ring: next byte in (writes)
TASK_ZP     SER_TX_TAIL, 1          ;   next byte out (the IRQ handler)
TASK_ZP     SER_RD_WAIT, 2          ; Tasks waiting to read (bit = task), woken when a byte arrives
TASK_ZP     SER_WR_WAIT, 2          ; Tasks waiting to write, woken when the TX ring has room (or the
                                    ;   foreground changes: background tasks wait to write to /dev/cons)
TASK_ZP     SER_PREFIX, 1           ; Non-zero: the console prefix key came, the next key is a command
TASK_ZP     SER_RATE, 1             ; The port's settings (SER_CONFIG): the baud rate (SER_RATE_*)
TASK_ZP     SER_FORMAT, 1           ;   the character format (SER_FMT_*)
TASK_ZP     SER_BIT_CYC, 2          ;   a bit's time in CPU cycles
TASK_ZP     SER_T2_CHAR, 2          ;   a character's time (and a bit's margin): WDC ACIA pacing (VIA timer 2)
TASK_ZP     SER_IRQ_W, 1            ; The fast ACIA handler (SER_IRQ_FAST): the interrupted ROM page
TASK_ZP     SER_IRQ_T, 1            ;   and task
TASK_ZP     SER_PEND, 1             ;   what it left for the driver's handler (SER_PEND_*: SER_DO_PENDING)
TASK_ZP     SER_PEND_KEY, 1         ;   the console command key (SER_PEND_CMD)
TASK_ZP_END

; Storage task ZP (valid in the storage task: SPI, the SD card and its server; spi.s, sd.s, sd_srv.s)
TASK_ZP_BEGIN
TASK_ZP     SPI_PORT, 1             ; Port B for the selected device (/CS enable, device, MOSI high)
TASK_ZP     SPI_IN, 1               ; The byte coming in
TASK_ZP     SPI_OUT, 1              ; The byte going out
TASK_ZP     SD_DEV, 1               ; The card (SPI device 0-7) SD_INIT, SD_READ_BLOCK and SD_WRITE_BLOCK use
TASK_ZP     SD_R1, 1                ; The card's last answer (for diagnosis)
TASK_ZP     SD_TMP, 1
TASK_ZP     SD_COUNT, 2             ; Tries left
TASK_ZP     SD_ARG, 4               ; A command's argument, MSB first
TASK_ZP     SD_LBA, 4               ; Block number (for SD_READ_BLOCK / SD_WRITE_BLOCK)
TASK_ZP     SD_BUF, 2               ;   and its 512 bytes
TASK_ZP     SD_CACHE, 2             ; The block cache (512 bytes)
TASK_ZP     SD_CBLOCK, 4            ;   the block in it
TASK_ZP     SD_CCARD, 1             ;   its card
TASK_ZP     SD_CVALID, 1            ;   <> 0: it's there
TASK_ZP     SD_CLIENT, 1            ; The server: the request's task
TASK_ZP     SD_FID, 1               ;   its fid (SD_FID_* | the card)
TASK_ZP     SD_OP, 1                ;   H9_READ or H9_WRITE
TASK_ZP     SD_POS, 4               ;   the offset
TASK_ZP     SD_LEFT, 2              ;   bytes left
TASK_ZP     SD_DONE, 1              ;   bytes done (0-255; 256 when finished: then 0)
TASK_ZP     SD_N, 2                 ;   bytes in this block
TASK_ZP     SD_SRC, 2               ;   the cache, and ...
TASK_ZP     SD_DST, 2               ;   the data area, where this block's bytes go
TASK_ZP_END

; Sound driver task ZP: the sound task's (SND_PLAYER), and the test tune's (in its player task; snd_test.s)
TASK_ZP_BEGIN
TASK_ZP     SND_PLAYER, 1           ; The player task (the test tune, in the background; $FF: none)
TASK_ZP     YMN0L, 4
YMN0H = YMN0L + 1
YMN1L = YMN0L + 2
YMN1H = YMN0L + 3

TASK_ZP     AZP0L, 4
AZP0H = AZP0L + 1
YMTMP1 = AZP0L + 2
YMTMP2 = AZP0L + 3
TASK_ZP_END

.feature org_per_seg
.segment "STACK"