.debuginfo

; ****************************************************************************
; Kernel: task numbers, error codes, the system's shared bank, task ZP allocation, the scheduler's tick

SYSTEM_TASK_NUM     = 0
SHELL_TASK_NUM      = $01       ; Forth / WOZMON shell (the default serial-capture task)
PIPE_TASK_NUM       = $0D       ; Pipe server (resident)
STORAGE_TASK_NUM    = $0C       ; Storage: the SD card (and later FAT32) server (resident)
SOUND_TASK_NUM      = $0E       ; Sound driver (resident)
SERIAL_TASK_NUM     = $0F       ; Serial driver (resident)
SERIAL_OWNER_TASK   = SERIAL_TASK_NUM   ; Task whose ZP holds the serial driver state

; ERROR VALUES
ERR_SUCCESS         = $00
ERR_NOT_SYSTEM_TASK = $01
ERR_OUT_OF_MEMORY   = $02

ERR_MATH_INVD       = $31       ; invalid digit
ERR_MATH_INVB       = $32       ; invalid base
ERR_MATH_OVF        = $33       ; overflow
ERR_MATH_UNF        = $34       ; underflow

ERR_MEM_NOT_ALLOC   = $40
ERR_MEM_NOT_VALID   = $41
ERR_MEM_NOT_SUPPORTED = $42
ERR_MEM_BAD_ARG     = $43
ERR_MEM_NO_HANDLES  = $44
ERR_MEM_LOCKED      = $45

ERR_IRQ_CHAIN_FULL  = $50
ERR_IRQ_NOT_FOUND   = $51

ERR_IO_NOT_FOUND    = $70       ; no such file / device
ERR_IO_BAD_FD       = $71       ; fd not open (or out of range)
ERR_IO_MODE         = $72       ; not opened for that (e.g. write on a read-only fd)
ERR_IO_WOULD_BLOCK  = $73       ; no data yet (non-blocking fd; servers return it to make the IO layer wait)
ERR_IO_EOF          = $74       ; end of file (IO_GETC)
ERR_IO_NO_FDS       = $75       ; all of the task's fds are open
ERR_IO_NO_DEVS      = $76       ; the device table is full
ERR_IO_NAME         = $77       ; bad or duplicate device name
ERR_IO_BAD_REQ      = $78       ; the server doesn't support that request
ERR_IO_DEVICE       = $79       ; the device didn't respond
ERR_IO_BROKEN       = $7A       ; write to a pipe nobody reads
ERR_IO_NO_PIPES     = $7B       ; all pipes are in use
ERR_IO_NS_FULL      = $7C       ; the task's namespace is full
ERR_IO_NS_LOOP      = $7D       ; too many binds in a row (a loop?)
ERR_IO_NOT_READY    = $7E       ; the device isn't started (e.g. the SD card: open /dev/sd first)
ERR_IO_MEDIA        = $7F       ; the medium refused the command or the data (e.g. the SD card)

; ERROR CODES
ERR_NO_TASKS_AVAILABLE = $F1
ERR_TASK_BUSY       = $F2
ERR_BAD_TASK        = $F3

; max task idle
MAX_TASK_NUMBER = $0F           ; 16 tasks, numbered 0-F
NUM_RAM_MODULES = 15            ;
NUM_BANKS_PER_MODULE = 16
NUM_RAM_BANKS   = NUM_RAM_MODULES * NUM_BANKS_PER_MODULE

; The system's data in shared RAM: shared bank ID $00 (U = 0, bank $F0): the shared memory tables
; (shared.s) and the device table (below)
SYS_SHARED_U        = 0
SYS_BANK            = $F0

; Save RAM_BANK_REG and U on the stack, and map a bank at $8000 (U = u: for a shared bank $F0-$FF).
; Uses .Y.  Undo with _M_BANK_LEAVE
.macro _M_BANK_ENTER    bank, u
            ldy         RAM_BANK_REG
            phy
            ldy         U_REGISTER
            phy
.if (u) = 0
            stz         U_REGISTER
.else
            ldy         #u
            sty         U_REGISTER
.endif
            ldy         #bank
            sty         RAM_BANK_REG
.endmacro

; Restore U and RAM_BANK_REG (saved by _M_BANK_ENTER).  Uses .Y; preserves .A and C
.macro _M_BANK_LEAVE
            ply
            sty         U_REGISTER
            ply
            sty         RAM_BANK_REG
.endmacro

; The system's shared bank (shared bank ID $00)
.macro _M_SYS_ENTER
            _M_BANK_ENTER   ::SYS_BANK, ::SYS_SHARED_U      ; (:: so the .if works in the page scopes too)
.endmacro

.macro _M_SYS_LEAVE
            _M_BANK_LEAVE
.endmacro

; ***  TASK ZERO-PAGE ALLOCATION  ***
;
; OS/BIOS ZP grows up from $02 (the ZEROPAGE segment, zero.s) and has the same layout in every task.
; Task ZP grows down from $FF.  Each task image (shell, sound driver, ...) brackets its ZP with:
;
;       TASK_ZP_BEGIN
;       TASK_ZP     NAME, size      ; NAME = next free address below the previous one
;       TASK_ZP_END                 ; link-time check that task ZP doesn't collide with OS ZP
;
; Task images run in different tasks, so their task ZP may overlap each other.

.import     __ZEROPAGE_RUN__, __ZEROPAGE_SIZE__

.macro TASK_ZP_BEGIN
task_zp_top .set $100
.endmacro

.macro TASK_ZP  name, size
task_zp_top .set task_zp_top - (size)
name = task_zp_top
.endmacro

.macro TASK_ZP_END
.assert     task_zp_top >= __ZEROPAGE_RUN__ + __ZEROPAGE_SIZE__, lderror, "Task ZP overlaps OS ZP"
.endmacro

; Task switcher interrupt timer (one interrupt per 5ms or so, with 64 cycles for INT Handler overhead)
SCHED_TICK_HZ   = 200
TIMER_TASK_INT  = (CLK_CPS / SCHED_TICK_HZ) - 64           ; 17833 at 3.58 MHz
.assert     TIMER_TASK_INT < $10000, error, "Scheduler tick doesn't fit VIA T1: raise SCHED_TICK_HZ"
TIMER_TASK_INT_H = >TIMER_TASK_INT
TIMER_TASK_INT_L = <TIMER_TASK_INT
